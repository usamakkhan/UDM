/* Short-lived, document-bound keyboard leases. No keys, media URLs or tokens persist. */
(function(root){
 'use strict';
 function readState(token){
  const videos=[],roots=[document];for(let i=0;i<roots.length&&i<40;i++){videos.push(...roots[i].querySelectorAll('video'));for(const e of Array.from(roots[i].querySelectorAll('*')).slice(0,3000))if(e.shadowRoot)roots.push(e.shadowRoot);}
  const video=videos.find(v=>v.getAttribute('data-udm-player')===token);let active=document.activeElement;
  for(let depth=0;active?.shadowRoot?.activeElement&&depth<40;depth++)active=active.shadowRoot.activeElement;
  const base={page:location.href,timeOrigin:performance.timeOrigin,focused:document.hasFocus(),visible:document.visibilityState==='visible',editing:!!(active?.isContentEditable||active?.closest('input,textarea,select,[contenteditable]:not([contenteditable="false"])')||active?.shadowRoot?.activeElement)};
  if(!video)return base;const r=video.getBoundingClientRect();let ad=false;for(let e=video;e;e=e.parentElement||e.getRootNode()?.host)if(e.matches('.ad-showing,.ad-interrupting,[data-ad-playing="true"]'))ad=true;
  return {...base,player:{url:video.currentSrc||'',playing:!video.paused&&!video.ended,duration:video.duration,visible:r.width>=120&&r.height>=70&&getComputedStyle(video).visibility==='visible',encrypted:!!video.mediaKeys||video.hasAttribute('data-udm-encrypted'),ad,stamp:video.getAttribute('data-udm-epoch'),title:document.title}};
 }
 function create(api,settings,disabled,requestContext,handoff){
  const leases=new Map(),submitted=new Map(),requests=new Map();
  const key=sender=>sender.tab.id+':'+(sender.frameId??0),now=()=>Date.now();
  const prune=()=>{for(const [k,v] of requests)if(now()-v.time>30000)requests.delete(k);while(requests.size>128)requests.delete(requests.keys().next().value);for(const [k,v] of leases)if(now()-v.time>1500)leases.delete(k);while(submitted.size>128)submitted.delete(submitted.keys().next().value);};
  async function current(sender,token){
   if(sender.id!==api.runtime.id||!Number.isInteger(sender.tab?.id)||sender.tab.incognito||!/^https?:\/\//.test(sender.url||'')||await disabled(sender.tab.id))throw Error('Capture is unavailable in this tab.');
   const tab=await api.tabs.get(sender.tab.id),frame=sender.frameId??0;
   if(tab.incognito||(frame===0&&tab.url!==sender.url))throw Error('The source page changed.');
   const row=(await api.scripting.executeScript({target:{tabId:sender.tab.id,frameIds:[frame]},func:readState,args:[token||'']})).find(r=>r.frameId===frame);
   if(!row?.result||row.result.page!==sender.url||(sender.documentId&&row.documentId!==sender.documentId))throw Error('The source document changed.');
   const prefs=await settings();if(!prefs.capture||prefs.captureAllowed===false||UdmMedia.policy.blocked(sender.url,prefs)||UdmMedia.policy.blocked(tab.url,prefs))throw Error('Browser capture is disabled.');
   return {state:row.result,documentId:row.documentId||'',prefs};
  }
  async function update(message,sender){
   if(!Number.isInteger(sender.tab?.id))return {ok:false};prune();const id=key(sender);
   // Release does not need asynchronous validation; revoke immediately.
   if(!message.keys){leases.delete(id);return {ok:true};}
   const keys=message.keys;if(Object.keys(keys).some(k=>!['altKey','ctrlKey','shiftKey','insertKey','deleteKey','metaKey'].includes(k))||Object.values(keys).some(v=>typeof v!=='boolean'))throw Error('Invalid keyboard state.');
   const entry={sender,keys,time:now(),ready:null};leases.set(id,entry);if(leases.size>128)leases.delete(leases.keys().next().value);
   entry.ready=current(sender).then(ctx=>{
    if(!ctx.state.focused||!ctx.state.visible||ctx.state.editing)throw Error('Capture keys require the active page.');
    entry.documentId=ctx.documentId;entry.timeOrigin=ctx.state.timeOrigin;return ctx;
   });
   try{await entry.ready;return {ok:true};}catch(e){if(leases.get(id)===entry)leases.delete(id);throw e;}
  }
  function observe(event){
   prune();if(event.tabId<0||event.incognito||!/^https?:\/\//.test(event.url||''))return;
   const id=event.tabId+':'+(event.frameId??0),previous=requests.get(event.requestId),entry=leases.get(id);
   if(previous){if(previous.tabId===event.tabId&&previous.frameId===(event.frameId??0)&&previous.documentId===(event.documentId||''))previous.url=event.url;else requests.delete(event.requestId);return;}
   if(!entry||now()-entry.time>1500)return;
   requests.set(event.requestId,{entry,time:now(),url:event.url,tabId:event.tabId,frameId:event.frameId??0,documentId:event.documentId||''});
  }
  api.webRequest?.onBeforeRequest?.addListener(observe,{urls:['http://*/*','https://*/*']});
  api.webRequest?.onErrorOccurred?.addListener(event=>requests.delete(event.requestId),{urls:['http://*/*','https://*/*']});
  async function intent(record,download=false){
   if(!record||!Number.isInteger(record.tabId)||!Number.isInteger(record.frameId))return '';
   prune();const id=record.tabId+':'+record.frameId,captured=download?requests.get(record.requestId):null;
   const frozen=captured&&captured.url===record.url&&captured.tabId===record.tabId&&captured.frameId===record.frameId&&captured.documentId===(record.documentId||'');
   const entry=frozen?captured.entry:leases.get(id);if(!entry)return '';
   try{await entry.ready;const ctx=await current(entry.sender);
    if((!frozen&&(leases.get(id)!==entry||now()-entry.time>1500||!ctx.state.focused||!ctx.state.visible||ctx.state.editing))||ctx.documentId!==entry.documentId||ctx.state.timeOrigin!==entry.timeOrigin||record.documentId&&record.documentId!==ctx.documentId)return '';
    const value=UdmMedia.policy.intent(entry.keys,ctx.prefs);return value==='force'&&ctx.prefs.forceClick!==false?'':value;
   }catch{return '';}
  }
  async function player(message,sender){
   if(typeof message.token!=='string'||!/^[a-z0-9-]{1,80}$/i.test(message.token))throw Error('Invalid player identity.');
   const ctx=await current(sender,message.token),p=ctx.state.player;
   if(!p||!p.playing||!Number.isFinite(p.duration)||p.duration<=0||!p.visible||p.encrypted||p.ad||!ctx.state.visible)return {ok:true,skipped:true};
   let url;try{url=new URL(p.url);}catch{return {ok:true,skipped:true};}
   // Only an actual unprotected whole-file source. Adaptive fragments and blob players stay with their format panel.
   if(!/^https?:$/.test(url.protocol)||url.username||url.password||/(^|\.)googlevideo\.com$/.test(url.hostname)||UdmMedia.kind(url.href)!=='direct'||['range','sq','ump','sabr','segment','fragment'].some(k=>url.searchParams.has(k))||UdmMedia.policy.blocked(url.href,ctx.prefs))return {ok:true,skipped:true};
   if(ctx.prefs.panelEnabled===false||UdmMedia.policy.panelBlocked(sender.url,ctx.prefs))return {ok:true,skipped:true};
   const item={url:url.href,referrer:sender.url,tabId:sender.tab.id,frameId:sender.frameId??0,documentId:ctx.documentId};
   const record=requestContext?.resolve(item,false);if(!record||record.method!=='GET'||!/^video\//i.test(record.mime||'')||record.pending||record.proxy?.unsupported)return {ok:true,skipped:true};
   const force=await intent(record);if(force==='bypass'||(force!=='force'&&!ctx.prefs.captureWebPlayers))return {ok:true,skipped:true};
   if(!UdmMedia.policy.panelAllowed({url:url.href,size:record.size},ctx.prefs))return {ok:true,skipped:true};
   const identity=JSON.stringify([item.tabId,item.frameId,item.documentId||ctx.state.timeOrigin,p.stamp,url.href]);
   if(submitted.has(identity))return {ok:true,skipped:true};
   const final=await current(sender,message.token);
   if(final.documentId!==ctx.documentId||final.state.timeOrigin!==ctx.state.timeOrigin||final.state.player?.url!==p.url||final.state.player?.stamp!==p.stamp||final.state.player?.encrypted||final.state.player?.ad||!final.state.player?.playing||!final.state.visible)return {ok:true,skipped:true};
   if(final.prefs.panelEnabled===false||UdmMedia.policy.panelBlocked(sender.url,final.prefs)||UdmMedia.policy.blocked(url.href,final.prefs)||!UdmMedia.policy.panelAllowed({url:url.href,size:record.size},final.prefs))return {ok:true,skipped:true};
   const finalIntent=await intent(record);if(finalIntent==='bypass'||(finalIntent!=='force'&&!final.prefs.captureWebPlayers))return {ok:true,skipped:true};
   if(submitted.has(identity))return {ok:true,skipped:true};
   submitted.set(identity,{tabId:sender.tab.id});prune();
   // Retain uncertain acknowledgements: repeating an automated handoff can duplicate jobs.
   return handoff(item);
  }
  function clear(tabId){for(const [k,v] of requests)if(v.tabId===tabId)requests.delete(k);for(const [k,v] of leases)if(v.sender.tab.id===tabId)leases.delete(k);for(const [k,v] of submitted)if(v.tabId===tabId)submitted.delete(k);}
  return {update,intent,player,clear,observe};
 }
 root.UdmKeyCapture={create};
})(globalThis);
