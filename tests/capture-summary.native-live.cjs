'use strict';
const assert=require('node:assert/strict');
module.exports=async({worker,jobs,pass})=>{
 const result=await worker.evaluate(async()=>{
  const captureToken=Date.now()+'-'+crypto.randomUUID();
  const prepared=await nativeRequest({action:'capture-prepare',captureToken,download:{action:'add',url:'https://example.invalid/private/form.zip?token=private-query',filename:'saved-form.zip',headers:{Authorization:'Basic dGVzdDpzZWNyZXQ='},downloadLater:true,request:{method:'POST',contentType:'text/plain',body:btoa('private-form-body')}}});
  try{return {prepared,routine:await nativeRequest({action:'capture-status',captureToken}),detail:await nativeRequest({action:'capture-status',captureToken,details:true})};}
  finally {const released=await nativeRequest({action:'capture-release',captureToken});if(released.status!=='released')throw Error('Prepared fixture request was not released');}
 });
 for(const reply of [result.prepared,result.routine,result.detail]){assert(Number.isSafeInteger(reply.requestId));delete reply.requestId;}
 assert.equal(result.prepared.status,'prepared');assert.deepEqual(result.routine,{ok:true,status:'prepared'});
 assert.deepEqual(result.detail,{ok:true,status:'prepared',summary:{filename:'saved-form.zip',host:'example.invalid',method:'POST'}});
 assert.equal(jobs().length,0);pass('Edge real native host returns only requested recovery summary and releases without creating a download',{summary:result.detail.summary});
};