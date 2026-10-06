'use strict';
const fs=require('fs'),path=require('path'),http=require('http'),{spawn}=require('child_process');
const exe=process.env.UDM_APP_EXE,root=path.resolve(process.argv[2]);
if(!exe||fs.existsSync(root))throw Error('Set UDM_APP_EXE and use a fresh test directory');
fs.mkdirSync(root,{recursive:true});
const bytes=Buffer.alloc(65536,73),checks=[],children=new Set(),hangupGuards=process.env.UDM_TEST_HANGUP_GUARDS==='1',guardArgs=hangupGuards?['/h']:[];let server,error;
const delay=ms=>new Promise(r=>setTimeout(r,ms));
function check(ok,name){checks.push({name,passed:!!ok});if(!ok)throw Error(name);}
async function until(fn,label,timeout=30000){const start=performance.now();while(performance.now()-start<timeout){if(fn())return;await delay(100);}throw Error('Timed out: '+label);}
function launch(args){const p=spawn(exe,args,{windowsHide:true,stdio:'ignore'});children.add(p);p.on('error',e=>{error=e.stack;});p.on('exit',()=>children.delete(p));return p;}
async function stop(p){if(p.exitCode===null){const done=new Promise(r=>p.once('exit',r));p.kill();await done;}}
function fixture(name){const data=path.join(root,name),folder=path.join(data,'files');fs.mkdirSync(folder,{recursive:true});fs.writeFileSync(path.join(data,'state.json'),JSON.stringify({Schema:1,Settings:{DownloadFolder:folder,CategoryFolders:false,ClipboardMonitor:false,ShowTipsOnStartup:false,Sound:false,SuppressProgressDialog:true,SuppressCompletionDialog:true,PrefetchFileInfo:false},Downloads:[],Queues:[],Projects:[]}));return {folder,args:['--background','--data-dir',data,'--instance-tag','cliquit'+process.pid+name],jobs(){try{return JSON.parse(fs.readFileSync(path.join(data,'state.json'))).Downloads;}catch{return [];}}};}
(async()=>{
 server=http.createServer((req,res)=>{if(req.url==='/missing.bin'){res.writeHead(404);res.end();return;}const m=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||''),start=m?+m[1]:0,end=m&&m[2]?Math.min(+m[2],bytes.length-1):bytes.length-1;const h={'Content-Length':end-start+1,'Content-Type':'application/octet-stream','Accept-Ranges':'bytes','ETag':'"quit-test"'};if(m)h['Content-Range']=`bytes ${start}-${end}/${bytes.length}`;res.writeHead(m?206:200,h);res.end(req.method==='HEAD'?undefined:bytes.subarray(start,end+1));});
 await new Promise(r=>server.listen(0,'127.0.0.1',r));const url='http://127.0.0.1:'+server.address().port;
 const first=fixture('first');let app=launch([...first.args,'/n','/q','/d',url+'/first.bin']);
 await until(()=>app.exitCode!==null,'first instance exits');
 check(app.exitCode===0,'First instance exits successfully after /q download');
 check(first.jobs().length===1&&first.jobs()[0].Status==='Complete','Exit follows persisted completion');
 check(fs.readFileSync(path.join(first.folder,'first.bin')).equals(bytes),'Exit-after-download file matches every source byte');
 const forwarded=fixture('forwarded');app=launch(forwarded.args);await delay(2000);check(app.exitCode===null,'Existing isolated app is running');
 const sender=launch([...forwarded.args,'/n','/q','/d',url+'/forwarded.bin']);await until(()=>sender.exitCode!==null,'forwarder exits');check(sender.exitCode===0,'Forwarded /q command is accepted');
 await until(()=>forwarded.jobs().some(j=>j.Status==='Complete'),'forwarded file completes');await delay(2500);
 check(app.exitCode===null,'Forwarded /q never closes the existing instance');check(fs.readFileSync(path.join(forwarded.folder,'forwarded.bin')).equals(bytes),'Forwarded download has exact source bytes');await stop(app);
 const paused=fixture('paused');app=launch([...paused.args,...guardArgs,'/a','/q','/d',url+'/paused.bin']);await until(()=>paused.jobs().length===1,'paused entry');await delay(2500);check(app.exitCode===null&&paused.jobs()[0].Status==='Paused'&&!fs.existsSync(path.join(paused.folder,'paused.bin')),'Queue-only /q remains paused without exiting or downloading');await stop(app);
 const failed=fixture('failed');app=launch([...failed.args,...guardArgs,'/n','/q','/d',url+'/missing.bin']);await until(()=>failed.jobs().some(j=>j.Status==='Failed'),'failed download',60000);await delay(2500);check(app.exitCode===null,'Failed download does not trigger /q exit');await stop(app);
 const bare=fixture('bare');app=launch([...bare.args,...guardArgs,'/q']);await delay(2500);check(app.exitCode===null&&bare.jobs().length===0,'Bare /q has no exit or download side effect');await stop(app);
})().catch(e=>{error=e.stack;process.exitCode=1;}).finally(async()=>{for(const p of children)await stop(p);if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({passed:!error,error,checks,hangupGuards,app:exe},null,2));console.log(JSON.stringify({passed:!error,checks:checks.length,error}));});