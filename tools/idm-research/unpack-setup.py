"""Extract this observed Tonec setup format without executing the installer.

The PE overlay consists of <IIQQQ> headers followed by zlib streams.
Record zero is an installation manifest; P1..Pn name subsequent records.
Unknown bytes and header mismatches are recorded, not silently discarded.
"""
import argparse, hashlib, json, pathlib, re, struct, sys, zlib
def sha(b): return hashlib.sha256(b).hexdigest()
def main():
    ap=argparse.ArgumentParser();ap.add_argument('installer');ap.add_argument('--output',required=True);ap.add_argument('--python-libs',required=True);a=ap.parse_args();sys.path.insert(0,a.python_libs)
    import pefile
    src=pathlib.Path(a.installer);b=src.read_bytes();pe=pefile.PE(data=b)
    out=pathlib.Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True);(out/src.name).write_bytes(b)
    pe_end=max(s.PointerToRawData+s.SizeOfRawData for s in pe.sections);cert=pe.OPTIONAL_HEADER.DATA_DIRECTORY[4];end=cert.VirtualAddress or len(b)
    marker=b.rfind(b'!--END FILE--!\0',max(0,len(b)-0x5000),end)
    if marker<4:raise ValueError('setup footer not found')
    start=struct.unpack_from('<I',b,marker-4)[0]
    if start<pe_end or start>=marker:raise ValueError('invalid footer payload offset')
    pos=start;rows=[];names={};payload_root=out/'payload';payload_root.mkdir(exist_ok=True)
    def read_record(index,name):
        nonlocal pos
        if pos+32>end: raise ValueError('truncated record header')
        declared,packed,ct,at,mt=struct.unpack_from('<IIQQQ',b,pos)
        if packed>end-pos-32 or declared>512*1024*1024:raise ValueError('out of bounds record')
        stream=b[pos+32:pos+32+packed];d=zlib.decompressobj();raw=d.decompress(stream,512*1024*1024+1)
        if not d.eof or d.unused_data or d.unconsumed_tail:raise ValueError('zlib stream boundary mismatch')
        if index and len(raw)!=declared:raise ValueError('payload size mismatch')
        row={'index':index,'name':name,'headerOffset':pos,'compressedOffset':pos+32,'compressedBytes':packed,'declaredBytes':declared,'actualBytes':len(raw),'sizeMatches':declared==len(raw),'sha256':sha(raw),'compressedSha256':sha(stream),'creationFiletime':ct,'accessFiletime':at,'modifiedFiletime':mt,'zlibChecksumVerified':True}
        rows.append(row);pos+=32+packed;return raw
    manifest=read_record(0,'install-manifest.txt');(out/'install-manifest.txt').write_bytes(manifest)
    names={int(i):s.decode('cp1252') for i,s in re.findall(rb'<P(\d+)="([^"\r\n]+)"',manifest)}
    if sorted(names)!=list(range(1,len(names)+1)): raise ValueError('non-contiguous or duplicate manifest IDs')
    for i,name in sorted(names.items()):
        q=pathlib.PurePosixPath(name.replace('\\','/'))
        if q.is_absolute() or '..' in q.parts or ':' in name: raise ValueError('unsafe manifest destination')
        dest=payload_root.joinpath(*q.parts);dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(read_record(i,name))
    trailer=b[pos:end];(out/'pre-certificate-trailer.bin').write_bytes(trailer)
    certificate=b[end:end+cert.Size];(out/'authenticode-table.bin').write_bytes(certificate)
    tail=b[end+cert.Size:];(out/'after-certificate.bin').write_bytes(tail)
    if pos!=marker-4:raise ValueError('unaccounted bytes between records and footer')
    report={'installer':str(src),'sha256':sha(b),'bytes':len(b),'peEnd':pe_end,'payloadOffset':start,'footerOffset':marker-4,'recordEnd':pos,'certificateOffset':end,'certificateBytes':cert.Size,'postCertificateBytes':len(tail),'preCertificateTrailerHex':trailer.hex(),'payloadFiles':len(names),'allPayloadSizesMatch':all(x['sizeMatches'] for x in rows[1:]),'records':rows,'limits':['Manifest record first field is output allocation capacity, not exact decompressed length; confirmed in setup function 0x402000.','Extraction verifies zlib checksums, not the authenticity of the signature.','Installer has not been executed.']}
    (out/'setup-records.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:v for k,v in report.items() if k!='records'},indent=2))
if __name__=='__main__':main()
