'use strict';
// Prepares an actual native Grabber download, then serves only its saved files
// for manual/CUA browser acceptance after the original origin has shut down.
const fs=require('fs'),path=require('path'),http=require('http'),crypto=require('crypto');
const {spawn}=require('child_process');
const exe=path.resolve(process.argv[2]),output=path.resolve(process.argv[3]);
if(fs.existsSync(output))throw Error('Use a new output directory.');
fs.mkdirSync(output,{recursive:true});
const routes=new Map([
 ['/docs/index.html',['text/html',`<!doctype html><html><head><base href='/docs/'><title>UDM saved module acceptance</title></head><body><h1>Saved module execution</h1><p id='external'>External module: pending</p><p id='inline'>Inline module: pending</p><p id='lazy'>Dynamic module: pending</p><p id='classic'>Classic dynamic: pending</p><p id='inlineLazy'>Inline dynamic: pending</p><p id='interpolation'>Template import: pending</p><p id='literalTemplate'>Literal template: pending</p><p id='postfix'>Postfix division: pending</p><p id='moduleAsset'>Module asset: pending</p><p id='errors'>Runtime errors: 0</p><script>for(const event of ['error','unhandledrejection'])window.addEventListener(event,()=>{document.getElementById('errors').textContent='Runtime errors: detected';});</script><script src='/classic.js'></script><script>import('./inline-lazy.js').then(m=>document.getElementById('inlineLazy').textContent='Inline dynamic: '+m.value);</script><script type='module' src='/redirect.js'></script><script type='module'>import {leaf} from './inline.js'; document.getElementById('inline').textContent='Inline module: '+leaf.leaf;</script></body></html>`]],
 ['/modules/main.js',['text/javascript',`import {value} from './nested/dep.js'; export * from '../shared.js'; document.getElementById('external').textContent='External module: '+value; import('./lazy.js').then(m=>document.getElementById('lazy').textContent='Dynamic module: '+m.value);`]],
 ['/modules/nested/dep.js',['text/javascript',`import '../main.js'; export const value=42;`]],
 ['/modules/asset.txt',['text/plain','module-relative-asset']],
 ['/modules/postfix.js',['text/javascript',"export const value=88;"]],
 ['/modules/literal.js',['text/javascript',`export const value=67;`]],
 ['/modules/inside.js',['text/javascript',`export const value=56;`]],
 ['/modules/lazy.js',['text/javascript',`export const value=91;`]],
 ['/classic.js',['text/javascript',`import('./classic-lazy.js').then(m=>document.getElementById('classic').textContent='Classic dynamic: '+m.value);`]],
 ['/classic-lazy.js',['text/javascript',`export const value=84;`]],
 ['/docs/inline-lazy.js',['text/javascript',`export const value=65;`]],
 ['/shared.js',['text/javascript',`export const shared=2;`]],
 ['/docs/inline.js',['text/javascript',`export * as leaf from '../leaf.js';`]],
 ['/leaf.js',['text/javascript',`export const leaf=73;`]],
]);
const templateEntry=routes.get('/modules/main.js');
// Exercise dense scripts and ordinary escaped literals in the real browser.
templateEntry[1]='void 0;'.repeat(75000)+String.raw`void 'ordinary \u0061 text';`.repeat(16000)+templateEntry[1];
templateEntry[1]="const template = `import './template-fake.js' ${(()=>{const marker='}'; return `nested ${1}`;})()}`;\n"+templateEntry[1];
templateEntry[1]+="\nconst inside = `${import('./inside.js').then(m=>document.getElementById('interpolation').textContent='Template import: '+m.value)}`;";
templateEntry[1]+="\nimport(`./literal.js`).then(m=>document.getElementById('literalTemplate').textContent='Literal template: '+m.value);";
templateEntry[1]+="\nlet counter=4; counter++ / 2; import('./postfix.js').then(m=>document.getElementById('postfix').textContent='Postfix division: '+m.value);";
templateEntry[1]+="\nfetch(new URL('asset.txt', import.meta.url)).then(r=>{if(!r.ok)throw Error('Asset fetch failed');return r.text();}).then(value=>document.getElementById('moduleAsset').textContent='Module asset: '+value);";
const originRequests=[],savedRequests=[];
const originServer=http.createServer((req,res)=>{
 const url=new URL(req.url,'http://localhost').pathname;originRequests.push({path:url,method:req.method});
 if(url==='/redirect.js'){res.writeHead(302,{Location:'/modules/main.js','Content-Length':0});res.end();return;}
 const entry=routes.get(url);if(!entry){res.writeHead(404,{'Content-Length':0});res.end();return;}
 const body=Buffer.from(entry[1]);res.writeHead(200,{'Content-Type':entry[0],'Content-Length':body.length,Connection:'close'});res.end(req.method==='HEAD'?undefined:body);
});
let savedServer;
(async()=>{
 await new Promise(resolve=>originServer.listen(0,'127.0.0.1',resolve));
 const origin=`http://127.0.0.1:${originServer.address().port}`;
 for(const file of ['modules/nested/dep.js','docs/inline.js','modules/inside.js','modules/literal.js','modules/postfix.js','modules/asset.txt']){const target=path.join(output,'downloads',file);fs.mkdirSync(path.dirname(target),{recursive:true});fs.writeFileSync(target,'Existing user file');}
 const input=path.join(output,'input.json');fs.writeFileSync(input,JSON.stringify({url:origin+'/docs/index.html'}));
 await new Promise((resolve,reject)=>{const child=spawn(exe,['--grabber-link-spec',input],{windowsHide:true});const log=fs.createWriteStream(path.join(output,'native.log'));child.stdout.pipe(log);child.stderr.pipe(log);child.once('error',reject);child.once('exit',code=>code===0?resolve():reject(Error(`Native exit ${code}`)));});
 await new Promise(resolve=>originServer.close(resolve));
 const result=JSON.parse(fs.readFileSync(path.join(output,'result.json'),'utf8'));
 if(result.project.LinkConversionState!=='Complete'||result.files.some(f=>f.status!=='Complete'))throw Error('Native publication incomplete');
 const saved=new Map();for(const file of result.files){const key='/'+path.relative(path.join(output,'downloads'),file.path).split(path.sep).join('/');saved.set(key,file);}
 const evidence={origin,originClosed:true,originRequests,savedRequests,nativeSha256:crypto.createHash('sha256').update(fs.readFileSync(exe)).digest('hex')};
 const save=()=>fs.writeFileSync(path.join(output,'browser-server.json'),JSON.stringify(evidence,null,2));
 savedServer=http.createServer((req,res)=>{
  if(req.method==='POST'&&req.url==='/__finish'){evidence.finished=true;save();res.end('Closed');savedServer.close();return;}
  let key;try{key=decodeURIComponent(new URL(req.url,'http://localhost').pathname);}catch{res.writeHead(400);res.end();return;}
  const file=saved.get(key);savedRequests.push({path:key,status:file?200:404});save();
  if(!file){res.writeHead(404,{'Content-Length':0});res.end();return;}
  const bytes=fs.readFileSync(file.path),type=key.endsWith('.html')?'text/html':key.endsWith('.js')?'text/javascript':'application/octet-stream';
  res.writeHead(200,{'Content-Type':type,'Content-Length':bytes.length,'Cache-Control':'no-store'});res.end(bytes);
 });
 await new Promise(resolve=>savedServer.listen(0,'127.0.0.1',resolve));
 const page=result.files.find(f=>f.url===origin+'/docs/index.html');
 evidence.savedOrigin=`http://127.0.0.1:${savedServer.address().port}`;
 evidence.browserUrl=evidence.savedOrigin+'/'+path.relative(path.join(output,'downloads'),page.path).split(path.sep).map(encodeURIComponent).join('/');save();console.log(evidence.browserUrl);
 const timer=setTimeout(()=>savedServer.close(),10*60*1000);timer.unref();
})().catch(error=>{originServer.close();savedServer?.close();console.error(error.message);process.exitCode=1;});
