/* Explicit Site Grabber sign-in. Cookie values stay in memory until native handoff. */
(function(root){
 'use strict';
 const web=value=>{const u=new URL(value);if(!['http:','https:'].includes(u.protocol)||u.username||u.password)throw Error('Open the project website first.');return u;};
 function matches(host,cookie){const domain=String(cookie.domain||'').replace(/^\./,'').toLowerCase();return !!domain&&(host===domain||(!cookie.hostOnly&&host.endsWith('.'+domain)));}
 function selectedCookies(values,host,store,site){
  const result=[],seen=new Set();let size=0;
  for(const cookie of values){
   if(cookie.storeId!==store||!matches(host,cookie))continue;
   if(cookie.firstPartyDomain&&cookie.firstPartyDomain!==new URL(site).hostname)continue;
   if(cookie.partitionKey&&(cookie.partitionKey.topLevelSite!==site||cookie.partitionKey.hasCrossSiteAncestor===true))continue;
   if(cookie.expirationDate!==undefined&&cookie.expirationDate<=Date.now()/1000)continue;
   const identity=[cookie.name,cookie.domain,cookie.path,cookie.partitionKey?.topLevelSite||''].join('\n');if(seen.has(identity))continue;seen.add(identity);
   const entry={name:cookie.name,value:cookie.value,domain:cookie.domain,path:cookie.path,secure:cookie.secure,hostOnly:cookie.hostOnly};if(cookie.expirationDate!==undefined)entry.expirationDate=cookie.expirationDate;if(cookie.partitionKey)entry.partitioned=true;
   size+=new TextEncoder().encode(String(cookie.name)+'='+String(cookie.value)).length+(result.length?2:0);if(size>16384||result.length>=512)throw Error('This website has too much session data to transfer.');result.push(entry);
  }
  return result;
 }
 function create(api,nativeRequest,navigation){
  function trusted(sender){return sender?.id===api.runtime.id&&sender?.url===api.runtime.getURL('popup.html');}
  async function tabFor(message){
   if(!Number.isInteger(message.tabId))throw Error('Choose a browser tab first.');const tab=await api.tabs.get(message.tabId);
   if(tab.incognito)throw Error('Use a regular browser window for saved Grabber projects.');web(tab.url);return tab;
  }
  async function pending(message,sender){
   if(!trusted(sender))throw Error('Open the UDM extension popup to share a website session.');
   const tab=await tabFor(message),reply=await nativeRequest({action:'grabber-login-pending',url:tab.url});if(!reply?.ok)throw Error(reply?.error||'Open Site Grabber and start browser sign-in first.');
   return {ok:true,projects:reply.projects||[]};
  }
  async function complete(message,sender){
   if(!trusted(sender))throw Error('Website sessions can only be shared from the UDM popup.');
   const tab=await tabFor(message),page=web(tab.url),stamp=navigation?.supported?navigation.token(tab.id,0):null;
   if(!await api.permissions.contains({permissions:['cookies']}))throw Error('Allow cookie access to use this signed-in session.');
   const list=await pending(message,sender),project=list.projects.find(x=>x.id===message.projectId&&x.ticket===message.ticket);if(!project)throw Error('This sign-in request expired. Start it again in UDM.');
   if(page.origin!==project.origin)throw Error('Return to the project website first.');const domain=project.cookieDomain;
   if(typeof domain!=='string'||!(page.hostname===domain||page.hostname.endsWith('.'+domain)))throw Error('The project cookie scope is invalid.');
   const stores=await api.cookies.getAllCookieStores(),matching=stores.filter(x=>x.tabIds.includes(tab.id));
   const store=tab.cookieStoreId||((matching.length===1&&matching[0].id)||'');if(!store)throw Error('The browser could not identify this tab’s cookie store.');
   const site=page.protocol+'//'+domain,firefox=typeof api.runtime.getBrowserInfo==='function';
   const filter={domain,storeId:store};let values;
   if(firefox)values=await api.cookies.getAll({...filter,firstPartyDomain:null,partitionKey:{}});
   else {
    let key={topLevelSite:site,hasCrossSiteAncestor:false};
    if(api.cookies.getPartitionKey){const details=await api.cookies.getPartitionKey({tabId:tab.id,frameId:0});key=details.partitionKey||details;}
    if(key.topLevelSite!==site||key.hasCrossSiteAncestor===true)throw Error('Return to a top-level page on the project website.');
    // One query preserves the browser's cookie ordering across partitions.
    // Selection below retains only this top-level site's partition and ordinary cookies.
    values=await api.cookies.getAll({...filter,partitionKey:{}});
   }
   const cookies=selectedCookies(values,page.hostname,store,site),after=await tabFor(message);
   if(after.url!==tab.url||after.cookieStoreId!==tab.cookieStoreId||(stamp!==null&&navigation.token(tab.id,0)!==stamp))throw Error('The page changed while reading its session. Try again.');
   const reply=await nativeRequest({action:'grabber-login-complete',projectId:project.id,ticket:project.ticket,url:tab.url,session:{Origin:page.origin,UserAgent:root.navigator?.userAgent||'',Cookies:cookies,LogoutPages:''}});
   if(!reply?.ok)throw Error(reply?.error||'UDM could not save this session.');return {ok:true,projectId:project.id};
  }
  return {pending,complete};
 }
 root.UdmGrabberSession={create,selectedCookies};if(typeof module==='object'&&module.exports)module.exports=root.UdmGrabberSession;
})(globalThis);
