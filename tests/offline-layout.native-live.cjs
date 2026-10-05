'use strict';
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict'),crypto=require('node:crypto');
const {spawn,execFileSync}=require('node:child_process');
const {pathToFileURL}=require('node:url');
const exe=process.env.UDM_TEST_EXE||path.join(__dirname,'../release-native/Udm.NativeTests.exe');
const root=path.resolve(process.env.UDM_TEST_ROOT||path.join(__dirname,'../benchmarks/offline-layout-'+Date.now()));
fs.mkdirSync(root,{recursive:true});
const requests=[],results=[],sockets=new Set();let origin,cdnOrigin,browser;
const svg=(width=16,color='green')=>'<svg xmlns="http://www.w3.org/2000/svg" width="'+width+'" height="16"><rect width="'+width+'" height="16" fill="'+color+'"/></svg>';
const serve=(req,res)=>{
 requests.push({host:req.headers.host,url:req.url});
 const send=(type,body)=>{res.writeHead(200,{'Content-Type':type,'Content-Length':Buffer.byteLength(body)});res.end(body);};
 if(req.url==='/docs/start.html')return send('text/html', '<!doctype html><html><head><meta charset="utf-8"><title>Nested offline fixture</title><base href="/docs/"><link rel="stylesheet" href="/styles/main.css"><link rel="alternate" href="/UDM-offline-report.json"></head><body><h1>Nested offline website</h1>'+
  '<a id="next" href="next.html#part">Next page</a> <a id="base-anchor" href="#shared">Base anchor</a> <a id="server-index" href="/index.html">Original server index</a>'+
  '<img id="picture" src="/assets/pixel.svg"><img id="unicode" src="/assets/%E6%96%87%20space/%E5%9B%BE%23%25.svg"><img id="query-one" src="/assets/image?id=1"><img id="query-two" src="/assets/image?id=2">'+
  '<img id="upper" src="/Case/one.svg"><img id="lower" src="/case/one.svg"><link rel="stylesheet" href="/assets/occupied.css"><img id="occupied" src="/assets/occupied.css/child.svg">'+
  '<img id="site-external" src="/_external/local.svg"><img id="cdn" src="'+cdnOrigin+'/assets/pixel.svg"><img id="alias" src="/alias.svg"><img id="set" srcset="/assets/pixel.svg 1x, /assets/large.svg 2x">'+
  '<img id="sanitized-a" src="/a%3Ab/pixel.svg"><img id="sanitized-b" src="/a_b/pixel.svg"><img id="missing" src="/missing.svg"><img id="unsafe" src="/assets/%2F/escape.svg"><script>fetch("/never-run")</script></body></html>');
 if(req.url==='/docs/next.html')return send('text/html','<!doctype html><html><head><title>Next page</title><link rel="stylesheet" href="/styles/main.css"></head><body><h1 id="part">Offline next page</h1><a id="back" href="start.html">Back</a><img src="/assets/pixel.svg"></body></html>');
 if(req.url==='/docs/')return send('text/html','<html><head><title>Base document</title></head><body><p id="shared">Correct base document</p></body></html>');
 if(req.url==='/index.html')return send('text/html','<html><body><h1>Original server index</h1></body></html>');
 if(req.url==='/styles/main.css')return send('text/css','@import "nested/theme.css";h1{color:rgb(19,71,129);background-image:url("../assets/pixel.svg")}body{font-family:Arial}');
 if(req.url==='/styles/nested/theme.css')return send('text/css','body{background-color:rgb(230,240,250)}h1{border-left:3px solid rgb(14,83,117)}');
 if(req.url==='/assets/occupied.css')return send('text/css','h1{padding-left:8px}');
 if(req.url==='/UDM-offline-report.json')return send('application/json','{"site":"original report-named resource"}');
 if(req.url==='/alias.svg'){res.writeHead(302,{Location:'/assets/pixel.svg','Content-Length':'0'});return res.end();}
 if(req.url==='/missing.svg'){res.writeHead(404);return res.end();}
 if(req.url==='/resume/start.html')return send('text/html','<html><head><link rel="stylesheet" href="/styles/main.css"></head><body><h1>Resume</h1><img src="/assets/pixel.svg"><img src="/resume/slow.svg"></body></html>');
 if(req.url==='/resume/slow.svg'){const timer=setTimeout(()=>send('image/svg+xml',svg()),4500);res.once('close',()=>clearTimeout(timer));return;}
 if(req.url==='/')return send('text/html','<!doctype html><html><body><h1>Root starting page</h1><img src="/assets/pixel.svg"></body></html>');
 if(req.url.includes('.svg')||req.url.startsWith('/assets/image?'))return send('image/svg+xml',svg(req.url.endsWith('id=1')?17:req.url.endsWith('id=2')?19:16));
 res.writeHead(404);res.end();
};
const server=http.createServer(serve),cdn=http.createServer(serve);
for(const s of [server,cdn])s.on('connection',socket=>{sockets.add(socket);socket.on('close',()=>sockets.delete(socket));});
function run(name,spec){
 const folder=path.join(root,name);fs.mkdirSync(folder,{recursive:true});const input=path.join(folder,'input.json');fs.writeFileSync(input,JSON.stringify(spec));
 return new Promise((resolve,reject)=>{
  const child=spawn(exe,['--feature-spec',input],{windowsHide:true});let output='',error='';
  const timer=setTimeout(()=>{child.kill();reject(Error('Offline fixture exceeded 30 seconds'));},30000);
  child.stdout.on('data',b=>output+=b);child.stderr.on('data',b=>error+=b);child.once('error',e=>{clearTimeout(timer);reject(e);});
  child.once('close',code=>{clearTimeout(timer);fs.writeFileSync(path.join(folder,'stdout.log'),output);fs.writeFileSync(path.join(folder,'stderr.log'),error);
   try{assert.equal(code,0,error||output);resolve(JSON.parse(fs.readFileSync(path.join(folder,'result.json'),'utf8')));}catch(e){reject(e);}
  });
 });
}
const project=(name,extra={})=>({Template:'Offline website (ZIP)',OriginalSubfolders:true,SaveMode:'Folder',Folder:path.join(root,name,'downloads'),Depth:1,MaxPages:10,MaxMiB:16,ExternalAssets:true,...extra});
function extract(result,name){
 assert.equal(result.status,'Complete',result.error);
 assert.equal(crypto.createHash('sha256').update(fs.readFileSync(result.path)).digest('hex'),result.sha256);
 const destination=path.join(root,name);fs.mkdirSync(destination,{recursive:true});
 const script="import sys,pathlib,zipfile,json\nroot=pathlib.Path(sys.argv[2]).resolve()\nwith zipfile.ZipFile(sys.argv[1]) as z:\n names=z.namelist();assert z.testzip() is None\n assert len(names)==len(set(n.casefold() for n in names))\n for n in names:\n  assert '\\\\' not in n and ':' not in n and not n.startswith('/')\n  assert all(p not in ('','.','..') for p in n.split('/'))\n  assert (root/n).resolve().is_relative_to(root)\n  assert not any(p.as_posix().casefold() in set(x.casefold() for x in names) for p in pathlib.PurePosixPath(n).parents if p.as_posix()!='.')\n z.extractall(root)\n print(json.dumps(names))";
 const names=JSON.parse(execFileSync(process.env.UDM_TEST_PYTHON||'python',['-c',script,result.path,destination],{windowsHide:true,encoding:'utf8'}));
 return {destination,names,report:JSON.parse(fs.readFileSync(path.join(destination,'UDM-offline-report.json'),'utf8'))};
}
function receipt(){fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({nativeExe:exe,passed:results.filter(x=>x.passed).length,failed:results.filter(x=>!x.passed).length,requests,results},null,2));}
async function check(name,fn){try{const detail=await fn();results.push({name,passed:true,detail});console.log('PASS '+name);}catch(e){results.push({name,passed:false,error:e.stack});console.error('FAIL '+name+': '+e.message);}receipt();}
(async()=>{
 await Promise.all([server,cdn].map(s=>new Promise(resolve=>s.listen(0,'127.0.0.1',resolve))));origin='http://127.0.0.1:'+server.address().port;cdnOrigin='http://127.0.0.1:'+cdn.address().port;
 let saved,archive;
 await check('Native capture preserves hierarchy and publishes a verified ZIP',async()=>{
  saved=await run('nested',{url:origin+'/docs/start.html',offline:project('nested')});archive=extract(saved,'extracted');
  assert.equal(path.dirname(saved.path),path.join(root,'nested','downloads'));assert.equal(archive.report.OriginalSubfolders,true);assert.equal(archive.report.Entry,'docs/start.html');
  for(const name of ['index.html','docs/start.html','docs/next.html','docs/index.html','styles/main.css','styles/nested/theme.css'])assert.ok(archive.names.includes(name),name);
  const state=JSON.parse(fs.readFileSync(path.join(root,'nested/state/state.json')));assert.equal(state.Downloads[0].OfflineProject.OriginalSubfolders,true);
  return {saved,names:archive.names,report:archive.report};
 });
 await check('Query, case, sanitized, external and file-directory collisions remain distinct',async()=>{
  const map=archive.report.Files;
  for(const pair of [['/assets/image?id=1','/assets/image?id=2'],['/Case/one.svg','/case/one.svg'],['/a%3Ab/pixel.svg','/a_b/pixel.svg']])assert.notEqual(map[origin+pair[0]].toLowerCase(),map[origin+pair[1]].toLowerCase());
  assert.notEqual(map[origin+'/assets/occupied.css'],path.posix.dirname(map[origin+'/assets/occupied.css/child.svg']));
  assert.ok(map[cdnOrigin+'/assets/pixel.svg'].startsWith('_external/'));assert.ok(!map[origin+'/_external/local.svg'].startsWith('_external/'));
  assert.notEqual(map[origin+'/UDM-offline-report.json'],'UDM-offline-report.json');
  assert.notEqual(map[origin+'/index.html'],'index.html');
  assert.equal(JSON.parse(fs.readFileSync(path.join(archive.destination,map[origin+'/UDM-offline-report.json']))).site,'original report-named resource');
  return {uniqueNames:archive.names.length};
 });
 await check('Redirect aliases share one archived resource and omitted paths are reported',async()=>{
  assert.equal(archive.report.Files[origin+'/alias.svg'],archive.report.Files[origin+'/assets/pixel.svg']);
  assert.ok(archive.report.Errors.some(x=>x.Url===origin+'/missing.svg'));
  assert.ok(archive.report.Errors.some(x=>x.Url.includes('%2F')));
  assert.ok(!requests.some(x=>x.url==='/never-run'));
  assert.ok(!fs.existsSync(path.join(root,'nested/state/parts',saved.jobId,'offline')));
  return {omitted:archive.report.Errors};
 });
 browser=await require('playwright').chromium.launch({channel:'msedge',headless:true});
 await check('Extracted hierarchy renders CSS, Unicode images and srcset in offline Edge',async()=>{
  const context=await browser.newContext({offline:true}),page=await context.newPage(),network=[];page.on('request',r=>{if(/^https?:/.test(r.url()))network.push(r.url());});
  try{
   await page.goto(pathToFileURL(path.join(archive.destination,'index.html')).href);
   assert.equal(await page.locator('h1').innerText(),'Nested offline website');
   assert.equal(await page.locator('h1').evaluate(e=>getComputedStyle(e).color),'rgb(19, 71, 129)');
   assert.equal(await page.locator('body').evaluate(e=>getComputedStyle(e).backgroundColor),'rgb(230, 240, 250)');
   assert.equal(await page.locator('h1').evaluate(e=>getComputedStyle(e).paddingLeft),'8px');
   for(const id of ['picture','unicode','upper','lower','occupied','site-external','cdn','alias','set','sanitized-a','sanitized-b'])assert.equal(await page.locator('#'+id).evaluate(e=>e.naturalWidth),16,id);
   assert.equal(await page.locator('#query-one').evaluate(e=>e.naturalWidth),17);assert.equal(await page.locator('#query-two').evaluate(e=>e.naturalWidth),19);
   await page.screenshot({path:path.join(root,'offline-hierarchy-edge.png')});assert.equal(network.length,0);
   return {networkRequests:network.length,images:13,screenshot:path.join(root,'offline-hierarchy-edge.png')};
  }finally{await context.close();}
 });
 await check('Root shortcut, nested pages, base anchors and original index navigate offline',async()=>{
  const context=await browser.newContext({offline:true}),page=await context.newPage(),network=[];page.on('request',r=>{if(/^https?:/.test(r.url()))network.push(r.url());});
  const entry=pathToFileURL(path.join(archive.destination,'index.html')).href;
  try{
   await page.goto(entry);await page.locator('#next').click();assert.equal(await page.locator('#part').innerText(),'Offline next page');assert.ok(page.url().endsWith('/docs/next.html#part'));
   await page.locator('#back').click();assert.ok(page.url().endsWith('/docs/start.html'));assert.equal(await page.locator('#unicode').evaluate(e=>e.naturalWidth),16);
   await page.locator('#base-anchor').click();assert.ok(page.url().endsWith('/docs/index.html#shared'));assert.equal(await page.locator('#shared').innerText(),'Correct base document');
   await page.goto(entry);await page.locator('#server-index').click();assert.equal(await page.locator('h1').innerText(),'Original server index');
   assert.equal(network.length,0);return {networkRequests:network.length};
  }finally{await context.close();}
 });
 await check('Disabling external assets prevents CDN requests and leaves a usable archive',async()=>{
  const before=requests.length,result=await run('no-cdn',{url:origin+'/docs/start.html',offline:project('no-cdn',{ExternalAssets:false})}),got=extract(result,'no-cdn-extracted');
  assert.ok(!requests.slice(before).some(r=>r.host===new URL(cdnOrigin).host));assert.ok(!Object.keys(got.report.Files).some(u=>u.startsWith(cdnOrigin)));
  assert.ok(got.names.includes('docs/start.html'));return {resources:got.report.Resources};
 });
 await check('Paused hierarchical jobs resume the same record using verified cached pages',async()=>{
  const before=requests.length,paused=await run('resume',{url:origin+'/resume/start.html',offline:project('resume'),cancelMs:2000});
  assert.equal(paused.status,'Paused',JSON.stringify(paused));const startRequests=requests.filter(r=>r.url==='/resume/start.html').length;
  fs.writeFileSync(path.join(root,'paused-result.json'),JSON.stringify(paused,null,2));
  const cache=path.join(root,'resume/state/parts',paused.jobId,'offline'),checkpoint=JSON.parse(fs.readFileSync(path.join(cache,'cache.json'),'utf8'));
  fs.writeFileSync(path.join(root,'paused-checkpoint.json'),JSON.stringify(checkpoint,null,2));
  const cached=checkpoint.Files[origin+'/resume/start.html'];assert.ok(cached,'The paused fixture must contain a completed starting-page checkpoint');
  assert.equal(crypto.createHash('sha256').update(fs.readFileSync(path.join(cache,cached.Raw))).digest('hex'),cached.Hash);
  const final=await run('resume',{resume:true});assert.equal(final.jobId,paused.jobId);assert.equal(final.records,1);
  const got=extract(final,'resume-extracted');assert.equal(requests.filter(r=>r.url==='/resume/start.html').length,startRequests);assert.ok(got.names.includes('resume/start.html')&&got.names.includes('resume/slow.svg'));
  return {paused,final,requests:requests.slice(before)};
 });
 await check('Root starting pages do not create duplicate archive entry points',async()=>{
  const got=extract(await run('root-page',{url:origin+'/',offline:project('root-page')}),'root-extracted');assert.equal(got.names.filter(n=>n==='index.html').length,1);
  assert.equal(got.report.Entry,'index.html');assert.match(fs.readFileSync(path.join(got.destination,'index.html'),'utf8'),/Root starting page/);return got.names;
 });
 await check('Legacy flat archives retain flat layout and render in offline Edge',async()=>{
  const got=extract(await run('flat',{url:origin+'/docs/start.html',offline:project('flat',{OriginalSubfolders:false})}),'flat-extracted');
  assert.ok(got.names.every(n=>!n.includes('/')));assert.equal(got.report.OriginalSubfolders,false);
  const context=await browser.newContext({offline:true}),page=await context.newPage();try{
   await page.goto(pathToFileURL(path.join(got.destination,'index.html')).href);assert.equal(await page.locator('h1').evaluate(e=>getComputedStyle(e).color),'rgb(19, 71, 129)');assert.equal(await page.locator('#query-two').evaluate(e=>e.naturalWidth),19);
   await page.locator('#next').click();assert.equal(await page.locator('#part').innerText(),'Offline next page');
   return {entries:got.names.length};
  }finally{await context.close();}
 });
})().catch(e=>{console.error(e);results.push({name:'Fixture setup',passed:false,error:e.stack});process.exitCode=1;}).finally(async()=>{
 if(browser)await browser.close();for(const socket of sockets)socket.destroy();server.close();cdn.close();receipt();
 if(results.some(x=>!x.passed))process.exitCode=1;
 console.log(JSON.stringify({passed:results.filter(x=>x.passed).length,failed:results.filter(x=>!x.passed).length}));
});
