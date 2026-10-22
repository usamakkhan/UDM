'use strict';
// Local HTTP and isolated native windows only; never opens the user's catalog.
const http=require('node:http'),fs=require('node:fs'),path=require('node:path'),{spawn}=require('node:child_process');
const [exe,output,crossFolder]=process.argv.slice(2);
if(!exe||!output||fs.existsSync(output))throw Error('Pass the fixture executable and a fresh output directory');
const requests=[],gets=new Map();
const server=http.createServer((req,res)=>{
 requests.push({method:req.method,path:req.url});
 if((req.url.startsWith('/start-')||req.url.startsWith('/later-'))&&req.method==='GET'){const count=(gets.get(req.url)||0)+1;gets.set(req.url,count);if(count>1){res.writeHead(410,{'Content-Length':'0'});res.end();return;}}
 setTimeout(()=>{res.writeHead(200,{'Content-Length':req.url.includes('stream')?'262144':'4097','Content-Type':'application/zip',...(req.url==='/missing'?{}:{'Content-Disposition':'attachment; filename=server.zip'})});if(req.method==='GET'){if(req.url.includes('stream')){res.write(Buffer.alloc(131072,1));setTimeout(()=>res.end(Buffer.alloc(131072,1)),3000);}else res.end(Buffer.alloc(4097,1));}else res.end();},400);
});
server.listen(0,'127.0.0.1',()=>{
 const child=spawn(path.resolve(exe),[path.resolve(output),`http://127.0.0.1:${server.address().port}`,...(crossFolder?[path.resolve(crossFolder)]:[])],{stdio:'inherit'});
 const timer=setTimeout(()=>{child.kill();},90000);
 child.on('error',error=>{console.error(error);clearTimeout(timer);server.close();process.exitCode=1;});
 child.on('exit',code=>{
  clearTimeout(timer);server.close();
  const file=path.join(output,'results.json');
  const result=fs.existsSync(file)?JSON.parse(fs.readFileSync(file,'utf8').replace(/^\uFEFF/,'')):{passed:false,error:'No native result'};
  result.httpRequests=requests;result.headOnly=requests.filter(r=>r.path!=='/prefetch'&&!r.path.startsWith('/start-')&&!r.path.startsWith('/later-')).length>=6&&requests.filter(r=>r.path!=='/prefetch'&&!r.path.startsWith('/start-')&&!r.path.startsWith('/later-')).every(r=>r.method==='HEAD');result.prefetchUsedGet=requests.some(r=>r.path==='/prefetch'&&r.method==='GET');result.singleUseStarts=['/later-complete','/later-damaged','/later-missing','/later-checksum','/start-stream','/start-complete','/start-move-stream',...(crossFolder?['/start-cross-stream']:[])].every(url=>requests.filter(r=>r.path===url).length===1&&gets.get(url)===1);
  fs.writeFileSync(file,JSON.stringify(result,null,2)+'\n');
  console.log(JSON.stringify({exitCode:code,passed:result.passed,checks:result.checks?.length,headOnly:result.headOnly,singleUseStarts:result.singleUseStarts,error:result.error}));
  process.exitCode=code===0&&result.passed&&result.headOnly&&result.prefetchUsedGet&&result.singleUseStarts?0:1;
 });
});
