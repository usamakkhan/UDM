'use strict';
const assert=require('node:assert/strict'),{create}=require('../browser/chromium/grabber-rendered.js');
const url='https://example.test/project',snapshot={version:1,url,references:[],truncated:false};let passed=0;
function fixture(change={}){
 let frameReads=0,tabReads=0,epoch=0;
 const api={tabs:{get:async()=>({url,status:'complete',incognito:false,...(tabReads++?change.afterTab:change.beforeTab)})},webNavigation:{getFrame:async()=>({url,documentId:'original',...(frameReads++?change.afterFrame:change.beforeFrame)})},scripting:{executeScript:async request=>{assert.equal(request.world,'ISOLATED');assert.deepEqual(request.target,{tabId:1,frameIds:[0]});if(change.epoch)epoch++;if(change.throw)throw Error('Tab closed');return [{frameId:0,documentId:'original',result:snapshot,...change.result}];}}};
 return create(api,{settled:async()=>{},token:()=>epoch});
}
(async()=>{
 const result=await fixture().capture({tabId:1,url});assert.equal(result.documentId,'original');assert.equal(result.tabId,1);console.log('PASS stable document capture');passed++;
 for(const [name,change] of Object.entries({reload:{afterFrame:{documentId:'reloaded'}},route:{epoch:true},navigated:{afterTab:{url:url+'/next'}},private:{beforeTab:{incognito:true}},loading:{beforeTab:{status:'loading'}},closed:{throw:true},wrongFrame:{result:{frameId:1}},staleResult:{result:{documentId:'retired'}},wrongUrl:{result:{result:{...snapshot,url:url+'/other'}}},missingDocument:{beforeFrame:{documentId:undefined}},errorPage:{afterFrame:{errorOccurred:true}}})){
  await assert.rejects(fixture(change).capture({tabId:1,url}));passed++;console.log('PASS reject '+name);
 }
 console.log('ALL '+passed+' RENDERED NAVIGATION CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});
