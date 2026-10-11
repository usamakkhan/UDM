"""Run captured-response continuation tests against an isolated HTTP origin.
Usage: python tests/original-response-resume.py PATH_TO_Udm.OriginalResponseTests.exe
"""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=Path)
    args = parser.parse_args()
    executable = args.executable.resolve(strict=True)
    requests = []
    leaks = []

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_):
            pass

        def do_GET(self):
            requested = self.headers.get('Range', '')
            requests.append((self.path, requested, self.headers.get('If-Range', '')))
            payload = b'abcdefgh'
            if self.path == '/target-6' and any(self.headers.get(key) for key in ('Cookie', 'Authorization', 'Origin', 'Referer')):
                leaks.append(self.path)
                self.send_response(403)
                self.send_header('Content-Length', '0')
                self.end_headers()
                return
            if self.path in ('/resume-4', '/resume-5', '/resume-6', '/target-5') and (self.headers.get('Cookie') != 'fixture_session=allowed' or self.headers.get('Authorization') != 'Bearer fixture-secret'):
                self.send_response(403)
                self.send_header('Content-Length', '0')
                self.end_headers()
                return
            if self.path in ('/resume-5', '/resume-6'):
                self.send_response(302)
                self.send_header('Location', '/target-5' if self.path == '/resume-5' else f'http://127.0.0.1:{other.server_port}/target-6')
                self.send_header('Content-Length', '0')
                self.end_headers()
                return
            if self.path not in ('/resume-0', '/resume-1', '/resume-2', '/resume-3', '/resume-4', '/target-5', '/target-6'):
                self.send_response(404)
                self.send_header('Content-Length', '0')
                self.end_headers()
                return
            if self.path == '/resume-2' or (self.path == '/resume-3' and requested != 'bytes=0-0'):
                self.send_response(200)
                self.send_header('Content-Length', '8')
                self.send_header('ETag', '"v1"')
                self.end_headers()
                self.wfile.write(payload)
                return
            if requested not in ('bytes=0-0', 'bytes=4-7'):
                self.send_response(400)
                self.send_header('Content-Length', '0')
                self.end_headers()
                return
            begin, end = map(int, requested[6:].split('-'))
            self.send_response(206)
            self.send_header('Content-Length', str(end - begin + 1))
            self.send_header('Content-Range', f'bytes {begin}-{end}/8')
            self.send_header('ETag', '"v2"' if self.path == '/resume-1' else '"v1"')
            self.end_headers()
            self.wfile.write(payload[begin:end + 1])

    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    other = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    other_worker = threading.Thread(target=other.serve_forever, daemon=True)
    other_worker.start()
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    try:
        result = subprocess.run([str(executable), f'127.0.0.1:{server.server_port}'], timeout=120)
    finally:
        server.shutdown()
        server.server_close()
        worker.join()
        other.shutdown()
        other.server_close()
        other_worker.join()
    if result.returncode:
        raise SystemExit(result.returncode)
    expected = [
        ('/resume-0', 'bytes=0-0', ''),
        ('/resume-0', 'bytes=4-7', '"v1"'),
        ('/resume-1', 'bytes=0-0', ''),
        ('/resume-2', 'bytes=0-0', ''),
        ('/resume-3', 'bytes=0-0', ''),
        ('/resume-3', 'bytes=4-7', '"v1"'),
        ('/resume-4', 'bytes=0-0', ''),
        ('/resume-4', 'bytes=4-7', '"v1"'),
    ]
    for mode in (5, 6):
        for requested, validator in (('bytes=0-0', ''), ('bytes=4-7', '"v1"')):
            expected.extend([(f'/resume-{mode}', requested, validator), (f'/target-{mode}', requested, validator)])
    if leaks:
        raise SystemExit('Captured credentials leaked across origins')
    if requests != expected:
        raise SystemExit(f'Unexpected HTTP requests; no restart-from-zero is allowed: {requests!r}')
    print('PASS Same-origin credentials retained and cross-origin credentials omitted')
    print('PASS Exact range requests and validators; no replay from byte zero')


if __name__ == '__main__':
    main()