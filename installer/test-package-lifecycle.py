"""Exercise production installer payload/logic with private paths and HKCU roots."""
import argparse,hashlib,json,os,re,shutil,subprocess,uuid,winreg
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--stage',required=True);p.add_argument('--compiler',required=True);p.add_argument('--root',required=True);args=p.parse_args()
root=Path(args.root).resolve();assert not root.exists();root.mkdir(parents=True)
stage=Path(args.stage).resolve();repo=Path(__file__).resolve().parents[1];source=root/'source';source.mkdir();checks=[]
identity=uuid.uuid4().hex;prefix='Software\\UDMInstallerFixtures\\'+identity;lock='Local\\UDMPackageFixture'+identity
app=root/'installed';history=root/'custom-history';fallback=root/'default-history';old=root/'old-app';flags=subprocess.CREATE_NO_WINDOW

def check(value,name):
 checks.append({'name':name,'passed':bool(value)})
 if not value:raise AssertionError(name)
 print('PASS',name,flush=True)
def digest(file):return hashlib.sha256(file.read_bytes()).hexdigest()
def run(exe,label,*extra):
 command=[str(exe),'/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-',f'/DIR={app}',f'/UDMLOCKNAME={lock}',f'/LOG={root/(label+".log")}',*extra]
 completed=subprocess.run(command,creationflags=flags)
 check(completed.returncode==0,label+' exits successfully')
def registry_paths():
 for scope in range(3):
  for vendor in ['Google\\Chrome','Microsoft\\Edge','Chromium','Mozilla']:
   yield prefix+f'\\Scope{scope}\\SOFTWARE\\'+vendor+'\\NativeMessagingHosts\\com.udm.download_manager',vendor

def remove_private_registry(key):
 assert key==prefix or key.startswith(prefix+'\\')
 try:
  with winreg.OpenKey(winreg.HKEY_CURRENT_USER,key,0,winreg.KEY_READ|winreg.KEY_WRITE) as h:
   children=[]
   while True:
    try:children.append(winreg.EnumKey(h,len(children)))
    except OSError:break
  for child in children:remove_private_registry(key+'\\'+child)
  winreg.DeleteKey(winreg.HKEY_CURRENT_USER,key)
 except FileNotFoundError:pass
try:
 for name in ['release','browser']:shutil.copytree(stage/name,source/name,copy_function=os.link)
 shutil.copytree(repo/'installer',source/'installer')
 for name in ['LICENSE','NOTICE']:shutil.copy2(stage/name,source/name)
 script=source/'installer/UDM-Setup.iss';text=script.read_text(encoding='utf-8')
 text='#define UDM_NATIVE_REGISTRATION_TEST\n'+text
 replacements={'AppId':identity,'AppName':'UDM isolated package lifecycle','PrivilegesRequired':'lowest','Compression':'none','SolidCompression':'no','OutputDir':'../fixture-out','OutputBaseFilename':'PackageFixture'}
 for key,value in replacements.items():text=re.sub(r'^'+key+r'=.*$',key+'='+value,text,flags=re.M)
 text=text.replace('[Files]','CreateUninstallRegKey=no\n\n[Files]',1)
 text=re.sub(r'\[Icons\].*?(?=\[Code\])','',text,flags=re.S)
 text=text.replace("ExpandConstant('{localappdata}\\UDM')","'"+str(fallback).replace("'","''")+"'")
 script.write_text(text,encoding='utf-8')
 lifecycle=source/'installer/InstallerLifecycle.iss';text=lifecycle.read_text()
 marker=' Result := False;';assert marker in text;text=text.replace(marker," NativeTestPrefix := '"+prefix+"';\n"+marker,1);lifecycle.write_text(text)
 with (root/'compile.log').open('w') as log:
  result=subprocess.run([args.compiler,'/DAppVersion=0.85.0','/DBrowserVersion='+json.loads((source/'browser/chromium/manifest.json').read_text())['version'],str(script)],stdout=log,stderr=subprocess.STDOUT,creationflags=flags)
 check(result.returncode==0,'Production-derived package fixture compiles')
 history.mkdir();old.mkdir();(history/'state.json').write_text(json.dumps({'Downloads':[{'Id':'retained','Status':'Paused','Received':7}],'Queues':[{'Name':'Main queue'}]}));(history/'retained.part').write_bytes(b'partial');(old/'udm-data.json').write_text(json.dumps({'dataDirectory':str(history)}));os.link(stage/'release/Udm.NativeHost.exe',old/'Udm.NativeHost.exe')
 oldmanifest=old/'host.json';oldmanifest.write_text(json.dumps({'name':'com.udm.download_manager','type':'stdio','path':str(old/'Udm.NativeHost.exe')}))
 for key,_ in registry_paths():
  with winreg.CreateKey(winreg.HKEY_CURRENT_USER,key) as h:winreg.SetValueEx(h,'',0,winreg.REG_SZ,str(oldmanifest))
 retained={str(f):digest(f) for f in history.iterdir()}
 setup=source/'fixture-out/PackageFixture.exe';run(setup,'install')
 config=app/'udm-data.json';check(json.loads(config.read_text())['dataDirectory'].lower()==str(history).lower(),'Install preserves registered custom history location')
 for key,vendor in registry_paths():
  with winreg.OpenKey(winreg.HKEY_CURRENT_USER,key) as h:value=winreg.QueryValueEx(h,'')[0]
  check(value.lower()==str(app/'native-messaging'/('firefox.json' if vendor=='Mozilla' else 'chromium.json')).lower(),'Install publishes correct private native-host registration')
 check(all(digest(Path(f))==sha for f,sha in retained.items()),'Install preserves history and partial bytes')
 config_before=config.read_bytes();repair=app/'browser/chromium/recovery.js';expected=digest(source/'browser/chromium/recovery.js')
 # Delete only this fixture-owned installed file, after checking its absolute parent.
 assert repair.resolve().is_relative_to(app.resolve()) and app.resolve().is_relative_to(root)
 repair.unlink();run(setup,'repair')
 check(digest(repair)==expected,'Repair restores missing browser recovery script exactly')
 check(config.read_bytes()==config_before,'Repair preserves custom history configuration bytes')
 check(all(digest(Path(f))==sha for f,sha in retained.items()),'Repair preserves history and partial bytes')
 run(app/'unins000.exe','uninstall')
 check(not (app/'UDM.exe').exists() and not repair.exists(),'Uninstall removes owned application and browser payload')
 for key,_ in registry_paths():
  exists=False
  try:
   with winreg.OpenKey(winreg.HKEY_CURRENT_USER,key) as h:winreg.QueryValueEx(h,'');exists=True
  except FileNotFoundError:pass
  check(not exists,'Uninstall removes its private native-host registration')
 check(all(digest(Path(f))==sha for f,sha in retained.items()),'Uninstall preserves history and partial bytes')
finally:
 remove_private_registry(prefix)
 (root/'results.json').write_text(json.dumps({'checks':checks,'registryPrefix':prefix,'registryRemoved':True,'scope':'Production payload and shared installer logic; lowest privilege, private paths/registry, no shortcuts or launch, unique AppId. Not machine-wide production installer acceptance.'},indent=2))
