'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),https=require('node:https'),net=require('node:net'),crypto=require('node:crypto'),assert=require('node:assert/strict'),{spawn}=require('node:child_process');
const {socksFixture}=require('./support/socks-fixture.cjs');
const root=path.resolve(process.argv[2]),exe=process.env.UDM_TEST_EXE;
assert(exe);assert(!fs.existsSync(root),'Use a fresh output directory');fs.mkdirSync(root,{recursive:true});
const payload=Buffer.alloc(3*1024*1024+717);for(let i=0;i<payload.length;i++)payload[i]=(i*37+11)%251;
const hash=b=>crypto.createHash('sha256').update(b).digest('hex'),results=[],traffic=[],sockets=new Set(),servers=[];
let origin,proxy,base,proxyAddress;const socks=[];const proxyChallenges=[];
function track(s){sockets.add(s);s.on('error',()=>{});s.on('close',()=>sockets.delete(s));}
async function listen(s){servers.push(s);s.on('connection',track);await new Promise(r=>s.listen(0,'127.0.0.1',r));return s.address().port;}
function serve(route,req,res){
 const u=new URL(req.url,'http://'+req.headers.host);traffic.push({route,url:u.href,path:u.pathname,method:req.method,range:req.headers.range||'',authorization:req.headers.authorization||'',proxyAuthorization:req.headers['proxy-authorization']||'',cookie:req.headers.cookie||''});
 if(u.pathname==='/stall-headers')return;
 if(u.pathname==='/stall-body'){res.writeHead(200,{'Content-Length':payload.length});res.flushHeaders();return;}
 if(u.pathname==='/oversize-headers'){res.writeHead(200,{'Content-Length':0,'X-Large':'x'.repeat(220000)});return res.end();}
 if(u.pathname==='/same'||u.pathname==='/cross'){res.writeHead(302,{Location:(u.pathname==='/cross'?base.replace('127.0.0.1','localhost'):base)+'/sensitive','Content-Length':0});return res.end();}
 if(u.pathname==='/basic'&&req.headers.authorization!=='Basic '+Buffer.from('u:p:2').toString('base64')){res.writeHead(401,{'WWW-Authenticate':'Basic realm="fixture"','Content-Length':0});return res.end();}
 if(u.pathname==='/digest'||u.pathname==='/bad-digest'){
  const a=req.headers.authorization||'',fields=Object.fromEntries([...a.matchAll(/([a-zA-Z0-9_-]+)=(?:"([^"]*)"|([^, ]+))/g)].map(m=>[m[1],m[2]??m[3]]));
  const md5=s=>crypto.createHash('md5').update(s).digest('hex');
  const valid=a.startsWith('Digest ')&&fields.username==='u'&&fields.realm==='fixture'&&fields.nonce==='nonce-fixture'&&fields.qop==='auth'&&[req.url,u.pathname+u.search].includes(fields.uri)&&fields.response===md5(md5('u:fixture:p:2')+':nonce-fixture:'+fields.nc+':'+fields.cnonce+':auth:'+md5(req.method+':'+fields.uri));
  if(!valid||u.pathname==='/bad-digest'){res.writeHead(401,{'WWW-Authenticate':'Digest realm="fixture", nonce="nonce-fixture", algorithm=MD5, qop="auth"','Content-Length':0});return res.end();}
 }
 if(req.method==='POST'){const chunks=[];req.on('data',b=>chunks.push(b));req.on('end',()=>{const b=Buffer.concat(chunks);res.writeHead(200,{'Content-Length':b.length});res.end(b);});return;}
 let first=0,last=payload.length-1;const range=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||'');if(range){first=+range[1];last=range[2]?+range[2]:last;}
 const header={'Content-Length':last-first+1,ETag:'"proxy-transport-v1"','Content-Type':'application/octet-stream','Set-Cookie':['one=first; Path=/','two=second; Path=/'],...(range?{'Content-Range':`bytes ${u.pathname==='/bad-range'&&last>0?first+1:first}-${last}/${payload.length}`}:{})};
 res.writeHead(range?206:200,header);if(req.method==='HEAD')return res.end();
 if(u.pathname==='/truncate'&&last>0)return res.end(payload.subarray(first,Math.max(first+1,Math.floor((first+last)/2))));
 if(u.pathname!=='/slow')return res.end(payload.subarray(first,last+1));
 let at=first;const timer=setInterval(()=>{const end=Math.min(at+16384,last+1);res.write(payload.subarray(at,end));at=end;if(at>last){clearInterval(timer);res.end();}},20);res.on('close',()=>clearInterval(timer));
}
async function run(name,spec){
 const dir=path.join(root,name);fs.mkdirSync(dir,{recursive:true});const input=path.join(dir,'input.json');fs.writeFileSync(input,JSON.stringify(spec));
 const began=performance.now();const output=await new Promise((resolve,reject)=>{const p=spawn(exe,['--feature-spec',input],{windowsHide:true});let out='';p.stdout.on('data',b=>out+=b);p.stderr.on('data',b=>out+=b);const timer=setTimeout(()=>{p.kill();reject(Error('Native fixture timeout'));},45000);p.on('error',e=>{clearTimeout(timer);reject(e);});p.on('close',code=>{clearTimeout(timer);fs.writeFileSync(path.join(dir,'native.log'),out);if(code)reject(Error(out));else resolve(out);});});
 const result=JSON.parse(fs.readFileSync(path.join(dir,'result.json'),'utf8').replace(/^\uFEFF/,''));return {...result,wallMs:performance.now()-began};
}
function exact(r,expected=payload){assert.equal(r.status,'Complete',r.error);assert.equal(r.sha256,hash(expected));assert.deepEqual(fs.readFileSync(r.path),expected);}
async function test(name,fn){try{const detail=await fn();results.push({name,passed:true,detail});console.log('PASS '+name);}catch(e){results.push({name,passed:false,error:e.stack});console.error('FAIL '+name+': '+e.message);}fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({passed:results.filter(x=>x.passed).length,failed:results.filter(x=>!x.passed).length,results,traffic,proxyChallenges},null,2));}
(async()=>{
 origin=http.createServer((q,r)=>serve('direct',q,r));base='http://127.0.0.1:'+await listen(origin);
 proxy=http.createServer((q,r)=>{
 const routePath=new URL(q.url).pathname;
 if(routePath.startsWith('/proxy-digest')){
  const a=q.headers['proxy-authorization']||'',f=Object.fromEntries([...a.matchAll(/([a-zA-Z0-9_-]+)=(?:"([^"]*)"|([^, ]+))/g)].map(m=>[m[1],m[2]??m[3]]));
  const md5=s=>crypto.createHash('md5').update(s).digest('hex');
  const valid=a.startsWith('Digest ')&&f.username==='proxy-user'&&f.realm==='proxy-fixture'&&f.nonce==='proxy-nonce'&&f.qop==='auth'&&[q.url,new URL(q.url).pathname+new URL(q.url).search].includes(f.uri)&&f.response===md5(md5('proxy-user:proxy-fixture:proxy-secret')+':proxy-nonce:'+f.nc+':'+f.cnonce+':auth:'+md5(q.method+':'+f.uri));
  proxyChallenges.push({path:routePath,method:q.method,scheme:a.split(' ')[0],uri:f.uri,request:q.url,valid});
  if(!valid){const body=Buffer.alloc(40000,120);r.writeHead(407,{'Proxy-Authenticate':'Digest realm="proxy-fixture", nonce="proxy-nonce", algorithm=MD5, qop="auth"','Set-Cookie':'proxy-poison=1','Content-Length':body.length});return r.end(body);}
 }
if(new URL(q.url).pathname==='/proxy-login'&&q.headers['proxy-authorization']!=='Basic '+Buffer.from('proxy-user:proxy-secret').toString('base64')){r.writeHead(407,{'Proxy-Authenticate':'Basic realm="fixture"','Content-Length':0});return r.end();}serve('proxy',q,r);});proxyAddress='127.0.0.1:'+await listen(proxy);
 const route={mode:'Use a proxy server',address:proxyAddress};
 await test('HTTPS CONNECT negotiates proxy Digest without exposing origin secrets',async()=>{
  const seen=[],tunnel=http.createServer();
  tunnel.on('connect',(q,socket)=>{
   const a=q.headers['proxy-authorization']||'',f=Object.fromEntries([...a.matchAll(/([a-zA-Z0-9_-]+)=(?:"([^"]*)"|([^, ]+))/g)].map(m=>[m[1],m[2]??m[3]])),md5=s=>crypto.createHash('md5').update(s).digest('hex');
   const valid=a.startsWith('Digest ')&&f.username==='proxy-user'&&f.realm==='tunnel'&&f.nonce==='tunnel-nonce'&&f.qop==='auth'&&f.uri===q.url&&f.response===md5(md5('proxy-user:tunnel:proxy-secret')+':tunnel-nonce:'+f.nc+':'+f.cnonce+':auth:'+md5('CONNECT:'+q.url));
   seen.push({valid,cookie:q.headers.cookie,authorization:q.headers.authorization,target:q.url});
   socket.end(valid?'HTTP/1.1 502 Fixture stops after authentication\r\nContent-Length: 0\r\nConnection: close\r\n\r\n':'HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Digest realm="tunnel", nonce="tunnel-nonce", algorithm=MD5, qop="auth"\r\nContent-Length: 0\r\nConnection: close\r\n\r\n');
  });
  const port=await listen(tunnel),r=await run('proxy-digest-connect',{url:'https://files.example.test/file',proxy:{mode:'Use a proxy server',address:'127.0.0.1:'+port,user:'proxy-user',password:'proxy-secret'},headers:{Cookie:'origin=private',Authorization:'Basic '+Buffer.from('origin:secret').toString('base64')}});
  assert.equal(r.status,'Failed');assert(!fs.existsSync(r.path));assert(seen.some(x=>x.valid),'CONNECT Digest response must validate');assert(seen.length<=3);assert(seen.every(x=>!x.cookie&&!x.authorization&&x.target==='files.example.test:443'));return {result:r,connects:seen};
 });
 for(const host of ['127.0.0.1','localhost','localhost.','127.1'])await test('Explicit HTTP proxy keeps '+host+' on the selected route',async()=>{const start=traffic.length,r=await run('loop-'+host.replaceAll('.','_'),{url:base.replace('127.0.0.1',host)+'/file',proxy:route});exact(r);const seen=traffic.slice(start);assert(seen.length>1&&seen.every(x=>x.route==='proxy'));return r;});
 await test('Metadata HEAD uses the explicit proxy',async()=>{const start=traffic.length,r=await run('head',{url:base+'/file',proxy:route,preview:true});assert.equal(r.preview.Status,'Ready');assert.equal(r.preview.Size,payload.length);assert(!r.fileExists&&!r.partsExist);assert(traffic.slice(start).some(x=>x.method==='HEAD'));assert(traffic.slice(start).every(x=>x.route==='proxy'));return r;});
 await test('Explicit bypass downloads directly',async()=>{const start=traffic.length,r=await run('bypass',{url:base+'/file',proxy:{...route,bypass:'127.0.0.1'}});exact(r);assert(traffic.slice(start).every(x=>x.route==='direct'));return r;});
 for(const where of ['stall-headers','stall-body','slow'])await test('Pause promptly cancels '+where+' through the proxy',async()=>{const start=traffic.length,r=await run(where,{url:base+'/'+where,proxy:route,cancelMs:400,connections:1});assert.equal(r.status,'Paused',r.error);assert(r.wallMs<2500,JSON.stringify(r));assert(traffic.slice(start).some(x=>x.route==='proxy'));return r;});
 await test('Resume reuses verified proxy-downloaded bytes',async()=>{const first=await run('resume',{url:base+'/slow',proxy:route,cancelMs:700,connections:1});assert.equal(first.status,'Paused');assert(first.bytes>0);const start=traffic.length,r=await run('resume',{resume:true,proxy:route});exact(r);assert(traffic.slice(start).some(x=>Number(/^bytes=(\d+)/.exec(x.range)?.[1])>=first.bytes));return {first,result:r};});
 for(const auth of ['basic','digest'])await test(auth+' authentication works on proxied loopback downloads',async()=>{const r=await run(auth,{url:base+'/'+auth,proxy:route,headers:{Authorization:'Basic '+Buffer.from('u:p:2').toString('base64')}});exact(r);return r;});
 await test('Rejected Digest credentials stop without a retry loop',async()=>{const start=traffic.length,r=await run('bad-digest',{url:base+'/bad-digest',proxy:route,headers:{Authorization:'Basic '+Buffer.from('u:p:2').toString('base64')}});assert.equal(r.status,'Failed');assert(traffic.length-start<=3);return r;});
 await test('Proxy login is independently applied',async()=>{const start=traffic.length,r=await run('proxy-login',{url:base+'/proxy-login',proxy:{...route,user:'proxy-user',password:'proxy-secret'}});exact(r);assert(traffic.slice(start).every(x=>x.proxyAuthorization==='Basic '+Buffer.from('proxy-user:proxy-secret').toString('base64')&&!x.authorization));return r;});
 for(const host of ['127.0.0.1','files.example.test'])await test('Proxy Digest authenticates '+host,async()=>{const start=traffic.length,r=await run('proxy-digest-'+host,{url:base.replace('127.0.0.1',host)+'/proxy-digest',proxy:{...route,user:'proxy-user',password:'proxy-secret'}});exact(r);const seen=traffic.slice(start);assert(seen.length&&seen.every(x=>x.route==='proxy'&&x.proxyAuthorization.startsWith('Digest ')&&!x.authorization&&!x.cookie.includes('proxy-poison')));return r;});
 await test('Wrong proxy Digest credentials stop without publishing',async()=>{const start=proxyChallenges.length,r=await run('proxy-digest-wrong',{url:base+'/proxy-digest-wrong',proxy:{...route,user:'proxy-user',password:'wrong'}});assert.equal(r.status,'Failed');assert(!fs.existsSync(r.path));assert(proxyChallenges.length-start<=3);return r;});
 await test('Proxy Digest preserves a POST body after challenge',async()=>{const post='digest-body'.repeat(10000),r=await run('proxy-digest-post',{url:base+'/proxy-digest-post',post,proxy:{...route,user:'proxy-user',password:'proxy-secret'}});exact(r,Buffer.from(post));return r;});
 for(const kind of ['same','cross'])await test(kind+' origin redirect preserves credential scope',async()=>{const start=traffic.length,r=await run(kind,{url:base+'/'+kind,proxy:route,headers:{Authorization:'Basic '+Buffer.from('u:p:2').toString('base64'),Cookie:'scope=fixture'}});exact(r);const seen=traffic.slice(start).filter(x=>x.path==='/sensitive');assert(seen.length);assert(seen.every(x=>x.route==='proxy'&&!!x.authorization===(kind==='same')&&!!x.cookie===(kind==='same')));return r;});
 await test('POST body survives the proxy without GET replay',async()=>{const post='synthetic\u0000body\r\n'+('payload'.repeat(10000)),start=traffic.length,r=await run('post',{url:base+'/post',proxy:route,post});exact(r,Buffer.from(post));assert(traffic.slice(start).every(x=>x.method==='POST'&&x.route==='proxy'));return r;});
 for(const bad of ['bad-range','truncate','oversize-headers'])await test(bad+' cannot publish a completed file',async()=>{const r=await run(bad,{url:base+'/'+bad,proxy:route});assert.equal(r.status,'Failed');assert(!fs.existsSync(r.path));return r;});
 for(const version of [4,5])await test('SOCKS'+version+' explicitly proxies loopback downloads',async()=>{const p=await socksFixture({url:base,ports:new Set()},{version}).start();socks.push(p);const start=p.metrics.accepted,r=await run('socks'+version,{url:base+'/file',proxy:p.settings});exact(r);assert(p.metrics.accepted>start+1);return r;});
 await test('Unreachable explicit proxy never falls back to direct',async()=>{const reserve=net.createServer(),port=await listen(reserve);await new Promise(r=>reserve.close(r));const start=traffic.length,r=await run('unreachable',{url:base+'/file',proxy:{mode:'Use a proxy server',address:'127.0.0.1:'+port}});assert.equal(r.status,'Failed');assert.equal(traffic.length,start);return r;});
 if(process.env.UDM_TEST_TLS_DIR){
  const tls=https.createServer({key:fs.readFileSync(path.join(process.env.UDM_TEST_TLS_DIR,'key.pem')),cert:fs.readFileSync(path.join(process.env.UDM_TEST_TLS_DIR,'cert.pem'))},(q,r)=>serve('tls',q,r)),port=await listen(tls),connects=[];
  proxy.on('connect',(req,socket,head)=>{connects.push({url:req.url,headers:req.headers});assert.equal(req.url,'127.0.0.1:'+port);const peer=net.connect(port,'127.0.0.1',()=>{socket.write('HTTP/1.1 200 Connection Established\r\n\r\n');if(head.length)peer.write(head);socket.pipe(peer);peer.pipe(socket);});track(peer);track(socket);peer.on('error',()=>socket.destroy());socket.on('close',()=>peer.destroy());});
  await test('HTTPS CONNECT checks the certificate and keeps origin secrets inside TLS',async()=>{const r=await run('tls-invalid',{url:'https://127.0.0.1:'+port+'/file',proxy:route,headers:{Cookie:'private-cookie=fixture',Authorization:'Basic Zm9vOmJhcg=='}});assert.equal(r.status,'Failed');assert(!fs.existsSync(r.path));assert(connects.length);assert(connects.every(x=>!x.headers.cookie&&!x.headers.authorization));assert(!traffic.some(x=>x.route==='tls'));return r;});
 }
})().catch(e=>{console.error(e);process.exitCode=1;}).finally(()=>{for(const p of socks)p.close();for(const s of sockets)s.destroy();for(const s of servers)s.close();if(results.some(x=>!x.passed))process.exitCode=1;console.log(JSON.stringify({passed:results.filter(x=>x.passed).length,failed:results.filter(x=>!x.passed).length}));});
