'use strict';
const assert=require('node:assert/strict');let passed=0;
(async()=>{for(const family of ['chromium','firefox'])for(const mode of ['review','legacy','show-failure','review-failure']){
 const calls=[];const recovery=require('../browser/'+family+'/capture-recovery.js').create({},async message=>{calls.push(message.action);if(message.action==='show')return {ok:mode!=='show-failure'};if(message.action==='hello')return {ok:true,capabilities:mode==='legacy'?[]:['capture-review']};return {ok:mode!=='review-failure',reviews:1};},()=>{});
 if(mode.endsWith('failure'))await assert.rejects(recovery.openNative());else{const result=await recovery.openNative();assert.equal(result.ok,true);assert.equal(result.reviews,mode==='review'?1:undefined);}
 assert.deepEqual(calls,mode==='show-failure'?['show']:mode==='legacy'?['show','hello']:['show','hello','capture-review-open']);passed++;
}console.log(passed+' recovery open checks passed');})().catch(e=>{console.error(e);process.exitCode=1;});