/* Original browser-level SABR observation. Captured context remains session-only. */
const UdmStreamingCapture=(()=>{
 'use strict';
 let api;const pending=new Map(),queues=new Map(),counts=new Map(),epochs=new Map(),TTL=300000,MAX_BODY=131072;
 const note=(tab,key)=>{const row=counts.get(tab)||{};row[key]=(row[key]||0)+1;counts.set(tab,row);};
 function endpoint(value){try{const u=new URL(value);return u.protocol==='https:'&&u.hostname.endsWith('.googlevideo.com')&&u.pathname==='/videoplayback'&&(u.searchParams.has('sabr')||u.searchParams.get('ump')==='1');}catch{return false;}}
 function body(request){
  if(!request||request.error||!Array.isArray(request.raw)||request.raw.some(p=>p.file||!p.bytes))return null;
  const chunks=request.raw.map(p=>new Uint8Array(p.bytes)),size=chunks.reduce((n,p)=>n+p.length,0);if(!size||size>MAX_BODY)return null;
  const result=new Uint8Array(size);let at=0;for(const part of chunks){result.set(part,at);at+=part.length;}
  try{return globalThis.__udmUmpV1?.request(result)?result:null;}catch{return null;}
 }
 function player(){
  const u=new URL(location.href),id=u.pathname==='/watch'?u.searchParams.get('v'):/^\/(?:embed|shorts)\/([\w-]{11})/.exec(u.pathname)?.[1],p=document.getElementById('movie_player');
  if(!/^https:\/\/(?:www\.|m\.)?youtube(?:-nocookie)?\.com\//.test(u.href)||!/^[-\w]{11}$/.test(id||'')||p?.getVideoData?.()?.video_id!==id||p?.classList?.contains('ad-showing')||p?.classList?.contains('ad-interrupting'))return null;
  return {videoId:id,page:u.href,timeOrigin:performance.timeOrigin};
 }
 function before(event){
  if(event.tabId<0||!endpoint(event.url))return;
  note(event.tabId,'seen');if(event.frameId<0){note(event.tabId,'unassociatedFrame');return;}
  if(event.method!=='POST'){note(event.tabId,'nonPost');return;}
  note(event.tabId,'requests');const bytes=body(event.requestBody);if(!bytes){note(event.tabId,'unsupportedBody');return;}
  for(const [key,value] of pending)if(Date.now()-value.time>30000)pending.delete(key);
  if(pending.size>=64)pending.delete(pending.keys().next().value);
  const entry={tabId:event.tabId,frameId:event.frameId,documentId:event.documentId||'',url:event.url,time:Date.now(),epoch:epochs.get(event.tabId)||0};
  // Resolve page identity at request time, independently of page fetch wrappers.
  entry.proof=(async()=>{try{
   const tab=await api.tabs.get(event.tabId);if(tab.incognito)return null;
   const rows=await api.scripting.executeScript({target:{tabId:event.tabId,frameIds:[event.frameId]},world:'MAIN',func:player});
   const row=rows.find(x=>x.frameId===event.frameId),value=row?.result;
   if(!value||entry.documentId&&row.documentId!==entry.documentId||!entry.documentId&&(!Number.isFinite(value.timeOrigin)||entry.time<value.timeOrigin))return null;
   let raw='';for(const b of bytes)raw+=String.fromCharCode(b);
   return {...value,body:btoa(raw),documentId:row.documentId||entry.documentId};
  }catch{return null;}})();
  pending.set(event.requestId,entry);
 }
 async function response(event){
  const entry=pending.get(event.requestId);if(!entry)return;pending.delete(event.requestId);
  const mime=event.responseHeaders?.find(h=>h.name.toLowerCase()==='content-type')?.value||'';
  if(![200,206].includes(event.statusCode)||!/^application\/vnd\.yt-ump(?:;|$)/i.test(mime)||event.url!==entry.url){note(entry.tabId,'rejectedResponse');return;}
  const proof=await entry.proof;if(entry.epoch!==(epochs.get(entry.tabId)||0))return;if(!proof){note(entry.tabId,'unmatchedPlayer');return;}
  if(event.documentId&&event.documentId!==proof.documentId)return;
  const key='sabr:'+entry.tabId,task=(queues.get(entry.tabId)||Promise.resolve()).catch(()=>{}).then(async()=>{
   if(entry.epoch!==(epochs.get(entry.tabId)||0))return;
   const prior=(await api.storage.session.get(key))[key]||[];
   const item={...proof,url:entry.url,frameId:entry.frameId,capturedAt:entry.time,captureSource:'browser-request'};
   await api.storage.session.set({[key]:[...prior.filter(p=>Date.now()-p.capturedAt<TTL&&!(p.videoId===item.videoId&&p.frameId===item.frameId&&p.documentId===item.documentId)),item].slice(-8)});
   note(entry.tabId,'accepted');
  });queues.set(entry.tabId,task);try{await task;}finally{if(queues.get(entry.tabId)===task)queues.delete(entry.tabId);}
 }
 async function session(context){
  await (queues.get(context.tabId)||Promise.resolve()).catch(()=>{});
  const values=(await api.storage.session.get('sabr:'+context.tabId))['sabr:'+context.tabId]||[];
  return values.filter(x=>x.videoId===context.id&&x.frameId===context.frameId&&Date.now()-x.capturedAt<TTL&&x.capturedAt<=Date.now()&&
   (context.documentId?x.documentId===context.documentId:!x.documentId&&Number.isFinite(context.snapshot?.timeOrigin)&&x.timeOrigin===context.snapshot.timeOrigin))
   .sort((a,b)=>b.capturedAt-a.capturedAt)[0]||null;
 }
 async function clear(tab){epochs.set(tab,(epochs.get(tab)||0)+1);for(const [id,p] of pending)if(p.tabId===tab)pending.delete(id);counts.delete(tab);const task=(queues.get(tab)||Promise.resolve()).catch(()=>{}).then(()=>api.storage.session.remove('sabr:'+tab));queues.set(tab,task);try{await task;}finally{if(queues.get(tab)===task)queues.delete(tab);}}
 function install(value){api=value;
  api.webRequest.onBeforeRequest?.addListener(before,{urls:['https://*.googlevideo.com/*']},['requestBody']);
  api.webRequest.onHeadersReceived.addListener(e=>{void response(e).catch(()=>note(e.tabId,'storageError'));},{urls:['https://*.googlevideo.com/*']},['responseHeaders']);
  api.webRequest.onErrorOccurred?.addListener(e=>pending.delete(e.requestId),{urls:['https://*.googlevideo.com/*']});
  api.tabs.onRemoved.addListener(id=>void clear(id));api.tabs.onUpdated.addListener((id,change)=>{if(change.url)void clear(id);});
 }
 return {install,before,response,session,clear,diagnostics:tab=>({...counts.get(tab)}),endpoint,body};
})();
if(typeof module!=='undefined')module.exports=UdmStreamingCapture;
