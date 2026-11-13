'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),zlib=require('node:zlib'),crypto=require('node:crypto'),assert=require('node:assert/strict'),{spawn}=require('node:child_process');
const root=path.resolve(process.argv[2]),exe=process.env.UDM_POST_GZIP_TEST_EXE;assert(exe,'Set UDM_POST_GZIP_TEST_EXE');assert(!fs.existsSync(root),'Use a fresh result directory');
const coding=process.env.UDM_GZIP_ENCODING||'gzip';assert(['gzip','x-gzip','X-GZip'].includes(coding),'Supported gzip fixture coding');
const payload=Buffer.from('UDM owned complete form response.\r\n'.repeat(4000)),compressed=zlib.gzipSync(payload),body=Buffer.from([102,111,114,109,0,255,43,38,61,13,10]);
const hash=b=>crypto.createHash('sha256').update(b).digest('hex'),requests=[];let server,child;
(async()=>{
 fs.mkdirSync(root,{recursive:true});server=http.createServer((req,res)=>{
  const chunks=[];req.on('data',b=>chunks.push(b));req.on('end',()=>{
   const route=req.url.slice(1),input=Buffer.concat(chunks);requests.push({route,method:req.method,bytes:input.length,bodySha256:hash(input),range:req.headers.range||''});
   if(route==='post-redirect'&&req.method==='POST'){res.writeHead(303,{'Location':'/redirected-get','Content-Length':0});res.end();return;}
   const headers={'Content-Type':'application/octet-stream','Content-Encoding':coding,'Repr-Digest':'sha-256=:'+crypto.createHash('sha256').update(compressed).digest('base64')+':'};
   if(route!=='post-unknown')headers['Content-Length']=compressed.length;
   res.writeHead(200,headers);
   if(route==='post-loss'){res.write(compressed.subarray(0,20));setTimeout(()=>res.destroy(),50);return;}
   res.end(compressed);
  });
 });
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));child=spawn(exe,[path.join(root,'native'),'http://127.0.0.1:'+server.address().port,hash(payload)],{windowsHide:true,stdio:['ignore','pipe','pipe']});let output='';for(const s of [child.stdout,child.stderr])s.on('data',b=>{output+=b;process.stdout.write(b);});
 const timer=setTimeout(()=>child.kill(),180000);const code=await new Promise((resolve,reject)=>{child.once('error',reject);child.once('exit',resolve);});clearTimeout(timer);fs.writeFileSync(path.join(root,'native.log'),output);assert.equal(code,0,'Native form decoding integration');
 assert.equal(requests.filter(r=>r.route==='fresh').length,0,'Fresh forms must not prefetch POST');
 for(const route of ['post-known','post-unknown','post-redirect','post-info','post-confirmed-hash','post-later','post-changed','post-reopened','post-loss'])assert.equal(requests.filter(r=>r.route===route).length,1,route+' must not resubmit');
 assert.equal(requests.filter(r=>r.route==='post-damaged').length,2,'Only the explicit second Restart may resubmit damaged saved data');
 assert.equal(requests.filter(r=>r.route==='redirected-get').length,1,'Redirected response is fetched once');
 for(const request of requests){assert.equal(request.range,'','POST responses must not use ranges');assert.equal(request.method,request.route==='redirected-get'?'GET':'POST');assert.equal(request.bodySha256,hash(request.method==='GET'?Buffer.alloc(0):body),'Exact original binary form body');}
 console.log('PASS exact original POST bodies, one-use completed response reuse and explicit-only restart');
})().catch(error=>{process.exitCode=1;console.error(error.stack);}).finally(async()=>{
 if(child&&child.exitCode===null)child.kill();if(server){server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
 fs.writeFileSync(path.join(root,'requests.json'),JSON.stringify({payloadBytes:payload.length,payloadSha256:hash(payload),wireBytes:compressed.length,requestBodyBytes:body.length,requestBodySha256:hash(body),requests},null,2));
});
