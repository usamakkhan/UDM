"""Account for image containers, CHM metadata streams, and package signature blobs."""
import argparse, collections, hashlib, json, pathlib, struct, subprocess, zlib

def png(b):
    pos=8;chunks=[];compressed=bytearray()
    while pos<len(b):
        n=struct.unpack_from('>I',b,pos)[0];kind=b[pos+4:pos+8];payload=b[pos+8:pos+8+n]
        if len(payload)!=n or pos+12+n>len(b):raise ValueError('Truncated PNG chunk')
        if zlib.crc32(kind+payload)&0xffffffff!=struct.unpack_from('>I',b,pos+8+n)[0]:raise ValueError('PNG CRC mismatch')
        chunks.append({'type':kind.decode('ascii'),'bytes':n});pos+=12+n
        if kind==b'IDAT':compressed.extend(payload)
        if kind==b'IEND':break
    d=zlib.decompressobj();pixels=d.decompress(compressed,128*1024*1024)
    if not d.eof or d.unused_data or d.unconsumed_tail:raise ValueError('PNG IDAT boundary/size failure')
    return {'parser':'PNG chunks and IDAT zlib','chunks':chunks,'inflatedBytes':len(pixels),'trailingBytes':len(b)-pos}

def gif(b):
    pos=13;images=0;extensions=0
    if b[10]&128:pos+=3*(1<<((b[10]&7)+1))
    def blocks(at):
        while True:
            n=b[at];at+=1
            if not n:return at
            at+=n
            if at>len(b):raise ValueError('GIF block exceeds file')
    while pos<len(b):
        kind=b[pos];pos+=1
        if kind==0x3b:return {'parser':'GIF block structure','images':images,'extensions':extensions,'trailingBytes':len(b)-pos,'limitation':'LZW pixels not decoded'}
        if kind==0x21:extensions+=1;pos=blocks(pos+1)
        elif kind==0x2c:
            images+=1;flags=b[pos+8];pos+=9
            if flags&128:pos+=3*(1<<((flags&7)+1))
            pos=blocks(pos+1)
        else:raise ValueError('Invalid GIF block type')
    raise ValueError('GIF trailer missing')

class CBOR:
    def __init__(self,b):self.b=b;self.p=0
    def take(self,n):
        v=self.b[self.p:self.p+n];self.p+=n
        if len(v)!=n:raise ValueError('CBOR truncation')
        return v
    def value(self,depth=0):
        if depth>32:raise ValueError('CBOR nesting limit')
        first=self.take(1)[0];major=first>>5;add=first&31
        n=add if add<24 else int.from_bytes(self.take({24:1,25:2,26:4,27:8}[add]),'big')
        if major==0:return n
        if major==1:return -n-1
        if major==2:
            v=self.take(n);return {'bytes':n,'sha256':hashlib.sha256(v).hexdigest(),'prefixHex':v[:16].hex()}
        if major==3:return self.take(n).decode('utf-8')
        if major==4:return [self.value(depth+1) for _ in range(n)]
        if major==5:return [{'key':self.value(depth+1),'value':self.value(depth+1)} for _ in range(n)]
        if major==6:return {'tag':n,'value':self.value(depth+1)}
        return {'simpleOrFloatBits':n,'additionalInfo':add}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('evidence');a=ap.parse_args();root=pathlib.Path(a.evidence);out=root/'auxiliary';out.mkdir(exist_ok=True)
    inv=json.loads((root/'inventory.json').read_text());rows=[]
    for o in inv['objects']:
        if o['kind'] not in ('image','bitmap','other resource'):continue
        p=pathlib.Path(o['snapshot']);b=p.read_bytes();row={'name':o['name'],'sha256':o['sha256'],'bytes':len(b),'kind':o['kind']}
        try:
            if b.startswith(b'\x89PNG'):row.update(png(b))
            elif b.startswith((b'GIF87a',b'GIF89a')):row.update(gif(b))
            elif b.startswith(b'BM'):
                size,offset=struct.unpack_from('<I',b,2)[0],struct.unpack_from('<I',b,10)[0]
                if size!=len(b) or offset>len(b):raise ValueError('BMP size/offset mismatch')
                row.update({'parser':'BMP file and DIB headers','declaredBytes':size,'pixelOffset':offset,'width':struct.unpack_from('<i',b,18)[0],'height':struct.unpack_from('<i',b,22)[0],'bitsPerPixel':struct.unpack_from('<H',b,28)[0],'limitation':'Pixel rendering not reviewed'})
            elif o['name'].endswith('.rsa'):
                r=subprocess.run(['certutil','-dump',str(p)],capture_output=True,text=True,errors='replace');(out/(o['sha256']+'-certutil.txt')).write_text(r.stdout+r.stderr,encoding='utf-8');row.update({'parser':'CertUtil signature/certificate dump','exitCode':r.returncode,'limitation':'Structure dump; package signature cryptographic validation not performed'})
                if r.returncode:raise ValueError('CertUtil rejected signature container')
            elif o['name']=='cose.sig':
                c=CBOR(b);row.update({'parser':'CBOR structure','structure':c.value(),'trailingBytes':len(b)-c.p,'limitation':'COSE signature not cryptographically verified'})
            elif o['name']=='#SYSTEM':
                pos=4;entries=[]
                while pos<len(b):
                    tag,n=struct.unpack_from('<HH',b,pos);pos+=4;v=b[pos:pos+n];pos+=n
                    if len(v)!=n:raise ValueError('CHM system record exceeds file')
                    entries.append({'tag':tag,'bytes':n,'offset':pos-n,'sha256':hashlib.sha256(v).hexdigest()})
                row.update({'parser':'CHM system record boundaries','version':struct.unpack_from('<I',b)[0],'records':entries,'trailingBytes':len(b)-pos,'limitation':'Record semantics not fully mapped'})
            elif o['name'] in ('#TOPICS','#URLTBL'):
                stride=16 if o['name']=='#TOPICS' else 12
                row.update({'parser':'CHM fixed-record words','stride':stride,'records':[list(struct.unpack_from('<'+'I'*(stride//4),b,p)) for p in range(0,len(b)-stride+1,stride)],'trailingBytes':len(b)%stride,'limitation':'Cross-stream semantics not fully mapped'})
            else:row.update({'parser':'Retained raw auxiliary stream','prefixHex':b[:64].hex(),'limitation':'Complete format semantics unresolved'})
        except Exception as e:row['error']=str(e)
        rows.append(row)
    (out/'summary.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'files':len(rows),'parsers':dict(collections.Counter(x.get('parser','failed') for x in rows)),'errors':[x for x in rows if 'error' in x]},indent=2))
if __name__=='__main__':main()
