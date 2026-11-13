const test=require('node:test'),assert=require('node:assert/strict');
const {summarizeNetlog}=require('./loopback-netlog.cjs');
const constants={logEventTypes:{URL_REQUEST_START_JOB:1,HTTP_STREAM_PARSER_READ_HEADERS:2,HTTP_TRANSACTION_READ_RESPONSE_HEADERS:3,URL_REQUEST_JOB_FILTERED_BYTES_READ:4},logSourceType:{URL_REQUEST:1,HTTP_STREAM_JOB_CONTROLLER:2}};
function event(type,params={},id=1,sourceType=1){return {type,phase:2,source:{id,type:sourceType},params};}
const own=event(1,{url:'http://127.0.0.1:4567/video.mp4?private=secret'});
test('Parser errors and byte counts survive without raw data or arbitrary params',()=>{
 const r=summarizeNetlog({constants,events:[own,event(2,{net_error:-370,secret:'private'}),event(4,{byte_count:123,bytes:'private-payload'})]},'http://127.0.0.1:4567');
 assert.equal(r.requests[0].bodyBytes,123);assert.equal(r.requests[0].events[1].netError,-370);assert.deepEqual(r.requests[0].paths,['/video.mp4']);assert(!JSON.stringify(r).includes('private'));assert(!JSON.stringify(r).includes('secret'));
});
test('Only framing-header allowlist and numeric status escape the raw header list',()=>{
 const r=summarizeNetlog({constants,events:[own,event(3,{headers:['HTTP/1.1 200 private-status','Content-Length: 123','Content-Type: video/mp4','Cookie: private-cookie','Set-Cookie: private-cookie','Authorization: private-token']})]},'http://127.0.0.1:4567');
 assert.equal(r.requests[0].events[1].status,200);assert.deepEqual(r.requests[0].events[1].headers,{'content-length':'123','content-type':'video/mp4'});assert(!JSON.stringify(r).includes('private'));
});
test('External, credential-bearing, other-port and Blob requests are excluded',()=>{
 const events=['https://example.test/private','http://user:private@127.0.0.1:4567/file','http://127.0.0.1:4568/private','blob:http://127.0.0.1:4567/private'].map((url,id)=>event(1,{url},id));
 assert.deepEqual(summarizeNetlog({constants,events},'http://127.0.0.1:4567').requests,[]);
});
test('A source that redirects outside the owned origin excludes its entire history',()=>{
 const events=[own,event(3,{headers:['Content-Length: 123']}),event(1,{url:'https://example.test/private'}),own];
 assert.deepEqual(summarizeNetlog({constants,events},'http://127.0.0.1:4567').requests,[]);
});
test('Other source types cannot enter through matching URL or shared source ID',()=>{
 const events=[own,event(2,{net_error:-370},1,2),event(1,{url:'http://127.0.0.1:4567/private'},2,2)];
 const r=summarizeNetlog({constants,events},'http://127.0.0.1:4567');assert.equal(r.requests.length,1);assert.equal(r.requests[0].events.length,1);
});
test('Event order does not lose early parser errors and malformed roots are rejected',()=>{
 const r=summarizeNetlog({constants,events:[event(2,{net_error:-370}),own]},'http://127.0.0.1:4567');assert.equal(r.requests[0].events[0].netError,-370);
 assert.throws(()=>summarizeNetlog({constants,events:[]},'https://example.test'));assert.throws(()=>summarizeNetlog({},'http://127.0.0.1:4567'));
});

test('Arbitrary source IDs and phase text cannot be exported',()=>{
 const bad=event(1,{url:'http://127.0.0.1:4567/file'},'private-id');const phase={...event(2,{net_error:-320}),phase:'private-phase'};
 const r=summarizeNetlog({constants,events:[bad,own,phase]},'http://127.0.0.1:4567');assert.equal(r.requests.length,1);assert(!JSON.stringify(r).includes('private'));
});
