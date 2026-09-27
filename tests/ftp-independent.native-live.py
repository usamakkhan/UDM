"""Interoperability with pyftpdlib (test dependency only, never shipped with UDM)."""
import base64, hashlib, json, logging, os, subprocess, sys, threading, time
from pathlib import Path
if os.environ.get('UDM_FTP_TEST_PACKAGES'):
    sys.path.insert(0, os.environ['UDM_FTP_TEST_PACKAGES'])
import pyftpdlib
from pyftpdlib.authorizers import DummyAuthorizer
from pyftpdlib.handlers import FTPHandler, ThrottledDTPHandler
from pyftpdlib.servers import FTPServer

root=Path(sys.argv[1]).resolve();root.mkdir(parents=True,exist_ok=True)
exe=Path(os.environ.get('UDM_TEST_EXE',str(Path(__file__).resolve().parent.parent/'release-native/Udm.NativeTests.exe')))
logging.basicConfig(filename=root/'server.log',level=logging.INFO)
source=root/'server-files';source.mkdir(exist_ok=True)
body=bytes((n*17+n//173)%251 for n in range(8*1024*1024+41));name='fixture space.bin';(source/name).write_bytes(body)
expected=hashlib.sha256(body).hexdigest()
auth=DummyAuthorizer();auth.add_user('fixture-user','fixture-password',str(source),perm='elr')
class Data(ThrottledDTPHandler):
    write_limit=512*1024
class Handler(FTPHandler):
    authorizer=auth
    dtp_handler=Data
    use_sendfile=False
    commands=[]
    offsets=[]
    def pre_process_command(self,line,cmd,arg):
        self.commands.append(cmd)
        return super().pre_process_command(line,cmd,arg)
    def ftp_REST(self,line):
        self.offsets.append(int(line))
        return super().ftp_REST(line)
server=FTPServer(('127.0.0.1',0),Handler);server.max_cons=64;server.max_cons_per_ip=32
port=server.socket.getsockname()[1]
worker=threading.Thread(target=server.serve_forever,kwargs={'timeout':0.05,'handle_exit':False},daemon=True);worker.start()
results=[]
def run(test,**spec):
    folder=root/test;folder.mkdir(exist_ok=True)
    value={'url':f'ftp://127.0.0.1:{port}/fixture%20space.bin','connections':4,'headers':{'Authorization':'Basic '+base64.b64encode(b'fixture-user:fixture-password').decode()},**spec}
    (folder/'input.json').write_text(json.dumps(value));start=time.perf_counter()
    p=subprocess.run([str(exe),'--feature-spec',str(folder/'input.json')],capture_output=True,timeout=60,creationflags=subprocess.CREATE_NO_WINDOW)
    (folder/'stdout.log').write_bytes(p.stdout);(folder/'stderr.log').write_bytes(p.stderr)
    assert p.returncode==0,p.stderr
    result=json.loads((folder/'result.json').read_text(encoding='utf-8-sig'));result['wallSeconds']=time.perf_counter()-start
    return result
def exact(result):
    assert result['status']=='Complete',result
    assert result['sha256']==expected,result
    assert Path(result['path']).read_bytes()==body
def check(name,fn):
    try:
        detail=fn();results.append({'name':name,'passed':True,'detail':detail});print('PASS',name,flush=True)
    except Exception as e:
        results.append({'name':name,'passed':False,'error':str(e)});print('FAIL',name,str(e),flush=True)
def normal(active=False):
    before=len(Handler.commands);r=run('active' if active else 'passive',passive=not active);exact(r)
    assert r['segments']==4 and r['rangeSupported'];assert ('EPRT' if active else 'EPSV') in Handler.commands[before:]
    return r
def resumed():
    first=run('resumed',cancelMs=350);assert first['status']=='Paused' and 0<first['bytes']<len(body),first
    before=len(Handler.offsets);r=run('resumed',resume=True);exact(r);assert any(n>0 for n in Handler.offsets[before:]);return {'paused':first,'resumed':r}
try:
    check('pyftpdlib parallel passive FTP',normal)
    check('pyftpdlib parallel active FTP',lambda:normal(True))
    check('pyftpdlib pause and process restart',resumed)
finally:
    server.close_all();worker.join(3)
    report={'server':'pyftpdlib','version':pyftpdlib.__ver__,'passed':sum(r['passed'] for r in results),'failed':sum(not r['passed'] for r in results),'results':results}
    (root/'results.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:v for k,v in report.items() if k!='results'}))
    if report['failed']:sys.exit(1)
