/* Ephemeral browser request context. Credentials never enter extension storage. */
(function(root){
 'use strict';
 const names={'accept':'Accept','accept-language':'Accept-Language','origin':'Origin','referer':'Referer','user-agent':'User-Agent','cookie':'Cookie','authorization':'Authorization'};
 function address(value){try{const u=new URL(value);if(!/^https?:$/.test(u.protocol)||u.username||u.password||u.href.length>16000)return '';u.hash='';return u.href;}catch{return '';}}
 const MAX_BODY=65536;
 function captureBody(body){
  if(!body||body.error)return null;
  if(Array.isArray(body.raw)){
   let size=0;const chunks=[];
   for(const part of body.raw){if(part.file||!(part.bytes instanceof ArrayBuffer)||(size+=part.bytes.byteLength)>MAX_BODY)return null;chunks.push(new Uint8Array(part.bytes));}
   const bytes=new Uint8Array(size);let offset=0;for(const chunk of chunks){bytes.set(chunk,offset);offset+=chunk.length;}
   return {raw:bytes};
  }
  if(body.formData&&typeof body.formData==='object'){
   const form=new URLSearchParams();let size=0;
   for(const [key,values] of Object.entries(body.formData)){if(!Array.isArray(values))return null;for(const value of values){if(typeof value!=='string'||(size+=key.length+value.length)>MAX_BODY)return null;form.append(key,value);}}
   const bytes=new TextEncoder().encode(form.toString());return bytes.length<=MAX_BODY?{form:bytes}:null;
  }
  return null;
 }
 function replay(record){
  if(record.method!=='POST'||!record.body||record.status<200||record.status>=300||!record.attachment)return null;
  const type=record.contentType||'',bytes=record.body.raw||(/^application\/x-www-form-urlencoded(?:\s*;|$)/i.test(type)?record.body.form:null);
  if(!bytes||!type||type.length>256||/[\x00-\x1f\x7f]/.test(type))return null;
  let binary='';for(const byte of bytes)binary+=String.fromCharCode(byte);
  return {method:'POST',contentType:type,body:btoa(binary)};
 }
 function create(api){
  const records=new Map(),TTL=120000;
  function prune(){const now=Date.now();for(const [id,r] of records)if(now-r.time>TTL)records.delete(id);while(records.size>=256)records.delete(records.keys().next().value);}
  function begin(e){
   if(e.tabId<0||e.incognito||e.method==='OPTIONS'||!address(e.url))return;prune();
   const old=records.get(e.requestId),aliases=old?[...old.aliases,old.url].slice(-8):[];
   const method=String(e.method||'GET').toUpperCase();
   // Some browsers omit requestBody on a same-origin 307/308 continuation.
   const preserved=old&&[307,308].includes(old.status)&&method==='POST'&&new URL(old.url).origin===new URL(e.url).origin;
   records.set(e.requestId,{url:address(e.url),aliases,method,tabId:e.tabId,frameId:e.frameId??0,documentId:e.documentId||'',page:e.documentUrl||e.initiator||'',headers:{},body:method==='POST'?(captureBody(e.requestBody)||(preserved?old.body:null)):null,contentType:preserved?old.contentType:'',time:Date.now()});
  }
  function headers(e){
   const r=records.get(e.requestId);if(!r||r.url!==address(e.url))return;
   let size=0;const values={};
   r.contentType=e.requestHeaders?.find(h=>h.name.toLowerCase()==='content-type')?.value||r.contentType||'';
   for(const h of e.requestHeaders||[]){const key=names[h.name.toLowerCase()],value=h.value;
    if(!key||typeof value!=='string'||value.length>16384||/[\x00-\x08\x0a-\x1f\x7f]/.test(value))continue;
    size+=value.length;if(size>32768){r.headers={};return;}values[key]=value;
   }r.headers=values;r.time=Date.now();
  }
  function finish(e){const r=records.get(e.requestId);if(r&&r.url===address(e.url)){r.status=e.statusCode;r.attachment=/^attachment(?:\s*;|$)/i.test(e.responseHeaders?.find(h=>h.name.toLowerCase()==='content-disposition')?.value||'');r.time=Date.now();}}
  function resolve(item,credentials=false){
   prune();const url=address(item.finalUrl||item.url);if(!url)return null;
   let referrerOrigin='';try{referrerOrigin=new URL(item.referrer).origin;}catch{}
   const matches=[...records.values()].filter(r=>r.url===url&&(item.tabId==null||r.tabId===item.tabId)&&(item.frameId==null||r.frameId===item.frameId)&&(!item.documentId||r.documentId===item.documentId)&&(!item.referrer||r.headers.Referer===item.referrer||r.page===item.referrer||(referrerOrigin&&r.page===referrerOrigin)));
   // No credentials are guessed when two live documents requested the same URL.
   const identities=new Set(matches.map(r=>[r.tabId,r.frameId,r.documentId,r.page].join('|')));if(identities.size!==1)return matches.some(r=>r.method!=='GET')?{method:'POST',headers:{},request:null}:null;
   const record=matches.sort((a,b)=>b.time-a.time)[0];if(!record)return null;
   // Different submissions in one document are ambiguous too. Never guess a body.
   if(matches.some(r=>r.method!=='GET')&&new Set(matches.map(r=>JSON.stringify([r.method,replay(r)]))).size!==1)return {method:'POST',headers:{},request:null};
   const selected={...record.headers};if(!credentials){delete selected.Cookie;delete selected.Authorization;}
   return {url:record.url,method:record.method,headers:selected,request:item.browserDownload?replay(record):null,tabId:record.tabId,frameId:record.frameId,documentId:record.documentId};
  }
  function clear(tabId){for(const [id,r] of records)if(r.tabId===tabId)records.delete(id);}
  function install(){
   const filter={urls:['http://*/*','https://*/*']},web=api.webRequest;
   web.onBeforeRequest?.addListener(begin,filter,['requestBody']);
   if(web.onBeforeSendHeaders){try{web.onBeforeSendHeaders.addListener(headers,filter,['requestHeaders','extraHeaders']);}catch{web.onBeforeSendHeaders.addListener(headers,filter,['requestHeaders']);}}
   web.onHeadersReceived?.addListener(finish,filter,['responseHeaders']);
   web.onErrorOccurred?.addListener(e=>records.delete(e.requestId),filter);
   api.tabs.onRemoved.addListener(clear);api.tabs.onUpdated.addListener((id,change)=>{if(change.url)clear(id);});
  }
  return {install,resolve,clear,begin,headers,finish,diagnostics:()=>({requests:records.size})};
 }
 root.UdmRequestContext={create};if(typeof module==='object'&&module.exports)module.exports=root.UdmRequestContext;
})(globalThis);
