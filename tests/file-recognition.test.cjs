'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
let count=0;const pass=name=>{count++;console.log('PASS '+name)};
const empty='https://files.test/download',item=(filename,mime,url=empty)=>({url,filename,mime});
function test(family,name,run){run();pass(family+': '+name);}
async function harness(family,opts={}){
 const handlers={},calls=[],local={settings:{capture:true,cookies:false,extensions:opts.local||['pdf']},desktopPolicy:{}};
 const event=name=>({addListener:f=>(handlers[name]??=[]).push(f)}),emit=async(name,event)=>{for(const fn of handlers[name]||[])await fn(event)};
 const api={runtime:{onInstalled:event('installed'),onMessage:event('message'),sendNativeMessage:async(_,msg)=>{
  calls.push(msg);if(msg.action==='preferences')return {ok:true,captureAllowed:opts.allowed!==false,extensions:opts.desktop||['pdf'],postDownloads:true,postBodyLimit:1048576,forceSkipWeb:opts.skipWeb!==false};
  return {ok:!opts.reject,error:opts.reject?'fixture rejection':undefined};
 }},storage:{local:{get:async()=>local,set:async data=>Object.assign(local,data)},session:{get:async()=>({}),set:async()=>{},remove:async()=>{}}},
 permissions:{contains:async()=>false},action:{setBadgeText:async()=>{}},tabs:{onUpdated:event('updated'),onRemoved:event('removed')},
 contextMenus:{onClicked:event('context')},downloads:{onCreated:event('download'),pause:async()=>{calls.push({action:'pause'});await emit('error',{requestId:'one'});},resume:async()=>calls.push({action:'resume'}),cancel:async()=>calls.push({action:'cancel'}),search:async()=>[{id:1,url:opts.url||empty,state:'in_progress',paused:true,filename:opts.resolvedFilename??(opts.filename||(/filename\*=UTF-8''/.test(opts.disposition||'')?'résumé.pdf':'download.bin'))}]},
 webRequest:{onBeforeRequest:event('begin'),onBeforeSendHeaders:event('headers'),onHeadersReceived:event('response'),onErrorOccurred:event('error')}};
 const box=vm.createContext({chrome:api,URL,URLSearchParams,TextEncoder,ArrayBuffer,Uint8Array,btoa:s=>Buffer.from(s,'binary').toString('base64'),console,navigator:{userAgent:'fixture'},setTimeout,clearTimeout});
 for(const f of ['file-recognition.js','media.js','request-context.js','background.js'])vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser',family,f),'utf8'),box);
 if(opts.force)vm.runInContext("takeIntent=async()=> 'force'",box);
 if(opts.bypass)vm.runInContext("takeIntent=async()=> 'bypass'",box);
 const url=opts.url||empty,referrer='https://files.test/page';
 return {calls,local,run:async()=>{
  if(opts.excluded)local.settings.excluded=['files.test'];if(opts.off)local.settings.capture=false;
  if(!opts.unknown){
   await emit('begin',{requestId:'one',url,tabId:3,frameId:0,method:opts.method||'GET',documentUrl:referrer,requestBody:opts.method==='POST'?{raw:[{bytes:new TextEncoder().encode('name=test').buffer}]}:undefined});
   await emit('headers',{requestId:'one',url,requestHeaders:[{name:'Content-Type',value:opts.bodyType||'application/x-www-form-urlencoded'},{name:'Referer',value:referrer}]});
   await emit('response',{requestId:'one',url,statusCode:200,responseHeaders:[{name:'Content-Type',value:opts.serverMime||opts.mime||'application/pdf'},...(!opts.noDisposition?[{name:'Content-Disposition',value:opts.disposition||'attachment; filename="download.bin"'}]:[]),...(opts.extraHeaders||[])]});
  }
  await emit('download',{id:1,url,referrer,filename:opts.filename??'download.bin',mime:opts.mime??'application/pdf',state:'in_progress',incognito:!!opts.incognito});
 },actions:()=>calls.map(x=>x.action),add:()=>calls.find(x=>x.action==='add')};
}
(async()=>{
for(const family of ['chromium','firefox']){
 const R=require('../browser/'+family+'/file-recognition.js'),c=(file,type,url)=>R.classify(item(file,type,url));
 const cases=[
 ['extensionless PDF','download','application/pdf','pdf'],['generic PDF binary name','download.bin','application/pdf','pdf'],
 ['MIME parameters and case','download',' Application/PDF; charset=utf-8','pdf'],
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
 const dispositionCases=[
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
 test(family,'duplicate type fields mark metadata ambiguous',()=>assert(R.response([{name:'Content-Type',value:'application/pdf'},{name:'content-type',value:'application/zip'}]).ambiguous));
 test(family,'duplicate disposition fields mark metadata ambiguous',()=>assert(R.response([{name:'Content-Disposition',value:'attachment; filename=a.pdf'},{name:'content-disposition',value:'attachment; filename=b.pdf'}]).ambiguous));
 test(family,'file-type declarations do not rename a browser-selected filename',()=>assert.equal(c('C:\\Downloads\\my file.bin','application/pdf').filename,'my file.bin'));
 for(const [label,opts,expected]of [
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
  if(expected){assert(h.actions().indexOf('add')<h.actions().indexOf('cancel'));assert(!h.actions().includes('resume'));if(opts.method==='POST')assert.equal(h.add().request.method,'POST');}
  else{assert(!h.actions().includes('cancel'));if(h.actions().includes('pause'))assert(h.actions().includes('resume'));}
  pass(family+': actual capture policy '+label);
 }
 const failed=await harness(family,{reject:true});await failed.run();assert(failed.add());assert(failed.actions().includes('resume'));assert(!failed.actions().includes('cancel'));pass(family+': rejected inferred-type handoff resumes the browser');
}
const folder=path.resolve(__dirname,'../browser'),chrome=JSON.parse(fs.readFileSync(path.join(folder,'chromium/manifest.json'))),firefox=JSON.parse(fs.readFileSync(path.join(folder,'firefox/manifest.json')));
assert.equal(chrome.version,'0.53.1');assert.equal(firefox.version,chrome.version);assert(firefox.background.scripts.indexOf('file-recognition.js')<firefox.background.scripts.indexOf('request-context.js'));
assert.equal(fs.readFileSync(path.join(folder,'chromium/file-recognition.js'),'utf8'),fs.readFileSync(path.join(folder,'firefox/file-recognition.js'),'utf8'));pass('Both versioned browser bundles load the same classifier before request capture');
console.log('ALL '+count+' FILE RECOGNITION CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1});
