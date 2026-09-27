'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict'),{spawn}=require('node:child_process');
const root=path.resolve(process.argv[2]),exe=process.env.UDM_APP_EXE||path.resolve(__dirname,'../release-native/UDM.exe'),host=path.join(path.dirname(exe),'Udm.NativeHost.exe'),results=[],children=[];
const wait=ms=>new Promise(r=>setTimeout(r,ms));
async function until(fn,ms=15000){const end=Date.now()+ms;while(Date.now()<end){const result=await fn();if(result)return result;await wait(100);}throw Error('Scheduler fixture timed out');}
function start(dir,tag){const child=spawn(exe,['--background','--data-dir',dir,'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});children.push(child);return child;}
async function stop(child){if(child.exitCode===null){child.kill();await until(()=>child.exitCode!==null||child.signalCode!==null,5000);}}
function saved(file){try{return JSON.parse(fs.readFileSync(file));}catch{return null;}}
async function diagnostics(tag){return new Promise((resolve,reject)=>{const child=spawn(host,[],{windowsHide:true,env:{...process.env,UDM_INSTANCE_TAG:tag},stdio:['pipe','pipe','pipe']});children.push(child);const raw=Buffer.from(JSON.stringify({action:'diagnostics',requestId:1})),header=Buffer.alloc(4);header.writeUInt32LE(raw.length);const chunks=[];child.stdout.on('data',b=>chunks.push(b));child.on('error',reject);child.stdin.on('error',()=>{});const timer=setTimeout(()=>{child.kill();reject(Error('Diagnostics timed out'));},12000);child.on('close',code=>{clearTimeout(timer);try{assert.equal(code,0);const b=Buffer.concat(chunks);const value=JSON.parse(b.subarray(4,4+b.readUInt32LE()));assert.equal(value.ok,true);resolve(value);}catch(e){reject(e);}});child.stdin.end(Buffer.concat([header,raw]));});}
function pass(name,detail={}){results.push({name,passed:true,...detail});console.log('PASS '+name);}
let server;const requests=[],payload=Buffer.alloc(500123,43);
async function scenario(name,{wake=true,disabled=false,restart=false}={}){
 const dir=path.join(root,name),data=path.join(dir,'state'),file=path.join(data,'state.json'),tag='wake'+Date.now();fs.mkdirSync(data,{recursive:true});
 const deadline=Date.now()+7000,output=path.join(dir,'downloads');fs.mkdirSync(output,{recursive:true});
 const state={Schema:1,Settings:{DownloadFolder:output,CategoryFolders:false,ProxyMode:'Connect directly',Connections:2,Parallel:1,Retries:0,PrefetchFileInfo:false,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false},Projects:[],Queues:[{Name:'Wake fixture',Enabled:!disabled,Parallel:1,RunOnce:true,OnceStarted:false,StartOnceUtc:new Date(deadline).toISOString(),StopOnceUtc:null,WakeComputer:wake,FinishAction:'None'}],Downloads:[{Id:require('node:crypto').randomBytes(16).toString('hex'),Url:`http://127.0.0.1:${server.address().port}/${name}`,FileName:name+'.bin',Folder:output,Queue:'Wake fixture',QueueMember:true,Status:'Paused',Size:-1,Received:0,Connections:2,Segments:[]}]};
 fs.writeFileSync(file,JSON.stringify(state));let app=start(data,tag);
 // A new checkpoint proves the launched process opened this particular history
 // before contacting its tagged native host, which must never use normal history.
 await until(()=>{const s=saved(file);return s?.Downloads[0]?.ConfirmationPending===false;});
 const before=await diagnostics(tag);assert.equal(path.resolve(before.dataDirectory).toLowerCase(),data.toLowerCase());assert.equal(before.downloads,1);assert.equal(before.version,'0.33.0');
 const expected=wake&&!disabled;
 if(expected){assert.ok(['Armed','Unsupported'].includes(before.wakeTimer.Status),JSON.stringify(before.wakeTimer));assert.equal(Date.parse(before.wakeTimer.NextWakeUtc),deadline);assert.equal(before.wakeTimer.Queue,'Wake fixture');}
 else assert.equal(before.wakeTimer.Status,'Off');
 assert.equal(requests.filter(r=>r.name===name).length,0,'No network request before the schedule');
 pass(name+' exposes the correct applied timer before any download',{wakeStatus:before.wakeTimer.Status});
 if(restart){await stop(app);app=start(data,tag);await wait(1000);const reloaded=await diagnostics(tag);assert.equal(Date.parse(reloaded.wakeTimer.NextWakeUtc),deadline);assert.equal(saved(file).Downloads[0].Status,'Paused');pass('Restart rearms the saved future timer without starting the file early');}
 if(disabled){await wait(Math.max(0,deadline-Date.now()+500));assert.equal(requests.filter(r=>r.name===name).length,0);assert.equal(saved(file).Downloads[0].Status,'Paused');pass('Disabled opted-in queue makes no request at its old scheduled time');}
 else {const complete=await until(()=>{const s=saved(file),j=s?.Downloads[0];if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;});assert.deepEqual(fs.readFileSync(path.join(complete.Folder,complete.FileName)),payload);assert.ok(requests.filter(r=>r.name===name).every(r=>r.time>=deadline));const after=await until(async()=>{const d=await diagnostics(tag);return d.wakeTimer.Status==='Off'&&d;});assert.ok(!after.wakeTimer.NextWakeUtc);pass(name+' starts at the deadline, verifies exact bytes and clears its timer',{firstRequestDelayMs:requests.find(r=>r.name===name).time-deadline});}
 await stop(app);
}
(async()=>{
 fs.mkdirSync(root,{recursive:true});
 server=http.createServer((req,res)=>{requests.push({name:req.url.slice(1),time:Date.now()});const r=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||''),a=r?+r[1]:0,b=r&&r[2]?+r[2]:payload.length-1;res.writeHead(r?206:200,{'Content-Length':b-a+1,ETag:'"wake-fixture"',...(r?{'Content-Range':`bytes ${a}-${b}/${payload.length}`}:{})});res.end(payload.subarray(a,b+1));});await new Promise(r=>server.listen(0,'127.0.0.1',r));
 await scenario('wake-on');await scenario('wake-off',{wake:false});await scenario('wake-restart',{restart:true});await scenario('disabled',{disabled:true});
})().catch(e=>{results.push({passed:false,error:e.stack});console.error(e);process.exitCode=1;}).finally(async()=>{for(const child of children)if(child.exitCode===null)child.kill();if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({scope:'Isolated native app, real Windows timer registration, no sleep or system power changes',passed:results.filter(r=>r.passed).length,failed:results.filter(r=>!r.passed).length,results},null,2));});
