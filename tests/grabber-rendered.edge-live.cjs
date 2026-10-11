'use strict';
const {chromium}=require('playwright'),http=require('node:http'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const {collect}=require('../browser/chromium/grabber-rendered.js'),root=path.resolve(process.argv[2]),checks=[];let browser,server;
const check=(value,name)=>{assert(value,name);checks.push({name,passed:true});console.log('PASS '+name);};
(async()=>{
 assert(!fs.existsSync(root));fs.mkdirSync(root,{recursive:true});
 server=http.createServer((q,r)=>{r.setHeader('Content-Type','text/html');r.end('<!doctype html><title>Rendered fixture</title><base href="/assets/"><main></main>');});await new Promise(r=>server.listen(0,'127.0.0.1',r));const origin='http://127.0.0.1:'+server.address().port;
 browser=await chromium.launch({channel:'msedge',headless:true});const page=await browser.newPage();await page.goto(origin+'/page');
 await page.evaluate(()=>{
  const a=document.createElement('a');a.href=['dynamic','.zip'].join('');a.textContent='Runtime file';document.querySelector('main').append(a,a.cloneNode(true));
  for(const href of ['javascript:alert(1)','data:text/plain,private','https://user:password@example.test/file','#section']){const link=document.createElement('a');link.href=href;document.body.append(link);}
  const host=document.createElement('div');document.body.append(host);host.attachShadow({mode:'open'}).innerHTML='<a href="shadow.zip">Shadow link</a><img src="picture.png" alt="Image">';
  const frame=document.createElement('iframe');frame.src='/child';document.body.append(frame);
  const input=document.createElement('input');input.value='private-form-value';document.body.append(input);
 });
 const result=await page.evaluate(collect);const urls=result.references.map(x=>x.url);
 check(urls.includes(origin+'/assets/dynamic.zip'),'Discovers JavaScript-created link resolved against document base');
 check(urls.filter(x=>x.endsWith('/dynamic.zip')).length===1,'Deduplicates repeated live links');
 check(urls.includes(origin+'/assets/shadow.zip'),'Discovers links in open shadow roots');
 check(urls.includes(origin+'/assets/picture.png'),'Discovers rendered image resources');
 check(result.references.some(x=>x.kind==='frame'&&x.url===origin+'/child'),'Records frame navigation for later scoped exploration');
 check(!urls.some(x=>x.includes('password')||x.startsWith('data:')||x.startsWith('javascript:')||x.includes('#')),'Excludes credentialed, non-network and fragment-only links');
 check(!JSON.stringify(result).includes('private-form-value'),'Does not collect form values');
 check(!result.truncated&&result.url===origin+'/page','Retains page identity and complete-result status');
 await page.evaluate(()=>{for(let i=0;i<2200;i++){const a=document.createElement('a');a.href='/file-'+i;document.body.append(a);}});
 const bounded=await page.evaluate(collect);check(bounded.truncated&&bounded.references.length===2000,'Marks reference limit explicitly without exceeding it');
 fs.writeFileSync(path.join(root,'snapshot.json'),JSON.stringify(result,null,2));
})().catch(e=>{checks.push({passed:false,error:e.stack});console.error(e);process.exitCode=1;}).finally(async()=>{await browser?.close();if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({checks},null,2));});
