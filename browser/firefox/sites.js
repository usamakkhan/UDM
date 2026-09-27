/* Cross-site discovery is scoped to the clicked video, its document and frame. */
const UdmSites=(()=>{
 'use strict';
 let api;const queues=new Map(),TTL=180000;
 const setting=async()=>{const data=await api.storage.local.get(['settings','desktopPolicy']);return UdmMedia.policy.merge({excluded:[],cookies:false,...data.settings},data.desktopPolicy||{});};
 const excluded=(host,list)=>list.some(x=>host===x||host.endsWith('.'+x));
 function readPlayer(token){
  const roots=[document],videos=[];for(let i=0;i<roots.length&&i<40;i++){videos.push(...roots[i].querySelectorAll('video'));for(const e of Array.from(roots[i].querySelectorAll('*')).slice(0,3000))if(e.shadowRoot)roots.push(e.shadowRoot);}
  const video=videos.find(v=>v.getAttribute('data-udm-player')===token);if(!video)return null;
  let dailymotion=null;
  if(/(^|\.)dailymotion\.com$/.test(location.hostname)){
   // Read only playback fields from the actual player; never copy its account context.
   const metadata=globalThis.__PLAYER_CONFIG__?.metadata,stream=metadata?.stream;
   if(metadata&&stream)dailymotion={id:metadata.id,url:stream.url,type:stream.content_type,streamType:stream.stream_type,duration:Number(metadata.info?.duration)||0,title:metadata.info?.title||'',protected:!!(metadata.protected_delivery||stream.drm||stream.protected_delivery)};
  }
  return {manifests:globalThis.__udmMediaObserverV1?.read?.()||[],dailymotion,page:location.href,timeOrigin:performance.timeOrigin,token,stamp:video.getAttribute('data-udm-epoch'),current:video.currentSrc||video.src||'',sources:Array.from(video.querySelectorAll('source')).map(s=>({url:s.src,type:s.type})),height:video.videoHeight,width:video.videoWidth,duration:Number.isFinite(video.duration)?video.duration:0,encrypted:!!video.mediaKeys||video.hasAttribute('data-udm-encrypted'),videoCount:videos.filter(v=>{const r=v.getBoundingClientRect();return r.width>=120&&r.height>=70;}).length,title:document.title};
 }
 async function context(message,sender){
  if(!sender.tab||!Number.isInteger(sender.tab.id)||!Number.isInteger(sender.frameId??0)||!/^[a-z0-9-]{1,80}$/i.test(message.token||''))throw Error('Use the download panel on the video.');
  if(typeof browserControls!=='undefined'&&await browserControls?.isDisabled(sender.tab.id))throw Error('UDM is disabled on this tab.');
  const tab=await api.tabs.get(sender.tab.id);if(tab.incognito)throw Error('Video capture is disabled in private windows.');
  const settings=await setting();if(settings.panelEnabled===false||UdmMedia.policy.blocked(tab.url,settings)||UdmMedia.policy.blocked(sender.url,settings))throw Error('Video panels are disabled on this site.');
  const frameId=sender.frameId??0;const result=await api.scripting.executeScript({target:{tabId:tab.id??sender.tab.id,frameIds:[frameId]},world:'MAIN',func:readPlayer,args:[message.token]});
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
  return {tab,tabId:sender.tab.id,frameId,documentId,player,settings,signature:[tab.url,documentId,player.timeOrigin||'',player.page,player.token,player.stamp,player.current,dm?.id||'',dm?.url||''].join('\n')};
 }
 async function observe(event){
  if(event.tabId<0||event.statusCode>=400)return;let target;try{target=UdmMedia.url(event.url);}catch{return;}
  if(new URL(target).hostname.endsWith('.googlevideo.com'))return;
  const mime=event.responseHeaders?.find(h=>h.name.toLowerCase()==='content-type')?.value||'',kind=UdmMedia.kind(target,mime);
  if(!kind||kind==='fragment')return;
  const pending=(queues.get(event.tabId)||Promise.resolve()).catch(()=>{}).then(async()=>{
   const tab=await api.tabs.get(event.tabId);if(tab.incognito)return;
   const settings=await setting();if(settings.panelEnabled===false||UdmMedia.policy.blocked(tab.url,settings))return;
   const key='site-media:'+event.tabId,items=(await api.storage.session.get(key))[key]||[];
   const item={url:target,kind,mime,frameId:event.frameId??0,documentId:event.documentId||'',page:event.documentUrl||event.initiator||'',time:Date.now()};
   await api.storage.session.set({[key]:[...items.filter(x=>Date.now()-x.time<TTL&&!(x.url===target&&x.frameId===item.frameId)),item].slice(-60)});
  });queues.set(event.tabId,pending);try{await pending;}finally{if(queues.get(event.tabId)===pending)queues.delete(event.tabId);}
 }
 async function fetchText(address,ctx){
  const target=UdmMedia.url(address);
  const captured=ctx?.player.manifests?.find(item=>item.url===target&&item.page===ctx.player.page&&Date.now()-item.time<120000&&typeof item.text==='string'&&item.text.length<=2*1024*1024);
  if(captured)return {text:captured.text,url:target};
  if(!await api.permissions.contains({origins:[new URL(target).origin+'/*']}))throw Error('Allow video detection for this site’s media host in UDM’s extension settings.');
  const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),12000);
  try{const response=await fetch(target,{credentials:'omit',signal:controller.signal,cache:'no-store'});if(!response.ok)throw Error('The playlist server returned HTTP '+response.status+'.');if(!await api.permissions.contains({origins:[new URL(response.url||target).origin+'/*']})){await response.body?.cancel();throw Error('The redirected media host needs site permission.');}const reader=response.body.getReader(),decoder=new TextDecoder();let text='',size=0;
   for(;;){const item=await reader.read();if(item.done)break;size+=item.value.length;if(size>2e6){await reader.cancel();throw Error('Playlist exceeds the size limit.');}text+=decoder.decode(item.value,{stream:true});}text+=decoder.decode();return {text,url:UdmMedia.url(response.url||target)};
  }finally{clearTimeout(timer);}
 }
 async function list(message,sender){
  const ctx=await context(message,sender),p=ctx.player,candidates=[],notes=[];
  const sources=p.dailymotion?[{url:p.dailymotion.url,type:p.dailymotion.type}]:[{url:p.current,type:''},...p.sources];
  for(const s of sources)try{if(!s.url)continue;const target=UdmMedia.url(s.url,p.page),kind=UdmMedia.kind(target,s.type)||(s.url===p.current&&p.width>0?'direct':'');if(kind)candidates.push({url:target,kind,owned:true});}catch{}
  // Never mix background requests from other frames/players into a direct source.
  if(!candidates.length&&p.current.startsWith('blob:')){
   if(p.videoCount!==1)throw Error('Several videos share this frame. UDM cannot safely associate their streaming requests yet.');
   const key='site-media:'+ctx.tabId;
   // The player may finish loading before the webRequest capture reaches storage.
   // Read the catalog after captures already observed for this tab have settled.
   await (queues.get(ctx.tabId)||Promise.resolve()).catch(()=>{});
   const observed=(await api.storage.session.get(key))[key]||[];
   for(const entry of p.manifests||[]){if(entry.page!==p.page||Date.now()-entry.time>=TTL)continue;try{const target=UdmMedia.url(entry.url),kind=UdmMedia.kind(target,entry.type);if(['hls','dash'].includes(kind))candidates.push({url:target,kind,owned:true});}catch{}}
   const matches=observed.filter(x=>x.frameId===ctx.frameId&&Date.now()-x.time<TTL&&
    // Frame IDs survive navigation; document identity does not. Older browsers
    // use the page's navigation timestamp and URL as a conservative fallback.
    (ctx.documentId?x.documentId===ctx.documentId:!x.documentId&&Number.isFinite(p.timeOrigin)&&x.time>=p.timeOrigin)&&(x.page===p.page||x.page===new URL(p.page).origin)&&['hls','dash'].includes(x.kind));
   candidates.push(...matches.slice(-8));
   if(candidates.length)notes.push('These playlists were observed in this player’s frame. Check the source label before choosing; embedded ads cannot always be distinguished.');
  }
  const choices=[],coveredPlaylists=new Set();
  for(const item of [...new Map(candidates.map(x=>[x.url,x])).values()].slice(0,8)){
   try{
    const sourceName=decodeURIComponent(new URL(item.url).pathname.split('/').pop()||new URL(item.url).hostname).slice(0,65);
    if(item.kind==='direct'){const own=item.url===p.current;choices.push({kind:'direct',url:item.url,height:own?p.height:0,label:(own&&p.height?p.height+'p':'Original quality')+' · '+(new URL(item.url).pathname.split('.').pop()||'video').toUpperCase().slice(0,8),source:sourceName});}
    else if(item.kind==='hls'){
     const fetched=await fetchText(item.url,ctx),parsed=UdmMedia.hls(fetched.text,fetched.url);
     if(parsed.kind==='master')for(const variant of parsed.variants){coveredPlaylists.add(variant.url);let audioOptions;try{audioOptions=UdmMedia.hlsAudio(parsed,variant);}catch(e){notes.push(e.message);continue;}
      for(const track of audioOptions)if(track.url)coveredPlaylists.add(track.url);
      choices.push({kind:'hls',url:variant.url,audioOptions,height:variant.height,bandwidth:variant.bandwidth,codecs:variant.codecs,label:(variant.height?variant.height+'p':'Original quality')+' · HLS',source:sourceName});
     }else choices.push({manifestUrl:fetched.url,kind:'adaptive',height:0,label:'Original quality · HLS',source:sourceName,plan:{type:'hls',height:0,audioExpected:false,tracks:[{kind:'video',segments:parsed.segments}]}});
    }else if(item.kind==='dash'){const fetched=await fetchText(item.url,ctx);for(const choice of UdmMedia.dash(fetched.text,fetched.url))choices.push({kind:'adaptive',...choice,source:sourceName});}
   }catch(e){notes.push(e.message);}
  }
  const current=await context(message,sender);if(current.signature!==ctx.signature)throw Error('The video changed while reading its formats.');
  const key='site-offers:'+ctx.tabId,offers=(await api.storage.session.get(key))[key]||[];
  const platform=p.dailymotion?'Dailymotion':new URL(p.page).hostname.replace(/^www\./,'');
  const formats=choices.filter(choice=>!choice.manifestUrl||!coveredPlaylists.has(choice.manifestUrl)).flatMap(choice=>{
   if(choice.kind==='direct')return [{...choice,source:platform}];
   // Offer transport-stream output only for explicitly compatible HLS codecs.
   const codecList=(choice.codecs||'').split(',').map(c=>c.trim()).filter(Boolean);
   const ts=choice.kind==='hls'&&codecList.length>0&&codecList.every(c=>/^(avc[13]|hvc1|hev1|mp4a)(\.|$)/i.test(c));
   return (ts?['mp4','ts']:['mp4']).map(container=>({...choice,container,source:p.dailymotion?'':choice.source,
    detail:(choice.kind==='hls'||choice.plan?.type==='hls'?'HLS':'DASH')+' • '+new URL(choice.url||choice.plan?.tracks?.[0]?.segments?.[0]?.url||p.page).hostname,
    label:platform+' · '+container.toUpperCase()+' · '+(choice.height?choice.height+'p'+(choice.height>=720?' HD':''):'Original quality')+(choice.bandwidth>0?' · '+Math.round(choice.bandwidth/1000)+' kbps':'')+(choice.audioOptions?.length>1?' · '+choice.audioOptions.length+' audio tracks':choice.audioOptions?.length===1?' · '+choice.audioOptions[0].label:'')}));
  });
  const records=formats.slice(0,80).sort((a,b)=>b.height-a.height).map(choice=>({...choice,key:crypto.randomUUID(),token:p.token,frameId:ctx.frameId,signature:ctx.signature,created:Date.now()}));
  if(JSON.stringify(records).length>4000000)throw Error('This audio/video catalog is too large for the current browser cache.');
  await api.storage.session.set({[key]:[...offers.filter(o=>Date.now()-o.created<TTL&&!(o.token===p.token&&o.frameId===ctx.frameId)),...records].slice(-100)});
  if(!records.length&& !notes.length)notes.push('No supported playlist is captured for this player. Reload the video page, play the video, then Refresh.');
  return {ok:true,choices:records.map(({key,label,source,height,container,detail,audioOptions})=>({key,label,source,height,container,detail,...(audioOptions?.length?{audioOptions:audioOptions.map(({key,label,default:preferred})=>({key,label,default:preferred}))}:{})})),note:[...new Set(notes)].join(' ')};
 }
 async function download(message,sender){
  const ctx=await context(message,sender),key='site-offers:'+ctx.tabId;
  const offer=((await api.storage.session.get(key))[key]||[]).find(o=>o.key===message.key&&o.token===ctx.player.token&&o.frameId===ctx.frameId&&o.signature===ctx.signature&&Date.now()-o.created<TTL);
  if(!offer)throw Error('This video or selection changed. Refresh the panel.');
  const options=offer.audioOptions||[],selection=message.audioKey===undefined?(options.find(x=>x.default)||options[0]):options.find(x=>x.key===message.audioKey);
  if(message.audioKey!==undefined&&(!selection||typeof message.audioKey!=='string'))throw Error('This audio track is no longer available. Refresh the panel.');
  if(offer.kind==='direct'){const latest=await context(message,sender);if(latest.signature!==ctx.signature)throw Error('The video changed before download.');let filename=(ctx.player.title||ctx.tab.title||'Video').replace(/[\\/:*?"<>|]/g,'_').slice(0,160);const ext=/\.(mp4|webm|mov|m4v|ogv)$/i.exec(new URL(offer.url).pathname)?.[0]||'.mp4';if(!filename.toLowerCase().endsWith(ext.toLowerCase()))filename+=ext;return handoff({url:offer.url,filename,referrer:ctx.player.page,tabId:ctx.tabId,frameId:ctx.frameId,documentId:ctx.documentId});}
  let plan=offer.plan;
  if(offer.kind==='hls'){const v=await fetchText(offer.url,ctx),video=UdmMedia.hls(v.text,v.url);if(video.kind!=='media')throw Error('Nested HLS master playlists need additional support.');const tracks=[{kind:'video',segments:video.segments}];if(selection?.url){const a=await fetchText(selection.url,ctx),audio=UdmMedia.hls(a.text,a.url);if(audio.kind!=='media')throw Error('Unsupported audio playlist.');tracks.push({kind:'audio',segments:audio.segments});}plan={type:'hls',height:offer.height,audioExpected:!!selection,tracks};}
  if(offer.kind==='adaptive'&&selection?.track)plan={...plan,audioExpected:true,tracks:[plan.tracks[0],selection.track]};
  plan={...plan,container:offer.container||'mp4',...(selection?{audioName:selection.name,audioLanguage:selection.language}:{})};
  if(JSON.stringify(plan).length>200000)throw Error('This playlist is too large for the current browser handoff.');
  const latest=await context(message,sender);if(latest.signature!==ctx.signature)throw Error('The video changed before download.');
  // Credentials are scoped by exact origin and remain optional.
  const originCookies={},originHeaders={};
  const allowCredentials=ctx.settings.cookies&&await api.permissions.contains({permissions:['cookies'],origins:['http://*/*','https://*/*']});
  if(typeof requestContext!=='undefined'&&requestContext){for(const segment of plan.tracks.flatMap(track=>track.segments)){const origin=new URL(segment.url).origin;if(originHeaders[origin])continue;const captured=requestContext.resolve({url:segment.url,tabId:ctx.tabId,frameId:ctx.frameId,documentId:ctx.documentId},allowCredentials);if(captured?.method==='GET')originHeaders[origin]=captured.headers;}}
  if(allowCredentials){
   const origins=[...new Set(plan.tracks.flatMap(t=>t.segments.map(s=>new URL(s.url).origin)))];if(origins.length>20)throw Error('Too many media origins.');
   for(const origin of origins)originCookies[origin]=(await api.cookies.getAll({url:origin+'/'})).map(c=>c.name+'='+c.value).join('; ');
  }
  const result=await (typeof nativeRequest==='function'?nativeRequest:message=>api.runtime.sendNativeMessage('com.udm.download_manager',message))({action:'adaptive',url:ctx.player.page,filename:ctx.player.title||'Video',plan,originCookies,originHeaders,userAgent:navigator.userAgent,referrer:new URL(ctx.player.page).origin+'/'});
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
 function install(value){api=value;api.permissions.onAdded?.addListener(()=>sync(true).catch(()=>{}));api.permissions.onRemoved?.addListener(()=>sync().catch(()=>{}));api.runtime.onStartup?.addListener(()=>sync(true).catch(()=>{}));api.runtime.onInstalled?.addListener(()=>sync(true).catch(()=>{}));sync().catch(()=>{});}
 return {install,observe,list,download,sync,clear,readPlayer};
})();
