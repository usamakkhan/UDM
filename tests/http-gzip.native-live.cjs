'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),zlib=require('node:zlib'),crypto=require('node:crypto'),assert=require('node:assert/strict'),{spawn}=require('node:child_process');
const root=path.resolve(process.argv[2]),exe=process.env.UDM_GZIP_TEST_EXE;
const coding=process.env.UDM_GZIP_ENCODING||'gzip';assert(['gzip','x-gzip','X-GZip'].includes(coding),'Supported gzip fixture coding');
assert(exe,'Set UDM_GZIP_TEST_EXE to the freshly built production-core component test');assert(!fs.existsSync(root),'Use a fresh result directory');
const payload=Buffer.alloc(512*1024);for(let at=0;at<payload.length;at++)payload[at]=(at*37+(at/19|0))%256;
const hash=bytes=>crypto.createHash('sha256').update(bytes).digest('hex'),gzip=zlib.gzipSync(payload),stored=zlib.gzipSync(payload,{level:0}),requests=[];
const bodies={known:gzip,unknown:gzip,concatenated:Buffer.concat([zlib.gzipSync(payload.subarray(0,12345)),zlib.gzipSync(payload.subarray(12345))]),empty:zlib.gzipSync(Buffer.alloc(0)),crc:Buffer.from(gzip),truncated:gzip.subarray(0,-1),trailing:Buffer.concat([gzip,Buffer.from([255])]),unsupported:gzip,prefetch:gzip,relocated:gzip,deferred:gzip,paused:stored};bodies.crc[bodies.crc.length-8]^=1;
let server,child;
(async()=>{
 fs.mkdirSync(root,{recursive:true});
 server=http.createServer((req,res)=>{
  const route=req.url.slice(1),body=bodies[route];if(!body){res.writeHead(404);res.end();return;}
  let start=0,end=body.length-1;const range=req.headers.range?.match(/^bytes=(\d+)-(\d*)$/);
  if(range){start=Number(range[1]);if(range[2])end=Number(range[2]);}
  assert(start>=0&&end>=start&&end<body.length);
  requests.push({route,range:req.headers.range||'',ifRange:req.headers['if-range']||'',start,end});
  const headers={'Content-Type':'application/octet-stream','Content-Encoding':route==='unsupported'?'br':range&&process.env.UDM_GZIP_RESUME_CANONICAL==='1'?'gzip':coding,'ETag':'"gzip-fixture-v1"','Accept-Ranges':'bytes'};
  if(route==='known'||route==='paused')headers['Repr-Digest']='sha-256=:'+crypto.createHash('sha256').update(body).digest('base64')+':';
  if(route!=='unknown')headers['Content-Length']=end-start+1;
  if(range)headers['Content-Range']=`bytes ${start}-${end}/${body.length}`;
  res.writeHead(range?206:200,headers);
  if(route==='paused'){
   let at=start;const timer=setInterval(()=>{if(at>end){clearInterval(timer);res.end();return;}const next=Math.min(at+16384,end+1);res.write(body.subarray(at,next));at=next;},15);res.on('close',()=>clearInterval(timer));
  }else res.end(body.subarray(start,end+1));
 });
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
 child=spawn(exe,[path.join(root,'native'),'http://127.0.0.1:'+server.address().port,hash(payload),...(process.env.UDM_GZIP_DEST_ROOT?[path.resolve(process.env.UDM_GZIP_DEST_ROOT)]:[])],{windowsHide:true,stdio:['ignore','pipe','pipe']});let output='';for(const stream of [child.stdout,child.stderr])stream.on('data',b=>{output+=b;process.stdout.write(b);});
 const timeout=setTimeout(()=>child.kill(),150000);
 const code=await new Promise((resolve,reject)=>{child.once('error',reject);child.once('exit',resolve);});clearTimeout(timeout);fs.writeFileSync(path.join(root,'native.log'),output);assert.equal(code,0,'Native gzip integration checks');
 for(const route of ['known','unknown','concatenated','empty','prefetch','relocated','deferred'])assert.equal(requests.filter(r=>r.route===route).length,1,route+' must use one response');
 const resumed=requests.filter(r=>r.route==='paused');assert(resumed.some(r=>r.start>0&&r.ifRange==='"gzip-fixture-v1"'),'Pause resumes encoded offsets with the strong validator');
 console.log('PASS one-response reuse and validated encoded-offset resume');
})().catch(error=>{process.exitCode=1;console.error(error.stack);}).finally(async()=>{
 if(child&&child.exitCode===null)child.kill();if(server){server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
 fs.writeFileSync(path.join(root,'requests.json'),JSON.stringify({payloadBytes:payload.length,payloadSha256:hash(payload),gzipBytes:gzip.length,requests},null,2));
});
