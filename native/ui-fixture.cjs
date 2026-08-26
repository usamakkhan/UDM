'use strict';
// Local-only, bounded fixture for manually exercising the native MFC dialogs.
const http=require('node:http'), fs=require('node:fs'), path=require('node:path'), crypto=require('node:crypto');
const {spawn}=require('node:child_process');
const data=Buffer.alloc(32*1024*1024+731);
for(let i=0;i<data.length;i++)data[i]=(i*31+7)%251;
const evidence={size:data.length,sha256:crypto.createHash('sha256').update(data).digest('hex'),requests:0};
const file=path.join(__dirname,'ui-fixture.json');
const server=http.createServer((req,res)=>{
 evidence.requests++;
 if(req.url==='/') {res.writeHead(200,{'Content-Type':'text/html'});return res.end('<a href="/udm-native-ui-check.bin">UDM fixture</a>');}
 if(req.url!=='/udm-native-ui-check.bin'){res.writeHead(404);return res.end();}
 const range=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||'');
 let from=range?Number(range[1]):0, end=range&&range[2]?Number(range[2]):data.length-1;
 if(from>end||end>=data.length){res.writeHead(416,{'Content-Range':`bytes */${data.length}`});return res.end();}
 res.writeHead(range?206:200,{'Content-Length':end-from+1,'Content-Type':'application/octet-stream','ETag':'"native-ui-v1"',...(range?{'Content-Range':`bytes ${from}-${end}/${data.length}`}:{})});
 if(from===0&&end===0)return res.end(data.subarray(0,1));
 const timer=setInterval(()=>{const next=Math.min(end+1,from+16384);res.write(data.subarray(from,next));from=next;if(from>end){clearInterval(timer);res.end();}},80);
 res.on('close',()=>clearInterval(timer));
});
server.listen(0,'127.0.0.1',()=>{
 evidence.url=`http://127.0.0.1:${server.address().port}/udm-native-ui-check.bin`;
 fs.writeFileSync(file,JSON.stringify(evidence,null,2));
 console.log(JSON.stringify(evidence));
 if(process.argv.includes('--handoff')){
  const host=spawn(path.join(__dirname,'../release-native/Udm.NativeHost.exe'),[],{windowsHide:true,env:{...process.env,UDM_INSTANCE_TAG:'native-validation'}});
  const message=Buffer.from(JSON.stringify({action:'add',url:evidence.url,filename:'udm-native-ui-check.bin'}));
  const prefix=Buffer.alloc(4);prefix.writeUInt32LE(message.length);const chunks=[];
  host.stdout.on('data',c=>chunks.push(c));host.on('exit',code=>{const result=Buffer.concat(chunks);evidence.handoff=JSON.parse(result.subarray(4));fs.writeFileSync(file,JSON.stringify(evidence,null,2));console.log('Handoff',code,JSON.stringify(evidence.handoff));});
  host.stdin.end(Buffer.concat([prefix,message]));
 }
});
setTimeout(()=>{fs.writeFileSync(file,JSON.stringify(evidence,null,2));server.closeAllConnections();server.close();},300000).unref();
