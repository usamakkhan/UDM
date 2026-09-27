'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict'),crypto=require('node:crypto'),{spawn}=require('node:child_process');
const project=path.resolve(__dirname,'..'),root=path.resolve(process.argv[2]||path.join(project,'benchmarks','launch-live-'+Date.now())),exe=process.env.UDM_APP_EXE||path.join(project,'release/UDM.exe'),tag='cli'+Date.now(),state=path.join(root,'state/state.json'),results=[],children=[];
const wait=ms=>new Promise(r=>setTimeout(r,ms));let desktop,server;
async function until(fn,ms=15000){const end=Date.now()+ms;while(Date.now()<end){const value=await fn();if(value)return value;await wait(100);}throw Error('Timed out waiting for launch test condition');}
function jobs(){try{return JSON.parse(fs.readFileSync(state)).Downloads;}catch{return [];}}
function run(args){const child=spawn(exe,['--background','--data-dir',path.dirname(state),'--instance-tag',tag,...args],{windowsHide:true,stdio:'ignore'});children.push(child);return child;}
async function forwarded(args){const child=run(args);await until(()=>child.exitCode!==null);assert.equal(child.exitCode,0);}
function pass(name){results.push({name,passed:true});console.log('PASS '+name);}
(async()=>{
 fs.mkdirSync(root,{recursive:true});if(fs.existsSync(state))throw Error('Use a new test output directory.');fs.mkdirSync(path.dirname(state),{recursive:true});
 fs.writeFileSync(state,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(root,'default'),CategoryFolders:false,Connections:2,PrefetchFileInfo:false,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false},Queues:[{Name:'Main queue',Enabled:true,Parallel:1}],Downloads:[],Projects:[]}));
 const payload=Buffer.alloc(300731);for(let i=0;i<payload.length;i++)payload[i]=(i*31+7)%251;
 server=http.createServer((req,res)=>{let begin=0,end=payload.length-1;if(req.headers.range){const r=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range);if(!r){res.writeHead(416);return res.end();}begin=+r[1];if(r[2])end=Math.min(end,+r[2]);res.statusCode=206;res.setHeader('Content-Range','bytes '+begin+'-'+end+'/'+payload.length);}res.setHeader('ETag','"launch-fixture-v1"');res.setHeader('Content-Length',end-begin+1);res.end(payload.subarray(begin,end+1));});
 await new Promise(r=>server.listen(0,'127.0.0.1',r));const base='http://127.0.0.1:'+server.address().port;
 const coldFolder=path.join(root,'cold folder'),warmFolder=path.join(root,'warm folder');
 desktop=run(['--paused','--add',base+'/cold','--folder',coldFolder,'--name','cold.bin']);
 const cold=await until(()=>jobs().find(j=>j.FileName==='cold.bin'));assert.equal(cold.Status,'Paused');assert.equal(cold.Folder,coldFolder);pass('Cold launch retains the explicit save folder, filename and paused state');
 await forwarded(['--paused','/d',base+'/warm','/p',warmFolder,'/f','warm.bin']);const warm=jobs().find(j=>j.FileName==='warm.bin');assert.equal(warm.Status,'Paused');assert.equal(warm.Folder,warmFolder);pass('Second process forwards the save folder and paused state to the running app');
 await forwarded(['/n','/d',base+'/silent','/p',warmFolder,'/f','silent.bin']);
 const complete=await until(()=>{const job=jobs().find(j=>j.FileName==='silent.bin');if(job?.Status==='Failed')throw Error(job.Error);return job?.Status==='Complete'&&job;},30000);
 assert.deepEqual(fs.readFileSync(path.join(complete.Folder,complete.FileName)),payload);pass('IDM-style /n queues immediately and downloads byte-identical content');
 const request=Buffer.from(JSON.stringify({action:'cli-add',url:base+'/blocked',folder:warmFolder,filename:'blocked.bin',silent:true})),length=Buffer.alloc(4);length.writeUInt32LE(request.length);
 const host=spawn(path.join(path.dirname(exe),'Udm.NativeHost.exe'),[],{windowsHide:true,env:{...process.env,UDM_INSTANCE_TAG:tag},stdio:['pipe','pipe','pipe']});children.push(host);const chunks=[];host.stdout.on('data',c=>chunks.push(c));host.stdin.end(Buffer.concat([length,request]));await until(()=>host.exitCode!==null);const output=Buffer.concat(chunks),reply=JSON.parse(output.subarray(4,4+output.readUInt32LE(0)));assert.equal(reply.ok,false);assert(!jobs().some(j=>j.FileName==='blocked.bin'));pass('Browser native messaging cannot use the local-only destination/launch command');
 await forwarded(['--add',base+'/confirm','--folder',coldFolder,'--name','confirm.bin']);assert.equal(jobs().find(j=>j.FileName==='confirm.bin').Status,'Awaiting confirmation');pass('Ordinary launch still asks for File Info confirmation');
 // Allow the main window's timer to enter the modal File Info loop before forwarding another job.
 await wait(1800);await forwarded(['/n','/d',base+'/while-dialog-open','/p',warmFolder,'/f','while-dialog-open.bin']);
 const started=performance.now();const modalComplete=await until(()=>{const job=jobs().find(j=>j.FileName==='while-dialog-open.bin');if(job?.Status==='Failed')throw Error(job.Error);return job?.Status==='Complete'&&job;},15000);
 assert.equal(jobs().find(j=>j.FileName==='confirm.bin').Status,'Awaiting confirmation');assert.deepEqual(fs.readFileSync(path.join(modalComplete.Folder,modalComplete.FileName)),payload);pass('Queued download completes byte-for-byte while File Info remains awaiting confirmation ('+((performance.now()-started)/1000).toFixed(2)+' s)');
})().catch(error=>{results.push({passed:false,error:error.stack});console.error(error);process.exitCode=1;}).finally(async()=>{
 for(const child of children)if(child.exitCode===null)child.kill();if(server)await new Promise(r=>{server.close(r);server.closeAllConnections();});
 fs.writeFileSync(path.join(root,'launch-results.json'),JSON.stringify({time:new Date().toISOString(),exe,scope:'Isolated native desktop launches and loopback transfer',results},null,2));
});
