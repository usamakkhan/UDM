'use strict';
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),{spawn}=require('node:child_process');
const root=path.resolve(process.argv[2]),exe=path.resolve(process.argv[3]);
assert(!fs.existsSync(root));fs.mkdirSync(root,{recursive:true});const results=[];
const reply=result=>({ok:true,interrupted:false,error:'',result});
async function check(name,payload,expected){
 const child=spawn(exe,['--routes'],{windowsHide:true,stdio:['pipe','pipe','pipe']});let output='',expired=false;
 const timer=setTimeout(()=>{expired=true;child.kill()},4000);
 const ended=new Promise((resolve,reject)=>{child.once('error',reject);child.once('close',resolve)});
 child.stdout.on('data',data=>output+=data);child.stdin.on('error',()=>{});
 child.stdin.end(typeof payload==='string'?payload:JSON.stringify(payload));
 try{const code=await ended;assert(!expired);const parsed=JSON.parse(output);
  if(expected){assert.equal(code,0);assert.deepEqual(parsed,expected)}else{assert.equal(code,1);assert.equal(typeof parsed.error,'string')}
  results.push({name,passed:true});console.log('PASS '+name);
 }finally{clearTimeout(timer)}
}
(async()=>{
 await check('ordered-routes',reply('PROXY one.test:80; HTTPS two.test:443; SOCKS three.test:1080; SOCKS5 four.test:1081; DIRECT'),[
  {kind:'http',host:'one.test',port:80},{kind:'https',host:'two.test',port:443},
  {kind:'socks4',host:'three.test',port:1080},{kind:'socks5',host:'four.test',port:1081},{kind:'direct',host:'',port:0}]);
 await check('ipv6',reply('HTTP [::1]:8080'),[{kind:'http',host:'::1',port:8080}]);
 await check('whitespace-trailing-separator',reply(' \tSOCKS4 host.test:1080 ; DIRECT; '),[{kind:'socks4',host:'host.test',port:1080},{kind:'direct',host:'',port:0}]);
 await check('no-implicit-direct',reply('PROXY host.test:80'),[{kind:'http',host:'host.test',port:80}]);
 for(const [name,text] of [
  ['empty',''],['empty-alternatives',' ; ; '],['unsupported','QUIC host.test:443; DIRECT'],
  ['missing-port','PROXY host.test'],['zero-port','PROXY host.test:0'],['large-port','PROXY host.test:65536'],
  ['signed-port','PROXY host.test:+80'],['junk-port','PROXY host.test:80x'],['empty-host','PROXY :80'],
  ['credentials','PROXY user:pass@host.test:80'],['url','PROXY http://host.test:80'],
  ['bad-ipv6','PROXY [bogus]:80'],['unbracketed-ipv6','PROXY ::1:80'],
  ['direct-argument','DIRECT host.test'],['nul','DIRECT\0'],['line-in-host','PROXY ho\nst.test:80'],
  ['too-many',Array(33).fill('DIRECT').join(';')],['oversized',' '.repeat(8193)]
 ])await check(name,reply(text));
 await check('wrong-ok-type',{...reply('DIRECT'),ok:'true'});
 await check('interrupted-success',{...reply('DIRECT'),interrupted:true});
 await check('error-success',{...reply('DIRECT'),error:'failed'});
 await check('nonstring-result',reply([]));
 await check('malformed-json','{');
 await check('trailing-json',JSON.stringify(reply('DIRECT'))+'{}');
 await check('deep-json','['.repeat(10000)+']'.repeat(10000));
 await check('duplicate-field','{"ok":false,"ok":true,"result":"DIRECT","interrupted":false,"error":""}');
})().catch(error=>{results.push({failed:true,error:error.message});console.error(error);process.exitCode=1})
.finally(()=>fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({results},null,2)));
