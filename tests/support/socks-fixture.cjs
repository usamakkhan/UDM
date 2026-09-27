'use strict';
const net=require('node:net'),assert=require('node:assert/strict');
// The reserved destination is deliberately unresolvable outside this proxy.
// Only our FTP control port and its own passive listeners are forwarded locally.
function socksFixture(ftp,options={}){
 const o={version:5,...options},sockets=new Set(),metrics={destinations:[],authenticated:0,accepted:0,errors:[]};let dataDenied=false;
 const controlPort=Number(new URL(ftp.url).port);
 const track=s=>{sockets.add(s);s.on('error',()=>{});s.on('close',()=>sockets.delete(s));return s;};
 const server=net.createServer(async s=>{track(s);let buffer=Buffer.alloc(0),wake=null,ended=false;
  const onData=b=>{buffer=Buffer.concat([buffer,b]);if(wake){const w=wake;wake=null;w();}};
  const onEnd=()=>{ended=true;if(wake){const w=wake;wake=null;w();}};
  s.on('data',onData);s.on('close',onEnd);
  const read=async n=>{while(buffer.length<n){if(ended)throw Error('closed');await new Promise(r=>wake=r);}const b=buffer.subarray(0,n);buffer=buffer.subarray(n);return b;};
  const zero=async()=>{let b=[];for(let i=0;i<256;i++){const v=(await read(1))[0];if(!v)return Buffer.from(b).toString();b.push(v);}throw Error('oversize string');};
  try{
   if(o.stall)return;
   let host,port,domain=false;
   if(o.version===4){const h=await read(8);assert.equal(h[0],4);assert.equal(h[1],1);port=h.readUInt16BE(2);const user=await zero();assert.equal(user,o.user||'');metrics.authenticated++;domain=h[4]===0&&h[5]===0&&h[6]===0&&h[7]!==0;host=domain?await zero():Array.from(h.subarray(4)).join('.');}
   else {const h=await read(2);assert.equal(h[0],5);const methods=await read(h[1]);const method=o.user?2:0;assert.ok(methods.includes(method));s.write(Buffer.from([5,method]));
    if(method===2){const a=await read(2);assert.equal(a[0],1);const user=(await read(a[1])).toString(),n=(await read(1))[0],password=(await read(n)).toString();const ok=!o.rejectAuth&&user===o.user&&password===o.password;s.write(Buffer.from([1,ok?0:1]));if(!ok)return s.end();metrics.authenticated++;}
    const h2=await read(4);assert.deepEqual(Array.from(h2.subarray(0,3)),[5,1,0]);if(h2[3]===3){domain=true;host=(await read((await read(1))[0])).toString();}else if(h2[3]===1)host=Array.from(await read(4)).join('.');else throw Error('Unexpected fixture address family');port=(await read(2)).readUInt16BE();
   }
   metrics.destinations.push({host,port,domain});assert.ok(['udm-ftp.invalid','198.51.100.25','127.0.0.1'].includes(host));assert.ok(port===controlPort||Array.from(ftp.ports||[]).includes(port),'Unexpected destination port');
   const data=port!==controlPort,deny=o.deny||(data&&o.denyData)||(data&&o.denyDataOnce&&!dataDenied);if(data&&deny)dataDenied=true;
   const code=deny?(o.denyDataOnce?5:2):0;
   let reply=o.version===4?Buffer.from([0,deny?91:90,0,0,127,0,0,1]):Buffer.from([5,code,0,1,127,0,0,1,0,0]);
   if(o.reply==='bad-version')reply[0]=7;if(o.reply==='reserved')reply[2]=8;
   if(o.reply==='empty-domain')reply=Buffer.from([5,0,0,3,0,0,0]);
   if(o.reply==='bad-type')reply=Buffer.from([5,0,0,9]);
   if(o.reply==='domain')reply=Buffer.from([5,0,0,3,3,102,116,112,0,0]);
   if(o.reply==='ipv6')reply=Buffer.concat([Buffer.from([5,0,0,4]),Buffer.alloc(18)]);
   if(deny||['bad-version','reserved','empty-domain','bad-type'].includes(o.reply)){s.end(reply);return;}
   if(data&&o.stallDataHandshake)return;
   const remote=track(net.connect({host:'127.0.0.1',port}));await new Promise((resolve,reject)=>{remote.once('connect',resolve);remote.once('error',reject);});metrics.accepted++;
   if(o.fragmented){for(const b of reply){s.write(Buffer.from([b]));await new Promise(r=>setTimeout(r,2));}}else s.write(reply);
   s.pause();s.removeListener('data',onData);if(buffer.length)s.unshift(buffer);s.pipe(remote);remote.pipe(s);s.resume();s.once('close',()=>remote.destroy());remote.once('close',()=>s.destroy());
  }catch(e){if(e.message!=='closed')metrics.errors.push(e.message);s.destroy();}
 });
 return {o,metrics,async start(){await new Promise(r=>server.listen(0,'127.0.0.1',r));this.settings={address:'127.0.0.1:'+server.address().port,mode:o.version===4?'Use a SOCKS4 / 4a proxy':'Use a SOCKS5 proxy',...(o.user?{user:o.user,password:o.password||''}:{})};return this;},close(){for(const s of sockets)s.destroy();server.close();}};
}
module.exports={socksFixture};
