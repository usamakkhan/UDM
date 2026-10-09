'use strict';
// Acceptance for retaining the initial HTTP response. A failed case is an open product gap.
const fs=require('node:fs'),path=require('node:path'),http=require('node:http'),assert=require('node:assert/strict'),crypto=require('node:crypto'),{spawn}=require('node:child_process');
const root=path.resolve(process.argv[2]),exe=process.env.UDM_TEST_EXE;
assert(exe,'Set UDM_TEST_EXE');assert(!fs.existsSync(root),'Use a fresh output directory');fs.mkdirSync(root,{recursive:true});
const payload=Buffer.alloc(131089);for(let i=0;i<payload.length;i++)payload[i]=(i*37+17)%251;
const sockets=new Set(),requests=[],results=[];
const server=http.createServer((req,res)=>{
 const previous=requests.filter(x=>x.path===req.url).length;
 requests.push({path:req.url,method:req.method,range:req.headers.range||''});
 if(req.url.includes('once')&&previous){res.writeHead(410,{'Content-Length':0});return res.end();}
 const m=!req.url.includes('ignore-range')&&/^bytes=(\d+)-(\d*)$/.exec(req.headers.range||'');
 const start=m?Number(m[1]):0,end=m&&m[2]?Math.min(Number(m[2]),payload.length-1):payload.length-1;
 res.writeHead(m?206:200,{'Content-Length':end-start+1,'Content-Type':'application/octet-stream','Accept-Ranges':'bytes',ETag:'"initial-response"',...(m?{'Content-Range':`bytes ${start}-${end}/${payload.length}`}:{})});res.end(payload.subarray(start,end+1));
});
server.on('connection',s=>{sockets.add(s);s.on('error',()=>{});s.on('close',()=>sockets.delete(s));});
async function run(name,connections){
 const folder=path.join(root,name);fs.mkdirSync(folder);const input=path.join(folder,'input.json');fs.writeFileSync(input,JSON.stringify({url:`http://127.0.0.1:${server.address().port}/${name}`,connections}));
 await new Promise((resolve,reject)=>{const child=spawn(exe,['--feature-spec',input],{windowsHide:true});let log='';child.stdout.on('data',b=>log+=b);child.stderr.on('data',b=>log+=b);const timer=setTimeout(()=>{child.kill();reject(Error('Native fixture timeout'));},45000);child.on('error',e=>{clearTimeout(timer);reject(e);});child.on('close',code=>{clearTimeout(timer);fs.writeFileSync(path.join(folder,'native.log'),log);code?reject(Error(log)):resolve();});});
 return JSON.parse(fs.readFileSync(path.join(folder,'result.json'),'utf8').replace(/^\uFEFF/,''));
}
(async()=>{
 await new Promise(r=>server.listen(0,'127.0.0.1',r));
 for(const [name,connections] of [['ordinary-ranges',8],['once-ignore-range',8],['once-range-aware-single',1],['once-range-aware-parallel',8]]){
  let result;try{result=await run(name,connections);assert.equal(result.status,'Complete',result.error);assert.deepEqual(fs.readFileSync(result.path),payload);if(name.startsWith('once'))assert.equal(requests.filter(r=>r.path==='/'+name).length,1);results.push({name,passed:true,result});console.log('PASS '+name);}
  catch(e){results.push({name,passed:false,error:e.message,result});console.error('FAIL '+name+': '+e.message);}
 }
})().catch(e=>{results.push({name:'harness',passed:false,error:e.stack});}).finally(()=>{
 for(const s of sockets)s.destroy();server.close();const report={passed:results.filter(r=>r.passed).length,failed:results.filter(r=>!r.passed).length,payloadSha256:crypto.createHash('sha256').update(payload).digest('hex'),results,requests};fs.writeFileSync(path.join(root,'results.json'),JSON.stringify(report,null,2));console.log(JSON.stringify({passed:report.passed,failed:report.failed}));if(report.failed)process.exitCode=1;
});
