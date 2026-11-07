/* Cross-site discovery is scoped to the clicked video, its document and frame. */
const UdmSites=(()=>{
 'use strict';
 let api,lifecycle;const queues=new Map(),requestStarts=new Map(),catalogOwners=new Map(),TTL=180000;
 const setting=async()=>{const data=await api.storage.local.get(['settings','desktopPolicy']);return UdmMedia.policy.merge({excluded:[],cookies:false,...data.settings},data.desktopPolicy||{});};
 const excluded=(host,list)=>list.some(x=>host===x||host.endsWith('.'+x));
 function readPlayer(token){
  const dailymotionHost=/(^|\.)dailymotion\.com$/.test(location.hostname);
  const roots=[document],videos=[];
  let video;
  const requiresVisibleCount=()=>dailymotionHost||(video?.currentSrc||video?.src||'').startsWith('blob:');
  for(let i=0;i<roots.length;i++){
   const root=roots[i],players=Array.from(root.querySelectorAll('video'));
   video??=players.find(player=>player.getAttribute('data-udm-player')===token);
   // Only blob discovery and Dailymotion rely on how many players are visible.
   if(video&&!requiresVisibleCount())break;
   videos.push(...players);
   if(roots.length>=40)continue;
   const elements=root.querySelectorAll('*');
   for(let j=0;j<elements.length&&j<3000&&roots.length<40;j++){
    const shadow=elements[j].shadowRoot;
    if(shadow)roots.push(shadow);
   }
  }
  if(!video)return null;
  // Custom X posters lack an asset ID. Match a media record on this player's
  // ancestors by thumbnail AND duration; never infer ownership by request order.
  let twitter=null;
  if(/(^|\.)(x\.com|twitter\.com)$/.test(location.hostname)&&video.poster){
   const posterKey=value=>{try{const u=new URL(value);return u.protocol==='https:'&&u.hostname==='pbs.twimg.com'&&!u.username&&!u.password?u.origin+u.pathname.replace(/\.(?:jpe?g|png|webp)$/i,''):'';}catch{return '';}};
   const poster=posterKey(video.poster),seen=new WeakSet(),matches=new Map();let budget=5000;
   function visit(value,depth){
    if(!value||typeof value!=='object'||depth>10||budget--<=0||seen.has(value))return;seen.add(value);
    const fields=Object.getOwnPropertyDescriptors(value),get=name=>fields[name]?.value;
    const info=get('video_info');
    if(poster&&posterKey(get('media_url_https')||get('media_url'))===poster&&info&&typeof info==='object'){
     const d=Object.getOwnPropertyDescriptors(info),variants=d.variants?.value,duration=Number(d.duration_millis?.value)/1000;
     if(Array.isArray(variants)&&variants.length<=32&&duration>0&&Number.isFinite(video.duration)&&Math.abs(duration-video.duration)<0.5){
      const sources=[];for(const v of variants){if(!v||typeof v!=='object')continue;const f=Object.getOwnPropertyDescriptors(v);try{const u=new URL(f.url?.value),type=f.content_type?.value;
       if(u.protocol==='https:'&&u.hostname==='video.twimg.com'&&!u.username&&!u.password&&['video/mp4','application/x-mpegURL','application/vnd.apple.mpegurl'].includes(type))sources.push({url:u.href,type});
      }catch{}}
      if(sources.length){sources.sort((a,b)=>a.url.localeCompare(b.url));matches.set(JSON.stringify(sources),{sources,duration});}
     }
    }
    // Do not run site getters or traverse React ownership/DOM cycles.
    for(const [key,d] of Object.entries(fields)){if(['children','return','stateNode','_owner'].includes(key))continue;if(d.value&&typeof d.value==='object')visit(d.value,depth+1);}
   }
   for(let node=video,level=0;node&&level<8;node=node.parentElement,level++){
    for(const key of Object.keys(node))if(key.startsWith('__reactProps$')){const d=Object.getOwnPropertyDescriptor(node,key);if(d?.value)visit(d.value,0);}
   }
   // X's logged-out server-rendered page also embeds JSON media records in
   // serialized router data. Decode JSON string literals only; never execute it.
   if(matches.size===0&&budget>0){
    let bytes=0;for(const script of Array.from(document.querySelectorAll('script[data-tsr-stream-part]')).slice(0,8)){
     const text=script.textContent||'';bytes+=text.length;if(bytes>2*1024*1024)break;
     if(!text.includes('video_info')||!text.includes('media_url_https'))continue;
     for(const literal of text.matchAll(/"(?:[^"\\]|\\.)*"/g)){
      if(!literal[0].includes('video_info')||!literal[0].includes('media_url_https'))continue;
      try{const json=JSON.parse(literal[0]);if(typeof json==='string'&&json.startsWith('{'))visit(JSON.parse(json),0);}catch{}
     }
    }
   }
   if(budget>0&&matches.size===1)twitter=[...matches.values()][0];
  }
  let dailymotion=null;
  if(dailymotionHost){
   // Read only playback fields from the actual player; never copy its account context.
   const metadata=globalThis.__PLAYER_CONFIG__?.metadata,stream=metadata?.stream;
   if(metadata&&stream)dailymotion={id:metadata.id,url:stream.url,type:stream.content_type,streamType:stream.stream_type,duration:Number(metadata.info?.duration)||0,title:metadata.info?.title||'',protected:!!(metadata.protected_delivery||stream.drm||stream.protected_delivery)};
  }
  const videoCount=requiresVisibleCount()?videos.filter(v=>{const r=v.getBoundingClientRect();return r.width>=120&&r.height>=70;}).length:null;
  return {manifests:globalThis.__udmMediaObserverV1?.read?.()||[],dailymotion,twitter,page:location.href,timeOrigin:performance.timeOrigin,token,stamp:video.getAttribute('data-udm-epoch'),current:video.currentSrc||video.src||'',poster:video.poster||'',sources:Array.from(video.querySelectorAll('source')).map(s=>({url:s.src,type:s.type})),height:video.videoHeight,width:video.videoWidth,duration:Number.isFinite(video.duration)?video.duration:0,encrypted:!!video.mediaKeys||video.hasAttribute('data-udm-encrypted'),videoCount,title:document.title};
 }
 async function context(message,sender){
  if(!sender.tab||!Number.isInteger(sender.tab.id)||!Number.isInteger(sender.frameId??0)||!/^[a-z0-9-]{1,80}$/i.test(message.token||''))throw Error('Use the download panel on the video.');
  const tabId=sender.tab.id,frameId=sender.frameId??0;
  const [disabled,tab,settings]=await Promise.all([
   typeof browserControls!=='undefined'&&browserControls?.isDisabled(tabId),
   api.tabs.get(tabId),
   setting()
  ]);
  if(disabled)throw Error('UDM is disabled on this tab.');
  if(tab.incognito)throw Error('Video capture is disabled in private windows.');
  if(settings.panelEnabled===false||UdmMedia.policy.panelBlocked(tab.url,settings)||UdmMedia.policy.panelBlocked(sender.url,settings))throw Error('Video panels are disabled on this site.');
  const result=await api.scripting.executeScript({target:{tabId,frameIds:[frameId]},world:'MAIN',injectImmediately:true,func:readPlayer,args:[message.token]});
  const injection=result.find(r=>r.frameId===frameId),player=injection?.result;
  if(sender.documentId&&injection?.documentId&&sender.documentId!==injection.documentId)throw Error('The video page changed. Reopen the panel.');
  const documentId=injection?.documentId||sender.documentId||'';
  if(!player||player.page!==sender.url||player.page!==message.page)throw Error('The video page changed. Reopen the panel.');
  UdmMedia.url(player.page);if(player.encrypted)throw Error('This player uses encrypted or DRM-protected media.');
  const dm=player.dailymotion;
  if(dm){
   const top=new URL(tab.url),frame=new URL(player.page);
   const expected=/(^|\.)dailymotion\.com$/.test(top.hostname)?/^\/video\/([a-z0-9]+)(?:_|\/|$)/i.exec(top.pathname)?.[1]||top.searchParams.get('video'):frame.searchParams.get('video');
   if(!expected||dm.id!==expected)throw Error('The player is changing videos. Wait for the current video, then Refresh.');
   if(dm.protected)throw Error('This player uses encrypted or DRM-protected media.');
   if(dm.streamType!=='recorded')throw Error('Only recorded Dailymotion videos are supported.');
   if(player.videoCount!==1||!player.duration||!dm.duration||Math.abs(player.duration-dm.duration)>2)throw Error('Wait for the main video to start, then Refresh.');
   const target=new URL(UdmMedia.url(dm.url));
   if(!/(^|\.)(dailymotion\.com|dmcdn\.net)$/.test(target.hostname))throw Error('Unrecognized Dailymotion media host.');
   player.title=dm.title||tab.title||player.title;
  }
  const sourceBytes=player.sources?.length?new TextEncoder().encode(JSON.stringify(player.sources)):null;
  const sourceStamp=sourceBytes?Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',sourceBytes))).join(','):'';
  return {tab,tabId,frameId,documentId,player,settings,signature:[tab.url,documentId,player.timeOrigin||'',player.page,player.token,player.stamp,player.current,sourceStamp,player.poster||'',JSON.stringify(player.twitter||null),dm?.id||'',dm?.url||''].join('\n')};
 }
 function requestStarted(event){
  if(!lifecycle||event.tabId<0||typeof event.requestId!=='string')return;
  const now=Date.now();for(const [id,item] of requestStarts)if(now-item.time>TTL)requestStarts.delete(id);
  // Redirect legs retain the original navigation stamp for this request ID.
  if(!requestStarts.has(event.requestId))requestStarts.set(event.requestId,{tabId:event.tabId,frameId:event.frameId??0,stamp:lifecycle.token(event.tabId,event.frameId??0),time:now});
  while(requestStarts.size>2048)requestStarts.delete(requestStarts.keys().next().value);
 }
 async function observe(event){
  if(event.tabId<0||event.statusCode>=400)return;let target;try{target=UdmMedia.url(event.url);}catch{return;}
  if(new URL(target).hostname.endsWith('.googlevideo.com'))return;
  const mime=event.responseHeaders?.find(h=>h.name.toLowerCase()==='content-type')?.value||'',kind=UdmMedia.kind(target,mime);
  if(!kind||kind==='fragment')return;
  const started=requestStarts.get(event.requestId);
  if(started&&(started.tabId!==event.tabId||started.frameId!==(event.frameId??0)))return;
  const stamp=started?started.stamp:lifecycle?.token(event.tabId,event.frameId??0);
  const pending=(queues.get(event.tabId)||Promise.resolve()).catch(()=>{}).then(async()=>{
   await lifecycle?.ready;
   const tab=await api.tabs.get(event.tabId);if(tab.incognito||lifecycle&&!lifecycle.valid(event,stamp))return;
   const settings=await setting();if(settings.panelEnabled===false||UdmMedia.policy.blocked(tab.url,settings))return;
   const key='site-media:'+event.tabId,items=(await api.storage.session.get(key))[key]||[];
   const item={size:UdmMedia.policy.responseSize(event),url:target,kind,mime,frameId:event.frameId??0,documentId:event.documentId||'',page:event.documentUrl||event.initiator||'',time:Date.now()};
   if(lifecycle&&!lifecycle.valid(event,stamp))return;
   await api.storage.session.set({[key]:[...items.filter(x=>Date.now()-x.time<TTL&&!(x.url===target&&x.frameId===item.frameId&&x.documentId===item.documentId)),item].slice(-60)});
  });queues.set(event.tabId,pending);try{await pending;}finally{if(queues.get(event.tabId)===pending)queues.delete(event.tabId);}
 }
 async function fetchText(address,ctx,signal,chargeBytes){
  signal?.throwIfAborted();
  const target=UdmMedia.url(address);
  const captured=ctx?.player.manifests?.find(item=>item.url===target&&item.page===ctx.player.page&&Date.now()-item.time<120000&&typeof item.text==='string'&&item.text.length<=2*1024*1024);
  if(captured){if(chargeBytes)chargeBytes(new TextEncoder().encode(captured.text).length);return {text:captured.text,url:target};}
  if(!await api.permissions.contains({origins:[new URL(target).origin+'/*']}))throw Error('Allow video detection for this site’s media host in UDM’s extension settings.');
  signal?.throwIfAborted();
  const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),12000);
  const abort=()=>controller.abort();signal?.addEventListener('abort',abort,{once:true});if(signal?.aborted)abort();
  try{const response=await fetch(target,{credentials:'omit',signal:controller.signal,cache:'no-store'});if(!response.ok)throw Error('The playlist server returned HTTP '+response.status+'.');if(!await api.permissions.contains({origins:[new URL(response.url||target).origin+'/*']})){await response.body?.cancel();throw Error('The redirected media host needs site permission.');}const reader=response.body.getReader(),decoder=new TextDecoder();let text='',size=0;
   for(;;){const item=await reader.read();if(item.done)break;size+=item.value.length;if(size>2e6){await reader.cancel();throw Error('Playlist exceeds the size limit.');}chargeBytes?.(item.value.length);text+=decoder.decode(item.value,{stream:true});}text+=decoder.decode();return {text,url:UdmMedia.url(response.url||target)};
  }finally{clearTimeout(timer);signal?.removeEventListener('abort',abort);}
 }
 async function selectLiveDashPeriod(address,ctx,capabilityChecked=false,signal){
  const target=UdmMedia.url(address);if(!await api.permissions.contains({origins:[new URL(target).origin+'/*']}))throw Error('Allow video detection for this media host before selecting a live stream.');
  const request=typeof nativeRequest==='function'?nativeRequest:m=>api.runtime.sendNativeMessage('com.udm.download_manager',m);
  if(!capabilityChecked){
   signal?.throwIfAborted();
   const desktop=await request({action:'hello'});
   if(!desktop?.ok||!desktop.capabilities?.includes('live-dash-period-selection'))throw Error('Update the UDM desktop app before selecting this live DASH stream.');
  }
  const allowCredentials=ctx.settings.cookies&&await api.permissions.contains({permissions:['cookies'],origins:['http://*/*','https://*/*']});
  const captured=typeof requestContext!=='undefined'&&requestContext?requestContext.resolve({url:target,tabId:ctx.tabId,frameId:ctx.frameId,documentId:ctx.documentId},allowCredentials):null;
  if(captured&&captured.method!=='GET')throw Error('This live manifest requires an unsupported request method.');
  const headers=captured?.headers?{...captured.headers}:{'User-Agent':navigator.userAgent,'Referer':new URL(ctx.player.page).origin+'/'};
  if(allowCredentials){for(const name of Object.keys(headers))if(name.toLowerCase()==='cookie')delete headers[name];const cookies=await api.cookies.getAll({url:target});if(cookies.length)headers.Cookie=cookies.map(c=>c.name+'='+c.value).join('; ');}
  signal?.throwIfAborted();
  const selected=await request({action:'dash-live-period',url:target,headers});if(!selected?.ok)throw Error(selected?.error||'UDM could not select the active live stream.');
  if(UdmMedia.url(selected.manifestUrl)!==target)throw Error('The live playlist address changed; refresh its formats.');
  if(typeof selected.periodId!=='string'||!selected.periodId||selected.periodId.length>512||/[\x00-\x1f\x7f]/.test(selected.periodId))throw Error('UDM returned an invalid live stream selection.');return selected.periodId;
 }
  async function fetchFullHls(address,ctx,signal,chargeBytes){
   signal?.throwIfAborted();
   const source=UdmMedia.url(address),normalized=UdmMedia.fullHlsUrl(source);
   if(source!==normalized){const captured=ctx?.player.manifests?.find(item=>item.url===source&&item.page===ctx.player.page&&Date.now()-item.time<120000&&typeof item.text==='string'&&item.text.length<=2*1024*1024);
    if(captured&&!/(^|\n)\s*#EXT-X-SKIP:/.test(captured.text)){if(chargeBytes)chargeBytes(new TextEncoder().encode(captured.text).length);return {text:captured.text,url:source};}}
   const first=await fetchText(normalized,ctx,signal,chargeBytes);
   if(!/(^|\n)\s*#EXT-X-SKIP:/.test(first.text))return first;
   const complete=UdmMedia.fullHlsUrl(first.url);
   if(complete===first.url)throw Error('A complete HLS playlist is required before a delta update.');
   const second=await fetchText(complete,ctx,signal,chargeBytes);
   if(/(^|\n)\s*#EXT-X-SKIP:/.test(second.text))throw Error('The HLS server did not return a complete playlist.');
   return second;
  }

 async function expandIndexes(plan,ctx,signal,sharedBudget){
  const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),12000),budget=sharedBudget||{requests:0,bytes:0,references:0},tracks=[],resources=[],identities=new Map();
  const abort=()=>controller.abort();signal?.addEventListener('abort',abort,{once:true});if(signal?.aborted)abort();
  async function permission(target){if(!await api.permissions.contains({origins:[new URL(target).origin+'/*']}))throw Error('Allow video detection for this media host to read its DASH index.');}
  async function read(target,span,charged=false){
   if(controller.signal.aborted)throw Error('Reading the DASH index tree timed out.');
   const latest=await context({token:ctx.player.token,page:ctx.player.page},{tab:{id:ctx.tabId},frameId:ctx.frameId,documentId:ctx.documentId,url:ctx.player.page});
   if(latest.signature!==ctx.signature)throw Error('The video changed while reading its indexes.');
   await permission(target);
   if(!charged&&(++budget.requests>64||(span&&(budget.bytes+=span.length)>2*1024*1024)))throw Error('DASH index tree exceeds its request or byte limit.');
   const credentials=ctx.settings.cookies&&await api.permissions.contains({permissions:['cookies'],origins:['http://*/*','https://*/*']});
   const captured=typeof requestContext!=='undefined'&&requestContext?requestContext.resolve({url:target,tabId:ctx.tabId,frameId:ctx.frameId,documentId:ctx.documentId},credentials):null;
   const identity=identities.get(target),headers=span?{Range:'bytes='+span.start+'-'+(span.start+span.length-1)}:{};
   if(credentials&&captured?.method==='GET'&&captured.headers.Authorization)headers.Authorization=captured.headers.Authorization;
   if(identity?.etag)headers['If-Match']=identity.etag;
   else if(identity?.modified)headers['If-Unmodified-Since']=identity.modified;
   // Each index or media probe has its own permission, authorization and identity.
   const response=await fetch(target,{headers,credentials:credentials?'include':'omit',redirect:'error',cache:'no-store',signal:controller.signal});
   const encoding=response.headers.get('Content-Encoding'),cr=response.headers.get('Content-Range'),match=/^bytes (\d+)-(\d+)\/(\d+)$/.exec(cr||''),declared=response.headers.get('Content-Length');
   let total=span&&match?Number(match[3]):declared!==null?Number(declared):null;
   if((span?(response.status!==206||!match||Number(match[1])!==span.start||Number(match[2])!==span.start+span.length-1||!Number.isSafeInteger(total)||total<=Number(match[2])):(response.status!==200||cr!==null))||(encoding&&encoding.toLowerCase()!=='identity')||UdmMedia.url(response.url||target)!==target){
    await response.body?.cancel();throw Error('The server did not return the requested DASH index range or file.');
   }
   const limit=span?span.length:524288;
   if(declared!==null&&(!/^\d+$/.test(declared)||!Number.isSafeInteger(Number(declared))||Number(declared)<1||(span?Number(declared)!==span.length:Number(declared)>limit))){
    await response.body?.cancel();throw Error('Invalid or oversized DASH index response length.');
   }
   const tag=response.headers.get('ETag'),modified=response.headers.get('Last-Modified');
   if(identity&&((total!==null&&total!==identity.size)||(identity.etag&&tag!==identity.etag)||(!identity.etag&&identity.modified&&modified!==identity.modified))){
    await response.body?.cancel();throw Error('DASH media changed while reading its indexes.');
   }
   const bytes=new Uint8Array(limit),reader=response.body.getReader();let offset=0;
   for(;;){const chunk=await reader.read();if(chunk.done)break;
    if(offset+chunk.value.length>limit||(!charged&&!span&&(budget.bytes+=chunk.value.length)>2*1024*1024)){await reader.cancel();throw Error('DASH index exceeds its requested range or byte limit.');}
    bytes.set(chunk.value,offset);offset+=chunk.value.length;
   }
   if(!offset||(span&&offset!==span.length)||(declared!==null&&offset!==Number(declared)))throw Error('The DASH index response was truncated.');
   if(total===null)total=offset;
   if(identity&&total!==identity.size)throw Error('DASH media changed while reading its indexes.');
   if(!identity){
    const etag=tag&&/^"[\x21\x23-\x7e\x80-\xff]*"$/.test(tag)&&tag.length<=1024?tag:'';
    const date=modified&&/^[A-Z][a-z]{2}, \d{2} [A-Z][a-z]{2} \d{4} \d{2}:\d{2}:\d{2} GMT$/.test(modified)&&Number.isFinite(Date.parse(modified))?modified:'';
    identities.set(target,{url:target,size:total,...(etag?{etag}:date?{modified:date}:{})});
   }
   return {bytes:bytes.subarray(0,offset),total};
  }
  try{for(const track of plan.tracks){
   if(!track.index){tracks.push(track);continue;}
   const spec=track.index,indexUrl=UdmMedia.url(spec.url),target=UdmMedia.url(spec.mediaUrl||spec.url);
   await permission(indexUrl);await permission(target);
   let ranges;
   if(indexUrl!==target){
    const span=spec.start===undefined?null:{start:spec.start,length:spec.length};
    const index=await read(indexUrl,span),media=await read(target,{start:0,length:1});
    ranges=await UdmMedia.resolveExternalSidx(spec,index,media.total,child=>read(indexUrl,child,true),budget);
   }else ranges=await UdmMedia.resolveSidx(spec,span=>read(target,span,true),budget);
   const resource=identities.get(target);
   if(!resources.some(r=>r.url===target))resources.push(resource);
   for(const init of track.segments)if(init.url===target&&(!Number.isSafeInteger(init.start+init.length)||init.start+init.length>ranges[0].start))throw Error('DASH initialization overlaps the media segments.');
   const {index,...resolved}=track;tracks.push({...resolved,segments:[...track.segments,...ranges.map(part=>({url:target,...part}))]});
   if(tracks.reduce((sum,t)=>sum+t.segments.length,0)>UdmMedia.MAX)throw Error('This selection exceeds the current segment limit.');
  }
  if(tracks.reduce((sum,t)=>sum+t.segments.length,0)>UdmMedia.MAX)throw Error('This selection exceeds the current segment limit.');
  return {...plan,tracks,resourceVersion:1,resources};
  }finally{controller.abort();clearTimeout(timer);signal?.removeEventListener('abort',abort);}
 }


 const catalogKey=(tabId,frameId,token)=>JSON.stringify([tabId,frameId,token]);
 function cancelFormats(message,sender){
  if(!Number.isInteger(sender.tab?.id)||sender.tab.incognito||typeof message.token!=='string'||
   typeof message.progressId!=='string'||!/^[0-9a-f-]{36}$/i.test(message.progressId))return false;
  const owner=catalogOwners.get(catalogKey(sender.tab.id,sender.frameId??0,message.token));
  if(!owner||owner.progressId!==message.progressId||owner.page!==sender.url||owner.documentId!==(sender.documentId||''))return false;
  owner.controller.abort(Error('Video format lookup canceled.'));
  return true;
 }
 async function list(message,sender,onProgress){
  const tabId=sender.tab?.id,frameId=sender.frameId??0;
  const playerKey=catalogKey(tabId,frameId,message.token);
  const controller=new AbortController(),signal=controller.signal;
  const owner={controller,tabId,frameId,documentId:sender.documentId||'',page:sender.url,progressId:message.progressId};
  catalogOwners.get(playerKey)?.controller.abort(Error('This player’s format list was refreshed.'));
  catalogOwners.set(playerKey,owner);
  const checkOwner=()=>{
   signal.throwIfAborted();
   if(catalogOwners.get(playerKey)!==owner)throw Error('This player’s format list was refreshed.');
  };
  let cancelReply;
  const canceled=new Promise((_,reject)=>{
   cancelReply=()=>reject(signal.reason);
   signal.addEventListener('abort',cancelReply,{once:true});
  });
  try{return await Promise.race([canceled,discoverFormats(message,sender,onProgress,checkOwner,signal)]);}
  finally{
   signal.removeEventListener('abort',cancelReply);
   if(catalogOwners.get(playerKey)===owner)catalogOwners.delete(playerKey);
  }
 }
 async function discoverFormats(message,sender,onProgress,checkOwner,ownerSignal){
  const stamp=lifecycle?.token(sender.tab?.id,sender.frameId??0);
  await lifecycle?.ready;
  checkOwner();
  const ctx=await context(message,sender),p=ctx.player,candidates=[],notes=[];
  checkOwner();
  const feedAsset=p.current.startsWith('blob:')?UdmMedia.twitterAsset(p.poster):'';
  const sources=p.twitter?.sources?.length?p.twitter.sources:p.dailymotion?[{url:p.dailymotion.url,type:p.dailymotion.type}]:[{url:p.current,type:''},...p.sources];
  for(const s of sources)try{if(!s.url)continue;const target=UdmMedia.url(s.url,p.page),kind=UdmMedia.kind(target,s.type)||(s.url===p.current&&p.width>0?'direct':'');if(kind)candidates.push({url:target,kind,owned:true});}catch{}
  // Never mix background requests from other frames/players into a direct source.
  const blobFallback=!candidates.length&&p.current.startsWith('blob:');
  if(blobFallback&&p.videoCount!==1&&!feedAsset)throw Error('Several videos share this frame. UDM cannot safely associate their streaming requests yet.');
  const observedKey='site-media:'+ctx.tabId;
  // Include captures already queued when this lookup reached the player.
  await (queues.get(ctx.tabId)||Promise.resolve()).catch(()=>{});
  const observed=(await api.storage.session.get(observedKey))[observedKey]||[];
  checkOwner();
  if(blobFallback){
   const asset=feedAsset;
   for(const entry of p.manifests||[]){if(entry.page!==p.page||Date.now()-entry.time>=TTL||(asset&&UdmMedia.twitterAsset(entry.url)!==asset))continue;try{const target=UdmMedia.url(entry.url),kind=UdmMedia.kind(target,entry.type);if(['hls','dash'].includes(kind))candidates.push({url:target,kind,owned:true});}catch{}}
   const matches=observed.filter(x=>(!asset||(UdmMedia.twitterAsset(x.url)===asset&&!UdmMedia.feedMediaFragment(x.url)))&&x.frameId===ctx.frameId&&Date.now()-x.time<TTL&&
    // Frame IDs survive navigation; document identity does not. Older browsers
    // use the page's navigation timestamp and URL as a conservative fallback.
    (ctx.documentId?x.documentId===ctx.documentId:!x.documentId&&Number.isFinite(p.timeOrigin)&&x.time>=p.timeOrigin)&&(x.page===p.page||x.page===new URL(p.page).origin)&&(asset?['hls','dash','direct']:['hls','dash']).includes(x.kind));
   candidates.push(...matches.slice(-8));
   if(candidates.length)notes.push('These playlists were observed in this player’s frame. Check the source label before choosing; embedded ads cannot always be distinguished.');
  }
  const sourcesToRead=[...new Map(candidates.map(item=>[item.url,item])).values()].slice(0,8);
  const directChoices=new Map();
  for(const item of sourcesToRead){
   if(item.kind!=='direct')continue;
   const own=item.url===p.current,explicitX=p.twitter?.sources?.some(source=>source.url===item.url&&source.type==='video/mp4');
   const dimensions=explicitX?/\/([1-9]\d{1,4})x([1-9]\d{1,4})\/[^/]+\.mp4$/i.exec(new URL(item.url).pathname):null;
   const height=dimensions?Number(dimensions[2]):own?p.height:0;
   const known=observed.find(offer=>offer.url===item.url&&offer.frameId===ctx.frameId&&Date.now()-offer.time<TTL&&(ctx.documentId?offer.documentId===ctx.documentId:!offer.documentId&&offer.time>=p.timeOrigin));
   directChoices.set(item.url,{kind:'direct',originalFile:!!explicitX,size:known?.size||0,mime:known?.mime||'',url:item.url,height,label:(height?height+'p':'Original quality')+' · '+(new URL(item.url).pathname.split('.').pop()||'video').toUpperCase().slice(0,8)+(explicitX?' · Original file':'')});
  }
  const isStream=item=>item.kind==='hls'||item.kind==='dash';
  const moreFormats=message.directFirst===true&&sourcesToRead.some(isStream)&&
   [...directChoices.values()].some(choice=>UdmMedia.policy.panelAllowed(choice,ctx.settings));
  const periodCatalogs=new Map(),keysByFormat=new Map(),issuedKeys=new Set();
  const sections=sourcesToRead.map(()=>({choices:[],covered:[],nested:[],notes:[]}));
  let progressTimer,progressRunning=false,progressDirty=false,finalizing=false;
  function scheduleProgress(){
   if(!onProgress||moreFormats||finalizing||ownerSignal.aborted)return;
   progressDirty=true;
   if(progressTimer||progressRunning)return;
   progressTimer=setTimeout(async()=>{
    progressTimer=null;
    progressRunning=true;
    progressDirty=false;
    try{
     const update=await publishCatalog(true);
     if(update&&!finalizing){
      checkOwner();
      for(const choice of update.choices)issuedKeys.add(choice.key);
      onProgress(update);
     }
    }catch{}finally{
     progressRunning=false;
     if(progressDirty)scheduleProgress();
    }
   },80);
  }
  const streamController=new AbortController(),signal=streamController.signal;
  const cancelStream=()=>streamController.abort(ownerSignal.reason);
  ownerSignal.addEventListener('abort',cancelStream,{once:true});
  const hlsController=new AbortController(),hlsSignal=hlsController.signal;
  signal.addEventListener('abort',()=>hlsController.abort(signal.reason),{once:true});
  const deadline=Error('Some streaming playlists did not respond in time. Refresh to try again.');
  // Leave time for the final player check and offer write before the 12-second panel timeout.
  const streamTimer=!moreFormats&&sourcesToRead.some(isStream)?setTimeout(()=>streamController.abort(deadline),9000):null;
  const waiting=[],hlsCache=new Map();
  let activeReads=0,hlsBytes=0;
  async function readStream(operation,readSignal=signal){
   if(activeReads===4)await new Promise(resolve=>waiting.push(resolve));else ++activeReads;
   let onAbort;
   try{
    readSignal.throwIfAborted();
    const stopped=new Promise((_,reject)=>{
     onAbort=()=>reject(readSignal.reason);
     readSignal.addEventListener('abort',onAbort,{once:true});
    });
    return await Promise.race([stopped,operation()]);
   }finally{
    if(onAbort)readSignal.removeEventListener('abort',onAbort);
    const next=waiting.shift();if(next)next();else --activeReads;
   }
  }
  function chargeHls(bytes){
   hlsSignal.throwIfAborted();
   if(hlsBytes+bytes>8*1024*1024){
    const error=Error('The HLS catalog exceeds its eight MiB playlist limit.');
    hlsController.abort(error);
    throw error;
   }
   hlsBytes+=bytes;
  }
  function readHls(address){
   hlsSignal.throwIfAborted();
   const target=UdmMedia.fullHlsUrl(address);
   if(!hlsCache.has(target)){
    if(hlsCache.size>=64)throw Error('The HLS catalog exceeds its 64-playlist limit.');
    hlsCache.set(target,readStream(()=>fetchFullHls(address,ctx,hlsSignal,chargeHls),hlsSignal));
   }
   return hlsCache.get(target);
  }
  async function readSource(item,index){
   const section=sections[index];
   if(item.kind==='direct'){section.choices.push(directChoices.get(item.url));return;}
   if(moreFormats||!isStream(item))return;
   try{
    const sourceName=decodeURIComponent(new URL(item.url).pathname.split('/').pop()||new URL(item.url).hostname).slice(0,65);
    if(item.kind==='hls'){
     const fetched=await readHls(item.url),parsed=UdmMedia.hls(fetched.text,fetched.url,{allowLive:true});
     if(parsed.kind==='master'){
      const catalog=await UdmMedia.hlsCatalog(fetched,readHls,hlsSignal,choice=>{
       section.choices.push({kind:'hls',...choice,catalogRoot:item.url,label:(choice.height?choice.height+'p':'Original quality')+' · HLS',source:sourceName});
       scheduleProgress();
      });
      // A partial parent may have seen a nested URL without verifying its formats.
      if(!hlsSignal.aborted&&!catalog.notes.length){
       section.covered=catalog.covered;
       section.nested=catalog.covered.filter(url=>url!==fetched.url&&url!==item.url);
      }
      section.notes.push(...catalog.notes);
     }else if(parsed.live)section.choices.push({manifestUrl:fetched.url,kind:'hls',url:fetched.url,live:true,height:p.height||0,label:'Live · HLS',source:sourceName});
     else section.choices.push({manifestUrl:fetched.url,kind:'adaptive',height:0,label:'Original quality · HLS',source:sourceName,plan:{type:'hls',height:0,audioExpected:false,tracks:[{kind:'video',segments:parsed.segments}]}});
    }else{
     const fetched=await readStream(()=>fetchText(item.url,ctx,signal));
     let catalog=UdmMedia.dashCatalog(fetched.text,fetched.url);
     if(catalog.livePeriodSelectionRequired){
      const period=await readStream(()=>selectLiveDashPeriod(fetched.url,ctx,false,signal));
      catalog=UdmMedia.dashCatalog(fetched.text,fetched.url,{livePeriodId:period});
     }
     if(catalog.presentation){
      const catalogKey=crypto.randomUUID();periodCatalogs.set(catalogKey,catalog.presentation);
      for(const [qualityIndex,choice] of catalog.presentation.periods[0].choices.entries()){
       const {plan,audioOptions=[],subtitleOptions=[],...display}=choice;
       const describe=({track,...option})=>option;
       section.choices.push({kind:'dash-periods',...display,catalogKey,qualityIndex,audioOptions:audioOptions.map(describe),subtitleOptions:subtitleOptions.map(describe),label:(choice.height?choice.height+'p':'Original quality')+' · DASH',source:sourceName});
      }
     }else for(const choice of catalog.choices)section.choices.push({kind:'adaptive',...choice,source:sourceName});
    }
   }catch(error){section.notes.push(error.message);}
   if(section.choices.length)scheduleProgress();
  }
  const identity=choice=>JSON.stringify([choice.kind,choice.url,choice.manifestUrl,choice.height,choice.bandwidth,choice.frameRate,choice.codecs,choice.audioOptions,choice.subtitleOptions,choice.hlsVariables,choice.plan,choice.catalogKey,choice.qualityIndex,choice.container,choice.live]);
  async function publishCatalog(partial=false){
   checkOwner();
   const choices=sections.flatMap(section=>section.choices);
   if(partial&&(!choices.length||finalizing))return null;
   const coveredPlaylists=new Set(sections.flatMap(section=>section.covered));
   const nestedPlaylists=new Set(sections.flatMap(section=>section.nested));
   const messages=[...notes,...sections.flatMap(section=>section.notes)];
   if(signal.reason===deadline)messages.push(deadline.message);
   const current=await context(message,sender);
   if(current.signature!==ctx.signature)throw Error('The video changed while reading its formats.');
   if(partial&&finalizing)return null;
   checkOwner();
   const key='site-offers:'+ctx.tabId;
   const platform=p.dailymotion?'Dailymotion':new URL(p.page).hostname.replace(/^www\./,'');
   // Adapted from Awesome Downloader media-choices.js (MIT): prefer the richer
   // catalog only after successful parsing, retaining direct fallback on failure.
   const richFeed=feedAsset&&choices.some(c=>c.catalogRoot&&UdmMedia.twitterAsset(c.catalogRoot)===feedAsset&&(c.height>0||/avc|hvc|hev|vp0?9|av01/i.test(c.codecs||'')));
   const uniqueChoices=[...new Map(choices.filter(choice=>(choice.originalFile||!richFeed||choice.kind!=='direct'||UdmMedia.twitterAsset(choice.url)!==feedAsset)&&(!choice.catalogRoot||!nestedPlaylists.has(choice.catalogRoot))&&(!choice.manifestUrl||!coveredPlaylists.has(choice.manifestUrl))).map(choice=>[identity(choice),choice])).values()];
   const formats=uniqueChoices.flatMap(choice=>{
    if(choice.kind==='direct')return [{...choice,source:platform}];
    // Offer transport-stream output only for explicitly compatible HLS codecs.
    const codecList=(choice.codecs||'').split(',').map(c=>c.trim()).filter(Boolean);
    const ts=choice.kind==='hls'&&codecList.length>0&&codecList.every(c=>/^(avc[13]|hvc1|hev1|mp4a)(\.|$)/i.test(c));
    return (choice.onlyAudio?['m4a']:ts?['mp4','ts']:['mp4']).map(container=>({...choice,container,audioOnlyAvailable:!!choice.audioOptions?.length||!!choice.plan?.audioExpected||codecList.some(c=>/^(mp4a|ac-3|ec-3|opus|vorbis)(\.|$)/i.test(c)),subtitleOptions:container==='ts'?[]:choice.subtitleOptions,source:p.dailymotion?'':choice.source,
     detail:(choice.kind==='hls'||choice.plan?.type==='hls'?'HLS':'DASH')+' • '+new URL(choice.url||choice.plan?.manifest||choice.plan?.tracks?.[0]?.segments?.[0]?.url||p.page).hostname,
     label:platform+(choice.live?' · Live':'')+' · '+container.toUpperCase()+' · '+(choice.onlyAudio?'Audio only':choice.height?choice.height+'p'+(choice.height>=720?' HD':''):'Original quality')+(choice.frameRate>0?' · '+Number(choice.frameRate.toFixed(3))+' fps':'')+(choice.bandwidth>0?' · '+Math.round(choice.bandwidth/1000)+' kbps':'')+(choice.audioOptions?.length>1?' · '+choice.audioOptions.length+' audio tracks':choice.audioOptions?.length===1?' · '+choice.audioOptions[0].label:'')}));
   });
   const allowed=formats.filter(c=>UdmMedia.policy.panelAllowed(c,current.settings)).map(c=>({...c,audioOnlyAvailable:c.audioOnlyAvailable&&UdmMedia.policy.panelAllowed({container:'m4a'},current.settings)}));
   if(formats.length&&!allowed.length)messages.push('These formats are hidden by your video-panel file-type or minimum-size settings.');
   const records=allowed.slice(0,80).sort((a,b)=>b.height-a.height||Number(!!b.originalFile)-Number(!!a.originalFile)).map(choice=>{
    const format=identity(choice);
    const key=keysByFormat.get(format)||crypto.randomUUID();
    keysByFormat.set(format,key);
    return {...choice,key,token:p.token,frameId:ctx.frameId,documentId:ctx.documentId,signature:ctx.signature,created:Date.now()};
   });
   if(partial&&!records.length)return null;
   const catalogRecords=[...new Set(records.map(record=>record.catalogKey).filter(Boolean))].map(catalogKey=>({kind:'dash-period-catalog',key:catalogKey,presentation:periodCatalogs.get(catalogKey),token:p.token,frameId:ctx.frameId,documentId:ctx.documentId,signature:ctx.signature,created:Date.now()}));
   const save=(queues.get(ctx.tabId)||Promise.resolve()).catch(()=>{}).then(async()=>{
    const offers=(await api.storage.session.get(key))[key]||[];
    if(partial&&finalizing)return;
    checkOwner();
    if(lifecycle&&!lifecycle.valid(ctx,stamp))throw Error('The video changed while reading its formats.');
    const previous=offers.filter(offer=>Date.now()-offer.created<TTL);
    const forPlayer=offer=>offer.token===p.token&&offer.frameId===ctx.frameId;
    const previews=new Map(previous.filter(offer=>forPlayer(offer)&&offer.preview&&offer.kind==='direct'&&
     offer.signature===ctx.signature&&directChoices.has(offer.url)&&UdmMedia.policy.panelAllowed(directChoices.get(offer.url),current.settings)).map(offer=>[offer.url,offer]));
    for(const record of records){
     if(record.kind!=='direct')continue;
     const existing=previews.get(record.url);
     if(existing){record.key=existing.key;keysByFormat.set(identity(record),record.key);previews.delete(record.url);}
     if(partial||moreFormats||existing)record.preview=true;
    }
    // A click on an earlier row can arrive as a richer catalog replaces it.
    const latestKeys=new Set(records.map(record=>record.key));
    const retained=previous.filter(offer=>forPlayer(offer)&&offer.kind!=='direct'&&issuedKeys.has(offer.key)&&
     !latestKeys.has(offer.key)&&offer.signature===ctx.signature&&UdmMedia.policy.panelAllowed(offer,current.settings));
    const retainedCatalogIds=new Set(retained.map(offer=>offer.catalogKey).filter(Boolean));
    const latestCatalogIds=new Set(catalogRecords.map(record=>record.key));
    const retainedCatalogs=previous.filter(offer=>offer.kind==='dash-period-catalog'&&forPlayer(offer)&&
     offer.signature===ctx.signature&&retainedCatalogIds.has(offer.key)&&!latestCatalogIds.has(offer.key));
    const active=[...previews.values(),...retainedCatalogs,...retained,...catalogRecords,...records];
    if(JSON.stringify(active).length>4000000)throw Error('This audio/video catalog is too large for the current browser cache.');
    await api.storage.session.set({[key]:[...previous.filter(offer=>!forPlayer(offer)),...active].slice(-Math.max(100,active.length))});
   });queues.set(ctx.tabId,save);try{await save;}finally{if(queues.get(ctx.tabId)===save)queues.delete(ctx.tabId);}
   if(!records.length&&!messages.length)messages.push('No supported playlist is captured for this player. Reload the video page, play the video, then Refresh.');
   return {ok:true,...(partial||moreFormats?{moreFormats:true}:{}),title:String(p.dailymotion?.title||p.title||'').trim().slice(0,500),choices:records.map(({key,label,source,height,container,detail,onlyAudio,audioOnlyAvailable,audioOptions,subtitleOptions})=>({key,label,source,height,container,detail,onlyAudio:!!onlyAudio,audioOnlyAvailable,...(subtitleOptions?.length?{subtitleOptions:subtitleOptions.map(({key,label})=>({key,label}))}:{}),...(audioOptions?.length?{audioOptions:audioOptions.map(({key,label,default:preferred})=>({key,label,default:preferred}))}:{})})),note:[...new Set(messages)].join(' ')};
  }
  try{
   await Promise.all(sourcesToRead.map(readSource));
  }finally{
   finalizing=true;
   if(progressTimer)clearTimeout(progressTimer);
   if(streamTimer!==null)clearTimeout(streamTimer);
   ownerSignal.removeEventListener('abort',cancelStream);
   streamController.abort();
  }
  return publishCatalog();
 }
 async function download(message,sender){
  // The panel has a 65-second reply deadline. Stop preparation before it can
  // expire, and guard the side-effect boundary even if a browser API replies late.
  const controller=new AbortController();let timer,nativeStarted=false,closed=false,rejectDeadline,expiresAt=Date.now()+20000;
  const preparationError='Reading this video took too long. No download was sent to UDM. Refresh the panel and try again.';
  const uncertainError='UDM has not confirmed this download. Check its download list before trying again.';
  const deadline=new Promise((_,reject)=>{rejectDeadline=reject;});
  const expire=()=>{closed=true;controller.abort();rejectDeadline(Error(nativeStarted?uncertainError:preparationError));};
  const check=()=>{if(closed||Date.now()>=expiresAt)throw Error(nativeStarted?uncertainError:preparationError);};
  const markDispatched=()=>{check();nativeStarted=true;expiresAt=Date.now()+40000;clearTimeout(timer);timer=setTimeout(expire,40000);};
  const scope={signal:controller.signal,check,send:async(operation,validateSelection)=>{
   check();
   if(typeof nativeRequest==='function')return operation(markDispatched,validateSelection);
   await validateSelection();
   markDispatched();
   return operation();
  }};
  timer=setTimeout(expire,20000);
  try{return await Promise.race([downloadSelection(message,sender,scope),deadline]);}
  finally{closed=true;clearTimeout(timer);controller.abort();}
 }
 async function downloadSelection(message,sender,scope){
  const ctx=await context(message,sender),key='site-offers:'+ctx.tabId;
  scope.check();
  const storedOffers=(await api.storage.session.get(key))[key]||[];
  const offer=storedOffers.find(o=>o.key===message.key&&o.token===ctx.player.token&&o.frameId===ctx.frameId&&o.signature===ctx.signature&&Date.now()-o.created<TTL);
  scope.check();
  if(!offer)throw Error('This video or selection changed. Refresh the panel.');
  const permitted=prefs=>UdmMedia.policy.panelAllowed(message.output==='audio'?{container:'m4a'}:offer,prefs);
  if(!permitted(ctx.settings))throw Error('This format was disabled in video-panel settings. Refresh the panel.');
  const cookiePermission={permissions:['cookies'],origins:['http://*/*','https://*/*']};
  const sendCurrentSelection=(operation,allowCredentials)=>scope.send(operation,async()=>{
   scope.check();
   if(allowCredentials&&!await api.permissions.contains(cookiePermission))throw Error('Cookie permissions changed before download. Refresh the panel.');
   const latest=await context(message,sender);
   scope.check();
   if(latest.signature!==ctx.signature||!permitted(latest.settings)||(allowCredentials&&!latest.settings.cookies))throw Error('The video or panel settings changed before download.');
  });
  const options=offer.audioOptions||[],selection=message.audioKey===undefined?(options.find(x=>x.default)||options[0]):options.find(x=>x.key===message.audioKey);
  if(message.audioKey!==undefined&&(!selection||typeof message.audioKey!=='string'))throw Error('This audio track is no longer available. Refresh the panel.');
  const audioOnly=!!offer.onlyAudio||message.output==='audio';
  if(message.output!==undefined&&!['audio','video'].includes(message.output))throw Error('Invalid output selection.');
  if(audioOnly&&!offer.audioOnlyAvailable)throw Error('Audio-only output is not available for this selection.');
  const subtitle=message.subtitleKey?(offer.subtitleOptions||[]).find(s=>s.key===message.subtitleKey):null;
  if(message.subtitleKey!==undefined&&(typeof message.subtitleKey!=='string'||message.subtitleKey&&!subtitle))throw Error('This subtitle track is no longer available. Refresh the panel.');
  if(subtitle&&(audioOnly||offer.container==='ts'))throw Error('Choose MP4 video to include subtitles.');
  if(typeof warmMediaConnection==='function')warmMediaConnection();
  if(offer.kind==='direct'){
   let filename=(ctx.player.title||ctx.tab.title||'Video').replace(/[\\/:*?"<>|]/g,'_').slice(0,160);
   const ext='.'+UdmMedia.policy.mediaType(offer);
   if(!filename.toLowerCase().endsWith(ext.toLowerCase()))filename+=ext;
   return handoff({url:offer.url,filename,referrer:ctx.player.page,tabId:ctx.tabId,frameId:ctx.frameId,documentId:ctx.documentId},undefined,undefined,sendCurrentSelection);
  }
  const readPreferences=typeof mediaHandoffs!=='undefined'?mediaHandoffs?.preparePreferences?.():undefined;
  let plan=offer.plan,liveVariables=false,fetchedSubtitle;
  if(offer.kind==='dash-periods'){
   const catalog=storedOffers.find(o=>o.kind==='dash-period-catalog'&&o.key===offer.catalogKey&&o.token===ctx.player.token&&o.frameId===ctx.frameId&&o.signature===ctx.signature&&Date.now()-o.created<TTL);
   if(!catalog)throw Error('The DASH Period catalog expired. Refresh the panel.');
   plan=UdmMedia.dashPeriodSelection(catalog.presentation,{qualityIndex:offer.qualityIndex,audioOnly,audioKey:message.audioKey,subtitleKey:message.subtitleKey});
   if(subtitle)plan={...plan,subtitleName:subtitle.name,subtitleLanguage:subtitle.language};
  }
  if(offer.kind==='hls'){
   const playlists=[];
   // An external audio rendition can be downloaded without fetching the video.
   if(!audioOnly||!selection?.url)playlists.push((async()=>{
    const fetched=await fetchFullHls(offer.url,ctx,scope.signal);
    const video=UdmMedia.hls(fetched.text,fetched.url,{allowLive:true,imports:offer.hlsVariables});
    if(video.kind!=='media')throw Error('The selected HLS quality changed. Refresh the panel to read its current formats.');
    return {live:video.live,usesVariables:video.usesVariables,track:{kind:audioOnly?'audio':'video',playlist:fetched.url,segments:video.segments,...(offer.hlsVariables?{imports:offer.hlsVariables}:{})}};
   })());
   if(selection?.url)playlists.push((async()=>{
    const fetched=await fetchFullHls(selection.url,ctx,scope.signal);
    const audio=await UdmMedia.hlsAudioPlaylist(fetched,(address,signal)=>fetchFullHls(address,ctx,signal),scope.signal,selection.variables);
    return {live:audio.live,usesVariables:audio.usesVariables,track:{kind:'audio',playlist:audio.playlist,segments:audio.segments,...(audio.imports?{imports:audio.imports}:{})}};
   })());
   const [selected,captions]=await Promise.all([
    Promise.all(playlists),
    subtitle?.url?fetchFullHls(subtitle.url,ctx,scope.signal):null
   ]);
   fetchedSubtitle=captions;
   const live=selected.some(playlist=>playlist.live);
   liveVariables=selected.some(playlist=>playlist.usesVariables);
   const tracks=selected.map(playlist=>playlist.track);
   plan={type:'hls',height:audioOnly?0:offer.height,audioExpected:audioOnly||!!selection,tracks:live?tracks:tracks.map(({imports,...track})=>track),...(live?{live:true}:{} )};
  }
  if(offer.kind==='adaptive'&&selection?.track)plan={...plan,audioExpected:true,tracks:audioOnly?[selection.track]:[plan.tracks[0],selection.track]};
  else if(audioOnly&&offer.kind==='adaptive')plan={...plan,tracks:[{...plan.tracks[0],kind:'audio'}]};
  plan={...plan,height:audioOnly?0:plan.height,audioOnly,container:audioOnly?'m4a':offer.container||'mp4',...(selection?{audioName:selection.name,audioLanguage:selection.language}:{})};
  let desktopCapabilities;
  const requireDesktopCapability=async(capability,error)=>{
   if(!desktopCapabilities){
    const desktop=await (typeof nativeRequest==='function'?nativeRequest:m=>api.runtime.sendNativeMessage('com.udm.download_manager',m))({action:'hello'});
    desktopCapabilities=desktop?.ok&&Array.isArray(desktop.capabilities)?desktop.capabilities:[];
   }
   if(!desktopCapabilities.includes(capability))throw Error(error);
  };
  if(plan.periods)await requireDesktopCapability(plan.periods.length===1?'dash-single-period-timing':'dash-periods','Update the UDM desktop app before downloading DASH with Period timing.');
  if(plan.live&&plan.type==='dash'&&subtitle)plan={...plan,subtitleName:subtitle.name,subtitleLanguage:subtitle.language,tracks:[...plan.tracks,subtitle.track]};
  if(plan.live){
   await requireDesktopCapability(plan.type==='dash'?'live-dash':'live-hls','Update the UDM desktop app before recording this live stream.');
   if(plan.type==='dash'&&plan.tracks.some(track=>track.segmentList))await requireDesktopCapability('live-dash-segment-list','Update the UDM desktop app before recording this live DASH segment list.');
   if(plan.type==='dash'&&plan.tracks.some(track=>track.listIndexes))await requireDesktopCapability('live-dash-list-index-metadata','Update the UDM desktop app before recording this indexed DASH segment list.');
   if(plan.type==='dash'&&plan.tracks.some(track=>track.segmentIndexes))await requireDesktopCapability('live-dash-segment-index-metadata','Update the UDM desktop app before recording DASH with per-segment indexes.');
   if(plan.type==='dash'&&plan.tracks.some(track=>track.templateIndex))await requireDesktopCapability('live-dash-template-index','Update the UDM desktop app before recording this DASH index template.');
   if(plan.type==='dash'&&plan.tracks.some(track=>track.segmentBase))await requireDesktopCapability('live-dash-segment-base','Update the UDM desktop app before recording this indexed live DASH stream.');
   if(plan.type==='dash'&&plan.tracks.some(track=>track.durationBoundaryOverlap))await requireDesktopCapability('live-dash-duration-overlap','Update the UDM desktop app before recording DASH with a duration overlap.');
   if(plan.type==='dash'&&plan.tracks.some(track=>track.earliestPresentationOffset))await requireDesktopCapability('live-dash-ept-offset','Update the UDM desktop app before recording this DASH presentation offset.');
   if(plan.type==='dash'&&plan.tracks.some(track=>track.templateInitialization))await requireDesktopCapability('live-dash-template-initialization','Update the UDM desktop app before recording this DASH initialization format.');
   if(plan.type==='dash'&&plan.livePeriodSelection){
    await requireDesktopCapability('live-dash-period-selection','Update the UDM desktop app before selecting this live DASH stream.');
    scope.check();
    const current=await selectLiveDashPeriod(plan.manifest,ctx,true);
    scope.check();
    if(current!==plan.tracks[0].periodId)throw Error('The live stream changed while its menu was open; refresh its formats.');
   }
   if(subtitle)await requireDesktopCapability(plan.type==='dash'?'live-dash-mp4-webvtt':'live-hls-subtitles','Update the UDM desktop app before recording live subtitles.');
  }
  if(subtitle&&!plan.periods&&!(plan.live&&plan.type==='dash')){
   let track=subtitle.track;
    if(subtitle.url){const fetched=fetchedSubtitle??await fetchFullHls(subtitle.url,ctx,scope.signal),parsed=UdmMedia.hls(fetched.text,fetched.url,{allowLive:!!plan.live,imports:subtitle.variables});if(parsed.kind!=='media')throw Error('Subtitles require a WebVTT media playlist.');liveVariables=liveVariables||!!parsed.usesVariables;
    if(parsed.hasInit){
     if(plan.live){await requireDesktopCapability('live-hls-subtitle-map','Update the UDM desktop app before recording subtitles with separate headers.');}
     else{if(!parsed.segments[0]?.initialization||parsed.segments.length<2)throw Error('The subtitle header must precede its recorded segments.');await requireDesktopCapability('hls-subtitle-map','Update the UDM desktop app before downloading subtitles with separate headers.');}
    }
    track={kind:'subtitle',...(plan.live?{playlist:fetched.url,...(subtitle.variables?{imports:subtitle.variables}:{})}:parsed.hasInit?{webVttHeader:true}:{}),segments:parsed.segments};}
   plan={...plan,subtitleName:subtitle.name,subtitleLanguage:subtitle.language,tracks:[...plan.tracks,track]};
  }
  if(plan.live&&liveVariables)await requireDesktopCapability('hls-variables','Update the UDM desktop app before recording HLS playlists with variables.');
  if(!plan.live&&plan.type==='hls'&&plan.tracks.some(track=>track.kind==='subtitle'&&track.segments.filter(part=>part.initialization).length>1))await requireDesktopCapability('hls-recorded-subtitle-map-changes','Update the UDM desktop app before downloading HLS subtitles with changing headers.');
  if(!plan.live&&plan.type==='hls'&&plan.tracks.some(track=>track.kind!=='subtitle'&&(track.segments.filter(part=>part.initialization).length>1||track.segments.some(part=>part.mapIndex!==undefined)&&track.segments.some(part=>part.discontinuity)))){
   await requireDesktopCapability('hls-recorded-map-changes','Update the UDM desktop app before downloading HLS with changing initialization sections.');
  }
  if(!plan.live&&plan.type==='hls'&&plan.tracks.some(track=>track.kind==='subtitle'&&track.segments.some(part=>part.discontinuity))){
   const boundaries=track=>{let elapsed=0;const marks=[];for(const part of track.segments){if(part.initialization)continue;if(!(part.duration>0))throw Error('Subtitle discontinuities require segment durations on both tracks.');if(part.discontinuity)marks.push(elapsed);elapsed+=part.duration;}return marks;};
   const media=boundaries(plan.tracks[0]),captions=boundaries(plan.tracks.at(-1));if(media.length!==captions.length||media.some((time,index)=>time<=0||Math.abs(time-captions[index])>0.5))throw Error('Subtitle discontinuities do not align with the selected media track.');
   await requireDesktopCapability('hls-recorded-subtitle-discontinuity','Update the UDM desktop app before downloading HLS subtitles across discontinuities.');
  }
  if(!plan.live&&plan.type==='hls'&&plan.tracks.some(track=>track.kind!=='subtitle'&&track.segments.some(part=>part.discontinuity))){
   await requireDesktopCapability('hls-recorded-discontinuity','Update the UDM desktop app before downloading HLS with discontinuities.');
  }
  const groups=plan.periods||[plan];
  if(groups.some(group=>group.tracks.some(track=>track.mp4WebVtt)))await requireDesktopCapability('dash-mp4-webvtt','Update the UDM desktop app before downloading MP4 WebVTT captions.');
  if(groups.some(group=>group.tracks.some(track=>track.index))){
   const desktop=await (typeof nativeRequest==='function'?nativeRequest:m=>api.runtime.sendNativeMessage('com.udm.download_manager',m))({action:'preferences'});
   if(!desktop?.ok||desktop.adaptiveResources!==1)throw Error('Update the UDM desktop app before downloading indexed DASH media.');
   if(plan.periods){const periods=[],budget={requests:0,bytes:0,references:0};for(const period of plan.periods){scope.check();periods.push(period.tracks.some(track=>track.index)?await expandIndexes(period,ctx,scope.signal,budget):period);}plan={...plan,periods};}else plan=await expandIndexes(plan,ctx,scope.signal);
  }
  UdmMedia.validatePlanBudget(plan);
  scope.check();
  // Credentials are scoped by exact origin and remain optional.
  const originCookies={},originHeaders={};
  const allowCredentials=ctx.settings.cookies&&await api.permissions.contains(cookiePermission);
  const resources=plan.live&&plan.type==='dash'?[{url:plan.manifest},...plan.tracks.flatMap(track=>track.credentialUrls.map(url=>({url}))) ]:(plan.periods?plan.periods.flatMap(period=>period.tracks):plan.tracks).flatMap(track=>[...(plan.live?[{url:track.playlist}]:[]),...track.segments]);
  if(typeof requestContext!=='undefined'&&requestContext){for(const segment of resources){const origin=new URL(segment.url).origin;if(originHeaders[origin])continue;const captured=requestContext.resolve({url:segment.url,tabId:ctx.tabId,frameId:ctx.frameId,documentId:ctx.documentId},allowCredentials);if(captured?.method==='GET')originHeaders[origin]=captured.headers;}}
  if(plan.live&&plan.type==='dash')for(const resource of resources){const origin=new URL(resource.url).origin;if(!originHeaders[origin])originHeaders[origin]={'User-Agent':navigator.userAgent,'Referer':new URL(ctx.player.page).origin+'/'};}
  if(allowCredentials){
   const origins=[...new Set(resources.map(s=>new URL(s.url).origin))];if(origins.length>20)throw Error('Too many media origins.');
   for(const origin of origins){scope.check();originCookies[origin]=(await api.cookies.getAll({url:origin+'/'})).map(c=>c.name+'='+c.value).join('; ');}
  }
  const request={action:'adaptive',url:ctx.player.page,filename:ctx.player.title||'Video',plan,originCookies,originHeaders,userAgent:navigator.userAgent,referrer:new URL(ctx.player.page).origin+'/'};
  const binding=JSON.stringify({page:ctx.player.page,type:plan.type,height:plan.height,container:plan.container,audioOnly,live:!!plan.live,audioName:plan.audioName||'',audioLanguage:plan.audioLanguage||'',subtitleName:plan.subtitleName||'',subtitleLanguage:plan.subtitleLanguage||''});
  const guardedScope={...scope,readPreferences,send:operation=>sendCurrentSelection(operation,allowCredentials)};
  const result=typeof mediaHandoffs!=='undefined'&&mediaHandoffs?await mediaHandoffs.submit(request,guardedScope,binding,request.filename):await guardedScope.send((beforeSend,validateSelection)=>{
   if(typeof nativeRequest==='function')return nativeRequest(request,beforeSend,validateSelection);
   return api.runtime.sendNativeMessage('com.udm.download_manager',request);
  });
  if(!result?.ok)throw Error(result?.error||'UDM did not accept this video.');return result;
 }
 // Permission events and popup activation can arrive together. Serialize registration
 // so one request cannot unregister the scripts just registered by another.
 let registration=Promise.resolve();
 function sync(inject=false,tabId){
  const pending=registration.catch(()=>{}).then(()=>synchronize(inject,tabId));
  registration=pending;return pending;
 }
 async function synchronize(inject,tabId){
  if(!api.scripting.registerContentScripts||!api.permissions.getAll)return {};
  const permissions=await api.permissions.getAll(),matches=[...new Set((permissions.origins||[]).filter(x=>/^https?:\/\//.test(x)||x==='<all_urls>'))].sort();
  const registered=await api.scripting.getRegisteredContentScripts({ids:['udm-video-panels']});
  if(!matches.length){if(registered.length)await api.scripting.unregisterContentScripts({ids:['udm-video-panels']});return {};}
  const script={id:'udm-video-panels',matches,js:['media.js','content.js','selection.js'],allFrames:true,runAt:'document_idle',persistAcrossSessions:true};
  if(registered.length){
   if(JSON.stringify([...registered[0].matches].sort())!==JSON.stringify(matches))await api.scripting.updateContentScripts([script]);
  }else await api.scripting.registerContentScripts([script]);
  const activations=[];
  if(inject){
   const tabs=Number.isInteger(tabId)?[await api.tabs.get(tabId)]:await api.tabs.query({url:['http://*/*','https://*/*']});
   for(const tab of tabs){if(tab.incognito||!/^https?:/.test(tab.url||''))continue;
    try{const results=await api.scripting.executeScript({target:{tabId:tab.id,allFrames:true},files:script.js});activations.push({tabId:tab.id,frames:results.length});}
    catch(e){activations.push({tabId:tab.id,frames:0,error:e.message||String(e)});}
   }
  }
  return {activation:activations.find(x=>x.tabId===tabId),activations};
 }
 function clear(tabId){const pending=(queues.get(tabId)||Promise.resolve()).catch(()=>{}).then(()=>api.storage.session.remove(['site-media:'+tabId,'site-offers:'+tabId]));queues.set(tabId,pending);}
 function navigate(event){
  const id=event.tabId,keys=['site-media:'+id,'site-offers:'+id];
  for(const owner of catalogOwners.values()){
   if(owner.tabId===id&&!lifecycle.keep(owner,event))owner.controller.abort(Error('The video page changed. Reopen the panel.'));
  }
  const task=(queues.get(id)||Promise.resolve()).catch(()=>{}).then(async()=>{
   if(event.kind==='removed')return api.storage.session.remove(keys);
   const values=await api.storage.session.get(keys),updates={};
   for(const key of keys)if(values[key])updates[key]=values[key].filter(row=>lifecycle.keep({...row,tabId:id},event));
   if(Object.keys(updates).length)await api.storage.session.set(updates);
  });queues.set(id,task);return task.finally(()=>{if(queues.get(id)===task)queues.delete(id);});
 }
 function install(value,navigation){api=value;lifecycle=navigation?.supported?navigation:null;const filter={urls:['http://*/*','https://*/*']};api.webRequest?.onBeforeRequest?.addListener(requestStarted,filter);for(const event of [api.webRequest?.onCompleted,api.webRequest?.onErrorOccurred])event?.addListener(details=>requestStarts.delete(details.requestId),filter);lifecycle?.subscribe(navigate);api.permissions.onAdded?.addListener(()=>sync(true).catch(()=>{}));api.permissions.onRemoved?.addListener(()=>sync().catch(()=>{}));api.runtime.onStartup?.addListener(()=>sync(true).catch(()=>{}));api.runtime.onInstalled?.addListener(()=>sync(true).catch(()=>{}));sync().catch(()=>{});}
 return {install,observe,list,cancelFormats,download,sync,clear,readPlayer};
})();
