'use strict';
const assert=require('node:assert/strict'),Proxy=require('../browser/chromium/chromium-proxy.js'),Context=require('../browser/chromium/request-context.js');let passed=0;
const address='https://files.test/file.zip',endpoint={host:'127.0.0.1',port:8123},fixed=rules=>({mode:'fixed_servers',rules}),single=extra=>fixed({singleProxy:endpoint,...extra});
function test(name,fn){fn();passed++;console.log('PASS '+name);}
test('An explicit browser direct setting overrides a different desktop route',()=>assert.deepEqual(Proxy.route({mode:'direct'},address),{url:address,type:'direct'}));
test('System configuration remains unknown instead of being guessed direct',()=>assert.equal(Proxy.route({mode:'system'},address),undefined));
test('An HTTP proxy retains its exact endpoint and URL',()=>assert.deepEqual(Proxy.route(single(),address),{url:address,type:'http',host:'127.0.0.1',port:8123}));
test('HTTP and HTTPS choose their own protocol endpoints',()=>{const rules=fixed({proxyForHttp:{host:'http.test',port:80},proxyForHttps:{host:'https.test',port:81}});assert.equal(Proxy.route(rules,address).host,'https.test');assert.equal(Proxy.route(rules,address.replace('https','http')).host,'http.test');});
test('Missing protocol mappings use fallback or direct',()=>{assert.equal(Proxy.route(fixed({proxyForHttp:endpoint}),address).type,'direct');assert.equal(Proxy.route(fixed({proxyForHttp:endpoint,fallbackProxy:{host:'fallback.test'}}),address).host,'fallback.test');});
test('HTTP default port is 80',()=>assert.equal(Proxy.route(fixed({singleProxy:{host:'proxy.test'}}),address).port,80));
test('SOCKS5 preserves remote DNS and its default port',()=>assert.deepEqual(Proxy.route(fixed({singleProxy:{scheme:'socks5',host:'proxy.test'}}),address),{url:address,type:'socks5',host:'proxy.test',port:1080,proxyDNS:true}));
test('IPv6 proxy endpoints are canonicalized for the native contract',()=>assert.equal(Proxy.route(fixed({singleProxy:{host:'0:0::1',port:80}}),address).host,'::1'));
for(const mode of ['pac_script','auto_detect','unknown'])test(mode+' cannot silently become an unverified direct route',()=>assert.equal(Proxy.route({mode,pacScript:{data:'return DIRECT'}},address).unsupported,true));
test('HTTPS proxy preserves encryption and defaults to port 443',()=>assert.deepEqual(Proxy.route(fixed({singleProxy:{scheme:'https',host:'proxy.test'}}),address),{url:address,type:'https',host:'proxy.test',port:443}));
test('Chromium SOCKS4 retains local DNS',()=>assert.deepEqual(Proxy.route(fixed({singleProxy:{scheme:'socks4',host:'proxy.test'}}),address),{url:address,type:'socks4',host:'proxy.test',port:1080,proxyDNS:false}));
for(const scheme of ['quic'])test(scheme+' is not silently reinterpreted as HTTP or remote-DNS SOCKS',()=>assert.equal(Proxy.route(fixed({singleProxy:{scheme,...endpoint}}),address).unsupported,true));
for(const host of ['user:pass@proxy.test','proxy.test/path','proxy.test;other','proxy.test\r\nInjected:yes',''])test('Invalid proxy host '+JSON.stringify(host)+' is rejected',()=>assert(Proxy.route(fixed({singleProxy:{host,port:80}}),address).unsupported));
for(const port of [0,65536,'80',1.5])test('Invalid proxy port '+JSON.stringify(port)+' is rejected',()=>assert(Proxy.route(fixed({singleProxy:{host:'proxy.test',port}}),address).unsupported));
test('Unexpected proxy credentials are neither guessed nor serialized',()=>assert.deepEqual(Proxy.route(fixed({singleProxy:{...endpoint,username:'secret'}}),address),{unsupported:true}));
test('Conflicting single and per-protocol rules are rejected',()=>assert(Proxy.route(fixed({singleProxy:endpoint,proxyForHttp:endpoint}),address).unsupported));
for(const [pattern,url,expected] of [
 ['files.test',address,true],['files.test','https://sub.files.test/file',false],['.files.test','https://sub.files.test/file',true],['.files.test',address,false],['*files.test','https://otherfiles.test/file',true],
 ['https://files.test:443',address,true],['http://files.test:443',address,false],['files.test:444',address,false],['https://x.*.y.test:99','https://x.one.y.test:99/file',true],
 ['<local>','http://intranet/file',true],['<local>','http://intranet./file',false],['192.168.0.0/16','http://192.168.4.2/file',true],['192.168.0.0/16','http://192.169.4.2/file',false],['192.168.0.0/16','http://intranet/file',false],
 ['2001:db8::/32','http://[2001:db8:4::2]/file',true],['2001:db8::/32','http://[2001:db9::1]/file',false],['[2001:0db8::1]','https://[2001:db8::1]/file',true],['127.0.1','http://127.0.0.1/file',true]
])test('Bypass '+pattern+' matches '+url+' = '+expected,()=>assert.equal(Proxy.route(single({bypassList:['<-loopback>',pattern]}),url).type,expected?'direct':'http'));
for(const host of ['localhost','sub.localhost','localhost.','loopback','127.1.2.3','169.254.1.2','[::1]','[fe80::1234]'])test('Implicit bypass applies to '+host,()=>assert.equal(Proxy.route(single(),'http://'+host+'/file').type,'direct'));
test('Subtractive rules honor order',()=>{assert.equal(Proxy.route(single({bypassList:['<-loopback>','127.0.0.1']}),'http://127.0.0.1/file').type,'direct');assert.equal(Proxy.route(single({bypassList:['127.0.0.1','<-loopback>']}),'http://127.0.0.1/file').type,'http');});
test('Malformed and oversized bypass rules do not guess routing',()=>{for(const bypassList of [['192.168.0.0/33'],['[fe80::]/10'],['<unknown>'],Array(1025).fill('*')])assert(Proxy.route(single({bypassList}),address).unsupported);});
test('Credentialed and non-HTTP download addresses cannot create route records',()=>{for(const u of ['https://user:pass@files.test/a','ftp://files.test/a','file:///C:/a'])assert(Proxy.route(single(),u).unsupported);});
test('Firefox keeps its existing request-bound capture implementation',()=>assert.equal(Proxy.create({runtime:{getBrowserInfo(){}},proxy:{settings:{get(){throw Error('Do not read Firefox settings');}}}}),null));
test('Older or unavailable proxy APIs remain compatible',()=>assert.equal(Proxy.create({}),null));
(async()=>{
 let config=single(),changed,proxyError,reads=0,delayed,release;
 const api={runtime:{},proxy:{settings:{get(options,cb){assert.deepEqual(options,{incognito:false});reads++;if(delayed)release=()=>cb({value:config});else cb({value:config});},set(){throw Error('Production code must never change browser proxy settings');},onChange:{addListener(fn){changed=fn;}}},onProxyError:{addListener(fn){proxyError=fn;}}}};
 const capture=Proxy.create(api);capture.install();capture.install();
 let ob=capture.observe(address);await ob.ready;assert.equal(capture.resolve(ob).type,'http');assert.equal(reads,1);test('Startup and simultaneous observations share one settings read',()=>{});
 config={mode:'direct'};changed();let next=capture.observe(address);await next.ready;assert(capture.resolve(ob).unsupported);assert.equal(capture.resolve(next).type,'direct');test('A settings change invalidates prior request snapshots',()=>{});
 assert.equal(proxyError,undefined);assert.equal(capture.resolve(next).type,'direct');test('Unrelated proxy errors cannot invalidate a fixed route for another tab',()=>{});
 delayed=true;changed();const stale=capture.observe(address);const oldRelease=release;config=single();changed();const current=capture.observe(address);release();await current.ready;oldRelease();await stale.ready;assert(capture.resolve(stale).unsupported);assert.equal(capture.resolve(current).type,'http');test('A stale asynchronous read cannot replace the newer route',()=>{});
 const context=Context.create(api,null,capture),event={requestId:'request',url:address,method:'GET',tabId:1,frameId:0,documentId:'doc'};context.begin(event);context.finish({...event,statusCode:200});const resolved=await context.resolveDownload({url:address});assert.equal(resolved.proxy.type,'http');test('A real request context waits for route metadata before handoff',()=>{});
 changed();assert(context.resolve({url:address}).proxy.unsupported);test('Routing changes after response capture are not silently rebound',()=>{});release();
 const scopes=[];const privateApi={runtime:{},proxy:{settings:{get(options,cb){scopes.push(options.incognito);cb({value:options.incognito?{mode:'direct'}:single()});}}}};
 const privateCapture=Proxy.create(privateApi),regular=privateCapture.observe(address),privateRequest=privateCapture.observe(address,true);await Promise.all([regular.ready,privateRequest.ready]);
 test('Regular and InPrivate requests read separate effective configurations',()=>{assert.equal(privateCapture.resolve(regular).type,'http');assert.equal(privateCapture.resolve(privateRequest).type,'direct');assert.deepEqual(scopes,[false,true]);});
 const privateAgain=privateCapture.observe(address,true);await privateAgain.ready;test('InPrivate caching cannot overwrite the regular profile route',()=>{assert.equal(scopes.length,2);assert.equal(privateCapture.resolve(regular).type,'http');});
 const privateContext=Context.create(privateApi,null,privateCapture),privateEvent={...event,requestId:'private-request',incognito:true};privateContext.begin(privateEvent);privateContext.finish({...privateEvent,statusCode:200});const privateResolved=await privateContext.resolveDownload({url:address});test('Automatic request capture keeps the existing InPrivate exclusion',()=>assert.equal(privateResolved,null));
 console.log('ALL '+passed+' CHROMIUM PROXY CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});
