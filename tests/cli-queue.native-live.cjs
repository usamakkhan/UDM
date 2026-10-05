'use strict';
const fs=require('fs'),path=require('path'),http=require('http'),{spawn}=require('child_process');
const root=path.resolve(process.argv[2]),exe=process.env.UDM_APP_EXE;
if(!exe||fs.existsSync(root))throw Error('Set UDM_APP_EXE and provide a fresh output directory');
fs.mkdirSync(root,{recursive:true});
const delay=ms=>new Promise(r=>setTimeout(r,ms));
const checks=[],requests=[];let app,server,error;
const bytes=Buffer.alloc(65536);for(let i=0;i<bytes.length;i++)bytes[i]=(i*31)&255;
function check(ok,name){checks.push({name,passed:!!ok});if(!ok)throw Error(name);}
async function stop(){if(app&&app.exitCode===null){const closed=new Promise(r=>app.once('exit',r));app.kill();await closed;}app=null;}
(async()=>{
 server=http.createServer((req,res)=>{requests.push({method:req.method,url:req.url,range:req.headers.range||''});const range=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||'');const start=range?Number(range[1]):0,end=range&&range[2]?Math.min(Number(range[2]),bytes.length-1):bytes.length-1;if(start>end||start>=bytes.length){res.writeHead(416,{'Content-Range':'bytes */'+bytes.length});res.end();return;}const headers={'Content-Length':end-start+1,'Content-Type':'application/octet-stream','Accept-Ranges':'bytes','ETag':'"queue-fixture"'};if(range)headers['Content-Range']='bytes '+start+'-'+end+'/'+bytes.length;res.writeHead(range?206:200,headers);res.end(req.method==='HEAD'?undefined:bytes.subarray(start,end+1));});
 await new Promise(r=>server.listen(0,'127.0.0.1',r));
 const data=path.join(root,'data'),folder=path.join(root,'files'),tag='cliqueue'+Date.now(),url='http://127.0.0.1:'+server.address().port;
 fs.mkdirSync(data);fs.mkdirSync(folder);
 fs.writeFileSync(path.join(data,'state.json'),JSON.stringify({Schema:1,Settings:{DownloadFolder:folder,CategoryFolders:false,ClipboardMonitor:false,ShowTipsOnStartup:false,Sound:false,SuppressProgressDialog:true,SuppressCompletionDialog:true,PrefetchFileInfo:false},Downloads:[],Queues:[],Projects:[]}));
 const base=['--background','--data-dir',data,'--instance-tag',tag];
 function launch(args){const child=spawn(exe,args,{windowsHide:true,stdio:'ignore'});child.on('error',e=>error=e.message);return child;}
 function records(){try{return JSON.parse(fs.readFileSync(path.join(data,'state.json'))).Downloads;}catch{return [];}}
 async function waitCount(n){const start=performance.now();while(performance.now()-start<20000){if(error)throw Error(error);if(records().length===n)return;await delay(100);}throw Error('Catalog did not reach '+n+' downloads');}
 app=launch([...base,'/a','/d',url+'/first.bin','/p',folder,'/f','first.bin']);await waitCount(1);
 const forwarded=launch([...base,'/d',url+'/second.bin','/p',folder,'/f','second.bin','/n','/a']);
 await new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('Forwarder timed out')),20000);forwarded.once('exit',code=>{clearTimeout(timer);code===0?resolve():reject(Error('Forwarder exit '+code));});});
 await waitCount(2);await delay(3000);
 check(records().every(j=>j.Status==='Paused'&&j.QueueMember&&j.Queue==='Main queue'),'Both first-launch and forwarded additions are paused queue members');
 check(records().every(j=>path.resolve(j.Folder)===folder)&&records().map(j=>j.FileName).sort().join(',')==='first.bin,second.bin','Both CLI destinations and filenames are preserved');
 check(requests.length===0&&fs.readdirSync(folder).length===0,'No HTTP request or output file before explicit start');
 await stop();app=launch(base);await delay(3000);
 check(app.exitCode===null&&records().length===2&&records().every(j=>j.Status==='Paused'&&j.QueueMember),'Restart retains both paused queue members');
 check(requests.length===0,'Restart does not initiate either transfer');
 if(process.env.UDM_TEST_QUEUE_START==='1'){
  if(!process.env.UDM_HOST_EXE)throw Error('Set UDM_HOST_EXE for queue-start native-host boundary tests');
  const bridge=await require('./isolated-native-bridge.cjs')(process.env.UDM_HOST_EXE,tag);
  try{for(const action of ['cli-add','cli-start-queue']){const response=await fetch(bridge.url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({action,url:url+'/unauthorized.bin'})});const reply=await response.json();check(reply.ok===false&&/local UDM executable/.test(reply.error),'Browser host rejects '+action+' without queue side effects');}}finally{await bridge.close();}
  check(requests.length===0&&records().length===2&&records().every(j=>j.Status==='Paused'),'Rejected browser commands leave downloads paused and unchanged');
  async function forward(args){const child=launch([...base,...args]);await new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('Queue command timed out')),20000);child.once('exit',code=>{clearTimeout(timer);code===0?resolve():reject(Error('Queue command exit '+code));});});}
  async function waitComplete(n){const start=performance.now();while(performance.now()-start<30000){if(error)throw Error(error);const jobs=records();if(jobs.length===n&&jobs.every(j=>j.Status==='Complete'))return;await delay(100);}throw Error('Queue did not finish '+n+' files');}
  await forward(['/s']);await waitComplete(2);
  check(['first.bin','second.bin'].every(name=>fs.readFileSync(path.join(folder,name)).equals(bytes)),'Forwarded scheduler start downloads both queued files with exact bytes');
  check(requests.length>0,'Explicit queue-start begins network transfers');
  await stop();const before=requests.length;
  app=launch([...base,'/a','/d',url+'/third.bin','/p',folder,'/f','third.bin']);await waitCount(3);await delay(1500);
  check(records().find(j=>j.FileName==='third.bin').Status==='Paused'&&requests.length===before,'Adding after a finished queue still waits for explicit start');
  await stop();app=launch([...base,'/s']);await waitComplete(3);
  check(fs.readFileSync(path.join(folder,'third.bin')).equals(bytes),'First-launch scheduler start resumes the saved queue and writes exact bytes');
 }
})().catch(e=>{error=e.stack;process.exitCode=1;}).finally(async()=>{
 await stop();if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}
 fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({passed:!error,error,checks,requests,app:exe},null,2));
 console.log(JSON.stringify({passed:!error,checks:checks.length,error}));
});
