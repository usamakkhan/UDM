"""Read-only decoder derived from IDMFType.dll 10001000/100013f0/10004400.
Keeps the original evidence and checks the transform by round trip.
"""
import argparse,hashlib,json,pathlib,struct
def transform(b):
    d=bytearray(b)
    for i in range(0,len(d),4):
        mask=struct.pack('<I',((i//4+1)*0xe395a6c7)&0xffffffff)
        for j in range(min(4,len(d)-i)):d[i+j]^=mask[j]
    return bytes(d)
def main():
    ap=argparse.ArgumentParser();ap.add_argument('input');ap.add_argument('--output',required=True);a=ap.parse_args();p=pathlib.Path(a.input);b=p.read_bytes();out=pathlib.Path(a.output);out.mkdir(parents=True,exist_ok=True)
    if b[0]!=0x99 or b[5:13]!=b'IDMFTYPE' or b[13]!=1:raise ValueError('unrecognized file type table header')
    flags=struct.unpack_from('<I',b,15)[0];pos=31;rows=[];tables={};blocks={}
    while pos<len(b):
        tag,n=struct.unpack_from('<BI',b,pos)
        if pos+5+n>len(b):raise ValueError('truncated block')
        original=b[pos+5:pos+5+n];raw=transform(original) if flags&2 else original
        if flags&2 and transform(raw)!=original:raise ValueError('transform round trip failed')
        name=f'{tag:02x}';blocks[tag]=raw;(out/(name+'.bin')).write_bytes(raw);row={'tag':hex(tag),'headerOffset':pos,'bytes':n,'roundTripVerified':True,'decodedSha256':hashlib.sha256(raw).hexdigest()}
        if tag in (0xf0,0xf1,0xf2):
            if not raw.endswith(b'\0'):raise ValueError('string table not null terminated')
            strings=raw[:-1].split(b'\0');tables[tag]=[None]+[s.decode('cp1252',errors='replace') for s in strings];(out/(name+'-strings.json')).write_text(json.dumps(tables[tag][1:],indent=2),encoding='utf-8');row['strings']=len(strings)
        elif tag==0xf3:
            cursor=0;blobs=[]
            while cursor<len(raw):
                size=raw[cursor];blobs.append(raw[cursor+1:cursor+1+size].hex());cursor+=size+1
            if cursor!=len(raw):raise ValueError('signature table boundary mismatch')
            row['lengthPrefixedBlobs']=len(blobs);(out/'f3-blobs.json').write_text(json.dumps(blobs,indent=2),encoding='utf-8')
        elif tag==0xf4:
            if n%2:raise ValueError('odd mapping table length')
            row['uint16Entries']=n//2
        elif tag==0xf5:
            if n%38:raise ValueError('rule record alignment failed')
            row['ruleRecordBytes']=38;row['ruleRecords']=n//38
        rows.append(row);pos+=5+n
    mappings=[];current=None
    for value, in struct.iter_unpack('<H',blocks[0xf4]):
        if value&0x4000:
            current={'mime':tables[0xf0][value&0xbfff],'extensions':[]};mappings.append(current)
        elif value&0x8000:
            if current is None:raise ValueError('extension without MIME')
            current['extensions'].append(tables[0xf1][value&0x7fff])
        else:raise ValueError('unrecognized mapping entry')
    (out/'mime-extension-map.json').write_text(json.dumps(mappings,indent=2)+'\n',encoding='utf-8')
    result={'source':str(p),'sha256':hashlib.sha256(b).hexdigest(),'headerTag':hex(b[0]),'headerDeclaredBytes':struct.unpack_from('<I',b,1)[0],'headerBytesConsumed':31,'flags':flags,'fileFullyConsumed':pos==len(b),'blocks':rows,'mimeMappings':len(mappings),'evidenceFunctions':['IDMFType.dll:0x10001000','IDMFType.dll:0x100013f0','IDMFType.dll:0x10001490','IDMFType.dll:0x10004400','IDMFType.dll:0x100019a0'],'limitation':'Data tables and rule record layout recovered; not a reimplementation or runtime validation of the full classifier.'}
    (out/'summary.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print(json.dumps(result,indent=2))
if __name__=='__main__':main()
