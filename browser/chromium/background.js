'use strict';
if(typeof importScripts==='function')importScripts('formats.js','media.js','sites.js');
const api = globalThis.browser || chrome;
const HOST = 'com.udm.download_manager';
const defaults = { capture: false, cookies: false, excluded: [], extensions: ['zip','7z','rar','iso','exe','msi','pdf','mp4','mkv','mp3','flac'] };
const acceptable = url => /^https?:\/\//i.test(url || '');
const youtubeId = address => {try{const u=new URL(address);if(u.protocol!=='https:'||!['www.youtube.com','youtube.com','m.youtube.com','www.youtube-nocookie.com'].includes(u.hostname))return '';const id=u.pathname==='/watch'?u.searchParams.get('v'):/^\/(?:embed|shorts)\/([\w-]{11})/.exec(u.pathname)?.[1];return /^[\w-]{11}$/.test(id||'')?id:'';}catch{return '';}};
const youtubePage = address => !!youtubeId(address);
const mediaQueues = new Map();
const qualityItags = {137:1080,136:720,135:480,134:360,133:240,160:144};
function streamInfo(raw, type) {
  const u=new URL(raw);
  if(!u.hostname.endsWith('.googlevideo.com') || u.pathname!=='/videoplayback' || u.searchParams.has('sq') || u.searchParams.get('ump')==='1') return null;
  const itag=Number(u.searchParams.get('itag')), size=Number(u.searchParams.get('clen'));
  if(!Number.isSafeInteger(itag)||itag<=0||!Number.isSafeInteger(size)||size<=0)return null;
  const signed=(u.searchParams.get('sparams')||'').split(',');
  for(const key of ['range','rn','rbuf']) {if(signed.includes(key)&&u.searchParams.has(key))return null;u.searchParams.delete(key);}
  return {url:u.href,type,itag,size,height:qualityItags[itag]||0,observedAt:Date.now()};
}
async function mediaContext(message,sender) {
  if(!youtubePage(message.url))throw new Error('Open a YouTube video first.');
  const tabId=sender.tab?.id ?? message.tabId;
  const tab=await api.tabs.get(tabId);
  const id=youtubeId(message.url),frameId=sender.tab?(sender.frameId??0):0;
  if(sender.tab&&youtubeId(sender.url)!==id)throw new Error('The message did not come from this video page.');
  if(tab.incognito)throw new Error('Media capture is disabled in private windows.');
  if(frameId===0&&youtubeId(tab.url)!==id)throw new Error('The video page changed. Open UDM on the current page again.');
  return {tabId,tab,id,frameId};
}
async function availableMedia(message,sender) {
  const context=await mediaContext(message,sender);
  const results=await api.scripting.executeScript({target:{tabId:context.tabId,frameIds:[context.frameId]},world:'MAIN',func:readYouTubeFormats,args:[context.id]});
  await mediaContext(message,sender); // Navigation may happen while the snapshot is read.
  const snapshot=results.find(r=>r.frameId===context.frameId)?.result;
  if(snapshot?.live)throw new Error('Live streams are not supported.');
  const key='media:'+context.tabId;
  const observed=(await api.storage.session.get(key))[key]||[];
  const choices=UdmFormats.choices(UdmFormats.attachObserved(snapshot,observed),context.id);
  if(!choices.length)throw new Error('No downloadable qualities detected yet. Play this video, then refresh the list.');
  return {...context,choices,snapshot};
}
async function sabrFor(context,choice){
  const video=context.snapshot?.formats?.find(f=>f.id===choice.formatId&&!f.muxed&&/^video\/mp4\b/.test(f.mime));
  const audio=context.snapshot?.formats?.find(f=>f.audioDefault!==false&&/^audio\/mp4\b/.test(f.mime)&&/mp4a/.test(f.mime));
  if(!video||!audio||!/^\d+$/.test(video.lastModified||'')||!/^\d+$/.test(audio.lastModified||''))return null;
  const result=await api.scripting.executeScript({target:{tabId:context.tabId,frameIds:[context.frameId]},world:'MAIN',func:id=>globalThis.__udmCaptureV1?.session?.(id),args:[context.id]});
  const session=result.find(r=>r.frameId===context.frameId)?.result;
  if(!session||session.videoId!==context.id||typeof session.body!=='string'||session.body.length>175000||!session.url)return null;
  const select=f=>({id:f.id,lastModified:f.lastModified,xtags:f.xtags||'',mime:f.mime});
  return {...session,durationMs:context.snapshot.durationMs,video:select(video),audio:select(audio)};
}
async function mediaHandoff(message, sender) {
  let context=await availableMedia(message,sender);
  let choice=context.choices.find(x=>x.key===message.formatKey&&x.height===Number(message.height));
  if(!choice)throw new Error('That quality is no longer available. Refresh the list and choose again.');
  let sabr=choice.videoUrl?null:await sabrFor(context,choice);
  if(!choice.videoUrl&&!sabr){
    await api.scripting.executeScript({target:{tabId:context.tabId,frameIds:[context.frameId]},world:'MAIN',func:(id,height)=>globalThis.__udmCaptureV1?.prepare(id,height),args:[context.id,choice.height]});
    for(let attempt=0;attempt<8&&!choice.videoUrl&&!sabr;attempt++){
      await new Promise(resolve=>setTimeout(resolve,400));
      const latest=await availableMedia(message,sender);
      context=latest;
      choice=latest.choices.find(x=>x.key===message.formatKey&&x.height===Number(message.height));
      if(!choice)throw new Error('The video or available formats changed. Refresh the list.');
      if(!choice.videoUrl)sabr=await sabrFor(context,choice);
    }
    if(!choice.videoUrl&&!sabr)throw new Error('This quality is listed by the player, but UDM has not captured a downloadable stream yet. Play the video at this quality, then retry.');
  }
  await mediaContext(message,sender);
  const result=await api.runtime.sendNativeMessage(HOST,{action:sabr?'sabr':'media',sabr,url:message.url,filename:message.title||context.tab.title||'YouTube video',height:choice.height,pixelHeight:choice.pixelHeight||0,formatId:choice.formatId,exactQuality:true,videoUrl:choice.videoUrl,audioUrl:choice.audioUrl,referrer:message.url,userAgent:navigator.userAgent});
  if(!result?.ok)throw new Error(result?.error||'UDM did not accept the video.');
  await report('');return result;
}
const report = async text => {
  await api.storage.local.set({ lastError: text });
  await api.action.setBadgeText({text: text ? '!' : ''});
};
async function handoff(item) {
  if (!acceptable(item.url)) throw new Error('Only HTTP and HTTPS browser downloads can be handed off.');
  const settings = { ...defaults, ...(await api.storage.local.get('settings')).settings };
  let cookies = '';
  if (settings.cookies && await api.permissions.contains({permissions:['cookies'],origins:['http://*/*','https://*/*']})) {
    cookies = (await api.cookies.getAll({url:item.url})).map(c => c.name+'='+c.value).join('; ');
  }
  const result = await api.runtime.sendNativeMessage(HOST, {
    action: 'add', url: item.url, filename: item.filename || '',
    referrer: item.referrer || '', cookies, userAgent: navigator.userAgent
  });
  if (!result?.ok) throw new Error(result?.error || 'UDM did not accept this download.');
  await report('');
  return result;
}
api.runtime.onInstalled.addListener(async () => {
  await api.contextMenus.removeAll();
  api.contextMenus.create({id:'udm-link',title:'Download with UDM',contexts:['link','video','audio']});
});
api.contextMenus.onClicked.addListener((info, tab) => {
  handoff({url:info.linkUrl || info.srcUrl, referrer:tab?.url}).catch(e=>report(e.message));
});
api.downloads.onCreated.addListener(async item => {
  let paused = false;
  try {
    const settings = { ...defaults, ...(await api.storage.local.get('settings')).settings };
    const url = item.finalUrl || item.url;
    if (!settings.capture || item.incognito || !acceptable(url)) return;
    const parsed = new URL(url);
    if (settings.excluded.some(h=>parsed.hostname === h || parsed.hostname.endsWith('.'+h))) return;
    // Keep the local opt-in and exclusions authoritative; desktop rules further restrict capture.
    let desktop;try{desktop=await api.runtime.sendNativeMessage(HOST,{action:'preferences'});}catch{return;}
    if(!desktop?.ok)return;
    if(Array.isArray(desktop.excluded)&&desktop.excluded.some(h=>parsed.hostname===h||parsed.hostname.endsWith('.'+h)))return;
    const filename = (item.filename || parsed.pathname).split(/[\\/]/).pop();
    const ext = filename.split('.').pop().toLowerCase();
    if (!settings.extensions.includes(ext)) return;
    if(Array.isArray(desktop.extensions)&&!desktop.extensions.includes(ext))return;
    await api.downloads.pause(item.id); paused = true;
    await handoff({url,filename,referrer:item.referrer});
    // Cancel only after UDM acknowledges that it has saved the job.
    await api.downloads.cancel(item.id); paused = false;
  } catch (e) {
    if (paused) { try { await api.downloads.resume(item.id); } catch {} }
    await report(e.message);
  }
});
api.webRequest.onHeadersReceived.addListener(async event => {
  if(typeof UdmSites!=='undefined')await UdmSites.observe(event).catch(()=>{});
  if (event.tabId < 0 || !acceptable(event.url)) return;
  const contentType = event.responseHeaders?.find(h=>h.name.toLowerCase()==='content-type')?.value || '';
  if (!/^(audio|video)\//i.test(contentType) || /mpegurl/i.test(contentType)) return;
  const key = 'media:'+event.tabId;
  const pending=(mediaQueues.get(event.tabId)||Promise.resolve()).catch(()=>{}).then(async()=>{
    const tab=await api.tabs.get(event.tabId);if(tab.incognito)return;
    const stream=streamInfo(event.url,contentType);
    // Adaptive YouTube responses are grouped by format, never handed off as a partial playback fragment.
    if(new URL(event.url).hostname.endsWith('.googlevideo.com')&&!stream)return;
    const existing=(await api.storage.session.get(key))[key]||[];
    const item=stream||{url:event.url,type:contentType};
    const next=existing.filter(x=>stream?x.itag!==stream.itag:x.url!==event.url);next.push(item);
    await api.storage.session.set({[key]:next.slice(-50)});
  });
  mediaQueues.set(event.tabId,pending);try{await pending;}catch{}finally{if(mediaQueues.get(event.tabId)===pending)mediaQueues.delete(event.tabId);}
}, {urls:['http://*/*','https://*/*']}, ['responseHeaders']);
api.tabs.onRemoved.addListener(id => api.storage.session.remove('media:'+id));
api.tabs.onUpdated.addListener((id,change)=>{if(change.url) {const prior=mediaQueues.get(id)||Promise.resolve();const clear=prior.catch(()=>{}).then(()=>api.storage.session.remove('media:'+id));mediaQueues.set(id,clear);}});
api.runtime.onMessage.addListener((message, sender, respond) => {
  if (sender.id !== api.runtime.id) return false;
  if(message.action==='site-formats'||message.action==='site-download'){
    const operation=message.action==='site-formats'?UdmSites.list:UdmSites.download;
    operation(message,sender).then(respond,error=>respond({ok:false,error:error.message}));return true;
  }
  if(message.action==='sync-video-panels'&&!sender.tab){
    UdmSites.sync(true).then(()=>respond({ok:true}),error=>respond({ok:false,error:error.message}));return true;
  }
  if (message.action === 'formats') {
    availableMedia(message,sender).then(context=>respond({ok:true,videoId:context.id,choices:context.choices.map(({key,height,label})=>({key,height,label}))}),error=>respond({ok:false,error:error.message}));return true;
  }
  if (message.action === 'media') {
    mediaHandoff(message,sender).then(respond,error=>{report(error.message);respond({ok:false,error:error.message});});return true;
  }
  if (message.action === 'download') {
    handoff(message).then(result=>respond(result),error=>respond({ok:false,error:error.message})); return true;
  }
  if (message.action === 'ping') {
    api.runtime.sendNativeMessage(HOST,{action:'ping'}).then(result=>respond(result),error=>respond({ok:false,error:error.message}));return true;
  }
  return false;
});
if(typeof UdmSites!=='undefined'){
  UdmSites.install(api);
  api.tabs.onRemoved.addListener(id=>UdmSites.clear(id));
  api.tabs.onUpdated.addListener((id,change)=>{if(change.url)UdmSites.clear(id);});
}
