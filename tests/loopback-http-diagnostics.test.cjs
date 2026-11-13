'use strict';
const test=require('node:test'),assert=require('node:assert/strict'),{createTrace,headers}=require('./loopback-http-diagnostics.cjs');
test('HTTP evidence retains framing headers without credentials or cookies',()=>{
 assert.deepEqual(headers({'Content-Length':'123','Transfer-Encoding':'chunked',Authorization:'secret',Cookie:'private','Set-Cookie':'private','X-Private':'private'}),{'content-length':'123','transfer-encoding':'chunked'});
});
test('Reordered protocol events remain attributable after the request arrives',()=>{
 const trace=createTrace('http://127.0.0.1:4567');trace.response({requestId:'a',statusCode:200,headers:{'Content-Length':'123'}});
 trace.failed({requestId:'a',errorText:'net::ERR_INVALID_HTTP_RESPONSE'});trace.request({requestId:'a',request:{url:'http://127.0.0.1:4567/video.mp4?token=private',method:'GET'}});
 const result=trace.result();assert.equal(result.events.length,2);assert.deepEqual(result.requests[0].history,[{path:'/video.mp4',method:'GET'}]);assert(!JSON.stringify(result).includes('private'));
});
test('Other origins, ports and credential-bearing URLs cannot enter exported evidence',()=>{
 const trace=createTrace('http://127.0.0.1:4567');
 for(const [id,url]of ['https://example.test/private','http://127.0.0.1:4568/private','http://user:secret@127.0.0.1:4567/private','blob:http://127.0.0.1:4567/private'].entries()){
  trace.request({requestId:id,request:{url,method:'GET'}});trace.failed({requestId:id,errorText:'net::ERR_FAILED'});
 }
 assert.deepEqual(trace.result().requests,[]);assert.deepEqual(trace.result().events,[]);
});
test('Redirect histories are kept without guessing which response belongs to which hop',()=>{
 const trace=createTrace('http://127.0.0.1:4567');for(const path of ['/first?secret=1','/second'])trace.request({requestId:'a',request:{url:'http://127.0.0.1:4567'+path,method:'GET'}});
 trace.finished({requestId:'a',encodedDataLength:42});assert.equal(trace.result().requests[0].history.length,2);assert.equal(trace.result().events[0].encodedDataLength,42);
});
test('A reused request ID cannot export an external redirect or its earlier extra-info event',()=>{
 const trace=createTrace('http://127.0.0.1:4567');trace.request({requestId:'a',request:{url:'http://127.0.0.1:4567/local',method:'GET'}});
 trace.response({requestId:'a',statusCode:302,headers:{'Content-Length':'123'}});
 trace.request({requestId:'a',request:{url:'https://example.test/private',method:'GET'}});
 trace.response({requestId:'a',statusCode:200,headers:{'Content-Length':'456'}});
 trace.request({requestId:'a',request:{url:'http://127.0.0.1:4567/returned',method:'GET'}});
 assert.deepEqual(trace.result().requests,[]);assert.deepEqual(trace.result().events,[]);
});
test('Nonstandard error details are omitted and non-loopback tracing is rejected',()=>{
 assert.throws(()=>createTrace('https://example.test'));const trace=createTrace('http://127.0.0.1:4567');trace.request({requestId:'a',request:{url:'http://127.0.0.1:4567/file',method:'GET'}});trace.failed({requestId:'a',errorText:'private detail'});assert.equal(trace.result().events[0].error,'network request failed');
});
