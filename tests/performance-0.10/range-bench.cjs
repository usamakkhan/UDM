'use strict';
// Alternating runs of two production-engine binaries against identical local bytes.
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),crypto=require('node:crypto'),{spawn}=require('node:child_process'),assert=require('node:assert/strict');
const [baseline,candidate,output]=process.argv.slice(2);if(!output)throw Error('Use: node range-bench.cjs baseline.exe candidate.exe output-directory');
fs.mkdirSync(output,{recursive:true});const size=32*1024*1024,body=Buffer.allocUnsafe(size);for(let i=0;i<size;i++)body[i]=(i*31+Math.floor(i/65536)+7)%251;
const sha256=crypto.createHash('sha256').update(body).digest('hex'),records=[],runs=[],sockets=new Set();
const server=http.createServer((req,res)=>{
 const match=/^\/(latency|steady|straggler)\/([a-z0-9-]+)$/.exec(req.url);if(!match){res.writeHead(404);res.end();return;}
 const range=/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||''),begin=range?Number(range[1]):0,end=range&&range[2]?Number(range[2]):size-1,probe=begin===0&&end===0;
 if(begin<0||end<begin||end>=size){res.writeHead(416,{'Content-Range':`bytes */${size}`});res.end();return;}
 const record={label:match[2],profile:match[1],begin,end,probe,sent:0,started:performance.now()};records.push(record);
 let timer;res.on('close',()=>{clearTimeout(timer);record.closed=performance.now();record.finished=res.writableFinished;});
 const delay=match[1]==='straggler'&&begin===0&&!probe?125:match[1]==='latency'?1:8;
 timer=setTimeout(()=>{
  if(res.destroyed)return;
  res.writeHead(range?206:200,{'Content-Length':end-begin+1,'Content-Type':'application/octet-stream','ETag':`"${sha256}"`,...(range?{'Content-Range':`bytes ${begin}-${end}/${size}`}:{})});res.flushHeaders();
  let offset=begin;function pump(){if(res.destroyed)return;const stop=Math.min(end+1,offset+32768),chunk=body.subarray(offset,stop);offset=stop;record.sent+=chunk.length;const ready=res.write(chunk);if(offset>end){res.end();return;}if(ready)timer=setTimeout(pump,delay);else res.once('drain',()=>{timer=setTimeout(pump,delay);});}pump();
 },!probe&&match[1]==='latency'?180:0);
});
server.on('connection',socket=>{socket.setNoDelay(true);sockets.add(socket);socket.on('close',()=>sockets.delete(socket));});
const executableHash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
function run(exe,input,label){return new Promise((resolve,reject)=>{const child=spawn(exe,[input,label],{windowsHide:true});let log='';child.stdout.on('data',x=>log+=x);child.stderr.on('data',x=>log+=x);const timeout=setTimeout(()=>{child.kill();reject(Error('Benchmark timeout '+label));},90000);child.on('error',reject);child.on('exit',code=>{clearTimeout(timeout);fs.writeFileSync(path.join(output,label+'.log'),log);code?reject(Error(label+': '+log)):resolve();});});}
(async()=>{await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));try{
 for(const profile of ['latency','steady','straggler'])for(let pair=0;pair<3;pair++)for(const version of pair%2?['candidate','baseline']:['baseline','candidate']){
  const label=`${profile}-${pair}-${version}`,input=path.join(output,label+'-input.json');fs.writeFileSync(input,JSON.stringify({url:`http://127.0.0.1:${server.address().port}/${profile}/${label}`,sha256}));
  await run(version==='baseline'?baseline:candidate,input,label);
  const report=JSON.parse(fs.readFileSync(path.join(output,label+'-result.json')));assert.equal(report.sha256,sha256);assert.equal(report.bytes,size);
  const actual=executableHash(path.join(output,'downloads',label+'.bin'));assert.equal(actual,sha256);
  const reqs=records.filter(x=>x.label===label&&!x.probe);runs.push({...report,profile,version,dataRequests:reqs.length,serverSent:reqs.reduce((n,x)=>n+x.sent,0),independentFileHash:actual});
  console.log(`${label}: ${report.seconds.toFixed(3)}s, ${reqs.length} data requests, hash verified`);
  fs.writeFileSync(path.join(output,'results.json'),JSON.stringify({size,sha256,baselineSha256:executableHash(baseline),candidateSha256:executableHash(candidate),runs,requests:records},null,2));
 }
 }finally{server.close();for(const s of sockets)s.destroy();}})().catch(error=>{console.error(error);process.exitCode=1;});
