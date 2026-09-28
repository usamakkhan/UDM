/* Document-aware cleanup. Browser URL notifications are not network barriers. */
(function(root){
 'use strict';
 function create(api){
  const supported=!!api.webNavigation?.onCommitted,subscribers=[],frames=new Map(),retired=new Map(),epochs=new Map(),closed=new Set(),TTL=180000,KEY='udm-navigation';
  let sequence=Promise.resolve();
  const key=(tab,frame)=>tab+':'+frame,docKey=(tab,doc)=>tab+':'+doc;
  const ready=(async()=>{if(!supported)return;try{const saved=(await api.storage.session.get(KEY))[KEY];for(const f of (saved?.frames||[]).slice(-512))if(Number.isInteger(f.tabId)&&Number.isInteger(f.frameId)&&typeof f.documentId==='string'&&Number.isFinite(f.time))frames.set(key(f.tabId,f.frameId),f);for(const r of (saved?.retired||[]).slice(-2048))if(Number.isInteger(r.tabId)&&typeof r.documentId==='string'&&Number.isFinite(r.time)&&Date.now()-r.time<TTL)retired.set(docKey(r.tabId,r.documentId),r);}catch{}})();
  // A page can play for hours. Age out retired IDs, not still-current frames.
  function prune(){const now=Date.now();for(const [k,r] of retired)if(now-r.time>TTL)retired.delete(k);while(frames.size>512)frames.delete(frames.keys().next().value);while(retired.size>2048)retired.delete(retired.keys().next().value);while(epochs.size>1024)epochs.delete(epochs.keys().next().value);while(closed.size>256)closed.delete(closed.values().next().value);}
  function children(tab,frame){const affected=new Set([frame]);let changed=true;while(changed){changed=false;for(const f of frames.values())if(f.tabId===tab&&affected.has(f.parentFrameId)&&!affected.has(f.frameId)){affected.add(f.frameId);changed=true;}}return affected;}
  function token(tab,frame){return (epochs.get(key(tab,-1))||0)+':'+(epochs.get(key(tab,frame))||0);}
  function valid(record,stamp){return !closed.has(record.tabId)&&(!record.documentId||!retired.has(docKey(record.tabId,record.documentId)))&&(stamp===undefined||stamp===token(record.tabId,record.frameId??0));}
  function keep(record,event){if(!valid(record))return false;if(record.tabId!==event.tabId)return true;if(event.kind==='removed')return false;if(event.invalidate&&(event.frameId===0||event.affected.has(record.frameId??0)))return false;return true;}
  async function dispatch(kind,details){
   const tabId=details.tabId;if(!Number.isInteger(tabId)||tabId<0)return;
   await ready;prune();
   if(kind!=='removed'){try{const tab=await api.tabs.get(tabId);if(tab.incognito)return;}catch{return;}}
   const frameId=details.frameId??0,affected=children(tabId,frameId),event={...details,tabId,frameId,kind,affected,invalidate:false};
   if(kind==='removed'){
    closed.add(tabId);epochs.set(key(tabId,-1),(epochs.get(key(tabId,-1))||0)+1);for(const [k,f] of frames)if(f.tabId===tabId)frames.delete(k);
   }else if(kind==='commit'&&details.documentId){
    const prior=frames.get(key(tabId,frameId));
    // Only a document previously committed in this frame is proven obsolete.
    // An unknown ID may be a new response delivered before its commit event.
    if(prior?.documentId&&prior.documentId!==details.documentId){
     for(const [k,f] of frames)if(f.tabId===tabId&&affected.has(f.frameId)){
      if(f.documentId)retired.set(docKey(tabId,f.documentId),{tabId,documentId:f.documentId,time:Date.now()});frames.delete(k);
     }
    }
    retired.delete(docKey(tabId,details.documentId));
    frames.set(key(tabId,frameId),{tabId,frameId,documentId:details.documentId,parentFrameId:details.parentFrameId??-1,time:Date.now()});
   }else{
    // Same-document routes and older browsers lack proof that pre-notification
    // captures belong to the new page. Keep their invalidation conservative.
    const prior=frames.get(key(tabId,frameId));
    if(details.documentId&&prior?.documentId&&details.documentId!==prior.documentId)return;
    event.invalidate=true;
    if(frameId===0)epochs.set(key(tabId,-1),(epochs.get(key(tabId,-1))||0)+1);
    else for(const frame of affected)epochs.set(key(tabId,frame),(epochs.get(key(tabId,frame))||0)+1);
    if(kind==='commit')frames.set(key(tabId,frameId),{tabId,frameId,documentId:'',parentFrameId:details.parentFrameId??-1,time:Date.now()});
   }
   for(const callback of subscribers)try{await callback(event);}catch{}
   prune();try{await api.storage.session.set({[KEY]:{frames:[...frames.values()],retired:[...retired.values()]}});}catch{}
  }
  function notify(kind,details){const next=sequence.catch(()=>{}).then(()=>dispatch(kind,details));sequence=next;return next;}
  function install(){if(!supported)return;api.webNavigation.onCommitted.addListener(e=>void notify('commit',e));api.webNavigation.onHistoryStateUpdated?.addListener(e=>void notify('history',e));api.webNavigation.onReferenceFragmentUpdated?.addListener(e=>void notify('history',e));api.tabs.onRemoved.addListener(tabId=>void notify('removed',{tabId}));api.webNavigation.onTabReplaced?.addListener(e=>void notify('removed',{tabId:e.replacedTabId}));}
  return {supported,ready,install,subscribe:callback=>subscribers.push(callback),token,valid,keep,notify,settled:()=>sequence};
 }
 root.UdmNavigation={create};if(typeof module==='object'&&module.exports)module.exports=root.UdmNavigation;
})(globalThis);
