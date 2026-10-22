'use strict';
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),{spawn}=require('node:child_process');
const root=path.resolve(process.argv[2]),exe=path.resolve(process.argv[3]);assert(!fs.existsSync(root));assert(fs.existsSync(exe));fs.mkdirSync(root,{recursive:true});
const results=[];
function run(file){return new Promise((resolve,reject)=>{
 const began=Date.now(),child=spawn(exe,[file,'250'],{windowsHide:true,stdio:['ignore','pipe','pipe']});let output='',error='',expired=false;
 const timer=setTimeout(()=>{expired=true;child.kill();},3000);
 child.stdout.on('data',data=>output+=data);child.stderr.on('data',data=>error+=data);child.once('error',e=>{clearTimeout(timer);reject(e);});
 child.once('close',code=>{clearTimeout(timer);try{assert(!expired,'Owned helper exceeded its external deadline');assert.notEqual(code,null);resolve({...JSON.parse(output),exitCode:code,workerExited:true,processWallMs:Date.now()-began});}catch(e){reject(Error(e.message+(error?' '+error:'')));}});
});}
(async()=>{
 for(const item of [
  ...require('./pac-string-functions.cjs'),
  ...require('./pac-dns-functions.cjs'),
  ...require('./pac-time-functions.cjs'),
  {name:'direct',source:'function FindProxyForURL(){return "DIRECT";}',value:'DIRECT'},
  {name:'ordered-routes',source:'function FindProxyForURL(){return "PROXY 127.0.0.1:18001; SOCKS 127.0.0.1:18002; HTTPS 127.0.0.1:18003; DIRECT";}',value:'PROXY 127.0.0.1:18001; SOCKS 127.0.0.1:18002; HTTPS 127.0.0.1:18003; DIRECT'},
  {name:'no-host-io',source:'function FindProxyForURL(){return [typeof std,typeof os,typeof require,typeof process,typeof ActiveXObject,typeof fetch].join(",");}',value:Array(6).fill('undefined').join(',')},
  {name:'unicode',source:'/* é漢 */ function FindProxyForURL(){return "DIRECT";}',value:'DIRECT'},
  {name:'top-level-loop',source:'while(true){}',interrupt:true},
  {name:'function-loop',source:'function FindProxyForURL(){while(true){}}',interrupt:true},
  {name:'glob-budget',source:'function FindProxyForURL(){return String(shExpMatch("a".repeat(1000000), "*"+"a".repeat(30000)+"b"));}',interrupt:true},
  {name:'memory-limit',source:'function FindProxyForURL(){return new ArrayBuffer(64*1024*1024);}',error:/memory/i},
  {name:'stack-limit',source:'function f(){return f()} function FindProxyForURL(){return f()}',error:/stack/i},
  {name:'syntax-error',source:'function broken(',error:/syntax/i},
  {name:'oversized-result',source:'function FindProxyForURL(){return "x".repeat(8193);}',error:/oversized/i},
  {name:'non-string-result',source:'function FindProxyForURL(){return 42;}',error:/string/i}
 ]){
  const file=path.join(root,item.name+'.pac');fs.writeFileSync(file,item.source);
  const result=await run(file);results.push({name:item.name,...result});
  if(item.value!==undefined){assert(result.ok,result.error);assert.equal(result.result,item.value);assert.equal(result.exitCode,0);}
  else {assert.equal(result.ok,false);assert.equal(result.exitCode,1);if(item.interrupt){assert(result.interrupted);assert(result.wallMs<1000);}if(item.error)assert.match(result.error,item.error);}
  console.log('PASS '+item.name+'; helper exited; '+result.wallMs+' ms');
 }
})().catch(error=>{results.push({failed:true,error:error.message});console.error(error);process.exitCode=1;}).finally(()=>fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({prototypeOnly:true,results},null,2)));
