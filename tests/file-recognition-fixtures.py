from pathlib import Path
import sys,zipfile,hashlib,json
out=Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=True)
parts=[b'%PDF-1.4\n'];offsets=[0]
for index,body in enumerate([b'<< /Type /Catalog /Pages 2 0 R >>',b'<< /Type /Pages /Kids [3 0 R] /Count 1 >>',b'<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Resources << >> /Contents 4 0 R >>',b'<< /Length 0 >>\nstream\n\nendstream'],1):
 offsets.append(sum(map(len,parts)));parts.append(str(index).encode()+b' 0 obj\n'+body+b'\nendobj\n')
parts.append(b' '*(2*1024*1024))
xref=sum(map(len,parts));parts.append(b'xref\n0 5\n0000000000 65535 f \n'+b''.join(f'{x:010} 00000 n \n'.encode() for x in offsets[1:])+f'trailer\n<< /Size 5 /Root 1 0 R >>\nstartxref\n{xref}\n%%EOF\n'.encode())
(out/'blank.pdf').write_bytes(b''.join(parts))
with zipfile.ZipFile(out/'archive.zip','w',compression=zipfile.ZIP_STORED) as archive:archive.writestr('payload.txt',b'UDM independent recognition fixture.\n'*(2*1024*1024//37+1))
with zipfile.ZipFile(out/'archive.zip') as archive:assert archive.testzip() is None
(out/'error.html').write_bytes(b'<!doctype html><title>Test error</title><p>Download not available.</p>'+b' '*(2*1024*1024))
print(json.dumps({f.name:{'bytes':f.stat().st_size,'sha256':hashlib.sha256(f.read_bytes()).hexdigest()} for f in out.iterdir() if f.is_file()},indent=2))
