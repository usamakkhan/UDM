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
 root.UdmRenderedGrabber={collect};if(typeof module==='object'&&module.exports)module.exports=root.UdmRenderedGrabber;
})(globalThis);
