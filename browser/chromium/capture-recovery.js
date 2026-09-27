/* Durable ownership journal for automatically paused browser downloads. No request data is persisted. */
(function(root){
 'use strict';
 function create(api,nativeRequest,report){
  const key='captureOwnershipV1',alarm='udm-capture-recovery',active=new Set();let serial=Promise.resolve(),recovering=null;
  const ordered=fn=>{const next=serial.catch(()=>{}).then(fn);serial=next;return next;};
  const valid=r=>r&&Number.isSafeInteger(r.id)&&r.id>=0&&/^\d{13}-[a-f0-9-]{36}$/.test(r.token)&&/^[a-f0-9]{64}$/.test(r.fingerprint)&&['held','submitting','accepted'].includes(r.phase)&&[0,1].includes(r.protocol);
  async function read(){const value=(await api.storage.local.get(key))[key]||{};if(!value||typeof value!=='object'||Array.isArray(value)||Object.keys(value).length>128||Object.entries(value).some(([t,r])=>!valid(r)||r.token!==t))throw Error('The interrupted-download journal is invalid. Check UDM and the browser Downloads page.');return value;}
  async function save(records){await api.storage.local.set({[key]:records});if(Object.keys(records).length)await api.alarms?.create(alarm,{periodInMinutes:1});else await api.alarms?.clear(alarm);}
  const update=r=>ordered(async()=>{const records=await read();records[r.token]={id:r.id,token:r.token,fingerprint:r.fingerprint,phase:r.phase,protocol:r.protocol};await save(records);});
  const remove=r=>ordered(async()=>{const records=await read();delete records[r.token];await save(records);});
  async function fingerprint(item){
   if(!Number.isSafeInteger(item.id)||typeof item.startTime!=='string'||!Number.isFinite(Date.parse(item.startTime))||typeof item.url!=='string')throw Error('The browser did not provide a stable download identity.');
   const bytes=new TextEncoder().encode(JSON.stringify([item.id,item.startTime,item.url]));
   return Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),b=>b.toString(16).padStart(2,'0')).join('');
  }
  async function begin(item){
   const record={id:item.id,token:Date.now()+'-'+crypto.randomUUID(),fingerprint:await fingerprint(item),phase:'held',protocol:0};
   active.add(record.token);
   try{await ordered(async()=>{const records=await read();if(Object.keys(records).length>=128||Object.values(records).some(r=>r.id===item.id))throw Error('This browser download already has an interrupted handoff. Use Recover interrupted downloads.');
    records[record.token]=record;await save(records);
   });return record;}catch(e){active.delete(record.token);throw e;}
  }
  async function submit(record,message){
   record.phase='submitting';await update(record);
   const result=await nativeRequest({...message,...(record.protocol===1?{captureToken:record.token}:{})});
   if(result?.ok){record.phase='accepted';await update(record);}
   return result;
  }
  async function settle(record){
   const [item]=await api.downloads.search({id:record.id});
   if(!item||item.state==='complete'||await fingerprint(item)!==record.fingerprint){await remove(record);return true;}
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
    const zeroBytePause=!!root.browser&&item.state==='interrupted'&&item.error==='USER_CANCELED'&&item.bytesReceived===0;
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
  function install(){
   api.alarms?.onAlarm.addListener(event=>{if(event.name===alarm)void recover().catch(e=>report(e.message));});
   api.runtime.onStartup?.addListener(()=>void recover().catch(e=>report(e.message)));
   api.runtime.onInstalled?.addListener(()=>void recover().catch(e=>report(e.message)));
   void recover().catch(e=>report(e.message));
  }
  return {begin,submit,finish,recover,install};
 }
 root.UdmCaptureRecovery={create};if(typeof module==='object'&&module.exports)module.exports=root.UdmCaptureRecovery;
})(globalThis);
