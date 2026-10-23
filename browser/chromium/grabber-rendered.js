/* Live DOM discovery for the rendered Grabber handoff. Not installed as a content
 * listener: callers must bind this isolated-world result to a native project and
 * browser navigation before accepting it. No cookies or form values are read. */
(function(root){
 'use strict';
 function collect(includeHtml=false){
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
  const result={version:1,url:page.href,baseUrl:document.baseURI,title:document.title.slice(0,1024),references,truncated,visited};
  // Optional until desktop publication is negotiated. Never return a partial
  // document: callers must distinguish unavailable HTML from complete HTML.
  if(includeHtml){
   const limit=512*1024;let cost=0,count=0,oversized=false;
   const adopted=new Map();
   const captureSheets=scope=>{
    const sheets=[];
    try{for(const sheet of scope.adoptedStyleSheets||[]){
     if(sheet.disabled)continue;
     let css='';for(const rule of sheet.cssRules){
      const text=rule.cssText;cost+=text.length;
      if(++count>20000||cost>limit){oversized=true;return;}
      css+=text+'\n';
     }
     sheets.push({css,media:sheet.media.mediaText});
    }}catch{oversized=true;return;}
    adopted.set(scope,sheets);
   };
   captureSheets(document);
   const budget=[document.documentElement];
   while(budget.length&&!oversized){
    const node=budget.pop();if(++count>20000){oversized=true;break;}
    cost+=(node.nodeValue||'').length;
    if(node.nodeType===1)for(const attribute of node.attributes)cost+=attribute.name.length+attribute.value.length+4;
    if(cost>limit){oversized=true;break;}
    if(node.nextSibling)budget.push(node.nextSibling);
    if(node.firstChild)budget.push(node.firstChild);
    if(node.localName==='template'&&node.content.firstChild)budget.push(node.content.firstChild);
    if(node.shadowRoot){captureSheets(node.shadowRoot);if(node.shadowRoot.firstChild)budget.push(node.shadowRoot.firstChild);}
   }
   if(!oversized){
    const copy=document.documentElement.cloneNode(true),pairs=[[document.documentElement,copy]];
    while(pairs.length){
     const [live,saved]=pairs.pop();
     // Serialize open component trees as declarative shadow DOM. Ordinary DOM
     // cloning omits them, leaving an otherwise complete offline page blank.
     if(live.shadowRoot){
      const template=document.createElement('template');template.setAttribute('shadowrootmode','open');
      for(const child of live.shadowRoot.childNodes)template.content.append(child.cloneNode(true));
      saved.prepend(template);pairs.push([live.shadowRoot,template.content]);
     }
     const liveChildren=live.localName==='template'?live.content.childNodes:live.childNodes;
     const savedParent=saved.localName==='template'?saved.content:saved;
     const savedChildren=[...savedParent.childNodes].filter(child=>!(live.shadowRoot&&child===saved.firstChild));
     for(let i=0;i<liveChildren.length;i++)if(savedChildren[i])pairs.push([liveChildren[i],savedChildren[i]]);
     const sheets=adopted.get(live===document.documentElement?document:live);
     if(sheets)for(const sheet of sheets){
      const style=document.createElement('style');
      // CSS escapes prevent a quoted </style> from terminating serialized HTML.
      style.textContent=sheet.css.replace(/</g,'\\3c ');
      if(sheet.media)style.setAttribute('media',sheet.media);
      (live===document.documentElement?(saved.querySelector('body')||saved):saved).append(style);
     }
     if(['img','video','audio'].includes(live.localName)&&live.currentSrc){
      try{
       const address=new URL(live.currentSrc,document.baseURI);
       if(['http:','https:'].includes(address.protocol)&&!address.username&&!address.password){
        saved.setAttribute('src',address.href);
        if(live.localName==='img'){
         saved.removeAttribute('srcset');saved.removeAttribute('sizes');
         if(saved.parentElement?.localName==='picture')for(const child of [...saved.parentElement.children])if(child.localName==='source')child.remove();
        }else{
         // Retain the browser-selected media URL, not stale alternatives. Keep
         // captions and fallback text; blob/MSE URLs cannot be archived here.
         for(const child of [...saved.children])if(child.localName==='source')child.remove();
        }
       }
      }catch{}
     }
    }
    const sanitize=[copy];
    while(sanitize.length){
     const node=sanitize.pop();
     if(node.nodeType!==1)continue;
     sanitize.push(...node.children);
     if(node.localName==='template')sanitize.push(...node.content.children);
     const tag=node.localName;
     if(tag==='script'||tag==='base'||(tag==='meta'&&/^(refresh|content-security-policy)$/i.test(node.getAttribute('http-equiv')||''))){node.remove();continue;}
     for(const attribute of [...node.attributes]){
      const key=attribute.name.toLowerCase();
      if(key.startsWith('on')||key==='srcdoc'||((tag==='input'||tag==='button')&&key==='value')||(tag==='input'&&key==='checked')||(tag==='option'&&key==='selected')){node.removeAttribute(attribute.name);continue;}
      if(['href','src','action','formaction','poster','xlink:href'].includes(key)){
       try{const address=new URL(attribute.value,document.baseURI);if(address.username||address.password||address.protocol==='javascript:')node.removeAttribute(attribute.name);}catch{node.removeAttribute(attribute.name);}
      }
     }
     if(tag==='textarea')node.textContent='';
     if(tag==='meta'&&node.hasAttribute('charset'))node.setAttribute('charset','utf-8');
    }
    const head=copy.querySelector('head');
    if(head){const base=document.createElement('base');base.setAttribute('href',document.baseURI);head.prepend(base);const charset=document.createElement('meta');charset.setAttribute('charset','utf-8');head.prepend(charset);}
    const html='<!doctype html>\n'+copy.outerHTML;
    if(new TextEncoder().encode(html).length<=limit)result.html=html;else oversized=true;
   }
   result.htmlUnavailable=oversized;
  }
  return result;
 }
 function create(api,navigation,nativeRequest){
  async function capture({tabId,url,includeHtml=false}){
   if(!Number.isInteger(tabId)||tabId<0||typeof url!=='string')throw Error('Choose a project page first.');
   const expected=new URL(url);if(!['http:','https:'].includes(expected.protocol)||expected.username||expected.password)throw Error('Use an HTTP or HTTPS project page.');
   expected.hash='';
   const canonical=value=>{const parsed=new URL(value);parsed.hash='';return parsed.href;};
   if(!api.webNavigation?.getFrame||!api.scripting?.executeScript)throw Error('This browser cannot identify rendered pages.');
   await navigation?.settled?.();
   const tab=await api.tabs.get(tabId),before=await api.webNavigation.getFrame({tabId,frameId:0});
   if(tab.incognito||tab.status==='loading'||canonical(tab.url)!==expected.href||before?.url!==tab.url||before.errorOccurred||!before.documentId)throw Error('Wait for the project page to finish loading in a regular tab.');
   const stamp=navigation?.token(tabId,0);
   const results=await api.scripting.executeScript({target:{tabId,frameIds:[0]},world:'ISOLATED',func:collect,args:[includeHtml===true]});
   await navigation?.settled?.();
   const after=await api.webNavigation.getFrame({tabId,frameId:0}),current=await api.tabs.get(tabId),item=results?.[0];
   if(results?.length!==1||item?.frameId!==0||item.documentId!==before.documentId||after?.documentId!==before.documentId||after.url!==before.url||after.errorOccurred||current.url!==tab.url||current.incognito||current.status==='loading'||(navigation&&navigation.token(tabId,0)!==stamp)||item.result?.url!==tab.url)throw Error('The page changed during rendered discovery. Try again.');
   return {...item.result,url:expected.href,tabId,frameId:0,documentId:before.documentId};
  }
  async function captureProject({tabId,projectId}){
   if(typeof nativeRequest!=='function'||typeof projectId!=='string'||!/^[a-f0-9]{32}$/i.test(projectId))throw Error('Choose a saved Grabber project first.');
   const hello=await nativeRequest({action:'hello'});
   if(!hello?.ok||!hello.capabilities?.includes('grabber-rendered-storage'))throw Error('Update UDM before capturing rendered pages.');
   const started=await nativeRequest({action:'grabber-rendered-begin',projectId});
   if(!started?.ok)throw Error(started?.error||'UDM could not start rendered discovery.');
   const request=started.request;
   if(request?.projectId!==projectId||typeof request.ticket!=='string'||!request.ticket||!Number.isSafeInteger(request.generation)||request.generation<1||typeof request.url!=='string')throw Error('UDM returned an invalid rendered request.');
   let snapshot;
   try{snapshot=await capture({tabId,url:request.url,includeHtml:request.captureHtml===true});}
   catch(error){
    // No completion was submitted. Retire only this request, never a newer ticket.
    try{await nativeRequest({action:'grabber-rendered-cancel',projectId,ticket:request.ticket});}catch{}
    throw error;
   }
   let reply;
   try{reply=await nativeRequest({action:'grabber-rendered-complete',projectId,ticket:request.ticket,generation:request.generation,snapshot});}
   catch(error){
    // Completion may already be durable. Query its receipt; never replay it.
    const receipt=await nativeRequest({action:'grabber-rendered-status',projectId,ticket:request.ticket});
    if(receipt?.ok&&receipt.status==='saved')return {ok:true,projectId,references:snapshot.references.length,truncated:snapshot.truncated,htmlSaved:typeof snapshot.html==='string',htmlUnavailable:snapshot.htmlUnavailable===true,reconciled:true};
    throw error;
   }
   if(!reply?.ok)throw Error(reply?.error||'UDM could not save the rendered page.');
   return {...reply,projectId};
  }
  async function projects(message,sender){
   if(sender?.id!==api.runtime.id||sender?.url!==api.runtime.getURL('popup.html'))throw Error('Open the UDM popup to capture a project page.');
   if(!Number.isInteger(message.tabId)||message.tabId<0)throw Error('Choose a project page first.');
   const tab=await api.tabs.get(message.tabId),url=new URL(tab.url);
   if(tab.incognito||tab.status==='loading'||!['http:','https:'].includes(url.protocol)||url.username||url.password)throw Error('Open the project page in a regular tab and wait for it to load.');
   const reply=await nativeRequest({action:'grabber-rendered-projects',url:url.href});
   if(!reply?.ok||!Array.isArray(reply.projects))throw Error(reply?.error||'Open UDM and save a Site Grabber project for this page first.');
   return {ok:true,projects:reply.projects};
  }
  async function complete(message,sender){
   const available=await projects(message,sender);
   if(!available.projects.some(p=>p.projectId===message.projectId))throw Error('This page no longer matches the selected project.');
   return captureProject({tabId:message.tabId,projectId:message.projectId});
  }
  return {capture,captureProject,projects,complete};
 }
 root.UdmRenderedGrabber={collect,create};if(typeof module==='object'&&module.exports)module.exports=root.UdmRenderedGrabber;
})(globalThis);
