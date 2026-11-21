"""Recorder lifecycle fault tests. No real WPR start or elevation is used."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import unittest
from unittest.mock import patch, Mock
import uuid

sys.dont_write_bytecode = True
import finish_io_trace as trace


class Clock:
    def __init__(self):
        self.now = 0.0

    def __call__(self):
        return self.now

    def sleep(self, seconds):
        self.now += seconds


class Backend:
    def __init__(self, clock):
        self.clock = clock
        self.sessions = set()
        self.actions = []
        self.start_error = False
        self.stop_error = False
        self.cancel_error = False
        self.export_ok = True
        self.unknown = False

    def preflight(self, instance):
        self.actions.append(("preflight", instance))
        if self.sessions:
            raise RuntimeError("existing recording")

    def start_owned(self, instance):
        self.actions.append(("start", instance))
        self.sessions.add(instance)
        if self.start_error:
            raise RuntimeError("partial start failure")

    def stop_export(self, instance, output, stop_deadline):
        self.stop_deadline = stop_deadline
        self.actions.append(("stop", instance))
        if self.stop_error:
            raise RuntimeError("stop failed")
        self.sessions.discard(instance)
        self.clock.sleep(.1)
        return {"inactiveAt": self.clock(), "exportComplete": self.export_ok}

    def owns_instance(self, instance):
        self.actions.append(("probe", instance))
        if self.unknown:
            raise RuntimeError("unknown session state")
        return instance in self.sessions

    def cancel_owned(self, instance):
        self.actions.append(("cancel", instance))
        if self.cancel_error:
            raise RuntimeError("cancel failed")
        self.sessions.discard(instance)


class RecorderTests(unittest.TestCase):
    def setUp(self):
        self.clock = Clock()
        self.backend = Backend(self.clock)
        self.instance = "UDMFinish-" + uuid.uuid4().hex

    def run_session(self, alive=lambda: True, finish=lambda: True, ready=lambda: None):
        result = trace.record_session(self.backend, self.instance, Path("unused.etl"),
                                      alive, finish, ready, 20, self.clock, self.clock.sleep)
        self.assertTrue(all(instance == self.instance for _, instance in self.backend.actions))
        return result

    def test_complete_capture_stops_only_owned_session(self):
        result = self.run_session()
        self.assertTrue(result["captureComplete"])
        self.assertFalse(self.backend.sessions)
        self.assertNotIn(("cancel", self.instance), self.backend.actions)
        self.assertEqual(20 - trace.CANCEL_RESERVE, self.backend.stop_deadline)

    def test_slow_stop_uses_remaining_budget_without_five_second_cutoff(self):
        backend = trace.WprBackend(Path("unused"), trace.PROFILE, trace.sha(trace.PROFILE))
        process = Mock()
        process.returncode = None
        def poll():
            if self.clock() >= 12:
                process.returncode = 0
            return process.returncode
        process.poll.side_effect = poll
        process.communicate.return_value = ("", "")
        output = Mock()
        output.is_file.return_value = True
        output.stat.return_value.st_size = 1
        with patch.object(trace.subprocess, "Popen", return_value=process), \
             patch.object(trace.time, "monotonic", self.clock), \
             patch.object(trace.time, "sleep", self.clock.sleep), \
             patch.object(backend, "owns_instance", side_effect=lambda _: self.clock() < 10):
            result = backend.stop_export(self.instance, output, 18)
        self.assertTrue(result["exportComplete"])
        self.assertGreaterEqual(result["inactiveAt"], 10)
        process.kill.assert_not_called()

    def test_stop_deadline_still_terminates_only_owned_export_child(self):
        backend = trace.WprBackend(Path("unused"), trace.PROFILE, trace.sha(trace.PROFILE))
        process = Mock()
        process.poll.return_value = None
        process.communicate.return_value = ("", "")
        with patch.object(trace.subprocess, "Popen", return_value=process), \
             patch.object(trace.time, "monotonic", self.clock), \
             patch.object(trace.time, "sleep", self.clock.sleep), \
             patch.object(backend, "owns_instance", return_value=True):
            with self.assertRaisesRegex(TimeoutError, "cleanup budget"):
                backend.stop_export(self.instance, Mock(), 6)
        process.kill.assert_called_once()
        self.assertTrue(backend.commands[-1]["timedOut"])

    def test_controller_exited_before_start_does_nothing(self):
        result = self.run_session(alive=lambda: False)
        self.assertFalse(result["startAttempted"])
        self.assertEqual([], self.backend.actions)

    def test_existing_recording_is_never_stopped(self):
        self.backend.sessions.add("somebody-else")
        result = self.run_session()
        self.assertFalse(result["startAttempted"])
        self.assertEqual({"somebody-else"}, self.backend.sessions)
        self.assertEqual([("preflight", self.instance)], self.backend.actions)

    def test_partial_start_is_cleaned_by_exact_instance(self):
        self.backend.start_error = True
        result = self.run_session()
        self.assertFalse(result["captureComplete"])
        self.assertTrue(result["recordingStopped"])
        self.assertIn(("cancel", self.instance), self.backend.actions)

    def test_controller_loss_after_ready_still_stops_recording(self):
        state = {"alive": True}
        result = self.run_session(alive=lambda: state["alive"], finish=lambda: False,
                                  ready=lambda: state.update(alive=False))
        self.assertEqual("controller-exited", result["stopReason"])
        self.assertTrue(result["recordingStopped"])
        self.assertFalse(result["captureComplete"])

    def test_deadline_reserves_cleanup_and_marks_capture_incomplete(self):
        result = self.run_session(finish=lambda: False)
        self.assertEqual("deadline", result["stopReason"])
        self.assertTrue(result["bounded"])
        self.assertLessEqual(result["recordingObservedSeconds"], 20)
        self.assertFalse(result["captureComplete"])

    def test_failed_stop_uses_only_named_cancel(self):
        self.backend.stop_error = True
        result = self.run_session()
        self.assertTrue(result["recordingStopped"])
        self.assertFalse(result["captureComplete"])
        self.assertIn(("cancel", self.instance), self.backend.actions)

    def test_failed_export_is_not_reported_as_complete(self):
        self.backend.export_ok = False
        result = self.run_session()
        self.assertTrue(result["recordingStopped"])
        self.assertFalse(result["exportComplete"])
        self.assertFalse(result["captureComplete"])

    def test_unknown_cleanup_never_claims_stopped(self):
        self.backend.stop_error = self.backend.cancel_error = self.backend.unknown = True
        result = self.run_session()
        self.assertFalse(result["recordingStopped"])
        self.assertFalse(result["bounded"])
        self.assertFalse(result["captureComplete"])

    def test_actual_overrun_is_not_a_pass(self):
        result = self.run_session(ready=lambda: self.clock.sleep(25))
        self.assertFalse(result["bounded"])
        self.assertFalse(result["captureComplete"])

    def test_ready_notification_failure_still_cleans_up(self):
        def failure():
            raise OSError("status write failed")
        result = self.run_session(ready=failure)
        self.assertTrue(result["recordingStopped"])
        self.assertFalse(result["captureComplete"])

    def test_invalid_identity_cannot_target_an_existing_recorder(self):
        with self.assertRaises(ValueError):
            trace.record_session(self.backend, "WprApp", Path("unused.etl"),
                                  lambda: True, lambda: True, lambda: None)
        self.assertFalse(self.backend.actions)

    def test_elevated_controller_cannot_launch_a_workload(self):
        with patch.object(trace, "is_admin", return_value=True):
            with self.assertRaisesRegex(RuntimeError, "controller unelevated"):
                trace.controller(None)

    def test_actual_profile_is_validated_by_hash(self):
        self.assertEqual(trace.PROFILE, trace.validate_profile(trace.PROFILE, trace.sha(trace.PROFILE)))
        with self.assertRaises(ValueError):
            trace.validate_profile(trace.PROFILE, "0" * 64)

    def test_extra_kernel_capture_is_rejected_even_with_matching_hash(self):
        content = trace.PROFILE.read_bytes().replace(b'<Keyword Value="FileIO" />',
                   b'<Keyword Value="FileIO" /><Keyword Value="Network" />')
        with patch.object(Path, "read_bytes", return_value=content):
            with self.assertRaisesRegex(ValueError, "kernel keywords"):
                trace.validate_profile(trace.PROFILE, hashlib.sha256(content).hexdigest().upper())

    def test_unexpected_inherited_profile_is_rejected(self):
        content = trace.PROFILE.read_bytes().replace(b'Base="UDMFinish.Verbose.File"',
                                                    b'Base="GeneralProfile.Verbose.File"')
        with patch.object(Path, "read_bytes", return_value=content):
            with self.assertRaisesRegex(ValueError, "inherited profile"):
                trace.validate_profile(trace.PROFILE, hashlib.sha256(content).hexdigest().upper())

    def test_cleanup_supervisor_retries_owned_instance_until_confirmed_off(self):
        self.backend.sessions.update([self.instance, "another-recording"])
        self.backend.cancel_error = True
        notifications = []
        def unblock(seconds):
            self.clock.sleep(seconds)
            self.backend.cancel_error = False
        trace.recover_owned(self.backend, self.instance, lambda: notifications.append("attention"), unblock)
        self.assertEqual({"another-recording"}, self.backend.sessions)
        self.assertEqual(["attention"], notifications)
        self.assertEqual(2, self.backend.actions.count(("cancel", self.instance)))

    @unittest.skipUnless(os.name == "nt", "Windows process identity test")
    def test_real_child_handle_detects_exit(self):
        child = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
        try:
            with trace.PinnedProcess(child.pid) as pinned:
                self.assertTrue(pinned.alive())
                child.terminate()  # Only this test's Popen-owned helper.
                child.wait(timeout=5)
                self.assertFalse(pinned.alive())
        finally:
            if child.poll() is None:
                child.kill()
                child.wait(timeout=5)

    @unittest.skipUnless(os.name == "nt", "Windows process identity test")
    def test_creation_mismatch_rejects_pid_only_ownership(self):
        with trace.PinnedProcess(os.getpid()) as pinned:
            with self.assertRaisesRegex(RuntimeError, "identity changed"):
                trace.PinnedProcess(os.getpid(), str(int(pinned.creation) + 1))


if __name__ == "__main__":
    report = Path(sys.argv.pop(1)) if len(sys.argv) > 1 else None
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(RecorderTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if report:
        report.write_text(json.dumps({"passed": result.wasSuccessful(),
                          "tests": result.testsRun, "failures": len(result.failures),
                          "errors": len(result.errors), "skipped": len(result.skipped),
                          "realWprStarted": False, "elevationRequested": False,
                          "method": "Injected recorder lifecycle faults plus real owned child process handle/creation-time checks. Not a privileged live ETW capture."}, indent=2) + "\n")
    raise SystemExit(0 if result.wasSuccessful() else 1)
