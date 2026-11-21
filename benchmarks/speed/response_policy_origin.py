"""Deterministic hostile-response policies for isolated UDM qualification.

The Origin transport owns sockets, pacing and request accounting. Each response
snapshots its version's bytes, so changing a later response never mutates an
already accepted full body in flight. No payload files are created by the server.
"""
import socket
import time

from origin import BLOCK, Origin, Options, Request, pattern_block


class ResponsePolicyOrigin(Origin):
    policies = {
        "range", "explicit-none", "omitted-ignore", "advertised-ignore",
        "one-use-bytes", "one-use-omitted", "no-validator-once", "capacity-one",
        "bad-helper-range", "bad-helper-etag", "truncated-helper", "resource-change",
    }

    def __init__(self, policy, *, size=8 * 1024**2, seed=20261113, checkpoint_gate=None):
        if policy not in self.policies:
            raise ValueError("Unknown isolated response policy")
        self.policy = policy
        self.checkpoint_gate = checkpoint_gate
        self.sequence = 0
        self.policies_sent = []
        self.versions = {1: pattern_block(seed), 2: pattern_block(seed + 1)}
        super().__init__(Options(size=size, seed=seed, rate=8_000_000,
                                 max_conns=1 if policy == "capacity-one" else 0,
                                 latency_ms=10))

    def _serve(self, sock, conn):
        # Keep the primary body alive long enough for helper admission/splitting.
        if conn.id == 1:
            conn.rate = 1_000_000
        return super()._serve(sock, conn)

    def _respond(self, sock, conn, method, target, headers):
        if target.split("?", 1)[0] != self.o.path:
            return super()._respond(sock, conn, method, target, headers)
        with self.lock:
            self.sequence += 1
            number = self.sequence
        if self.policy == "resource-change" and number >= 3 and self.checkpoint_gate:
            deadline = time.monotonic() + 12
            while not self.checkpoint_gate.is_file():
                if time.monotonic() >= deadline:
                    raise AssertionError("Old checkpoint capture was not established")
                time.sleep(0.005)
        time.sleep(self.o.latency_ms / 1000)
        rng = headers.get("range")
        version = 2 if self.policy == "resource-change" and number >= 3 else 1
        if self.policy == "bad-helper-etag" and rng:
            version = 2
        tag = f'"udm-response-v{version}"'
        if self.policy.startswith("one-use") or self.policy == "no-validator-once":
            if number > 1:
                sock.sendall(b"HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: close\r\n\r\n")
                with self.lock:
                    conn.requests.append(Request(self.now(), method, target, rng, 403,
                                                 0, -1, outcome="one-use-refusal"))
                    self.policies_sent.append({"number": number, "policy": self.policy,
                                               "status": 403, "version": version})
                return False

        start, end, status = 0, self.o.size - 1, 200
        ignore = self.policy in {"explicit-none", "omitted-ignore", "advertised-ignore",
                                 "no-validator-once"}
        # A real representation change honors If-Range by returning the new full
        # response. The bad-helper-etag policy deliberately violates that rule.
        if self.policy == "resource-change" and headers.get("if-range") not in {None, tag}:
            ignore = True
        if rng and not ignore:
            text = rng.removeprefix("bytes=")
            left, _, right = text.partition("-")
            start, end = int(left), int(right) if right else self.o.size - 1
            if not 0 <= start <= end < self.o.size:
                raise AssertionError("Fixture received an invalid range")
            status = 206

        advertised = "bytes"
        if self.policy == "explicit-none":
            advertised = "none"
        if self.policy in {"omitted-ignore", "one-use-omitted"}:
            advertised = ""
        length = end - start + 1
        lines = [f"HTTP/1.1 {status} {'Partial Content' if status == 206 else 'OK'}",
                 "Content-Type: application/octet-stream", f"Content-Length: {length}",
                 "Connection: keep-alive",
                 "Date: " + time.strftime("%a, %d %b %Y %H:%M:%S GMT", time.gmtime())]
        if self.policy != "no-validator-once":
            lines += [f"ETag: {tag}", "Last-Modified: Sat, 18 Oct 2026 00:00:00 GMT"]
        if advertised:
            lines.append("Accept-Ranges: " + advertised)
        content_range = None
        if status == 206:
            low = start + 1 if self.policy == "bad-helper-range" else start
            content_range = f"bytes {low}-{end}/{self.o.size}"
            lines.append("Content-Range: " + content_range)
        request = Request(self.now(), method, target, rng, status, start, end)
        with self.lock:
            conn.requests.append(request)
            self.policies_sent.append({"number": number, "policy": self.policy,
                                       "status": status, "version": version,
                                       "range": rng, "ifRange": headers.get("if-range"),
                                       "contentRange": content_range,
                                       "responseTag": tag})
        sock.sendall(("\r\n".join(lines) + "\r\n\r\n").encode())
        request.headers_sent = self.now()
        if method == "HEAD":
            request.outcome, request.finished = "head", self.now()
            return True

        truncated = self.policy == "truncated-helper" and rng and number == 2
        stop = min(end, start + 65535) if truncated else end
        block = self.versions[version]
        budget = [conn.bytes, time.perf_counter(), 0]
        at = start
        while at <= stop:
            off = at % BLOCK
            take = min(BLOCK - off, stop - at + 1)
            if not self._send_all(sock, block[off:off + take], conn, request, budget):
                request.outcome, request.finished = "transport-drop", self.now()
                return False
            at += take
        request.finished = self.now()
        if truncated:
            request.outcome = "scripted-short-body"
            sock.shutdown(socket.SHUT_WR)
            return False
        request.outcome = "complete"
        return True

    def snapshot(self):
        result = super().snapshot()
        with self.lock:
            result["responsePolicies"] = list(self.policies_sent)
        return result
