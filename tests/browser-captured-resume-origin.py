"""Local browser/driver resume fixture; payload is generated, never saved here."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import argparse
import hashlib
import json
from pathlib import Path
import threading
import time

parser = argparse.ArgumentParser()
parser.add_argument('--port', type=int, default=80)
parser.add_argument('--log', type=Path, required=True)
args = parser.parse_args()
block = bytes(range(256)) * 256
size = len(block) * 128
released = threading.Event()
log_lock = threading.Lock()


def record(item):
    with log_lock, args.log.open('a', encoding='utf-8') as stream:
        stream.write(json.dumps(item) + '\n')


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *_):
        pass

    def do_GET(self):
        if self.path == '/':
            page = b'<title>UDM captured resume test</title><h1>UDM captured resume test</h1><p>8 MiB generated fixture; first request pauses after 1 MiB.</p><a href="/UDM-captured-resume.zip">Download resume fixture</a>'
            self.send_response(200)
            self.send_header('Content-Type', 'text/html')
            self.send_header('Content-Length', str(len(page)))
            self.end_headers()
            self.wfile.write(page)
            return
        if self.path != '/UDM-captured-resume.zip':
            self.send_response(404)
            self.send_header('Content-Length', '0')
            self.end_headers()
            return
        requested = self.headers.get('Range', '')
        begin, end = 0, size - 1
        if requested:
            try:
                if not requested.startswith('bytes='):
                    raise ValueError()
                first, last = requested[6:].split('-')
                begin, end = int(first), int(last) if last else size - 1
                if not 0 <= begin <= end < size:
                    raise ValueError()
            except ValueError:
                self.send_response(416)
                self.send_header('Content-Length', '0')
                self.end_headers()
                return
        record({'range': requested, 'ifRange': self.headers.get('If-Range', ''), 'begin': begin, 'end': end})
        self.send_response(206 if requested else 200)
        self.send_header('Content-Type', 'application/zip')
        self.send_header('Content-Disposition', 'attachment; filename="UDM-captured-resume.zip"')
        self.send_header('Content-Length', str(end - begin + 1))
        self.send_header('ETag', '"udm-captured-resume-v1"')
        self.send_header('Accept-Ranges', 'bytes')
        if requested:
            self.send_header('Content-Range', f'bytes {begin}-{end}/{size}')
            released.set()
        self.end_headers()
        sent = 0
        try:
            for position in range(begin, end + 1, len(block)):
                count = min(len(block), end - position + 1)
                offset = position % len(block)
                payload = (block + block)[offset:offset + count]
                self.wfile.write(payload)
                self.wfile.flush()
                sent += count
                if not requested and sent == 1048576:
                    released.wait(180)
                if not requested:
                    time.sleep(0.02)
        except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
            pass
        finally:
            record({'finishedRange': requested, 'sent': sent})


args.log.parent.mkdir(parents=True, exist_ok=True)
server = ThreadingHTTPServer(('127.0.0.1', args.port), Handler)
print(json.dumps({'port': server.server_port, 'bytes': size, 'sha256': hashlib.sha256(block * 128).hexdigest()}), flush=True)
server.serve_forever()