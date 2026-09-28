/* Ephemeral browser request context. Credentials never enter extension storage. */
(function(root){
 'use strict';
 const names={'accept':'Accept','accept-language':'Accept-Language','origin':'Origin','referer':'Referer','user-agent':'User-Agent','cookie':'Cookie','authorization':'Authorization'};
 function address(value){try{const u=new URL(value);if(!/^https?:$/.test(u.protocol)||u.username||u.password||u.href.length>16000)return '';u.hash='';return u.href;}catch{return '';}}
 const MAX_BODY=1024*1024,MAX_CACHED_BODY=8*MAX_BODY;
 function captureBody(body){
  if(!body||body.error)return null;
  if(Array.isArray(body.raw)){
   if(body.raw.length>1024)return null;
   let size=0;const chunks=[];
   for(const part of body.raw){if(!part||part.file||!(part.bytes instanceof ArrayBuffer)||(size+=part.bytes.byteLength)>MAX_BODY)return null;chunks.push(new Uint8Array(part.bytes));}
   const bytes=new Uint8Array(size);let offset=0;for(const chunk of chunks){bytes.set(chunk,offset);offset+=chunk.length;}
   return {raw:bytes};
  }
  if(body.formData&&typeof body.formData==='object'){
   const form=new URLSearchParams();let size=0,count=0;
   for(const [key,values] of Object.entries(body.formData)){if(!Array.isArray(values))return null;for(const value of values){if(typeof value!=='string'||++count>16384||(size+=key.length+value.length)>MAX_BODY)return null;form.append(key,value);}}
   const bytes=new TextEncoder().encode(form.toString());return bytes.length<=MAX_BODY?{form:bytes}:null;
  }
  return null;
 }
 function replayBytes(record){
  if(record.method!=='POST'||!record.body||record.status<200||record.status>=300||!record.attachment)return null;
  const type=record.contentType||'',bytes=record.body.raw||(/^application\/x-www-form-urlencoded(?:\s*;|$)/i.test(type)?record.body.form:null);
  // Chromium may omit Blob/file content from raw multipart parts without an error.
  // A syntactically valid captured envelope is not proof that upload bytes are complete.
  if(/^multipart\//i.test(type))return null;
  // Some browsers expose a truncated/empty body without a requestBody.error.
  // Content-Length is verification metadata, never a header forwarded to the desktop.
  if(bytes&&record.bodyLength!==undefined&&record.bodyLength!==bytes.length)return null;
  if(bytes?.length===0&&record.bodyLength!==0)return null;
  if(!bytes||!type||type.length>256||/[\x00-\x1f\x7f]/.test(type))return null;
  return bytes;
 }
 function sameRequest(a,b){
  if(a.method!==b.method)return false;
  const one=replayBytes(a),two=replayBytes(b);
  if(!one||!two)return one===two;
  if(a.contentType!==b.contentType||one.length!==two.length)return false;
  for(let i=0;i<one.length;i++)if(one[i]!==two[i])return false;
  return true;
 }
 function replay(record){
  const bytes=replayBytes(record);if(!bytes)return null;
  // Bounded chunks avoid per-byte string nodes and argument-stack overflow.
  const chunks=[];for(let i=0;i<bytes.length;i+=8192)chunks.push(String.fromCharCode(...bytes.subarray(i,i+8192)));
  return {method:'POST',contentType:record.contentType,body:btoa(chunks.join(''))};
 }

 function proxyRoute(e,firefox){
  if(!e.proxyInfo&&!firefox)return undefined;
  const p=e.proxyInfo;if(!p||p.type==='direct')return {url:address(e.url),type:'direct'};
  if(p.username||!['http','socks','socks4'].includes(p.type))return {unsupported:true};
  if(typeof p.host!=='string'||p.host.length>255||!Number.isInteger(p.port)||p.port<1||p.port>65535)return {unsupported:true};
  let host=p.host;if(host.startsWith('[')&&host.endsWith(']'))host=host.slice(1,-1);
  if(!host||/[^\x21-\x7e]|[/\\@?#;=\[\]]/.test(host))return {unsupported:true};
  try{const parsed=new URL('http://'+(host.includes(':')?'['+host+']':host)+':'+p.port+'/');host=parsed.hostname.replace(/^\[|\]$/g,'');}catch{return {unsupported:true};}
  const route={url:address(e.url),type:p.type==='socks'?'socks5':p.type,host,port:p.port};
  if(p.type!=='http'){if(p.proxyDNS!==true)return {unsupported:true};route.proxyDNS=true;}
  return route;
 }

 function create(api){
  const records=new Map(),waiters=new Set(),TTL=120000;
  const bodySize=r=>(r.body?.raw||r.body?.form)?.length||0;
  const cachedBytes=()=>[...records.values()].reduce((n,r)=>n+bodySize(r),0);
  function prune(){
   const now=Date.now();for(const [id,r] of records)if(now-r.time>TTL)records.delete(id);
   while(records.size>256)records.delete(records.keys().next().value);
   let bytes=cachedBytes();
   // Retain method metadata when evicting bytes so a known POST cannot become GET.
   for(const r of records.values()){if(bytes<=MAX_CACHED_BODY)break;bytes-=bodySize(r);r.body=null;}
  }
  function begin(e){
   if(e.tabId<0||e.incognito||e.method==='OPTIONS'||!address(e.url))return;prune();
   const old=records.get(e.requestId),aliases=old?[...old.aliases,old.url].slice(-8):[];
   const method=String(e.method||'GET').toUpperCase();
   // Only an omitted same-origin 307/308 body can inherit prior bytes.
   // Explicitly unavailable/oversized replacements must not reuse an earlier submission.
   const preserved=old&&[307,308].includes(old.status)&&method==='POST'&&new URL(old.url).origin===new URL(e.url).origin;
   records.delete(e.requestId);
   records.set(e.requestId,{requestId:e.requestId,url:address(e.url),aliases,method,tabId:e.tabId,frameId:e.frameId??0,documentId:e.documentId||'',page:e.documentUrl||e.originUrl||e.initiator||'',headers:{},body:method==='POST'?(e.requestBody==null&&preserved?old.body:captureBody(e.requestBody)):null,contentType:preserved?old.contentType:'',bodyLength:preserved?old.bodyLength:undefined,time:Date.now()});
   prune();
  }
  function headers(e){
   const r=records.get(e.requestId);if(!r||r.url!==address(e.url))return;
   let size=0;const values={};
   const length=e.requestHeaders?.find(h=>h.name.toLowerCase()==='content-length')?.value;
   if(length!==undefined)r.bodyLength=/^\d{1,16}$/.test(length)&&Number.isSafeInteger(Number(length))?Number(length):-1;
   r.contentType=e.requestHeaders?.find(h=>h.name.toLowerCase()==='content-type')?.value||r.contentType||'';
   for(const h of e.requestHeaders||[]){const key=names[h.name.toLowerCase()],value=h.value;
    if(!key||typeof value!=='string'||value.length>16384||/[\x00-\x08\x0a-\x1f\x7f]/.test(value))continue;
    size+=value.length;if(size>32768){r.headers={};return;}values[key]=value;
   }r.headers=values;r.time=Date.now();
  }
  function finish(e){const r=records.get(e.requestId);if(r&&r.url===address(e.url)){r.status=e.statusCode;r.mime=e.responseHeaders?.find(h=>h.name.toLowerCase()==='content-type')?.value||'';r.size=typeof UdmMedia!=='undefined'?UdmMedia.policy.responseSize(e):0;r.proxy=proxyRoute(e,typeof api.runtime?.getBrowserInfo==='function');r.attachment=/^attachment(?:\s*;|$)/i.test(e.responseHeaders?.find(h=>h.name.toLowerCase()==='content-disposition')?.value||'');r.time=Date.now();for(const notify of [...waiters])notify();}}
  function resolve(item,credentials=false){
   prune();const url=address(item.finalUrl||item.url);if(!url)return null;
   let referrerOrigin='';try{referrerOrigin=new URL(item.referrer).origin;}catch{}
   const matches=[...records.values()].filter(r=>r.url===url&&(item.tabId==null||r.tabId===item.tabId)&&(item.frameId==null||r.frameId===item.frameId)&&(!item.documentId||r.documentId===item.documentId)&&(!item.referrer||r.headers.Referer===item.referrer||r.page===item.referrer||(referrerOrigin&&r.page===referrerOrigin)));
   // No credentials are guessed when two live documents requested the same URL.
   const identities=new Set(matches.map(r=>[r.tabId,r.frameId,r.documentId,r.page].join('|')));if(identities.size!==1)return matches.some(r=>r.method!=='GET')?{method:'POST',headers:{},request:null}:null;
   const record=matches.sort((a,b)=>b.time-a.time)[0];if(!record)return null;
   // Different submissions in one document are ambiguous too. Never guess a body.
   if(matches.some(r=>r.method!=='GET')&&matches.some(r=>!sameRequest(record,r)))return {method:'POST',headers:{},request:null};
   if(matches.some(r=>JSON.stringify(r.proxy)!==JSON.stringify(record.proxy)))return {method:record.method,headers:{},request:null,proxy:{unsupported:true}};
   const selected={...record.headers};if(!credentials){delete selected.Cookie;delete selected.Authorization;}
   return {requestId:record.requestId,url:record.url,method:record.method,mime:record.mime||'',size:record.size||0,headers:selected,...(record.proxy?{proxy:record.proxy}:{}),request:item.browserDownload?replay(record):null,tabId:record.tabId,frameId:record.frameId,documentId:record.documentId,pending:record.status==null};
  }
  // Firefox can deliver downloads.onCreated before its queued webRequest events.
  // Wait for matching response metadata without pausing/aborting the original request.
  // Unknown requests stay in the browser; they must never be guessed to be GET.
  function resolveDownload(item,credentials=false){
   return new Promise(done=>{
    let timer;
    const finish=value=>{clearTimeout(timer);waiters.delete(check);done(value);};
    const check=()=>{const value=resolve(item,credentials);if(value&&!value.pending)finish(value);};
    waiters.add(check);timer=setTimeout(()=>finish(null),750);check();
   });
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
  return {install,resolve,resolveDownload,clear,begin,headers,finish,diagnostics:()=>{prune();return {requests:records.size,bodyBytes:cachedBytes()};}};
 }
 root.UdmRequestContext={create};if(typeof module==='object'&&module.exports)module.exports=root.UdmRequestContext;
})(globalThis);
