'use strict';
const assert=require('node:assert/strict'),{harness}=require('./browser.test.cjs');
let checks=0;const check=name=>{++checks;console.log('PASS '+name);};
(async()=>{
  for(const variant of ['chromium','firefox']){
    const calls=[];const url='https://www.youtube.com/watch?v=Q3TI27IN7X0';
    const tools=await harness({},false,{},null,null,variant,[],{
      AbortController,
      UdmNativeBridge:{create:()=>({warmControl:()=>{calls.push('warm');return new Promise(()=>{});},request:async(message,timeout,beforeSend,validateSelection)=>{await validateSelection?.();beforeSend?.();calls.push(message.action);return {ok:true,id:'native-job'};}})},
      UdmSites:{install(){},sync:async(_,tabId)=>{calls.push('sync:'+tabId);return {};},list:async()=>({ok:true,choices:[{key:'fixture'}]})},
      UdmCaptureRecovery:{create:()=>({install(){},recover:async()=>({ok:true,recovered:0}),reviews:async()=>({items:[]})})}
    });
    tools.api.runtime.getURL=file=>'extension://test-extension/'+file;
    tools.api.tabs.get=async id=>{assert.equal(id,7);return {id,url,incognito:false};};
    tools.api.tabs.sendMessage=async(id,message)=>calls.push(message.action+':'+id);
    tools.api.scripting.executeScript=async({target})=>{
      assert.equal(target.tabId,7);return [{frameId:0,documentId:'video-document',result:tools.player.snapshot}];
    };
    const toolSender={id:'test-extension',url:tools.api.runtime.getURL('popup.html')+'#tab=7',tab:{id:13},documentId:'tools-document'};
    const webSender={id:'test-extension',url,tab:{id:7,url},documentId:'video-document'};
    const sendTool=(action,sender=toolSender,fields={})=>new Promise(resolve=>tools.events.message({action,tabId:7,url,...fields},sender,resolve));
    assert.equal((await sendTool('formats')).ok,true);
    assert.equal((await sendTool('site-formats',webSender)).ok,true);
    assert.deepEqual(calls,['warm','youtube-player','warm']);
    check(variant+' lists formats from the source tab and warms the native channel without waiting or submitting a download');
    for(const action of ['open-browser-settings','restore-panels','sync-video-panels','capture-recover']){
      assert.equal(tools.events.message({action,tabId:7},webSender,()=>assert.fail('Web page invoked a Browser Tools command')),false);
      assert.equal((await sendTool(action)).ok,true);
    }
    assert(calls.includes('browser-settings')&&calls.includes('restore-panels:7'));
    const media=await sendTool('media',toolSender,{height:1080,formatKey:'137'});assert.equal(media.ok,true,media.error);
    assert.equal(calls.filter(action=>action==='media').length,1);
    check(variant+' Browser Tools commands stay trusted and a selected format submits once');
  }
  console.log('ALL '+checks+' BROWSER TOOLS HANDOFF CHECKS PASSED');
})().catch(error=>{console.error(error);process.exitCode=1;});
