"""Run the native limiter UI fixture against a synthetic, in-memory HTTP file."""
from pathlib import Path
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
import argparse
import hashlib
import json
import os
import re
import subprocess
import threading

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('build_root', type=Path)
parser.add_argument('run_root', type=Path)
args = parser.parse_args()
build = args.build_root.resolve()
run = args.run_root.resolve()
runner_log = run.with_name(run.name + '-runner.log')
http_record = run.with_name(run.name + '-http.json')
if run.exists() or runner_log.exists() or http_record.exists():
    parser.error('Use a fresh run root and evidence paths; previous results are preserved.')
if not (build / 'release-native/Udm.ProgressLimiterUiTests.exe').is_file():
    parser.error('Build the native ProgressLimiterUiTests target first.')
run.parent.mkdir(parents=True, exist_ok=True)
payload = bytes(i % 251 for i in range(1024 * 1024))
requests = []


class Handler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def log_message(self, *_):
        pass

    def do_HEAD(self):
        self.respond(False)

    def do_GET(self):
        self.respond(True)

    def respond(self, body):
        if self.path != '/limiter.bin':
            self.send_error(404)
            return
        match = re.fullmatch(r'bytes=(\d+)-(\d*)', self.headers.get('Range', ''))
        first = int(match[1]) if match else 0
        last = min(int(match[2]), len(payload) - 1) if match and match[2] else len(payload) - 1
        if first > last:
            self.send_response(416)
            self.send_header('Content-Length', '0')
            self.end_headers()
            return
        code = 206 if match else 200
        entry = dict(method=self.command, first=first, last=last, status=code)
        requests.append(entry)
        self.send_response(code)
        self.send_header('Content-Type', 'application/octet-stream')
        self.send_header('Content-Length', str(last - first + 1))
        self.send_header('Accept-Ranges', 'bytes')
        self.send_header('ETag', '"limiter-live-fixture"')
        if match:
            self.send_header('Content-Range', f'bytes {first}-{last}/{len(payload)}')
        self.end_headers()
        if body:
            try:
                self.wfile.write(payload[first:last + 1])
                entry['bodyWritten'] = True
            except (ConnectionError, OSError):
                entry['clientClosed'] = True


server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
server.daemon_threads = True
worker = threading.Thread(target=server.serve_forever, daemon=True)
worker.start()
exit_code = -1
error = ''
try:
    endpoint = f'http://127.0.0.1:{server.server_port}/limiter.bin'
    runner = Path(__file__).resolve().parents[1] / 'native/test-progress-limiter.ps1'
    with runner_log.open('w', encoding='utf-8') as log:
        result = subprocess.run(
            ['powershell.exe', '-NoProfile', '-NonInteractive', '-File', str(runner),
             '-BuildRoot', str(build), '-RunRoot', str(run), '-LiveEndpoint', endpoint, '-SkipBuild'],
            stdout=log, stderr=subprocess.STDOUT,
            creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0)
        exit_code = result.returncode
except Exception as failure:
    error = str(failure)
finally:
    server.shutdown()
    server.server_close()
    worker.join()
    http_record.write_text(json.dumps(dict(
        runnerExitCode=exit_code, error=error, sourceBytes=len(payload),
        sourceSha256=hashlib.sha256(payload).hexdigest().upper(), requests=requests), indent=2) + '\n', encoding='utf-8')
if exit_code:
    raise SystemExit(f'Native limiter runner failed; inspect {runner_log}')
print(f'Native limiter live test passed; evidence: {http_record}')
