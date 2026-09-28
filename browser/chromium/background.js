'use strict';
if(typeof importScripts==='function')importScripts('formats.js','media.js','native-bridge.js','capture-recovery.js','navigation.js','grabber-session.js','download-session.js','chromium-proxy.js','request-context.js','browser-controls.js','key-capture.js','sites.js','ump.js','streaming-capture.js');
const api = globalThis.browser || chrome;
const captureNavigation=typeof UdmNavigation!=='undefined'?UdmNavigation.create(api):null;
captureNavigation?.install();
const HOST = 'com.udm.download_manager';
const nativeClient=typeof UdmNativeBridge!=='undefined'?UdmNativeBridge.create(api,HOST):null;
function nativeRequest(message){return nativeClient?nativeClient.request(message):api.runtime.sendNativeMessage(HOST,message);}
const chromiumProxy=typeof UdmChromiumProxy!=='undefined'?UdmChromiumProxy.create(api):null;
chromiumProxy?.install();
const requestContext=typeof UdmRequestContext!=='undefined'?UdmRequestContext.create(api,captureNavigation,chromiumProxy):null;
requestContext?.install();
const grabberSessions=typeof UdmGrabberSession!=='undefined'?UdmGrabberSession.create(api,nativeRequest,captureNavigation):null;
const downloadSessions=typeof UdmDownloadSession!=='undefined'?UdmDownloadSession.create(api,captureNavigation):null;
const defaults = { capture: false, cookies: false, excluded: [], extensions: ['zip','7z','rar','iso','exe','msi','pdf','mp4','mkv','mp3','flac'] };
const acceptable = url => /^https?:\/\//i.test(url || '');
const youtubeId = address => {try{const u=new URL(address);if(u.protocol!=='https:'||!['www.youtube.com','youtube.com','m.youtube.com','www.youtube-nocookie.com'].includes(u.hostname))return '';const id=u.pathname==='/watch'?u.searchParams.get('v'):/^\/(?:embed|shorts)\/([\w-]{11})/.exec(u.pathname)?.[1];return /^[\w-]{11}$/.test(id||'')?id:'';}catch{return '';}};
const youtubePage = address => !!youtubeId(address);
const mediaQueues = new Map();
let policyRead=0,policyPending=null;
const captureIntents=new Map();
async function syncDesktopPolicy(){
 if(policyPending)return policyPending;
 if(Date.now()-policyRead<15000)return;
 policyRead=Date.now();policyPending=(async()=>{try{const data=await nativeRequest({action:'preferences-if-running'});if(data?.ok)await api.storage.local.set({desktopPolicy:data});}catch{}finally{policyPending=null;}})();return policyPending;
}
async function takeIntent(item){
 const now=Date.now(),matches=[];
 for(const [key,value] of captureIntents){
  if(now-value.time>8000){captureIntents.delete(key);continue;}
  if((value.url===item.url||value.url===item.finalUrl)&&(!item.referrer||item.referrer===value.page))matches.push([key,value]);
 }
 const verified=[];for(const [key,value] of matches){if(await value.ready.catch(()=>false)&&captureIntents.get(key)===value&&Date.now()-value.time<=8000)verified.push([key,value]);}
 if(verified.length!==1)return '';
 captureIntents.delete(verified[0][0]);return verified[0][1].intent;
}

const qualityItags = {137:1080,136:720,135:480,134:360,133:240,160:144};
function streamInfo(raw, type) {
  const u=new URL(raw);
  if(!u.hostname.endsWith('.googlevideo.com') || u.pathname!=='/videoplayback' || u.searchParams.has('sq') || u.searchParams.get('ump')==='1' || u.searchParams.has('sabr')) return null;
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
  if(await browserControls?.isDisabled(tabId))throw Error('UDM is disabled on this tab.');
  const id=youtubeId(message.url),frameId=sender.tab?(sender.frameId??0):0;
  if(sender.tab&&youtubeId(sender.url)!==id)throw new Error('The message did not come from this video page.');
  if(tab.incognito)throw new Error('Media capture is disabled in private windows.');
  if(frameId===0&&youtubeId(tab.url)!==id)throw new Error('The video page changed. Open UDM on the current page again.');
  return {tabId,tab,id,frameId};
}
async function availableMedia(message,sender) {
  const context=await mediaContext(message,sender);
  const stored=await api.storage.local.get(['settings','desktopPolicy']),prefs=UdmMedia.policy.merge(stored.settings||{},stored.desktopPolicy||{});
  if(prefs.panelEnabled===false||UdmMedia.policy.panelBlocked(context.tab.url,prefs)||UdmMedia.policy.panelBlocked(message.url,prefs))throw Error('Video panels are disabled on this site.');
  const results=await api.scripting.executeScript({target:{tabId:context.tabId,frameIds:[context.frameId]},world:'MAIN',func:readYouTubeFormats,args:[context.id]});
  await mediaContext(message,sender); // Navigation may happen while the snapshot is read.
  const injection=results.find(r=>r.frameId===context.frameId);
  if(sender.documentId&&injection?.documentId&&sender.documentId!==injection.documentId)throw new Error('The video document changed. Refresh the list.');
  const snapshot=injection?.result;context.documentId=injection?.documentId||sender.documentId||'';
  if(snapshot?.live)throw new Error('Live streams are not supported.');
  const key='media:'+context.tabId;
  const observed=(await api.storage.session.get(key))[key]||[];
  const captured=UdmFormats.attachObserved(snapshot,observed.filter(x=>x.frameId===context.frameId&&(context.documentId?x.documentId===context.documentId:!x.documentId&&Number.isFinite(snapshot?.timeOrigin)&&x.observedAt>=snapshot.timeOrigin)));
  const choices=UdmFormats.choices(captured,context.id).filter(c=>UdmMedia.policy.panelAllowed(c,prefs)),audioChoices=UdmFormats.audioChoices(captured,context.id,true).filter(c=>UdmMedia.policy.panelAllowed(c,prefs));
  if(!choices.length&&!audioChoices.length)throw new Error('No downloadable qualities detected yet. Play this video, then refresh the list.');
  return {...context,choices,audioChoices,snapshot,prefs};
}
async function sabrFor(context,choice){
  const video=context.snapshot?.formats?.find(f=>f.id===choice.formatId&&!f.muxed&&/^video\/mp4\b/.test(f.mime));
  const audio=context.snapshot?.formats?.find(f=>f.audioDefault!==false&&/^audio\/mp4\b/.test(f.mime)&&/mp4a/.test(f.mime));
  if(!video||!audio||!/^\d+$/.test(video.lastModified||'')||!/^\d+$/.test(audio.lastModified||''))return null;
  const result=await api.scripting.executeScript({target:{tabId:context.tabId,frameIds:[context.frameId]},world:'MAIN',func:id=>globalThis.__udmCaptureV1?.session?.(id),args:[context.id]});
  const row=result.find(r=>r.frameId===context.frameId);
  if(context.documentId&&row?.documentId!==context.documentId)return null;
  const session=row?.result||(typeof UdmStreamingCapture!=='undefined'?await UdmStreamingCapture.session(context):null);
  if(!session||session.videoId!==context.id||typeof session.body!=='string'||session.body.length>175000||!session.url)return null;
  const select=f=>({id:f.id,lastModified:f.lastModified,xtags:f.xtags||'',mime:f.mime});
  return {...session,durationMs:context.snapshot.durationMs,video:select(video),audio:select(audio)};
}
// Bound browser/host waits so a stalled API cannot leave the panel busy indefinitely.
// Each awaited step must finish before handoff continues; a late read cannot create a job.
function mediaStep(operation,error,timeout=12000){
  let timer;
  return Promise.race([Promise.resolve().then(operation),new Promise((_,reject)=>{
    timer=setTimeout(()=>reject(new Error(error)),timeout);
  })]).finally(()=>clearTimeout(timer));
}
// Short-lived memory cache binds native retrieval to the current video document.
// No signed URL, cookie or player response is written to extension storage.
const playerPairs=new Map(),playerReports=new Map();let playerRequests=0;
function playerDiagnostics(tabId){return playerReports.get(tabId)||null;}
function reportPlayer(context,pair,began,outcome){
 const d=pair?.diagnostics||{},safe={outcome,elapsedMs:Date.now()-began,visitorPresent:!!context.snapshot?.visitorData};
 if(['player-request','player-response','video-range','audio-range','complete'].includes(d.phase))safe.phase=d.phase;
 if(typeof d.timeout==='boolean')safe.timeout=d.timeout;
 if(/^[A-Z_]{1,40}$/.test(d.playability||''))safe.playability=d.playability;
 for(const k of ['formats','direct','cipher','nTransform'])if(Number.isSafeInteger(d[k])&&d[k]>=0&&d[k]<=400)safe[k]=d[k];
 if(typeof d.videoMatches==='boolean')safe.videoMatches=d.videoMatches;
 if(playerReports.size>=32&&!playerReports.has(context.tabId))playerReports.delete(playerReports.keys().next().value);
 playerReports.set(context.tabId,safe);
}
function clearPlayerPairs(tabId){playerReports.delete(tabId);for(const [key,entry] of playerPairs)if(entry.tabId===tabId)playerPairs.delete(key);}
function validPlayerPair(pair,context,choice){
 if(!pair?.ok||pair.transport!=='player-direct'||pair.videoId!==context.id||pair.formatId!==choice.formatId||pair.height!==choice.height||!Number.isInteger(pair.pixelHeight)||pair.pixelHeight<1||pair.pixelHeight>4320||pair.userAgent!=='Mozilla/5.0 (Macintosh; Intel Mac OS X 15_7_3) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/26.0 Safari/605.1.15'||!Number.isFinite(pair.validatedAt)||Date.now()-pair.validatedAt>60000||pair.validatedAt>Date.now()+5000)return false;
 try{for(const kind of ['video','audio']){
   const stream=pair[kind],u=new URL(stream.url);
   if(u.protocol!=='https:'||u.username||u.password||(u.port&&u.port!=='443')||!u.hostname.endsWith('.googlevideo.com')||u.pathname!=='/videoplayback'||stream.kind!==kind||!Number.isSafeInteger(stream.size)||stream.size<=0||!Number.isSafeInteger(stream.itag)||u.searchParams.get('itag')!==String(stream.itag)||Number(u.searchParams.get('expire'))*1000<Date.now()+120000||!/^\d+$/.test(u.searchParams.get('expire')||''))return false;
   if(kind==='video'&&String(stream.itag)!==choice.formatId)return false;
   if(['sabr','sq','ump','n','range','rn','rbuf'].some(key=>u.searchParams.has(key)))return false;
 }return true;}catch{return false;}
}
async function nativePlayerPair(context,choice){
 if(choice.videoUrl||!/^\d{1,6}$/.test(choice.formatId)||(!context.documentId&&!Number.isFinite(context.snapshot?.timeOrigin)))return null;
 const key=JSON.stringify([context.tabId,context.frameId,context.documentId||context.snapshot.timeOrigin,context.id,choice.formatId,choice.height]);
 const previous=playerPairs.get(key);if(previous&&Date.now()<previous.until)return previous.promise;
 if(playerRequests>=2)return null;
 for(const [k,v] of playerPairs)if(Date.now()>=v.until)playerPairs.delete(k);
 if(playerPairs.size>=32)playerPairs.delete(playerPairs.keys().next().value);
 const began=Date.now(),entry={tabId:context.tabId,until:Date.now()+120000,promise:null};++playerRequests;
 entry.promise=mediaStep(()=>nativeRequest({action:'youtube-player',videoId:context.id,formatId:choice.formatId,height:choice.height,signatureTimestamp:context.snapshot?.signatureTimestamp||0,visitorData:context.snapshot?.visitorData||''}),'Player retrieval timed out.',10000).then(pair=>{
   if(playerPairs.get(key)!==entry)return null;
   entry.until=Date.now()+5000;const valid=validPlayerPair(pair,context,choice);reportPlayer(context,pair,began,valid?'ready':pair?.ok?'invalid-pair':'unavailable');if(!valid)return null;
   entry.until=Math.min(Date.now()+45000,pair.validatedAt+55000);return pair;
 }).catch(()=>{entry.until=Date.now()+5000;if(playerPairs.get(key)===entry)reportPlayer(context,null,began,'host-error');return null;}).finally(()=>--playerRequests);
 playerPairs.set(key,entry);return entry.promise;
}
function prefetchPlayerPair(context){
 const choice=context.choices.find(x=>x.height<=1080&&!x.videoUrl)||context.choices.find(x=>!x.videoUrl);
 if(choice)void nativePlayerPair(context,choice);
}

const mediaReadTimeout='The browser took too long to read this video. Refresh the video page and try again.';
async function audioSession(context,choice){
  if(!choice.streamFormat)return null;
  const result=await api.scripting.executeScript({target:{tabId:context.tabId,frameIds:[context.frameId]},world:'MAIN',func:id=>({session:globalThis.__udmCaptureV1?.session?.(id),timeOrigin:performance.timeOrigin}),args:[context.id]});
  const row=result.find(r=>r.frameId===context.frameId);
  if(context.documentId?row?.documentId!==context.documentId:
    !Number.isFinite(context.snapshot?.timeOrigin)||row?.result?.timeOrigin!==context.snapshot.timeOrigin)return null;
  const session=row?.result?.session||(typeof UdmStreamingCapture!=='undefined'?await UdmStreamingCapture.session(context):null);
  if(!session||session.videoId!==context.id||typeof session.body!=='string'||!session.body.length||session.body.length>175000||!session.url||
    !Number.isFinite(session.capturedAt)||Date.now()-session.capturedAt>300000||session.capturedAt>Date.now()+30000)return null;
  try{const u=new URL(session.url);if(u.protocol!=='https:'||u.username||u.password||(u.port&&u.port!=='443')||!u.hostname.endsWith('.googlevideo.com')||u.pathname!=='/videoplayback')return null;}catch{return null;}
  return {url:session.url,body:session.body,videoId:context.id,capturedAt:session.capturedAt,durationMs:context.snapshot.durationMs,audio:choice.streamFormat};
}
async function audioHandoff(message,sender) {
  const context=await mediaStep(()=>availableMedia(message,sender),mediaReadTimeout);
  const choice=context.audioChoices.find(x=>x.key===message.audioKey);
  if(!choice)throw Error('That audio track is no longer available. Play the video, then refresh the list.');
  let sabr=null;
  if(!choice.audioUrl){
    const host=await mediaStep(()=>nativeRequest({action:'hello'}),'UDM did not reply. Check the desktop connection.');
    if(!host?.ok||!host.capabilities?.includes('sabr-audio'))throw Error('Update the UDM desktop app to use YouTube streaming audio.');
    sabr=await mediaStep(()=>audioSession(context,choice),mediaReadTimeout);
    if(!sabr)throw Error('Play this video to capture its audio session, then refresh the list.');
  }
  const confirmed=await mediaStep(()=>availableMedia(message,sender),mediaReadTimeout);
  const current=confirmed.audioChoices.find(x=>x.key===choice.key);
  const sameDocument=context.documentId?context.documentId===confirmed.documentId:
    Number.isFinite(context.snapshot?.timeOrigin)&&context.snapshot.timeOrigin===confirmed.snapshot?.timeOrigin;
  if(!sameDocument||!current||current.lastModified!==choice.lastModified||
    choice.audioUrl&&(!current.audioUrl||current.resource!==choice.resource)||
    !current.audioUrl&&(!sabr||JSON.stringify(current.streamFormat)!==JSON.stringify(choice.streamFormat)||confirmed.snapshot.durationMs!==sabr.durationMs))throw Error('The video or audio track changed before handoff. Refresh the list.');
  const clean=value=>String(value||'').replace(/[<>:"/\\|?*\x00-\x1f\x7f]/g,' ').trim().replace(/[. ]+$/,'');
  const filename=(clean(message.title||confirmed.tab.title||'YouTube audio').slice(0,150)||'YouTube audio')+' - '+clean(current.label).slice(0,70)+'.m4a';
  const request=current.audioUrl?{action:'add',url:current.audioUrl,filename,referrer:message.url,userAgent:navigator.userAgent}:
    {action:'sabr',output:'audio',url:'https://www.youtube.com/watch?v='+context.id,height:0,pixelHeight:0,formatId:current.formatId,exactQuality:true,sabr,filename,referrer:message.url,userAgent:navigator.userAgent};
  const result=await mediaStep(()=>nativeRequest(request),'UDM did not reply in time. Check its download list before trying again.',40000);
  if(!result?.ok)throw Error(result?.error||'UDM did not accept the audio.');
  void report('').catch(()=>{});return result;
}
async function mediaHandoff(message, sender) {
  let context=await mediaStep(()=>availableMedia(message,sender),mediaReadTimeout);
  let choice=context.choices.find(x=>x.key===message.formatKey&&x.height===Number(message.height));
  if(!choice)throw new Error('That quality is no longer available. Refresh the list and choose again.');
  // The native lookup already has a bounded deadline. Await the same cached
  // promise: racing it with 1.2 seconds discarded usable direct pairs.
  const retrieved=choice.videoUrl?null:await nativePlayerPair(context,choice);
  if(retrieved)choice={...choice,videoUrl:retrieved.video.url,audioUrl:retrieved.audio.url,pixelHeight:retrieved.pixelHeight};
  let sabr=choice.videoUrl?null:await mediaStep(()=>sabrFor(context,choice),mediaReadTimeout);
  if(!choice.videoUrl&&!sabr){
    await mediaStep(()=>api.scripting.executeScript({target:{tabId:context.tabId,frameIds:[context.frameId]},world:'MAIN',func:(id,height)=>globalThis.__udmCaptureV1?.prepare(id,height),args:[context.id,choice.height]}),mediaReadTimeout);
    for(let attempt=0;attempt<8&&!choice.videoUrl&&!sabr;attempt++){
      await new Promise(resolve=>setTimeout(resolve,400));
      const latest=await mediaStep(()=>availableMedia(message,sender),mediaReadTimeout);
      context=latest;
      choice=latest.choices.find(x=>x.key===message.formatKey&&x.height===Number(message.height));
      if(!choice)throw new Error('The video or available formats changed. Refresh the list.');
      if(!choice.videoUrl)sabr=await mediaStep(()=>sabrFor(context,choice),mediaReadTimeout);
    }
    if(!choice.videoUrl&&!sabr)throw new Error('This quality is listed by the player, but UDM has not captured a downloadable stream yet. Play the video at this quality, then retry.');
  }
  const confirmed=await mediaStep(()=>availableMedia(message,sender),mediaReadTimeout);
  const currentChoice=confirmed.choices.find(x=>x.key===choice.key&&x.height===choice.height);
  if(context.documentId!==confirmed.documentId||!currentChoice)throw new Error('The video or format changed before handoff. Refresh the list.');
  // Rebind the transport as well as the quality: direct URLs can arrive or be
  // replaced while the browser is reading its captured streaming session.
  if(currentChoice.videoUrl){choice=currentChoice;sabr=null;}
  else if(choice.videoUrl&&!retrieved)throw new Error('The direct video links changed before handoff. Refresh the list and choose the quality again.');
  const knownSize=retrieved&&!currentChoice.videoUrl?retrieved.video.size+retrieved.audio.size:choice.size;
  if(!UdmMedia.policy.panelAllowed({...choice,size:knownSize},confirmed.prefs))throw Error('This video is below the minimum size in video-panel settings.');
  const result=await mediaStep(()=>nativeRequest({action:sabr?'sabr':'media',sabr,url:message.url,filename:message.title||context.tab.title||'YouTube video',height:choice.height,pixelHeight:choice.pixelHeight||0,formatId:choice.formatId,exactQuality:true,videoUrl:choice.videoUrl,audioUrl:choice.audioUrl,referrer:message.url,userAgent:retrieved&&!currentChoice.videoUrl?retrieved.userAgent:navigator.userAgent}),'UDM did not reply in time. Check its download list before trying again.',40000);
  if(!result?.ok)throw new Error(result?.error||'UDM did not accept the video.');
  void report('').catch(()=>{});return result;
}
if(typeof UdmStreamingCapture!=='undefined')UdmStreamingCapture.install(api,captureNavigation);
const report = async text => {
  // Cosmetic reporting must never delay acknowledgement or browser recovery.
  void Promise.resolve().then(()=>api.storage.local.set({lastError:text})).catch(()=>{});
  void Promise.resolve().then(()=>api.action.setBadgeText({text:text?'!':''})).catch(()=>{});
};
const browserControls=typeof UdmBrowserControls!=='undefined'?UdmBrowserControls.create(api,handoff,report):null;
const keyCapture=typeof UdmKeyCapture!=='undefined'?UdmKeyCapture.create(api,async()=>{const s=await api.storage.local.get(['settings','desktopPolicy']);return UdmMedia.policy.merge({...defaults,...s.settings},s.desktopPolicy||{});},async id=>!!(await browserControls?.isDisabled(id)),requestContext,handoff):null;
const captureRecovery=typeof UdmCaptureRecovery!=='undefined'?UdmCaptureRecovery.create(api,nativeRequest,report):null;
captureRecovery?.install();
async function handoff(item, capturedContext, ownership) {
  if(await browserControls?.isDisabled(item.tabId))throw Error('UDM is disabled on this tab.');
  if (!acceptable(item.url)) throw new Error('Only HTTP and HTTPS browser downloads can be handed off.');
  const settings = { ...defaults, ...(await api.storage.local.get('settings')).settings };
  const canCredentials=settings.cookies&&await api.permissions.contains({permissions:['cookies'],origins:['http://*/*','https://*/*']});
  const observed=capturedContext===undefined?requestContext?.resolve(item,canCredentials):capturedContext;
  let browserProxy=observed?.proxy;
  // An explicit link command may precede any browser network request. Bind its
  // current configured route without inventing prior request headers or cookies.
  if(!observed&&!item.browserDownload&&chromiumProxy){
    const tab=Number.isInteger(item.tabId)&&item.tabId>=0&&api.tabs?.get?await api.tabs.get(item.tabId):null;
    const observation=chromiumProxy.observe(item.finalUrl||item.url,tab?.incognito===true);await observation.ready;
    browserProxy=chromiumProxy.resolve(observation);
  }
  if(observed&&observed.method!=='GET'&&!observed.request)throw Error('The original '+observed.method+' download request could not be captured safely. This download remains in the browser.');
  if(browserProxy?.unsupported)throw Error('UDM cannot reproduce this browser proxy route. Download this file in the browser.');
  if(browserProxy){const desktop=await nativeRequest({action:'preferences'});if(!desktop?.ok||desktop.browserProxy!==1||(browserProxy.type!=='direct'&&desktop.explicitProxyTransport!==1)||((browserProxy.type==='https'||browserProxy.proxyDNS===false)&&desktop.browserProxyTypes!==2))throw Error('Update the UDM desktop app before handing off this browser proxy. This download remains in the browser.');}
  if(observed?.request){
    const desktop=await nativeRequest({action:'preferences'}),encoded=observed.request.body;
    const byteLength=encoded.length/4*3-(encoded.endsWith('==')?2:encoded.endsWith('=')?1:0);
    const limit=Number.isSafeInteger(desktop?.postBodyLimit)?Math.max(0,desktop.postBodyLimit):65536;
    if(!desktop?.ok||!desktop.postDownloads||byteLength>limit)throw Error('Update the UDM desktop app to capture this form download. This download remains in the browser.');
  }
  if(observed&&await browserControls?.isDisabled(observed.tabId))throw Error('UDM is disabled on this tab.');
  const headers={...(observed?.headers||{})};
  // A pre-pause snapshot is internal only; current consent still controls credentials.
  if(!canCredentials){delete headers.Cookie;delete headers.Authorization;}
  let cookies = headers.Cookie||'';
  const capturedSession=canCredentials?await downloadSessions?.capture(item,observed):null;
  if(capturedSession)cookies=capturedSession.cookies;
  const sessionPolicy=capturedSession?await nativeRequest({action:'preferences'}):null;
  if(capturedSession){
    await capturedSession.verify();
    if(!(await api.storage.local.get('settings')).settings?.cookies||!await api.permissions.contains({permissions:['cookies'],origins:['http://*/*','https://*/*']}))throw Error('Cookie sharing was disabled. Capture this download again.');
  }
  const send=ownership?message=>captureRecovery.submit(ownership,message):nativeRequest;
  const result = await send( {
    action: 'add', url: item.url, filename: item.filename || '', downloadLater:!!item.downloadLater,
    headers, request:observed?.request||{},
    ...(capturedSession&&sessionPolicy?.browserSession===1?{browserSession:capturedSession.session}:{}), ...(browserProxy?{browserProxy}:{}), referrer: headers.Referer || item.referrer || '', cookies, userAgent: headers['User-Agent'] || navigator.userAgent
  });
  if (!result?.ok) throw new Error(result?.error || 'UDM did not accept this download.');
  await report('');
  return result;
}
let menuPending=Promise.resolve();
async function menuPolicy(){const saved=await api.storage.local.get(['settings','desktopPolicy']);return UdmMedia.policy.merge(saved.settings||{},saved.desktopPolicy||{}).contextMenu||{};}
function reconcileMenus(){const work=menuPending.catch(()=>{}).then(async()=>{const menu=await menuPolicy();await api.contextMenus.removeAll();
 if(menu.Link!==false)api.contextMenus.create({id:'udm-link',title:'Download with UDM',contexts:['link','video','audio']});await browserControls?.menus(menu);
 });menuPending=work;return work;}
api.runtime.onInstalled.addListener(()=>reconcileMenus().catch(e=>report(e.message)));
api.runtime.onStartup?.addListener(()=>reconcileMenus().catch(()=>{}));
api.storage.onChanged?.addListener((changes,area)=>{if(area==='local'&&(changes.settings||changes.desktopPolicy))void reconcileMenus().catch(()=>{});});
api.contextMenus.onClicked.addListener((info,tab)=>{(async()=>{const menu=await menuPolicy();
 if((['udm-link','udm-selected-links'].includes(info.menuItemId)&&menu.Link===false)||(info.menuItemId==='udm-all-links'&&menu.All===false))return;
 if(browserControls?.handle(info,tab))return;
 if(info.menuItemId==='udm-link')await handoff({url:info.linkUrl||info.srcUrl,referrer:info.frameUrl||tab?.url,tabId:tab?.id,frameId:info.frameId??0});
 })().catch(e=>report(e.message));});
api.downloads.onCreated.addListener(async item => {
  let paused = false,ownership;
  try {
    let intent=await takeIntent(item);if(intent==='bypass')return;
    const settings = { ...defaults, ...(await api.storage.local.get('settings')).settings };
    const url = item.finalUrl || item.url;
    if (!settings.capture || item.incognito || !acceptable(url) || (item.state && item.state!=='in_progress')) return;
    if(UdmMedia.policy.blocked(url,settings))return;
    const parsed=new URL(url),filename=(item.filename||parsed.pathname).split(/[\\/]/).pop();
    const ext=filename.split('.').pop().toLowerCase();

    // Firefox aborts webRequest when paused, removing its ephemeral request record.
    // Bind the request before pausing; unsupported forms must keep running in-browser.
    const captureItem={url,filename,referrer:item.referrer,browserDownload:true};
    const capturedContext=requestContext?.resolveDownload?await requestContext.resolveDownload(captureItem,true):requestContext?.resolve(captureItem,true)??null;
    if(requestContext&&!capturedContext)return;
    if(!intent)intent=await keyCapture?.intent(capturedContext,true)||'';if(intent==='bypass')return;
    if(intent!=='force'&&!settings.extensions.includes(ext))return;
    if(capturedContext?.proxy?.unsupported)throw Error('UDM cannot reproduce this browser proxy route. This download remains in the browser.');
    if(capturedContext&&capturedContext.method!=='GET'&&!capturedContext.request)
      throw Error('The original '+capturedContext.method+' download request could not be captured safely. This download remains in the browser.');
    // Native startup can take longer than a small browser download. Hold an
    // eligible transfer before requesting desktop rules, then release it if declined.
    if(captureRecovery)ownership=await captureRecovery.begin(item);
    await api.downloads.pause(item.id);paused=true;
    let desktop;try{desktop=await nativeRequest({action:'preferences'});}catch{return;}
    if(!desktop?.ok||desktop.captureAllowed===false)return;
    if(intent==='force'&&desktop.forceSkipWeb!==false&&UdmMedia.policy.webResource(item))return;
    if(UdmMedia.policy.blocked(url,UdmMedia.policy.merge(settings,desktop)))return;
    if(intent!=='force'&&Array.isArray(desktop.extensions)&&!desktop.extensions.includes(ext))return;
    const [current]=await api.downloads.search({id:item.id});
    // Firefox has no partial file if pause wins before the first payload byte;
    // it then reports USER_CANCELED with paused=false despite acknowledging pause.
    const firefoxStopped=!!globalThis.browser&&current?.state==='interrupted'&&current.error==='USER_CANCELED'&&current.bytesReceived===0;
    if(!current||(!current.paused&&!firefoxStopped)){paused=false;return;}
    // Firefox reports a successful pause as interrupted; Chromium stays in_progress.
    if(!['in_progress','interrupted'].includes(current.state))return;
    if(ownership)ownership.protocol=desktop.captureRecovery===1?1:0;
    await handoff(captureItem,capturedContext,ownership);
    if(ownership){paused=false;return;}
    // Cancel only after UDM acknowledges that it has saved the job.
    paused=false; // Desktop already owns the job, even if browser cancellation fails.
    try{await api.downloads.cancel(item.id);}catch{throw Error('UDM accepted this download, but the browser could not cancel its transfer. Check UDM and the browser download list before resuming or retrying.');}
  } catch(e) {
    await report(e.message);
  } finally {
    if(ownership)await captureRecovery.finish(ownership);
    else if(paused){try{await api.downloads.resume(item.id);}catch{}}
  }
});
api.webRequest.onHeadersReceived.addListener(async event => {
  const stamp=captureNavigation?.token(event.tabId,event.frameId??0);
  if(typeof UdmSites!=='undefined')await UdmSites.observe(event).catch(()=>{});
  if (event.tabId < 0 || !acceptable(event.url) || ![200,206].includes(event.statusCode)) return;
  const contentType = event.responseHeaders?.find(h=>h.name.toLowerCase()==='content-type')?.value || '';
  if (!/^(audio|video)\//i.test(contentType) || /mpegurl/i.test(contentType)) return;
  const key = 'media:'+event.tabId;
  const pending=(mediaQueues.get(event.tabId)||Promise.resolve()).catch(()=>{}).then(async()=>{
    await captureNavigation?.ready;
    const tab=await api.tabs.get(event.tabId);if(tab.incognito||captureNavigation&&!captureNavigation.valid(event,stamp))return;
    const stream=streamInfo(event.url,contentType);
    // Adaptive YouTube responses are grouped by format, never handed off as a partial playback fragment.
    if(new URL(event.url).hostname.endsWith('.googlevideo.com')&&!stream)return;
    const existing=(await api.storage.session.get(key))[key]||[];
    const item={...(stream||{url:event.url,type:contentType}),frameId:event.frameId??0,documentId:event.documentId||'',observedAt:Date.now()};
    const next=existing.filter(x=>x.frameId!==item.frameId||x.documentId!==item.documentId||(stream?x.itag!==stream.itag:x.url!==event.url));next.push(item);
    if(captureNavigation&&!captureNavigation.valid(event,stamp))return;
    await api.storage.session.set({[key]:next.slice(-50)});
  });
  mediaQueues.set(event.tabId,pending);try{await pending;}catch{}finally{if(mediaQueues.get(event.tabId)===pending)mediaQueues.delete(event.tabId);}
}, {urls:['http://*/*','https://*/*']}, ['responseHeaders']);
api.tabs.onRemoved.addListener(id => {keyCapture?.clear(id);for(const [key,value] of captureIntents)if(key.startsWith(id+':'))captureIntents.delete(key);clearPlayerPairs(id);if(!captureNavigation?.supported)return api.storage.session.remove('media:'+id);});
api.tabs.onUpdated.addListener((id,change)=>{if(change.url) {keyCapture?.clear(id);for(const [key,value] of captureIntents)if(key.startsWith(id+':'))captureIntents.delete(key);clearPlayerPairs(id);if(captureNavigation?.supported)return;const prior=mediaQueues.get(id)||Promise.resolve();const clear=prior.catch(()=>{}).then(()=>api.storage.session.remove('media:'+id));mediaQueues.set(id,clear);}});
if(captureNavigation?.supported)captureNavigation.subscribe(event=>{
  const id=event.tabId,key='media:'+id;
  const task=(mediaQueues.get(id)||Promise.resolve()).catch(()=>{}).then(async()=>{
    if(event.kind==='removed')return api.storage.session.remove(key);
    const rows=(await api.storage.session.get(key))[key];
    if(rows)await api.storage.session.set({[key]:rows.filter(row=>captureNavigation.keep({...row,tabId:id},event))});
  });mediaQueues.set(id,task);return task.finally(()=>{if(mediaQueues.get(id)===task)mediaQueues.delete(id);});
});
api.runtime.onMessage.addListener((message, sender, respond) => {
  if(message?.action==='grabber-login-pending'||message?.action==='grabber-login-complete'){if(!grabberSessions){respond({ok:false,error:'Browser sign-in is unavailable.'});return false;}const operation=message.action==='grabber-login-pending'?grabberSessions.pending:grabberSessions.complete;operation(message,sender).then(respond,e=>respond({ok:false,error:e.message}));return true;}
  if (sender.id !== api.runtime.id) return false;
  if(['capture-keys','capture-player'].includes(message.action)){if(!keyCapture)return false;const operation=message.action==='capture-keys'?keyCapture.update:keyCapture.player;operation(message,sender).then(respond,e=>respond({ok:false,error:e.message}));return true;}
  if(message.action==='integration-state'){(async()=>{void syncDesktopPolicy().catch(()=>{});return {ok:true,disabled:!!(await browserControls?.isDisabled(sender.tab?.id))};})().then(respond,()=>respond({ok:false}));return true;}
  if(message.action==='batch-list'||message.action==='batch-download'){browserControls?.message(message,sender).then(respond,error=>respond({ok:false,error:error.message}));return true;}
  if(message.action==='capture-intent'){
    (async()=>{if(!sender.tab||sender.tab.incognito||!acceptable(sender.url)||(typeof message.url!=='string'||message.url.length>16000||!acceptable(message.url))||!['force','bypass'].includes(message.intent))throw Error('Invalid capture gesture.');
      const target=new URL(message.url);target.hash='';if(target.username||target.password)throw Error('Invalid capture URL.');
      const key=sender.tab.id+':'+(sender.frameId??0),entry={url:target.href,page:sender.url,intent:message.intent,time:Date.now(),ready:null};
      if(captureIntents.size>=128)captureIntents.delete(captureIntents.keys().next().value);
      // Record pending validation synchronously: a download event may arrive
      // before tabs.get replies. It must await validation, never bypass it.
      entry.ready=api.tabs.get(sender.tab.id).then(tab=>{if(tab.incognito||((sender.frameId??0)===0&&tab.url!==sender.url))throw Error('The source page changed.');return true;});
      captureIntents.set(key,entry);
      try{await entry.ready;return {ok:true};}catch(e){if(captureIntents.get(key)===entry)captureIntents.delete(key);throw e;}
    })().then(respond,e=>respond({ok:false,error:e.message}));return true;
  }
  if(message.action==='restore-panels'&&!sender.tab){
    (async()=>{const tab=await api.tabs.get(message.tabId);if(tab.incognito||!acceptable(tab.url))throw Error('Open a regular video page.');
      await UdmSites.sync(true,tab.id);await api.tabs.sendMessage(tab.id,{action:'restore-panels'});return {ok:true};
    })().then(respond,e=>respond({ok:false,error:e.message}));return true;
  }
  if(message.action==='capture-recover'&&!sender.tab){if(!captureRecovery){respond({ok:false,error:'Download recovery is unavailable.'});return false;}captureRecovery.recover().then(respond,e=>respond({ok:false,error:e.message}));return true;}
  if(message.action==='capture-diagnostics'){respond({ok:true,connection:nativeClient?.diagnostics(),requests:requestContext?.diagnostics(),player:playerDiagnostics(message.tabId),counts:typeof UdmStreamingCapture!=='undefined'?UdmStreamingCapture.diagnostics(message.tabId):{}});return false;}
  if(message.action==='site-formats'||message.action==='site-download'){
    const operation=message.action==='site-formats'?UdmSites.list:UdmSites.download;
    operation(message,sender).then(respond,error=>respond({ok:false,error:error.message}));return true;
  }
  if(message.action==='sync-video-panels'&&!sender.tab){
    UdmSites.sync(true,message.tabId).then(result=>respond({ok:true,...result}),error=>respond({ok:false,error:error.message}));return true;
  }
  if (message.action === 'formats') {
    mediaStep(()=>availableMedia(message,sender),mediaReadTimeout).then(context=>{prefetchPlayerPair(context);respond({ok:true,videoId:context.id,choices:context.choices.map(({key,height,label})=>({key,height,label})),audioChoices:context.audioChoices.map(({key,label,default:preferred})=>({key,label,default:preferred}))});},error=>respond({ok:false,error:error.message}));return true;
  }
  if(message.action==='youtube-audio'){audioHandoff(message,sender).then(respond,error=>{void report(error.message).catch(()=>{});respond({ok:false,error:error.message});});return true;}
  if (message.action === 'media') {
    mediaHandoff(message,sender).then(respond,error=>{void report(error.message).catch(()=>{});respond({ok:false,error:error.message});});return true;
  }
  if(message.action==='selected-links'){
    (async()=>{if(!sender.tab||sender.tab.incognito||!acceptable(sender.url)||message.page!==sender.url)throw Error('Open a regular web page to download selected links.');if(!Array.isArray(message.urls)||!message.urls.length||message.urls.length>100)throw Error('Select between 1 and 100 links.');const urls=[...new Set(message.urls)];for(const url of urls){if(typeof url!=='string'||url.length>16000||!acceptable(url))throw Error('Only HTTP and HTTPS links are supported.');const parsed=new URL(url);if(parsed.username||parsed.password)throw Error('Links containing embedded credentials are not supported.');}let count=0;try{for(const url of urls){await handoff({url,referrer:sender.url,tabId:sender.tab.id,frameId:sender.frameId??0,documentId:sender.documentId});count++;}return {ok:true,count};}catch(error){return {ok:false,count,error:count+' links were added. '+error.message+' Check UDM before retrying; the last request may also have been accepted.'};}})().then(respond,error=>respond({ok:false,count:0,error:error.message}));return true;
  }
  if (message.action === 'download') {
    handoff(message).then(result=>respond(result),error=>respond({ok:false,error:error.message})); return true;
  }
  if(message.action==='open-browser-settings'&&!sender.tab){nativeRequest({action:'browser-settings'}).then(respond,e=>respond({ok:false,error:e.message}));return true;}
  if (message.action === 'ping') {
    nativeRequest({action:'ping'}).then(result=>respond(result),error=>respond({ok:false,error:error.message}));return true;
  }
  return false;
});
if(typeof UdmSites!=='undefined'){
  UdmSites.install(api,captureNavigation);
  if(!captureNavigation?.supported)api.tabs.onRemoved.addListener(id=>UdmSites.clear(id));
  if(!captureNavigation?.supported)api.tabs.onUpdated.addListener((id,change)=>{if(change.url)UdmSites.clear(id);});
}
