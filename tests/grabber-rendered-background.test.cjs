'use strict';
const assert=require('node:assert/strict'),{harness}=require('./browser.test.cjs');
(async()=>{let passed=0;for(const variant of ['chromium','firefox']){
 const h=await harness({},false,{},null,null,variant,['grabber-rendered.js']),url='https://example.test/page',projectId='a'.repeat(32),calls=[];
 h.api.runtime.getURL=p=>'chrome-extension://test-extension/'+p;
 h.api.tabs.get=async()=>({url,status:'complete',incognito:false});
 h.api.webNavigation={getFrame:async()=>({url,documentId:'fixture-doc'})};
 h.api.scripting.executeScript=async()=>[{frameId:0,documentId:'fixture-doc',result:{version:1,url,baseUrl:url,title:'Fixture',references:[{url:'https://example.test/runtime.zip',kind:'link',description:'Runtime'}],truncated:false,visited:2}}];
 h.api.runtime.sendNativeMessage=async(_,message)=>{calls.push(message);switch(message.action){case 'grabber-rendered-projects':return {ok:true,projects:[{projectId,name:'Fixture'}]};case 'hello':return {ok:true,capabilities:['grabber-rendered-storage']};case 'grabber-rendered-begin':return {ok:true,request:{projectId,url,ticket:'bound-ticket',generation:1}};case 'grabber-rendered-complete':return {ok:true,references:message.snapshot.references.length,truncated:false};default:throw Error('Unexpected native action '+message.action);}};
 const trusted={id:'test-extension',url:h.api.runtime.getURL('popup.html')};
 const send=(action,sender=trusted)=>new Promise(resolve=>h.events.message({action,tabId:7,projectId},sender,resolve));
 for(const action of ['grabber-rendered-projects','grabber-rendered-capture'])for(const sender of [{id:'other',url:trusted.url},{id:'test-extension',url,tab:{id:7}},{id:'test-extension',url:h.api.runtime.getURL('diagnostics.html')}]){assert.equal((await send(action,sender)).ok,false);passed++;}
 assert.equal(calls.length,0);
 assert.equal((await send('grabber-rendered-projects')).projects[0].projectId,projectId);passed++;calls.length=0;
 const result=await send('grabber-rendered-capture');assert.equal(result.ok,true);assert.equal(result.references,1);assert.deepEqual(calls.map(x=>x.action),['grabber-rendered-projects','hello','grabber-rendered-begin','grabber-rendered-complete']);assert.equal(calls.at(-1).snapshot.documentId,'fixture-doc');assert.equal(calls.at(-1).ticket,'bound-ticket');passed++;
 console.log('PASS '+variant+' production background routes trusted popup and denies page/foreign senders');
}console.log('ALL '+passed+' RENDERED BACKGROUND ROUTING CHECKS PASSED');})().catch(e=>{console.error(e);process.exitCode=1;});
