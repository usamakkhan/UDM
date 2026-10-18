"""Loopback HTTP/1.1 origin for download-manager benchmarks.

The server synthesizes a deterministic large file on the fly (no disk use) and
supports the knobs the benchmark cases need:

* per-connection bandwidth cap (token bucket) and response latency
* dropping a connection abruptly after every N bytes sent on it
* ignoring Range requests (sequential path)
* a cap on concurrent connections (extra connections are closed at once)

Every TCP connection and every HTTP request is recorded so the harness can
report connection counts, request ranges, reconnections and rejected
connections. Only 127.0.0.1 is served.

Run standalone for manual tests::

    python origin.py --size 268435456 --rate 20000000 --port 18080
"""
from __future__ import annotations
import argparse, hashlib, random, socket, threading, time, json, sys
from dataclasses import dataclass, field, asdict

BLOCK = 1048573  # prime-ish length so no power-of-two offset repeats the pattern


def pattern_block(seed: int) -> bytes:
    return random.Random(seed).randbytes(BLOCK)


def expected_sha256(size: int, seed: int) -> str:
    block = pattern_block(seed)
    h = hashlib.sha256()
    at = 0
    while at < size:
        off = at % BLOCK
        take = min(BLOCK - off, size - at)
        h.update(block[off:off + take])
        at += take
    return h.hexdigest()


@dataclass
class Request:
    at: float
    method: str
    path: str
    range: str | None
    status: int
    start: int
    end: int
    sent: int = 0
    finished: float | None = None
    outcome: str = ""


@dataclass
class Connection:
    id: int
    opened: float
    closed: float | None = None
    peer_port: int = 0
    rejected: bool = False
    bytes: int = 0
    requests: list = field(default_factory=list)
    drops: int = 0


@dataclass
class Options:
    size: int = 256 * 1024 * 1024
    seed: int = 20261018
    rate: int = 0            # bytes/s per connection, 0 = unlimited
    latency_ms: int = 0      # delay before response headers
    drop_every: int = 0      # close the connection after this many body bytes, 0 = never
    no_range: bool = False   # ignore Range (200 + full body, no Accept-Ranges)
    max_conns: int = 0       # concurrent connections allowed, 0 = unlimited
    path: str = "/bench/file.bin"
    etag: str = '"udm-speed-bench-v1"'
    chunk: int = 256 * 1024


class Origin:
    def __init__(self, options: Options, host: str = "127.0.0.1", port: int = 0):
        self.o = options
        self.block = pattern_block(options.seed)
        self.listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.listener.bind((host, port))
        self.listener.listen(128)
        self.port = self.listener.getsockname()[1]
        self.lock = threading.Lock()
        self.connections: list[Connection] = []
        self.active = 0
        self.peak_active = 0
        self.stopping = False
        self.blackout_until = 0.0  # while in the future, every connection is reset and new ones refused
        self.epoch = time.perf_counter()
        self.thread = threading.Thread(target=self._accept_loop, daemon=True)
        self.thread.start()

    @property
    def url(self) -> str:
        return f"http://127.0.0.1:{self.port}{self.o.path}"

    def now(self) -> float:
        return time.perf_counter() - self.epoch

    def close(self):
        self.stopping = True
        try:
            self.listener.close()
        except OSError:
            pass

    # ---- stats -------------------------------------------------------------
    def snapshot(self) -> dict:
        with self.lock:
            conns = [asdict(c) for c in self.connections]
        total = sum(c["bytes"] for c in conns)
        accepted = [c for c in conns if not c["rejected"]]
        return {
            "connections": len(accepted),
            "rejected_connections": len(conns) - len(accepted),
            "requests": sum(len(c["requests"]) for c in accepted),
            "bytes_sent": total,
            "drops": sum(c["drops"] for c in accepted),
            "peak_concurrent": self.peak_active,
            "detail": conns,
        }

    def bytes_sent(self) -> int:
        with self.lock:
            return sum(c.bytes for c in self.connections)

    def outage(self, seconds: float):
        """Simulate a network outage: reset all connections and refuse new ones for ``seconds``."""
        self.blackout_until = time.perf_counter() + seconds

    def in_outage(self) -> bool:
        return time.perf_counter() < self.blackout_until

    def reset_stats(self):
        with self.lock:
            self.connections.clear()
            self.peak_active = 0

    # ---- serving -----------------------------------------------------------
    def _accept_loop(self):
        counter = 0
        while not self.stopping:
            try:
                sock, peer = self.listener.accept()
            except OSError:
                return
            counter += 1
            conn = Connection(id=counter, opened=self.now(), peer_port=peer[1])
            with self.lock:
                self.connections.append(conn)
                if self.in_outage() or (self.o.max_conns and self.active >= self.o.max_conns):
                    conn.rejected = True
                    conn.closed = conn.opened
                else:
                    self.active += 1
                    self.peak_active = max(self.peak_active, self.active)
            if conn.rejected:
                try:
                    sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, b"\x01\x00\x00\x00\x00\x00\x00\x00")
                    sock.close()
                except OSError:
                    pass
                continue
            threading.Thread(target=self._serve, args=(sock, conn), daemon=True).start()

    def _serve(self, sock: socket.socket, conn: Connection):
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        sock.settimeout(30)
        pending = b""
        try:
            while not self.stopping:
                while b"\r\n\r\n" not in pending:
                    data = sock.recv(65536)
                    if not data:
                        return
                    pending += data
                    if len(pending) > 65536:
                        return
                head, pending = pending.split(b"\r\n\r\n", 1)
                lines = head.decode("latin-1").split("\r\n")
                method, target, *_ = lines[0].split(" ") + ["", ""]
                headers = {}
                for line in lines[1:]:
                    if ":" in line:
                        k, v = line.split(":", 1)
                        headers[k.strip().lower()] = v.strip()
                keep_alive = headers.get("connection", "").lower() != "close"
                if not self._respond(sock, conn, method, target, headers):
                    return
                if not keep_alive:
                    return
        except (ConnectionResetError, ConnectionAbortedError, BrokenPipeError, socket.timeout, OSError):
            pass
        finally:
            try:
                sock.close()
            except OSError:
                pass
            with self.lock:
                conn.closed = self.now()
                self.active -= 1

    def _send_all(self, sock, data: bytes, conn: Connection, req: Request, budget: list) -> bool:
        """Send with the per-connection rate cap. Returns False when the drop rule fired."""
        view = memoryview(data)
        at = 0
        while at < len(view):
            take = min(self.o.chunk, len(view) - at)
            if self.in_outage():
                conn.drops += 1
                req.outcome = "outage"
                return False
            if self.o.drop_every:
                room = self.o.drop_every - budget[0]
                if room <= 0:
                    conn.drops += 1
                    req.outcome = "dropped"
                    return False
                take = min(take, room)
            if self.o.rate:
                # token bucket: allow at most rate * elapsed bytes since the request began
                allowed = self.o.rate * (time.perf_counter() - budget[1])
                if budget[2] + take > allowed:
                    wait = (budget[2] + take - allowed) / self.o.rate
                    time.sleep(min(wait, 0.25))
                    continue
            sock.sendall(view[at:at + take])
            at += take
            budget[0] += take
            budget[2] += take
            with self.lock:
                conn.bytes += take
                req.sent += take
        return True

    def _respond(self, sock, conn: Connection, method: str, target: str, headers: dict) -> bool:
        o = self.o
        now = self.now()
        path = target.split("?", 1)[0]
        if path != o.path:
            body = b"not found"
            sock.sendall(b"HTTP/1.1 404 Not Found\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s" % (len(body), body))
            with self.lock:
                conn.requests.append(Request(now, method, target, None, 404, 0, -1, outcome="404"))
            return False
        if o.latency_ms:
            time.sleep(o.latency_ms / 1000)
        rng = headers.get("range")
        start, end, status = 0, o.size - 1, 200
        if rng and not o.no_range and rng.startswith("bytes="):
            a, _, b = rng[6:].partition("-")
            try:
                start = int(a) if a else max(0, o.size - int(b))
                end = int(b) if (b and a) else o.size - 1
            except ValueError:
                start, end = 0, -1
            if start >= o.size or end < start:
                resp = (f"HTTP/1.1 416 Range Not Satisfiable\r\nContent-Range: bytes */{o.size}\r\n"
                        f"Content-Length: 0\r\nETag: {o.etag}\r\n\r\n")
                sock.sendall(resp.encode())
                with self.lock:
                    conn.requests.append(Request(now, method, target, rng, 416, 0, -1, outcome="416"))
                return True
            end = min(end, o.size - 1)
            status = 206
        length = end - start + 1
        lines = [f"HTTP/1.1 {status} {'Partial Content' if status == 206 else 'OK'}",
                 "Content-Type: application/octet-stream",
                 f"Content-Length: {length}",
                 f"ETag: {o.etag}",
                 "Last-Modified: Sat, 18 Oct 2026 00:00:00 GMT",
                 "Date: " + time.strftime("%a, %d %b %Y %H:%M:%S GMT", time.gmtime()),
                 "Connection: keep-alive"]
        if not o.no_range:
            lines.append("Accept-Ranges: bytes")
        if status == 206:
            lines.append(f"Content-Range: bytes {start}-{end}/{o.size}")
        req = Request(now, method, target, rng, status, start, end)
        with self.lock:
            conn.requests.append(req)
        sock.sendall(("\r\n".join(lines) + "\r\n\r\n").encode())
        if method == "HEAD":
            req.outcome = "head"
            req.finished = self.now()
            return True
        budget = [conn.bytes, time.perf_counter(), 0]  # [bytes sent on this connection, t0, bytes since t0]
        at = start
        ok = True
        while at <= end:
            off = at % BLOCK
            take = min(BLOCK - off, end - at + 1)
            if not self._send_all(sock, self.block[off:off + take], conn, req, budget):
                ok = False
                break
            at += take
        req.finished = self.now()
        if ok:
            req.outcome = "complete"
        else:
            # abrupt close: RST rather than FIN so the client sees a hard drop
            try:
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, b"\x01\x00\x00\x00\x00\x00\x00\x00")
            except OSError:
                pass
        return ok


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--size", type=int, default=256 * 1024 * 1024)
    p.add_argument("--rate", type=int, default=0, help="bytes/s per connection (0 = unlimited)")
    p.add_argument("--latency-ms", type=int, default=0)
    p.add_argument("--drop-every", type=int, default=0, help="close each connection after this many body bytes")
    p.add_argument("--no-range", action="store_true")
    p.add_argument("--max-conns", type=int, default=0)
    p.add_argument("--port", type=int, default=0)
    a = p.parse_args(argv)
    o = Options(size=a.size, rate=a.rate, latency_ms=a.latency_ms, drop_every=a.drop_every, no_range=a.no_range, max_conns=a.max_conns)
    origin = Origin(o, port=a.port)
    print(origin.url, "sha256", expected_sha256(o.size, o.seed), flush=True)
    try:
        while True:
            time.sleep(5)
            s = origin.snapshot()
            s.pop("detail")
            print(json.dumps(s), flush=True)
    except KeyboardInterrupt:
        origin.close()


if __name__ == "__main__":
    main()
