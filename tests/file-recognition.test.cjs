'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
let count=0;const pass=name=>{count++;console.log('PASS '+name)};
const empty='https://files.test/download',item=(filename,mime,url=empty)=>({url,filename,mime});
function test(family,name,run){run();pass(family+': '+name);}
async function harness(family,opts={}){
 const url=opts.url||empty,referrer='https://files.test/page';
 const handlers={},calls=[],local={settings:{capture:true,cookies:!!opts.cookies,...(!opts.defaultLocal?{extensions:opts.local||['pdf']}:{})},desktopPolicy:{captureRules:opts.rules}};
 const event=name=>({addListener:f=>(handlers[name]??=[]).push(f)}),emit=async(name,event)=>{for(const fn of handlers[name]||[])await fn(event)};
 const api={runtime:{id:'fixture',onInstalled:event('installed'),onMessage:event('message'),sendNativeMessage:async(_,msg)=>{
  calls.push(msg);if(msg.action==='preferences')return {ok:true,captureAllowed:opts.allowed!==false,extensions:opts.desktop||['pdf'],captureRules:opts.rules||[],postDownloads:true,postBodyLimit:opts.postLimit??1048576,browserProxy:opts.browserProxy,forceSkipWeb:opts.skipWeb!==false};
  return {ok:!opts.reject,error:opts.reject?'fixture rejection':undefined};
 }},storage:{local:{get:async()=>local,set:async data=>Object.assign(local,data)},session:{get:async()=>({}),set:async()=>{},remove:async()=>{}}},
 permissions:{contains:async()=>!!opts.cookiePermission},action:{setBadgeText:async()=>{}},tabs:{get:async()=>({url:referrer,incognito:false}),onUpdated:event('updated'),onRemoved:event('removed')},
 contextMenus:{onClicked:event('context')},downloads:{onCreated:event('download'),pause:async()=>{calls.push({action:'pause'});await emit('error',{requestId:'one'});},resume:async()=>calls.push({action:'resume'}),cancel:async()=>calls.push({action:'cancel'}),search:async()=>[{id:1,url:opts.url||empty,state:'in_progress',paused:true,filename:opts.resolvedFilename??(opts.filename||(/filename\*=UTF-8''/.test(opts.disposition||'')?'résumé.pdf':'download.bin'))}]},
 webRequest:{onBeforeRequest:event('begin'),onBeforeSendHeaders:event('headers'),onHeadersReceived:event('response'),onBeforeRedirect:event('redirect'),onErrorOccurred:event('error')}};
 const box=vm.createContext({chrome:api,URL,URLSearchParams,TextEncoder,ArrayBuffer,Uint8Array,btoa:s=>Buffer.from(s,'binary').toString('base64'),console,navigator:{userAgent:'fixture'},setTimeout,clearTimeout});
 for(const f of ['file-recognition.js','capture-rules.js','media.js','request-context.js','background.js'])vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser',family,f),'utf8'),box);
 if(opts.force)vm.runInContext("takeIntent=async()=> 'force'",box);
 if(opts.bypass)vm.runInContext("takeIntent=async()=> 'bypass'",box);
 return {calls,local,canCancel:(download,credentials)=>box.canCancelPreparedDownload(download,credentials),run:async()=>{
  if(opts.excluded)local.settings.excluded=['files.test'];if(opts.off)local.settings.capture=false;
  if(opts.redirectGesture){
   const start='https://files.test/redirect-start';
   const sender={id:'fixture',url:referrer,tab:{id:opts.gestureTab??3},frameId:opts.gestureFrame??0,documentId:opts.gestureDocument??'fixture-document'};
   const gesture=await new Promise(resolve=>handlers.message[0]({action:'capture-intent',url:start,intent:opts.redirectGesture},sender,resolve));
   assert.equal(gesture.ok,true);
   const first={requestId:'one',url:start,tabId:3,frameId:0,
    ...(opts.refererOnly?{initiator:new URL(referrer).origin}:{documentId:'fixture-document',documentUrl:referrer}),
    method:opts.redirectMethod||'GET'};
   await emit('begin',first);
   if(opts.refererOnly)await emit('headers',{...first,requestHeaders:[{name:'Referer',value:referrer}]});
   await emit('response',{...first,statusCode:302,responseHeaders:[]});
   const next=opts.multihop?'https://cdn.test/hop':url;
   if(!opts.noRedirectEvent)await emit('redirect',{...first,redirectUrl:opts.otherTarget||next,statusCode:302});
   if(opts.multihop){
    const hop={...first,url:next};
    await emit('begin',hop);
    await emit('response',{...hop,statusCode:307,responseHeaders:[]});
    await emit('redirect',{...hop,redirectUrl:url,statusCode:307});
   }
  }
  if(!opts.unknown){
   await emit('begin',{requestId:'one',url,tabId:3,frameId:0,documentId:'fixture-document',method:opts.method||'GET',documentUrl:referrer,requestBody:opts.method==='POST'?{raw:[{bytes:new TextEncoder().encode('name=test').buffer}]}:undefined});
   await emit('headers',{requestId:'one',url,requestHeaders:[{name:'Content-Type',value:opts.bodyType||'application/x-www-form-urlencoded'},{name:'Referer',value:referrer},...(opts.range?[{name:'rAnGe',value:opts.range}]:[]),...(opts.requestHeaders||[])]});
   await emit('response',{requestId:'one',url,tabId:3,frameId:0,type:opts.resourceType,method:opts.method||'GET',statusCode:opts.status||200,proxyInfo:opts.proxyInfo,responseHeaders:[{name:'Content-Type',value:opts.serverMime||opts.mime||'application/pdf'},...(!opts.noDisposition?[{name:'Content-Disposition',value:opts.disposition||'attachment; filename="download.bin"'}]:[]),...(opts.extraHeaders||[])]});
   if(opts.ambiguousRedirect){
    await emit('begin',{requestId:'direct',url,tabId:3,frameId:0,documentId:'fixture-document',documentUrl:referrer,method:'GET'});
    await emit('response',{requestId:'direct',url,tabId:3,frameId:0,method:'GET',statusCode:200,responseHeaders:[{name:'Content-Type',value:'application/pdf'},{name:'Content-Disposition',value:'attachment; filename="download.bin"'}]});
   }
  }
  if(!opts.backgroundOnly)await emit('download',{id:1,url,referrer,filename:opts.filename??'download.bin',mime:opts.mime??'application/pdf',state:'in_progress',incognito:!!opts.incognito});
  else await new Promise(resolve=>setImmediate(resolve));
 },actions:()=>calls.map(x=>x.action),add:()=>calls.find(x=>x.action==='add')};
}
(async()=>{
for(const family of ['chromium','firefox']){
 const R=require('../browser/'+family+'/file-recognition.js'),c=(file,type,url)=>R.classify(item(file,type,url));
 const cases=[
 ['extensionless PDF','download','application/pdf','pdf'],['generic PDF binary name','download.bin','application/pdf','pdf'],
 ['MIME parameters and case','download',' Application/PDF; charset=utf-8','pdf'],
 ['MIME tab parameter spacing','download.bin','application/pdf;\tcharset=utf-8','pdf'],
 ['MIME tab outer spacing','download.bin','\tapplication/pdf\t; charset=utf-8\t','pdf'],
 ['ZIP alias','download.bin','application/x-zip-compressed','zip'],['7-Zip','download','application/x-7z-compressed','7z'],
 ['RAR','download','application/vnd.rar','rar'],['ISO','download','application/x-iso9660-image','iso'],
 ['Word OOXML','download','application/vnd.openxmlformats-officedocument.wordprocessingml.document','docx'],
 ['Excel OOXML','download','application/vnd.openxmlformats-officedocument.spreadsheetml.sheet','xlsx'],
 ['MP3 alias','download','audio/mpeg','mp3'],['FLAC','download','audio/flac','flac'],['MP4','download','video/mp4','mp4'],
 ['JPEG alias','download','image/jpeg','jpeg'],['WebP','download','image/webp','webp'],
 ['PDF suffix remains with generic MIME','report.PDF','application/octet-stream','pdf'],
 ['unknown configured extension remains','report.udmfixture','application/octet-stream','udmfixture']
 ];
 for(const [label,file,type,ext] of cases)test(family,label,()=>assert(R.allowed(c(file,type),[ext],[ext])));
 test(family,'two allowlists must agree on an inferred extension',()=>assert(!R.allowed(c('download.bin','application/x-msdownload'),['exe'],['dll'])));
 test(family,'equivalent MIME aliases can agree on the same format',()=>assert(R.allowed(R.classify(item('download.bin','application/x-zip-compressed'),{mime:'application/zip'}),['zip'])));
 test(family,'registered and legacy FLAC types agree',()=>assert(R.allowed(R.classify(item('download','audio/flac'),{mime:'audio/x-flac'}),['flac'])));
 test(family,'punctuated custom suffixes retain existing capture behavior',()=>assert(R.allowed(c('source.c++','application/octet-stream'),['c++'])));
 test(family,'known explicit suffix cannot bypass configured type filters',()=>assert(!R.allowed(c('report.txt','application/pdf'),['pdf'])));
 test(family,'office ZIP container is not misclassified as ZIP',()=>assert(!R.allowed(c('report.docx','application/zip'),['zip'])));
 test(family,'generic binary MIME never implies an executable',()=>assert(!R.allowed(c('download.bin','application/octet-stream'),['exe','iso','zip'])));
 test(family,'a dotless filename equal to an extension is not a suffix',()=>assert(!R.allowed(c('pdf','application/octet-stream'),['pdf'])));
 test(family,'HTML error body does not impersonate a named PDF',()=>assert(!R.allowed(c('report.pdf','text/html'),['pdf'])));
 test(family,'MIME metadata never overrides web error evidence',()=>assert(!R.allowed(R.classify(item('report.pdf','application/pdf'),{mime:'application/problem+json'}),['pdf'])));
 test(family,'disagreeing specific MIME declarations do not infer a generic name',()=>assert(!R.allowed(R.classify(item('download.bin','application/pdf'),{mime:'application/zip'}),['pdf'])));
 test(family,'empty browser MIME uses matching response type',()=>assert(R.allowed(R.classify(item('download',''),{mime:'application/pdf'}),['pdf'])));
 test(family,'script URL filenames can use the actual response type',()=>assert(R.allowed(R.classify(item('export.php','application/pdf','https://files.test/export.php')),['pdf'])));
 test(family,'explicit PHP attachment stays a PHP file',()=>assert(!R.allowed(R.classify(item('export.php','application/pdf','https://files.test/export.php'),{response:{filename:'export.php'}}),['pdf'])));
 test(family,'ambiguous response metadata cannot infer a type',()=>assert(!R.allowed(R.classify(item('download.bin','application/pdf'),{response:{ambiguous:true,mime:'application/pdf'}}),['pdf'])));
 test(family,'URL query names cannot cause type capture',()=>assert(!R.allowed(c('','application/octet-stream','https://files.test/get?filename=secret.pdf'),['pdf'])));
 test(family,'encoded URL suffix recognized without query dependence',()=>assert(R.allowed(c('','application/octet-stream','https://files.test/r%C3%A9sum%C3%A9%2Epdf'),['pdf'])));
 test(family,'configured dot and case normalize consistently',()=>assert(R.allowed(c('report.PDF','application/pdf'),['.PDF'],['pdf'])));
 for(const value of ['application/\tpdf','application/pdf\r\nInjected: yes','application/pdf;\x00name=x','application/pdf;\vname=x'])test(family,'Invalid MIME control is rejected '+JSON.stringify(value),()=>assert.equal(R.mime(value),''));
 const dispositionCases=[
 ['attachment;\tfilename\t=\t"report.pdf"\t','report.pdf'],
 ["attachment; filename*\t=\tUTF-8''report.pdf",'report.pdf'],
 ['attachment; filename=report.pdf\t; size=12','report.pdf'],
 ['attachment; filename="a;b.pdf"','a;b.pdf'],['attachment; filename="../report.pdf"','report.pdf'],
 ["attachment; filename=fallback.txt; filename*=UTF-8'en'r%C3%A9sum%C3%A9.pdf",'résumé.pdf'],
 ["attachment; filename*=ISO-8859-1''caf%E9.pdf",'café.pdf'],
 ["attachment; filename=fallback.pdf; filename*=UTF-8''broken%FF",'fallback.pdf'],
 ['attachment; FILENAME="report.pdf"','report.pdf']
 ];
 for(const [header,name]of dispositionCases)test(family,'Content-Disposition '+header,()=>assert.equal(R.disposition(header).filename,name));
 for(const header of ['attachment; filename=one.pdf; filename=two.pdf','attachment; filename="unfinished.pdf','attachment; filename="report.pdf"\r\nInjected: x','attachment; filename="a.pdf"extra']){
  test(family,'malformed disposition is not a capture filename',()=>assert(R.disposition(header).invalid));
 }
 for(const [label,evidence]of [
  ['explicit Range ignored by the server',{range:'bytes=1024-'}],
  ['partial HTTP status',{status:206}],
  ['partial content header',{extraHeaders:[{name:'Content-Range',value:'bytes 0-1023/2048'}]}]
 ]){
  const browser=await harness(family,{...evidence,force:true});
  await browser.run();assert.deepEqual(browser.actions(),[]);
  const rule=await harness(family,{...evidence,rules:['S!'+empty],resourceType:'xmlhttprequest',backgroundOnly:true});
  await rule.run();assert.deepEqual(rule.actions(),[]);
  pass(family+': '+label+' leaves the browser response running without native admission');
 }
 const full=await harness(family,{rules:['S!'+empty],resourceType:'xmlhttprequest',backgroundOnly:true});
 await full.run();assert(full.add());
 pass(family+': a full response can still match a background capture rule');
 const cookie={name:'Cookie',value:'fixture=1'};
 const cookieVary={requestHeaders:[cookie],extraHeaders:[{name:'Vary',value:'Accept-Encoding, cOoKiE'}]};
 for(const [label,evidence]of [
  ['Authorization',{requestHeaders:[{name:'Authorization',value:'Bearer fixture'}]}],
  ['a cookie-varying response',cookieVary]
 ]){
  const denied=await harness(family,{...evidence,force:true,cookies:true});
  await denied.run();assert.deepEqual(denied.actions(),[]);
  const rule=await harness(family,{...evidence,rules:['S!'+empty],resourceType:'xmlhttprequest',backgroundOnly:true});
  await rule.run();assert(!rule.add());
  const allowed=await harness(family,{...evidence,cookies:true,cookiePermission:true});
  await allowed.run();assert.equal(allowed.add()?.headers[evidence.requestHeaders[0].name],evidence.requestHeaders[0].value);
  pass(family+': '+label+' needs credential permission before automatic takeover');
 }
 const incidental=await harness(family,{requestHeaders:[cookie]});
 await incidental.run();assert(incidental.add());assert.equal(incidental.add().headers.Cookie,undefined);
 pass(family+': incidental cookies do not prevent capture of an otherwise eligible response');
 const policy=await harness(family);
 const redirect={url:'https://redirect.test/start',finalUrl:empty};
 assert(await policy.canCancel(redirect,false));
 policy.local.settings.excluded=['files.test'];assert(!await policy.canCancel(redirect,false));
 policy.local.settings.excluded=[];
 policy.local.desktopPolicy.excludedUrls=['='+empty];assert(!await policy.canCancel(redirect,false));
 policy.local.desktopPolicy.excludedUrls=[];
 policy.local.desktopPolicy.captureAllowed=false;assert(!await policy.canCancel(redirect,false));
 policy.local.desktopPolicy.captureAllowed=true;
 policy.local.settings.capture=false;assert(!await policy.canCancel(redirect,false));
 pass(family+': final cancellation applies current automatic capture and exclusion settings');
 const proxied=await harness(family,{proxyInfo:{type:'direct'},browserProxy:1});
 await proxied.run();assert(proxied.add());
 assert.deepEqual(proxied.actions(),['preferences','pause','add','cancel']);
 pass(family+': an automatic browser route reuses the desktop policy request');
 for(const [label,options] of [
  ['desktop file type rejection',{desktop:['zip']}],
  ['unsupported desktop proxy route',{proxyInfo:{type:'direct'}}],
  ['desktop POST body limit',{method:'POST',postLimit:0}]
 ]){
  const retained=await harness(family,options);await retained.run();
  assert.deepEqual(retained.actions(),['preferences']);
  pass(family+': '+label+' is checked without pausing the original response');
 }
 test(family,'duplicate type fields mark metadata ambiguous',()=>assert(R.response([{name:'Content-Type',value:'application/pdf'},{name:'content-type',value:'application/zip'}]).ambiguous));
 test(family,'duplicate disposition fields mark metadata ambiguous',()=>assert(R.response([{name:'Content-Disposition',value:'attachment; filename=a.pdf'},{name:'content-disposition',value:'attachment; filename=b.pdf'}]).ambiguous));
 test(family,'file-type declarations do not rename a browser-selected filename',()=>assert.equal(c('C:\\Downloads\\my file.bin','application/pdf').filename,'my file.bin'));
 for(const [label,opts,expected]of [
  ['tab-spaced filename preserves generic PDF recognition',{disposition:'attachment;\tfilename\t=\t"download.bin"\t'},true],
  ['tab-spaced server Content-Type handoff',{mime:'',serverMime:'application/pdf;\tcharset=utf-8'},true],
  ['generic PDF handoff',{},true],['extensionless PDF handoff',{filename:'download'},true],
  ['matching server type when browser MIME absent',{mime:'',serverMime:'application/pdf'},true],
  ['server attachment filename when browser name is absent',{filename:'',mime:'application/octet-stream',disposition:"attachment; filename*=UTF-8''r%C3%A9sum%C3%A9.pdf"},true],
  ['late explicit filename overrides PDF URL',{filename:'',resolvedFilename:'keep.txt',url:'https://files.test/file.pdf',noDisposition:true},false],
  ['late generic filename captures through MIME',{filename:'',resolvedFilename:'archive.bin',url:'https://files.test/file.zip',mime:'application/zip',local:['zip'],desktop:['zip'],noDisposition:true},true],
  ['generic octet stream',{mime:'application/octet-stream'},false],
  ['explicit unselected suffix',{filename:'report.txt'},false],
  ['HTML error disguised as PDF',{filename:'report.pdf',mime:'text/html'},false],
  ['conflicting browser and response MIME',{serverMime:'application/zip'},false],
  ['duplicate response MIME',{extraHeaders:[{name:'Content-Type',value:'application/pdf'}]},false],
  ['desktop filter disagreement',{desktop:['zip']},false],
  ['ambiguous MIME aliases across filters',{mime:'application/x-msdownload',local:['exe'],desktop:['dll']},false],
  ['unknown request context',{unknown:true},false],
  ['disabled capture',{off:true},false],['site exclusion',{excluded:true},false],['private download',{incognito:true},false],
  ['bypass modifier',{bypass:true},false],['desktop capture disabled',{allowed:false},false],
  ['explicit force with unmatched filters',{force:true,mime:'application/octet-stream',desktop:['zip']},true],
  ['force does not capture web error',{force:true,filename:'error.pdf',mime:'',serverMime:'text/html'},false],
  ['matched POST remains POST',{method:'POST'},true],
  ['unsupported form body is not replayed',{method:'POST',bodyType:'multipart/form-data; boundary=missing'},false]
 ]){
  const h=await harness(family,opts);await h.run();assert.equal(!!h.add(),expected,label);
  if(expected){assert(h.actions().indexOf('add')<h.actions().indexOf('cancel'));assert(!h.actions().includes('resume'));if(opts.method==='POST'){assert.equal(h.add().request.method,'POST');assert.equal(h.actions().filter(action=>action==='preferences').length,1);}}
  else{assert(!h.actions().includes('cancel'));if(h.actions().includes('pause'))assert(h.actions().includes('resume'));}
  pass(family+': actual capture policy '+label);
 }
 for(const [name,opts,captured] of [
  ['force follows a verified redirect to an unmatched file type',{redirectGesture:'force',mime:'application/octet-stream'},true],
  ['bypass follows a verified redirect when the download exposes only the final URL',{redirectGesture:'bypass'},false],
  ['bypass follows multiple verified redirect hops',{redirectGesture:'bypass',multihop:true},false],
  ['bypass uses the initial referrer when the navigation has no document ID',{redirectGesture:'bypass',refererOnly:true},false],
  ['the same request ID alone does not transfer bypass',{redirectGesture:'bypass',noRedirectEvent:true},true],
  ['a redirect observed for a different target does not transfer bypass',{redirectGesture:'bypass',otherTarget:'https://cdn.test/unrelated'},true],
  ['a POST redirect does not inherit a link gesture',{redirectGesture:'bypass',redirectMethod:'POST'},true],
  ['redirect intent does not cross tabs',{redirectGesture:'bypass',gestureTab:4},true],
  ['redirect intent does not cross frames',{redirectGesture:'bypass',gestureFrame:1},true],
  ['redirect intent does not cross documents',{redirectGesture:'bypass',gestureDocument:'older-document'},true],
  ['an ambiguous redirect chain does not consume bypass',{redirectGesture:'bypass',ambiguousRedirect:true},true]
 ]){
  const h=await harness(family,{...opts,url:'https://cdn.test/final-file'});await h.run();
  assert.equal(!!h.add(),captured,name);
  if(captured)assert.equal(h.add().url,'https://cdn.test/final-file');
  else assert.deepEqual(h.actions(),[]);
  pass(family+': '+name);
 }
 for(const [extension,local,desktop,captured] of [
  ['gguf',undefined,['gguf'],true],['parquet',undefined,['parquet'],true],
  ['docx',undefined,['docx'],true],['bin',undefined,['bin'],false],
  ['gguf',['pdf'],['gguf'],false],['gguf',[],['gguf'],false],['gguf',undefined,['pdf'],false]
 ]){
  const filename='attachment.'+extension,mime='application/octet-stream';
  const h=await harness(family,{defaultLocal:local===undefined,local,desktop,filename,mime,disposition:'attachment; filename="'+filename+'"'});
  await h.run();
  assert.equal(!!h.add(),captured);
  if(!captured)assert(!h.actions().includes('pause'));
  pass(family+': capture '+extension+' with '+(local===undefined?'default browser types':JSON.stringify(local))+' and '+desktop);
 }
 const failed=await harness(family,{reject:true});await failed.run();assert(failed.add());assert(failed.actions().includes('resume'));assert(!failed.actions().includes('cancel'));pass(family+': rejected inferred-type handoff resumes the browser');
}
const folder=path.resolve(__dirname,'../browser'),chrome=JSON.parse(fs.readFileSync(path.join(folder,'chromium/manifest.json'))),firefox=JSON.parse(fs.readFileSync(path.join(folder,'firefox/manifest.json')));
if(process.env.EXPECT_BROWSER_VERSION)assert.equal(chrome.version,process.env.EXPECT_BROWSER_VERSION);else assert.match(chrome.version,/^\d+\.\d+\.\d+$/);assert.equal(firefox.version,chrome.version);assert(firefox.background.scripts.indexOf('file-recognition.js')<firefox.background.scripts.indexOf('request-context.js'));
assert.equal(fs.readFileSync(path.join(folder,'chromium/file-recognition.js'),'utf8'),fs.readFileSync(path.join(folder,'firefox/file-recognition.js'),'utf8'));pass('Both versioned browser bundles load the same classifier before request capture');
console.log('ALL '+count+' FILE RECOGNITION CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1});
