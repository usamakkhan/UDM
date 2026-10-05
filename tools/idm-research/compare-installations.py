"""Compare setup and installed-root contents using the hash-indexed inventory."""
import argparse, collections, json, pathlib

def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8')
def main():
    ap=argparse.ArgumentParser();ap.add_argument('evidence');ap.add_argument('--setup',required=True);ap.add_argument('--installed-root',default=r'C:\Program Files (x86)\Internet Download Manager');a=ap.parse_args();r=pathlib.Path(a.evidence);setup=pathlib.Path(a.setup);inv=read(r/'inventory.json');objs={o['sha256']:o for o in inv['objects']}
    def files(origin):return {f['path'].casefold():f for f in inv['files'] if f['origin'].casefold()==origin.casefold()}
    old=files(a.installed_root);new=files(str(setup/'payload'));rows=[];sections=[];strings=[]
    if not old or not new:raise ValueError('Comparison roots absent from inventory')
    for k,f in sorted(new.items()):
        previous=old.get(k);status='setup-only' if previous is None else 'same' if f['sha256']==previous['sha256'] else 'changed'
        rows.append({'path':f['path'],'status':status,'sha256':f['sha256'],'installedSha256':previous['sha256'] if previous else None})
        if status!='changed' or objs[f['sha256']]['kind']!='PE':continue
        now=pathlib.Path(objs[f['sha256']]['snapshot']);before=pathlib.Path(objs[previous['sha256']]['snapshot']);nb=now.read_bytes();ob=before.read_bytes();np=read(now.parent/'pe.json');op=read(before.parent/'pe.json');byname={s['name']:s for s in op['sections']};diff=[]
        for ns in np['sections']:
            os=byname.get(ns['name']);n=nb[ns['rawOffset']:ns['rawOffset']+ns['rawBytes']];o=ob[os['rawOffset']:os['rawOffset']+os['rawBytes']] if os else b''
            diff.append({'section':ns['name'],'sameBytes':n==o,'oldRva':os['rva'] if os else None,'newRva':ns['rva'],'newBytes':len(n),'oldBytes':len(o),'differingPositions':sum(x!=y for x,y in zip(n,o))+abs(len(n)-len(o))})
        sections.append({'name':f['path'],'oldSha256':previous['sha256'],'newSha256':f['sha256'],'sections':diff,'limitation':'Raw section differences include relocation/address shifts and do not measure semantic change.'})
        ns={x['text'] for x in read(now.parent/'strings.json')};os={x['text'] for x in read(before.parent/'strings.json')}
        strings.append({'name':f['path'],'oldSha256':previous['sha256'],'newSha256':f['sha256'],'added':sorted(ns-os),'removed':sorted(os-ns),'limitation':'Presence/absence of raw strings is evidence to trace, not a behavioral conclusion.'})
    report={'files':rows,'installedOnly':[f['path'] for k,f in sorted(old.items()) if k not in new]}
    write(setup/'installed-comparison.json',report);write(setup/'section-diff.json',sections);write(r/'build11-string-diff.json',strings)
    print(json.dumps({'payloadFiles':len(rows),'statuses':dict(collections.Counter(x['status'] for x in rows)),'installedOnly':report['installedOnly'],'nativeChanges':len(sections)}))
if __name__=='__main__':main()
