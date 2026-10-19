'use strict';
// Exercise the existing Windows PAC resolver before enabling inline browser scripts.
const fs=require('node:fs'),path=require('node:path'),http=require('node:http');
const assert=require('node:assert/strict'),{promisify}=require('node:util');
const execFile=promisify(require('node:child_process').execFile);
assert(process.argv[2]&&process.argv[3],'Supply fresh output directory and native probe');
const root=path.resolve(process.argv[2]),probe=path.resolve(process.argv[3]);
assert(!fs.existsSync(root));assert(fs.existsSync(probe));fs.mkdirSync(root,{recursive:true});
const results=[],servers=[],sockets=new Set(),scriptRequests=[];let originHits=0;
async function listen(handler){
 const server=http.createServer(handler);server.on('connection',socket=>{sockets.add(socket);socket.once('close',()=>sockets.delete(socket));});
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));servers.push(server);return server;
}
(async()=>{
 const origin=await listen((q,r)=>{originHits++;r.end('fixture');});
 const validScript='function FindProxyForURL(url,host){return "PROXY 127.0.0.1:'+origin.address().port+'";}';
 const script=mode=>mode==='invalid'?'function broken(':mode==='unicode'?'/* é漢 */'+validScript:mode==='boundary'?'/*'+'x'.repeat(65536-validScript.length-4)+'*/'+validScript:validScript;
 const pac=await listen((q,r)=>{
  scriptRequests.push(q.url);
  r.setHeader('Content-Type','application/x-ns-proxy-autoconfig');
  if(q.url==='/delayed.pac')return; // Client cancellation must release a stalled fetch.
  r.end(script(q.url.slice(1,-4)));
 });
 for(const mode of ['valid','unicode','boundary','delayed','loop','invalid']){
  // Closing WinHTTP's resolver returned to the caller but left three system PAC
  // workers spinning after prior runs. Do not submit another infinite script.
  // Keep this a failing qualification gate until execution is owned and bounded.
  if(mode==='loop'){
   const error='Infinite-script execution cancellation is unqualified: WinHTTP worker survived caller cancellation.';
   results.push({mode,blocked:true,ok:false,error});console.error('BLOCKED '+error);process.exitCode=1;continue;
  }
  const config=path.join(root,mode+'-config.json'),output=path.join(root,mode+'-result.json');
  const cancelling=mode==='delayed'||mode==='loop';
  const inline=process.env.UDM_PAC_INLINE==='1'&&mode!=='delayed';
  fs.writeFileSync(config,JSON.stringify({cases:[{id:mode,url:'http://udm-pac-cancel.invalid/payload',settings:{ProxyMode:'Use automatic configuration script',...(inline?{ProxyAutoConfigScript:script(mode)}:{ProxyAutoConfigUrl:`http://127.0.0.1:${pac.address().port}/${mode}.pac`})},...(cancelling?{cancelAfterMs:200}:{})}]}));
  const before=originHits,started=Date.now();
  await execFile(probe,[config,output],{timeout:10000,windowsHide:true});
  const result=JSON.parse(fs.readFileSync(output,'utf8')).cases[0],wallMs=Date.now()-started;
  results.push({mode,inline,ok:result.ok,cancelled:!!result.cancelled,elapsedMs:result.elapsedMs,wallMs,originRequests:originHits-before,scriptRequests:[...scriptRequests]});
  if(['valid','unicode','boundary'].includes(mode)){assert(result.ok,result.error);assert.equal(fs.readFileSync(path.join(root,mode+'.bin'),'utf8'),'fixture');}
  else {assert.equal(result.ok,false);assert.equal(originHits,before,'No direct fallback after failure/cancellation');}
  if(cancelling){assert(result.cancelled,result.error);assert(wallMs<3000,'PAC cancellation must not await script completion');}
  console.log('PASS '+mode+' '+wallMs+' ms');
 }
})().catch(error=>{results.push({error:error.message});console.error(error.message);process.exitCode=1;}).finally(async()=>{
 for(const socket of sockets)socket.destroy();
 for(const server of servers)await new Promise(resolve=>server.close(resolve));
 fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({results},null,2));
});
