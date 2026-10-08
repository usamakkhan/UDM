'use strict';
const assert=require('node:assert/strict'),Context=require('../browser/chromium/request-context.js'),Multipart=require('../browser/chromium/multipart.js'),fs=require('node:fs'),path=require('node:path');
let count=0;const test=(name,fn)=>{fn();console.log('PASS '+name);count++;};
const url='https://download.test/export',type='multipart/form-data; boundary=fixture',sender={tab:{id:2,incognito:false},frameId:0,documentId:'doc',url:'https://download.test/form'};
const fields=[['z','first'],['a','middle'],['z','last']],dictionary={a:['middle'],z:['first','last']};
function setup({before=true,snapshot=fields,observed=dictionary,source=sender}={}){
 const c=Context.create({}),event={requestId:'r',url,method:'POST',tabId:2,frameId:0,documentId:'doc',documentUrl:sender.url};
 if(before&&snapshot)c.captureForm({url,fields:snapshot},source);
 c.begin({...event,requestBody:{formData:observed}});
 c.headers({...event,requestHeaders:[{name:'Content-Type',value:type}]});
 c.finish({...event,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment; filename=report.zip'}]});
 if(!before&&snapshot)c.captureForm({url,fields:snapshot},source);
 return {c,event,read:()=>c.resolve({url,browserDownload:true})};
}
test('Ordered multipart snapshot preserves interleaved duplicate names and original boundary',()=>{
 const {read}=setup();const r=read();assert.equal(r.request.contentType,type);
 assert.equal(Buffer.from(r.request.body,'base64').toString(),'--fixture\r\nContent-Disposition: form-data; name="z"\r\n\r\nfirst\r\n--fixture\r\nContent-Disposition: form-data; name="a"\r\n\r\nmiddle\r\n--fixture\r\nContent-Disposition: form-data; name="z"\r\n\r\nlast\r\n--fixture--\r\n');
});
test('Snapshot delivered after response metadata binds to the same request',()=>assert(setup({before:false}).read().request));
test('Dictionary alone never claims the original multipart ordering',()=>assert.equal(setup({snapshot:null}).read().request,null));
test('Changed, missing or additional observed fields reject the snapshot',()=>{
 for(const observed of [{a:['middle'],z:['first','changed']},{z:['first','last']},{...dictionary,extra:['field']}])assert.equal(setup({observed}).read().request,null);
});
test('Tabs, frame IDs and browser-issued document identities cannot cross-bind',()=>{
 for(const source of [{...sender,tab:{id:9}},{...sender,frameId:1},{...sender,documentId:'old'}])assert.equal(setup({source}).read().request,null);
});
test('Incognito, popup and invalid source messages are rejected',()=>{
 for(const source of [{...sender,tab:{id:2,incognito:true}},{url:sender.url},{...sender,url:'file:///x'}])assert.equal(Context.create({}).captureForm({url,fields},source),false);
});
test('Snapshot is consumed by one request and cannot authorize a subsequent submission',()=>{
 const {c,event,read}=setup();assert(read().request);
 const next={...event,requestId:'second'};c.begin({...next,requestBody:{formData:dictionary}});c.headers({...next,requestHeaders:[{name:'Content-Type',value:type}]});c.finish({...next,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});
 assert.equal(read().request,null);
});
test('Concurrent request ambiguity leaves both responses in the browser',()=>{
 const {c,event}=setup({snapshot:null});const next={...event,requestId:'second'};
 c.begin({...next,requestBody:{formData:dictionary}});c.headers({...next,requestHeaders:[{name:'Content-Type',value:type}]});c.finish({...next,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});
 c.captureForm({url,fields},sender);assert.equal(c.resolve({url,browserDownload:true}).request,null);
});
test('Snapshot fields containing file/blob objects are never reconstructed',()=>{
 assert.equal(Multipart.capture([['file',{name:'private.txt'}]]),null);assert.equal(Multipart.capture([['file',new Uint8Array([1,2])]]),null);
});
test('Newline and quote escaping follows multipart text serialization without changing field order',()=>{
 const snapshot=Multipart.capture([['quote"\r\nname','one\ntwo\rthree\r\nfour'],['utf8','café ✓']]);
 const bytes=Multipart.encode(snapshot,type),text=Buffer.from(bytes).toString();
 assert(text.includes('name="quote%22%0D%0Aname"'));assert(text.includes('one\r\ntwo\r\nthree\r\nfour'));
 assert(text.includes('café ✓'));assert(Multipart.matches(snapshot,[['utf8','café ✓'],['quote"\r\nname','one\r\ntwo\r\nthree\r\nfour']]));
});
test('Only an unambiguous form-data boundary is accepted',()=>{
 for(const bad of ['multipart/mixed; boundary=fixture','multipart/form-data','multipart/form-data; boundary=a; boundary=b','multipart/form-data; boundary="bad\r\nvalue"','multipart/form-data; boundary=""','multipart/form-data; boundary='+('x'.repeat(71))])assert.equal(Multipart.encode(Multipart.capture(fields),bad),null);
 assert(Multipart.encode(Multipart.capture(fields),'multipart/form-data; boundary="fixture"'));
});
test('MIME overhead counts toward the native 4 MiB body limit',()=>{
 assert(Multipart.encode(Multipart.capture([['x','a'.repeat(4*1048576-100)]]),type));
 assert.equal(Multipart.encode(Multipart.capture([['x','a'.repeat(4*1048576-10)]]),type),null);
});
test('Content-Length mismatch or changed boundary invalidates an attached form',()=>{
 for(const header of [{name:'Content-Length',value:'1'},{name:'Content-Type',value:'multipart/form-data; boundary=other'}]){
  const {c,event,read}=setup();c.headers({...event,requestHeaders:[header]});assert.equal(read().request,null);
 }
});
test('Same-origin 307 retains verified bytes when the browser repeats the matching fields',()=>{
 const {c,event,read}=setup(),expected=read().request;c.finish({...event,statusCode:307});
 const next={...event,url:'https://download.test/next'};c.begin({...next,requestBody:{formData:dictionary}});c.headers({...next,requestHeaders:[{name:'Content-Type',value:type}]});c.finish({...next,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});
 assert.deepEqual(c.resolve({url:next.url,browserDownload:true}).request,expected);
});
test('Changed body or cross-origin 307 cannot inherit multipart authorization',()=>{
 for(const [nextUrl,observed] of [['https://download.test/next',{a:['changed'],z:['first','last']}],['https://different.test/next',dictionary]]){
  const {c,event}=setup();c.finish({...event,statusCode:307});const next={...event,url:nextUrl};
  c.begin({...next,requestBody:{formData:observed}});c.headers({...next,requestHeaders:[{name:'Content-Type',value:type}]});c.finish({...next,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});assert.equal(c.resolve({url:nextUrl,browserDownload:true}).request,null);
 }
});
test('Late snapshot can bind across an already observed same-origin 307',()=>{
 const {c,event}=setup({snapshot:null});c.finish({...event,statusCode:307});const next={...event,url:'https://download.test/next'};
 c.begin({...next,requestBody:{formData:dictionary}});c.headers({...next,requestHeaders:[{name:'Content-Type',value:type}]});c.finish({...next,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});c.captureForm({url,fields},sender);assert(c.resolve({url:next.url,browserDownload:true}).request);
});
test('Form snapshots expire and tab removal clears their bytes',()=>{
 const original=Date.now;try{let now=original();Date.now=()=>now;const c=Context.create({});c.captureForm({url,fields},sender);assert(c.diagnostics().bodyBytes>0);now+=2001;assert.deepEqual(c.diagnostics(),{requests:0,bodyBytes:0});c.captureForm({url,fields},sender);c.clear(2);assert.deepEqual(c.diagnostics(),{requests:0,bodyBytes:0});}finally{Date.now=original;}
});
test('Snapshot and request storage remain within the aggregate cache budget',()=>{
 const c=Context.create({});for(let i=0;i<80;i++)c.captureForm({url,fields:[['x','a'.repeat(200000)]]},sender);assert(c.diagnostics().bodyBytes<=8*1024*1024);
});
test('Context-menu actions never replay captured multipart forms',()=>{const {c}=setup();assert.equal(c.resolve({url}).request,null);});
test('Firefox and Chromium share matching capture code and document-start form registration',()=>{
 for(const name of ['multipart.js','forms.js','request-context.js'])assert.equal(fs.readFileSync(path.join(__dirname,'../browser/chromium',name),'utf8'),fs.readFileSync(path.join(__dirname,'../browser/firefox',name),'utf8'));
 for(const browser of ['chromium','firefox']){const m=JSON.parse(fs.readFileSync(path.join(__dirname,'../browser',browser,'manifest.json')));assert(m.content_scripts.some(x=>x.run_at==='document_start'&&x.all_frames&&x.js.includes('forms.js')));}
});

test('A reorder with indistinguishable observed fields is rejected instead of guessing',()=>{
 const {c,read}=setup({snapshot:null});c.captureForm({url,fields:[['a','middle'],['z','first'],['z','last']],priorFields:fields},sender);assert.equal(read().request,null);
});
test('Changed site formdata fields use the final observed entry list',()=>{
 const {c,read}=setup({snapshot:null});c.captureForm({url,fields,priorFields:[['original','yes']]},sender);assert(read().request);
});
test('Quote-heavy multipart is not limited by unrelated URL-encoding expansion',()=>{
 const fields=[['quotes','"'.repeat(350000)]],{read}=setup({snapshot:fields,observed:{quotes:[fields[0][1]]}});assert(read().request);assert.equal(Buffer.from(read().request.body,'base64').toString(),'--fixture\r\nContent-Disposition: form-data; name="quotes"\r\n\r\n'+fields[0][1]+'\r\n--fixture--\r\n');
});
test('Literal invalid-percent field names retain their exact multipart spelling',()=>{
 const fields=[['raw%literal','yes']],{read}=setup({snapshot:fields,observed:{'raw%literal':['yes']}});assert(read().request);assert(Buffer.from(read().request.body,'base64').toString().includes('name="raw%literal"'));
});

console.log('ALL '+count+' MULTIPART CHECKS PASSED');
