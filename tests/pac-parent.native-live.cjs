'use strict';
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const {spawn}=require('node:child_process');
const root=path.resolve(process.argv[2]),exe=path.resolve(process.argv[3]);
assert(!fs.existsSync(root));assert(fs.existsSync(exe));fs.mkdirSync(root,{recursive:true});
const results=[];
function frame(script,large=false){
 const parts=[script,large?'http://example.test/'+ 'x'.repeat(8172):'http://example.test/',large?'a'.repeat(255):'example.test'].map(s=>Buffer.from(s));
 const header=Buffer.alloc(20);header.write('UDM1');parts.forEach((p,i)=>header.writeUInt32LE(p.length,4+i*4));header.writeUInt32LE(2000,16);
 return Buffer.concat([header,...parts]);
}
async function run(name,args,input,expected,code){
 const start=Date.now();let output='',expired=false;
 const child=spawn(exe,args,{windowsHide:true,stdio:['pipe','pipe','pipe']});
 const timer=setTimeout(()=>{expired=true;child.kill()},6000);
 const ended=new Promise((resolve,reject)=>{child.once('error',reject);child.once('close',resolve)});
 child.stdout.on('data',data=>output+=data);
 child.stdin.on('error',error=>{if(error.code!=='EPIPE')console.error(error)});
 child.stdin.end(input);
 try{
  const exitCode=await ended;assert(!expired,'Parent hung');assert.equal(exitCode,code);
  const reply=JSON.parse(output);
  if(Array.isArray(expected))assert.deepEqual(reply,expected);else assert.equal(code===0?reply.result:reply.error,expected);
  const wallMs=Date.now()-start;assert(wallMs<5000);
  results.push({name,passed:true,exitCode,wallMs});console.log('PASS '+name);
 }finally{clearTimeout(timer)}
}
(async()=>{
 const loop=frame('function FindProxyForURL(){while(true){}}');
 await run('parent-deadline',['--parent','100'],loop,'PAC evaluation timed out.',5);
 await run('parent-cancel',['--cancel','3000'],loop,'PAC evaluation cancelled.',5);
 const direct='function FindProxyForURL(){return "DIRECT"}';
 await run('large-input',['--parent','3000'],frame(direct+' '.repeat(65536-direct.length),true),'DIRECT',0);
 await run('large-output',['--parent','3000'],frame('function FindProxyForURL(){return "\\u0001".repeat(8192)}'),'\u0001'.repeat(8192),0);
 for(let i=0;i<5;++i)await run('repeat-cancel-'+i,['--cancel','3000'],loop,'PAC evaluation cancelled.',5);
 await run('after-cancellations',['--parent','3000'],frame(direct),'DIRECT',0);
 await run('end-to-end-routes',['--resolve','3000'],frame('function FindProxyForURL(url,host){return isPlainHostName(host)?"DIRECT":"HTTPS secure.test:443; SOCKS5 socks.test:1080; DIRECT"}'),[
  {kind:'https',host:'secure.test',port:443},{kind:'socks5',host:'socks.test',port:1080},{kind:'direct',host:'',port:0}],0);
 await run('end-to-end-invalid',['--resolve','3000'],frame('function FindProxyForURL(){return "PROXY broken; DIRECT"}'),
  'Proxy script returned an invalid route. No direct fallback was used.',5);
})().catch(error=>{results.push({failed:true,error:error.message});console.error(error);process.exitCode=1})
.finally(()=>fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({prototypeOnly:true,results},null,2)));
