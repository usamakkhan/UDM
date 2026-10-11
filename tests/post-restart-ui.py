"""Run the actual Progress dialog's explicit form restart against loopback HTTP."""
import hashlib
import http.server
import json
from pathlib import Path
import subprocess
import sys
import threading

exe, output, reference = map(Path, sys.argv[1:4])
if output.exists():
    raise SystemExit('Use a fresh output directory')
expected = b'export=fixture&name=%E2%9C%93&value=one%2Btwo'
requests = []

class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *_):
        pass
    def do_GET(self):
        requests.append({'method': 'GET', 'path': self.path})
        self.send_error(405)
    def do_POST(self):
        body = self.rfile.read(int(self.headers.get('Content-Length', '0')))
        requests.append({'method': 'POST', 'path': self.path,
                         'bodySha256': hashlib.sha256(body).hexdigest(),
                         'contentType': self.headers.get('Content-Type'),
                         'range': self.headers.get('Range')})
        self.send_response(200)
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Content-Type', 'application/octet-stream')
        self.end_headers()
        self.wfile.write(body)

server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()
error = ''
try:
    result = subprocess.run([str(exe), str(output), str(reference),
                             f'http://127.0.0.1:{server.server_port}/echo'],
                            capture_output=True, timeout=45,
                            creationflags=subprocess.CREATE_NO_WINDOW)
    assert result.returncode == 0, f'GUI fixture exit {result.returncode}'
    gui = json.loads((output / 'results.json').read_text(encoding='utf-8-sig'))
    assert gui['passed'], gui['error']
    assert len(requests) == 1, requests
    request = requests[0]
    assert request == {'method': 'POST', 'path': '/echo',
                       'bodySha256': hashlib.sha256(expected).hexdigest(),
                       'contentType': 'application/x-www-form-urlencoded',
                       'range': None}, request
    print(f"{len(gui['checks'])} GUI checks passed; exactly one original POST, no probes or ranges")
except Exception as failure:
    error = str(failure)
    raise
finally:
    server.shutdown()
    server.server_close()
    thread.join()
    output.mkdir(parents=True, exist_ok=True)
    (output / 'http-request-evidence.json').write_text(json.dumps(
        {'passed': not error, 'error': error, 'requests': requests}, indent=2))
