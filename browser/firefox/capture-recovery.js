/* Durable ownership journal for automatic browser downloads. No request data is persisted. */
(function(root){
 'use strict';
 function create(api,nativeRequest,report){
  const key='captureOwnershipV1',alarm='udm-capture-recovery',active=new Set();let serial=Promise.resolve(),recovering=null;
  const ordered=fn=>{const next=serial.catch(()=>{}).then(fn);serial=next;return next;};
  const valid=r=>r&&Number.isSafeInteger(r.id)&&r.id>=0&&/^\d{13}-[a-f0-9-]{36}$/.test(r.token)&&/^[a-f0-9]{64}$/.test(r.fingerprint)&&['held','submitting','accepted','preparing','prepared','canceling','canceled','committing','browser-selected','native-selected','dismiss-selected'].includes(r.phase)&&[0,1,2].includes(r.protocol)&&(!r.phase.endsWith('-selected')||r.protocol<2)&&(r.browserRunning===undefined||(r.browserRunning===true&&r.protocol===2));
  async function read(){const value=(await api.storage.local.get(key))[key]||{};if(!value||typeof value!=='object'||Array.isArray(value)||Object.keys(value).length>128||Object.entries(value).some(([t,r])=>!valid(r)||r.token!==t))throw Error('The interrupted-download journal is invalid. Check UDM and the browser Downloads page.');return value;}
  async function save(records){await api.storage.local.set({[key]:records});if(Object.keys(records).length)await api.alarms?.create(alarm,{periodInMinutes:1});else await api.alarms?.clear(alarm);}
  const update=r=>ordered(async()=>{const records=await read();records[r.token]={id:r.id,token:r.token,fingerprint:r.fingerprint,phase:r.phase,protocol:r.protocol,...(r.browserRunning?{browserRunning:true}:{})};await save(records);});
  const remove=r=>ordered(async()=>{const records=await read();delete records[r.token];await save(records);});
  async function fingerprint(item){
   if(!Number.isSafeInteger(item.id)||typeof item.startTime!=='string'||!Number.isFinite(Date.parse(item.startTime))||typeof item.url!=='string')throw Error('The browser did not provide a stable download identity.');
   const bytes=new TextEncoder().encode(JSON.stringify([item.id,item.startTime,item.url]));
   return Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),b=>b.toString(16).padStart(2,'0')).join('');
  }
  async function begin(item,options={}){
   const record={id:item.id,token:Date.now()+'-'+crypto.randomUUID(),fingerprint:await fingerprint(item),phase:'held',protocol:options.browserRunning===true?2:0,...(options.browserRunning===true?{browserRunning:true}:{})};
   active.add(record.token);
   try{await ordered(async()=>{const records=await read();if(Object.keys(records).length>=128||Object.values(records).some(r=>r.id===item.id))throw Error('This browser download already has an interrupted handoff. Use Recover interrupted downloads.');
    records[record.token]=record;await save(records);
   });return record;}catch(e){active.delete(record.token);throw e;}
  }
  const isZeroBytePause=item=>api.runtime.getURL?.('')?.startsWith('moz-extension://')===true&&item?.state==='interrupted'&&item.error==='USER_CANCELED'&&item.bytesReceived===0;
  // A stopped Firefox response may have no partial data. Its error code is not
  // proof that resume is supported; use the browser's explicit capability.
  const resumable=item=>typeof item?.canResume==='boolean'?item.canResume:item?.paused===true;
  const fullyReceived=item=>item&&Number.isSafeInteger(item.totalBytes)&&item.totalBytes>0&&Number.isSafeInteger(item.bytesReceived)&&item.bytesReceived>=item.totalBytes;
  const eligible=(record,item)=>item&&!fullyReceived(item)&&(record.browserRunning?item.state==='in_progress'&&!item.paused:['in_progress','interrupted'].includes(item.state)&&(item.paused||isZeroBytePause(item)));
  async function sameItem(record){const [item]=await api.downloads.search({id:record.id});return item&&await fingerprint(item)===record.fingerprint?item:null;}
  async function retained(record){
   const item=await sameItem(record);
   // UDM never paused a running handoff. A new paused/canceled state belongs to
   // the user or browser and must not be undone when native preparation releases.
   if(!record.browserRunning&&item&&(item.paused||isZeroBytePause(item)))await api.downloads.resume(record.id);
   await remove(record);return {ok:true,browserRetained:true};
  }
  async function release(record){
   const status=await nativeRequest({action:'capture-release',captureToken:record.token});
   if(!status?.ok||status.status!=='released')throw Error('UDM could not release the prepared download. Recover interrupted downloads before retrying.');
   return retained(record);
  }
  async function transaction(record){
   const status=await nativeRequest({action:'capture-status',captureToken:record.token});
   if(!status?.ok)throw Error(status?.error||'UDM could not check the prepared download.');
   if(['accepted','review','discarded'].includes(status.status)){await remove(record);return status;}
   if(status.status==='released')return retained(record);
   if(status.status!=='prepared')throw Error('UDM cannot yet confirm the interrupted handoff. The saved request was not sent again; check UDM before retrying.');
   let item=await sameItem(record);
   const canceled=record.phase==='canceled'||record.phase==='committing';
   if(canceled){
    // A durable cancellation proof survives browser history removal or ID reuse.
    // A still-present original response must remain canceled before admission.
    if(item&&(item.state!=='interrupted'||item.error!=='USER_CANCELED'))return release(record);
   }else{
    if(!item||item.state==='complete')return release(record);
    const retryCancel=record.phase==='canceling'&&item.state==='interrupted'&&item.error==='USER_CANCELED';
    if(!retryCancel&&!eligible(record,item))return release(record);
    record.phase='canceling';await update(record);
    await api.downloads.cancel(record.id);
    // A resolved cancel is not sufficient: Chromium can complete just before it.
    item=await sameItem(record);
    if(item?.state==='complete')return release(record);
    if(!item||item.state!=='interrupted'||item.error!=='USER_CANCELED')throw Error('The browser has not confirmed cancellation. UDM has saved the request and will check again.');
    record.phase='canceled';await update(record);
   }
   record.phase='committing';await update(record);
   const result=await nativeRequest({action:'capture-commit',captureToken:record.token});
   if(!result?.ok||!['accepted','review'].includes(result.status))throw Error(result?.error||'UDM could not confirm the saved download. Recover interrupted downloads before retrying.');
   record.phase='accepted';await update(record);return result;
  }
  async function submit(record,message){
   if(record.protocol===2){
    const current=await sameItem(record);
    if(!eligible(record,current))return retained(record);
    record.phase='preparing';await update(record);
    const prepared=await nativeRequest({action:'capture-prepare',captureToken:record.token,download:message});
    if(!prepared?.ok||prepared.status!=='prepared')throw Error(prepared?.error||'UDM did not prepare this download.');
    record.phase='prepared';await update(record);
    return transaction(record);
   }
   record.phase='submitting';await update(record);
   // Filename/context reads and the durable checkpoint can outlast a tiny
   // browser transfer. Do not replay a response that has already arrived.
   const [current]=await api.downloads.search({id:record.id});
   const zeroBytePause=api.runtime.getURL?.('')?.startsWith('moz-extension://')===true&&current?.state==='interrupted'&&current.error==='USER_CANCELED'&&current.bytesReceived===0;
   const received=current&&Number.isSafeInteger(current.totalBytes)&&current.totalBytes>0&&Number.isSafeInteger(current.bytesReceived)&&current.bytesReceived>=current.totalBytes;
   if(!current||!['in_progress','interrupted'].includes(current.state)||(!current.paused&&!zeroBytePause)||received||await fingerprint(current)!==record.fingerprint){
    record.phase='held';await update(record);return {ok:true,browserRetained:true};
   }
   const result=await nativeRequest({...message,...(record.protocol===1?{captureToken:record.token}:{})});
   if(result?.ok){record.phase='accepted';await update(record);}
   return result;
  }
  async function settle(record){
   if(record.browserRunning&&record.phase==='held'){await retained(record);return true;}
   if(record.protocol===2&&record.phase!=='held'){await transaction(record);await remove(record);return true;}
   const [item]=await api.downloads.search({id:record.id});
   if(!item||item.state==='complete'||await fingerprint(item)!==record.fingerprint){await remove(record);return true;}
   if(record.phase==='browser-selected'||record.phase==='native-selected'||record.phase==='dismiss-selected'){
    let decision=record.phase;
    if(record.protocol===1){
     const status=await nativeRequest({action:'capture-legacy-release',captureToken:record.token,confirmed:true});
     if(!status?.ok||!['accepted','released'].includes(status.status))throw Error(status?.error||'Update UDM to finish this recovery choice.');
     // An accepted receipt found during review takes precedence over uncertainty.
     if(status.status==='accepted'&&decision!=='dismiss-selected'){decision='native-selected';record.phase=decision;await update(record);}
    }
    // Native inspection can take time. Never operate on a replaced browser ID.
    const current=await sameItem(record);
    if(decision!=='dismiss-selected'&&current&&current.state!=='complete'){
     if(decision==='native-selected'){
      if(current.state!=='interrupted'||current.error!=='USER_CANCELED')await api.downloads.cancel(record.id);
      const after=await sameItem(record);if(after&&after.state!=='complete'&&(after.state!=='interrupted'||after.error!=='USER_CANCELED'))throw Error('The browser has not confirmed cancellation. Retry the saved choice.');
     }
     else if(resumable(current)){
      await api.downloads.resume(record.id);
      const after=await sameItem(record);if(after&&after.state!=='complete'&&(after.paused||after.state!=='in_progress'))throw Error('The browser has not resumed this download. Retry the saved choice.');
     }
     else if(current.state==='interrupted')throw Error('The browser cannot resume this download automatically. Open Downloads to retry it.');
    }
    await remove(record);return true;
   }
   let ownership=record.phase==='held'?'released':record.phase==='accepted'?'accepted':'uncertain';
   if(record.phase==='submitting'&&record.protocol===1){
    const status=await nativeRequest({action:'capture-reconcile',captureToken:record.token});
    if(status?.ok&&['accepted','released'].includes(status.status))ownership=status.status;
   }
   if(ownership==='uncertain'){await report('UDM could not confirm an interrupted download. Check UDM and the browser Downloads page before resuming it. The request was not sent again.');return false;}
   if(ownership==='accepted'){
    // Persist known ownership before canceling; a failed cancel is retried after restart.
    record.phase='accepted';await update(record);await api.downloads.cancel(record.id);
   }else{
    const zeroBytePause=api.runtime.getURL?.('')?.startsWith('moz-extension://')===true&&item.state==='interrupted'&&item.error==='USER_CANCELED'&&item.bytesReceived===0;
    if(item.paused||zeroBytePause)await api.downloads.resume(record.id);
   }
   await remove(record);return true;
  }
  async function finish(record){try{return await settle(record);}catch{await report('UDM could not finish recovering an interrupted download. Check UDM and the browser Downloads page, or try Recover interrupted downloads.');return false;}finally{active.delete(record.token);}}
  function recover(){
   if(recovering)return recovering;
   recovering=(async()=>{let recovered=0;const records=await ordered(read);
    for(const record of Object.values(records)){if(active.has(record.token))continue;active.add(record.token);if(await finish(record))recovered++;}
    const pending=Object.keys(await ordered(read)).length;return {ok:true,recovered,pending};
   })().finally(()=>{recovering=null;});return recovering;
  }
  async function reviews(){
   const records=await ordered(read),items=[];
   for(const record of Object.values(records)){
    if(active.has(record.token)||record.protocol===2||!['submitting','browser-selected','native-selected','dismiss-selected'].includes(record.phase))continue;
    const item=await sameItem(record);if(!item||item.state==='complete')continue;
    let host='';try{host=new URL(item.url).hostname;}catch{}
    items.push({token:record.token,name:(item.filename||'Interrupted download').split(/[\\/]/).pop(),host,started:item.startTime,state:item.state,paused:!!item.paused,resumable:resumable(item),decision:record.phase==='submitting'?'':record.phase==='browser-selected'?'browser':record.phase==='native-selected'?'native':'dismiss'});
   }
   return {ok:true,items};
  }
  async function resolve(token,decision,confirmed){
   if(!['browser','native','dismiss'].includes(decision)||confirmed!==true)throw Error('Check the matching download in UDM and confirm your choice first.');
   if(active.has(token))throw Error('This download is being recovered. Refresh the list.');
   active.add(token);
   try{
    const record=await ordered(async()=>{
     const records=await read(),record=records[token];
     if(!record||record.protocol===2||!['submitting','browser-selected','native-selected','dismiss-selected'].includes(record.phase))throw Error('This recovery entry changed. Refresh the list.');
     const phase=decision==='browser'?'browser-selected':decision==='native'?'native-selected':'dismiss-selected';
     if(record.phase!=='submitting'&&record.phase!==phase&&decision!=='dismiss')throw Error('A saved choice is already pending. Retry that choice.');
     if(!await sameItem(record))throw Error('The original browser download is no longer available. Refresh the list.');
     record.phase=phase;records[token]=record;await save(records);return record;
    });
    await settle(record);return {ok:true,owner:record.phase==='native-selected'?'native':record.phase==='browser-selected'?'browser':'dismiss'};
   }finally{active.delete(token);}
  }
  function install(){
   api.alarms?.onAlarm.addListener(event=>{if(event.name===alarm)void recover().catch(e=>report(e.message));});
   api.runtime.onStartup?.addListener(()=>void recover().catch(e=>report(e.message)));
   api.runtime.onInstalled?.addListener(()=>void recover().catch(e=>report(e.message)));
   void recover().catch(e=>report(e.message));
  }
  return {begin,submit,finish,recover,reviews,resolve,install};
 }
 root.UdmCaptureRecovery={create};if(typeof module==='object'&&module.exports)module.exports=root.UdmCaptureRecovery;
})(globalThis);
