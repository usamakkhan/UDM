'use strict';
const assert=require('node:assert/strict'),{harness}=require('./browser.test.cjs');let checks=0;
const page='https://www.youtube.com/watch?v=Q3TI27IN7X0',sender=()=>({id:'test-extension',url:page,tab:{id:7,url:page,title:'Video',incognito:false},frameId:0,documentId:'current-document'});
const send=(h,m,s=sender())=>new Promise(resolve=>h.events.message(m,s,resolve)),pass=name=>{checks++;console.log('PASS '+name);};
(async()=>{
 let h=await harness(),tabReads=0,injection;
 h.api.tabs.get=async()=>{tabReads++;return new Promise(()=>{});};
 h.api.scripting.executeScript=async args=>{injection=args;return [{frameId:0,documentId:'current-document',result:h.player.snapshot}];};
 let r=await send(h,{action:'formats',url:page});assert.equal(r.ok,true);assert(r.choices.some(c=>c.height===1080));assert.equal(tabReads,0);assert.equal(injection.injectImmediately,true);pass('Document-bound panel formats do not wait on unrelated current-tab lookup or document idle');
 h.api.scripting.executeScript=async()=>[{frameId:0,documentId:'different-document',result:h.player.snapshot}];r=await send(h,{action:'formats',url:page});assert.equal(r.ok,false);assert.match(r.error,/document changed/);pass('Navigation to a different document cannot populate the old panel');
 r=await send(h,{action:'formats',url:page},{...sender(),tab:{...sender().tab,incognito:true}});assert.equal(r.ok,false);assert.match(r.error,/private/);pass('Private content-script contexts remain rejected');
 h=await harness();tabReads=0;h.api.tabs.get=async()=>{tabReads++;return {url:page,incognito:false};};h.api.scripting.executeScript=async()=>[{frameId:0,result:h.player.snapshot}];r=await send(h,{action:'formats',url:page});assert.equal(r.ok,true);assert.equal(tabReads,1);pass('Missing injection document identity retains a current-tab verification');
 tabReads=0;const legacy=sender();delete legacy.documentId;r=await send(h,{action:'formats',url:page},legacy);assert.equal(r.ok,true);assert.equal(tabReads,2);pass('Legacy senders retain both original current-tab checks');
 h=await harness();tabReads=0;h.api.tabs.get=async()=>{tabReads++;return {url:'https://www.youtube.com/watch?v=ZYXwvutsrqp',incognito:false};};h.api.scripting.executeScript=async()=>[{frameId:0,documentId:'current-document',result:h.player.snapshot}];r=await send(h,{action:'media',url:page,height:1080,formatKey:'137',displayOnly:true});assert.equal(r.ok,false);assert.match(r.error,/page changed/);assert.equal(tabReads,1);assert(!h.calls.some(x=>x[0]==='native'&&['media','sabr'].includes(x[1].action)));pass('Download handoff cannot request the display-only shortcut or trust a stale sender tab');
 h=await harness();tabReads=0;h.api.tabs.get=async()=>{tabReads++;return {url:page,incognito:false};};h.api.scripting.executeScript=async()=>[{frameId:0,documentId:'current-document',result:h.player.snapshot}];r=await send(h,{action:'formats',url:page,tabId:7},{id:'test-extension'});assert.equal(r.ok,true);assert.equal(tabReads,2);pass('Popup format requests still verify current browser tab state');
 console.log('ALL '+checks+' FORMAT CONTEXT CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});
