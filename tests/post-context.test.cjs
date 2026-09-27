'use strict';
const assert=require('node:assert/strict'),Context=require('../browser/chromium/request-context.js');
let count=0;const test=(name,fn)=>{fn();console.log('PASS '+name);count++;};
const event={requestId:'post',url:'https://download.test/export',method:'POST',tabId:2,frameId:0,documentId:'page',documentUrl:'https://download.test/form'};
function fixture(body={raw:[{bytes:new Uint8Array([0,1,128,255]).buffer}]},type='application/octet-stream',response=true){const c=Context.create({});c.begin({...event,requestBody:body});c.headers({...event,requestHeaders:[{name:'Content-Type',value:type},...(body?.raw?.length===0?[{name:'Content-Length',value:'0'}]:[]),{name:'Cookie',value:'secret=fixture'}]});c.finish({...event,statusCode:200,responseHeaders:response?[{name:'Content-Disposition',value:'attachment; filename=export.zip'}]:[]});return c;}
const read=c=>c.resolve({url:event.url,browserDownload:true});
test('Raw binary POST is reconstructed exactly and transport metadata is separate from credentials',()=>{const r=read(fixture());assert.equal(r.request.body,'AAGA/w==');assert.equal(r.request.contentType,'application/octet-stream');assert.equal(r.headers.Cookie,undefined);});
test('Repeated UTF-8 URL-encoded fields retain all values',()=>{const r=read(fixture({formData:{q:['one two','✓'],empty:['']}},'application/x-www-form-urlencoded; charset=UTF-8'));assert.equal(Buffer.from(r.request.body,'base64').toString(),'q=one+two&q=%E2%9C%93&empty=');});
test('Multipart form dictionaries are not reconstructed as urlencoded data',()=>assert.equal(read(fixture({formData:{file:['private.txt']}},'multipart/form-data; boundary=x')).request,null));
test('Multi-chunk raw bodies preserve exact ordering and bytes',()=>{const r=read(fixture({raw:[{bytes:new Uint8Array([1,2]).buffer},{bytes:new Uint8Array([3,4]).buffer}]}));assert.equal(r.request.body,'AQIDBA==');});
test('Unavailable, file-backed and oversized bodies remain in the browser',()=>{for(const body of [undefined,{error:'unavailable'},{raw:[{file:'private.txt'}]},{raw:[{bytes:new ArrayBuffer(1048577)}]}]){const c=fixture(body===undefined?{}:body);assert.equal(read(c).request,null);}});
test('Zero-byte POST bodies remain explicit POST requests',()=>assert.equal(read(fixture({raw:[]})).request.body,''));
test('Only successful attachment responses offer replay',()=>{const c=fixture(undefined,undefined,false);assert.equal(read(c).request,null);c.finish({...event,statusCode:500,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});assert.equal(read(c).request,null);});
test('Empty form dictionaries with a nonzero upload length cannot become empty native POSTs',()=>{
 const c=fixture({formData:{}},'application/x-www-form-urlencoded');c.headers({...event,requestHeaders:[{name:'Content-Length',value:'1048576'}]});assert.equal(read(c).request,null);
});
test('Truncated raw and form bodies fail the request Content-Length check',()=>{
 for(const body of [{raw:[{bytes:new Uint8Array([1,2]).buffer}]},{formData:{a:['b']}}]){
  const c=fixture(body,'application/x-www-form-urlencoded');c.headers({...event,requestHeaders:[{name:'Content-Length',value:'99'}]});assert.equal(read(c).request,null);
 }
});
test('An explicitly empty request needs a verified zero upload length',()=>{
 const c=fixture({formData:{}},'application/x-www-form-urlencoded');assert.equal(read(c).request,null);
 c.headers({...event,requestHeaders:[{name:'Content-Length',value:'0'}]});assert.equal(read(c).request.body,'');
});
test('Matching upload length preserves raw bytes without forwarding Content-Length',()=>{
 const c=fixture();c.headers({...event,requestHeaders:[{name:'Content-Length',value:'4'}]});assert.equal(read(c).request.body,'AAGA/w==');assert.equal(read(c).headers['Content-Length'],undefined);
});

test('Context-menu URL clicks never replay a prior form submission',()=>assert.equal(fixture().resolve({url:event.url}).request,null));
test('Different POST bodies in one live document are rejected rather than guessed',()=>{const c=fixture();c.begin({...event,requestId:'other',requestBody:{raw:[{bytes:new Uint8Array([99]).buffer}]}});c.headers({...event,requestId:'other',requestHeaders:[{name:'Content-Type',value:'application/octet-stream'}]});c.finish({...event,requestId:'other',statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});assert.equal(read(c).method,'POST');assert.equal(read(c).request,null);});
test('Ambiguous request identity cannot silently turn POST into GET',()=>{const c=fixture();c.begin({...event,requestId:'other',tabId:3,requestBody:{raw:[]}});assert.equal(read(c).method,'POST');assert.equal(read(c).request,null);});
test('Same-origin 307 continuation retains a body omitted by the browser',()=>{const c=fixture();c.finish({...event,statusCode:307});const next={...event,url:'https://download.test/export2'};c.begin(next);c.headers({...next,requestHeaders:[{name:'Content-Type',value:'application/octet-stream'}]});c.finish({...next,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});assert.equal(c.resolve({url:next.url,browserDownload:true}).request.body,'AAGA/w==');});
test('Cross-origin continuation never inherits a missing request body',()=>{const c=fixture();c.finish({...event,statusCode:307});const next={...event,url:'https://other.test/export'};c.begin(next);c.headers({...next,requestHeaders:[{name:'Content-Type',value:'application/octet-stream'}]});c.finish({...next,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});assert.equal(c.resolve({url:next.url,browserDownload:true}).request,null);});
test('303 GET continuation discards previous POST metadata',()=>{const c=fixture();const next={...event,method:'GET',url:'https://download.test/file.zip'};c.begin(next);assert.equal(c.resolve({url:next.url,browserDownload:true}).request,null);assert.equal(c.resolve({url:next.url}).method,'GET');});
test('Tab closure erases sensitive request bodies',()=>{const c=fixture();c.clear(2);assert.equal(read(c),null);assert.deepEqual(c.diagnostics(),{requests:0,bodyBytes:0});});
test('CORS preflight cannot obscure the actual authenticated GET',()=>{const c=Context.create({});c.begin({...event,requestId:'preflight',method:'OPTIONS'});c.begin({...event,method:'GET'});c.headers({...event,method:'GET',requestHeaders:[{name:'Authorization',value:'Bearer fixture'}]});const r=c.resolve({url:event.url},true);assert.equal(r.method,'GET');assert.equal(r.headers.Authorization,'Bearer fixture');});

test('Large raw bodies preserve every byte through the 1 MiB boundary',()=>{
 for(const size of [65537,262145,1048576]){const data=Uint8Array.from({length:size},(_,i)=>(i*37+17)%256);const c=fixture({raw:[{bytes:data.slice(0,100003).buffer},{bytes:data.slice(100003).buffer}]});assert.deepEqual(Buffer.from(read(c).request.body,'base64'),Buffer.from(data));}
});
test('URL-encoded body limit applies after percent encoding Unicode fields',()=>{
 assert(read(fixture({formData:{x:['a'.repeat(1048574)]}},'application/x-www-form-urlencoded')).request);
 assert.equal(read(fixture({formData:{x:['✓'.repeat(120000)]}},'application/x-www-form-urlencoded')).request,null);
});
test('Raw multipart envelopes remain in the browser because file/blob bytes may be omitted',()=>{
 const data=Buffer.from('--fixture\r\nContent-Disposition: form-data; name="item"\r\n\r\none\r\n--fixture--\r\n');
 const r=read(fixture({raw:[{bytes:Uint8Array.from(data).buffer}]},'multipart/form-data; boundary=fixture'));
 assert.equal(r.method,'POST');assert.equal(r.request,null);
});
test('Cached POST bodies stay within 8 MiB and evicted records cannot become GET',()=>{
 const c=Context.create({});
 for(let i=0;i<12;i++){const e={...event,requestId:'r'+i,url:event.url+'/'+i};c.begin({...e,requestBody:{raw:[{bytes:new ArrayBuffer(1048576)}]}});c.headers({...e,requestHeaders:[{name:'Content-Type',value:'application/octet-stream'}]});c.finish({...e,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});}
 assert.deepEqual(c.diagnostics(),{requests:12,bodyBytes:8*1048576});
 const old=c.resolve({url:event.url+'/0',browserDownload:true});assert.equal(old.method,'POST');assert.equal(old.request,null);
 assert(c.resolve({url:event.url+'/11',browserDownload:true}).request);c.clear(2);assert.deepEqual(c.diagnostics(),{requests:0,bodyBytes:0});
});
test('An explicitly unavailable redirected body never falls back to an earlier submission',()=>{
 for(const body of [{error:'unavailable'},{raw:[{file:'unavailable.txt'}]},{raw:[{bytes:new ArrayBuffer(1048577)}]}]){
  const c=fixture();c.finish({...event,statusCode:307});const next={...event,url:'https://download.test/export2'};c.begin({...next,requestBody:body});c.headers({...next,requestHeaders:[{name:'Content-Type',value:'application/octet-stream'}]});c.finish({...next,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});assert.equal(c.resolve({url:next.url,browserDownload:true}).request,null);
 }
});
test('Large conflicting submissions are rejected by byte identity',()=>{
 const data=new Uint8Array(1048576),c=fixture({raw:[{bytes:data.buffer}]});data[data.length-1]=1;
 const next={...event,requestId:'different'};c.begin({...next,requestBody:{raw:[{bytes:data.buffer}]}});c.headers({...next,requestHeaders:[{name:'Content-Type',value:'application/octet-stream'}]});c.finish({...next,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});assert.equal(read(c).request,null);
});
test('Refreshing an existing request keeps only its current body in the cache budget',()=>{
 const c=fixture({raw:[{bytes:new ArrayBuffer(1048576)}]});for(let i=0;i<20;i++)c.begin({...event,requestBody:{raw:[{bytes:new ArrayBuffer(1048576)}]}});assert.deepEqual(c.diagnostics(),{requests:1,bodyBytes:1048576});
});
test('Request contexts expire without persisting bodies',()=>{
 const original=Date.now;try{let now=original();Date.now=()=>now;const c=fixture();now+=120001;assert.deepEqual(c.diagnostics(),{requests:0,bodyBytes:0});}finally{Date.now=original;}
});
test('Firefox and Chromium use the identical request-capture implementation',()=>{
 const fs=require('node:fs'),path=require('node:path');assert.equal(fs.readFileSync(path.join(__dirname,'../browser/chromium/request-context.js'),'utf8'),fs.readFileSync(path.join(__dirname,'../browser/firefox/request-context.js'),'utf8'));
});
console.log('ALL '+count+' POST CONTEXT CHECKS PASSED');
