'use strict';
const net=require('node:net'),assert=require('node:assert/strict');
function connectFixture(ftp,options={}){
 const o={...options},sockets=new Set(),metrics={destinations:[],authenticated:0,accepted:0,errors:[]};let dataDenied=false;
 const controlPort=Number(new URL(ftp.url).port),track=s=>{sockets.add(s);s.on('error',()=>{});s.on('close',()=>sockets.delete(s));return s;};
 const server=net.createServer(async s=>{track(s);let buffer=Buffer.alloc(0),wake=null,ended=false;
  const onData=b=>{buffer=Buffer.concat([buffer,b]);if(wake){const f=wake;wake=null;f();}},onEnd=()=>{ended=true;if(wake){const f=wake;wake=null;f();}};
  s.on('data',onData);s.on('close',onEnd);
  try{
   if(o.stall)return;
   while(buffer.indexOf('\r\n\r\n')<0){if(ended)return;assert(buffer.length<32768);await new Promise(r=>wake=r);}
   const boundary=buffer.indexOf('\r\n\r\n')+4,header=buffer.subarray(0,boundary).toString();buffer=buffer.subarray(boundary);
   const match=/^CONNECT (\S+):(\d+) HTTP\/1.1\r\n/.exec(header);assert(match,'CONNECT required');
   const host=match[1],port=Number(match[2]);assert(['udm-ftp.invalid','127.0.0.1','198.51.100.25'].includes(host));assert(port===controlPort||ftp.ports.has(port));
   assert(header.includes('\r\nHost: '+host+':'+port+'\r\n'));metrics.destinations.push({host,port,domain:host==='udm-ftp.invalid'});
   const auth=/\r\nProxy-Authorization: ([^\r]+)\r\n/i.exec(header)?.[1];
   assert.equal(auth,o.user?'Basic '+Buffer.from(o.user+':'+(o.password||'')).toString('base64'):undefined);
   if(o.user)metrics.authenticated++;
   const data=port!==controlPort,deny=o.deny||(data&&o.denyData)||(data&&o.denyDataOnce&&!dataDenied);
   if(data&&deny)dataDenied=true;
   if(o.rejectAuth){s.end('HTTP/1.1 407 Proxy Authentication Required\r\nContent-Length: 0\r\n\r\n');return;}
   if(deny){s.end('HTTP/1.1 '+(o.denyDataOnce?'503 Unavailable':'403 Forbidden')+'\r\nContent-Length: 0\r\n\r\n');return;}
   if(o.reply==='bad-status'){s.end('HTTP/1.1 twenty Connected\r\n\r\n');return;}
   if(o.reply==='switch'){s.end('HTTP/1.1 101 Switching Protocols\r\n\r\n');return;}
   if(o.reply==='oversized'){s.end('HTTP/1.1 200 Connected\r\nX-Long: '+'x'.repeat(33000)+'\r\n\r\n');return;}
   if(o.reply==='null'){s.end('HTTP/1.1 200 Connected\r\nX: bad\0value\r\n\r\n');return;}
   if(o.reply==='interim-loop'){s.end(('HTTP/1.1 100 Continue\r\n\r\n').repeat(7));return;}
   if(data&&o.stallDataHandshake)return;
   const remote=track(net.connect({host:'127.0.0.1',port}));await new Promise((resolve,reject)=>{remote.once('connect',resolve);remote.once('error',reject);});remote.pause();metrics.accepted++;
   let reply=(o.reply==='interim'?'HTTP/1.1 100 Continue\r\n\r\n':'')+'HTTP/1.1 200 Connection established\r\n'+(o.reply==='length'?'Content-Length: 999\r\n':'')+'\r\n';
   if(o.fragmented){for(const byte of Buffer.from(reply)){s.write(Buffer.from([byte]));await new Promise(r=>setTimeout(r,1));}}else s.write(reply);
   s.pause();s.removeListener('data',onData);if(buffer.length)s.unshift(buffer);s.pipe(remote);remote.pipe(s);remote.resume();s.resume();s.once('close',()=>remote.destroy());remote.once('close',()=>s.destroy());
  }catch(e){if(!ended)metrics.errors.push(e.message);s.destroy();}
 });
 return {o,metrics,async start(){await new Promise(r=>server.listen(0,'127.0.0.1',r));this.settings={address:'127.0.0.1:'+server.address().port,mode:'Use a proxy server',...(o.user?{user:o.user,password:o.password||''}:{})};return this;},close(){for(const s of sockets)s.destroy();server.close();}};
}
module.exports={connectFixture};
