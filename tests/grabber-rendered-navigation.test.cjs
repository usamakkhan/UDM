'use strict';
const assert=require('node:assert/strict');
const url='https://example.test/project';let passed=0;
async function run(browser,includeHtml){
 const {create}=require('../browser/'+browser+'/grabber-rendered.js');
 const snapshot={version:1,url,references:[],truncated:false,...(includeHtml?{html:'<!doctype html><h1>Captured page</h1>',htmlUnavailable:false}:{})};
 function fixture(change={}){
  let frameReads=0,tabReads=0,epoch=0;
  const api={tabs:{get:async()=>({url,status:'complete',incognito:false,...(tabReads++?change.afterTab:change.beforeTab)})},webNavigation:{getFrame:async()=>({url,documentId:'original',...(frameReads++?change.afterFrame:change.beforeFrame)})},scripting:{executeScript:async request=>{
   assert.equal(request.world,'ISOLATED');assert.deepEqual(request.target,{tabId:1,frameIds:[0]});assert.deepEqual(request.args,[includeHtml]);
   if(change.epoch)epoch++;if(change.throw)throw Error('Tab closed');
   return change.noResults?[]:[{frameId:0,documentId:'original',result:snapshot,...change.result}];
  }}};
  return create(api,{settled:async()=>{},token:()=>epoch});
 }
 const capture=change=>fixture(change).capture({tabId:1,url,includeHtml});
 const result=await capture();assert.equal(result.documentId,'original');assert.equal(result.tabId,1);assert.equal(result.html,snapshot.html);passed++;
 for(const change of Object.values({reload:{afterFrame:{documentId:'reloaded'}},route:{epoch:true},navigated:{afterTab:{url:url+'/next'}},private:{beforeTab:{incognito:true}},loading:{beforeTab:{status:'loading'}},becamePrivate:{afterTab:{incognito:true}},startedLoading:{afterTab:{status:'loading'}},closed:{throw:true},wrongFrame:{result:{frameId:1}},staleResult:{result:{documentId:'retired'}},wrongUrl:{result:{result:{...snapshot,url:url+'/other'}}},missingDocument:{beforeFrame:{documentId:undefined}},errorPage:{afterFrame:{errorOccurred:true}},noResults:{noResults:true}})){
  await assert.rejects(capture(change));passed++;
 }
 const fragment=url+'#section';const fragmentChange={beforeTab:{url:fragment},afterTab:{url:fragment},beforeFrame:{url:fragment},afterFrame:{url:fragment},result:{result:{...snapshot,url:fragment}}};
 const canonical=await capture(fragmentChange);assert.equal(canonical.url,url);assert.equal(canonical.html,snapshot.html);passed++;
 await assert.rejects(capture({...fragmentChange,afterTab:{url:url+'#changed'},afterFrame:{url:url+'#changed'}}));passed++;
 console.log('PASS '+browser+' '+(includeHtml?'HTML':'references')+' navigation boundaries');
}
(async()=>{for(const browser of ['chromium','firefox'])for(const includeHtml of [false,true])await run(browser,includeHtml);console.log('ALL '+passed+' RENDERED NAVIGATION CHECKS PASSED');})().catch(e=>{console.error(e);process.exitCode=1;});
