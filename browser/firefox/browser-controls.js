(function(root){
 'use strict';
 function readLinks(selectionOnly){
  const selection=getSelection();if(selectionOnly&&(!selection||selection.isCollapsed))return [];
  return [...document.querySelectorAll('a[href],area[href]')].filter(e=>!selectionOnly||selection.containsNode(e,true)).slice(0,500).map(e=>({url:e.href,name:e.download||e.innerText?.trim().slice(0,160)||e.title||'',page:location.href}));
 }
 function create(api,handoff,report){
  const disabled=new Set(),batches=new Map();let persistence=Promise.resolve();
  const ready=Promise.all([api.storage.session.get('disabledTabs'),api.storage.session.get('linkBatches')]).then(([tabs,stored])=>{
   for(const id of tabs.disabledTabs||[])if(Number.isInteger(id))disabled.add(id);
   for(const entry of (stored.linkBatches||[]).slice(-8)){
    if(!Array.isArray(entry)||entry.length!==2)continue;const [token,batch]=entry;
    if(typeof token==='string'&&batch&&Number.isFinite(batch.time)&&Date.now()-batch.time<600000&&Array.isArray(batch.rows)&&batch.rows.length<=500)batches.set(token,batch);
   }
  });
  function saveBatches(){const snapshot=structuredClone([...batches]);const pending=persistence.catch(()=>{}).then(()=>api.storage.session.set({linkBatches:snapshot}));persistence=pending;return pending;}
  const isDisabled=async id=>{await ready;return disabled.has(id);};
  async function toggle(tab){await ready;if(!Number.isInteger(tab?.id))throw Error('Open a web page first.');if(disabled.has(tab.id))disabled.delete(tab.id);else disabled.add(tab.id);
   await api.storage.session.set({disabledTabs:[...disabled]});await api.tabs.sendMessage(tab.id,{action:'tab-integration',disabled:disabled.has(tab.id)}).catch(()=>{});
   await api.contextMenus.update('udm-toggle-tab',{title:disabled.has(tab.id)?'Enable UDM on this tab':'Disable UDM on this tab'}).catch(()=>{});
  }
  async function menus(policy={}){await ready;const active=api.tabs.query?await api.tabs.query({active:true,currentWindow:true}).catch(()=>[]):[];const tabId=active[0]?.id;for(const item of [{id:'udm-all-links',title:'Download all links with UDM',contexts:['page','frame']},{id:'udm-selected-links',title:'Download selected links with UDM',contexts:['selection']},{id:'udm-toggle-tab',title:disabled.has(tabId)?'Enable UDM on this tab':'Disable UDM on this tab',contexts:['page','frame']}])if(item.id==='udm-toggle-tab'||(item.id==='udm-all-links'?policy.All!==false:policy.Link!==false))api.contextMenus.create(item);}
  async function collect(tab,selectionOnly){
   if(!tab?.id||tab.incognito||!/^https?:/.test(tab.url||''))throw Error('Open a regular web page first.');
   if(await isDisabled(tab.id))throw Error('UDM is disabled on this tab.');
   const frames=await api.scripting.executeScript({target:{tabId:tab.id,allFrames:true},func:readLinks,args:[selectionOnly]});
   const unique=new Map();
   for(const frame of frames)for(const link of frame.result||[]){try{const u=new URL(link.url);if(!/^https?:$/.test(u.protocol)||u.username||u.password||u.href.length>16000)continue;if(!unique.has(u.href))unique.set(u.href,{url:u.href,name:link.name,page:link.page,tabId:tab.id,frameId:frame.frameId,documentId:frame.documentId||''});}catch{}}
   if(!unique.size)throw Error('No downloadable links were found in this selection.');
   const token=crypto.randomUUID();for(const [key,row] of batches)if(Date.now()-row.time>600000)batches.delete(key);if(batches.size>=8)batches.delete(batches.keys().next().value);
   const batch={time:Date.now(),rows:[...unique.values()].slice(0,500),busy:false};
   if(new TextEncoder().encode(JSON.stringify(batch)).length>512*1024)throw Error('This link list is too large. Select fewer links.');
   batches.set(token,batch);
   try{await saveBatches();await api.tabs.create({url:api.runtime.getURL('links.html')+'#'+token});}catch(e){batches.delete(token);void saveBatches().catch(()=>{});throw e;}
  }
  function handle(info,tab){
   const actions={'udm-toggle-tab':()=>toggle(tab),'udm-all-links':()=>collect(tab,false),'udm-selected-links':()=>collect(tab,true)};
   if(!actions[info.menuItemId])return false;actions[info.menuItemId]().catch(e=>report(e.message));return true;
  }
  async function message(value,sender){
   if(sender.url?.split('#')[0]!==api.runtime.getURL('links.html'))throw Error('Open the UDM link-selection window.');
   await ready;
   const batch=batches.get(value.token);if(!batch||Date.now()-batch.time>600000)throw Error('This selection expired. Collect the links again.');
   if(batch.busy)throw Error('This selection was already submitted. Check UDM before collecting a fresh list; an interrupted request may have been accepted.');
   if(value.action==='batch-list')return {ok:true,links:batch.rows.map((r,index)=>({index,url:r.url,name:r.name}))};
   if(value.action!=='batch-download')throw Error('Unknown link-selection command.');
   const indexes=[...new Set(value.indexes||[])];if(!indexes.length||indexes.length>100||indexes.some(i=>!Number.isInteger(i)||!batch.rows[i]))throw Error('Select between 1 and 100 links.');
   batch.busy=true;let count=0;
   // Persist before any handoff. A worker crash must not make a submitted list reusable.
   try{await saveBatches();}catch(e){batch.busy=false;throw Error('Could not save this selection. No downloads were sent. '+e.message);}
   try{for(const index of indexes){const r=batch.rows[index];await handoff({url:r.url,referrer:r.page,tabId:r.tabId,frameId:r.frameId,documentId:r.documentId,downloadLater:!!value.downloadLater});count++;}batches.delete(value.token);void saveBatches().catch(()=>{});return {ok:true,count};}
   catch(e){batches.delete(value.token);void saveBatches().catch(()=>{});return {ok:false,count,error:count+' links were added. '+e.message+' Check UDM before collecting a fresh list; the last request may also have been accepted.'};}
  }
  api.tabs.onRemoved.addListener(id=>{void ready.then(()=>{disabled.delete(id);return api.storage.session.set({disabledTabs:[...disabled]});}).catch(()=>{});});
  api.tabs.onActivated?.addListener(async({tabId})=>{await ready;api.contextMenus.update('udm-toggle-tab',{title:disabled.has(tabId)?'Enable UDM on this tab':'Disable UDM on this tab'}).catch(()=>{});});
  return {menus,handle,message,isDisabled,collect,toggle};
 }
 root.UdmBrowserControls={create};
})(globalThis);
