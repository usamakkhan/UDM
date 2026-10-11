"""Reproduce the known captured-response credential gap without a live driver.
This audit passes only when the gap is reproduced; it is not feature acceptance.
Usage: python tests/captured-auth-gap.py Udm.OriginalResponseTests.exe REPORT.json
"""
import argparse
import json
import re
import subprocess
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=Path)
    parser.add_argument('report', type=Path)
    args = parser.parse_args()
    requests = []

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_):
            pass

        def do_GET(self):
            authorized = self.headers.get('Cookie') == 'fixture_session=allowed'
            requests.append({'path': self.path, 'range': self.headers.get('Range', ''),
                             'cookiePresent': bool(self.headers.get('Cookie')), 'authorized': authorized})
            if not authorized:
                self.send_response(403)
                self.send_header('Content-Length', '0')
                self.end_headers()
                return
            requested = self.headers.get('Range', '')
            if requested not in ('bytes=0-0', 'bytes=4-7'):
                self.send_response(400)
                self.send_header('Content-Length', '0')
                self.end_headers()
                return
            first, last = map(int, requested[6:].split('-'))
            self.send_response(206)
            self.send_header('Content-Length', str(last - first + 1))
            self.send_header('Content-Range', f'bytes {first}-{last}/8')
            self.send_header('ETag', '"v1"')
            self.end_headers()
            self.wfile.write(b'abcdefgh'[first:last + 1])

    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    try:
        result = subprocess.run([str(args.executable.resolve(strict=True)),
                                 f'127.0.0.1:{server.server_port}'],
                                capture_output=True, text=True, timeout=120)
    finally:
        server.shutdown()
        server.server_close()
        worker.join()
    match = re.search(r'evidence (.+)', result.stdout)
    if not match:
        raise SystemExit('No native fixture evidence: ' + result.stderr)
    root = Path(match.group(1).strip())
    catalog = json.loads((root / 'resume-0' / 'state.json').read_text(encoding='utf-8-sig'))
    job = catalog['Downloads'][0]
    original = root / 'resume-0' / 'responses' / job['Id'] / 'body'
    prefix = Path(job['PartsFolder']) / '0000.part'
    preserved = original.read_bytes() == b'abcd' and prefix.read_bytes() == b'abcd'
    output_absent = not (Path(job['Folder']) / job['FileName']).exists()
    reproduced = (result.returncode != 0 and '403' in result.stdout and bool(requests)
                  and all(not row['cookiePresent'] for row in requests)
                  and not job.get('ProtectedHeaders') and preserved and output_absent)
    report = {'KnownGapReproduced': reproduced, 'FeatureAcceptance': False,
              'Scope': 'Native captured-prefix continuation against a local cookie-required origin; no live driver/browser capture.',
              'NativeExitCode': result.returncode, 'Requests': requests,
              'ProtectedHeadersPresent': bool(job.get('ProtectedHeaders')),
              'CapturedPrefixPreserved': preserved, 'OutputNotPublished': output_absent,
              'FixtureRoot': str(root), 'NativeOutput': result.stdout}
    args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: value for key, value in report.items() if key != 'NativeOutput'}, indent=2))
    if not reproduced:
        raise SystemExit('Expected credential gap was not reproduced; inspect the report.')


if __name__ == '__main__':
    main()