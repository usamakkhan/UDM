/* Scope ordinary browser handoffs to the observed request and its cookie store. */
(function(root){
 'use strict';
 const bytes=value=>new TextEncoder().encode(value).length;
 const wire=cookies=>cookies.map(c=>c.name+'='+c.value).join('; ');
 function select(values,url,store,header,key){
  const target=new URL(url),eligible=values.filter(c=>{
   const domain=String(c.domain||'').replace(/^\./,'').toLowerCase(),path=c.path||'/';
   if(c.storeId!==store||c.firstPartyDomain||!(target.hostname===domain||(!c.hostOnly&&target.hostname.endsWith('.'+domain))))return false;
   if(c.secure&&target.protocol!=='https:'||c.expirationDate!==undefined&&c.expirationDate<=Date.now()/1000)return false;
   if(target.pathname!==path&&(!target.pathname.startsWith(path)||!path.endsWith('/')&&target.pathname[path.length]!=='/'))return false;
   if(c.partitionKey&&(!key||c.partitionKey.topLevelSite!==key.topLevelSite||!!c.partitionKey.hasCrossSiteAncestor!==!!key.hasCrossSiteAncestor))return false;
   return true;
  });
  let selected;
  if(header!==undefined){
   if(typeof header!=='string'||bytes(header)>16384)return null;
   const remaining=[...eligible];selected=[];
   for(const pair of header?header.split('; '):[]){
    const choices=remaining.filter(c=>c.name+'='+c.value===pair);
    // Identical values in different paths/partitions cannot establish their scope.
    if(choices.length!==1)return null;
    selected.push(choices[0]);remaining.splice(remaining.indexOf(choices[0]),1);
   }
  }else selected=eligible.filter(c=>!c.partitionKey);
  selected.sort((a,b)=>b.path.length-a.path.length);
  if(selected.length>512||bytes(wire(selected))>16384)throw Error('This download has too much session data to transfer.');
  if(header!==undefined&&wire(selected)!==header)return null;
  return selected.map(c=>({name:c.name,value:c.value,domain:c.domain,path:c.path,secure:c.secure,hostOnly:c.hostOnly,...(c.expirationDate!==undefined?{expirationDate:c.expirationDate}:{}),...(c.partitionKey?{partitioned:true}:{})}));
 }
 function create(api,navigation){
  async function findStore(tabId,tab){
   const stores=await api.cookies.getAllCookieStores(),matching=stores.filter(s=>s.tabIds.includes(tabId));
   const store=tab.cookieStoreId||(matching.length===1?matching[0].id:'');
   return store&&matching.some(s=>s.id===store)?store:'';
  }
  const sameTab=(tab,current)=>current.url===tab.url&&current.cookieStoreId===tab.cookieStoreId&&!current.incognito;
  function sourceChanged(){
   return Object.assign(Error('The download page changed. Capture its link again.'),{batchSourceChanged:true});
  }
  async function capture(item,observed,batch){
   const tabId=observed?.tabId??item.tabId,frameId=observed?.frameId??item.frameId??0;
   if(!Number.isInteger(tabId)||tabId<0)return null;
   if(batch&&tabId!==batch.tab.id)throw sourceChanged();
   const tab=batch?batch.tab:await api.tabs.get(tabId);
   if(tab.incognito)throw Error('Signed-in download capture is disabled in private windows.');
   const stamp=navigation?.supported?navigation.token(tabId,frameId):null;
   const verifyScope=()=>{
    if(stamp!==null&&(navigation.token(tabId,frameId)!==stamp||
      (observed&&!navigation.valid(observed,observed.stamp))))throw sourceChanged();
   };
   verifyScope();
   // Without an observed request, only an explicit same-origin top-frame link establishes cookie scope.
   const outsideLink=!observed&&(new URL(item.url).origin!==new URL(tab.url).origin||frameId!==0);
   if(batch&&outsideLink)return null;
   const store=await (batch?batch.readStore():findStore(tabId,tab));if(!store)return null;
   const firefox=typeof api.runtime.getBrowserInfo==='function';let key;
   if(!firefox&&api.cookies.getPartitionKey){try{const result=await api.cookies.getPartitionKey({tabId,frameId,...(observed?.documentId?{documentId:observed.documentId}:{})});key=result.partitionKey||result;}catch{return null;}}
   const values=await api.cookies.getAll({url:item.url,storeId:store,partitionKey:{},...(firefox?{firstPartyDomain:null}:{})});
   if(!batch&&!sameTab(tab,await api.tabs.get(tabId)))throw sourceChanged();
   verifyScope();
   if(outsideLink)return null;
   const header=observed?(observed.headers?.Cookie||''):undefined;
   const cookies=select(values,item.url,store,header,key);if(cookies===null)return null;
   const userAgent=observed?.headers?.['User-Agent']||root.navigator?.userAgent||'';
   const verify=async()=>{
    if(!sameTab(tab,await api.tabs.get(tabId)))throw sourceChanged();
    verifyScope();
   };
   if(batch)batch.scopes.push(verifyScope);
   return {session:{Origin:new URL(item.url).origin,UserAgent:userAgent,Cookies:cookies,LogoutPages:''},cookies:wire(cookies),verify};
  }
  function beginBatch(tab){
   let storeRead;
   const batch={tab,scopes:[],readStore:()=>storeRead??=findStore(tab.id,tab)};
   return {
    capture:(item,observed)=>capture(item,observed,batch),
    async verify(current){
     if(!sameTab(tab,current))throw sourceChanged();
     if(batch.scopes.length&&!tab.cookieStoreId){
      const store=await findStore(tab.id,current);
      if(store!==await storeRead)throw sourceChanged();
     }
     for(const check of batch.scopes)check();
    }
   };
  }
  return {capture,beginBatch};
 }
 root.UdmDownloadSession={create,select,wire};if(typeof module==='object'&&module.exports)module.exports=root.UdmDownloadSession;
})(globalThis);
