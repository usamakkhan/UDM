'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),crypto=require('node:crypto'),assert=require('node:assert/strict'),{spawn}=require('node:child_process');
const root=path.resolve(process.argv[2]),exe=process.env.UDM_POST_IDENTITY_TEST_EXE;assert(exe,'Set UDM_POST_IDENTITY_TEST_EXE');assert(!fs.existsSync(root),'Use fresh results');
const payload=Buffer.from('UDM owned complete identity form response.\r\n'.repeat(2000)),pdf=Buffer.from('%PDF-1.7\n'+ 'owned file content\n'.repeat(20)+'%%EOF\n'),prefix=Buffer.from('<html><body>Your download will start shortly.</body></html>\n');
const body=Buffer.from([102,111,114,109,0,255,43,38,61,13,10]),hash=b=>crypto.createHash('sha256').update(b).digest('hex'),requests=[];let server,child;
(async()=>{
 fs.mkdirSync(root,{recursive:true});server=http.createServer((req,res)=>{const chunks=[];req.on('data',b=>chunks.push(b));req.on('end',()=>{
  const route=req.url.slice(1),input=Buffer.concat(chunks);requests.push({route,method:req.method,bytes:input.length,bodySha256:hash(input),range:req.headers.range||''});
  if(route==='redirect'&&req.method==='POST'){res.writeHead(303,{'Location':'/redirected-get','Content-Length':0});res.end();return;}
  const output=route==='empty'?Buffer.alloc(0):route==='prefix'?Buffer.concat([prefix,pdf]):payload;
  const headers={'Content-Type':'application/octet-stream'};if(route!=='unknown')headers['Content-Length']=output.length;res.writeHead(200,headers);res.end(output);
 });});
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));child=spawn(exe,[path.join(root,'native'),'http://127.0.0.1:'+server.address().port,hash(payload),hash(pdf)],{windowsHide:true,stdio:['ignore','pipe','pipe']});let output='';for(const s of [child.stdout,child.stderr])s.on('data',b=>{output+=b;process.stdout.write(b);});
 const timer=setTimeout(()=>child.kill(),180000);const code=await new Promise((resolve,reject)=>{child.once('error',reject);child.once('exit',resolve);});clearTimeout(timer);fs.writeFileSync(path.join(root,'native.log'),output);assert.equal(code,0,'Plain form response qualification');
 assert.equal(requests.filter(r=>r.route==='fresh').length,0);
 for(const route of ['success','info','unknown','redirect','redirected-get','empty','later','collision','journal','changed','prefix','reopened'])assert.equal(requests.filter(r=>r.route===route).length,1,route+' must not spend another response');
 assert.equal(requests.filter(r=>r.route==='damaged').length,2,'Separate explicit restart is the only resubmission');
 for(const r of requests){assert.equal(r.range,'');assert.equal(r.method,r.route==='redirected-get'?'GET':'POST');assert.equal(r.bodySha256,hash(r.method==='GET'?Buffer.alloc(0):body));}
 console.log('PASS exact request bodies, one-use plain response recovery and explicit-only resubmission');
})().catch(error=>{process.exitCode=1;console.error(error.stack);}).finally(async()=>{
 if(child&&child.exitCode===null)child.kill();if(server){server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
 fs.writeFileSync(path.join(root,'requests.json'),JSON.stringify({payloadBytes:payload.length,payloadSha256:hash(payload),pdfBytes:pdf.length,pdfSha256:hash(pdf),prefixBytes:prefix.length,requestBodyBytes:body.length,requestBodySha256:hash(body),requests},null,2));
});
