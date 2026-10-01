'use strict';
const http=require('node:http'),fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const root=__dirname, size=32*1024*1024, body=Buffer.alloc(size);
for(let i=0;i<size;i++)body[i]=(i*31+Math.floor(i/65536)+7)%251;
const sha256=crypto.createHash('sha256').update(body).digest('hex');
const events=[],sockets=new Map();let sequence=0,requestId=0;
const start=performance.now(), now=()=>+(performance.now()-start).toFixed(3);
function save(){fs.writeFileSync(path.join(root,'http-trace.json'),JSON.stringify({size,sha256,events},null,2));}
const server=http.createServer((req,res)=>{
 const match=/^\/(steady|slow)\/([a-zA-Z0-9_-]+)\.bin$/.exec(req.url);
 if(!match){res.writeHead(404,{'Content-Length':0});res.end();return;}
 const range=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||'');
 const begin=range?Number(range[1]):0,end=range&&range[2]?Number(range[2]):size-1;
 const id=++requestId,socket=sockets.get(req.socket),probe=begin===0&&end===0;
 const record={event:'request',id,connection:socket.id,at:now(),utc:Date.now(),path:req.url,method:req.method,range:req.headers.range||null,ifRange:req.headers['if-range']||null,agent:req.headers['user-agent']||null,begin,end,sent:0};events.push(record);
 if(begin<0||end<begin||end>=size){res.writeHead(416,{'Content-Range':`bytes */${size}`,'Content-Length':0});res.end();save();return;}
 const headers={'Content-Length':end-begin+1,'Content-Type':'application/octet-stream','Accept-Ranges':'bytes','ETag':`"${sha256}"`,'Last-Modified':'Sat, 19 Sep 2026 00:00:00 GMT','Cache-Control':'no-store','Connection':'keep-alive','Content-Disposition':`attachment; filename="${match[2]}.bin"`};
 if(range)headers['Content-Range']=`bytes ${begin}-${end}/${size}`;
 res.writeHead(range?206:200,headers);res.flushHeaders();
 if(req.method==='HEAD'){res.end();record.done=now();save();return;}
 const delay=match[1]==='slow'&&begin===0&&!probe?125:8;
 let position=begin,timer;const chunk=32768;
 function pump(){
  if(res.destroyed)return;
  const stop=Math.min(position+chunk,end+1),slice=body.subarray(position,stop);position=stop;record.sent+=slice.length;
  const ready=res.write(slice);
  if(position>end){res.end();return;}
  if(ready)timer=setTimeout(pump,delay);else res.once('drain',()=>{timer=setTimeout(pump,delay);});
 }
 res.once('close',()=>{clearTimeout(timer);record.closed=now();record.finished=res.writableFinished;save();});
 res.once('finish',()=>{record.done=now();save();});
 timer=setTimeout(pump,probe?0:delay);
});
server.keepAliveTimeout=15000;server.headersTimeout=20000;
server.on('connection',socket=>{const record={id:++sequence};sockets.set(socket,record);socket.setNoDelay(true);events.push({event:'connect',connection:record.id,at:now()});socket.once('close',()=>{events.push({event:'disconnect',connection:record.id,at:now()});sockets.delete(socket);save();});});
server.listen(Number(process.argv[2])||0,'127.0.0.1',()=>{const config={origin:`http://127.0.0.1:${server.address().port}`,size,sha256,started:new Date().toISOString(),pid:process.pid};fs.writeFileSync(path.join(root,'server.json'),JSON.stringify(config,null,2));console.log(JSON.stringify(config));});
const stop=()=>{save();server.close();for(const socket of sockets.keys())socket.destroy();setTimeout(()=>process.exit(),50);};
setTimeout(stop,30*60*1000);process.on('SIGTERM',stop);process.on('SIGINT',stop);
