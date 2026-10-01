'use strict';
const assert=require('node:assert/strict'),{create,selectedCookies}=require('../browser/chromium/grabber-session.js');
let passed=0;const pass=name=>{passed++;console.log('PASS '+name);};
const cookie=(name,value,extra={})=>({name,value,domain:'files.example.test',path:'/',hostOnly:true,secure:true,storeId:'0',...extra});
const chosen=selectedCookies([cookie('main','a'),cookie('foreign','b',{domain:'other.test'}),cookie('container','c',{storeId:'1'}),cookie('expired','d',{expirationDate:1}),cookie('parent','e',{domain:'.example.test',hostOnly:false}),cookie('partition','f',{partitionKey:{topLevelSite:'https://example.test',hasCrossSiteAncestor:false}}),cookie('foreignPartition','g',{partitionKey:{topLevelSite:'https://other.test'}}),cookie('crossAncestor','h',{partitionKey:{topLevelSite:'https://example.test',hasCrossSiteAncestor:true}})],'files.example.test','0','https://example.test');
assert.deepEqual(chosen.map(x=>x.name),['main','parent','partition']);pass('Cookie collection excludes foreign domains, stores, partitions, expired values and cross-site ancestors');
assert(chosen.every(x=>!('storeId' in x)&&!('partitionKey' in x)));pass('Cookie handoff contains only the scoped transport fields');
assert.throws(()=>selectedCookies(Array.from({length:513},(_,i)=>cookie('n'+i,'v')),'files.example.test','0','https://example.test'));pass('Cookie handoff enforces its count limit');
assert.throws(()=>selectedCookies([cookie('large','x'.repeat(25000))],'files.example.test','0','https://example.test'));pass('Cookie handoff enforces its value budget');
assert.throws(()=>selectedCookies([cookie('one','a'.repeat(8192)),cookie('two','b'.repeat(8192))],'files.example.test','0','https://example.test'));pass('Cookie collection stays within the native HTTP header limit');
function fixture(firefox=false){
 let tab={id:4,url:'https://files.example.test/private/index.html',incognito:false,...(firefox?{cookieStoreId:'firefox-container-2'}:{})},epoch='1:2',allowed=true,reads=[],sent=[],shared=[],domain='example.test',project={id:'p',name:'Private files',ticket:'t',origin:'https://files.example.test',url:tab.url,cookieDomain:domain};
 const store=firefox?'firefox-container-2':'0';
 const api={runtime:{id:'fixture',getURL:name=>'chrome-extension://fixture/'+name,...(firefox?{getBrowserInfo(){}}:{})},tabs:{get:async()=>({...tab})},permissions:{contains:async()=>allowed},cookies:{getAllCookieStores:async()=>[{id:store,tabIds:[4]}],getPartitionKey:async()=>({partitionKey:{topLevelSite:'https://example.test',hasCrossSiteAncestor:false}}),getAll:async filter=>{reads.push(filter);return [cookie('session','fixture-secret',{storeId:store,...(filter.partitionKey?.topLevelSite?{partitionKey:filter.partitionKey}:{})})];}}};
 const native=async message=>{sent.push(message);if(message.action==='grabber-login-pending')return {ok:true,projects:[{...project}]};shared.push(message);return {ok:true};};
 const navigation={supported:true,token:()=>epoch};const service=create(api,native,navigation);return {api,service,sender:{id:'fixture',url:api.runtime.getURL('popup.html')},message:{tabId:4,projectId:'p',ticket:'t'},reads,sent,shared,project,change(fn){fn(tab);},expire(){epoch='1:3';},deny(){allowed=false;}};
}
(async()=>{
 let f=fixture();await assert.rejects(f.service.pending(f.message,{id:'fixture',tab:{id:4},url:'https://files.example.test/'}));assert.equal(f.sent.length,0);pass('Web pages and content scripts cannot enumerate pending project logins');
 await assert.rejects(f.service.pending(f.message,{id:'different-extension',url:f.sender.url}));assert.equal(f.sent.length,0);pass('Other extensions cannot impersonate the UDM popup');
 await assert.rejects(f.service.complete(f.message,{id:'fixture',url:'chrome-extension://fixture/diagnostics.html'}));assert.equal(f.reads.length,0);pass('Only the actual extension popup can initiate website-session sharing');
 f=fixture();f.deny();await assert.rejects(f.service.complete(f.message,f.sender));assert.equal(f.reads.length,0);pass('Cookie access is checked before reading a browser session');
 f=fixture();f.change(tab=>{tab.incognito=true;});await assert.rejects(f.service.complete(f.message,f.sender));assert.equal(f.reads.length,0);pass('Private-window sessions are not saved into ordinary Grabber projects');
 f=fixture();await assert.rejects(f.service.complete({...f.message,ticket:'stale'},f.sender));assert.equal(f.reads.length,0);pass('Expired or replaced tickets cannot trigger cookie collection');
 f=fixture();f.project.cookieDomain='unrelated.test';await assert.rejects(f.service.complete(f.message,f.sender));assert.equal(f.reads.length,0);pass('Desktop cookie scope must match the selected browser website');
 f=fixture();const result=await f.service.complete(f.message,{...f.sender,tab:{id:9}});assert.equal(result.ok,true);assert.equal(f.shared.length,1);assert.equal(f.shared[0].session.Origin,'https://files.example.test');assert(f.shared[0].session.Cookies.every(x=>x.value==='fixture-secret'));assert.equal(f.reads.length,1);assert.deepEqual(f.reads[0].partitionKey,{});pass('Chromium reads matching ordinary and partitioned cookies before one native handoff');
 assert(!('session' in result));pass('Cookie values are not echoed back to the popup');
 f=fixture(true);await f.service.complete(f.message,f.sender);assert.equal(f.reads.length,1);assert.equal(f.reads[0].storeId,'firefox-container-2');assert.equal(f.reads[0].firstPartyDomain,null);assert.deepEqual(f.reads[0].partitionKey,{});pass('Firefox uses the active container and selects the matching first-party partition');
 f=fixture();const original=f.api.cookies.getAll;f.api.cookies.getAll=async q=>{const values=await original(q);f.change(tab=>{tab.url='https://other.test/';});return values;};await assert.rejects(f.service.complete(f.message,f.sender));assert.equal(f.shared.length,0);pass('Navigation to another website cancels session handoff');
 f=fixture();const originalAgain=f.api.cookies.getAll;f.api.cookies.getAll=async q=>{const values=await originalAgain(q);f.expire();return values;};await assert.rejects(f.service.complete(f.message,f.sender));assert.equal(f.shared.length,0);pass('Same-URL document replacement also cancels session handoff');
 console.log(`ALL ${passed} GRABBER BROWSER SESSION CHECKS PASSED`);
})().catch(e=>{console.error(e);process.exitCode=1;});
