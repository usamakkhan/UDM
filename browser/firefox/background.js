'use strict';
if(typeof importScripts==='function')importScripts('file-recognition.js','capture-rules.js','formats.js','media.js','native-bridge.js','capture-recovery.js','media-receipts.js','navigation.js','grabber-session.js','grabber-rendered.js','download-session.js','chromium-proxy.js','multipart.js','request-context.js','browser-controls.js','key-capture.js','sites.js','ump.js','streaming-capture.js');
const api = globalThis.browser || chrome;
const captureNavigation=typeof UdmNavigation!=='undefined'?UdmNavigation.create(api):null;
captureNavigation?.install();
const HOST = 'com.udm.download_manager';
const nativeClient=typeof UdmNativeBridge!=='undefined'?UdmNativeBridge.create(api,HOST):null;
async function nativeRequest(message,beforeSend,validateSelection){
  if(nativeClient)return nativeClient.request(message,undefined,beforeSend,validateSelection);
  if(validateSelection)await validateSelection(message);
  beforeSend?.();
  return api.runtime.sendNativeMessage(HOST,message);
}
function warmMediaConnection(){
  if(nativeClient?.warmControl)void nativeClient.warmControl().catch(()=>{});
}
function isBrowserToolsSender(sender){
  return !!sender&&sender.id===api.runtime.id&&!sender.tab?.incognito&&typeof api.runtime.getURL==='function'&&
    sender.url?.split('#')[0]===api.runtime.getURL('popup.html');
}
// Stages a captured body above the inline limit: begin (size, SHA-256, type), ordered base64 chunks, finish. Any failure leaves the download in the browser.
async function stagePostBody(bytes,contentType){
  const digest=new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),sha256=Array.from(digest,b=>b.toString(16).padStart(2,'0')).join('');
  const begun=await nativeRequest({action:'post-body-begin',size:bytes.length,sha256,contentType});
  if(!begun?.ok||typeof begun.bodyToken!=='string')throw Error(begun?.error||'UDM could not accept this form body. This download remains in the browser.');
  const chunk=Number.isSafeInteger(begun.chunkBytes)&&begun.chunkBytes>0?begun.chunkBytes:1048576;
  for(let offset=0;offset<bytes.length;offset+=chunk){
    const slice=bytes.subarray(offset,offset+chunk),parts=[];for(let i=0;i<slice.length;i+=8192)parts.push(String.fromCharCode(...slice.subarray(i,i+8192)));
    const reply=await nativeRequest({action:'post-body-chunk',bodyToken:begun.bodyToken,offset,data:btoa(parts.join(''))});
    if(!reply?.ok)throw Error(reply?.error||'UDM lost this form body. This download remains in the browser.');
  }
  const finished=await nativeRequest({action:'post-body-finish',bodyToken:begun.bodyToken});
  if(!finished?.ok||!finished.request)throw Error(finished?.error||'UDM could not verify this form body. This download remains in the browser.');
  return finished.request;
}
const mediaHandoffs=typeof UdmMediaReceipts!=='undefined'?UdmMediaReceipts.create(api,nativeRequest):null;
const chromiumProxy=typeof UdmChromiumProxy!=='undefined'?UdmChromiumProxy.create(api):null;
chromiumProxy?.install();
const requestContext=typeof UdmRequestContext!=='undefined'?UdmRequestContext.create(api,captureNavigation,chromiumProxy):null;
requestContext?.install();
const grabberSessions=typeof UdmGrabberSession!=='undefined'?UdmGrabberSession.create(api,nativeRequest,captureNavigation):null;
const renderedGrabber=typeof UdmRenderedGrabber!=='undefined'?UdmRenderedGrabber.create(api,captureNavigation,nativeRequest):null;
const downloadSessions=typeof UdmDownloadSession!=='undefined'?UdmDownloadSession.create(api,captureNavigation):null;
const defaults = { capture: false, cookies: false, excluded: [], extensions: UdmFileRecognition.defaultExtensions };
const acceptable = url => /^https?:\/\//i.test(url || '');
let compiledSiteRules=null,siteRulesLoaded=0;
// Site rules come from the desktop policy; the compiled set is reused for five
// seconds between reads because the response listener evaluates every reply.
async function compiledSiteRuleSet(){
 if(typeof UdmCaptureRules==='undefined')return null;
 const now=Date.now();if(compiledSiteRules&&now-siteRulesLoaded<5000)return compiledSiteRules.compiled;
 const lines=(await api.storage.local.get('desktopPolicy')).desktopPolicy?.captureRules;siteRulesLoaded=now;
 if(!Array.isArray(lines)||!lines.length){compiledSiteRules=null;return null;}
 const key=JSON.stringify(lines);if(!compiledSiteRules||compiledSiteRules.key!==key)compiledSiteRules={key,compiled:UdmCaptureRules.compile(lines)};
 return compiledSiteRules.compiled;
}
// Last 50 rule decisions, without query values, for the diagnostics page.
const captureRuleDiagnostics=[];
function noteRuleDecision(point,url,type,decision){
 try{const u=new URL(url);captureRuleDiagnostics.push({at:Date.now(),point,host:u.hostname,path:u.pathname.replace(/[0-9]+/g,'#').slice(0,200),query:[...u.searchParams.keys()].slice(0,20),type:type||'',action:decision.action,extension:decision.extension||'',rule:decision.rule||''});if(captureRuleDiagnostics.length>50)captureRuleDiagnostics.shift();}catch{}
}
function evaluateSiteRule(compiled,url,method,settings,request={}){
 if(!compiled)return null;
 return UdmCaptureRules.evaluate(compiled,{url,method:method||'GET',background:request.background===true,proxy:request.proxy},ext=>UdmFileRecognition.allowed({extensions:[ext]},settings.extensions));
}
async function siteRuleDecision(url,method,settings,request={}){
 const decision=evaluateSiteRule(await compiledSiteRuleSet(),url,method,settings,request);
 if(decision)noteRuleDecision(request.point||'download',url,request.type||'',decision);return decision;
}
async function backgroundRuleDecision(event){
 if(!event.type||event.type==='main_frame'||!acceptable(event.url))return null;
 // An attachment navigation in an embedded frame can become a browser
 // download. Leave it to onCreated, which owns the cancellation transaction.
 if(event.type==='sub_frame'&&event.responseHeaders?.some(h=>
  h.name.toLowerCase()==='content-disposition'&&/^\s*attachment\s*(?:;|$)/i.test(h.value||'')))return null;
 const settings={...defaults,...(await api.storage.local.get('settings')).settings};
 return siteRuleDecision(event.url,event.method,settings,{background:true,type:event.type,point:'response',proxy:event.proxyInfo?.type==='https'?'https':undefined});
}
// A stream rule (V/A/C) names the extension; the panel needs a media type.
function ruleMediaType(extension){const known={mp4:'video/mp4',m4v:'video/mp4',webm:'video/webm',mkv:'video/x-matroska',flv:'video/x-flv',mov:'video/quicktime',avi:'video/x-msvideo',ts:'video/mp2t',mp3:'audio/mpeg',m4a:'audio/mp4',aac:'audio/aac',ogg:'audio/ogg',opus:'audio/ogg',flac:'audio/flac',wav:'audio/wav'};return known[extension]||('video/x-'+extension);}
const backgroundCaptures=new Map();
// An S rule captures a background request (XHR, media, object). The browser keeps
// its own request running, since cancelling needs a blocking permission the
// extension does not hold; UDM fetches the file through its own transfer.
async function captureBackgroundRequest(event){
 // A background response already belongs to the page. Only an observed,
 // complete GET can be fetched independently by the desktop.
 if(event.method!=='GET'||event.incognito||!event.requestId)return;
 const now=Date.now();for(const [k,at] of backgroundCaptures)if(at&&now-at>300000)backgroundCaptures.delete(k);
 const key=event.tabId+':'+event.url;
 if(backgroundCaptures.has(key))return;
 backgroundCaptures.set(key,0);
 try{
  const settings={...defaults,...(await api.storage.local.get('settings')).settings};
  if(!settings.capture||UdmMedia.policy.blocked(event.url,settings))return;
  const tab=await api.tabs.get(event.tabId).catch(()=>null);if(!tab||tab.incognito)return;
  const context=await requestContext?.resolveDownload({
   url:event.url,tabId:event.tabId,frameId:event.frameId??0,documentId:event.documentId
  },true,event.requestId);
  if(context?.method!=='GET'||context.partialDownload)return;
  const desktop=await nativeRequest({action:'preferences'}).catch(()=>null);
  if(!desktop?.ok||desktop.captureAllowed===false||
     UdmMedia.policy.blocked(event.url,UdmMedia.policy.merge(settings,desktop)))return;
  // Cached rules shortlist responses. The current desktop rules authorize capture.
  const compiled=UdmCaptureRules.compile(desktop.captureRules);
  const decision=UdmCaptureRules.evaluate(compiled,{
   url:event.url,method:event.method,background:true,proxy:event.proxyInfo?.type
  },ext=>UdmFileRecognition.allowed({extensions:[ext]},settings.extensions,desktop.extensions));
  if(decision?.action!=='capture')return;
  const item={url:event.url,filename:'',referrer:event.initiator||event.originUrl||event.documentUrl||'',tabId:event.tabId,frameId:event.frameId??0,browserDownload:false,siteRule:decision.rule};
  const prepared=await prepareBrowserHandoff(item,context,()=>desktop);
  await sendBrowserHandoff(prepared);
  backgroundCaptures.set(key,Date.now());
 }finally{
  if(!backgroundCaptures.get(key))backgroundCaptures.delete(key);
 }
}
const youtubeId = address => {try{const u=new URL(address);if(u.protocol!=='https:'||!['www.youtube.com','youtube.com','m.youtube.com','www.youtube-nocookie.com'].includes(u.hostname))return '';const id=u.pathname==='/watch'?u.searchParams.get('v'):/^\/(?:embed|shorts)\/([\w-]{11})/.exec(u.pathname)?.[1];return /^[\w-]{11}$/.test(id||'')?id:'';}catch{return '';}};
const youtubePage = address => !!youtubeId(address);
function youtubeRefresh(address){
  try{
    const marker=new URL(address).hash;
    const match=/^#udm-stream-refresh=([0-9a-f]{32})\.([av])(\d{1,6})$/.exec(marker);
    return match?{marker,token:match[1],audio:match[2]==='a',formatId:match[3]}:null;
  }catch{return null;}
}
const mediaQueues = new Map();
let policyRead=0,policyPending=null;
const captureIntents=new Map();
async function syncDesktopPolicy(){
 if(policyPending)return policyPending;
 if(Date.now()-policyRead<15000)return;
 policyRead=Date.now();policyPending=(async()=>{try{const data=await nativeRequest({action:'preferences-if-running'});if(data?.ok)await api.storage.local.set({desktopPolicy:data});}catch{}finally{policyPending=null;}})();return policyPending;
}
async function takeIntent(item,redirected){
 const now=Date.now(),matches=[];
 for(const [key,value] of captureIntents){
  if(now-value.time>8000){captureIntents.delete(key);continue;}
  const matchesSource=redirected
   ?key===redirected.tabId+':'+redirected.frameId&&redirected.redirectSources.some(source=>
     source.url===value.url&&source.page===value.page&&value.time<=source.time&&
     (!value.documentId||!source.documentId||value.documentId===source.documentId))
   :(value.url===item.url||value.url===item.finalUrl)&&(!item.referrer||item.referrer===value.page);
  if(matchesSource)matches.push([key,value]);
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
async function mediaContext(message,sender,displayOnly=false) {
  if(!youtubePage(message.url))throw new Error('Open a YouTube video first.');
  // Tools runs in an extension tab. Its own document cannot supply a video.
  if(isBrowserToolsSender(sender))sender={};
  const tabId=sender.tab?.id ?? message.tabId;
  const streamRefresh=youtubeRefresh(message.url);
  // A content-script message carries browser-issued tab/document identity.
  // Format display can bind its snapshot to that exact document without two
  // blocking tab lookups. Download actions still re-read the current tab.
  const boundDocument=displayOnly&&!streamRefresh&&sender.tab&&typeof sender.documentId==='string'?sender.documentId:'';
  let tab=boundDocument?sender.tab:await api.tabs.get(tabId);
  if(await browserControls?.isDisabled(tabId))throw Error('UDM is disabled on this tab.');
  const id=youtubeId(message.url),frameId=sender.tab?(sender.frameId??0):0;
  const navigatedSender=!!sender.tab&&youtubeId(sender.url)!==id;
  if(navigatedSender){
    // The browser can retain the original sender URL after same-document navigation.
    // Require current top-frame URL and exact injection document identity below.
    // The content script may have started on YouTube Home before the same
    // document navigated to /watch. Its original URL need not have a video ID.
    if(frameId!==0||!sender.documentId)throw new Error('The message did not come from this video page.');
    if(boundDocument)tab=await api.tabs.get(tabId);
    if(new URL(sender.url).origin!==new URL(tab.url).origin)throw new Error('The message did not come from this video page.');
  }
  if(tab.incognito)throw new Error('Media capture is disabled in private windows.');
  if(streamRefresh&&(frameId!==0||youtubeRefresh(tab.url)?.marker!==streamRefresh.marker))
    throw Error('The streaming recovery page changed. Reopen it from UDM.');
  if(frameId===0&&youtubeId(tab.url)!==id)throw new Error('The video page changed. Open UDM on the current page again.');
  return {tabId,tab,id,frameId,boundDocument,navigatedSender,streamRefresh};
}
async function availableMedia(message,sender,displayOnly=false,includeSession=false) {
  if(isBrowserToolsSender(sender))sender={};
  // Independent browser reads share one wait; policy still gates page access.
  const [context,stored]=await Promise.all([mediaContext(message,sender,displayOnly),api.storage.local.get(['settings','desktopPolicy'])]);
  const prefs=UdmMedia.policy.merge(stored.settings||{},stored.desktopPolicy||{});
  if(prefs.panelEnabled===false||UdmMedia.policy.panelBlocked(context.tab.url,prefs)||UdmMedia.policy.panelBlocked(message.url,prefs))throw Error('Video panels are disabled on this site.');
  const key='media:'+context.tabId;
  const [results,observations]=await Promise.all([api.scripting.executeScript({target:{tabId:context.tabId,frameIds:[context.frameId]},world:'MAIN',injectImmediately:true,func:readYouTubeFormats,args:[context.id,includeSession]}),api.storage.session.get(key)]);
  const injection=results.find(r=>r.frameId===context.frameId);
  if(context.navigatedSender&&(!injection?.documentId||injection.documentId!==sender.documentId))throw new Error('The video document changed. Refresh the list.');
  // Older browsers without document IDs retain the current-tab check. A new
  // document can never supply formats for the originating panel's message.
  if(!context.boundDocument||!injection?.documentId)await mediaContext(message,sender);
  if(sender.documentId&&injection?.documentId&&sender.documentId!==injection.documentId)throw new Error('The video document changed. Refresh the list.');
  const snapshot=injection?.result;context.documentId=injection?.documentId||sender.documentId||'';
  if(snapshot?.live)throw new Error('Live streams are not supported.');
  const observed=observations[key]||[];
  const captured=UdmFormats.attachObserved(snapshot,observed.filter(x=>x.frameId===context.frameId&&(context.documentId?x.documentId===context.documentId:!x.documentId&&Number.isFinite(snapshot?.timeOrigin)&&x.observedAt>=snapshot.timeOrigin)));
  const refresh=context.streamRefresh;
  const choices=refresh?.audio?[]:UdmFormats.choices(captured,context.id,refresh?.formatId)
    .filter(choice=>UdmMedia.policy.panelAllowed(choice,prefs));
  const audioChoices=refresh&&!refresh.audio?[]:UdmFormats.audioChoices(captured,context.id,true)
    .filter(choice=>(!refresh||choice.formatId===refresh.formatId)&&UdmMedia.policy.panelAllowed(choice,prefs));
  const captionChoices=refresh||!UdmMedia.policy.panelAllowed({container:'ttml'},prefs)
    ?[]:UdmFormats.captionChoices(captured,context.id);
  if(!choices.length&&!audioChoices.length&&!captionChoices.length)throw new Error('No downloadable qualities detected yet. Play this video, then refresh the list.');
  return {...context,choices,audioChoices,captionChoices,snapshot,prefs};
}
function sabrSelection(context,choice){
  const video=context.snapshot?.formats?.find(f=>f.id===choice.formatId&&!f.muxed&&/^video\/mp4\b/.test(f.mime));
  const audio=context.snapshot?.formats?.find(f=>f.audioDefault!==false&&/^audio\/mp4\b/.test(f.mime)&&/mp4a/.test(f.mime));
  if(!video||!audio||!/^\d+$/.test(video.lastModified||'')||!/^\d+$/.test(audio.lastModified||''))return null;
  const select=f=>({id:f.id,lastModified:f.lastModified,xtags:f.xtags||'',mime:f.mime});
  return {durationMs:context.snapshot.durationMs,video:select(video),audio:select(audio)};
}
function validSabrSession(session,videoId){
  const now=Date.now();
  if(!session||session.videoId!==videoId||typeof session.url!=='string'||typeof session.body!=='string'||!session.body.length||session.body.length>175000||
    !Number.isFinite(session.capturedAt)||session.capturedAt<now-300000||session.capturedAt>now+30000)return false;
  try{
    const url=new URL(session.url);
    if(url.protocol!=='https:'||url.username||url.password||(url.port&&url.port!=='443')||
      !url.hostname.endsWith('.googlevideo.com')||url.pathname!=='/videoplayback')return false;
    const expires=url.searchParams.get('expire');
    return expires===null||(/^\d{1,12}$/.test(expires)&&Number(expires)*1000>now);
  }catch{return false;}
}
async function readSabrSession(context){
  const preferLatest=async page=>{
    if(typeof UdmStreamingCapture==='undefined')return page;
    const observed=await (page?
      mediaStep(()=>UdmStreamingCapture.session(context,false),'Browser capture lookup timed out.',150):
      UdmStreamingCapture.session(context)).catch(()=>null);
    // Keep the identity-verified page capture when both observers saw the same request.
    return validSabrSession(observed,context.id)&&(!page||observed.capturedAt>page.capturedAt&&
      (observed.url!==page.url||observed.body!==page.body))?observed:page;
  };
  const bundled=context.snapshot?.streamSession;
  if(validSabrSession(bundled,context.id))return preferLatest(bundled);
  const result=await api.scripting.executeScript({
    target:{tabId:context.tabId,frameIds:[context.frameId]},world:'MAIN',
    func:id=>({
      session:globalThis.__udmCaptureV1?.session?.(id),
      visit:globalThis.__udmCaptureV1?.visit?.(id),
      timeOrigin:performance.timeOrigin
    }),args:[context.id]
  });
  const row=result.find(r=>r.frameId===context.frameId);
  if(context.documentId?row?.documentId!==context.documentId:
    !Number.isFinite(context.snapshot?.timeOrigin)||row?.result?.timeOrigin!==context.snapshot.timeOrigin)return null;
  if(row?.result?.visit!==context.snapshot?.visit)return null;
  const captured=row?.result?.session;
  return preferLatest(validSabrSession(captured,context.id)?captured:null);
}
async function sabrFor(context,choice){
  const selection=sabrSelection(context,choice);
  if(!selection)return null;
  const session=await readSabrSession(context);
  return session?{url:session.url,body:session.body,videoId:context.id,capturedAt:session.capturedAt,...selection}:null;
}
// Bound browser/host waits so a stalled API cannot leave the panel busy indefinitely.
// Each awaited step must finish before handoff continues; a late read cannot create a job.
function mediaStep(operation,error,timeout=12000){
  let timer;
  return Promise.race([Promise.resolve().then(operation),new Promise((_,reject)=>{
    timer=setTimeout(()=>reject(new Error(error)),timeout);
  })]).finally(()=>clearTimeout(timer));
}
async function validateYouTubeSelection(message,sender,request,selectionContext,check,current){
  check();
  const audioFile=request.action==='add';
  // Receipt preparation can outlive a same-document navigation. Check
  // the visit and current policy again at the dispatch boundary.
  if(audioFile&&selectionContext.canCredentials&&
    !await api.permissions.contains({permissions:['cookies'],origins:['http://*/*','https://*/*']}))
    throw Error('Cookie permissions changed before download. Refresh the panel.');
  current??=await mediaStep(()=>availableMedia(message,sender),mediaReadTimeout);
  const audioOnly=request.output==='audio'||audioFile;
  const selected=audioOnly?current.audioChoices.find(track=>track.key===message.audioKey):
    current.choices.find(track=>track.key===message.formatKey&&track.height===Number(message.height));
  if(!selected||!sameMediaDocument(selectionContext,current))throw Error('The video or format changed before handoff. Refresh the list.');
  let transportCurrent,reportedSize=request.reportedSize;
  if(audioFile)transportCurrent=selected.audioUrl===request.url&&selected.lastModified===selectionContext.audioVersion;
  else if(request.action==='sabr'){
    if(!validSabrSession(request.sabr,current.id))
      throw Error('The browser streaming session expired. Play the video and refresh the list.');
    const tracks=audioOnly?{durationMs:current.snapshot.durationMs,audio:selected.streamFormat}:sabrSelection(current,selected);
    transportCurrent=!!tracks&&tracks.durationMs===request.sabr.durationMs&&
      JSON.stringify(tracks.audio)===JSON.stringify(request.sabr.audio)&&
      (audioOnly||JSON.stringify(tracks.video)===JSON.stringify(request.sabr.video));
    reportedSize=selected.size;
  }else if(selectionContext.preparedPlayerPair)
    transportCurrent=validPlayerPair(selectionContext.preparedPlayerPair,current,selected);
  else{
    transportCurrent=!!selected.videoUrl;
    reportedSize=selected.size;
  }
  if(!transportCurrent)throw Error('The selected video or audio stream changed before handoff. Refresh the list.');
  if(!UdmMedia.policy.panelAllowed({...selected,size:reportedSize||selected.size},current.prefs))
    throw Error('This video is below the minimum size in video-panel settings.');
  if(audioFile&&selectionContext.canCredentials&&!current.prefs.cookies)
    throw Error('Cookie permissions changed before download. Refresh the panel.');
  check();
  if(!audioFile){
    request.reportedSize=Number.isSafeInteger(reportedSize)&&reportedSize>0?reportedSize:0;
    if(request.action==='media'&&!selectionContext.preparedPlayerPair){
      request.videoUrl=selected.videoUrl;
      request.audioUrl=selected.audioUrl;
      request.pixelHeight=selected.pixelHeight||0;
    }
  }
}
async function submitYouTubeSelection(message,sender,prepare,error){
  const timeout=20000;let deadline=Date.now()+timeout;
  const expired='Reading this video took too long. No download was sent to UDM. Refresh the panel and try again.';
  const uncertain='UDM did not reply in time. Check its download list before trying again.';
  let dispatched=false,selectionContext,readPreferences;
  const check=()=>{if(Date.now()>=deadline)throw Error(dispatched?uncertain:expired);};
  // The selected quality identifies the receipt even when its playback URL
  // has expired or the player cannot supply that stream again.
  const videoId=youtubeId(message.url);
  const refreshToken=youtubeRefresh(message.url)?.token;
  const binding=JSON.stringify({type:'youtube',id:videoId,format:message.formatKey||'',height:Number(message.height)||0,
    audio:message.audioKey||'',...(refreshToken?{refreshToken}:{})});
  if(videoId)warmMediaConnection();
  if(mediaHandoffs){
    const previous=await mediaStep(()=>mediaHandoffs.recoverSelection(binding,check,()=>mediaContext(message,sender)),expired,timeout);
    check();
    if(previous){void report('').catch(()=>{});return previous;}
  }
  const prepareDesktop=()=>readPreferences??=mediaHandoffs?.preparePreferences?.();
  const request=await mediaStep(()=>prepare(check,context=>{selectionContext=context;},prepareDesktop),expired,Math.max(0,deadline-Date.now()));
  if(refreshToken){
    request.streamRefreshToken=refreshToken;
    request.url='https://www.youtube.com/watch?v='+videoId;
    request.referrer=request.url;
  }
  // Preparation can reply late, but only this continuation may submit a download.
  check();
  let result;
  const directAudio=request.action==='add'&&request.youtubeDirectAudio===true;
  {
    const controller=new AbortController();
    let timer,rejectDeadline;
    const expiration=new Promise((_,reject)=>{rejectDeadline=reject;});
    const expire=()=>{
      controller.abort();
      rejectDeadline(Error(dispatched?'UDM has not confirmed this download. Check its download list and any saved recovery receipt before retrying.':expired));
    };
    const markDispatched=()=>{
      check();dispatched=true;deadline=Date.now()+40000;
      clearTimeout(timer);timer=setTimeout(expire,40000);
    };
    const validateSelection=(wireRequest=request)=>validateYouTubeSelection(message,sender,wireRequest,selectionContext,check);
    const scope={signal:controller.signal,check,readPreferences,send:operation=>operation(markDispatched,validateSelection)};
    timer=setTimeout(expire,Math.max(0,deadline-Date.now()));
    try{
      const submission=mediaHandoffs&&(request.action==='media'||request.action==='sabr'||directAudio)
        ?mediaHandoffs.submit(request,scope,binding,request.filename):nativeRequest(request,markDispatched,validateSelection);
      result=await Promise.race([submission,expiration]);
    }finally{clearTimeout(timer);controller.abort();}
  }
  if(!result?.ok)throw Error(result?.error||error);
  void report('').catch(()=>{});return result;
}
// Short-lived memory cache binds native retrieval to the current video document.
// No signed URL, cookie or player response is written to extension storage.
const playerPairs=new Map(),playerReports=new Map(),playerWarmups=new Map();let playerRequests=0;
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
function clearPlayerPairs(tabId){
 playerReports.delete(tabId);
 for(const [key,entry] of playerPairs)if(entry.tabId===tabId)playerPairs.delete(key);
 for(const [key,entry] of playerWarmups)if(entry.context.tabId===tabId)playerWarmups.delete(key);
}
function validPlayerPair(pair,context,choice){
 if(!pair?.ok||pair.transport!=='player-direct'||pair.videoId!==context.id||pair.formatId!==choice.formatId||pair.height!==choice.height||!Number.isInteger(pair.pixelHeight)||pair.pixelHeight<1||pair.pixelHeight>4320||pair.userAgent!=='Mozilla/5.0 (Macintosh; Intel Mac OS X 15_7_3) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/26.0 Safari/605.1.15'||!Number.isFinite(pair.validatedAt)||Date.now()-pair.validatedAt>60000||pair.validatedAt>Date.now()+5000)return false;
 try{for(const kind of ['video','audio']){
   const stream=pair[kind],u=new URL(stream.url);
   if(u.protocol!=='https:'||u.username||u.password||(u.port&&u.port!=='443')||!u.hostname.endsWith('.googlevideo.com')||u.pathname!=='/videoplayback'||stream.kind!==kind||!Number.isSafeInteger(stream.size)||stream.size<=0||!Number.isSafeInteger(stream.itag)||u.searchParams.get('itag')!==String(stream.itag)||Number(u.searchParams.get('expire'))*1000<Date.now()+120000||!/^\d+$/.test(u.searchParams.get('expire')||''))return false;
   if(kind==='video'&&String(stream.itag)!==choice.formatId)return false;
   if(['sabr','sq','ump','n','range','rn','rbuf'].some(key=>u.searchParams.has(key)))return false;
 }return true;}catch{return false;}
}
async function nativePlayerPair(context,choice,allowNew=true){
 if(choice.videoUrl||!/^\d{1,6}$/.test(choice.formatId)||(!context.documentId&&!Number.isFinite(context.snapshot?.timeOrigin)))return null;
 const key=JSON.stringify([context.tabId,context.frameId,context.documentId||context.snapshot.timeOrigin,context.snapshot?.visit,context.id,choice.formatId,choice.height]);
 const previous=playerPairs.get(key);if(previous&&Date.now()<previous.until)return previous.promise;
 if(!allowNew||playerRequests>=2)return null;
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
function warmPlayerChoice(context){
 if(context.streamRefresh)return null;
 const choices=context.choices.filter(x=>!x.videoUrl&&/^\d{1,6}$/.test(x.formatId));
 const choice=choices.find(x=>x.height<=1080)||choices[0];
 return choice&&!(context.snapshot?.streamReady&&sabrSelection(context,choice))?choice:null;
}
function rememberPlayerWarmup(context,choice){
 const {tabId,frameId,documentId,id,snapshot}=context;
 if(!Number.isSafeInteger(snapshot?.visit)||!Number.isFinite(snapshot.timeOrigin))return;
 const {visit,timeOrigin,signatureTimestamp,visitorData}=snapshot;
 const {formatId,height,size,container}=choice;
 const key=tabId+':'+frameId;
 playerWarmups.delete(key);
 if(playerWarmups.size>=32)playerWarmups.delete(playerWarmups.keys().next().value);
 playerWarmups.set(key,{until:Date.now()+90000,choice:{formatId,height,size,container},canStream:!!sabrSelection(context,choice),
  context:{tabId,frameId,documentId,id,snapshot:{visit,timeOrigin,signatureTimestamp,visitorData}}});
}

const mediaReadTimeout='The browser took too long to read this video. Refresh the video page and try again.';
async function warmOpenedVideoPanel(message,sender){
 const tabId=sender.tab?.id;
 if(sender.id!==api.runtime.id||!Number.isInteger(tabId)||tabId<0||sender.tab.incognito||
    !acceptable(sender.url)||!acceptable(message.page))return {ok:false};
 const page=new URL(message.page),source=new URL(sender.url);
 if(page.origin!==source.origin)return {ok:false};
 const [tab,disabled,stored]=await Promise.all([
  api.tabs.get(tabId),browserControls?.isDisabled(tabId),api.storage.local.get(['settings','desktopPolicy'])
 ]);
 const prefs=UdmMedia.policy.merge(stored.settings||{},stored.desktopPolicy||{});
 if(tab.incognito||disabled||prefs.panelEnabled===false||UdmMedia.policy.panelBlocked(tab.url,prefs)||
    UdmMedia.policy.panelBlocked(page.href,prefs))return {ok:false};
 const frameId=sender.frameId??0;
 if(frameId===0&&tab.url!==page.href)return {ok:false};
 warmMediaConnection();
 if(youtubePage(message.youtubeUrl)&&youtubeId(page.href)===youtubeId(message.youtubeUrl)&&!youtubeRefresh(message.youtubeUrl)){
  const key=tabId+':'+frameId,prepared=playerWarmups.get(key),id=youtubeId(message.youtubeUrl);
  if(prepared?.context.id===id&&prepared.until>Date.now()){
   const {context,choice}=prepared;
   if((sender.documentId&&context.documentId&&sender.documentId!==context.documentId)||
      !UdmMedia.policy.panelAllowed(choice,prefs))return {ok:true};
   const rows=await mediaStep(()=>api.scripting.executeScript({target:{tabId,frameIds:[frameId]},world:'MAIN',
    injectImmediately:true,func:(id,url)=>{
     if(location.href!==url)return null;
     const capture=globalThis.__udmCaptureV1,player=document.getElementById('movie_player');
     const active=player?.getVideoData?.()?.video_id;
     const session=capture?.session?.(id);
     if(((active&&active!==id)||player?.classList.contains('ad-showing')||
         player?.classList.contains('ad-interrupting'))&&!session)return null;
     return {visit:capture?.visit?.(id),timeOrigin:performance.timeOrigin,streamReady:!!session};
    },args:[id,page.href]}),mediaReadTimeout);
   const row=rows.find(entry=>entry.frameId===frameId);
   if(playerWarmups.get(key)===prepared&&prepared.until>Date.now()&&!(prepared.canStream&&row?.result?.streamReady)&&
      sameMediaDocument(context,{documentId:row?.documentId,snapshot:row?.result}))
    void nativePlayerPair(context,choice);
  }else{
   const context=await mediaStep(()=>availableMedia({url:message.youtubeUrl},sender,true),mediaReadTimeout);
   const choice=warmPlayerChoice(context);
   if(choice)void nativePlayerPair(context,choice);
  }
 }
 return {ok:true};
}
function youtubeCaptionName(context,choice){
 const title=Array.from(UdmFormats.videoTitle(context.snapshot)).slice(0,112).join('');
 const track=Array.from(choice.description).slice(0,52).join('');
 return (title+' - '+track+'.ttml').replace(/[\\/:*?"<>|\x00-\x1f]/g,'_');
}
async function captionHandoff(message,sender){
 if(!sender.tab||!acceptable(message.page)||youtubeId(message.page)!==youtubeId(message.url)||
    !/^caption:[\w.-]{1,100}$/.test(message.formatKey||''))throw Error('Reopen the current YouTube video and select its subtitles again.');
 const initial=await mediaStep(()=>availableMedia(message,sender),mediaReadTimeout);
 const choice=initial.captionChoices.find(item=>item.key===message.formatKey);
 if(!choice)throw Error('The subtitle track is no longer available. Refresh the list.');
 const row={url:choice.url,page:message.page,tabId:initial.tabId,frameId:initial.frameId,documentId:initial.documentId};
 return handoff({...row,referrer:message.page,filename:youtubeCaptionName(initial,choice)},undefined,undefined,
  send=>send(undefined,async()=>{
   await browserControls.verifySource(initial.tab,[row]);
   const current=await mediaStep(()=>availableMedia(message,sender),mediaReadTimeout);
   if(!sameMediaDocument(initial,current)||!current.captionChoices.some(item=>item.key===choice.key&&item.url===choice.url))
    throw Error('The video or subtitle address changed. Refresh the list.');
  }));
}
function sameMediaDocument(context,current){
 if(context.snapshot?.visit!==current.snapshot?.visit)return false;
 return context.documentId?context.documentId===current.documentId:
  Number.isFinite(context.snapshot?.timeOrigin)&&context.snapshot.timeOrigin===current.snapshot?.timeOrigin;
}
async function audioSession(context,choice){
  if(!choice.streamFormat)return null;
  const session=await readSabrSession(context);
  return session?{url:session.url,body:session.body,videoId:context.id,capturedAt:session.capturedAt,
    durationMs:context.snapshot.durationMs,audio:choice.streamFormat}:null;
}
function audioHandoff(message,sender){
  return submitYouTubeSelection(message,sender,(check,remember,prepareDesktop)=>prepareYouTubeAudio(message,sender,check,remember,prepareDesktop),'UDM did not accept the audio.');
}
async function prepareYouTubeAudio(message,sender,check,remember,prepareDesktop) {
  const context=await mediaStep(()=>availableMedia(message,sender,false,true),mediaReadTimeout);
  check();
  const choice=context.audioChoices.find(x=>x.key===message.audioKey);
  if(!choice)throw Error('That audio track is no longer available. Play the video, then refresh the list.');
  const readPreferences=prepareDesktop?.();
  const streamingRefresh=!!context.streamRefresh;
  const initialDirect=!streamingRefresh&&!!choice.audioUrl;
  let sabr=null;
  if(!initialDirect){
    // Capability discovery and the read-only session lookup are independent.
    // Promise.all observes both failures; neither operation can submit a job.
    [,sabr]=await Promise.all([
      mediaStep(()=>nativeRequest({action:'hello'}),'UDM did not reply. Check the desktop connection.').then(host=>{
        if(!host?.ok||!host.capabilities?.includes('sabr-audio'))throw Error('Update the UDM desktop app to use YouTube streaming audio.');
      }),
      mediaStep(()=>audioSession(context,choice),mediaReadTimeout)
    ]);
    if(!sabr)throw Error('Play this video to capture its audio session, then refresh the list.');
  }
  check();
  // Refresh the signed URL before resolving its browser request context.
  // The dispatch guard checks the exact URL again after that work.
  const confirmed=await mediaStep(()=>availableMedia(message,sender),mediaReadTimeout);
  const current=confirmed.audioChoices.find(x=>x.key===choice.key);
  const useDirect=!streamingRefresh&&!!current?.audioUrl;
  if(!sameMediaDocument(context,confirmed)||!current||current.lastModified!==choice.lastModified||
    !streamingRefresh&&choice.audioUrl&&(!useDirect||current.resource!==choice.resource||
      new URL(current.audioUrl).searchParams.get('lmt')!==new URL(choice.audioUrl).searchParams.get('lmt'))||
    !useDirect&&(!sabr||JSON.stringify(current.streamFormat)!==JSON.stringify(choice.streamFormat)||confirmed.snapshot.durationMs!==sabr.durationMs))
    throw Error('The video or audio track changed before handoff. Refresh the list.');
  const clean=value=>String(value||'').replace(/[<>:"/\\|?*\x00-\x1f\x7f]/g,' ').trim().replace(/[. ]+$/,'');
  const filename=(clean(UdmFormats.videoTitle(confirmed.snapshot,message.title||confirmed.tab.title||'YouTube audio')).slice(0,150)||'YouTube audio')+' - '+clean(current.label).slice(0,70)+'.m4a';
  if(useDirect){
    check();
    const {message:request,canCredentials}=await prepareBrowserHandoff(
      {url:current.audioUrl,filename,referrer:message.url,tabId:confirmed.tabId,frameId:confirmed.frameId,documentId:confirmed.documentId},
      undefined,readPreferences?()=>readPreferences(check):undefined);
    check();
    if(canCredentials&&!await api.permissions.contains({permissions:['cookies'],origins:['http://*/*','https://*/*']}))throw Error('Cookie permissions changed before download. Refresh the panel.');
    remember({...confirmed,audioVersion:current.lastModified,canCredentials});
    if(!Object.keys(request.request||{}).length){
      const length=Number(new URL(current.audioUrl).searchParams.get('clen'));
      const signedSize=Number.isSafeInteger(length)&&length>0?length:0;
      const reportedSize=signedSize&&current.size&&signedSize!==current.size?0:signedSize||current.size;
      return {...request,youtubeDirectAudio:true,youtubePage:message.url,
        reportedDirectSize:Number.isSafeInteger(reportedSize)&&reportedSize>0?reportedSize:undefined};
    }
    return request;
  }
  remember(confirmed);
  return {action:'sabr',output:'audio',reportedSize:Number.isSafeInteger(current.size)&&current.size>0?current.size:0,url:'https://www.youtube.com/watch?v='+context.id,height:0,pixelHeight:0,formatId:current.formatId,exactQuality:true,sabr,filename,referrer:message.url,userAgent:navigator.userAgent};
}
function mediaHandoff(message,sender){
  return submitYouTubeSelection(message,sender,(check,remember,prepareDesktop)=>prepareYouTubeVideo(message,sender,check,remember,prepareDesktop),'UDM did not accept the video.');
}
// Notifications only wake the bounded wait. The subsequent format and
// session reads still prove that the selected stream is usable.
const youtubeStreamWaiters=new Map();
const youtubeWaitKey=(tab,frame,document,id)=>JSON.stringify([tab,frame,document||'',id]);
function wakeYouTubeStream(tab,frame,document,id){
  const key=youtubeWaitKey(tab,frame,document,id);
  for(const wake of youtubeStreamWaiters.get(key)||[])wake();
}
function waitForYouTubeStream(context,pendingPair){
  const key=youtubeWaitKey(context.tabId,context.frameId,context.documentId,context.id);
  let waiters=youtubeStreamWaiters.get(key);
  if(!waiters)youtubeStreamWaiters.set(key,waiters=new Set());
  return new Promise(resolve=>{
    const finish=()=>{
      clearTimeout(timer);
      if(!waiters.delete(finish))return;
      if(!waiters.size)youtubeStreamWaiters.delete(key);
      resolve();
    };
    const timer=setTimeout(finish,400);
    waiters.add(finish);
    pendingPair?.then(pair=>{if(pair)finish();},()=>{});
  });
}
async function prepareYouTubeVideo(message,sender,check,remember,prepareDesktop,batchContext) {
  let context=batchContext??await mediaStep(()=>availableMedia(message,sender,false,true),mediaReadTimeout);
  check();
  let choice=context.choices.find(x=>x.key===message.formatKey&&x.height===Number(message.height));
  if(!choice)throw new Error('That quality is no longer available. Refresh the list and choose again.');
  prepareDesktop?.();
  const streamingRefresh=!!context.streamRefresh;
  let confirmed=!streamingRefresh&&choice.videoUrl?context:null;
  if(streamingRefresh)choice={...choice,videoUrl:'',audioUrl:''};
  // Prefer a ready direct pair, but do not hold an already captured stream
  // behind a slow player lookup or start a new lookup for an admitted capture.
  const readyCapture=!choice.videoUrl&&validSabrSession(context.snapshot?.streamSession,context.id)&&sabrSelection(context,choice);
  let resolvedPair=null;
  const retrievePair=allowNew=>nativePlayerPair(context,choice,allowNew).then(pair=>{resolvedPair=pair;return pair;});
  let pendingPair=choice.videoUrl||streamingRefresh?null:retrievePair(!readyCapture);
  // Read the captured stream concurrently with direct retrieval. Keep errors
  // as values until fallback is needed: a usable direct pair must not wait for
  // (or fail because of) an unrelated browser capture read.
  const pendingCapture=choice.videoUrl?null:mediaStep(()=>sabrFor(context,choice),mediaReadTimeout)
    .then(value=>({value}),error=>({error}));
  let retrieved=null;
  if(pendingPair)retrieved=await Promise.race([pendingPair,pendingCapture.then(()=>new Promise(resolve=>setTimeout(()=>resolve(resolvedPair),0)))]);
  if(retrieved)choice={...choice,videoUrl:retrieved.video.url,audioUrl:retrieved.audio.url,pixelHeight:retrieved.pixelHeight};
  let sabr=null;
  if(!choice.videoUrl){
    // A native pair arriving later can still rescue a slow
    // browser read. An unavailable pair must not mask that read's result.
    const capture=await (pendingPair?Promise.race([pendingCapture,pendingPair.then(pair=>pair?{pair}:pendingCapture)]):pendingCapture);
    if(capture.pair){retrieved=capture.pair;choice={...choice,videoUrl:retrieved.video.url,audioUrl:retrieved.audio.url,pixelHeight:retrieved.pixelHeight};}
    else{
      if(capture.error)throw capture.error;
      sabr=capture.value;
      const bundled=context.snapshot?.streamSession;
      if(sabr&&bundled&&sabr.capturedAt===bundled.capturedAt&&sabr.url===bundled.url&&sabr.body===bundled.body)
        confirmed=context;
    }
  }
  // The capture can expire while it is being resolved. Restore direct fallback
  // in that case before asking the page to prepare a fresh stream.
  if(!choice.videoUrl&&!sabr&&readyCapture&&!streamingRefresh)pendingPair=retrievePair(true);
  if(!choice.videoUrl&&!sabr&&!batchContext){
    check();
    await mediaStep(()=>api.scripting.executeScript({target:{tabId:context.tabId,frameIds:[context.frameId]},world:'MAIN',func:(id,height)=>globalThis.__udmCaptureV1?.prepare(id,height),args:[context.id,choice.height]}),mediaReadTimeout);
    for(let attempt=0;attempt<8&&!choice.videoUrl&&!sabr;attempt++){
      // Preparation may have captured the session already; only retries need a delay.
      if(attempt&&!resolvedPair)await waitForYouTubeStream(context,pendingPair);
      check();
      const latest=await mediaStep(()=>availableMedia(message,sender,false,true),mediaReadTimeout);
      check();
      if(!sameMediaDocument(context,latest))throw new Error('The video document changed before handoff. Refresh the list.');
      context=latest;
      choice=latest.choices.find(x=>x.key===message.formatKey&&x.height===Number(message.height));
      if(!choice)throw new Error('The video or available formats changed. Refresh the list.');
      if(streamingRefresh)choice={...choice,videoUrl:'',audioUrl:''};
      if(!choice.videoUrl&&resolvedPair){retrieved=resolvedPair;choice={...choice,videoUrl:retrieved.video.url,audioUrl:retrieved.audio.url,pixelHeight:retrieved.pixelHeight};}
      if(!choice.videoUrl)sabr=await mediaStep(()=>sabrFor(context,choice),mediaReadTimeout);
      if(choice.videoUrl||sabr)confirmed=context;
    }
  }
  if(!choice.videoUrl&&!sabr&&pendingPair){retrieved=await pendingPair;if(retrieved)choice={...choice,videoUrl:retrieved.video.url,audioUrl:retrieved.audio.url,pixelHeight:retrieved.pixelHeight};}
  if(!choice.videoUrl&&!sabr)throw new Error('This quality is listed by the player, but UDM has not captured a downloadable stream yet. Play the video at this quality, then retry.');
  check();
  // A direct URL, a bundled session or a successful retry already has a
  // current format snapshot. Dispatch independently rechecks the selection.
  confirmed??=await mediaStep(()=>availableMedia(message,sender),mediaReadTimeout);
  const currentChoice=confirmed.choices.find(x=>x.key===choice.key&&x.height===choice.height);
  if(!sameMediaDocument(context,confirmed)||!currentChoice)throw new Error('The video or format changed before handoff. Refresh the list.');
  // Rebind the transport as well as the quality: direct URLs can arrive or be
  // replaced while the browser is reading its captured streaming session.
  if(!streamingRefresh&&currentChoice.videoUrl){choice=currentChoice;sabr=null;}
  else if(choice.videoUrl&&!retrieved)throw new Error('The direct video links changed before handoff. Refresh the list and choose the quality again.');
  if(sabr){
    // Reuse the final metadata read; do not add another browser round trip.
    const selected=sabrSelection(confirmed,currentChoice);
    if(!selected||selected.durationMs!==sabr.durationMs||JSON.stringify(selected.video)!==JSON.stringify(sabr.video)||JSON.stringify(selected.audio)!==JSON.stringify(sabr.audio))throw Error('The video or audio stream changed before handoff. Refresh the list.');
    choice=currentChoice; // Refresh reported lengths after proving track identity.
  }
  const knownSize=retrieved&&!currentChoice.videoUrl?(retrieved.video.size>0&&retrieved.audio.size>0?retrieved.video.size+retrieved.audio.size:0):choice.size;
  if(!UdmMedia.policy.panelAllowed({...choice,size:knownSize},confirmed.prefs))throw Error('This video is below the minimum size in video-panel settings.');
  remember({...confirmed,preparedPlayerPair:retrieved&&!currentChoice.videoUrl?retrieved:null});
  return {action:sabr?'sabr':'media',sabr,reportedSize:Number.isSafeInteger(knownSize)&&knownSize>0?knownSize:0,url:message.url,filename:UdmFormats.videoTitle(confirmed.snapshot,message.title||confirmed.tab.title||'YouTube video'),height:choice.height,pixelHeight:choice.pixelHeight||0,formatId:choice.formatId,exactQuality:true,videoUrl:sabr?'':choice.videoUrl,audioUrl:sabr?'':choice.audioUrl,referrer:message.url,userAgent:retrieved&&!currentChoice.videoUrl?retrieved.userAgent:navigator.userAgent};
}
async function reviewYouTubeVideoBatch(message,sender){
  const {page,youtubeChoices}=message,id=youtubeId(page);
  if(!id||!sender.tab||!Number.isInteger(sender.tab.id)||youtubeRefresh(page)||!Array.isArray(youtubeChoices)||
    youtubeChoices.length<2||youtubeChoices.length>80||
    youtubeChoices.some(choice=>!choice||typeof choice.key!=='string'||!Number.isInteger(choice.height)||
      !(choice.height===0?/^caption:[\w.-]{1,100}$/.test(choice.key):/^(?:\d{1,6}|height:\d{3,4})$/.test(choice.key)))||
    new Set(youtubeChoices.map(choice=>choice.key)).size!==youtubeChoices.length)throw Error('Reopen the current YouTube video and select its formats again.');
  const deadline=Date.now()+20000;
  const expired='The YouTube formats could not be prepared in time. No review was sent to UDM.';
  let closed=false;
  const check=()=>{if(closed||Date.now()>=deadline)throw Error(expired);};
  const url='https://www.youtube.com/watch?v='+id;
  const initial=await mediaStep(()=>availableMedia({url},sender),mediaReadTimeout);
  check();
  const byKey=new Map(initial.choices.map(choice=>[choice.key,choice]));
  const captions=new Map(initial.captionChoices.map(choice=>[choice.key,choice]));
  const selectedCaptions=[];
  const selections=new Map();
  let preparation;
  const title=Array.from(UdmFormats.videoTitle(initial.snapshot)).slice(0,150).join('');
  const rows=youtubeChoices.map(({key,height})=>{
    if(height===0){
      const choice=captions.get(key);
      if(!choice)throw Error('The YouTube subtitles changed. Reopen the video panel.');
      selectedCaptions.push(choice);
      return {url:choice.url,page,tabId:initial.tabId,frameId:initial.frameId,documentId:initial.documentId,
        linkText:choice.label,suggestedName:youtubeCaptionName(initial,choice)};
    }
    const choice=byKey.get(key);
    if(!choice||choice.height!==height)throw Error('The YouTube formats changed. Reopen the video panel.');
    return {url,page,tabId:initial.tabId,frameId:initial.frameId,documentId:initial.documentId,mediaKey:key,youtube:true,
      linkText:choice.label,suggestedName:title+' - '+height+'p.mp4'};
  });
  try{
    const result=await reviewBrowserBatch(rows,true,async()=>{
      check();
      const current=await mediaStep(()=>availableMedia({url},sender),mediaReadTimeout);
      if(!sameMediaDocument(initial,current)||selectedCaptions.some(choice=>
        !current.captionChoices.some(item=>item.key===choice.key&&item.url===choice.url)))
        throw Error('The video or subtitle address changed. Reopen the video panel.');
      for(const {selection,request,context} of selections.values())
        await validateYouTubeSelection(selection,sender,request,context,check,current);
      check();
    },async row=>{
      check();
      const selection={url,formatKey:row.mediaKey,height:byKey.get(row.mediaKey).height};
      let context;
      const request=await mediaStep(async()=>{
        // Read once after the batch capability checks. Each row still selects
        // its own transport, and dispatch validates all prepared requests.
        const snapshot=await (preparation??=mediaStep(()=>availableMedia({url},sender,false,true),mediaReadTimeout));
        check();
        if(!sameMediaDocument(initial,snapshot))throw Error('The video changed. Reopen the video panel.');
        return prepareYouTubeVideo(selection,sender,check,value=>{context=value;},undefined,snapshot);
      },expired,Math.max(0,deadline-Date.now())).catch(error=>{throw Error(row.linkText+' — '+error.message);});
      check();
      if(!sameMediaDocument(initial,context))throw Error('The video changed. Reopen the video panel.');
      selections.set(row.mediaKey,{selection,request,context});
      return request;
    });
    return result||{unsupported:true};
  }finally{closed=true;}
}
if(typeof UdmStreamingCapture!=='undefined')UdmStreamingCapture.install(api,captureNavigation,wakeYouTubeStream);
const report = async text => {
  // Cosmetic reporting must never delay acknowledgement or browser recovery.
  void Promise.resolve().then(()=>api.storage.local.set({lastError:text})).catch(()=>{});
  void Promise.resolve().then(()=>api.action.setBadgeText({text:text?'!':''})).catch(()=>{});
};
const browserControls=typeof UdmBrowserControls!=='undefined'?UdmBrowserControls.create(api,handoff,report,row=>{const observed=requestContext?.resolve({url:row.url,referrer:row.page,tabId:row.tabId,frameId:row.frameId,documentId:row.documentId},false);if(observed&&observed.method!=="GET")return {error:"Original request requires download confirmation"};if(!observed||observed.pending||observed.response?.ambiguous)return null;return {size:Number.isSafeInteger(observed.size)&&observed.size>0?observed.size:null,type:observed.mime||"",source:"Browser response"};},reviewBrowserBatch):null;
const keyCapture=typeof UdmKeyCapture!=='undefined'?UdmKeyCapture.create(api,async()=>{const s=await api.storage.local.get(['settings','desktopPolicy']);return UdmMedia.policy.merge({...defaults,...s.settings},s.desktopPolicy||{});},async id=>!!(await browserControls?.isDisabled(id)),requestContext,handoff):null;
const captureRecovery=typeof UdmCaptureRecovery!=='undefined'?UdmCaptureRecovery.create(api,nativeRequest,report,canCancelPreparedDownload):null;
captureRecovery?.install();
async function canShareBrowserCredentials(settings){
  if(!settings)({settings}=await api.storage.local.get('settings'));
  return !!settings?.cookies&&await api.permissions.contains({permissions:['cookies'],origins:['http://*/*','https://*/*']});
}
async function canCancelPreparedDownload(item,sharedCredentials){
  const {settings,desktopPolicy}=await api.storage.local.get(['settings','desktopPolicy']);
  const address=item.finalUrl||item.url;
  if(!settings?.capture||desktopPolicy?.captureAllowed===false||item.incognito||!acceptable(address)||
    UdmMedia.policy.blocked(address,UdmMedia.policy.merge(settings,desktopPolicy||{})))return false;
  return !sharedCredentials||await canShareBrowserCredentials(settings);
}
async function prepareBrowserHandoff(item,capturedContext,readDesktopPreferences=()=>nativeRequest({action:'preferences'}),batch){
  if(!batch&&await browserControls?.isDisabled(item.tabId))throw Error('UDM is disabled on this tab.');
  if (!acceptable(item.url)) throw new Error('Only HTTP and HTTPS browser downloads can be handed off.');
  // Share only this handoff's capability lookup; the next download gets fresh data.
  let capabilities;const desktopPreferences=()=>capabilities||(capabilities=readDesktopPreferences());
  const canCredentials=batch?batch.canCredentials:await canShareBrowserCredentials();
  const observed=capturedContext===undefined?requestContext?.resolve(item,canCredentials):capturedContext;
  if((item.browserDownload||item.siteRule)&&observed?.sessionBound&&(!canCredentials||!observed.requiresCredentials))
    throw Error('The download uses browser credentials that UDM cannot share. This download remains in the browser.');
  if(batch&&observed&&(observed.method!=='GET'||observed.request))return null;
  let browserProxy=observed?.proxy;
  // An explicit link command may precede any browser network request. Bind its
  // current configured route without inventing prior request headers or cookies.
  if(!observed&&!item.browserDownload&&chromiumProxy){
    const address=item.finalUrl||item.url;
    if(batch)browserProxy=await batch.proxy.resolve(address);
    else{
      const tab=Number.isInteger(item.tabId)&&item.tabId>=0&&api.tabs?.get?await api.tabs.get(item.tabId):null;
      const observation=chromiumProxy.observe(address,tab?.incognito===true);await observation.ready;
      browserProxy=chromiumProxy.resolve(observation);
    }
  }
  if(observed&&observed.method!=='GET'&&!observed.request)throw Error('The original '+observed.method+' download request could not be captured safely. This download remains in the browser.');
  if(browserProxy?.unsupported)throw Error('UDM cannot reproduce this browser proxy route. Download this file in the browser.');
  if(browserProxy){const desktop=await desktopPreferences();if(!desktop?.ok||desktop.browserProxy!==1||(browserProxy.type!=='direct'&&desktop.explicitProxyTransport!==1)||(browserProxy.type==='pac'&&(browserProxy.pacScript!==undefined?desktop.browserPacInline!==1:desktop.browserPacUrl!==1))||((browserProxy.type==='https'||browserProxy.proxyDNS===false)&&desktop.browserProxyTypes!==2))throw Error('Update the UDM desktop app before handing off this browser proxy. This download remains in the browser.');}
  const importProperties=item.importProperties&&Object.keys(item.importProperties).length?item.importProperties:null;
  if(importProperties){const desktop=await desktopPreferences();if(!desktop?.ok||desktop.browserImportProperties!==1||(Object.hasOwn(importProperties,'referer')&&desktop.browserImportReferer!==1))throw Error('Update UDM before importing custom folders, descriptions or request details. No download was sent.');}
  let stagedRequest=null;
  if(observed?.request){
    const desktop=await desktopPreferences(),bytes=observed.request.bytes,encoded=observed.request.body||'';
    const byteLength=bytes?bytes.length:encoded.length/4*3-(encoded.endsWith('==')?2:encoded.endsWith('=')?1:0);
    const limit=Number.isSafeInteger(desktop?.postBodyLimit)?Math.max(0,desktop.postBodyLimit):65536;
    const stagedLimit=Number.isSafeInteger(desktop?.stagedPostBodyLimit)?Math.max(0,desktop.stagedPostBodyLimit):0;
    if(!desktop?.ok||!desktop.postDownloads||byteLength>(bytes?stagedLimit:limit))throw Error('Update the UDM desktop app to capture this form download. This download remains in the browser.');
    // Above the inline limit the bytes go to the desktop in bounded chunks; the desktop verifies size and SHA-256 and returns a reference.
    if(bytes)stagedRequest=await stagePostBody(bytes,observed.request.contentType);
  }
  if(!batch&&observed&&await browserControls?.isDisabled(observed.tabId))throw Error('UDM is disabled on this tab.');
  const headers={...(observed?.headers||{})};
  // A pre-pause snapshot is internal only; current consent still controls credentials.
  if(!canCredentials){delete headers.Cookie;delete headers.Authorization;}
  // Explicit import credentials are user input, independent of browser-cookie sharing.
  if(item.authorization){
    const login=item.authorization;
    if(typeof login.username!=='string'||!login.username||login.username.length>256||/[:\x00-\x1f\x7f]/.test(login.username)||typeof login.password!=='string'||login.password.length>1024||/[\x00-\x1f\x7f]/.test(login.password))throw Error('Invalid import authorization.');
    const bytes=new TextEncoder().encode(login.username+':'+login.password);
    headers.Authorization='Basic '+btoa(String.fromCharCode(...bytes));
  }
  let cookies = headers.Cookie||'';
  const capturedSession=canCredentials&&!observed?.ambiguousCredentials?await (batch?.session||downloadSessions)?.capture(item,observed):null;
  if(capturedSession)cookies=capturedSession.cookies;
  const sessionPolicy=capturedSession?await desktopPreferences():null;
  if(capturedSession&&!batch?.session){
    await capturedSession.verify();
    if(!batch&&!await canShareBrowserCredentials())throw Error('Cookie sharing was disabled. Capture this download again.');
  }
  const requestUrl=new URL(item.url);requestUrl.hash='';
  let sourcePage='';
  if(item.browserDownload&&observed?.method==='GET'&&!observed.pending&&observed.url===requestUrl.href){
    const page=value=>{
      if(typeof value!=='string'||!value||value.length>16000)return null;
      try{
        const parsed=new URL(value);
        if(!['http:','https:'].includes(parsed.protocol)||parsed.username||parsed.password)return null;
        parsed.hash='';return parsed;
      }catch{return null;}
    };
    const reported=page(item.referrer),documentPage=page(observed.sourcePage);
    const precise=reported&&(reported.pathname!=='/'||reported.search);
    const matchingDocument=documentPage&&(!reported||documentPage.origin===reported.origin)?documentPage:null;
    sourcePage=(precise?reported:matchingDocument)?.href||'';
  }
  const matchedResponse=observed?.method==='GET'&&!observed.pending&&observed.url===requestUrl.href&&observed.response&&!observed.response.ambiguous&&!observed.ambiguousCredentials&&
    (!observed.requiresCredentials||canCredentials)&&!item.authorization&&!(importProperties&&Object.hasOwn(importProperties,'referer'));
  const reportedSize=matchedResponse?observed.reportedSize:0;
  const reportedName=matchedResponse?observed.response.filename:'';
  const message = {
    action: 'add', url: item.url,
    ...(sourcePage?{sourcePage}:{}),
    ...(Number.isSafeInteger(reportedSize)&&reportedSize>0?{reportedDirectSize:reportedSize}:{}),
    ...(typeof reportedName==='string'&&reportedName&&reportedName.length<=255?{reportedServerName:reportedName}:{}),
    filename: item.filename || '', downloadLater:!!item.downloadLater,
    ...(importProperties?{importProperties}:{}), headers, request:stagedRequest||observed?.request||{},
    ...(capturedSession&&sessionPolicy?.browserSession===1?{browserSession:capturedSession.session}:{}), ...(browserProxy?{browserProxy}:{}), referrer: headers.Referer || item.referrer || '', cookies, userAgent: headers['User-Agent'] || navigator.userAgent
  };
  const responseType=matchedResponse?UdmFileRecognition.mime(observed.response.mime):'';
  // A fresh cookie snapshot can change the account behind the same URL.
  const responsePreview=responseType&&responseType.split('/').every(part=>part.length<=64)&&message.reportedDirectSize&&
    (!reportedName||message.reportedServerName)&&cookies===(observed?.headers?.Cookie||'')
    ?{size:message.reportedDirectSize,contentType:responseType,serverName:message.reportedServerName||''}:undefined;
  return {message,canCredentials,responsePreview};
}
async function handoff(item,capturedContext,ownership,guardSend){
  return sendBrowserHandoff(await prepareBrowserHandoff(item,capturedContext),ownership,guardSend);
}
async function sendBrowserHandoff({message,canCredentials},ownership,guardSend){
  const send=ownership?message=>captureRecovery.submit(ownership,message):nativeRequest;
  const result=await (guardSend?guardSend((beforeSend,validateSelection)=>send(message,beforeSend,validateSelection),canCredentials):send(message));
  if (!result?.ok) throw new Error(result?.error || 'UDM did not accept this download.');
  await report('');
  return result;
}
async function reviewBrowserBatch(rows,fromMenu,validatePlayer,prepareMedia){
  const {tabId}=rows[0];
  const hasMedia=rows.some(row=>row.mediaKey);
  const hasYouTube=rows.some(row=>row.youtube);
  const [desktop,sourceTab,canCredentials]=await Promise.all([
    nativeRequest({action:'preferences'}).catch(()=>null),api.tabs.get(tabId),canShareBrowserCredentials().catch(()=>null)
  ]);
  if(sourceTab.incognito)throw Error('Open a regular web page to review selected links.');
  if(await browserControls.isDisabled(tabId))throw Error('UDM is disabled on this tab.');
  if(!desktop?.ok||![1,2].includes(desktop.browserBatchReview)||(fromMenu&&desktop.browserBatchReview!==2)||canCredentials===null||
    (hasYouTube?desktop.browserYouTubeBatchReview!==1:hasMedia&&desktop.browserAdaptiveBatchReview!==1))return null;
  // All rows belong to this tab. Per-row request and cookie scope still come
  // from the captured frame, and the batch is rechecked immediately before send.
  const batch={canCredentials,session:canCredentials?downloadSessions?.beginBatch(sourceTab):null,proxy:chromiumProxy?.beginBatch()};
  const candidates=new Map();
  for(const row of rows){
    const url=new URL(row.url);if(!row.mediaKey)url.hash='';
    const key=row.mediaKey?'media:'+row.mediaKey:url.href;
    if(!candidates.has(key))candidates.set(key,{...row,url:url.href,referrer:row.page});
    else candidates.get(key).imageOnPage||=row.imageOnPage;
  }
  const items=[...candidates.values()];
  const links=new Array(items.length);
  let next=0,unsupported=false;
  try{
    await Promise.all(Array.from({length:Math.min(4,items.length)},async()=>{
      while(!unsupported&&next<items.length){
        const index=next++,item=items[index];
        if(item.mediaKey){
          const media=await prepareMedia(item,desktop);
          links[index]={url:item.url,page:item.page,linkText:item.linkText,suggestedName:item.suggestedName,
            ...(item.youtube?{media}:{adaptive:media})};
          continue;
        }
        const prepared=await prepareBrowserHandoff(item,undefined,()=>desktop,batch);
        if(!prepared){unsupported=true;return;}
        const {message,responsePreview}=prepared;
        const {url,headers,cookies,referrer,userAgent,browserProxy,browserSession}=message;
        links[index]={url,headers,cookies,referrer,userAgent,browserProxy,browserSession,page:item.page,linkText:item.linkText,
          ...(item.imageOnPage?{imageOnPage:true}:{}),...(responsePreview?{responsePreview}:{}),
          ...(item.suggestedName?{suggestedName:item.suggestedName}:{})};
      }
    }));
  }catch(error){
    if(hasMedia||error?.batchSourceChanged||error?.batchProxyChanged)throw error;
    batch.proxy?.verify();
    return null;
  }
  batch.proxy?.verify();
  const message={action:'browser-batch-review',links};
  const payload=hasMedia?message:links;
  const limit=hasMedia?Math.min(8*1024*1024,Number(desktop.mediaRequestLimit)||8*1024*1024):512*1024;
  if(unsupported||new TextEncoder().encode(JSON.stringify(payload)).length>limit)return null;
  async function validateSelection(){
    const tab=await browserControls.verifySource(sourceTab,rows);
    await batch.session?.verify(tab);
    if(canCredentials&&!await canShareBrowserCredentials())
      throw Error('Cookie sharing changed. Select the links again.');
    await validatePlayer?.();
    batch.proxy?.verify();
  }
  let dispatched=false,reply;
  try{
    reply=await nativeRequest(message,()=>{dispatched=true;},validateSelection);
  }catch(error){
    if(!dispatched)throw error;
  }
  if(reply?.ok&&reply.count===links.length)return {ok:true,count:reply.count};
  if(reply?.ok===false&&reply.error)throw Error(reply.error);
  throw Error('UDM did not confirm the review request. Check for the desktop batch dialog before trying again.');
}
let menuPending=Promise.resolve();
async function menuPolicy(){const saved=await api.storage.local.get(['settings','desktopPolicy']);return UdmMedia.policy.merge(saved.settings||{},saved.desktopPolicy||{}).contextMenu||{};}
function reconcileMenus(){const work=menuPending.catch(()=>{}).then(async()=>{const menu=await menuPolicy();await api.contextMenus.removeAll();
 if(menu.Link!==false)api.contextMenus.create({id:'udm-link',title:'Download with UDM',contexts:['link','image','video','audio'],documentUrlPatterns:['http://*/*','https://*/*'],targetUrlPatterns:['http://*/*','https://*/*']});await browserControls?.menus(menu);
 });menuPending=work;return work;}
api.runtime.onInstalled.addListener(()=>reconcileMenus().catch(e=>report(e.message)));
api.runtime.onStartup?.addListener(()=>reconcileMenus().catch(()=>{}));
api.storage.onChanged?.addListener((changes,area)=>{if(area==='local'&&changes.desktopPolicy)compiledSiteRules=null;if(area==='local'&&(changes.settings||changes.desktopPolicy))void reconcileMenus().catch(()=>{});});
api.contextMenus.onClicked.addListener((info,tab)=>{(async()=>{const menu=await menuPolicy();
 if((['udm-link','udm-selected-links'].includes(info.menuItemId)&&menu.Link===false)||(info.menuItemId==='udm-all-links'&&menu.All===false))return;
 if(browserControls?.handle(info,tab))return;
 if(info.menuItemId==='udm-link')await handoff({url:acceptable(info.linkUrl)?info.linkUrl:info.srcUrl,referrer:info.frameUrl||tab?.url,tabId:tab?.id,frameId:info.frameId??0});
 })().catch(e=>report(e.message));});
function browserResponseReceived(item){
  // Edge can report zero for a body whose length is still unknown.
  return !!item&&Number.isSafeInteger(item.totalBytes)&&item.totalBytes>0&&
    Number.isSafeInteger(item.bytesReceived)&&item.bytesReceived>=item.totalBytes;
}
let latestAutomaticDownload=0;
api.downloads.onCreated.addListener(async item => {
  if(item.incognito||!acceptable(item.finalUrl||item.url)||(item.state&&item.state!=='in_progress'))return;
  const attempt=++latestAutomaticDownload;
  let outcome;
  let paused = false,ownership;
  try {
    let intent=await takeIntent(item);if(intent==='bypass'){outcome='bypass';return;}
    const settings = { ...defaults, ...(await api.storage.local.get('settings')).settings };
    if(!settings.capture){outcome='disabled';return;}
    item=await UdmFileRecognition.downloadItem(api,item);if(!item)return;
    const url = item.finalUrl || item.url;
    if (!settings.capture || item.incognito || !acceptable(url) || (item.state && item.state!=='in_progress')) return;
    if(UdmMedia.policy.blocked(url,settings)){outcome='excluded';return;}
    // Firefox aborts webRequest when paused, removing its ephemeral request record.
    // Bind the request before pausing; unsupported forms must keep running in-browser.
    const captureItem={url,filename:UdmFileRecognition.basename(item.filename),referrer:item.referrer,browserDownload:true};
    const capturedContext=requestContext?.resolveDownload?await requestContext.resolveDownload(captureItem,true):requestContext?.resolve(captureItem,true)??null;
    if(requestContext&&!capturedContext){outcome='request';return;}
    // A new native GET cannot continue the browser's partial representation.
    if(capturedContext?.partialDownload){outcome='partial';return;}
    if(!intent&&capturedContext?.redirectSources)intent=await takeIntent(item,capturedContext);
    if(!intent)intent=await keyCapture?.intent(capturedContext,true)||'';if(intent==='bypass'){outcome='bypass';return;}
    let recognition=UdmFileRecognition.classify({...item,url},capturedContext);
    captureItem.filename=recognition.filename;
    // An authenticated response should stay open when the browser cannot
    // share the credentials for its native replacement.
    if(capturedContext?.sessionBound&&(!capturedContext.requiresCredentials||!await canShareBrowserCredentials())){outcome='credentials';return;}
    if(capturedContext?.proxy?.unsupported)throw Error('UDM cannot reproduce this browser proxy route. This download remains in the browser.');
    if(capturedContext&&capturedContext.method!=='GET'&&!capturedContext.request)
      throw Error('The original '+capturedContext.method+' download request could not be captured safely. This download remains in the browser.');
    const method=capturedContext?.method,route={proxy:capturedContext?.proxy?.type};
    // Cached rules can justify waking UDM for a download outside the local list.
    // They cannot approve or deny the capture; an already-running app may have newer rules.
    let canWake=intent==='force'||UdmFileRecognition.allowed(recognition,settings.extensions);
    if(!canWake){
      const cached=evaluateSiteRule(await compiledSiteRuleSet(),url,method,settings,route);
      canWake=cached?.action==='capture';
    }
    const desktop=await nativeRequest({action:canWake?'preferences':'preferences-if-running'}).catch(()=>null);
    if(!desktop?.ok){outcome=canWake?'desktop-unavailable':'type-unconfirmed';return;}
    if(desktop.captureAllowed===false){outcome='desktop-disabled';return;}
    const compiled=typeof UdmCaptureRules!=='undefined'&&desktop.captureRules?.length
      ?UdmCaptureRules.compile(desktop.captureRules):null;
    const siteRule=evaluateSiteRule(compiled,url,method,settings,route);
    if(siteRule)noteRuleDecision('download',url,'',siteRule);
    if(siteRule?.action==='ignore'){outcome='rule';return;}
    if(siteRule?.action==='capture'&&intent!=='force'){
      if(siteRule.extension)recognition={...recognition,extensions:[siteRule.extension],source:'site-rule'};
      else intent='rule';
    }
    if(intent==='force'&&desktop.forceSkipWeb!==false&&(recognition.source==='conflicting-web-response'||UdmMedia.policy.webResource({...item,mime:recognition.mime||item.mime}))){outcome='web-resource';return;}
    if(UdmMedia.policy.blocked(url,UdmMedia.policy.merge(settings,desktop))){outcome='excluded';return;}
    if(intent!=='force'&&intent!=='rule'&&!UdmFileRecognition.allowed(recognition,settings.extensions,...(Array.isArray(desktop.extensions)?[desktop.extensions]:[]))){outcome='type';return;}
    const browserRunning=!!captureRecovery&&desktop.captureTransaction===1;
    if(!browserRunning&&api.runtime.getURL?.('')?.startsWith('moz-extension://'))
      throw Error('Update UDM to capture this Firefox download. This download remains in the browser.');
    // The same desktop snapshot covers policy, proxy, session and form
    // capabilities. Finish these checks before touching the browser response.
    const prepared=await prepareBrowserHandoff(captureItem,capturedContext,()=>desktop);
    if(!browserRunning){
      // Policy and request preparation may outlive a user action in Downloads.
      // Recheck before pausing or creating ownership for an obsolete transfer.
      const [live]=await api.downloads.search({id:item.id});
      if(!live||live.state!=='in_progress'||live.paused||browserResponseReceived(live))return;
      if(captureRecovery)ownership=await captureRecovery.begin(item);
      await api.downloads.pause(item.id);paused=true;
    }
    const [current]=await api.downloads.search({id:item.id});
    if(!current||(browserRunning?(current.state!=='in_progress'||current.paused||browserResponseReceived(current)):!current.paused)){paused=false;return;}
    if(!browserRunning&&browserResponseReceived(current))return;
    if(!['in_progress','interrupted'].includes(current.state))return;
    if(browserRunning)ownership=await captureRecovery.begin(item,{browserRunning:true});
    if(ownership)ownership.protocol=desktop.captureTransaction===1?2:desktop.captureRecovery===1?1:0;
    if(prepared.canCredentials&&!await canShareBrowserCredentials())return;
    await sendBrowserHandoff(prepared,ownership);
    outcome='';
    if(ownership){paused=false;return;}
    // Cancel only after UDM acknowledges that it has saved the job.
    paused=false; // Desktop already owns the job, even if browser cancellation fails.
    try{await api.downloads.cancel(item.id);}catch{throw Error('UDM accepted this download, but the browser could not cancel its transfer. Check UDM and the browser download list before resuming or retrying.');}
  } catch(e) {
    outcome='';
    await report(e.message);
  } finally {
    if(ownership)await captureRecovery.finish(ownership);
    else if(paused){try{await api.downloads.resume(item.id);}catch{}}
    // Store only a fixed reason. Private items, addresses and filenames never enter this notice.
    if(attempt===latestAutomaticDownload&&outcome!==undefined){
      try{await api.storage.session?.set({automaticDownloadNotice:outcome});}catch{}
    }
  }
});
api.webRequest.onHeadersReceived.addListener(async event => {
  if(event.method==='HEAD')return;
  const stamp=captureNavigation?.token(event.tabId,event.frameId??0);
  if(typeof UdmSites!=='undefined')await UdmSites.observe(event).catch(()=>{});
  if (event.tabId < 0 || !acceptable(event.url) || ![200,206].includes(event.statusCode)) return;
  const contentType = event.responseHeaders?.find(h=>h.name.toLowerCase()==='content-type')?.value || '';
  let mediaType=contentType;
  if (!/^(audio|video)\//i.test(contentType) || /mpegurl/i.test(contentType)) {
    // Site rules: V/A/C mark this reply as media for the panel; S captures a background request.
    const decision=await backgroundRuleDecision(event).catch(()=>null);
    if(!decision)return;
    if(decision.action==='capture'){
      if(!requestContext?.rangeBound(event))void captureBackgroundRequest(event).catch(()=>{});
      return;
    }
    if(decision.action!=='stream'||!decision.extension||/mpegurl/i.test(contentType))return;
    mediaType=ruleMediaType(decision.extension);
  }
  const key = 'media:'+event.tabId;
  const pending=(mediaQueues.get(event.tabId)||Promise.resolve()).catch(()=>{}).then(async()=>{
    await captureNavigation?.ready;
    const tab=await api.tabs.get(event.tabId);if(tab.incognito||captureNavigation&&!captureNavigation.valid(event,stamp))return;
    const stream=streamInfo(event.url,mediaType);
    // Adaptive YouTube responses are grouped by format, never handed off as a partial playback fragment.
    if(new URL(event.url).hostname.endsWith('.googlevideo.com')&&!stream)return;
    const existing=(await api.storage.session.get(key))[key]||[];
    const item={...(stream||{url:event.url,type:mediaType}),frameId:event.frameId??0,documentId:event.documentId||'',observedAt:Date.now()};
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
  if(message?.action==='capture-multipart-form'){
    (async()=>{
      const data=await api.storage.local.get(['settings','desktopPolicy']),prefs=UdmMedia.policy.merge({...defaults,...data.settings},data.desktopPolicy||{});
      if(!prefs.capture||prefs.captureAllowed===false||sender.tab?.incognito||await browserControls?.isDisabled(sender.tab?.id)||
        UdmMedia.policy.blocked(sender.url,prefs)||UdmMedia.policy.blocked(message.url,prefs))return {ok:false};
      return {ok:!!requestContext?.captureForm(message,sender)};
    })().then(respond,()=>respond({ok:false}));return true;
  }

  if(message?.action==='grabber-rendered-projects'||message?.action==='grabber-rendered-capture'){if(!renderedGrabber){respond({ok:false,error:'Rendered capture is unavailable.'});return false;}const operation=message.action==='grabber-rendered-projects'?renderedGrabber.projects:renderedGrabber.complete;operation(message,sender).then(respond,e=>respond({ok:false,error:e.message}));return true;}
  if(message?.action==='grabber-login-pending'||message?.action==='grabber-login-complete'){if(!grabberSessions){respond({ok:false,error:'Browser sign-in is unavailable.'});return false;}const operation=message.action==='grabber-login-pending'?grabberSessions.pending:grabberSessions.complete;operation(message,sender).then(respond,e=>respond({ok:false,error:e.message}));return true;}
  if (sender.id !== api.runtime.id) return false;
  if(message.action==='media-receipt-ack'){
    if(!mediaHandoffs){respond({ok:false,error:'Media recovery is unavailable.'});return false;}
    mediaHandoffs.ack(message.token).then(respond,error=>respond({ok:false,error:error.message}));return true;
  }
  if(['media-receipt-list','media-receipt-check','media-receipt-dismiss','media-receipt-show'].includes(message.action)){
    if(!mediaHandoffs||sender.url!==api.runtime.getURL('media-recovery.html')){respond({ok:false,error:'Open media recovery from the UDM extension.'});return false;}
    const operation=message.action==='media-receipt-list'?()=>mediaHandoffs.list():message.action==='media-receipt-check'?()=>mediaHandoffs.check(message.token):message.action==='media-receipt-show'?()=>nativeRequest({action:'show'}):()=>mediaHandoffs.ack(message.token,message.confirmed===true);
    Promise.resolve().then(operation).then(respond,error=>respond({ok:false,error:error.message}));return true;
  }
  if(['capture-keys','capture-player'].includes(message.action)){if(!keyCapture)return false;const operation=message.action==='capture-keys'?keyCapture.update:keyCapture.player;operation(message,sender).then(respond,e=>respond({ok:false,error:e.message}));return true;}
  if(message.action==='integration-state'){(async()=>{void syncDesktopPolicy().catch(()=>{});return {ok:true,disabled:!!(await browserControls?.isDisabled(sender.tab?.id))};})().then(respond,()=>respond({ok:false}));return true;}
  if(message.action==='selected-links-review'){if(!browserControls){respond({ok:false,error:'Reload UDM to open link review.'});return false;}browserControls.review(message,sender).then(respond,error=>respond({ok:false,error:error.message}));return true;}
  if(message.action==='batch-list'||message.action==='batch-download'||message.action==='batch-metadata'||message.action==='batch-progress'){browserControls?.message(message,sender).then(respond,error=>respond({ok:false,error:error.message}));return true;}
  if(message.action==='capture-intent'){
    (async()=>{if(!sender.tab||sender.tab.incognito||!acceptable(sender.url)||(typeof message.url!=='string'||message.url.length>16000||!acceptable(message.url))||!['force','bypass'].includes(message.intent))throw Error('Invalid capture gesture.');
      const target=new URL(message.url);target.hash='';if(target.username||target.password)throw Error('Invalid capture URL.');
      const key=sender.tab.id+':'+(sender.frameId??0),entry={url:target.href,page:sender.url,documentId:sender.documentId||'',intent:message.intent,time:Date.now(),ready:null};
      if(captureIntents.size>=128)captureIntents.delete(captureIntents.keys().next().value);
      // Record pending validation synchronously: a download event may arrive
      // before tabs.get replies. It must await validation, never bypass it.
      entry.ready=api.tabs.get(sender.tab.id).then(tab=>{if(tab.incognito||((sender.frameId??0)===0&&tab.url!==sender.url))throw Error('The source page changed.');return true;});
      captureIntents.set(key,entry);
      try{await entry.ready;return {ok:true};}catch(e){if(captureIntents.get(key)===entry)captureIntents.delete(key);throw e;}
    })().then(respond,e=>respond({ok:false,error:e.message}));return true;
  }
  if(message.action==='restore-panels'&&isBrowserToolsSender(sender)){
    (async()=>{const tab=await api.tabs.get(message.tabId);if(tab.incognito||!acceptable(tab.url))throw Error('Open a regular video page.');
      await UdmSites.sync(true,tab.id);await api.tabs.sendMessage(tab.id,{action:'restore-panels'});return {ok:true};
    })().then(respond,e=>respond({ok:false,error:e.message}));return true;
  }
  if(['capture-legacy-list','capture-legacy-resolve','capture-legacy-show'].includes(message.action)){
    if(sender.id!==api.runtime.id||sender.url!==api.runtime.getURL('recovery.html')){respond({ok:false,error:'Open interrupted-download review from UDM’s extension.'});return false;}
    const operation=message.action==='capture-legacy-list'?()=>captureRecovery.reviews():message.action==='capture-legacy-show'?()=>captureRecovery.openNative():()=>captureRecovery.resolve(message.token,message.decision,message.confirmed);
    Promise.resolve().then(operation).then(respond,error=>respond({ok:false,error:error.message}));return true;
  }
  if(message.action==='capture-recover'&&isBrowserToolsSender(sender)){if(!captureRecovery){respond({ok:false,error:'Download recovery is unavailable.'});return false;}(async()=>{
    const result=await captureRecovery.recover(),legacy=await captureRecovery.reviews();result.legacyReviews=legacy.items.length;
    const media=mediaHandoffs?await mediaHandoffs.list():{items:[]};result.mediaReviews=media.items.length;if(result.mediaReviews)await api.tabs.create({url:api.runtime.getURL('media-recovery.html')});
    if(result.legacyReviews)await api.tabs.create({url:api.runtime.getURL('recovery.html')});
    try{const hello=await nativeRequest({action:'hello'});if(hello?.capabilities?.includes('capture-review')){const review=await nativeRequest({action:'capture-review-open'});if(!review?.ok)throw Error(review?.error||'UDM could not open captured download review.');result.reviews=review.reviews||0;}}catch(error){result.desktopError=error.message;}
    return result;
  })().then(respond,e=>respond({ok:false,error:e.message}));return true;}
  if(message.action==='capture-diagnostics'){respond({ok:true,connection:nativeClient?.diagnostics(),requests:requestContext?.diagnostics(),player:playerDiagnostics(message.tabId),counts:typeof UdmStreamingCapture!=='undefined'?UdmStreamingCapture.diagnostics(message.tabId):{},rules:captureRuleDiagnostics.slice()});return false;}
  if(message.action==='site-formats-cancel'){
    respond({ok:UdmSites.cancelFormats(message,sender)});
    return false;
  }
  if(message.action==='site-formats-priority'){
    respond({ok:UdmSites.prioritizeFormats(message,sender)});
    return false;
  }
  if(message.action==='site-other-qualities'){
    (async()=>{
      const deadline=Date.now()+10000;
      await UdmSites.validateQualityHelp(message,sender);
      const desktop=await mediaStep(()=>nativeRequest({action:'preferences'}),'The desktop did not reply.',6500).catch(()=>null);
      if(!desktop?.ok||desktop.browserQualityHelp!==1)return {unsupported:true};
      let dispatched=false;
      try{
        const reply=await nativeRequest({action:'browser-quality-help'},()=>{dispatched=true;},async()=>{
          await UdmSites.validateQualityHelp(message,sender);
          if(Date.now()>=deadline)throw Error('The guidance request expired. Open Other qualities again.');
        });
        if(reply?.ok===false)return {unsupported:true};
        if(reply?.ok)return reply;
      }catch(error){if(!dispatched)throw error;}
      throw Error('UDM did not confirm the guidance request. Check for the desktop message box.');
    })().then(respond,error=>respond({ok:false,error:error.message}));return true;
  }
  if(message.action==='site-video-batch'){
    const review=message.youtubeChoices===undefined?UdmSites.reviewVideoBatch(message,sender,reviewBrowserBatch):reviewYouTubeVideoBatch(message,sender);
    review.then(respond,error=>respond({ok:false,error:error.message}));return true;
  }
  if(message.action==='site-formats'||message.action==='site-download'){
    let sequence=0;
    const progress=message.action==='site-formats'&&Number.isInteger(sender.tab?.id)&&typeof message.progressId==='string'&&/^[0-9a-f-]{36}$/i.test(message.progressId)?result=>{
      if(message.prepareOnly!==true)warmMediaConnection();
      void api.tabs.sendMessage(sender.tab.id,{action:'site-formats-progress',token:message.token,progressId:message.progressId,sequence:++sequence,result},{frameId:sender.frameId??0}).catch(()=>{});
    }:undefined;
    const pending=message.action==='site-formats'?UdmSites.list(message,sender,progress):UdmSites.download(message,sender);
    pending.then(result=>{
      if(message.action==='site-formats'&&message.prepareOnly!==true&&result?.ok&&result.choices?.length)warmMediaConnection();
      respond(result);
    },error=>respond({ok:false,error:error.message}));return true;
  }
  if(message.action==='sync-video-panels'&&isBrowserToolsSender(sender)){
    UdmSites.sync(true,message.tabId).then(result=>respond({ok:true,...result}),error=>respond({ok:false,error:error.message}));return true;
  }
  if(message.action==='video-panel-opened'){
    warmOpenedVideoPanel(message,sender).then(respond,()=>respond({ok:false}));
    return true;
  }
  if(message.action==='youtube-stream-ready'){
    const id=youtubeId(message.url);
    if(id&&sender.tab&&!sender.tab.incognito)
      wakeYouTubeStream(sender.tab.id,sender.frameId??0,sender.documentId,id);
    respond({ok:true});return;
  }
  if (message.action === 'formats') {
    mediaStep(()=>availableMedia(message,sender,true),mediaReadTimeout).then(context=>{
      const warmChoice=warmPlayerChoice(context);
      if(warmChoice&&sender.tab?.id===context.tabId&&!isBrowserToolsSender(sender))rememberPlayerWarmup(context,warmChoice);
      if(message.prepareOnly!==true){
        warmMediaConnection();
        if(warmChoice)void nativePlayerPair(context,warmChoice);
      }
      const singleFormat=context.choices.length===1&&context.choices[0].formatId&&
        (!context.streamRefresh||!context.streamRefresh.audio&&context.choices[0].formatId===context.streamRefresh.formatId)&&
        !context.audioChoices.length&&!context.captionChoices.length;
      respond({ok:true,videoId:context.id,title:UdmFormats.videoTitle(context.snapshot),streamRefreshAudio:context.streamRefresh?.audio===true,
        ...(warmChoice?{warmPlayer:true}:{}),...(singleFormat?{singleFormat:true}:{}),
        ...(!context.streamRefresh&&context.choices.length+context.captionChoices.length>1?{videoBatch:true}:{}),
        choices:context.choices.map(({key,height,label})=>({key,height,label})),
        captionChoices:context.captionChoices.map(({key,height,label})=>({key,height,label,output:'caption'})),
        audioChoices:context.audioChoices.map(({key,label,default:preferred})=>({key,label,default:preferred}))});
    },error=>respond({ok:false,error:error.message}));return true;
  }
  if(message.action==='youtube-audio'){audioHandoff(message,sender).then(respond,error=>{void report(error.message).catch(()=>{});respond({ok:false,error:error.message});});return true;}
  if(message.action==='youtube-caption'){captionHandoff(message,sender).then(respond,error=>respond({ok:false,error:error.message}));return true;}
  if (message.action === 'media') {
    mediaHandoff(message,sender).then(respond,error=>{void report(error.message).catch(()=>{});respond({ok:false,error:error.message});});return true;
  }
  if(message.action==='selected-links'){
    (async()=>{if(!sender.tab||sender.tab.incognito||!acceptable(sender.url)||message.page!==sender.url)throw Error('Open a regular web page to download selected links.');if(!Array.isArray(message.urls)||!message.urls.length||message.urls.length>100)throw Error('Select between 1 and 100 links.');const urls=[...new Set(message.urls)];for(const url of urls){if(typeof url!=='string'||url.length>16000||!acceptable(url))throw Error('Only HTTP and HTTPS links are supported.');const parsed=new URL(url);if(parsed.username||parsed.password)throw Error('Links containing embedded credentials are not supported.');}let count=0;try{for(const url of urls){await handoff({url,referrer:sender.url,tabId:sender.tab.id,frameId:sender.frameId??0,documentId:sender.documentId});count++;}return {ok:true,count};}catch(error){return {ok:false,count,error:count+' links were added. '+error.message+' Check UDM before retrying; the last request may also have been accepted.'};}})().then(respond,error=>respond({ok:false,count:0,error:error.message}));return true;
  }
  if (message.action === 'download') {
    handoff(message).then(result=>respond(result),error=>respond({ok:false,error:error.message})); return true;
  }
  if(message.action==='open-browser-settings'&&isBrowserToolsSender(sender)){nativeRequest({action:'browser-settings'}).then(respond,e=>respond({ok:false,error:e.message}));return true;}
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
