/* Cross-site discovery is scoped to the clicked video, its document and frame. */
const UdmSites=(()=>{
 'use strict';
 let api;const queues=new Map(),TTL=180000;
 const setting=async()=>({excluded:[],cookies:false,...(await api.storage.local.get('settings')).settings});
 const excluded=(host,list)=>list.some(x=>host===x||host.endsWith('.'+x));
 function readPlayer(token){
  const roots=[document],videos=[];for(let i=0;i<roots.length&&i<40;i++){videos.push(...roots[i].querySelectorAll('video'));for(const e of Array.from(roots[i].querySelectorAll('*')).slice(0,3000))if(e.shadowRoot)roots.push(e.shadowRoot);}
  const video=videos.find(v=>v.getAttribute('data-udm-player')===token);if(!video)return null;
  return {page:location.href,token,stamp:video.getAttribute('data-udm-epoch'),current:video.currentSrc||video.src||'',sources:Array.from(video.querySelectorAll('source')).map(s=>({url:s.src,type:s.type})),height:video.videoHeight,width:video.videoWidth,duration:Number.isFinite(video.duration)?video.duration:0,encrypted:!!video.mediaKeys||video.hasAttribute('data-udm-encrypted'),videoCount:videos.filter(v=>{const r=v.getBoundingClientRect();return r.width>=120&&r.height>=70;}).length,title:document.title};
 }
 async function context(message,sender){
  if(!sender.tab||!Number.isInteger(sender.tab.id)||!Number.isInteger(sender.frameId??0)||!/^[a-z0-9-]{1,80}$/i.test(message.token||''))throw Error('Use the download panel on the video.');
  const tab=await api.tabs.get(sender.tab.id);if(tab.incognito)throw Error('Video capture is disabled in private windows.');
  const settings=await setting();if(excluded(new URL(tab.url).hostname,settings.excluded))throw Error('Video panels are disabled on this site.');
  const frameId=sender.frameId??0;const result=await api.scripting.executeScript({target:{tabId:tab.id??sender.tab.id,frameIds:[frameId]},func:readPlayer,args:[message.token]});
  const player=result.find(r=>r.frameId===frameId)?.result;
  if(!player||player.page!==sender.url||player.page!==message.page)throw Error('The video page changed. Reopen the panel.');
  UdmMedia.url(player.page);if(player.encrypted)throw Error('This player uses encrypted or DRM-protected media.');
  return {tab,tabId:sender.tab.id,frameId,player,settings,signature:[player.page,player.token,player.stamp,player.current].join('\n')};
 }
 async function observe(event){
  if(event.tabId<0||event.statusCode>=400)return;let target;try{target=UdmMedia.url(event.url);}catch{return;}
  if(new URL(target).hostname.endsWith('.googlevideo.com'))return;
  const mime=event.responseHeaders?.find(h=>h.name.toLowerCase()==='content-type')?.value||'',kind=UdmMedia.kind(target,mime);
  if(!kind||kind==='fragment')return;
  const pending=(queues.get(event.tabId)||Promise.resolve()).catch(()=>{}).then(async()=>{
   const tab=await api.tabs.get(event.tabId);if(tab.incognito)return;
   const settings=await setting();if(excluded(new URL(tab.url).hostname,settings.excluded))return;
   const key='site-media:'+event.tabId,items=(await api.storage.session.get(key))[key]||[];
   const item={url:target,kind,mime,frameId:event.frameId??0,documentId:event.documentId||'',page:event.documentUrl||event.initiator||'',time:Date.now()};
   await api.storage.session.set({[key]:[...items.filter(x=>Date.now()-x.time<TTL&&!(x.url===target&&x.frameId===item.frameId)),item].slice(-60)});
  });queues.set(event.tabId,pending);try{await pending;}finally{if(queues.get(event.tabId)===pending)queues.delete(event.tabId);}
 }
 async function fetchText(address){
  const target=UdmMedia.url(address);
  if(!await api.permissions.contains({origins:[new URL(target).origin+'/*']}))throw Error('Allow video detection for this site’s media host in UDM’s extension settings.');
  const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),12000);
  try{const response=await fetch(target,{credentials:'omit',signal:controller.signal,cache:'no-store'});if(!response.ok)throw Error('The playlist server returned HTTP '+response.status+'.');if(!await api.permissions.contains({origins:[new URL(response.url||target).origin+'/*']})){await response.body?.cancel();throw Error('The redirected media host needs site permission.');}const reader=response.body.getReader(),decoder=new TextDecoder();let text='',size=0;
   for(;;){const item=await reader.read();if(item.done)break;size+=item.value.length;if(size>2e6){await reader.cancel();throw Error('Playlist exceeds the size limit.');}text+=decoder.decode(item.value,{stream:true});}text+=decoder.decode();return {text,url:UdmMedia.url(response.url||target)};
  }finally{clearTimeout(timer);}
 }
 async function list(message,sender){
  const ctx=await context(message,sender),p=ctx.player,candidates=[],notes=[];
  const sources=[{url:p.current,type:''},...p.sources];
  for(const s of sources)try{if(!s.url)continue;const target=UdmMedia.url(s.url,p.page),kind=UdmMedia.kind(target,s.type)||(s.url===p.current&&p.width>0?'direct':'');if(kind)candidates.push({url:target,kind,owned:true});}catch{}
  // Never mix background requests from other frames/players into a direct source.
  if(!candidates.length&&p.current.startsWith('blob:')){
   if(p.videoCount!==1)throw Error('Several videos share this frame. UDM cannot safely associate their streaming requests yet.');
   const key='site-media:'+ctx.tabId,observed=(await api.storage.session.get(key))[key]||[];
   const matches=observed.filter(x=>x.frameId===ctx.frameId&&Date.now()-x.time<TTL&&(x.page===p.page||x.page===new URL(p.page).origin)&&['hls','dash'].includes(x.kind));
   candidates.push(...matches.slice(-8));
   if(candidates.length)notes.push('These playlists were observed in this player’s frame. Check the source label before choosing; embedded ads cannot always be distinguished.');
  }
  const choices=[];
  for(const item of [...new Map(candidates.map(x=>[x.url,x])).values()].slice(0,8)){
   try{
    const sourceName=decodeURIComponent(new URL(item.url).pathname.split('/').pop()||new URL(item.url).hostname).slice(0,65);
    if(item.kind==='direct'){const own=item.url===p.current;choices.push({kind:'direct',url:item.url,height:own?p.height:0,label:(own&&p.height?p.height+'p':'Original quality')+' · '+(new URL(item.url).pathname.split('.').pop()||'video').toUpperCase().slice(0,8),source:sourceName});}
    else if(item.kind==='hls'){
     const fetched=await fetchText(item.url),parsed=UdmMedia.hls(fetched.text,fetched.url);
     if(parsed.kind==='master')for(const variant of parsed.variants){const audio=parsed.audio.filter(a=>a['GROUP-ID']===variant.audioGroup).sort((a,b)=>(b.DEFAULT==='YES')-(a.DEFAULT==='YES'))[0];
      choices.push({kind:'hls',url:variant.url,audioUrl:audio?.url||'',height:variant.height,label:(variant.height?variant.height+'p':'Original quality')+' · HLS'+(audio?' · '+(audio.LANGUAGE||audio.NAME||'audio'):''),source:sourceName});
     }else choices.push({kind:'adaptive',height:0,label:'Original quality · HLS',source:sourceName,plan:{type:'hls',height:0,audioExpected:false,tracks:[{kind:'video',segments:parsed.segments}]}});
    }else if(item.kind==='dash'){const fetched=await fetchText(item.url);for(const choice of UdmMedia.dash(fetched.text,fetched.url))choices.push({kind:'adaptive',...choice,source:sourceName});}
   }catch(e){notes.push(e.message);}
  }
  const current=await context(message,sender);if(current.signature!==ctx.signature)throw Error('The video changed while reading its formats.');
  const key='site-offers:'+ctx.tabId,offers=(await api.storage.session.get(key))[key]||[];
  const records=choices.slice(0,80).sort((a,b)=>b.height-a.height).map(choice=>({...choice,key:crypto.randomUUID(),token:p.token,frameId:ctx.frameId,signature:ctx.signature,created:Date.now()}));
  await api.storage.session.set({[key]:[...offers.filter(o=>Date.now()-o.created<TTL&&!(o.token===p.token&&o.frameId===ctx.frameId)),...records].slice(-100)});
  if(!records.length&& !notes.length)notes.push('Play the video first. This player has not exposed a supported file or playlist yet.');
  return {ok:true,choices:records.map(({key,label,source,height})=>({key,label,source,height})),note:[...new Set(notes)].join(' ')};
 }
 async function download(message,sender){
  const ctx=await context(message,sender),key='site-offers:'+ctx.tabId;
  const offer=((await api.storage.session.get(key))[key]||[]).find(o=>o.key===message.key&&o.token===ctx.player.token&&o.frameId===ctx.frameId&&o.signature===ctx.signature&&Date.now()-o.created<TTL);
  if(!offer)throw Error('This video or selection changed. Refresh the panel.');
  if(offer.kind==='direct'){let filename=(ctx.player.title||ctx.tab.title||'Video').replace(/[\\/:*?"<>|]/g,'_').slice(0,160);const ext=/\.(mp4|webm|mov|m4v|ogv)$/i.exec(new URL(offer.url).pathname)?.[0]||'.mp4';if(!filename.toLowerCase().endsWith(ext.toLowerCase()))filename+=ext;return handoff({url:offer.url,filename,referrer:ctx.player.page});}
  let plan=offer.plan;
  if(offer.kind==='hls'){const v=await fetchText(offer.url),video=UdmMedia.hls(v.text,v.url);if(video.kind!=='media')throw Error('Nested HLS master playlists need additional support.');const tracks=[{kind:'video',segments:video.segments}];if(offer.audioUrl){const a=await fetchText(offer.audioUrl),audio=UdmMedia.hls(a.text,a.url);if(audio.kind!=='media')throw Error('Unsupported audio playlist.');tracks.push({kind:'audio',segments:audio.segments});}plan={type:'hls',height:offer.height,audioExpected:!!offer.audioUrl,tracks};}
  if(JSON.stringify(plan).length>200000)throw Error('This playlist is too large for the current browser handoff.');
  const latest=await context(message,sender);if(latest.signature!==ctx.signature)throw Error('The video changed before download.');
  // Credentials are scoped by exact origin and remain optional.
  const originCookies={};
  if(ctx.settings.cookies&&await api.permissions.contains({permissions:['cookies'],origins:['http://*/*','https://*/*']})){
   const origins=[...new Set(plan.tracks.flatMap(t=>t.segments.map(s=>new URL(s.url).origin)))];if(origins.length>20)throw Error('Too many media origins.');
   for(const origin of origins)originCookies[origin]=(await api.cookies.getAll({url:origin+'/'})).map(c=>c.name+'='+c.value).join('; ');
  }
  const result=await api.runtime.sendNativeMessage('com.udm.download_manager',{action:'adaptive',url:ctx.player.page,filename:ctx.player.title||'Video',plan,originCookies,userAgent:navigator.userAgent,referrer:new URL(ctx.player.page).origin+'/'});
  if(!result?.ok)throw Error(result?.error||'UDM did not accept this video.');return result;
 }
 async function sync(inject=false){
  if(!api.scripting.registerContentScripts||!api.permissions.getAll)return;
  const permissions=await api.permissions.getAll(),matches=(permissions.origins||[]).filter(x=>/^https?:\/\//.test(x)||x==='<all_urls>');
  const registered=await api.scripting.getRegisteredContentScripts({ids:['udm-video-panels']});if(registered.length)await api.scripting.unregisterContentScripts({ids:['udm-video-panels']});
  if(!matches.length)return;
  await api.scripting.registerContentScripts([{id:'udm-video-panels',matches,js:['media.js','content.js'],allFrames:true,runAt:'document_idle',persistAcrossSessions:true}]);
  if(inject)for(const tab of await api.tabs.query({url:['http://*/*','https://*/*']})){if(tab.incognito)continue;try{await api.scripting.executeScript({target:{tabId:tab.id,allFrames:true},files:['media.js','content.js']});}catch{}}
 }
 function clear(tabId){const pending=(queues.get(tabId)||Promise.resolve()).catch(()=>{}).then(()=>api.storage.session.remove(['site-media:'+tabId,'site-offers:'+tabId]));queues.set(tabId,pending);}
 function install(value){api=value;api.permissions.onAdded?.addListener(()=>sync(true).catch(()=>{}));api.permissions.onRemoved?.addListener(()=>sync().catch(()=>{}));api.runtime.onStartup?.addListener(()=>sync().catch(()=>{}));sync().catch(()=>{});}
 return {install,observe,list,download,sync,clear,readPlayer};
})();
