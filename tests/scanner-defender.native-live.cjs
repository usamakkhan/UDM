'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict'),crypto=require('node:crypto'),{spawn}=require('node:child_process');
const root=path.resolve(process.argv[2]),scanner=path.resolve(process.argv[3]),exe=process.env.UDM_APP_EXE||path.resolve(__dirname,'../release/UDM.exe');
const results=[],payload=Buffer.from('UDM benign download and Microsoft Defender integration acceptance fixture.\r\n'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
let desktop,server;
const wait=ms=>new Promise(r=>setTimeout(r,ms));
async function until(f,ms=60000){const end=Date.now()+ms;while(Date.now()<end){const value=f();if(value)return value;await wait(100);}throw Error('Defender integration fixture timed out');}
(async()=>{
 assert.equal(path.basename(scanner).toLowerCase(),'mpcmdrun.exe');assert(fs.statSync(scanner).isFile());
 const data=path.join(root,'state'),out=path.join(root,'downloads'),file=path.join(data,'state.json');
 assert(!fs.existsSync(file),'Use a fresh fixture directory');fs.mkdirSync(data,{recursive:true});
 server=http.createServer((req,res)=>{res.writeHead(200,{'Content-Length':payload.length,'Content-Type':'text/plain',ETag:'"udm-benign-scanner"'});res.end(payload);});
 await new Promise(r=>server.listen(0,'127.0.0.1',r));
 const tag='defender'+crypto.randomBytes(8).toString('hex');
 fs.writeFileSync(file,JSON.stringify({Schema:1,Settings:{DownloadFolder:out,CategoryFolders:false,ProxyMode:'Connect directly',Connections:1,Parallel:1,Retries:0,PrefetchFileInfo:false,SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false,ScanProgram:scanner,ScanArguments:'-Scan -ScanType 3 -File "{file}"',ScanTimeoutSeconds:300},Projects:[],Queues:[{Name:'Defender fixture',Enabled:true,Parallel:1,FinishAction:'None'}],Downloads:[{Id:crypto.randomBytes(16).toString('hex'),Url:'http://127.0.0.1:'+server.address().port+'/benign.txt',FileName:'UDM benign scan.txt',Folder:out,Queue:'Defender fixture',QueueMember:true,Status:'Queued',Size:-1,Received:0,Connections:1,Segments:[]}]}));
 desktop=spawn(exe,['--background','--data-dir',data,'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
 let spawnError;desktop.on('error',e=>spawnError=e);
 const record=await until(()=>{if(spawnError)throw spawnError;let j;try{j=JSON.parse(fs.readFileSync(file)).Downloads[0];}catch{return null;}if(j.Status==='Failed')throw Error(j.Error);if(j.ScanResult?.Status&& !['Running'].includes(j.ScanResult.Status))return j;});
 assert.equal(record.Status,'Complete');assert.equal(hash(fs.readFileSync(path.join(out,record.FileName))),hash(payload));
 results.push({passed:true,name:'Real UDM publishes the exact harmless downloaded file before scanning'});
 assert.equal(record.ScanResult.Status,'Finished',JSON.stringify(record.ScanResult));assert.equal(record.ScanResult.ExitCode,0);assert.equal(record.ScanResult.Program.toLowerCase(),path.basename(scanner).toLowerCase());assert.equal(record.ScanResult.Scanner,'Microsoft Defender');
 results.push({passed:true,name:'UDM runs installed Microsoft Defender with its preset and persists the interpreted result',status:record.ScanResult.Status,exitCode:record.ScanResult.ExitCode});
 assert.equal(hash(fs.readFileSync(path.join(out,record.FileName))),hash(payload));
 results.push({passed:true,name:'Benign fixture remains byte-identical after the real antivirus process completes'});
})().catch(e=>{results.push({passed:false,error:e.stack});console.error(e);process.exitCode=1;}).finally(async()=>{
 if(desktop&&desktop.exitCode===null&&desktop.signalCode===null){desktop.kill();await until(()=>desktop.exitCode!==null||desktop.signalCode!==null,5000);}
 if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}
 fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({scope:'Actual installed Microsoft Defender scanning a harmless loopback download in an isolated UDM catalog; no antivirus settings changed',passed:results.filter(r=>r.passed).length,failed:results.filter(r=>!r.passed).length,results},null,2));for(const r of results)console.log((r.passed?'PASS ':'FAIL ')+(r.name||r.error));
});
