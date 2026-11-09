/* Durable identity for browser media handoffs. Never replay an uncertain request. */
(function(root){
 'use strict';
 const hash=async text=>Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',new TextEncoder().encode(text))),b=>b.toString(16).padStart(2,'0')).join('');
 const KEY='udm-media-receipts-v1',LIMIT=128,tokenPattern=/^[0-9]{13}-[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}$/,bindingPattern=/^[a-f0-9]{64}$/;
 // Match native selection identity without retaining URLs, cookies or validators.
 function selectionIdentity(plan){
  const value=JSON.parse(JSON.stringify(plan));
  delete value.durationMs;delete value.manifestUrl;delete value.resources;delete value.resourceVersion;delete value.live;delete value.encrypted;
  for(const group of value.periods||[value]){
   delete group.resources;delete group.resourceVersion;
   for(const track of group.tracks||[]){delete track.playlist;if(track.index)delete track.index.url;for(const part of track.segments||[])delete part.url;}
  }
  const canonical=value=>Array.isArray(value)?value.map(canonical):value&&typeof value==='object'?Object.fromEntries(Object.keys(value).sort().map(key=>[key,canonical(value[key])])):value;
  return canonical(value);
 }
 function create(api,send){
  let queue=Promise.resolve();const active=new Set();
  function serial(fn){const result=queue.catch(()=>{}).then(fn);queue=result;return result;}
  async function read(){const stored=(await api.storage.local.get(KEY))[KEY],rows=stored===undefined?[]:stored;
   if(!Array.isArray(rows)||rows.length>LIMIT||rows.some(r=>!r||!tokenPattern.test(r.token)||!bindingPattern.test(r.binding)||r.legacyBinding!==undefined&&!bindingPattern.test(r.legacyBinding)||r.retryBinding!==undefined&&!bindingPattern.test(r.retryBinding)||typeof r.label!=='string'||r.label.length>160||!Number.isFinite(r.created)||!Number.isFinite(r.updated)||r.status==='accepted'&&(typeof r.nativeId!=='string'||!r.nativeId||typeof r.present!=='boolean')||!['pending','accepted','released','uncertain'].includes(r.status))||new Set(rows.map(r=>r.token)).size!==rows.length)throw Error('The saved media recovery list needs repair. No new video was sent.');
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
   if(row.status==='released')return {ok:true,...row};
   const preferences=await send({action:'preferences-if-running'});
   if(!preferences?.ok||preferences.mediaReceipts!==1)throw Error('Open the UDM version that accepted this video, then check again.');
   const reply=await send({action:'media-status',mediaToken:token});
   if(!reply?.ok||!['accepted','released','uncertain'].includes(reply.status)||reply.status==='accepted'&&(typeof reply.id!=='string'||!reply.id||typeof reply.present!=='boolean'))throw Error(reply?.error||'UDM did not confirm this media receipt.');
   // Historical acceptance cannot be undone by a different profile or expired receipt.
   if(row.status==='accepted'&&(reply.status!=='accepted'||reply.id!==row.nativeId))throw Error('This UDM session could not verify the previously accepted download. The saved receipt was kept; review the original UDM profile before clearing it.');
   const value=await update(token,{status:reply.status,...(reply.status==='accepted'?{nativeId:reply.id,present:reply.present}:{} )});
   if(!value)throw Error('This media notice was cleared while it was being checked.');
   return {ok:true,...value};
  }
  function recovered(row){
   if(row.status!=='accepted')throw Error('This video still needs review. Open Recover interrupted downloads in the UDM extension.');
   if(row.present===false)throw Error('UDM accepted this video earlier, but its history record was removed. Review its media recovery notice before downloading again.');
   return {ok:true,id:row.nativeId,mediaReceipt:row.token,recoveredMedia:true};
  }
  async function recoverPrior(prior,guard){
   const row=prior.status==='released'?prior:await check(prior.token);guard();
   if(row.status==='released'){await ack(row.token);throw Error('The previous video request was not accepted. Select Download again to start a new request.');}
   return recovered(row);
  }
  async function recoverSelection(binding,guard,validate){
   guard();
   const key=await hash(binding);guard();
   const prior=await serial(async()=>(await read()).find(row=>row.binding===key||row.retryBinding===key));guard();
   if(!prior)return null;
   await validate();guard();
   return recoverPrior(prior,guard);
  }
  const readPreferences=()=>send({action:'preferences'});
  function preparePreferences(){
   // This read may start the desktop in the background. Keep it local to
   // the selection and discard a stale reply before negotiating admission.
   const read=()=>Promise.resolve().then(readPreferences).then(reply=>({reply,at:Date.now()}),error=>({error}));
   let pending=read();
   return async check=>{
    const current=pending;
    let result=await current;check();
    if(result.error||Date.now()-result.at>2000){
     if(pending===current)pending=read();
     result=await pending;check();
    }
    if(result.error)throw result.error;
    return result.reply;
   };
  }
  async function submit(request,scope,binding,label){
   scope.check();
   const legacyKey=await hash(binding);scope.check();
   const structured=request.action==='adaptive'&&request.plan&&!request.plan.live;
   const key=structured?await hash(JSON.stringify({schema:2,binding,selection:selectionIdentity(request.plan)})):legacyKey;scope.check();
   const retryKey=scope.retryBinding?await hash(scope.retryBinding):null;scope.check();
   const matches=row=>row.binding===key||retryKey===row.retryBinding;
   const conflict=rows=>structured&&rows.some(r=>r.binding===legacyKey||r.binding!==key&&r.legacyBinding===legacyKey&&!['accepted','released'].includes(r.status));
   // Old receipts have no selection structure. Never silently reuse or bypass them.
   const prior=await serial(async()=>{const rows=await read();if(conflict(rows))throw Error('An earlier media handoff for this page needs review before changing its selection. Open Recover interrupted downloads in the UDM extension.');return rows.find(matches);});scope.check();
   if(prior)return recoverPrior(prior,scope.check);
   // Negotiate with the running desktop, not only its possibly newer host.
   const preferences=await (scope.readPreferences?scope.readPreferences(scope.check):readPreferences());scope.check();
   if(!preferences?.ok)throw Error(preferences?.error||'UDM could not read its media settings.');
   const advertised=preferences.mediaRequestLimit;
   const messageLimit=Number.isSafeInteger(advertised)&&advertised>=200000&&advertised<=8*1024*1024?advertised:200000;
   // Reserve space for the receipt token and the native bridge request ID.
   const messageBytes=new TextEncoder().encode(JSON.stringify(request)).length+256;
   if(messageBytes>messageLimit)throw Error(advertised===undefined?'Update the UDM desktop app before sending this large video playlist.':'This video request exceeds the desktop message limit. Choose a smaller selection.');
   const youtube=request.action==='media'||request.action==='sabr';
   const directAudio=request.action==='add'&&request.youtubeDirectAudio===true;
   if(preferences.mediaReceipts!==1||youtube&&preferences.youtubeMediaReceipts!==1||directAudio&&preferences.youtubeDirectAudioReceipts!==1)
    return scope.send((beforeSend,validateSelection)=>send(request,beforeSend,validateSelection));
   let sent=false,token;
   const abort=()=>{if(token)active.delete(token);};
   try{
    token=await serial(async()=>{
     scope.check();const rows=await read();scope.check();
     if(conflict(rows)||rows.some(matches))throw Error('This video already has a pending handoff. Review its receipt before trying again.');
     if(rows.length>=LIMIT)throw Error('Review interrupted media downloads before sending more videos.');
     const token=Date.now()+'-'+crypto.randomUUID();
     rows.push({token,binding:key,...(structured?{legacyBinding:legacyKey}:{}),...(retryKey?{retryBinding:retryKey}:{}),label:String(label||'Video download').replace(/[\x00-\x1f\x7f]/g,' ').slice(0,160),status:'pending',created:Date.now(),updated:Date.now()});
     await write(rows);return token;
    });
    active.add(token);scope.signal?.addEventListener('abort',abort,{once:true});scope.check();
    const result=await scope.send((beforeSend,validateSelection)=>{
     if(!beforeSend){sent=true;return send({...request,mediaToken:token});}
     return send({...request,mediaToken:token},()=>{beforeSend();sent=true;},validateSelection);
    });
    if(!result?.ok||typeof result.id!=='string'||!result.id)throw Error(result?.error||'UDM did not acknowledge this video.');
    await update(token,{status:'accepted',nativeId:result.id,present:true});scope.check();
    return {...result,mediaReceipt:token};
   }catch(error){
    if(token&&!sent){await serial(async()=>{const rows=await read();await write(rows.filter(r=>r.token!==token));});throw error;}
    if(token&&sent&&!scope.signal?.aborted){
     let row;try{row=await check(token,true);}catch{}scope.check();if(row?.status==='accepted')return recovered(row);if(row?.status==='released'){await ack(token);throw Error((error?.message||'UDM did not accept this video.')+' No download was added. Resolve the error before trying again.');}
    }
    throw error;
   }finally{if(token)active.delete(token);scope.signal?.removeEventListener('abort',abort);}
  }
  return {submit,recoverSelection,preparePreferences,list,check,ack};
 }
 const api={create};if(typeof module!=='undefined'&&module.exports)module.exports=api;else root.UdmMediaReceipts=api;
})(typeof globalThis!=='undefined'?globalThis:this);
