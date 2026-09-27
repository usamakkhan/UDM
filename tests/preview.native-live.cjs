'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),https=require('node:https'),crypto=require('node:crypto'),assert=require('node:assert/strict'),{spawn}=require('node:child_process');
const root=path.resolve(process.argv[2]),exe=process.env.UDM_TEST_EXE||path.resolve(__dirname,'../release-native/Udm.NativeTests.exe');fs.mkdirSync(root,{recursive:true});const results=[],requests=[],sockets=new Set();let base,other;
const md5=s=>crypto.createHash('md5').update(s).digest('hex');
const serve=(req,res)=>{
 const name=new URL(req.url,'http://fixture').pathname;requests.push({name,method:req.method,range:req.headers.range||'',auth:!!req.headers.authorization,cookie:!!req.headers.cookie,origin:!!req.headers.origin,referer:!!req.headers.referer});
 const end=(status,headers={})=>{res.writeHead(status,{'Connection':'close',...headers});res.end();};
 if(name==='/stall')return;
 if(name==='/redirect')return end(302,{Location:base+'/head'});
 if(name==='/cross-login')return end(302,{Location:other+'/need-login'});
 if(name==='/bearer')return end(401,{'WWW-Authenticate':'Bearer realm="preview-fixture"','Content-Length':'0'});
 if(name==='/cross')return end(302,{Location:other+'/head'});
 if(name==='/ftp-redirect')return end(302,{Location:'ftp://127.0.0.1:9/file'});
 if(name==='/downgrade')return end(302,{Location:base+'/head'});
 if(name==='/loop')return end(302,{Location:base+'/loop'});
 if(name==='/missing')return end(404,{'Content-Length':'0'});
 if(name==='/need-login')return end(401,{'WWW-Authenticate':'Basic realm="preview-fixture"','Content-Length':'0'});
 if(name==='/basic'&&req.headers.authorization!=='Basic '+Buffer.from('u:p:2').toString('base64'))return end(401,{'WWW-Authenticate':'Basic realm="preview-fixture"','Content-Length':'0'});
 if(name==='/digest'){
  const fields={};for(const m of (req.headers.authorization||'').matchAll(/([\w-]+)=(?:"([^"]*)"|([^, ]+))/g))fields[m[1]]=m[2]??m[3];
  const expected=md5(md5('u:preview-fixture:p:2')+':preview-nonce:'+fields.nc+':'+fields.cnonce+':auth:'+md5(req.method+':'+name));
  if(fields.response!==expected||fields.username!=='u'||fields.uri!==name)return end(401,{'WWW-Authenticate':'Digest realm="preview-fixture", nonce="preview-nonce", algorithm=MD5, qop="auth"','Content-Length':'0'});
 }
 if(name==='/compressed')return end(200,{'Content-Encoding':'gzip','Content-Length':'912','Content-Type':'application/zip'});
 if(name==='/bad-mime')return end(200,{'Content-Length':'123','Content-Type':'not-a-mime<stuff>'});
 if(name==='/huge')return end(200,{'Content-Length':'6543210123','Content-Type':'application/octet-stream'});
 if(name==='/overflow')return end(200,{'Content-Length':'9223372036854775808','Content-Type':'application/octet-stream'});
 if(name==='/head-partial')return end(206,{'Content-Length':'1','Content-Range':'bytes 0-0/999'});
 const fallback=['/head-405','/head-501','/head-403','/empty-range','/bad-range','/bad-range-length','/ignored-range','/unknown','/no-head-size'];
 if(req.method==='HEAD'&&fallback.includes(name)){
  if(name==='/unknown'||name==='/no-head-size')return end(200,{'Content-Type':'application/zip','Transfer-Encoding':'chunked'});
  return end(name==='/head-403'?403:name==='/head-501'?501:405,{'Content-Length':'0'});
 }
 if(req.method==='GET'){
  if(name==='/empty-range')return end(416,{'Content-Range':'bytes */0','Content-Length':'0'});
  if(name==='/unknown'){res.writeHead(200,{'Content-Type':'application/octet-stream','Transfer-Encoding':'chunked','Connection':'close'});return res.end('unknown length content');}
  if(name==='/bad-range')return end(206,{'Content-Length':'1','Content-Range':'bytes 2-2/4097'});
  if(name==='/bad-range-length')return end(206,{'Content-Length':'7','Content-Range':'bytes 0-0/4097'});
  if(name==='/ignored-range'){res.writeHead(200,{'Content-Type':'application/zip','Content-Length':'4097'});res.flushHeaders();return;}
  res.writeHead(206,{'Content-Type':'application/zip','Content-Length':'1','Content-Range':'bytes 0-0/4097'});return res.end(Buffer.from([1]));
 }
 end(200,{'Content-Type':'Application/ZIP; charset=utf-8','Content-Length':'4097'});
};
const server=http.createServer(serve),second=http.createServer(serve),certDir='D:/UDM/benchmarks/stress-2026-09-20/tls-fixture';
const tls=https.createServer({key:fs.readFileSync(path.join(certDir,'key.pem')),cert:fs.readFileSync(path.join(certDir,'cert.pem'))},serve);tls.on('tlsClientError',()=>{});
for(const s of [server,second,tls])s.on('connection',socket=>{sockets.add(socket);socket.on('error',()=>{});socket.on('close',()=>sockets.delete(socket));});
async function execute(name,url,spec={}){
 const folder=path.join(root,name);fs.mkdirSync(folder,{recursive:true});const input=path.join(folder,'input.json');fs.writeFileSync(input,JSON.stringify({url,preview:true,...spec}));
 return new Promise((resolve,reject)=>{const child=spawn(exe,['--feature-spec',input],{windowsHide:true});let out='',err='';child.stdout.on('data',b=>out+=b);child.stderr.on('data',b=>err+=b);const timer=setTimeout(()=>{child.kill();reject(Error('Preview test timed out'));},20000);child.on('error',e=>{clearTimeout(timer);reject(e);});child.on('close',code=>{clearTimeout(timer);fs.writeFileSync(path.join(folder,'stdout.log'),out);fs.writeFileSync(path.join(folder,'stderr.log'),err);try{assert.equal(code,0,out||err);const r=JSON.parse(fs.readFileSync(path.join(folder,'result.json'),'utf8').replace(/^\uFEFF/,''));assert.ok(r.jobUnchanged&&!r.fileExists&&!r.partsExist,JSON.stringify(r));resolve(r);}catch(e){reject(e);}});});
}
async function check(name,route,spec,verify){const start=requests.length;try{const r=await execute(name,route.startsWith('https:')?route:base+route,spec);verify(r,requests.slice(start));results.push({name,passed:true,result:r,requests:requests.slice(start)});console.log('PASS '+name);}catch(e){results.push({name,passed:false,error:e.stack,requests:requests.slice(start)});console.error('FAIL '+name+': '+e.message);}}
const ready=(r,size=4097,type='application/zip')=>{assert.equal(r.preview.Status,'Ready',JSON.stringify(r));assert.equal(r.preview.Size,size);assert.equal(r.preview.ContentType,type);};
(async()=>{
 await Promise.all([server,second,tls].map(s=>new Promise(r=>s.listen(0,'127.0.0.1',r))));base='http://127.0.0.1:'+server.address().port;other='http://127.0.0.1:'+second.address().port;
 await check('head-only','/head',{},(r,req)=>{ready(r);assert.deepEqual(req.map(r=>r.method),['HEAD']);assert.equal(req[0].range,'');});
 await check('same-origin-redirect','/redirect',{headers:{Authorization:'Bearer fixture',Cookie:'fixture=1',Origin:base,Referer:base+'/page'}},(r,req)=>{ready(r);assert.deepEqual(req.map(r=>r.method),['HEAD','HEAD']);assert.ok(req[1].auth&&req[1].cookie&&req[1].origin&&req[1].referer);});
 await check('cross-origin-redirect','/cross',{headers:{Authorization:'Bearer fixture',Cookie:'fixture=1',Origin:base,Referer:base+'/page'}},(r,req)=>{ready(r);assert.deepEqual(req.map(r=>r.method),['HEAD','HEAD']);assert.ok(!req[1].auth&&!req[1].cookie&&!req[1].origin&&!req[1].referer);});
 for(const route of ['/head-405','/head-501','/head-403','/no-head-size'])await check(route.slice(1),route,{},(r,req)=>{ready(r);assert.deepEqual(req.map(r=>r.method),['HEAD','GET']);assert.equal(req[1].range,'bytes=0-0');});
 await check('range-ignored','/ignored-range',{},(r,req)=>{ready(r);assert.ok(r.elapsedMs<1500);assert.deepEqual(req.map(r=>r.method),['HEAD','GET']);});
 await check('empty-file','/empty-range',{},r=>ready(r,0,''));
 await check('unknown-size','/unknown',{},r=>ready(r,-1,'application/octet-stream'));
 await check('large-file','/huge',{},r=>ready(r,6543210123,'application/octet-stream'));
 await check('invalid-mime','/bad-mime',{},r=>ready(r,123,''));
 for(const route of ['/bad-range','/bad-range-length','/head-partial','/overflow','/compressed','/missing','/ftp-redirect','/loop'])await check(route.slice(1),route,{},r=>assert.equal(r.preview.Status,'Error',JSON.stringify(r)));
 await check('basic-login','/basic',{headers:{Authorization:'Basic '+Buffer.from('u:p:2').toString('base64')}},(r,req)=>{ready(r);assert.equal(req.length,1);});
 await check('digest-login','/digest',{headers:{Authorization:'Basic '+Buffer.from('u:p:2').toString('base64')}},(r,req)=>{ready(r);assert.equal(req.length,2);assert.ok(req.every(r=>r.method==='HEAD'));});
 await check('login-required','/need-login',{},(r,req)=>{assert.equal(r.preview.Status,'Error');assert.equal(r.preview.HttpStatus,401);assert.match(r.preview.Message,/Login is required/);assert.equal(req.length,1);});
 await check('cross-origin-login-hint','/cross-login',{headers:{Authorization:'Basic '+Buffer.from('u:p:2').toString('base64')}},(r,req)=>{assert.equal(r.preview.Status,'Error');assert.match(r.preview.Message,/browser authorization/);assert.equal(req.length,2);assert.ok(!req[1].auth);});
 await check('browser-token-login-hint','/bearer',{},r=>{assert.equal(r.preview.Status,'Error');assert.match(r.preview.Message,/browser authorization/);});
 await check('cancel-stalled-headers','/stall',{cancelMs:350},r=>{assert.equal(r.preview.Status,'Cancelled');assert.ok(r.elapsedMs<1200);});
 await check('deadline-stalled-headers','/stall',{previewTimeoutMs:400},r=>{assert.equal(r.preview.Status,'Error');assert.match(r.preview.Message,/timed out/);assert.ok(r.elapsedMs<1300);});
 await check('tls-certificate-validation','https://127.0.0.1:'+tls.address().port+'/head',{},(r,req)=>{assert.equal(r.preview.Status,'Error');assert.equal(req.length,0);});
 for(const [name,extra] of [['post',{post:'fixture=form'}],['recapture',{previewFields:{RequiresRequestCapture:true}}],['media',{previewFields:{SourceUrl:base+'/page'}}],['partial',{previewFields:{Received:12}}],['segments',{previewFields:{Segments:[{Index:0,Start:0,End:12,Done:0}]}}],['offline',{previewFields:{OfflineProject:{}}}]])await check('skip-'+name,'/head',extra,(r,req)=>{assert.equal(r.preview.Status,'Unavailable');assert.equal(req.length,0);});
})().catch(e=>{console.error(e);process.exitCode=1;}).finally(()=>{for(const socket of sockets)socket.destroy();for(const s of [server,second,tls])s.close();const report={passed:results.filter(r=>r.passed).length,failed:results.filter(r=>!r.passed).length,results};fs.writeFileSync(path.join(root,'results.json'),JSON.stringify(report,null,2));console.log(`${report.passed} passed, ${report.failed} failed`);if(report.failed)process.exitCode=1;});
