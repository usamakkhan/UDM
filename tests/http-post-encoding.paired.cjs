'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),zlib=require('node:zlib'),crypto=require('node:crypto'),{spawn}=require('node:child_process');
const root=path.resolve(process.argv[2]),release=process.env.UDM_TEST_RELEASE,reference=process.env.IDM_POST_PROBE_EXE;
if(!release||!reference||fs.existsSync(root))throw Error('Set explicit staged executable paths and use a fresh result directory');
const payload=Buffer.from('UDM owned HTTP form encoding comparison.\r\n'.repeat(1600)),gzip=zlib.gzipSync(payload),post='fixture=owned&value=a%2Bb',hash=b=>crypto.createHash('sha256').update(b).digest('hex'),requests=[],cases=[];
let server,desktop,bridge;const pause=ms=>new Promise(resolve=>setTimeout(resolve,ms));
(async()=>{
 fs.mkdirSync(root,{recursive:true});const data=path.join(root,'state'),files=path.join(root,'files');fs.mkdirSync(data);fs.mkdirSync(files);
 fs.writeFileSync(path.join(data,'state.json'),JSON.stringify({Schema:1,Settings:{DownloadFolder:files,CategoryFolders:false,Connections:1,Parallel:1,Retries:0,ProxyMode:'Connect directly',SuppressProgressDialog:true,SuppressCompletionDialog:true,Sound:false,ClipboardMonitor:false,CloseToTray:false,StopOnWebPage:false,CorrectFileExtensions:false},Downloads:[],Queues:[]}));
 server=http.createServer((req,res)=>{const chunks=[];req.on('data',b=>chunks.push(b));req.on('end',()=>{const encoded=req.url.endsWith('/gzip'),body=encoded?gzip:payload,input=Buffer.concat(chunks);requests.push({path:req.url,method:req.method,requestBytes:input.length,bodySha256:hash(input),range:req.headers.range||'',acceptEncoding:req.headers['accept-encoding']||''});res.writeHead(200,{'Content-Type':'application/octet-stream','Content-Length':body.length,...(encoded?{'Content-Encoding':'gzip'}:{})});res.end(body);});});
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));const origin='http://127.0.0.1:'+server.address().port,tag='formprobe'+Date.now();
 desktop=spawn(path.join(release,'UDM.exe'),['--background','--instance-tag',tag,'--data-dir',data],{windowsHide:true,stdio:'ignore'});
 bridge=await require('./isolated-native-bridge.cjs')(path.join(release,'Udm.NativeHost.exe'),tag);
 const request=async message=>(await fetch(bridge.url,{method:'POST',body:JSON.stringify(message)})).json();
 let identity;for(let attempt=0;attempt<100;attempt++){try{identity=await request({action:'diagnostics'});if(identity.ok)break;}catch{}await pause(100);}if(!identity?.ok)throw Error('Private desktop did not answer');
 for(const client of ['udm','idm'])for(const encoding of ['identity','gzip']){
  const name=client+'-'+encoding+'.udmform',url=origin+'/'+client+'/'+encoding,target=path.join(files,name);
  if(client==='udm'){const reply=await request({action:'automation-add',link:{url,data:post,folder:files,filename:name,flags:1}});if(!reply.ok)throw Error(reply.error);}
  else{const api=spawn(reference,[url,post,files,name],{windowsHide:true,stdio:['ignore','pipe','pipe']});let output='';for(const s of [api.stdout,api.stderr])s.on('data',b=>output+=b);const code=await new Promise((resolve,reject)=>{api.once('error',reject);api.once('exit',resolve);});if(code!==0)throw Error('Reference API failed: '+output);}
  const deadline=Date.now()+15000;let job,actual;
  while(Date.now()<deadline){if(client==='udm'){try{job=JSON.parse(fs.readFileSync(path.join(data,'state.json'))).Downloads.find(j=>j.Url===url);}catch{}if(job?.Status==='Failed')break;}
   try{const b=fs.readFileSync(target);if(b.length===payload.length){actual=b;if(client==='idm'||job?.Status==='Complete')break;}}catch{}await pause(100);}
  const result={client,version:client==='udm'?identity.version:'installed IDM via published COM API',encoding,state:job?.Status||(actual?'file-observed':'timeout'),error:job?.Error||'',bytes:actual?.length??null,sha256:actual&&hash(actual),matchesDecoded:!!actual&&hash(actual)===hash(payload),requests:requests.filter(r=>r.path==='/'+client+'/'+encoding)};cases.push(result);console.log(JSON.stringify(result));
 }
})().catch(error=>{process.exitCode=1;cases.push({error:error.stack});console.error(error.stack);}).finally(async()=>{
 if(bridge)await bridge.close();if(desktop&&desktop.exitCode===null)desktop.kill();if(server){server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
 fs.mkdirSync(root,{recursive:true});fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({scope:'Owned loopback POST comparison; private UDM catalog and named IDM fixture downloads only. No settings, queue-start or quit commands sent to IDM.',api:'https://www.internetdownloadmanager.com/support/idm_api.html',requestBodyBytes:Buffer.byteLength(post),requestBodySha256:hash(Buffer.from(post)),payloadBytes:payload.length,payloadSha256:hash(payload),cases},null,2));
});
