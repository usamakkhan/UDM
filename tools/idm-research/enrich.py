"""Build cross-file evidence from the complete inventory, without running target code."""
import argparse,collections,hashlib,json,pathlib,re,struct
def save(p,x):p.write_text(json.dumps(x,indent=2,ensure_ascii=True)+'\n',encoding='utf-8')
def main():
    ap=argparse.ArgumentParser();ap.add_argument('evidence');a=ap.parse_args();root=pathlib.Path(a.evidence);inv=json.loads((root/'inventory.json').read_text());objs=inv['objects'];by_hash={x['sha256']:x for x in objs}
    names={x['name'].lower() for x in objs};consumers=[];peers=[];resources=[];typelibs={};packages=[]
    categories={'network':r'^(WSA|recv|send|socket|connect|bind|listen|accept|Internet|WinHttp|Http|Ftp|Dns)', 'filesystem':r'^(CreateFile|ReadFile|WriteFile|MapView|CreateFileMapping|SetFile|MoveFile|DeleteFile|FindFirstFile)', 'registry':r'^Reg', 'process':r'^(CreateProcess|OpenProcess|ReadProcessMemory|WriteProcessMemory|VirtualAllocEx|CreateRemoteThread|ShellExecute)', 'hooks':r'^(SetWindowsHook|CallNextHook|UnhookWindows)', 'service_driver':r'^(SetupDi|SetupInstall|OpenSCManager|CreateService|StartService|ControlService|DeviceIoControl|Fwpm|Fwps|IoCreate|PsSet)', 'com':r'^(Co|Ole|LoadTypeLib|RegisterTypeLib)', 'crypto':r'^(Crypt|Cert|BCrypt|NCrypt|SSL_|EVP_)'}
    for obj in objs:
        d=pathlib.Path(obj['snapshot']).parent
        if obj['kind']=='COM type library':typelibs[obj['sha256']]={'sha256':obj['sha256'],'path':obj['snapshot'],'owners':[obj['name']]}
        if obj['kind']=='PE':
            pe=json.loads((d/'pe.json').read_text());caps=collections.defaultdict(list)
            for imp in pe['imports']:
                for sym in imp['symbols']:
                    for cat,pat in categories.items():
                        if sym['name'] and re.search(pat,sym['name'],re.I):caps[cat].append(imp['module']+'!'+sym['name'])
            peers.append({'name':obj['name'],'sha256':obj['sha256'],'machine':pe['machine'],'entryRva':pe['entryRva'],'exports':pe['exports'],'importEvidence':dict(caps),'resourceTypes':dict(collections.Counter(str(x['ids'][0]) for x in pe['resources'])),'limitation':'Imports identify possible API use, not observed runtime behavior.'})
            for r in pe['resources']:
                p=pathlib.Path(r['path']);b=p.read_bytes();row={'owner':obj['name'],'ownerSha256':obj['sha256'],**r,'interpretation':'raw resource retained'}
                if b[:4] in (b'MSFT',b'SLTG'):
                    entry=typelibs.setdefault(r['sha256'],{'sha256':r['sha256'],'path':str(p),'owners':[]});entry['owners'].append(obj['name']+':'+str(r['ids']));row['interpretation']='COM type library'
                elif r['ids'][0] in ('REGISTRY',24):
                    enc='utf-16' if b.startswith(b'\xff\xfe') else 'utf-16-le' if b[:100].count(b'\0')>15 else 'utf-8'
                    text=b.decode(enc,errors='replace');p.with_suffix('.txt').write_text(text,encoding='utf-8');row['interpretation']='registration or application manifest decoded'
                elif r['ids'][0]==6:
                    cur=0;entries=[]
                    try:
                        for i in range(16):
                            n=struct.unpack_from('<H',b,cur)[0];cur+=2;s=b[cur:cur+2*n].decode('utf-16-le');cur+=2*n
                            if s:entries.append({'id':(int(r['ids'][1])-1)*16+i,'value':s})
                        save(p.with_suffix('.json'),entries);row['interpretation']='all string-table entries decoded';row['strings']=len(entries)
                    except Exception as e:row['error']=str(e)
                elif r['ids'][0] in (2,240,241) and len(b)>=16:
                    size=struct.unpack_from('<I',b,0)[0]
                    if size in (40,52,56,108,124):row['interpretation']='DIB bitmap header';row['width'],row['height']=struct.unpack_from('<ii',b,4)
                resources.append(row)
            for s in json.loads((d/'strings.json').read_text()):
                value=s['text'].lower()
                for match in re.findall(r'[a-z0-9_.-]+\.(?:dll|exe|sys|dat|tlb|chm|inf|json|xpi|crx|nex)',value):
                    if match in names:consumers.append({'consumer':obj['name'],'consumerSha256':obj['sha256'],'referencedFile':match,'offset':s['offset'],'encoding':s['encoding']})
        if obj['kind']=='archive':
            expanded=pathlib.Path(obj['expanded']);manifest=expanded/'manifest.json';record={'name':obj['name'],'sha256':obj['sha256'],'children':len([f for f in inv['files'] if f['origin']=='archive:'+obj['sha256']])}
            if manifest.exists():
                m=json.loads(manifest.read_text(encoding='utf-8-sig'));record['manifest']={k:m.get(k) for k in ('name','version','manifest_version','permissions','host_permissions','background','content_scripts','browser_specific_settings','applications') if k in m}
            packages.append(record)
    save(root/'component-evidence.json',peers);save(root/'file-consumers.json',consumers);save(root/'resource-coverage.json',resources);save(root/'type-library-inputs.json',list(typelibs.values()));save(root/'package-evidence.json',packages)
    print(json.dumps({'components':len(peers),'resources':len(resources),'typeLibraries':len(typelibs),'fileReferences':len(consumers),'packages':len(packages)}))
if __name__=='__main__':main()
