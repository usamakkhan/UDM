'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),net=require('node:net'),https=require('node:https'),crypto=require('node:crypto'),assert=require('node:assert/strict'),{spawn}=require('node:child_process');
const out=path.resolve(process.argv[2]),fixture=path.resolve(process.argv[3]),exe=process.env.UDM_TEST_EXE;fs.mkdirSync(out,{recursive:true});
const small=fs.readFileSync(path.join(fixture,'small.zip')),large=fs.readFileSync(path.join(fixture,'large.zip')),empty=fs.readFileSync(path.join(fixture,'empty.zip')),zip64=JSON.parse(fs.readFileSync(path.join(fixture,'zip64.json'))),zip64tail=fs.readFileSync(path.join(fixture,'zip64-tail.bin'));
const expected=name=>JSON.parse(fs.readFileSync(path.join(fixture,name+'.json'),'utf8'));
const sockets=new Set(),requests=[],results=[];let base,other;
const md5=s=>crypto.createHash('md5').update(s).digest('hex');
function serve(req,res){
 const route=req.url.split('?')[0];const key=new URL(req.url,'http://fixture').searchParams.get('case')||route.slice(1,-4);const prior=requests.filter(r=>r.key===key).length;
 requests.push({key,method:req.method,range:req.headers.range||'',auth:!!req.headers.authorization,cookie:req.headers.cookie||'',condition:req.headers['if-match']||req.headers['if-unmodified-since']||'',host:req.headers.host});
 const end=(code,headers={},body='')=>{res.writeHead(code,{'Connection':'close',...headers});res.end(body);};
 if(key==='stall')return;
 if(key==='redirect')return end(302,{Location:base+'/small.zip'});
 if(key==='cross')return end(302,{Location:other+'/small.zip'});
 if(key==='expired')return end(403,{'Content-Length':0});
 if(key==='basic'&&req.headers.authorization!=='Basic '+Buffer.from('u:p:2').toString('base64'))return end(401,{'WWW-Authenticate':'Basic realm="zip"','Content-Length':0});
 if(key==='digest'){
  const fields={};for(const m of (req.headers.authorization||'').matchAll(/([\w-]+)=(?:"([^"]*)"|([^, ]+))/g))fields[m[1]]=m[2]??m[3];
  if(fields.username!=='u'||fields.response!==md5(md5('u:zip:p:2')+':nonce:'+fields.nc+':'+fields.cnonce+':auth:'+md5(req.method+':'+req.url)))return end(401,{'WWW-Authenticate':'Digest realm="zip", nonce="nonce", algorithm=MD5, qop="auth"','Content-Length':0});
 }
 const body=key==='large'||key==='ignored-large'?large:key==='empty'?empty:small;const virtual=key==='zip64',size=virtual?zip64.size:body.length;
 if(key==='ignored-small'||key==='ignored-large')return end(200,{'Content-Length':size,'Content-Type':'application/zip'},body);
 if(key==='unknown-length')return end(200,{'Transfer-Encoding':'chunked','Content-Type':'application/zip'},body);
 if(key==='compressed')return end(200,{'Content-Encoding':'gzip','Content-Length':size},body);
 if(key==='change-412'&&prior)return end(412,{'Content-Length':0});
 if(key==='range-ignored-later'&&prior)return end(200,{'Content-Length':size},body);
 const m=/^bytes=(\d+)-(\d+)$/.exec(req.headers.range||'');if(!m)return end(400,{'Content-Length':0});let from=Number(m[1]),to=Number(m[2]);
 if(from>to||to>=size)return end(416,{'Content-Range':'bytes */'+size,'Content-Length':0});
 const bytes=virtual?Buffer.alloc(to-from+1):Buffer.from(body.subarray(from,to+1));
 if(virtual){const start=Math.max(from,zip64.start),stop=Math.min(to+1,zip64.size);if(stop>start)zip64tail.copy(bytes,start-from,start-zip64.start,stop-zip64.start);}
 if(key==='bad-archive'&&from===0&&to>0)bytes[0]=0;
 if(key==='changed-footer'&&prior>=2&&to===size-1)bytes[bytes.length-1]^=1;
 const headers={'Content-Type':'application/zip','Content-Range':`bytes ${key==='bad-range'&&prior?from+1:from}-${to}/${key==='changed-size'&&prior?size+1:size}`,'Content-Length':bytes.length};
 if(!['no-validator','changed-footer','date-only'].includes(key))headers.ETag=key==='changed-etag'&&prior?'"v2"':'"zip-v1"';
 if(key==='date-only')headers['Last-Modified']='Wed, 21 Oct 2015 07:28:00 GMT';
 if(key==='cookie'){if(prior&&req.headers.cookie!=='zip=rotated')return end(403,{'Content-Length':0});headers['Set-Cookie']='zip=rotated; Path=/';}
 if(key==='truncate'&&prior){res.writeHead(206,headers);res.write(bytes.subarray(0,Math.max(1,bytes.length-1)));return res.socket.end();}
 if(key==='bad-range-length'&&prior)headers['Content-Length']=bytes.length+1;
 if(key==='stall-body'&&prior){res.writeHead(206,headers);res.flushHeaders();return;}
 end(206,headers,bytes);
}
const server=http.createServer(serve),second=http.createServer(serve);const cert='D:/UDM/benchmarks/stress-2026-09-20/tls-fixture';
const tls=https.createServer({key:fs.readFileSync(path.join(cert,'key.pem')),cert:fs.readFileSync(path.join(cert,'cert.pem'))},serve);tls.on('tlsClientError',()=>{});
for(const s of [server,second,tls])s.on('connection',socket=>{sockets.add(socket);socket.on('error',()=>{});socket.on('close',()=>sockets.delete(socket));});
let mode='none';const destinations=[];const track=socket=>{sockets.add(socket);socket.on('error',()=>{});socket.on('close',()=>sockets.delete(socket));};
function reader(socket){let buffer=Buffer.alloc(0),waiting=null,failure=null;const pump=()=>{if(waiting&&buffer.length>=waiting.n){const w=waiting;waiting=null;const value=buffer.subarray(0,w.n);buffer=buffer.subarray(w.n);w.resolve(value);}else if(waiting&&failure){const w=waiting;waiting=null;w.reject(failure);}};const data=b=>{buffer=Buffer.concat([buffer,b]);pump();},end=()=>{failure=Error('Socket closed');pump();};socket.on('data',data);socket.on('end',end);socket.on('error',end);return {read:n=>new Promise((resolve,reject)=>{waiting={n,resolve,reject};pump();}),release:()=>{socket.off('data',data);socket.off('end',end);socket.off('error',end);socket.pause();if(buffer.length)socket.unshift(buffer);}};}
const socks=net.createServer(async socket=>{track(socket);socket.setTimeout(10000,()=>socket.destroy());const r=reader(socket);try{
 const head=await r.read(2),methods=await r.read(head[1]);assert.equal(head[0],5);if(mode==='stall')return;
 const auth=mode==='auth'||mode==='reject-auth';assert.ok(methods.includes(auth?2:0));socket.write(Buffer.from([5,auth?2:0]));
 if(auth){const a=await r.read(2),user=await r.read(a[1]),n=await r.read(1),secret=await r.read(n[0]);assert.equal(a[0],1);const accepted=mode!=='reject-auth'&&user.toString()==='fixture-user'&&secret.toString()==='fixture-password';socket.write(Buffer.from([1,accepted?0:1]));if(!accepted)return socket.end();}
 const h=await r.read(4);assert.equal(h[0],5);assert.equal(h[1],1);let host,ipv6;if(h[3]===1)host=[...await r.read(4)].join('.');else if(h[3]===4){ipv6=(await r.read(16)).toString('hex');host='::1';}else if(h[3]===3){const n=await r.read(1);host=(await r.read(n[0])).toString();}else throw Error('Bad address type');const port=(await r.read(2)).readUInt16BE();destinations.push({host,port,type:h[3],ipv6});
 if(mode==='deny'){socket.write(Buffer.from([5,2,0,1,0,0,0,0,0,0]));return socket.end();}
 assert.ok(['127.0.0.1','localhost','udm-fixture.invalid','::1'].includes(host),'Fixture proxy only forwards local destinations');assert.ok([server.address().port,second.address().port,tls.address().port].includes(port));
 const target=net.connect(port,'127.0.0.1');track(target);target.once('error',()=>socket.destroy());await new Promise((resolve,reject)=>{target.once('connect',resolve);target.once('error',reject);});socket.write(Buffer.from([5,0,0,1,127,0,0,1,0,0]));r.release();socket.pipe(target);target.pipe(socket);socket.resume();socket.on('close',()=>target.destroy());target.on('close',()=>socket.destroy());
 }catch(e){socket.destroy();}});

async function run(name,key,verify,spec={}){
 const folder=path.join(out,name);fs.mkdirSync(folder,{recursive:true});const url=/^https?:/.test(key)?key:base+'/archive.zip?case='+key;const file=path.join(folder,'input.json');fs.writeFileSync(file,JSON.stringify({url,zipPreview:true,...spec}));const start=requests.length;
 try{
  const code=await new Promise((resolve,reject)=>{const child=spawn(exe,['--feature-spec',file],{windowsHide:true});let log='';child.stdout.on('data',x=>log+=x);child.stderr.on('data',x=>log+=x);const timer=setTimeout(()=>{child.kill();reject(Error('Fixture deadline'));},40000);child.once('error',e=>{clearTimeout(timer);reject(e);});child.once('close',c=>{clearTimeout(timer);fs.writeFileSync(path.join(folder,'native.log'),log);resolve(c);});});assert.equal(code,0);
  const r=JSON.parse(fs.readFileSync(path.join(folder,'result.json'),'utf8').replace(/^\uFEFF/,''));assert(r.jobUnchanged&&!r.fileExists&&!r.partsExist,JSON.stringify(r));verify(r,requests.slice(start));results.push({name,passed:true,result:r,requests:requests.slice(start)});console.log('PASS '+name);
 }catch(e){results.push({name,passed:false,error:e.stack,requests:requests.slice(start)});console.error('FAIL '+name+': '+e.message);}
}
const ready=(r,list=expected('small'))=>{assert.equal(r.preview.Status,'Ready',JSON.stringify(r));assert.deepEqual(r.preview.Entries,list);};
const rejected=r=>{assert.equal(r.preview.Status,'Error',JSON.stringify(r));assert(!r.preview.Entries);};
(async()=>{try{
 await Promise.all([server,second,tls,socks].map(s=>new Promise(resolve=>s.listen(0,'127.0.0.1',resolve))));base='http://127.0.0.1:'+server.address().port;other='http://127.0.0.1:'+second.address().port;
 await run('small','small',(r,req)=>{ready(r);assert(req.every(x=>x.method==='GET'&&x.range));assert(r.preview.Validated);});
 await run('large-comment','large',(r,req)=>{ready(r,expected('large'));assert(r.preview.Received<150000);assert(req.some(x=>!x.range.startsWith('bytes=0-')));});
 await run('zip64-over-4gb','zip64',r=>{ready(r,zip64.entries);assert.equal(r.preview.Size,zip64.size);assert(r.preview.Received<150000);});
 await run('empty','empty',r=>ready(r,[]));
 for(const key of ['ignored-small','unknown-length','no-validator','date-only'])await run(key,key,r=>ready(r));
 for(const key of ['ignored-large','expired','compressed','change-412','range-ignored-later','bad-range','changed-size','changed-etag','truncate','bad-range-length','changed-footer'])await run(key,key,rejected);
 const headers={Authorization:'Basic '+Buffer.from('u:p:2').toString('base64')};
 await run('login-required','basic',rejected);
 await run('basic','basic',r=>ready(r),{headers});
 await run('digest','digest',r=>ready(r),{headers});
 await run('redirect','redirect',r=>ready(r));
 await run('cross-origin','cross',(r,req)=>{ready(r);assert(req.filter(x=>x.host===new URL(other).host).every(x=>!x.auth&&!x.cookie));},{headers:{...headers,Cookie:'private=fixture'}});
 await run('post-rejected','small',rejected,{post:'must-not-send'});
 for(const key of ['stall','stall-body'])await run(key,key,r=>{assert.equal(r.preview.Status,'Cancelled');assert(r.elapsedMs<2000);},{cancelMs:150});
 await run('timeout','stall',r=>{rejected(r);assert(r.elapsedMs<2000);},{previewTimeoutMs:150});
 const cookieSession={Origin:base,UserAgent:'UDM ZIP fixture',LogoutPages:'',Cookies:[{name:'zip',value:'initial',domain:'127.0.0.1',path:'/',secure:false,hostOnly:true}]};
 await run('cookie-rotation','cookie',(r,req)=>{ready(r);assert(r.sessionUpdated);assert.equal(req[0].cookie,'zip=initial');assert(req.slice(1).every(x=>x.cookie==='zip=rotated'));},{browserSession:cookieSession});
 const proxy={address:'127.0.0.1:'+socks.address().port};
 await run('socks5','http://udm-fixture.invalid:'+server.address().port+'/archive.zip?case=socks5',(r,req)=>{ready(r);assert(req.length===destinations.length);assert(destinations.every(d=>d.host==='udm-fixture.invalid'));},{proxy});
 mode='deny';await run('socks5-denied','http://udm-fixture.invalid:'+server.address().port+'/archive.zip?case=socks-denied',(r,req)=>{rejected(r);assert.equal(req.length,0);},{proxy});mode='none';
 await run('tls-certificate-rejected','https://127.0.0.1:'+tls.address().port+'/archive.zip',rejected);

 if(process.env.UDM_ZIP_UI_EXE){
  for(const [name,key,settings] of [['file-info','small',{screenshot:true}],['draft-login','basic',{draftLogin:true}],['cancel','stall',{cancel:true}],['error','expired',{error:true,entries:0}],['nonzip-hidden','small',{filename:'fixture.bin',hidden:true}],['post-hidden','small',{post:true,hidden:true}]]){
   const folder=path.join(out,'ui-'+name);fs.mkdirSync(folder,{recursive:true});const file=path.join(folder,'input.json');
   fs.writeFileSync(file,JSON.stringify({url:base+'/archive.'+(name==='nonzip-hidden'?'bin':'zip')+'?case='+key,...settings}));
   try{
    const code=await new Promise((resolve,reject)=>{const child=spawn(process.env.UDM_ZIP_UI_EXE,[file],{windowsHide:true,stdio:'ignore'});const timer=setTimeout(()=>{child.kill();reject(Error('UI fixture deadline'));},16000);child.once('error',reject);child.once('close',c=>{clearTimeout(timer);resolve(c);});});
    const r=JSON.parse(fs.readFileSync(path.join(folder,'ui-result.json'),'utf8').replace(/^\uFEFF/,''));assert.equal(code,0,JSON.stringify(r));assert(r.passed);results.push({name:'ui-'+name,passed:true,result:r});console.log('PASS ui-'+name);
   }catch(e){results.push({name:'ui-'+name,passed:false,error:e.stack});console.error('FAIL ui-'+name+': '+e.message);}
  }
 }
 }finally{for(const socket of sockets)socket.destroy();await Promise.all([server,second,tls,socks].map(s=>new Promise(r=>s.close(r))));fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({scope:'Actual native ZIP reader against controlled HTTP endpoints; independent Python zipfile fixtures. Not a live IDM comparison.',passed:results.filter(x=>x.passed).length,failed:results.filter(x=>!x.passed).length,results},null,2));if(results.some(x=>!x.passed))process.exitCode=1;}})().catch(e=>{console.error(e);process.exitCode=1});
