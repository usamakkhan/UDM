'use strict';
const assert=require('node:assert/strict');
(async()=>{for(const browser of ['chromium','firefox'])for(const cleanupFails of [false,true]){
 const {create}=require('../browser/'+browser+'/grabber-rendered.js'),calls=[],projectId='a'.repeat(32),url='https://example.test/page';let failCapture=true,generation=0;
 const api={tabs:{get:async()=>({url,status:'complete'})},webNavigation:{getFrame:async()=>({url,documentId:'doc'})},scripting:{executeScript:async()=>{if(failCapture)throw Error('Tab closed during capture');return [{frameId:0,documentId:'doc',result:{version:1,url,references:[],truncated:false}}];}}};
 const collector=create(api,null,async message=>{calls.push(message);switch(message.action){case 'hello':return {ok:true,capabilities:['grabber-rendered-storage']};case 'grabber-rendered-begin':return {ok:true,request:{projectId,url,ticket:'ticket-'+(++generation),generation}};case 'grabber-rendered-cancel':if(cleanupFails)throw Error('Desktop unavailable');return {ok:true,cancelled:true};case 'grabber-rendered-complete':return {ok:true,references:0,truncated:false};default:throw Error('Unexpected action');}});
 await assert.rejects(collector.captureProject({tabId:1,projectId}),/Tab closed during capture/);
 assert.deepEqual(calls.at(-1),{action:'grabber-rendered-cancel',projectId,ticket:'ticket-1'});assert(!calls.some(x=>x.action==='grabber-rendered-complete'));
 failCapture=false;assert((await collector.captureProject({tabId:1,projectId})).ok);assert.equal(calls.at(-1).ticket,'ticket-2');assert.equal(calls.filter(x=>x.action==='grabber-rendered-cancel').length,1);
 console.log('PASS '+browser+' failed capture cleanup and retry; cleanup failure='+cleanupFails);
}})().catch(e=>{console.error(e);process.exitCode=1;});
