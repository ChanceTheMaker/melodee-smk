# SPDX-License-Identifier: GPL-3.0-only
"""Loopback-only trial server: / serves the staged dual-core trial; /serial/ serves the audited serial comparison."""
import argparse
import hashlib
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parents[1]
SERIAL = ROOT / 'build/melodee-smk-1.1.0-b1'

def verified_serial():
    raw = (SERIAL / 'manifest.json').read_bytes()
    manifest = json.loads(raw)
    if manifest['dual_core'] or manifest['release_channel'] != 'local-development':
        raise ValueError('Expected the single-core local comparison')
    image = manifest['image']
    if Path(image).name != image: raise ValueError('Invalid comparison filename')
    blob = (SERIAL / image).read_bytes()
    if len(blob) != manifest['bytes'] or hashlib.sha256(blob).hexdigest() != manifest['sha256']:
        raise ValueError('Serial comparison hash mismatch')
    audit = (SERIAL / 'storage-audit.json').read_bytes()
    if hashlib.sha256(audit).hexdigest() != manifest['storage_audit_sha256']:
        raise ValueError('Serial storage audit mismatch')
    return {'/firmware/candidate.json': raw, '/firmware/' + image: blob,
            '/firmware/storage-audit.json': audit}

class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(ROOT / 'web'), **kwargs)

    def send_head(self):
        path = urlsplit(self.path).path
        if path.startswith('/serial/'):
            path = path[len('/serial'):]
            if path.startswith('/firmware/'):
                from io import BytesIO
                payload = self.server.serial_files.get(path)
                if payload is None: self.send_error(404);return None
                self.send_response(200)
                self.send_header('Content-Type', 'application/json' if path.endswith('.json') else 'application/octet-stream')
                self.send_header('Content-Length', str(len(payload)))
                self.send_header('Cache-Control', 'no-store')
                self.end_headers()
                return BytesIO(payload)
            self.path = self.path[len('/serial'):]
        return super().send_head()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8792)
    args = parser.parse_args()
    serial_files = verified_serial()
    server = ThreadingHTTPServer(('127.0.0.1', args.port), Handler)
    server.serial_files = serial_files
    print(f'Dual: http://localhost:{args.port}/installer.html\nSerial: http://localhost:{args.port}/serial/installer.html', flush=True)
    server.serve_forever()
