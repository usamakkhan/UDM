/* Live DOM discovery for the rendered Grabber handoff. Not installed as a content
 * listener: callers must bind this isolated-world result to a native project and
 * browser navigation before accepting it. No cookies or form values are read. */
(function(root){
 'use strict';
 function collect(){
  const page=new URL(location.href);if(!['http:','https:'].includes(page.protocol)||page.username||page.password)throw Error('Use an HTTP or HTTPS project page.');
  const references=[],seen=new Set(),pending=[document];let visited=0,bytes=0,truncated=false;
  const add=(raw,kind,label='')=>{
   if(!raw||raw.startsWith('#'))return;
   try{const url=new URL(raw,document.baseURI);if(!['http:','https:'].includes(url.protocol)||url.username||url.password)return;url.hash='';if(url.href.length>16000)return;
    const key=kind+'\n'+url.href;if(seen.has(key))return;
    const description=String(label).replace(/\s+/g,' ').trim().slice(0,1024),cost=new TextEncoder().encode(url.href+description).length;
    if(references.length>=2000||bytes+cost>2*1024*1024){truncated=true;return;}
    seen.add(key);bytes+=cost;references.push({url:url.href,kind,description});
   }catch{}
  };
  while(pending.length&&visited<10000){
   const node=pending.pop();visited++;
   if(node.nodeType===1){
    const tag=node.localName;
    if(tag==='a'||tag==='area')add(node.getAttribute('href'),'link',node.textContent||node.getAttribute('title'));
    else if(tag==='iframe'||tag==='frame')add(node.getAttribute('src'),'frame',node.getAttribute('title'));
    else if(tag==='img'){add(node.currentSrc||node.getAttribute('src'),'resource',node.getAttribute('alt'));}
    else if(tag==='video'||tag==='audio'){add(node.currentSrc||node.getAttribute('src'),'resource');if(tag==='video')add(node.getAttribute('poster'),'resource');}
    else if(tag==='source'||tag==='track'||tag==='script')add(node.getAttribute('src'),'resource');
    else if(tag==='link'&&node.relList?.contains('stylesheet'))add(node.getAttribute('href'),'resource');
    if(node.shadowRoot)pending.push(node.shadowRoot);
   }
   // Traverse incrementally so a huge document cannot allocate an unbounded list.
   if(node.nextElementSibling)pending.push(node.nextElementSibling);
   if(node.firstElementChild)pending.push(node.firstElementChild);
  }
  if(pending.length)truncated=true;
  return {version:1,url:page.href,baseUrl:document.baseURI,title:document.title.slice(0,1024),references,truncated,visited};
 }
 function create(api,navigation,nativeRequest){
  async function capture({tabId,url}){
   if(!Number.isInteger(tabId)||tabId<0||typeof url!=='string')throw Error('Choose a project page first.');
   const expected=new URL(url);if(!['http:','https:'].includes(expected.protocol)||expected.username||expected.password)throw Error('Use an HTTP or HTTPS project page.');
   if(!api.webNavigation?.getFrame||!api.scripting?.executeScript)throw Error('This browser cannot identify rendered pages.');
   await navigation?.settled?.();
   const tab=await api.tabs.get(tabId),before=await api.webNavigation.getFrame({tabId,frameId:0});
   if(tab.incognito||tab.status==='loading'||tab.url!==expected.href||before?.url!==expected.href||before.errorOccurred||!before.documentId)throw Error('Wait for the project page to finish loading in a regular tab.');
   const stamp=navigation?.token(tabId,0);
   const results=await api.scripting.executeScript({target:{tabId,frameIds:[0]},world:'ISOLATED',func:collect});
   await navigation?.settled?.();
   const after=await api.webNavigation.getFrame({tabId,frameId:0}),current=await api.tabs.get(tabId),item=results?.[0];
   if(results?.length!==1||item?.frameId!==0||item.documentId!==before.documentId||after?.documentId!==before.documentId||after.url!==expected.href||after.errorOccurred||current.url!==expected.href||current.incognito||current.status==='loading'||(navigation&&navigation.token(tabId,0)!==stamp)||item.result?.url!==expected.href)throw Error('The page changed during rendered discovery. Try again.');
   return {...item.result,tabId,frameId:0,documentId:before.documentId};
  }
  async function captureProject({tabId,projectId}){
   if(typeof nativeRequest!=='function'||typeof projectId!=='string'||!/^[a-f0-9]{32}$/i.test(projectId))throw Error('Choose a saved Grabber project first.');
   const hello=await nativeRequest({action:'hello'});
   if(!hello?.ok||!hello.capabilities?.includes('grabber-rendered-storage'))throw Error('Update UDM before capturing rendered pages.');
   const started=await nativeRequest({action:'grabber-rendered-begin',projectId});
   if(!started?.ok)throw Error(started?.error||'UDM could not start rendered discovery.');
   const request=started.request;
   if(request?.projectId!==projectId||typeof request.ticket!=='string'||!request.ticket||!Number.isSafeInteger(request.generation)||request.generation<1||typeof request.url!=='string')throw Error('UDM returned an invalid rendered request.');
   const snapshot=await capture({tabId,url:request.url});
   let reply;
   try{reply=await nativeRequest({action:'grabber-rendered-complete',projectId,ticket:request.ticket,generation:request.generation,snapshot});}
   catch(error){
    // Completion may already be durable. Query its receipt; never replay it.
    const receipt=await nativeRequest({action:'grabber-rendered-status',projectId,ticket:request.ticket});
    if(receipt?.ok&&receipt.status==='saved')return {ok:true,projectId,references:snapshot.references.length,truncated:snapshot.truncated,reconciled:true};
    throw error;
   }
   if(!reply?.ok)throw Error(reply?.error||'UDM could not save the rendered page.');
   return {...reply,projectId};
  }
  return {capture,captureProject};
 }
 root.UdmRenderedGrabber={collect,create};if(typeof module==='object'&&module.exports)module.exports=root.UdmRenderedGrabber;
})(globalThis);
