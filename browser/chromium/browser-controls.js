(function(root){
 'use strict';
 function readLinks(selectionOnly){
  const selection=getSelection();if(selectionOnly&&(!selection||selection.isCollapsed))return [];
  const images=new Set([...document.images].flatMap(e=>[e.currentSrc,e.src]).filter(Boolean));
  return [...document.querySelectorAll('a[href],area[href],img[src],img[srcset]')].filter(e=>!selectionOnly||selection.containsNode(e,true)).slice(0,500).map(e=>{const url=e.href||e.currentSrc||e.src;const linkText=e.localName==='img'?'':(e.innerText||e.alt||'').trim().slice(0,1000);return {url,name:e.download||linkText.slice(0,160)||e.title||e.alt||'',linkText,imageOnPage:images.has(url),page:location.href};});
 }
 async function readMetadata(url,page,credentials){
  if(location.href!==page)return {error:'Source page changed'};
  const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),8000);
  try{
   const response=await fetch(url,{method:'HEAD',credentials:credentials?'include':'omit',redirect:'follow',signal:controller.signal});
   if(location.href!==page)return {error:'Source page changed'};
   if(!response.ok)return {error:'HTTP '+response.status};
   const raw=response.headers.get('content-length'),encoding=response.headers.get('content-encoding');
   const size=raw&&/^\d+$/.test(raw)&&(!encoding||encoding.toLowerCase()==='identity')?Number(raw):null;
   return {size:Number.isSafeInteger(size)&&size>=0?size:null,type:(response.headers.get('content-type')||'').split(';')[0].trim().slice(0,150),source:'HEAD'};
  }catch{return {error:'Metadata unavailable'};}finally{clearTimeout(timer);}
 }
 function create(api,handoff,report,observedMetadata){
  const disabled=new Set(),batches=new Map(),activeBatches=new Set(),indicators=new Map();let persistence=Promise.resolve();
  function updateIndicator(tabId){
   if(!Number.isInteger(tabId)||tabId<0)return;
   // Cosmetic work is serialized per tab, but never holds up capture or handoff.
   const pending=(indicators.get(tabId)||Promise.resolve()).catch(()=>{}).then(async()=>{
    const off=disabled.has(tabId);
    await Promise.allSettled([
     Promise.resolve().then(()=>api.action.setBadgeText({tabId,text:off?'OFF':null})),
     Promise.resolve().then(()=>api.action.setTitle({tabId,title:off?'UDM — capture disabled on this tab':(api.runtime.getManifest().action?.default_title||'UDM Browser Integration')}))
    ]);
   });
   indicators.set(tabId,pending);
   void pending.finally(()=>{if(indicators.get(tabId)===pending)indicators.delete(tabId);}).catch(()=>{});
  }
  const ready=Promise.all([api.storage.session.get('disabledTabs'),api.storage.session.get('linkBatches')]).then(([tabs,stored])=>{
   for(const id of tabs.disabledTabs||[])if(Number.isInteger(id)&&id>=0)disabled.add(id);
   for(const id of disabled)updateIndicator(id);
   for(const entry of (stored.linkBatches||[]).slice(-8)){
    if(!Array.isArray(entry)||entry.length!==2)continue;const [token,batch]=entry;
    if(typeof token==='string'&&batch&&Number.isFinite(batch.time)&&Date.now()-batch.time<600000&&Array.isArray(batch.rows)&&batch.rows.length<=500)batches.set(token,batch);
   }
  });
  function saveBatches(){const snapshot=structuredClone([...batches]);const pending=persistence.catch(()=>{}).then(()=>api.storage.session.set({linkBatches:snapshot}));persistence=pending;return pending;}
  const isDisabled=async id=>{await ready;return disabled.has(id);};
  async function toggle(tab){await ready;if(!Number.isInteger(tab?.id))throw Error('Open a web page first.');if(disabled.has(tab.id))disabled.delete(tab.id);else disabled.add(tab.id);
   updateIndicator(tab.id);
   await api.storage.session.set({disabledTabs:[...disabled]});await api.tabs.sendMessage(tab.id,{action:'tab-integration',disabled:disabled.has(tab.id)}).catch(()=>{});
   await api.contextMenus.update('udm-toggle-tab',{title:disabled.has(tab.id)?'Enable UDM on this tab':'Disable UDM on this tab'}).catch(()=>{});
  }
  async function menus(policy={}){await ready;const active=api.tabs.query?await api.tabs.query({active:true,currentWindow:true}).catch(()=>[]):[];const tabId=active[0]?.id;for(const item of [{id:'udm-all-links',title:'Download all links with UDM',contexts:['page','frame']},{id:'udm-selected-links',title:'Download selected links with UDM',contexts:['selection']},{id:'udm-toggle-tab',title:disabled.has(tabId)?'Enable UDM on this tab':'Disable UDM on this tab',contexts:['page','frame']}])if(item.id==='udm-toggle-tab'||(item.id==='udm-all-links'?policy.All!==false:policy.Link!==false))api.contextMenus.create(item);}
  async function collect(tab,selectionOnly){
   if(!Number.isInteger(tab?.id)||tab.id<0||tab.incognito||!/^https?:/.test(tab.url||''))throw Error('Open a regular web page first.');
   if(await isDisabled(tab.id))throw Error('UDM is disabled on this tab.');
   const frames=await api.scripting.executeScript({target:{tabId:tab.id,allFrames:true},func:readLinks,args:[selectionOnly]});
   const unique=new Map();
   for(const frame of frames)for(const link of frame.result||[]){try{const u=new URL(link.url);if(!/^https?:$/.test(u.protocol)||u.username||u.password||u.href.length>16000)continue;if(!unique.has(u.href))unique.set(u.href,{url:u.href,name:link.name,linkText:link.linkText||'',imageOnPage:!!link.imageOnPage,page:link.page,tabId:tab.id,frameId:frame.frameId,documentId:frame.documentId||''});else{const prior=unique.get(u.href);prior.imageOnPage=prior.imageOnPage||!!link.imageOnPage;if(!prior.linkText&&link.linkText)prior.linkText=link.linkText;}}catch{}}
   if(!unique.size)throw Error('No downloadable links were found in this selection.');
   await openBatch([...unique.values()],selectionOnly);
  }
  async function openBatch(rows,presetSelection=false){
   await ready;
   const token=crypto.randomUUID();for(const [key,row] of batches)if(!activeBatches.has(key)&&Date.now()-row.time>600000)batches.delete(key);if(batches.size>=8){const oldest=[...batches.keys()].find(key=>!activeBatches.has(key));if(!oldest)throw Error('Wait for a current link submission to finish.');batches.delete(oldest);}
   const batch={time:Date.now(),rows:rows.slice(0,500),busy:false,presetSelection};
   if(new TextEncoder().encode(JSON.stringify(batch)).length>512*1024)throw Error('This link list is too large. Select fewer links.');
   batches.set(token,batch);
   try{await saveBatches();await api.tabs.create({url:api.runtime.getURL('links.html')+'#'+token});}catch(e){batches.delete(token);void saveBatches().catch(()=>{});throw e;}
  }
  async function review(value,sender){
   if(!Number.isInteger(sender.tab?.id)||sender.tab.incognito||value.page!==sender.url||!/^https?:/.test(sender.url||''))throw Error('Open a regular web page to review selected links.');
   if(await isDisabled(sender.tab.id))throw Error('UDM is disabled on this tab.');
   if(!Array.isArray(value.links)||!value.links.length||value.links.length>100)throw Error('Select between 1 and 100 links.');
   const unique=new Map();for(const link of value.links){if(!link||typeof link.url!=='string'||link.url.length>16000||typeof link.title!=='string'||link.title.length>1000)throw Error('Invalid selected link.');const url=new URL(link.url);if(!/^https?:$/.test(url.protocol)||url.username||url.password)throw Error('Only HTTP and HTTPS links without embedded credentials are supported.');if(!unique.has(url.href))unique.set(url.href,{url:url.href,name:link.title.slice(0,160),linkText:link.title,page:sender.url,tabId:sender.tab.id,frameId:sender.frameId??0,documentId:sender.documentId||''});}
   await openBatch([...unique.values()],true);return {ok:true,count:unique.size};
  }
  function handle(info,tab){
   const actions={'udm-toggle-tab':()=>toggle(tab),'udm-all-links':()=>collect(tab,false),'udm-selected-links':()=>collect(tab,true)};
   if(!actions[info.menuItemId])return false;actions[info.menuItemId]().catch(e=>report(e.message));return true;
  }
  function progress(token,batch){const receipt=batch.handoff||{};return {phase:receipt.phase==='complete'?'complete':receipt.phase==='failed'?'failed':activeBatches.has(token)?'sending':batch.busy?'interrupted':'ready',count:Number.isInteger(receipt.count)?receipt.count:0,total:Number.isInteger(receipt.total)?receipt.total:0};}
  async function message(value,sender){
   if(sender.url?.split('#')[0]!==api.runtime.getURL('links.html'))throw Error('Open the UDM link-selection window.');
   await ready;
   const batch=batches.get(value.token);if(!batch||(!activeBatches.has(value.token)&&Date.now()-batch.time>600000))throw Error('This selection expired. Collect the links again.');
   if(value.action==='batch-progress')return {ok:true,...progress(value.token,batch)};
   if(value.action==='batch-list'&&batch.busy)return {ok:true,submitted:true,progress:progress(value.token,batch),links:[]};
   if(batch.busy)throw Error('This selection was already submitted. Check UDM before collecting a fresh list; an interrupted request may have been accepted.');
   if(value.action==='batch-list')return {ok:true,presetSelection:!!batch.presetSelection,links:batch.rows.map((r,index)=>({index,url:r.url,name:r.name,linkText:r.linkText||'',imageOnPage:!!r.imageOnPage}))};
   if(value.action==='batch-metadata'){
    const row=Number.isInteger(value.index)?batch.rows[value.index]:null;if(!row)throw Error('Invalid link selection.');
    if(row.metadata)return {ok:true,...row.metadata};
    const observed=observedMetadata?.(row);if(observed){row.metadata=observed;return {ok:true,...observed};}
    const settings=(await api.storage.local.get('settings')).settings||{};
    const credentials=!!settings.cookies&&await api.permissions.contains({permissions:['cookies'],origins:['http://*/*','https://*/*']});
    const target=row.documentId?{tabId:row.tabId,documentIds:[row.documentId]}:{tabId:row.tabId,frameIds:[row.frameId]};
    const results=await api.scripting.executeScript({target,func:readMetadata,args:[row.url,row.page,credentials]});
    row.metadata=results[0]?.result||{error:'Metadata unavailable'};return {ok:true,...row.metadata};
   }
   if(value.action!=='batch-download')throw Error('Unknown link-selection command.');
   const indexes=[...new Set(value.indexes||[])];if(!indexes.length||indexes.length>500||indexes.some(i=>!Number.isInteger(i)||!batch.rows[i]))throw Error('Select between 1 and 500 links.');
   const filenames=value.filenames??{};if(!filenames||typeof filenames!=='object'||Array.isArray(filenames))throw Error('Invalid file names.');
   const names=new Map();for(const index of indexes){const name=filenames[index]??'';if(typeof name!=='string'||name.length>240||/[<>:"/\\|?*\x00-\x1f]/.test(name)||/[. ]$/.test(name)||/^(?:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)/i.test(name))throw Error('Enter a valid file name without folders.');names.set(index,name);}
   const properties=value.properties??{};if(!properties||typeof properties!=='object'||Array.isArray(properties))throw Error('Invalid import properties.');
   const edits=new Map();for(const index of indexes){const edit=properties[index]??{};if(!edit||typeof edit!=='object'||Array.isArray(edit)||Object.keys(edit).some(key=>!['folder','description','referer'].includes(key)))throw Error('Invalid import properties.');for(const field of Object.values(edit))if(typeof field!=='string'||new TextEncoder().encode(field).length>4096||field.includes('\0'))throw Error('Import properties are too long or invalid.');if(Object.hasOwn(edit,'referer')&&edit.referer){const page=new URL(edit.referer);if(!/^https?:$/.test(page.protocol)||page.username||page.password||/[\r\n]/.test(edit.referer))throw Error('Enter an HTTP or HTTPS Referer without credentials.');}edits.set(index,edit);}
   const authorizations=value.authorizations??{};if(!authorizations||typeof authorizations!=='object'||Array.isArray(authorizations))throw Error('Invalid import authorization.');
   const logins=new Map();for(const index of indexes){if(!Object.hasOwn(authorizations,index))continue;const login=authorizations[index];if(!login||typeof login!=='object'||Array.isArray(login)||Object.keys(login).some(key=>!['username','password'].includes(key))||typeof login.username!=='string'||!login.username||login.username.length>256||/[:\x00-\x1f\x7f]/.test(login.username)||typeof login.password!=='string'||login.password.length>1024||/[\x00-\x1f\x7f]/.test(login.password))throw Error('Enter a valid authorization user name and password.');logins.set(index,{username:login.username,password:login.password});}
   batch.busy=true;batch.handoff={phase:'sending',count:0,total:indexes.length};batch.time=Date.now();activeBatches.add(value.token);let count=0;
   // Persist before any handoff. A worker crash must not make a submitted list reusable.
   try{await saveBatches();}catch(e){batch.busy=false;delete batch.handoff;activeBatches.delete(value.token);throw Error('Could not save this selection. No downloads were sent. '+e.message);}
   try{for(const index of indexes){const r=batch.rows[index];await handoff({url:r.url,filename:names.get(index),importProperties:edits.get(index),authorization:logins.get(index),referrer:r.page,tabId:r.tabId,frameId:r.frameId,documentId:r.documentId,downloadLater:!!value.downloadLater});count++;batch.handoff.count=count;batch.time=Date.now();await saveBatches();}batch.handoff.phase='complete';batch.time=Date.now();await saveBatches();return {ok:true,count};}
   catch(e){batch.handoff={phase:'failed',count,total:indexes.length};batch.time=Date.now();await saveBatches().catch(()=>{});return {ok:false,count,error:count+' links were added. '+e.message+' Check UDM before collecting a fresh list; the last request may also have been accepted.'};}finally{activeBatches.delete(value.token);logins.clear();}
  }
  api.tabs.onRemoved.addListener(id=>{void ready.then(()=>{disabled.delete(id);return api.storage.session.set({disabledTabs:[...disabled]});}).catch(()=>{});});
  api.tabs.onActivated?.addListener(async({tabId})=>{await ready;updateIndicator(tabId);api.contextMenus.update('udm-toggle-tab',{title:disabled.has(tabId)?'Enable UDM on this tab':'Disable UDM on this tab'}).catch(()=>{});});
  api.tabs.onUpdated?.addListener((tabId,change)=>{if(change.status==='loading')void ready.then(()=>updateIndicator(tabId)).catch(()=>{});});
  return {menus,handle,message,isDisabled,collect,toggle,review};
 }
 root.UdmBrowserControls={create};
})(globalThis);
