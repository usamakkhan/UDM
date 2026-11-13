'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),zlib=require('node:zlib'),crypto=require('node:crypto'),assert=require('node:assert/strict'),{spawn}=require('node:child_process');
const root=path.resolve(process.argv[2]);if(fs.existsSync(root))throw Error('Use a fresh result directory');
const udm=process.env.UDM_APP_EXE,idm=process.env.IDM_APP_EXE;
if(!udm||!idm)throw Error('Set explicit UDM_APP_EXE and IDM_APP_EXE');
const coding=process.env.UDM_HTTP_PROBE_ENCODING||'gzip';assert(['gzip','x-gzip'].includes(coding),'Supported paired fixture coding');
const payload=Buffer.from('UDM owned HTTP content-encoding fixture.\r\n'.repeat(1600)),compressed=zlib.gzipSync(payload),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const requests=[],cases=[];let server,desktop;
const pause=ms=>new Promise(resolve=>setTimeout(resolve,ms));
(async()=>{
 fs.mkdirSync(root,{recursive:true});const data=path.join(root,'state'),files=path.join(root,'files');fs.mkdirSync(data);fs.mkdirSync(files);
 fs.writeFileSync(path.join(data,'state.json'),JSON.stringify({Schema:1,Settings:{DownloadFolder:files,CategoryFolders:false,Connections:1,Parallel:1,Retries:0,ProxyMode:'Connect directly',SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false,ClipboardMonitor:false,CloseToTray:false,StopOnWebPage:false,CorrectFileExtensions:false},Downloads:[],Queues:[]}));
 server=http.createServer((req,res)=>{const gzip=req.url.includes('gzip'),body=gzip?compressed:payload;requests.push({path:req.url,method:req.method,range:req.headers.range||'',acceptEncoding:req.headers['accept-encoding']||'',gzip});res.writeHead(200,{'Content-Type':'application/octet-stream','Content-Disposition':'attachment; filename="probe.udmcodec"','Content-Length':body.length,...(gzip?{'Content-Encoding':coding}:{})});res.end(req.method==='HEAD'?undefined:body);});
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));const origin='http://127.0.0.1:'+server.address().port;
 const tag='encoding'+Date.now();
 for(const client of ['udm','idm'])for(const encoding of ['identity',coding]){
  const name=client+'-'+encoding+'.udmcodec',url=origin+'/'+client+'/'+encoding,target=path.join(files,name);
  const args=client==='udm'?['--instance-tag',tag,'--data-dir',data,'/d',url,'/p',files,'/f',name,'/n']:['/d',url,'/p',files,'/f',name,'/n'];
  const process=spawn(client==='udm'?udm:idm,args,{windowsHide:true,stdio:'ignore'});if(client==='udm'&&!desktop)desktop=process;
  const deadline=Date.now()+20000;let job,actual;
  while(Date.now()<deadline){
   if(client==='udm'){try{job=JSON.parse(fs.readFileSync(path.join(data,'state.json'))).Downloads.find(job=>job.Url===url);}catch{}if(job?.Status==='Failed')break;}
   try{const bytes=fs.readFileSync(target);if(bytes.length===payload.length||bytes.length===compressed.length){actual=bytes;if(client==='idm'||job?.Status==='Complete')break;}}catch{}
   await pause(100);
  }
  const result={client,encoding,state:job?.Status||(actual?'file-observed':'timeout'),error:job?.Error||'',bytes:actual?.length??null,sha256:actual&&hash(actual),matchesDecoded:!!actual&&hash(actual)===hash(payload),matchesWire:!!actual&&hash(actual)===hash(encoding===coding?compressed:payload),requests:requests.filter(request=>request.path==='/'+client+'/'+encoding)};
  cases.push(result);console.log(JSON.stringify(result));
 }
})().catch(error=>{cases.push({error:error.stack});process.exitCode=1;}).finally(async()=>{
 if(desktop&&desktop.exitCode===null)desktop.kill();if(server){server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
 fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({scope:'Small synthetic loopback CLI comparison. UDM uses a private catalog. IDM receives only named fixture downloads; no /q, queue command, process termination or settings modification.',payloadBytes:payload.length,payloadSha256:hash(payload),compressedBytes:compressed.length,cases},null,2));
});
