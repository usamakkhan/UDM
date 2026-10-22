'use strict';
const {spawn}=require('node:child_process');
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const root=path.resolve(process.argv[2]),exe=path.resolve(process.argv[3]);
assert(!fs.existsSync(root));assert(fs.existsSync(exe));fs.mkdirSync(root,{recursive:true});
const results=[];
function frame(script,url='http://example.test/file',host='example.test',budget=250){
  const parts=[script,url,host].map(v=>Buffer.isBuffer(v)?v:Buffer.from(v));
  const header=Buffer.alloc(20);header.write('UDM1');
  parts.forEach((p,i)=>header.writeUInt32LE(p.length,4+i*4));header.writeUInt32LE(budget,16);
  return Buffer.concat([header,...parts]);
}
function run(input){return new Promise((resolve,reject)=>{
  const child=spawn(exe,process.argv[4]==='parent'?['--parent','3000']:['--request'],{windowsHide:true,stdio:['pipe','pipe','pipe']});
  let output='',expired=false;const timer=setTimeout(()=>{expired=true;child.kill()},4000);
  child.stdin.on('error',error=>{if(error.code!=='EPIPE')reject(error)});
  child.stdout.on('data',data=>output+=data);
  child.once('error',error=>{clearTimeout(timer);reject(error)});
  child.once('close',code=>{clearTimeout(timer);try{assert(!expired);assert(output.length<20000);resolve({code,...JSON.parse(output)})}catch(error){reject(error)}});
  child.stdin.end(input);
});}
(async()=>{
  const direct='function FindProxyForURL(){return "DIRECT"}';
  const badMagic=frame(direct);badMagic[3]=50;
  const huge=frame(direct);huge.writeUInt32LE(0xffffffff,4);
  const cases=[
    ['actual-url-host',frame('function FindProxyForURL(url,host){return url+"|"+host}', 'http://media.example.test/é?q=1','media.example.test'),'http://media.example.test/é?q=1|media.example.test'],
    ['request-routing',frame('function FindProxyForURL(url,host){return dnsDomainIs(host,".example.test")&&shExpMatch(url,"*/movie.mp4")?"PROXY 127.0.0.1:8080":"DIRECT"}', 'http://media.example.test/movie.mp4','media.example.test'),'PROXY 127.0.0.1:8080'],
    ['other-routing',frame('function FindProxyForURL(url,host){return host==="example.test"?"DIRECT":"PROXY 127.0.0.1:8080"}'),'DIRECT'],
    ['unicode-source',frame('/* 漢 */'+direct),'DIRECT'],
    ['binary-newline-source',frame('// first\r\n'+direct),'DIRECT'],
    ['bad-version',badMagic],['oversized-length',huge],
    ['truncated-header',Buffer.from('UDM1')],['truncated-body',frame(direct).subarray(0,23)],
    ['empty-script',frame('')],['nul-host',frame(direct,'http://example.test/','example.test\0x')],
    ['invalid-utf8',frame(Buffer.from([0xff]))],['url-newline',frame(direct,'http://example.test/\n')],
    ['host-slash',frame(direct,'http://example.test/','example.test/path')],
    ['invalid-budget',frame(direct,undefined,undefined,2001)]
  ];
  for(const [name,input,expected] of cases){
    const result=await run(input);
    if(expected!==undefined){assert.equal(result.code,0);assert.equal(result.result,expected)}
    else {assert.equal(result.code,2);assert.equal(result.ok,false);assert.equal(result.error,'Invalid PAC request')}
    results.push({name,passed:true,exitCode:result.code});console.log('PASS '+name);
  }
  const result=await run(frame('function FindProxyForURL(){throw "x".repeat(100000)}'));
  assert.equal(result.code,1);assert.equal(result.error.length,2048);
  results.push({name:'bounded-error',passed:true,exitCode:result.code});console.log('PASS bounded-error');
  const unicode=await run(frame('function FindProxyForURL(){throw "漢".repeat(10000)}'));
  assert.equal(unicode.code,1);assert.equal(unicode.error,'漢'.repeat(682));
  results.push({name:'bounded-utf8-error',passed:true,exitCode:unicode.code});console.log('PASS bounded-utf8-error');
})().catch(error=>{results.push({failed:true,error:error.message});console.error(error);process.exitCode=1})
.finally(()=>fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({prototypeOnly:true,results},null,2)));
