import copy
import unittest
from analyze_idm_io import analyze, TARGET


def fixture(periodic_flush=False):
    part, output, handle = r'C:\owned\DwnlData\opaque', r'C:\owned\case0001.bin', '0x40'
    events = []

    def add(api, **extra):
        events.append(dict(api=api, kind='owned-io', pid=17, sequence=len(events) + 1,
                           begin=0, end=100, tid=3, path=part, handle=handle, **extra))

    add('candidate-part-adopted')
    add('CreateFileW', disposition=2, flags=0, access=0xc0000000, share=1, ok=True)
    add('file-part-open', matchedScope=True, returnCode=0)
    add('NtWriteFile', requested=16, completed=16, ok=True, pending=False)
    add('WriteFile', requested=16, completed=16, ok=True, overlapped=False)
    if periodic_flush:
        add('NtFlushBuffersFile', ok=True)
        add('FlushFileBuffers', ok=True)
        add('file-part-flush', matchedScope=True)
        add('NtWriteFile', requested=16, completed=16, ok=True, pending=False)
        add('WriteFile', requested=16, completed=16, ok=True, overlapped=False)
    add('CloseHandle', ok=True)
    add('file-part-close', matchedScope=True)
    add('MoveFileW', ok=True)
    events[-1].update({'from': part, 'to': output})
    ready = dict(kind='observer-ready', architecture='ia32', pid=17, scope='case0001',
                 hooks=['CreateFileW', 'WriteFile', 'NtWriteFile', 'FlushFileBuffers', 'NtFlushBuffersFile',
                        'CloseHandle', 'file-part-open', 'file-part-close', 'file-part-flush', 'MoveFileW'])
    observed = dict(mode='spawn', state='detached', pid=17, source_sha256='a' * 64,
                    messages_truncated=False, cleanup_error=None,
                    messages=[dict(type='send', payload=p) for p in [ready, *events]])
    size = 32 if periodic_flush else 16
    return dict(inputs=dict(targetSha256=TARGET, agentSha256='a' * 64, token='case0001', bytes=size),
                acceptance=dict(targetBinaryUnchanged=True, targetPid=17, currentSessionIdmRemaining=[],
                                settingsRestored=True, hashMatches=True, bytes=size),
                frida=dict(result=dict(structuredContent=dict(normalized_result=observed))))


class IdmIoAnalysis(unittest.TestCase):
    def test_nested_native_writes_are_verified_not_double_counted(self):
        report = analyze(fixture())
        self.assertEqual(report['logicalBodyBytes'], 16)
        self.assertTrue(report['noObservedWriterSessionFlushCalls'])
        self.assertFalse(report['wholeApplicationFlushAbsenceProven'])
        self.assertFalse(report['kernelOrDeviceWritebackMeasured'])

    def test_real_periodic_flush_remains_visible_in_the_result(self):
        report = analyze(fixture(True))
        self.assertEqual(report['logicalBodyBytes'], 32)
        self.assertFalse(report['noObservedWriterSessionFlushCalls'])
        self.assertTrue(report['observedFlushBeforeFinalWrite'])
        self.assertFalse(report['observedFlushAfterFinalWrite'])

    def test_missing_native_write_coverage_is_rejected(self):
        data = fixture()
        rows = data['frida']['result']['structuredContent']['normalized_result']['messages']
        rows[:] = [r for r in rows if r['payload'].get('api') != 'NtWriteFile']
        with self.assertRaises(AssertionError):
            analyze(data)

    def test_truncation_and_script_errors_cannot_prove_no_flush(self):
        for key in ('truncation', 'script', 'cap'):
            data = fixture()
            obs = data['frida']['result']['structuredContent']['normalized_result']
            if key == 'truncation':
                obs['messages_truncated'] = True
            elif key == 'script':
                obs['messages'].append(dict(type='error', description='Owned script failure'))
            else:
                obs['messages'].append(dict(type='send', payload=dict(kind='capture-cap')))
            with self.subTest(key=key), self.assertRaises(AssertionError):
                analyze(data)

    def test_unscoped_opaque_part_is_rejected_despite_successful_file_receipt(self):
        data = fixture()
        for row in data['frida']['result']['structuredContent']['normalized_result']['messages']:
            if row['payload'].get('api') == 'file-part-open':
                row['payload']['matchedScope'] = False
        with self.assertRaises(AssertionError):
            analyze(data)

    def test_move_must_link_the_part_to_the_exact_owned_output(self):
        data = fixture()
        data['frida']['result']['structuredContent']['normalized_result']['messages'][-1]['payload']['to'] = r'C:\owned\other.bin'
        with self.assertRaises(AssertionError):
            analyze(data)

    def test_an_incomplete_or_different_body_cannot_pass_on_hashmatches_alone(self):
        data = fixture()
        data['inputs']['bytes'] = data['acceptance']['bytes'] = 8192
        with self.assertRaises(AssertionError):
            analyze(data)

    def test_wrong_source_or_foreign_process_is_rejected(self):
        for key in ('source', 'pid'):
            data = copy.deepcopy(fixture())
            obs = data['frida']['result']['structuredContent']['normalized_result']
            if key == 'source':
                obs['source_sha256'] = 'b' * 64
            else:
                obs['messages'][-1]['payload']['pid'] = 18
            with self.subTest(key=key), self.assertRaises(AssertionError):
                analyze(data)


if __name__ == '__main__':
    unittest.main()
