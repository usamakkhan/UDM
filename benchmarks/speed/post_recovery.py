"""Verify ambiguous POST completion is not replayed by transport or queue retries."""
from __future__ import annotations

import argparse
import base64
import hashlib
import http.server
import json
import re
import socket
import subprocess
import threading
from urllib.parse import urlsplit
from pathlib import Path

from bench import sha256_file

PAYLOAD = b"UDM form fixture\x00\r\n" * 101
RESPONSE = b"UDM completed form response\x00\xff" * 37


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--cases", help="Comma-separated case names; defaults to all cases.")
    args = parser.parse_args()
    executable = args.fixture.resolve(strict=True)
    root, output = args.work.resolve(), args.out.resolve()
    if output.exists() or root == output or root in output.parents or output in root.parents:
        parser.error("Use fresh independent paths.")
    root.mkdir(parents=True, exist_ok=False)
    output.parent.mkdir(parents=True, exist_ok=True)
    records = []
    specs = [("direct-loss", "loss", False, False, "POST"),
             ("proxy-loss", "loss", True, False, "POST"),
             ("proxy-empty-loss", "loss", True, True, "POST"),
             ("proxy-timeout", "timeout", True, False, "POST"),
             ("proxy-explicit", "explicit", True, False, "POST"),
             ("proxy-success", "success", True, False, "POST"),
             ("proxy-digest-loss", "loss", True, False, "POST"),
             ("proxy-digest-success", "success", True, False, "POST"),
             ("proxy-get-retry", "loss", True, False, "GET"),
             ("get-queue-retry", "loss", False, False, "GET")]
    if args.cases:
        selected = set(args.cases.split(","))
        if not selected.issubset({spec[0] for spec in specs}):
            parser.error("Unknown recovery case.")
        specs = [spec for spec in specs if spec[0] in selected]
    for name, mode, proxied, empty, method in specs:
        case = root / name
        case.mkdir()
        state = {"requests": [], "challenges": 0, "digest_checks": []}
        lock = threading.Lock()
        body = b"" if empty else PAYLOAD
        digest_proxy = name.startswith("proxy-digest-")

        class Handler(http.server.BaseHTTPRequestHandler):
            protocol_version = "HTTP/1.1"

            def log_message(self, *_):
                pass

            def handle_request(self):
                received = self.rfile.read(int(self.headers.get("Content-Length", 0)))
                authorization = self.headers.get("Proxy-Authorization", "")
                authenticated = authorization == "Basic " + base64.b64encode(b"fixture-user:fixture-secret").decode()
                if digest_proxy:
                    fields = {key.lower(): quoted or plain for key, quoted, plain in re.findall(r'(\w+)=(?:"([^"]*)"|([^,\s]+))', authorization)}
                    md5 = lambda value: hashlib.md5(value.encode()).hexdigest()
                    first = md5("fixture-user:UDM form fixture:fixture-secret")
                    second = md5(self.command + ":" + fields.get("uri", ""))
                    middle = ":".join((fields.get("nc", ""), fields.get("cnonce", ""), fields.get("qop", "")))
                    expected = md5(first + ":fixture-nonce:" + middle + ":" + second)
                    route = urlsplit(self.path)
                    local_target = route.path + ("?" + route.query if route.query else "")
                    uri_matches = fields.get("uri") in (self.path, local_target)
                    authenticated = (authorization.startswith("Digest ") and fields.get("username") == "fixture-user"
                                     and fields.get("nonce") == "fixture-nonce" and fields.get("qop") == "auth"
                                     and uri_matches and fields.get("response") == expected)
                    if authorization:
                        with lock:
                            state["digest_checks"].append({"uri_matches": uri_matches, "response_matches": fields.get("response") == expected,
                                                           "username_matches": fields.get("username") == "fixture-user", "qop_auth": fields.get("qop") == "auth"})
                if proxied and not authenticated:
                    with lock:
                        state["challenges"] += 1
                    self.send_response(407)
                    self.send_header("Proxy-Authenticate", 'Digest realm="UDM form fixture", nonce="fixture-nonce", algorithm=MD5, qop="auth"' if digest_proxy else 'Basic realm="UDM form fixture"')
                    self.send_header("Content-Length", "0")
                    self.end_headers()
                    return
                with lock:
                    state["requests"].append({"method": self.command, "body_sha256": hashlib.sha256(received).hexdigest(), "bytes": len(received)})
                    count = len(state["requests"])
                success = mode == "success" or ((mode == "explicit" or method == "GET") and count > 1)
                if success:
                    self.send_response(200)
                    self.send_header("Content-Type", "application/octet-stream")
                    self.send_header("Content-Length", str(len(RESPONSE)))
                    self.end_headers()
                    self.wfile.write(RESPONSE)
                elif mode == "timeout":
                    self.connection.settimeout(0.1)
                    while True:
                        try:
                            if not self.connection.recv(1):
                                break
                        except socket.timeout:
                            continue
                else:
                    self.close_connection = True
                    try:
                        self.connection.shutdown(socket.SHUT_RDWR)
                    except OSError:
                        pass

            do_POST = handle_request
            do_GET = handle_request

        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        server.daemon_threads = True
        threading.Thread(target=server.serve_forever, daemon=True).start()
        endpoint = f"127.0.0.1:{server.server_port}"
        config = {"url": f"http://{('udm-post-recovery.invalid' if proxied else endpoint)}/submit", "proxy": endpoint if proxied else "",
                  "mode": mode, "method": method, "body": base64.b64encode(body).decode(), "sha256": hashlib.sha256(RESPONSE).hexdigest()}
        source = case / "input.json"
        source.write_text(json.dumps(config), encoding="utf-8")
        try:
            with (case / "native.log").open("w", encoding="utf-8") as log:
                process = subprocess.run([str(executable), str(case / "native"), "--post-recovery", str(source)], stdout=log, stderr=subprocess.STDOUT, timeout=40)
            native = json.loads((case / "native" / "results.json").read_text(encoding="utf-8"))
            expected_count = 2 if mode == "explicit" or method == "GET" else 1
            exact_requests = len(state["requests"]) == expected_count and all(request["method"] == method and request["body_sha256"] == hashlib.sha256(body if method == "POST" else b"").hexdigest() for request in state["requests"])
            passed = native["passed"] and process.returncode == 0 and exact_requests and (not proxied or state["challenges"] > 0)
            records.append({"case": name, "passed": passed, "native": native, "traffic": state})
            print(json.dumps({"case": name, "passed": passed, "requests": len(state["requests"]), "native": native}), flush=True)
        finally:
            server.shutdown()
            server.server_close()
            for file in case.rglob("*"):
                if file.is_file() and file.suffix in (".bin", ".part", ".data"):
                    if not file.resolve().is_relative_to(root):
                        raise RuntimeError("Payload escaped the fixture directory.")
                    file.unlink()
    result = {"passed": all(record["passed"] for record in records), "cases": records,
              "fixture_sha256": sha256_file(executable), "temporary_payloads_removed": True}
    with output.open("x", encoding="utf-8") as receipt:
        json.dump(result, receipt, indent=2)
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
