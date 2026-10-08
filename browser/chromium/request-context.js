/* Ephemeral browser request context. Credentials never enter extension storage. */
(function(root){
 'use strict';
 const names={'accept':'Accept','accept-language':'Accept-Language','origin':'Origin','referer':'Referer','user-agent':'User-Agent','cookie':'Cookie','authorization':'Authorization'};
 function address(value){try{const u=new URL(value);if(!/^https?:$/.test(u.protocol)||u.username||u.password||u.href.length>16000)return '';u.hash='';return u.href;}catch{return '';}}
 const multipart=typeof UdmMultipart!=='undefined'?UdmMultipart:typeof require==='function'?require('./multipart.js'):null;
 const MAX_BODY=4*1024*1024,MAX_CACHED_BODY=8*MAX_BODY;
 function captureBody(body){
  if(!body||body.error)return null;
  if(Array.isArray(body.raw)){
   if(body.raw.length>1024)return null;
   let size=0,fileParts=0;const chunks=[];
   for(const part of body.raw){
    if(!part)return null;
    if(part.file!==undefined){if(typeof part.file!=='string'||!part.file||part.file.length>32768||part.bytes!==undefined)return null;fileParts++;continue;}
    if(!(part.bytes instanceof ArrayBuffer)||(size+=part.bytes.byteLength)>MAX_BODY)return null;chunks.push(new Uint8Array(part.bytes));
   }
   const bytes=new Uint8Array(size);let offset=0;for(const chunk of chunks){bytes.set(chunk,offset);offset+=chunk.length;}
   return {raw:bytes,fileParts};
  }
  if(body.formData&&typeof body.formData==='object'){
   const form=new URLSearchParams(),pairs=[];let size=0,count=0;
   for(const [key,values] of Object.entries(body.formData)){if(!Array.isArray(values))return null;for(const value of values){if(typeof value!=='string'||++count>16384||(size+=key.length+value.length)>MAX_BODY)return null;form.append(key,value);pairs.push([key,value]);}}
   const bytes=new TextEncoder().encode(form.toString());return {form:bytes.length<=MAX_BODY?bytes:null,pairs,pairsSize:new TextEncoder().encode(JSON.stringify(pairs)).length};
  }
  return null;
 }
 function replayBytes(record){
  if(record.method!=='POST'||!record.body||record.status<200||record.status>=300||!record.attachment||record.body.fileParts&&!record.body.verifiedMultipart)return null;
  const type=record.contentType||'',bytes=record.body.raw||(/^application\/x-www-form-urlencoded(?:\s*;|$)/i.test(type)?record.body.form:null);
  // A browser-validated, ordered submission is required for multipart.
  // Raw envelopes can silently omit file/Blob bytes, even when they parse correctly.
  if(/^multipart\//i.test(type)&&(!record.body.verifiedMultipart||record.body.multipartType!==type))return null;
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
  if(p.username||!['http','https','socks','socks4'].includes(p.type))return {unsupported:true};
  if(typeof p.host!=='string'||p.host.length>255||!Number.isInteger(p.port)||p.port<1||p.port>65535)return {unsupported:true};
  let host=p.host;if(host.startsWith('[')&&host.endsWith(']'))host=host.slice(1,-1);
  if(!host||/[^\x21-\x7e]|[/\\@?#;=\[\]]/.test(host))return {unsupported:true};
  try{const parsed=new URL('http://'+(host.includes(':')?'['+host+']':host)+':'+p.port+'/');host=parsed.hostname.replace(/^\[|\]$/g,'');}catch{return {unsupported:true};}
  const route={url:address(e.url),type:p.type==='socks'?'socks5':p.type,host,port:p.port};
  if(p.type==='socks'||p.type==='socks4'){if(typeof p.proxyDNS!=='boolean')return {unsupported:true};route.proxyDNS=p.proxyDNS;}
  return route;
 }

 function create(api,navigation,chromiumProxy){
  const lifecycle=navigation?.supported?navigation:null;
  const records=new Map(),waiters=new Set(),forms=[],TTL=120000;
  const bodySize=r=>((r.body?.raw||r.body?.form)?.length||0)+(r.body?.identitySize||0)+(r.body?.pairsSize||0);
  const cachedBytes=()=>[...records.values()].reduce((n,r)=>n+bodySize(r),0)+forms.reduce((n,s)=>n+s.size,0);
  function prune(){
   const now=Date.now();for(let i=forms.length-1;i>=0;i--)if(now-forms[i].time>2000||lifecycle&&!lifecycle.valid(forms[i],forms[i].stamp))forms.splice(i,1);
   for(const [id,r] of records)if(now-r.time>TTL||lifecycle&&!lifecycle.valid(r,r.stamp))records.delete(id);
   while(records.size>256)records.delete(records.keys().next().value);
   let bytes=cachedBytes();
   while(forms.length&&(forms.length>32||bytes>MAX_CACHED_BODY))bytes-=forms.shift().size;
   // Retain method metadata when evicting bytes so a known POST cannot become GET.
   for(const r of records.values()){if(bytes<=MAX_CACHED_BODY)break;bytes-=bodySize(r);r.body=null;}
  }
  function matchesBody(form,body,type){
   if(!form||!body)return false;
   return body.raw?multipart.matchesRaw(form,body.raw,type,body.fileParts||0):multipart.matches(form,body.pairs,typeof api.runtime?.getBrowserInfo!=='function');
  }
  const sameRaw=(a,b)=>!!a&&!!b&&a.length===b.length&&a.every((value,i)=>value===b[i]);
  function formMatches(form,r){
   return r.method==='POST'&&!r.body?.verifiedMultipart&&!!(r.body?.pairs||r.body?.raw)&&multipart.boundary(r.contentType)&&
    form.url===r.formUrl&&form.tabId===r.tabId&&form.frameId===r.frameId&&
    (!form.documentId||!r.documentId||form.documentId===r.documentId)&&
    (!form.stamp||!r.stamp||form.stamp===r.stamp)&&Math.abs(form.time-r.created)<=2000&&
    matchesBody(form,r.body,r.contentType)&&(!form.prior||!matchesBody(form.prior,r.body,r.contentType)||multipart.sameEncoding(form.prior,form));
  }
  function bindForms(){
   if(!multipart)return;
   prune();
   for(const r of records.values()){
    const matches=forms.filter(form=>formMatches(form,r));
    if(matches.length!==1)continue;
    const form=matches[0];
    if([...records.values()].filter(other=>formMatches(form,other)).length!==1)continue;
    const bytes=multipart.encode(form,r.contentType);if(!bytes)continue;
    r.body={raw:bytes,verifiedMultipart:true,multipartType:r.contentType,formIdentity:r.body.pairs?multipart.identity(r.body.pairs):undefined,observedRaw:r.body.raw,observedFileParts:r.body.fileParts||0,identitySize:r.body.raw?.length||r.body.pairsSize||0};forms.splice(forms.indexOf(form),1);
   }
   prune();for(const notify of [...waiters])notify();
  }
  function captureForm(message,sender){
   if(!multipart||!sender?.tab||sender.tab.incognito||sender.tab.id<0||!address(sender.url)||!address(message?.url))return false;
   const owner={tabId:sender.tab.id,frameId:sender.frameId??0,documentId:sender.documentId||''};
   if(lifecycle&&!lifecycle.valid(owner))return false;
   const snapshot=multipart.capture(message.fields),prior=message.priorFields===undefined?null:multipart.capture(message.priorFields);if(!snapshot||message.priorFields!==undefined&&!prior)return false;
   const size=new TextEncoder().encode(JSON.stringify([snapshot.fields,prior?.fields])).length;
   if(size>4*MAX_BODY)return false;
   forms.push({...snapshot,prior,...owner,size,url:address(message.url),stamp:lifecycle?.token(owner.tabId,owner.frameId),time:Date.now()});
   bindForms();return true;
  }
  function begin(e){
   if(e.tabId<0||e.incognito||e.method==='OPTIONS'||!address(e.url)||lifecycle&&!lifecycle.valid(e))return;prune();
   const old=records.get(e.requestId),aliases=old?[...old.aliases,old.url].slice(-8):[];
   const method=String(e.method||'GET').toUpperCase();
   // Only an omitted same-origin 307/308 body can inherit prior bytes.
   // Explicitly unavailable/oversized replacements must not reuse an earlier submission.
   const preserved=old&&[307,308].includes(old.status)&&method==='POST'&&new URL(old.url).origin===new URL(e.url).origin;
   let captured=method==='POST'?(e.requestBody==null&&preserved?old.body:captureBody(e.requestBody)):null;
   if(preserved&&old.body?.verifiedMultipart&&((captured?.pairs&&multipart.identity(captured.pairs)===old.body.formIdentity)||(captured?.raw&&sameRaw(captured.raw,old.body.observedRaw)&&(captured.fileParts||0)===old.body.observedFileParts)))captured=old.body;
   records.delete(e.requestId);
   records.set(e.requestId,{stamp:lifecycle?.token(e.tabId,e.frameId??0),requestId:e.requestId,url:address(e.url),proxyObservation:chromiumProxy?.observe(address(e.url),e.incognito===true),aliases,method,tabId:e.tabId,frameId:e.frameId??0,documentId:e.documentId||'',page:e.documentUrl||e.originUrl||e.initiator||'',headers:{},body:captured,contentType:preserved?old.contentType:'',bodyLength:preserved?old.bodyLength:undefined,formUrl:preserved?old.formUrl:address(e.url),created:preserved?old.created:Date.now(),time:Date.now()});
   const observation=records.get(e.requestId)?.proxyObservation;if(observation)void observation.ready.then(()=>{for(const notify of [...waiters])notify();});
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
   }r.headers=values;r.time=Date.now();bindForms();
  }
  function finish(e){const r=records.get(e.requestId);if(r&&r.url===address(e.url)){r.status=e.statusCode;r.response=typeof UdmFileRecognition!=='undefined'?UdmFileRecognition.response(e.responseHeaders):undefined;r.mime=e.responseHeaders?.find(h=>h.name.toLowerCase()==='content-type')?.value||'';r.size=typeof UdmMedia!=='undefined'?UdmMedia.policy.responseSize(e):0;r.proxy=proxyRoute(e,typeof api.runtime?.getBrowserInfo==='function');r.attachment=/^attachment(?:\s*;|$)/i.test(e.responseHeaders?.find(h=>h.name.toLowerCase()==='content-disposition')?.value||'');r.time=Date.now();for(const notify of [...waiters])notify();}}
  const routeFor=record=>record.proxyObservation?chromiumProxy.resolve(record.proxyObservation):record.proxy;
  function resolve(item,credentials=false){
   prune();const url=address(item.finalUrl||item.url);if(!url)return null;
   let referrerOrigin='';try{referrerOrigin=new URL(item.referrer).origin;}catch{}
   const matches=[...records.values()].filter(r=>r.url===url&&(item.tabId==null||r.tabId===item.tabId)&&(item.frameId==null||r.frameId===item.frameId)&&(!item.documentId||r.documentId===item.documentId)&&(!item.referrer||r.headers.Referer===item.referrer||r.page===item.referrer||(referrerOrigin&&r.page===referrerOrigin)));
   // No credentials are guessed when two live documents requested the same URL.
   const identities=new Set(matches.map(r=>[r.tabId,r.frameId,r.documentId,r.page].join('|')));if(identities.size!==1)return matches.some(r=>r.method!=='GET')?{method:'POST',headers:{},request:null}:null;
   const record=matches.sort((a,b)=>b.time-a.time)[0];if(!record)return null;
   // Different submissions in one document are ambiguous too. Never guess a body.
   if(matches.some(r=>r.method!=='GET')&&matches.some(r=>!sameRequest(record,r)))return {method:'POST',headers:{},request:null};
   if(matches.some(r=>JSON.stringify(routeFor(r))!==JSON.stringify(routeFor(record))))return {method:record.method,headers:{},request:null,proxy:{unsupported:true}};
   const selected={...record.headers};if(!credentials){delete selected.Cookie;delete selected.Authorization;}
   return {stamp:record.stamp,requestId:record.requestId,url:record.url,method:record.method,mime:record.mime||'',response:record.response?{...record.response,ambiguous:record.response.ambiguous||matches.some(r=>JSON.stringify(r.response)!==JSON.stringify(record.response))}:undefined,size:record.size||0,headers:selected,...(routeFor(record)?{proxy:routeFor(record)}:{}),request:item.browserDownload?replay(record):null,tabId:record.tabId,frameId:record.frameId,documentId:record.documentId,pending:record.status==null||!!record.proxyObservation?.pending||!!(multipart?.boundary(record.contentType)&&!record.body?.verifiedMultipart&&(record.body?.pairs||record.body?.raw)&&Date.now()-record.created<2000)};
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
  function clear(tabId){for(const [id,r] of records)if(r.tabId===tabId)records.delete(id);for(let i=forms.length-1;i>=0;i--)if(forms[i].tabId===tabId)forms.splice(i,1);}
  function install(){
   const filter={urls:['http://*/*','https://*/*']},web=api.webRequest;
   web.onBeforeRequest?.addListener(begin,filter,['requestBody']);
   if(web.onBeforeSendHeaders){try{web.onBeforeSendHeaders.addListener(headers,filter,['requestHeaders','extraHeaders']);}catch{web.onBeforeSendHeaders.addListener(headers,filter,['requestHeaders']);}}
   web.onHeadersReceived?.addListener(finish,filter,['responseHeaders']);
   web.onErrorOccurred?.addListener(e=>records.delete(e.requestId),filter);
   if(lifecycle)lifecycle.subscribe(event=>{for(const [id,r] of records)if(!lifecycle.keep(r,event))records.delete(id);for(let i=forms.length-1;i>=0;i--)if(!lifecycle.keep(forms[i],event))forms.splice(i,1);});
   else {api.tabs.onRemoved.addListener(clear);api.tabs.onUpdated.addListener((id,change)=>{if(change.url)clear(id);});}
  }
  return {install,resolve,resolveDownload,clear,begin,headers,finish,captureForm,diagnostics:()=>{prune();return {requests:records.size,bodyBytes:cachedBytes()};}};
 }
 root.UdmRequestContext={create};if(typeof module==='object'&&module.exports)module.exports=root.UdmRequestContext;
})(globalThis);
