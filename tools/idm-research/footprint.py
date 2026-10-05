"""Inventory IDM state without copying history, credentials, or download payloads."""
import argparse,collections,hashlib,json,os,pathlib,winreg
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--root',action='append',required=True);ap.add_argument('--output',required=True);a=ap.parse_args();files=[];errors=[];registry=[]
    for raw in a.root:
        root=pathlib.Path(raw)
        for folder,dirs,names in os.walk(root,followlinks=False,onerror=lambda e:errors.append(str(e))):
            dirs[:]=[n for n in dirs if not bool(getattr((pathlib.Path(folder)/n).lstat(),'st_file_attributes',0)&0x400)]
            for name in names:
                p=pathlib.Path(folder)/name
                try:
                    st=p.stat();h=hashlib.sha256()
                    with p.open('rb') as f:
                        while chunk:=f.read(1024*1024):h.update(chunk)
                    after=p.stat();rel=p.relative_to(root).as_posix()
                    files.append({'root':str(root),'path':rel,'bytes':st.st_size,'sha256':h.hexdigest(),'changedDuringRead':st.st_mtime_ns!=after.st_mtime_ns or st.st_size!=after.st_size,'role':'download data/state' if rel.lower().startswith('dwnldata/') else 'application support/state','contentCopied':False})
                except Exception as e:errors.append({'path':str(p),'error':str(e)})
    def walkreg(path):
        try:
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER,path) as k:
                sub,vals,stamp=winreg.QueryInfoKey(k);row={'path':'HKCU\\'+path,'valueCount':vals,'subkeyCount':sub,'values':[]}
                for i in range(vals):
                    name,value,typ=winreg.EnumValue(k,i);row['values'].append({'name':name,'type':typ,'length':len(value) if isinstance(value,(str,bytes,list)) else None})
                registry.append(row)
                for i in range(sub):walkreg(path+'\\'+winreg.EnumKey(k,i))
        except OSError as e:errors.append({'registry':path,'error':str(e)})
    walkreg('Software\\DownloadManager')
    report={'roots':a.root,'files':files,'registrySchema':registry,'errors':errors,'policy':'Files fingerprinted without content copies; registry values omitted. A running application may change state after collection.'}
    pathlib.Path(a.output).write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print(json.dumps({'files':len(files),'bytes':sum(x['bytes'] for x in files),'registryKeys':len(registry),'errors':len(errors)}))
if __name__=='__main__':main()
