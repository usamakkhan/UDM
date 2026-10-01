'use strict';
// Test transport only: forward unchanged extension messages to a staged host
// bound to the test instance. This avoids altering the user's host registration.
const http=require('node:http'),crypto=require('node:crypto'),{spawn}=require('node:child_process');
module.exports=async function(exe,tag){
 const token='/'+crypto.randomBytes(24).toString('hex'),children=new Set();
 const server=http.createServer((req,res)=>{
  if(req.url!==token||req.method!=='POST'){res.writeHead(404).end();return;}
  const chunks=[];let bytes=0;req.on('data',b=>{bytes+=b.length;if(bytes>2097152)req.destroy();else chunks.push(b);});
  req.on('end',()=>{const body=Buffer.concat(chunks),header=Buffer.alloc(4);header.writeUInt32LE(body.length);
   const child=spawn(exe,[],{env:{...process.env,UDM_INSTANCE_TAG:tag},windowsHide:true,stdio:['pipe','pipe','ignore']});children.add(child);
   const out=[];let size=0;const timer=setTimeout(()=>child.kill(),40000);
   child.stdout.on('data',b=>{size+=b.length;if(size>262148)child.kill();else out.push(b);});child.stdin.on('error',()=>{});
   child.on('error',()=>{clearTimeout(timer);children.delete(child);res.writeHead(500).end('Host launch failed');});
   child.on('close',()=>{clearTimeout(timer);children.delete(child);const data=Buffer.concat(out);if(data.length<4||data.readUInt32LE(0)!==data.length-4){res.writeHead(502).end('Invalid native reply');return;}res.writeHead(200,{'Content-Type':'application/json','Access-Control-Allow-Origin':'*'}).end(data.subarray(4));});
   child.stdin.end(Buffer.concat([header,body]));
  });
 });
 await new Promise(r=>server.listen(0,'127.0.0.1',r));
 return {url:'http://127.0.0.1:'+server.address().port+token,close:async()=>{for(const child of children)child.kill();server.closeAllConnections();await new Promise(r=>server.close(r));}};
};
