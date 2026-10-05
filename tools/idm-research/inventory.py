"""Read-only, exhaustive reference inventory. Outputs belong outside distributable source."""
import argparse, collections, csv, hashlib, io, json, math, os, pathlib, re, shutil, struct, subprocess, sys, zipfile
from datetime import datetime, timezone

def digest(b): return hashlib.sha256(b).hexdigest()
def write_json(p, v):
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(json.dumps(v, indent=2, ensure_ascii=True) + '\n', encoding='utf-8')
def entropy(b):
    return -sum((n/len(b))*math.log2(n/len(b)) for n in collections.Counter(b).values()) if b else 0
def decode(b):
    if b.startswith((b'\xff\xfe', b'\xfe\xff')): return b.decode('utf-16'), 'utf-16'
    if b[:200].count(b'\0') > 30: return b.decode('utf-16-le', errors='replace'), 'utf-16-le (inferred)'
    try: return b.decode('utf-8-sig'), 'utf-8'
    except UnicodeDecodeError: return b.decode('cp1252', errors='replace'), 'cp1252 fallback (locale unverified)'

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--root', action='append', required=True); ap.add_argument('--output', required=True); ap.add_argument('--sevenzip', required=True); ap.add_argument('--python-libs'); a=ap.parse_args()
    if a.python_libs: sys.path.insert(0,a.python_libs)
    import pefile, capstone
    out=pathlib.Path(a.output).resolve(); out.mkdir(parents=True,exist_ok=True)
    records=[]; objects={}; errors=[]; links=[]; native=[]; archive_members=[]
    def error(stage,path,e): errors.append({'stage':stage,'path':str(path),'error':str(e)})
    def inspect(p, origin, relative, depth=0):
        try: b=p.read_bytes(); h=digest(b); st=p.stat()
        except Exception as e: error('read',p,e); return
        record={'origin':origin,'path':relative,'sha256':h,'bytes':len(b),'attributes':getattr(st,'st_file_attributes',0),'modifiedNs':st.st_mtime_ns}
        records.append(record)
        if h in objects: return
        d=out/'objects'/h; d.mkdir(parents=True,exist_ok=True); snap=d/p.name; snap.write_bytes(b)
        info={'sha256':h,'name':p.name,'bytes':len(b),'entropy':round(entropy(b),5),'magic':b[:16].hex(),'snapshot':str(snap),'status':'fingerprinted','evidence':[]}
        objects[h]=info
        strings=[]
        for m in re.finditer(rb'[\x20-\x7e]{5,}',b): strings.append({'offset':hex(m.start()),'encoding':'ascii','text':m.group().decode('ascii')})
        for m in re.finditer(rb'(?:[\x20-\x7e]\x00){5,}',b): strings.append({'offset':hex(m.start()),'encoding':'utf-16-le','text':m.group().decode('utf-16-le')})
        write_json(d/'strings.json',strings); info['strings']=len(strings); info['evidence'].append('strings.json')
        ext=p.suffix.lower()
        try:
            if b[:2]==b'MZ':
                pe=pefile.PE(data=b); pe.parse_data_directories()
                imports=[]
                for kind,attr in [('normal','DIRECTORY_ENTRY_IMPORT'),('delay','DIRECTORY_ENTRY_DELAY_IMPORT')]:
                    for entry in getattr(pe,attr,[]): imports.append({'kind':kind,'module':entry.dll.decode(errors='replace'),'symbols':[{'name':x.name.decode(errors='replace') if x.name else None,'ordinal':x.ordinal,'address':hex(x.address)} for x in entry.imports]})
                exports=[{'name':e.name.decode(errors='replace') if e.name else None,'ordinal':e.ordinal,'rva':hex(e.address),'forwarder':e.forwarder.decode(errors='replace') if e.forwarder else None} for e in getattr(getattr(pe,'DIRECTORY_ENTRY_EXPORT',None),'symbols',[])]
                resources=[]
                def walk(node,ids=[]):
                    for ent in node.entries:
                        route=ids+[str(ent.name) if ent.name else ent.id]
                        if hasattr(ent,'directory'): walk(ent.directory,route)
                        else:
                            raw=pe.get_data(ent.data.struct.OffsetToData,ent.data.struct.Size); rh=digest(raw)
                            rp=d/'resources'/('_'.join(map(str,route))+'.bin');rp.parent.mkdir(exist_ok=True);rp.write_bytes(raw)
                            resources.append({'ids':route,'rva':hex(ent.data.struct.OffsetToData),'bytes':len(raw),'sha256':rh,'path':str(rp)})
                if hasattr(pe,'DIRECTORY_ENTRY_RESOURCE'): walk(pe.DIRECTORY_ENTRY_RESOURCE)
                sections=[{'name':s.Name.rstrip(b'\0').decode(errors='replace'),'rva':hex(s.VirtualAddress),'rawOffset':s.PointerToRawData,'rawBytes':s.SizeOfRawData,'virtualBytes':s.Misc_VirtualSize,'entropy':s.get_entropy(),'characteristics':hex(s.Characteristics)} for s in pe.sections]
                overlay=pe.get_overlay_data_start_offset(); cert=pe.OPTIONAL_HEADER.DATA_DIRECTORY[4]
                pi={'machine':hex(pe.FILE_HEADER.Machine),'imageBase':hex(pe.OPTIONAL_HEADER.ImageBase),'entryRva':hex(pe.OPTIONAL_HEADER.AddressOfEntryPoint),'timestamp':pe.FILE_HEADER.TimeDateStamp,'sections':sections,'imports':imports,'exports':exports,'resources':resources,'overlayOffset':overlay,'overlayBytes':len(b)-overlay if overlay else 0,'certificateFileOffset':cert.VirtualAddress,'certificateBytes':cert.Size,'warnings':pe.get_warnings()}
                write_json(d/'pe.json',pi); info.update({'kind':'PE','machine':pi['machine'],'status':'structurally parsed','imports':len(imports),'exports':len(exports),'resources':len(resources)});info['evidence'].append('pe.json')
                modes={0x14c:(capstone.CS_ARCH_X86,capstone.CS_MODE_32),0x8664:(capstone.CS_ARCH_X86,capstone.CS_MODE_64),0xaa64:(capstone.CS_ARCH_ARM64,capstone.CS_MODE_ARM)}
                if pe.FILE_HEADER.Machine in modes:
                    cs=capstone.Cs(*modes[pe.FILE_HEADER.Machine]); va=pe.OPTIONAL_HEADER.ImageBase+pe.OPTIONAL_HEADER.AddressOfEntryPoint
                    asm=[f'{i.address:x}\t{i.bytes.hex()}\t{i.mnemonic}\t{i.op_str}' for i in cs.disasm(pe.get_data(pe.OPTIONAL_HEADER.AddressOfEntryPoint,256),va)]
                    (d/'entry-capstone.asm').write_text('\n'.join(asm),encoding='utf-8'); info['evidence'].append('entry-capstone.asm')
                dest=out/'native-inputs'/(h[:12]+'__'+p.name);dest.parent.mkdir(exist_ok=True);shutil.copyfile(snap,dest);native.append({'sha256':h,'name':p.name,'path':str(dest),'machine':pi['machine']})
            elif ext in {'.crx','.xpi','.nex','.zip','.jar','.chm'} or b[:4]==b'PK\x03\x04':
                info['kind']='archive'; dest=out/'expanded'/h; dest.mkdir(parents=True,exist_ok=True)
                if depth>=8: raise ValueError('archive nesting limit reached; not silently omitted')
                if ext=='.chm':
                    check=subprocess.run([a.sevenzip,'t',str(snap)],capture_output=True,text=True,errors='replace');(d/'archive-test.txt').write_text(check.stdout+check.stderr,encoding='utf-8')
                    listing=subprocess.run([a.sevenzip,'l','-slt',str(snap)],capture_output=True,text=True,errors='replace');(d/'archive-list.txt').write_text(listing.stdout+listing.stderr,encoding='utf-8')
                    for name in re.findall(r'^Path = (.+)$',listing.stdout,flags=re.M)[1:]:
                        q=pathlib.PurePosixPath(name.replace('\\','/'))
                        if q.is_absolute() or '..' in q.parts or ':' in name: raise ValueError('unsafe archive member path')
                    run=subprocess.run([a.sevenzip,'x','-y','-o'+str(dest),str(snap)],capture_output=True,text=True,errors='replace');(d/'archive-extract.txt').write_text(run.stdout+run.stderr,encoding='utf-8')
                    if check.returncode or run.returncode: raise ValueError(f'7-Zip test={check.returncode} extract={run.returncode}')
                else:
                    offset=0
                    if b[:4]==b'Cr24':
                        ver=struct.unpack_from('<I',b,4)[0]
                        offset=12+struct.unpack_from('<I',b,8)[0] if ver==3 else 16+sum(struct.unpack_from('<II',b,8)) if ver==2 else -1
                        if offset<0: raise ValueError('unknown CRX version')
                        info['crxVersion']=ver;info['zipOffset']=offset
                    with zipfile.ZipFile(io.BytesIO(b[offset:])) as z:
                        if sum(x.file_size for x in z.infolist())>512*1024*1024: raise ValueError('archive size limit')
                        for member in z.infolist():
                            q=pathlib.PurePosixPath(member.filename.replace('\\','/'))
                            if q.is_absolute() or '..' in q.parts or ':' in member.filename: raise ValueError('unsafe archive member')
                            target=dest.joinpath(*q.parts)
                            if member.is_dir(): target.mkdir(parents=True,exist_ok=True);continue
                            target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(z.read(member))
                            archive_members.append({'archiveSha256':h,'path':member.filename,'bytes':member.file_size,'crc32':hex(member.CRC)})
                for child in sorted(dest.rglob('*')):
                    if child.is_file(): inspect(child,'archive:'+h,child.relative_to(dest).as_posix(),depth+1)
                info['status']='expanded and children inspected';info['expanded']=str(dest)
            elif ext in {'.lng','.txt','.inf','.json','.htm','.html','.js','.css','.tbi','.log','.rdf','.xml','.idl','.manifest','.hhc','.hhk','.xul','.dtd','.properties','.mf','.sf'}:
                text,enc=decode(b); (d/'decoded.txt').write_text(text,encoding='utf-8'); info.update({'kind':'text','status':'text decoded','encoding':enc,'lines':len(text.splitlines())}); info['evidence'].append('decoded.txt')
                keys=re.findall(r'^\s*([^;#\r\n=]+)=(.*)$',text,re.M)
                schema={'sections':re.findall(r'^\s*\[([^\]\r\n]+)\]',text,re.M),'keyCounts':dict(collections.Counter(k.strip() for k,v in keys)),'urls':sorted(set(re.findall(r'https?://[^\s<>"\x00]+',text)))}
                if ext=='.json': schema['json']=json.loads(text)
                if ext=='.tbi': schema['values']={k.strip():v.strip() for k,v in keys}
                if ext=='.js': schema['apiCalls']=dict(collections.Counter(re.findall(r'\b(?:chrome|browser)\.([\w.]+)',text)))
                write_json(d/'text-structure.json',schema);info['evidence'].append('text-structure.json')
            elif b[:2]==b'BM':
                width,height=struct.unpack_from('<ii',b,18); info.update({'kind':'bitmap','status':'header parsed','width':width,'height':height,'bitsPerPixel':struct.unpack_from('<H',b,28)[0]})
            elif b[:8]==b'\x89PNG\r\n\x1a\n':
                width,height=struct.unpack_from('>II',b,16);info.update({'kind':'image','status':'PNG header parsed','width':width,'height':height})
            elif b[:6] in {b'GIF87a',b'GIF89a'}:
                width,height=struct.unpack_from('<HH',b,6);info.update({'kind':'image','status':'GIF header parsed','width':width,'height':height})
            elif b.startswith(b'XPCOM\nTypeLib\r\n\x1a'):
                info.update({'kind':'XPCOM type library','status':'header identified; interface decoding pending'})
            elif b[:4] in {b'MSFT',b'SLTG'}: info.update({'kind':'COM type library','status':'header identified; COM metadata pass required'})
            elif ext=='.cat': info.update({'kind':'signed catalog','status':'fingerprinted; certificate/catalog pass required'})
            elif ext=='.dat': info.update({'kind':'opaque data','status':'strings and entropy only; format unresolved'})
            else: info.update({'kind':'other resource','status':'fingerprinted and strings inspected'})
        except Exception as e: info['analysisError']=str(e);error('analyze',p,e)
        write_json(d/'summary.json',info)
    for raw in a.root:
        root=pathlib.Path(raw).resolve()
        if not root.exists(): error('root',root,'not found');continue
        if root.is_file(): inspect(root,str(root.parent),root.name);continue
        for folder,dirs,files in os.walk(root,followlinks=False,onerror=lambda e:error('walk',root,e)):
            for name in list(dirs):
                p=pathlib.Path(folder)/name
                if p.is_symlink() or bool(getattr(p.lstat(),'st_file_attributes',0)&0x400): links.append(str(p));dirs.remove(name)
            for name in sorted(files):
                p=pathlib.Path(folder)/name;inspect(p,str(root),p.relative_to(root).as_posix())
    write_json(out/'inventory.json',{'createdUtc':datetime.now(timezone.utc).isoformat(),'roots':a.root,'files':records,'objects':list(objects.values()),'errors':errors,'reparseDirectoriesNotFollowed':links,'nativeInputs':native,'archiveMembers':archive_members,'tools':{'python':sys.version,'pefile':pefile.__version__,'capstone':capstone.__version__}})
    with (out/'file-coverage.csv').open('w',newline='',encoding='utf-8') as f:
        w=csv.DictWriter(f,fieldnames=['origin','path','sha256','bytes','kind','status','analysisError']);w.writeheader()
        for r in records: w.writerow({**{k:r[k] for k in ['origin','path','sha256','bytes']},**{k:objects[r['sha256']].get(k,'') for k in ['kind','status','analysisError']}})
    print(json.dumps({'fileOccurrences':len(records),'uniqueObjects':len(objects),'nativeInputs':len(native),'errors':errors,'kinds':dict(collections.Counter(x.get('kind','error') for x in objects.values()))},indent=2))
if __name__=='__main__': main()
