'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
let count=0;const pass=name=>{count++;console.log('PASS '+name)};
function test(family,name,run){run();pass(family+': '+name);}
const empty='https://files.test/go';
// Minimal worker harness: loads the real extension scripts and feeds one download.
async function harness(family,opts={}){
 const handlers={},calls=[],session=new Map(),local={settings:{capture:true,cookies:false,extensions:opts.local||['pdf']},desktopPolicy:{captureRules:opts.rules||[]}};
 const event=name=>({addListener:f=>(handlers[name]??=[]).push(f)}),emit=async(name,event)=>{for(const fn of handlers[name]||[])await fn(event)};
 const api={runtime:{onInstalled:event('installed'),onMessage:event('message'),sendNativeMessage:async(_,msg)=>{
  calls.push(msg);if(msg.action==='preferences')return {ok:true,captureAllowed:true,extensions:opts.desktop||['pdf'],postDownloads:true,postBodyLimit:1048576,forceSkipWeb:true};
  return {ok:true};
 }},storage:{local:{get:async()=>local,set:async data=>Object.assign(local,data)},session:{get:async k=>{const out={};for(const key of Array.isArray(k)?k:[k])if(session.has(key))out[key]=session.get(key);return out;},set:async data=>{for(const [k,v] of Object.entries(data))session.set(k,v);},remove:async k=>{for(const key of Array.isArray(k)?k:[k])session.delete(key);}}},
 permissions:{contains:async()=>false},action:{setBadgeText:async()=>{}},tabs:{onUpdated:event('updated'),onRemoved:event('removed'),get:async()=>({incognito:false})},
 contextMenus:{onClicked:event('context')},downloads:{onCreated:event('download'),pause:async()=>{calls.push({action:'pause'});await emit('error',{requestId:'one'});},resume:async()=>calls.push({action:'resume'}),cancel:async()=>calls.push({action:'cancel'}),search:async()=>[{id:1,url:opts.url||empty,state:'in_progress',paused:true,filename:opts.filename||'download.bin'}]},
 webRequest:{onBeforeRequest:event('begin'),onBeforeSendHeaders:event('headers'),onHeadersReceived:event('response'),onErrorOccurred:event('error')}};
 const box=vm.createContext({chrome:api,URL,URLSearchParams,TextEncoder,ArrayBuffer,Uint8Array,btoa:s=>Buffer.from(s,'binary').toString('base64'),console,navigator:{userAgent:'fixture'},setTimeout,clearTimeout});
 for(const f of ['file-recognition.js','capture-rules.js','media.js','request-context.js','background.js'])vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser',family,f),'utf8'),box);
 const url=opts.url||empty,referrer='https://files.test/page';
 if(opts.skipDownload)return {calls,emit,handlers,session,settle:()=>new Promise(r=>setTimeout(r,120))};
 await emit('begin',{requestId:'one',url,tabId:3,frameId:0,method:opts.method||'GET',documentUrl:referrer,requestBody:opts.method==='POST'?{raw:[{bytes:new TextEncoder().encode('name=test').buffer}]}:undefined});
 await emit('headers',{requestId:'one',url,requestHeaders:[{name:'Referer',value:referrer}]});
 await emit('response',{requestId:'one',url,statusCode:200,responseHeaders:[{name:'Content-Type',value:opts.mime||'application/octet-stream'}]});
 await emit('download',{id:1,url,referrer,filename:opts.filename||'download.bin',mime:opts.mime||'application/octet-stream',state:'in_progress',incognito:false});
 // The worker finishes its handoff after the listener returns.
 for(let i=0;i<20&&!calls.some(x=>x.action==='add'||x.action==='cancel');i++)await new Promise(r=>setTimeout(r,25));
 return {calls,add:calls.find(x=>x.action==='add')};
}
(async()=>{
for(const family of ['chromium','firefox']){
 const R=require('../browser/'+family+'/capture-rules.js');
 const allow=ext=>['zip','pdf','mp4'].includes(ext);
 const run=(lines,url,method='GET',extra={})=>R.evaluate(R.compile(lines),{url,method,...extra},allow);
 test(family,'Invalid lines are reported and skipped',()=>{const c=R.compile(['bad line','Z!http://x.test/a','http://x.test/<Q:*>','http://x.test/<F:*','G#toolongextensionname!http://x.test/a','',' // note ','http://ok.test/*']);assert.equal(c.rules.length,1);assert.deepEqual(c.errors,['bad line','Z!http://x.test/a','http://x.test/<Q:*>','http://x.test/<F:*','G#toolongextensionname!http://x.test/a']);});
 test(family,'Patterns match whole addresses literally with shortest-first wildcards',()=>{const r=R.parseLine('G!http://a.test/x.php?id=*&(y)=[1]');assert.equal(r.flags,'G');assert.deepEqual(R.match(r.tokens,'http://a.test/x.php?id=12&(y)=[1]'),[]);assert.equal(R.match(r.tokens,'http://a.test/x.php?id=12&(y)=[1]&z'),null);assert.equal(R.match(r.tokens,'xhttp://a.test/x.php?id=12&(y)=[1]'),null);
  const c=R.parseLine('https://a.test/<F:*>/<T:*>/*/*');assert.deepEqual(R.match(c.tokens,'https://a.test/name.zip/mp4/p/q/r'),['name.zip','mp4']);assert.deepEqual(R.match(R.parseLine('https://a.test/*-*-end').tokens,'https://a.test/a-b-c-end'),[],'wildcards back off to later separators when needed');});
 test(family,'A scheme wildcard allows http and https only',()=>{const l=['http*://a.test/f/*'];assert.ok(run(l,'https://a.test/f/1'));assert.ok(run(l,'http://a.test/f/1'));assert.ok(run(l,'httpx://a.test/f/1'),'word characters complete the scheme, as in the reference grammar');assert.equal(run(l,'ftp://a.test/f/1'),null);});
 test(family,'A host tilde allows one optional subdomain label',()=>{const l=['https://~example.test/get'];assert.ok(run(l,'https://example.test/get'));assert.ok(run(l,'https://www.example.test/get'));assert.equal(run(l,'https://a.b.example.test/get'),null);assert.equal(run(l,'https://notexample.test/get'),null);});
 test(family,'A trailing question mark makes the query optional',()=>{const l=['https://a.test/get?'];assert.ok(run(l,'https://a.test/get'));assert.ok(run(l,'https://a.test/get?x=1'));assert.equal(run(l,'https://a.test/get/'),null);});
 test(family,'A rule without captures or extension captures outright',()=>{assert.deepEqual(run(['https://a.test/dl/*'],'https://a.test/dl/anything'),{action:'capture',extension:'',rule:'https://a.test/dl/*'});});
 test(family,'A file-name capture supplies the extension to the type list',()=>{const l=['S!http://s*.t.test/download.php?name=<F:*>&md5=*'];assert.equal(run(l,'http://s12.t.test/download.php?name=file.zip&md5=1').action,'capture');assert.equal(run(l,'http://s12.t.test/download.php?name=file.zip&md5=1').extension,'zip');assert.equal(run(l,'http://s12.t.test/download.php?name=notes.txt&md5=1').action,'continue');assert.equal(run(l,'http://s12.t.test/download.php?name=dir/file.zip&md5=1'),null);});
 test(family,'An extension capture is used directly',()=>{const l=['https://a.test/get/*.<T:*>?token=*'];const r=run(l,'https://a.test/get/file.MP4?token=1');assert.equal(r.action,'capture');assert.equal(r.extension,'mp4');assert.equal(run(l,'https://a.test/get/file.exe?token=1').action,'continue');});
 test(family,'The T flag falls back to the address extension and a literal extension comes last',()=>{assert.deepEqual(run(['T!https://a.test/*/*'],'https://a.test/x/movie.mp4'),{action:'capture',extension:'mp4',rule:'T!https://a.test/*/*'});assert.equal(run(['T!https://a.test/*/*'],'https://a.test/x/y').action,'continue');assert.equal(run(['G#PDF!https://c.test/view?doi=*'],'https://c.test/view?doi=1').extension,'pdf');assert.equal(run(['T#PDF!https://c.test/*'],'https://c.test/a.zip').extension,'zip');});
 test(family,'Method flags stop the rule without ignoring the request',()=>{assert.equal(run(['G!https://a.test/*'],'https://a.test/x','POST').action,'continue');assert.equal(run(['P!https://a.test/*'],'https://a.test/x','POST').action,'capture');assert.equal(run(['GP!https://a.test/*'],'https://a.test/x','POST').action,'capture');assert.equal(run(['P!https://a.test/*'],'https://a.test/x','GET').action,'continue');});
 test(family,'D requires a secure request and X only page requests',()=>{assert.equal(run(['D!http*://a.test/*'],'http://a.test/x').action,'ignore');assert.equal(run(['D!http*://a.test/*'],'https://a.test/x').action,'capture');assert.equal(run(['X!https://a.test/*'],'https://a.test/x').action,'ignore');assert.equal(run(['https://a.test/*'],'https://a.test/x','GET',{background:true}).action,'ignore');assert.equal(run(['S!https://a.test/*'],'https://a.test/x','GET',{background:true}).action,'capture');});
 test(family,'D also accepts a plain address routed through an https proxy',()=>{assert.equal(run(['D!http*://a.test/*'],'http://a.test/x','GET',{proxy:'https'}).action,'capture');assert.equal(run(['D!http*://a.test/*'],'http://a.test/x','GET',{proxy:'http'}).action,'ignore');assert.equal(run(['D!http*://a.test/*'],'http://a.test/x','GET',{proxy:'https',secure:false}).action,'ignore');});
 test(family,'Stream rules are reported without capturing and H hides them',()=>{assert.equal(run(['V!https://v.test/*.m3u8'],'https://v.test/a.m3u8').action,'stream');assert.equal(run(['HV!https://v.test/*'],'https://v.test/a').action,'ignore');});
 test(family,'The first matching rule decides',()=>{const l=['G!https://a.test/*','https://a.test/*'];assert.equal(run(l,'https://a.test/x','POST').action,'continue');assert.equal(run(l,'https://a.test/x','GET').action,'capture');});
 test(family,'Rule and address limits are enforced',()=>{assert.equal(R.compile(['https://a.test/'+'a'.repeat(R.limits.line)]).rules.length,0);assert.equal(R.compile(Array.from({length:R.limits.rules+5},(_,i)=>'https://a.test/'+i)).rules.length,R.limits.rules);assert.equal(run(['https://a.test/*'],'https://a.test/'+'x'.repeat(R.limits.url)),null);});
 test(family,'Many wildcards stay fast on long addresses',()=>{const l=['https://a.test/'+'*-'.repeat(40)+'end'];const started=Date.now();for(let i=0;i<20;i++)run(l,'https://a.test/'+'x-'.repeat(2000)+'nope');assert.ok(Date.now()-started<2000);});
 let h=await harness(family,{rules:['https://files.test/go']});assert.ok(h.add,'rule without an extension captures a generic download');pass(family+': A matching site rule captures a download outside the type list');
 h=await harness(family);assert.equal(h.add,undefined);pass(family+': Without rules the generic download stays in the browser');
 h=await harness(family,{rules:['X!https://files.test/*'],url:'https://files.test/report.pdf',filename:'report.pdf',mime:'application/pdf'});assert.equal(h.add,undefined);pass(family+': An ignore rule keeps an otherwise captured file in the browser');
 h=await harness(family,{rules:['https://files.test/dl?name=<F:*>'],url:'https://files.test/dl?name=archive.zip',local:['zip'],desktop:['zip']});assert.ok(h.add);pass(family+': A captured file name is checked against the type list');
 h=await harness(family,{rules:['https://files.test/dl?name=<F:*>'],url:'https://files.test/dl?name=archive.zip',local:['zip'],desktop:['pdf']});assert.equal(h.add,undefined);pass(family+': The desktop type list still applies to a captured file name');
 h=await harness(family,{rules:['https://files.test/dl?name=<F:*>'],url:'https://files.test/dl?name=archive.zip',local:['pdf']});assert.equal(h.add,undefined);pass(family+': A captured file name outside the type list is not captured');
 h=await harness(family,{rules:['P!https://files.test/go']});assert.equal(h.add,undefined);pass(family+': A method-filtered rule leaves the normal decision in place');
 // Response-level rules: V/A/C feed the media panel, S captures background requests.
 const response=(url,type='xmlhttprequest',mime='application/octet-stream')=>({requestId:'r'+Math.random(),url,tabId:3,frameId:0,type,method:'GET',statusCode:200,responseHeaders:[{name:'Content-Type',value:mime}]});
 let w=await harness(family,{rules:['V!https://files.test/stream/<F:*>?'],skipDownload:true});
 await w.emit('response',response('https://files.test/stream/clip.mp4?token=secret-value'));await w.settle();
 let observed=w.session.get('media:3')||[];assert.equal(observed.length,1);assert.equal(observed[0].type,'video/mp4');assert.equal(observed[0].url,'https://files.test/stream/clip.mp4?token=secret-value');pass(family+': A stream rule routes a non-media reply to the video panel with its extension type');
 await w.emit('response',response('https://files.test/other/file.bin'));await w.settle();assert.equal((w.session.get('media:3')||[]).length,1);pass(family+': Replies outside the stream rules stay off the panel');
 assert.equal(w.calls.filter(c=>c.action==='add').length,0);pass(family+': Stream rules never hand a request off as a download');
 w=await harness(family,{rules:['S!https://files.test/api/<F:*>?'],local:['zip'],skipDownload:true});
 await w.emit('response',response('https://files.test/api/data.zip?sig=private'));await w.settle();
 let adds=w.calls.filter(c=>c.action==='add');assert.equal(adds.length,1);assert.equal(adds[0].url,'https://files.test/api/data.zip?sig=private');pass(family+': An S rule hands a matching background request off to UDM');
 await w.emit('response',response('https://files.test/api/data.zip?sig=private'));await w.settle();assert.equal(w.calls.filter(c=>c.action==='add').length,1);pass(family+': A repeated background request is handed off once');
 await w.emit('response',response('https://files.test/api/notes.txt'));await w.settle();assert.equal(w.calls.filter(c=>c.action==='add').length,1);pass(family+': A captured file name outside the type list is not handed off');
 await w.emit('response',{...response('https://files.test/api/page.zip'),type:'main_frame'});await w.settle();assert.equal(w.calls.filter(c=>c.action==='add').length,1);pass(family+': Page navigations are left to the download path');
 let diagnostics=null;for(const fn of w.handlers.message||[])fn({action:'capture-diagnostics'},{},r=>{diagnostics=r;});
 const rules=diagnostics?.rules||[];assert.ok(rules.length>=3);const text=JSON.stringify(rules);assert.ok(!text.includes('private')&&text.includes('sig')&&rules.some(r=>r.point==='response'&&r.action==='capture'&&r.host==='files.test'));pass(family+': Diagnostics keep rule decisions without query values');
}
console.log(count+' passed');
})().catch(e=>{console.error(e);process.exit(1);});
