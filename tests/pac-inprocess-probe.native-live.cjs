'use strict';
// Finite scripts only: check actual evaluation location before using any cancellation stress case.
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict');
const {promisify}=require('node:util'),execFile=promisify(require('node:child_process').execFile);
const root=path.resolve(process.argv[2]),probe=path.resolve(process.argv[3]);
assert(!fs.existsSync(root));assert(fs.existsSync(probe));fs.mkdirSync(root,{recursive:true});
const scripts={
 direct:'function FindProxyForURL(){return "DIRECT";}',
 busy:'function FindProxyForURL(){var end=new Date().getTime()+1500;for(var i=0;i<10000000&&new Date().getTime()<end;i++){}return "DIRECT";}',
 alternatives:'function FindProxyForURL(){return "PROXY 127.0.0.1:18001; PROXY 127.0.0.1:18002; DIRECT";}',
 socks:'function FindProxyForURL(){return "SOCKS 127.0.0.1:18003; DIRECT";}',
 secure:'function FindProxyForURL(){return "HTTPS 127.0.0.1:18004; DIRECT";}'
};
const results=[],sockets=new Set();
const server=http.createServer((q,r)=>{const name=q.url.slice(1,-4);r.setHeader('Content-Type','application/x-ns-proxy-autoconfig');r.end(scripts[name]||scripts.direct);});
server.on('connection',s=>{sockets.add(s);s.once('close',()=>sockets.delete(s));});
(async()=>{
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
 for(const name of Object.keys(scripts)){
  let output;try{output=await execFile(probe,['http://127.0.0.1:'+server.address().port+'/'+name+'.pac','http://udm-owned-pac-probe.invalid/file'],{windowsHide:true,timeout:10000});}catch(error){output=error;if(!error.stdout)throw error;}
  const result={name,...JSON.parse(output.stdout)};results.push(result);console.log(JSON.stringify(result));
 }
 const busy=results.find(r=>r.name==='busy');
 assert(busy.ok&&busy.cpuMs>=750,'Finite busy script did not demonstrate evaluator CPU ownership inside the helper');
})().catch(error=>{results.push({error:error.message});console.error(error.message);process.exitCode=1;}).finally(async()=>{
 for(const socket of sockets)socket.destroy();await new Promise(resolve=>server.close(resolve));
 fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({finiteScriptsOnly:true,results},null,2));
});
