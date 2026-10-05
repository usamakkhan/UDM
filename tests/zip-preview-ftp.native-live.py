"""Native remote ZIP interoperability with pyftpdlib; test-only loopback servers."""
import base64, json, logging, os, shutil, subprocess, sys, threading, time
from pathlib import Path
if os.environ.get('UDM_FTP_TEST_PACKAGES'):
    sys.path.insert(0, os.environ['UDM_FTP_TEST_PACKAGES'])
import pyftpdlib
from pyftpdlib.authorizers import DummyAuthorizer
from pyftpdlib.handlers import FTPHandler
from pyftpdlib.servers import FTPServer
from support.socks_testserver import SocksServer

root=Path(sys.argv[1]).resolve(); root.mkdir(parents=True,exist_ok=True)
fixtures=Path(sys.argv[2]); exe=os.environ['UDM_TEST_EXE']
logging.basicConfig(filename=root/'server.log',level=logging.INFO)
files=root/'server-files';files.mkdir(exist_ok=True)
for name in ('small','large','empty'):
    shutil.copy2(fixtures/(name+'.zip'), files/(name+'.zip'))
auth=DummyAuthorizer();auth.add_user('fixture-user','fixture-password',str(files),perm='elr')
class Handler(FTPHandler):
    authorizer=auth
    passive_ports=range(41200,41300)
    auth_failed_timeout=0
    use_sendfile=False
    mode='normal'
    commands=[]
    offsets=[]
    metadata_count=0
    def pre_process_command(self,line,cmd,arg):
        self.commands.append(cmd)
        return super().pre_process_command(line,cmd,arg)
    def ftp_REST(self,line):
        self.offsets.append(int(line))
        if self.mode=='no-rest':return self.respond('502 Restart unsupported')
        return super().ftp_REST(line)
    def ftp_MDTM(self,path):
        Handler.metadata_count+=1
        if self.mode=='changed' and Handler.metadata_count>1:
            return self.respond('213 20000101000000')
        return super().ftp_MDTM(path)
server=FTPServer(('127.0.0.1',0),Handler);server.max_cons=64;server.max_cons_per_ip=32
port=server.socket.getsockname()[1]
worker=threading.Thread(target=server.serve_forever,kwargs={'timeout':0.05,'handle_exit':False},daemon=True);worker.start()
proxy=SocksServer(port,Handler.passive_ports);results=[]
def run(name,file='small',expect='Ready',mode='normal',**options):
    folder=root/name;folder.mkdir(exist_ok=True);Handler.mode=mode;Handler.metadata_count=0
    before=len(Handler.commands);offset_start=len(Handler.offsets);route_start=len(proxy.destinations)
    spec={'url':f'ftp://127.0.0.1:{port}/{file}.zip','zipPreview':True,'headers':{'Authorization':'Basic '+base64.b64encode(b'fixture-user:fixture-password').decode()},**options}
    (folder/'input.json').write_text(json.dumps(spec))
    try:
        start=time.perf_counter()
        p=subprocess.run([exe,'--feature-spec',str(folder/'input.json')],capture_output=True,timeout=40,creationflags=subprocess.CREATE_NO_WINDOW)
        (folder/'stdout.log').write_bytes(p.stdout);(folder/'stderr.log').write_bytes(p.stderr)
        assert p.returncode==0,p.stderr
        r=json.loads((folder/'result.json').read_text(encoding='utf-8-sig'))
        assert r['jobUnchanged'] and not r['fileExists'] and not r['partsExist'],r
        assert r['preview']['Status']==expect,r
        if expect=='Ready':
            expected=[] if file=='empty' else json.loads((fixtures/(file+'.json')).read_text(encoding='utf-8'))
            assert r['preview']['Entries']==expected,r
            assert r['preview']['Received']<150000,r
            assert not r['preview']['Validated'],r
        if name=='active':assert 'EPRT' in Handler.commands[before:]
        if name=='large':assert any(n>2**20 for n in Handler.offsets[offset_start:])
        if name.startswith('socks'):
            assert len(proxy.destinations)>route_start and not proxy.errors,proxy.errors
        results.append({'name':name,'passed':True,'result':r,'wallSeconds':time.perf_counter()-start,'commands':Handler.commands[before:],'offsets':Handler.offsets[offset_start:],'routes':proxy.destinations[route_start:]})
        print('PASS',name,flush=True)
    except Exception as e:
        results.append({'name':name,'passed':False,'error':repr(e)});print('FAIL',name,repr(e),flush=True)
try:
    run('passive');run('active',passive=False);run('large',file='large');run('empty',file='empty')
    for version in (4,5):
        run('socks'+str(version),url=f'ftp://udm-independent.invalid:{port}/small.zip',proxy=proxy.settings(version))
    run('no-rest-small',mode='no-rest')
    run('no-rest-large',file='large',mode='no-rest',expect='Error')
    run('changed-metadata',mode='changed',expect='Error')
    run('wrong-login',headers={'Authorization':'Basic '+base64.b64encode(b'fixture-user:wrong').decode()},expect='Error')
finally:
    proxy.close();server.close_all();worker.join(3)
    report={'server':'pyftpdlib','version':pyftpdlib.__ver__,'passed':sum(r['passed'] for r in results),'failed':sum(not r['passed'] for r in results),'results':results}
    (root/'results.json').write_text(json.dumps(report,indent=2))
    if report['failed']:sys.exit(1)
