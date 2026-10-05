/* Durable identity for adaptive media handoffs. Never replay an uncertain request. */
(function(root){
 'use strict';
 const KEY='udm-media-receipts-v1',LIMIT=128,tokenPattern=/^[0-9]{13}-[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}$/;
 function create(api,send){
  let queue=Promise.resolve();const active=new Set();
  function serial(fn){const result=queue.catch(()=>{}).then(fn);queue=result;return result;}
  async function read(){const stored=(await api.storage.local.get(KEY))[KEY],rows=stored===undefined?[]:stored;
   if(!Array.isArray(rows)||rows.length>LIMIT||rows.some(r=>!r||!tokenPattern.test(r.token)||!(/^[a-f0-9]{64}$/.test(r.binding))||typeof r.label!=='string'||r.label.length>160||!Number.isFinite(r.created)||!Number.isFinite(r.updated)||r.status==='accepted'&&(typeof r.nativeId!=='string'||!r.nativeId||typeof r.present!=='boolean')||!['pending','accepted','released','uncertain'].includes(r.status))||new Set(rows.map(r=>r.token)).size!==rows.length)throw Error('The saved media recovery list needs repair. No new video was sent.');
   return rows;
  }
  const write=rows=>api.storage.local.set({[KEY]:rows});
  async function update(token,change){return serial(async()=>{const rows=await read(),row=rows.find(r=>r.token===token);if(!row)return null;Object.assign(row,change,{updated:Date.now()});await write(rows);return row;});}
  async function ack(token,reviewed=false){if(!tokenPattern.test(token))throw Error('Invalid media receipt.');return serial(async()=>{const rows=await read(),row=rows.find(r=>r.token===token);if(row&&!['accepted','released'].includes(row.status)&&!(reviewed===true&&Date.now()-Number(token.slice(0,13))>660000))throw Error('Check this media handoff with UDM before clearing it.');if(row)await write(rows.filter(r=>r.token!==token));return {ok:true};});}
  async function list(){return serial(async()=>({ok:true,items:await read()}));}
  async function check(token,allowActive=false){
   if(!tokenPattern.test(token))throw Error('Invalid media receipt.');
   if(active.has(token)&&!allowActive)throw Error('This video is still awaiting its first reply. Check again shortly.');
   const row=await serial(async()=>(await read()).find(r=>r.token===token));
   if(!row)throw Error('This media notice was already cleared.');
   if(['accepted','released'].includes(row.status))return {ok:true,...row};
   const preferences=await send({action:'preferences-if-running'});
   if(!preferences?.ok||preferences.mediaReceipts!==1)throw Error('Open the UDM version that accepted this video, then check again.');
   const reply=await send({action:'media-status',mediaToken:token});
   if(!reply?.ok||!['accepted','released','uncertain'].includes(reply.status)||reply.status==='accepted'&&(typeof reply.id!=='string'||!reply.id||typeof reply.present!=='boolean'))throw Error(reply?.error||'UDM did not confirm this media receipt.');
   const value=await update(token,{status:reply.status,...(reply.status==='accepted'?{nativeId:reply.id,present:reply.present}:{} )});
   return {ok:true,...value};
  }
  function recovered(row){
   if(row.status!=='accepted')throw Error('This video still needs review. Open Recover interrupted downloads in the UDM extension.');
   if(row.present===false)throw Error('UDM accepted this video earlier, but its history record was removed. Review its media recovery notice before downloading again.');
   return {ok:true,id:row.nativeId,mediaReceipt:row.token,recoveredMedia:true};
  }
  async function submit(request,scope,binding,label){
   scope.check();
   // Negotiate with the running desktop, not only its possibly newer host.
   const preferences=await send({action:'preferences'});scope.check();
   if(!preferences?.ok)throw Error(preferences?.error||'UDM could not read its media settings.');
   const digest=await crypto.subtle.digest('SHA-256',new TextEncoder().encode(binding));scope.check();
   const key=Array.from(new Uint8Array(digest),b=>b.toString(16).padStart(2,'0')).join('');
   // Even an older desktop must not bypass a notice saved by a newer one.
   const prior=await serial(async()=>(await read()).find(r=>r.binding===key));scope.check();
   if(prior){
    const row=prior.status==='accepted'||prior.status==='released'?prior:await check(prior.token);scope.check();
    if(row.status==='released'){await ack(row.token);throw Error('The previous video request was not accepted. Select Download again to start a new request.');}
    return recovered(row);
   }
   if(preferences.mediaReceipts!==1)return scope.send(()=>send(request));
   let sent=false,token;
   const abort=()=>{if(token)active.delete(token);};
   try{
    token=await serial(async()=>{
     scope.check();const rows=await read();scope.check();
     if(rows.some(r=>r.binding===key))throw Error('This video already has a pending handoff. Review its receipt before trying again.');
     if(rows.length>=LIMIT)throw Error('Review interrupted media downloads before sending more videos.');
     const token=Date.now()+'-'+crypto.randomUUID();
     rows.push({token,binding:key,label:String(label||'Video download').replace(/[\x00-\x1f\x7f]/g,' ').slice(0,160),status:'pending',created:Date.now(),updated:Date.now()});
     await write(rows);return token;
    });
    active.add(token);scope.signal?.addEventListener('abort',abort,{once:true});scope.check();
    const result=await scope.send(()=>{sent=true;return send({...request,mediaToken:token});});
    if(!result?.ok||typeof result.id!=='string'||!result.id)throw Error(result?.error||'UDM did not acknowledge this video.');
    await update(token,{status:'accepted',nativeId:result.id,present:true});scope.check();
    return {...result,mediaReceipt:token};
   }catch(error){
    if(token&&!sent){await serial(async()=>{const rows=await read();await write(rows.filter(r=>r.token!==token));});throw error;}
    if(token&&sent&&!scope.signal?.aborted){
     let row;try{row=await check(token,true);}catch{}scope.check();if(row?.status==='accepted')return recovered(row);if(row?.status==='released'){await ack(token);throw Error('UDM did not accept this video. Select Download again to retry.');}
    }
    throw error;
   }finally{if(token)active.delete(token);scope.signal?.removeEventListener('abort',abort);}
  }
  return {submit,list,check,ack};
 }
 const api={create};if(typeof module!=='undefined'&&module.exports)module.exports=api;else root.UdmMediaReceipts=api;
})(typeof globalThis!=='undefined'?globalThis:this);
