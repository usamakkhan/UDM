"""Qualify admission for independently encrypted matching proxy credentials."""
from __future__ import annotations

import argparse
import base64
import json
import socket
import subprocess
import threading
from pathlib import Path

from bench import sha256_file
from origin import Options, Origin, expected_sha256


class AccountProxy(Origin):
    def __init__(self, cap_reduction=False, cap_restore=False):
        self.cap_reduction = cap_reduction
        self.cap_restore = cap_restore
        self.cap_restored_at = None
        self.restored_accepts = []
        self.cap_reduced_at = None
        self.reduced_accepts = []
        self.refusal_events = []
        self.account_lock = threading.Lock()
        self.account_active = {"account-a": 0, "account-b": 0}
        self.account_peak = dict(self.account_active)
        self.refused = dict(self.account_active)
        self.started = {"account-a": [], "account-b": []}
        self.finished = {"account-a": [], "account-b": []}
        self.tokens = {"Basic " + base64.b64encode(f"{user}:fixture-password".encode()).decode(): user
                       for user in self.account_active}
        self.challenges = 0
        self.stalled_requests = 0
        self.independent_started_while_a_full = False
        super().__init__(Options(size=2 * 1024 * 1024, rate=256 * 1024, chunk=16 * 1024,
                                 path="http://udm-account-fixture.invalid/bench/file.bin"))

    def _respond(self, sock, connection, method, target, headers):
        user = self.tokens.get(headers.get("proxy-authorization", ""))
        if user is None:
            with self.account_lock:
                self.challenges += 1
            sock.sendall(b'HTTP/1.1 407 Proxy Authentication Required\r\n'
                         b'Proxy-Authenticate: Basic realm="UDM fixture"\r\nContent-Length: 0\r\n\r\n')
            return True
        if "stallheaders=1" in target:
            with self.account_lock:
                self.stalled_requests += 1
            sock.settimeout(0.1)
            while not self.stopping:
                try:
                    if not sock.recv(1):
                        break
                except socket.timeout:
                    pass
            return False
        with self.account_lock:
            account_a_cap = 1 if self.cap_reduced_at is not None and self.cap_restored_at is None else 2
            cap = account_a_cap if user == "account-a" else 1
            if self.account_active[user] >= cap:
                self.refused[user] += 1
                self.refusal_events.append({"at": round(self.now(), 4), "account": user,
                                            "active": self.account_active[user], "cap": cap})
                return False
            self.account_active[user] += 1
            if user == "account-b" and self.account_active["account-a"] >= account_a_cap:
                self.independent_started_while_a_full = True
            if user == "account-a" and self.cap_restored_at is not None:
                self.restored_accepts.append({"at": round(self.now(), 4), "active": self.account_active[user]})
            elif user == "account-a" and self.cap_reduced_at is not None:
                self.reduced_accepts.append({"at": round(self.now(), 4), "active": self.account_active[user]})
                if self.cap_restore:
                    self.cap_restored_at = round(self.now(), 4)
            self.account_peak[user] = max(self.account_peak[user], self.account_active[user])
            self.started[user].append(round(self.now(), 4))
        try:
            return super()._respond(sock, connection, method, target, headers)
        finally:
            with self.account_lock:
                self.account_active[user] -= 1
                self.finished[user].append(round(self.now(), 4))
                if self.cap_reduction and user == "account-a" and self.cap_reduced_at is None:
                    self.cap_reduced_at = round(self.now(), 4)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--challenge", action="store_true", help="Exercise the default Automatic authentication challenge path.")
    parser.add_argument("--header-timeout", action="store_true", help="Verify a silent response obeys one retry without learning a connection cap.")
    parser.add_argument("--pause-waiter", action="store_true", help="Pause and resume the third download while admission waits.")
    parser.add_argument("--pause-active", action="store_true", help="Pause an active file, admit its waiter, and resume from saved bytes.")
    parser.add_argument("--cap-reduction", action="store_true", help="Reduce the account cap from two to one when an active file pauses.")
    parser.add_argument("--cap-restore", action="store_true", help="Restore the cap to two after one response is accepted at the reduced limit.")
    args = parser.parse_args()
    if sum((args.header_timeout, args.pause_waiter, args.pause_active)) > 1:
        parser.error("Use separate timeout and pause/resume cases.")
    if args.cap_reduction and not args.pause_active:
        parser.error("Cap reduction requires --pause-active.")
    if args.cap_restore and not args.cap_reduction:
        parser.error("Cap restoration requires --cap-reduction.")
    executable = args.fixture.resolve(strict=True)
    root, output = args.work.resolve(), args.out.resolve()
    if output.exists() or root == output or root in output.parents or output in root.parents:
        parser.error("Use fresh independent paths.")
    root.mkdir(parents=True, exist_ok=False)
    output.parent.mkdir(parents=True, exist_ok=True)
    proxy = AccountProxy(args.cap_reduction, args.cap_restore)
    config = root / "config.json"
    config.write_text(json.dumps({"proxy": f"127.0.0.1:{proxy.port}",
                                 "authMode": "Automatic" if args.challenge else "Basic",
                                 "headerTimeout": args.header_timeout,
                                 "pauseWaiter": args.pause_waiter,
                                 "pauseActive": args.pause_active,
                                 "capReduction": args.cap_reduction,
                                 "sha256": expected_sha256(proxy.o.size, proxy.o.seed)}), encoding="utf-8")
    try:
        with (root / "native.log").open("w", encoding="utf-8") as log:
            process = subprocess.run([str(executable), str(root / "native"), "--proxy-admission", str(config)],
                                     stdout=log, stderr=subprocess.STDOUT, timeout=65)
        native = json.loads((root / "native" / "results.json").read_text(encoding="utf-8"))
        account_b_started_while_a_busy = proxy.independent_started_while_a_full
        traffic = proxy.snapshot()
        saved_range_reused = any(request["status"] == 206 and request["start"] > 0
                                 for connection in traffic["detail"] for request in connection["requests"])
        result = {"passed": (native["passed"] and native["differentCiphertexts"]
                             and native["sameCredentialKey"] and account_b_started_while_a_busy
                             and (not args.challenge or proxy.challenges >= 4)
                             and (not args.header_timeout or proxy.stalled_requests == 2)
                             and (not args.pause_active or saved_range_reused)
                             and (not args.cap_reduction or (proxy.cap_reduced_at is not None
                                  and len(proxy.reduced_accepts) >= (1 if args.cap_restore else 2)
                                  and all(item["active"] == 1 for item in proxy.reduced_accepts)))
                             and (not args.cap_restore or (proxy.cap_restored_at is not None
                                  and any(item["active"] == 2 for item in proxy.restored_accepts)))),
                  "native": native, "fixture_exit_code": process.returncode,
                  "independent_account_started_while_cap_occupied": account_b_started_while_a_busy,
                  "account_peak": proxy.account_peak, "refused": proxy.refused,
                  "started": proxy.started, "finished": proxy.finished,
                  "challenges": proxy.challenges, "origin": traffic,
                  "authentication_mode": "Automatic" if args.challenge else "Basic",
                  "header_timeout": args.header_timeout, "stalled_requests": proxy.stalled_requests,
                  "pause_waiter": args.pause_waiter,
                  "pause_active": args.pause_active, "saved_range_reused": saved_range_reused,
                  "cap_reduction": args.cap_reduction, "cap_reduced_at": proxy.cap_reduced_at,
                  "reduced_accepts": proxy.reduced_accepts,
                  "cap_restore": args.cap_restore, "cap_restored_at": proxy.cap_restored_at,
                  "restored_accepts": proxy.restored_accepts, "refusal_events": proxy.refusal_events,
                  "fixture_sha256": sha256_file(executable)}
    finally:
        proxy.close()
        for file in root.rglob("*"):
            if file.is_file() and file.suffix in (".bin", ".part", ".data"):
                if not file.resolve().is_relative_to(root):
                    raise RuntimeError("Payload escaped the fixture directory.")
                file.unlink()
    result["temporary_payloads_removed"] = True
    with output.open("x", encoding="utf-8") as receipt:
        json.dump(result, receipt, indent=2)
    print(json.dumps({"passed": result["passed"], "native": result["native"]}), flush=True)
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
