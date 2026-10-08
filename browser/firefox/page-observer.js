/* UDM-owned early manifest observer. It leaves the site's request and response unchanged. */
(()=>{
 'use strict';
 if(globalThis.__udmMediaObserverV1)return;
 const cache=new Map(),MAX=2*1024*1024,TTL=120000;
 let currentPage=location.href,generation=0;
 function navigation(){if(location.href!==currentPage){currentPage=location.href;generation++;cache.clear();}return generation;}
 // URL equality alone cannot distinguish a later visit to the same SPA route.
 // Preserve the site's return values and exceptions while observing navigation.
 if(globalThis.history)for(const name of ['pushState','replaceState']){const original=history[name];if(typeof original==='function')try{history[name]=function(...args){const result=Reflect.apply(original,this,args);navigation();return result;};}catch{}}
 globalThis.addEventListener?.('popstate',navigation);
 globalThis.addEventListener?.('hashchange',navigation);
 function url(value){try{const u=new URL(value,location.href);return /^https?:$/.test(u.protocol)&&!u.username&&!u.password?u.href:'';}catch{return '';}}
 function candidate(address,type){return /(?:\.m3u8|\.mpd)(?:[?#]|$)/i.test(address)||/mpegurl|dash\+xml/i.test(type||'');}
 function remember(address,text,type){
  address=url(address);if(!address||typeof text!=='string'||text.length>MAX)return;
  const head=text.trimStart();if(!head.startsWith('#EXTM3U')&&!/^<\?xml[^>]*>\s*<MPD\b|^<MPD\b/i.test(head))return;
  // Captures belong to the current SPA page as well as the current document.
  cache.set(address,{url:address,text,type,page:location.href,time:Date.now()});
  while(cache.size>8)cache.delete(cache.keys().next().value);
 }
 async function inspect(response,address,page,epoch){
  let reader;try{
   if(epoch!==navigation()||page!==location.href)return;
   const type=response.headers.get('content-type')||'';
   if(!response.ok||!candidate(address,type)||Number(response.headers.get('content-length'))>MAX)return;
   const copy=response.clone();reader=copy.body?.getReader();if(!reader)return;
   const decoder=new TextDecoder();let text='',size=0;
   for(;;){const {done,value}=await reader.read();if(done)break;size+=value.length;if(size>MAX){void reader.cancel().catch(()=>{});return;}text+=decoder.decode(value,{stream:true});}
   text+=decoder.decode();if(epoch===navigation()&&page===location.href)remember(response.url||address,text,type);
  }catch{}finally{try{reader?.releaseLock();}catch{}}
 }
 const fetchOriginal=globalThis.fetch;
 if(typeof fetchOriginal==='function')globalThis.fetch=function(input,...args){
  const epoch=navigation(),page=location.href,result=Reflect.apply(fetchOriginal,this,[input,...args]);let target;try{target=url(typeof input==='string'||input instanceof URL?String(input):input?.url);}catch{}
  if(target)result.then(response=>void inspect(response,target,page,epoch),()=>{});return result;
 };
 if(globalThis.XMLHttpRequest){const original=XMLHttpRequest.prototype.open;
  XMLHttpRequest.prototype.open=function(method,address,...rest){
   const target=url(address),epoch=navigation(),page=location.href;
   if(target)this.addEventListener('load',()=>{try{const type=this.getResponseHeader('content-type')||'';if(this.status>=200&&this.status<300&&epoch===navigation()&&page===location.href&&candidate(target,type)&&(this.responseType===''||this.responseType==='text'))remember(this.responseURL||target,this.responseText,type);}catch{}},{once:true});
   return Reflect.apply(original,this,[method,address,...rest]);
  };
 }
 const api=Object.freeze({read(){navigation();const now=Date.now();for(const [key,item] of cache)if(now-item.time>TTL||item.page!==location.href)cache.delete(key);return [...cache.values()].map(item=>({...item}));}});
 Object.defineProperty(globalThis,'__udmMediaObserverV1',{value:api,configurable:false});
})();
