import json,os,pathlib,struct,subprocess,sys,time,uuid
root=pathlib.Path(sys.argv[1]);root.mkdir(exist_ok=False);release=pathlib.Path(sys.argv[2]);tag='rendered'+uuid.uuid4().hex[:16];pid=uuid.uuid4().hex;url='https://example.test/page';checks=[]
state=root/'state';state.mkdir();(state/'state.json').write_text(json.dumps({'Schema':1,'Settings':{},'Downloads':[],'Queues':[{'Name':'Main queue'}],'Projects':[{'Id':pid,'Name':'Rendered fixture','StartUrl':url,'FileInclude':'*.zip','Depth':1,'MaxPages':10,'Links':[]}]}))
env=dict(os.environ,UDM_INSTANCE_TAG=tag)
app=subprocess.Popen([str(release/'UDM.exe'),'--background','--data-dir',str(state),'--instance-tag',tag],creationflags=subprocess.CREATE_NO_WINDOW)
def send(message):
 raw=json.dumps(message).encode();r=subprocess.run([str(release/'Udm.NativeHost.exe')],input=struct.pack('<I',len(raw))+raw,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env,creationflags=subprocess.CREATE_NO_WINDOW,timeout=20);assert len(r.stdout)>=4,r.stderr;length=struct.unpack('<I',r.stdout[:4])[0];return json.loads(r.stdout[4:4+length])
def check(value,name):
 assert value,name;checks.append({'name':name,'passed':True});print('PASS',name,flush=True)
try:
 for _ in range(40):
  answer=send({'action':'diagnostics'})
  if answer.get('ok'):break
  time.sleep(.1)
 (root/'last-reply.json').write_text(json.dumps(answer));check(pathlib.Path(answer['dataDirectory']).resolve()==state.resolve(),'Native host targets only the isolated project catalog')
 check('grabber-rendered-storage' in send({'action':'hello'})['capabilities'],'Native host advertises rendered storage capability')
 pending=send({'action':'grabber-rendered-begin','projectId':pid});check(pending.get('ok'),'Native messaging creates a durable project request');ticket=pending['request'];status={'action':'grabber-rendered-status','projectId':pid,'ticket':ticket['ticket']};check(send(status)['status']=='pending','Pending receipt is queryable')
 snapshot={'version':1,'url':url,'baseUrl':url,'title':'Rendered','documentId':'fixture-document','tabId':1,'frameId':0,'visited':2,'truncated':False,'references':[{'url':'https://example.test/runtime.zip','kind':'link','description':'Runtime'}]}
 reply=dict(ticket,action='grabber-rendered-complete',snapshot=snapshot);check(send(reply).get('ok'),'Native messaging saves the protected rendered snapshot');check(send(status)['status']=='saved','Receipt confirms a completed request without resubmission');check(not send(reply).get('ok'),'Repeated completion cannot overwrite the saved snapshot')
 stored=json.loads((state/'state.json').read_text());check('runtime.zip' not in json.dumps(stored['RenderedGrabber']),'Snapshot reference remains encrypted in persistent state')
 newer=send({'action':'grabber-rendered-begin','projectId':pid})['request'];check(send({'action':'grabber-rendered-cancel','projectId':pid}).get('ok'),'Native messaging cancels the pending request');check(send({'action':'grabber-rendered-status','projectId':pid,'ticket':newer['ticket']})['status']=='unavailable','Canceled request is no longer pending')
finally:
 app.terminate();app.wait(timeout=10);(root/'results.json').write_text(json.dumps({'checks':checks},indent=2))
