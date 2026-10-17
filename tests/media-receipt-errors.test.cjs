'use strict';
const assert=require('node:assert/strict');
async function run(variant,status){
 const {create}=require('../browser/'+variant+'/media-receipts.js');
 const storage={},calls=[];
 const api={storage:{local:{get:async key=>({[key]:storage[key]}),set:async value=>Object.assign(storage,structuredClone(value))}}};
 const client=create(api,async message=>{
  calls.push(message.action);
  if(message.action.startsWith('preferences'))return {ok:true,mediaReceipts:1};
  if(message.action==='adaptive')return {ok:false,error:'FFmpeg is missing. Install media tools.'};
  if(message.action==='media-status')return {ok:true,status,...(status==='accepted'?{id:'saved-job',present:true}:{})};
  throw Error('Unexpected command');
 });
 const pending=client.submit({action:'adaptive'},{check(){},send:fn=>fn()},'fixture-binding','Fixture');
 if(status==='accepted')assert.equal((await pending).id,'saved-job');
 else await assert.rejects(pending,error=>{
  assert.match(error.message,/FFmpeg is missing/);
  assert.equal(error.message.includes('No download was added'),status==='released');return true;
 });
 assert.equal(calls.filter(action=>action==='adaptive').length,1,'Never replay a mutation');
 const rows=(await client.list()).items;
 assert.equal(rows.length,status==='released'?0:1);
 if(status!=='released')assert.equal(rows[0].status,status);
 console.log('PASS '+variant+' preserves rejection details with '+status+' receipt');
}
(async()=>{for(const variant of ['chromium','firefox'])for(const status of ['released','accepted','uncertain'])await run(variant,status);})().catch(error=>{console.error(error);process.exitCode=1;});
