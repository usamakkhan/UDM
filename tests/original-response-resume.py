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

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_):
            pass

        def do_GET(self):
            requested = self.headers.get('Range', '')
            requests.append((self.path, requested, self.headers.get('If-Range', '')))
            payload = b'abcdefgh'
            if self.path not in ('/resume-0', '/resume-1', '/resume-2', '/resume-3'):
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
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    try:
        result = subprocess.run([str(executable), f'127.0.0.1:{server.server_port}'], timeout=120)
    finally:
        server.shutdown()
        server.server_close()
        worker.join()
    if result.returncode:
        raise SystemExit(result.returncode)
    expected = [
        ('/resume-0', 'bytes=0-0', ''),
        ('/resume-0', 'bytes=4-7', '"v1"'),
        ('/resume-1', 'bytes=0-0', ''),
        ('/resume-2', 'bytes=0-0', ''),
        ('/resume-3', 'bytes=0-0', ''),
        ('/resume-3', 'bytes=4-7', '"v1"'),
    ]
    if requests != expected:
        raise SystemExit(f'Unexpected HTTP requests; no restart-from-zero is allowed: {requests!r}')
    print('PASS Exact range requests and validators; no replay from byte zero')


if __name__ == '__main__':
    main()