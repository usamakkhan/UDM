// UDM's original, page-local observer. It does not alter requests or response bodies.
(()=>{
  'use strict';
  if(globalThis.__udmCaptureV1)return;
  const catalog=new Map(),sessions=new Map(),captionRequests=new Map(),acceptedPlayers=new Map(),playerAuthority=new Map();
  const snapshotVisits=new WeakMap(),xhrPlayerListeners=new WeakMap();
  const playerReads=new Map(),maxPlayerReads=4,maxPlayerBytes=2097152;
  let playerReadPeak=0,playerReadCancels=0;
  function clearPlayerReadTimer(entry){if(entry.timer!==undefined){try{globalThis.clearTimeout?.(entry.timer);}catch{}entry.timer=undefined;}}
  function cancelPlayerRead(reader){
    const entry=playerReads.get(reader);if(!entry)return;
    entry.cancelled=true;playerReads.delete(reader);++playerReadCancels;clearPlayerReadTimer(entry);
    // Cancel only our clone. Awaiting tee cancellation can wait for playback's
    // original response to end, so release the inspection independently.
    try{void Promise.resolve(reader.cancel()).catch(()=>{});}catch{}
  }
  function streamingReadPool(){
    const reads=new Map();let peak=0,cancels=0;
    function cancel(reader){
      const entry=reads.get(reader);if(!entry)return;
      entry.cancelled=true;reads.delete(reader);++cancels;clearPlayerReadTimer(entry);entry.resolveStop(null);
      try{void Promise.resolve(reader.cancel()).catch(()=>{});}catch{}
    }
    return {
      open(reader,context){
        while(reads.size>=4)cancel(reads.keys().next().value);
        let resolveStop;const stop=new Promise(resolve=>{resolveStop=resolve;});
        const entry={context,stop,resolveStop,cancelled:false,timer:undefined};reads.set(reader,entry);peak=Math.max(peak,reads.size);
        try{if(typeof globalThis.setTimeout==='function')entry.timer=globalThis.setTimeout(()=>cancel(reader),30000);}catch{cancel(reader);}
        return entry;
      },
      close(reader,entry,ended=false){
        if(!entry)return;
        if(!ended&&!entry.cancelled)cancel(reader);
        if(reads.get(reader)===entry)reads.delete(reader);
        clearPlayerReadTimer(entry);entry.resolveStop(null);try{reader.releaseLock();}catch{}
      },
      retire(predicate){for(const [reader,entry] of reads)if(predicate(entry.context))cancel(reader);},
      stats(){return {active:reads.size,peak,cancels};}
    };
  }
  const sabrReads=streamingReadPool(),requestReads=streamingReadPool();
  const streamTasks=new Map(),xhrStreamingHooks=new WeakMap();let streamTaskPeak=0;
  function retireStreamTasks(predicate){for(const task of streamTasks.values())if(predicate(task.context))retireStreamingContext(task.context);}
  function beginStreamTask(url,context,produce){
    while(streamTasks.size>=8)retireStreamingContext(streamTasks.values().next().value.context);
    let resolveStop;const stop=new Promise(resolve=>{resolveStop=resolve;});
    const task={url,context,stop,resolveStop,bytes:null,ready:null,active:true,timer:undefined,detach:null};streamTasks.set(context.sequence,task);streamTaskPeak=Math.max(streamTaskPeak,streamTasks.size);
    try{
      if(typeof globalThis.setTimeout==='function')task.timer=globalThis.setTimeout(()=>retireStreamingContext(context),30000);
      if(task.active)task.ready=Promise.resolve(produce()).then(bytes=>{
        if(!task.active||!sabrContextCurrent(context))return;
        if(!bytes?.byteLength||bytes.byteLength>maxSabrRequestBytes){retireStreamingContext(context);return;}
        task.bytes=bytes;
        // Resolve without the bytes: retiring the task must be able to drop
        // their only observer-owned reference even while fetch never settles.
      },()=>retireStreamingContext(context));
    }catch{retireStreamingContext(context);}
    return task;
  }
  let pageVideoId=null,pageRoute=null,routeGeneration=0,playerSequence=0,sabrSequence=0,acceptedSabrSequence=0;
  function snapshotRevisions(data){
    const revisions=new Map();
    for(const field of ['formats','adaptiveFormats'])for(const format of (Array.isArray(data?.streamingData?.[field])?data.streamingData[field]:[]).slice(0,200)){
      const value=String(format?.lastModified??'');
      if(!/^\d{1,20}$/.test(value))continue;
      const key=JSON.stringify([field,String(format.itag),String(format.audioTrack?.id||''),String(format.xtags||'')]);
      const revision=value.replace(/^0+(?=\d)/,'');
      if(revisions.has(key)&&revisions.get(key)!==revision)return null;
      revisions.set(key,revision);
    }
    return revisions;
  }
  function revisionsAdvance(previous,next){
    if(!previous?.size||!next)return false;
    let advanced=false;
    for(const [key,before] of previous){
      const after=next.get(key);if(after===undefined)return false;
      const order=after.length-before.length||(after>before?1:after<before?-1:0);
      if(order<0)return false;if(order>0)advanced=true;
    }
    return advanced;
  }
  function invalidateRoute(obsoleteId=pageVideoId){
    // Mark existing page objects before retiring the visit. Reading the same
    // old object after a round trip must not make its metadata fresh again.
    if(obsoleteId)for(const read of [()=>globalThis.ytInitialPlayerResponse,()=>document.getElementById('movie_player')?.getPlayerResponse?.()])try{
      const data=read();if(data&&typeof data==='object'&&data.videoDetails?.videoId===obsoleteId){
        const seen=snapshotVisits.get(data);
        if(!seen||seen.generation===routeGeneration||seen.id!==obsoleteId)snapshotVisits.set(data,{generation:routeGeneration,id:obsoleteId,revisions:snapshotRevisions(data)});
      }
    }catch{}
    routeGeneration++;
    retireStreamTasks(()=>true);
    for(const reader of playerReads.keys())cancelPlayerRead(reader);
    sabrReads.retire(()=>true);requestReads.retire(()=>true);acceptedSabrSequence=0;
    catalog.clear();acceptedPlayers.clear();playerAuthority.clear();
    sessions.clear();
    captionRequests.clear();
    document.documentElement?.removeAttribute?.('data-udm-content-ready');
  }
  document.addEventListener?.('yt-navigate-start',()=>invalidateRoute(),true);
  globalThis.addEventListener?.('popstate',()=>current(true),true);
  globalThis.addEventListener?.('hashchange',()=>current(true),true);
  if(globalThis.history)for(const name of ['pushState','replaceState']){
    const original=history[name];if(typeof original==='function')try{history[name]=function(...args){const result=Reflect.apply(original,this,args);current(true);return result;};}catch{}
  }
  let lastSabrObservation={requests:0,verified:0,error:''};
  function current(allowVerifiedContent=false){
    const page=new URL(location.href),id=page.searchParams.get('v')||/^\/(?:embed|shorts)\/([\w-]{11})/.exec(page.pathname)?.[1]||null;
    const route=page.pathname+'\n'+(id||'');
    if(route!==pageRoute){const obsolete=pageVideoId;pageVideoId=id;pageRoute=route;invalidateRoute(obsolete);}
    if(!/^[\w-]{11}$/.test(id||''))return null;
    if(allowVerifiedContent)return id;
    const player=document.getElementById('movie_player');
    return player?.getVideoData?.()?.video_id===id&&!player.classList?.contains('ad-showing')&&!player.classList?.contains('ad-interrupting')?id:null;
  }
  function sabrEndpoint(value){try{const u=new URL(value,location.href);return u.protocol==='https:'&&u.hostname.endsWith('.googlevideo.com')&&u.pathname==='/videoplayback'&&!(u.searchParams.getAll('advideo').some(value=>value!=='')||u.searchParams.getAll('el').includes('adunit'))&&(u.searchParams.has('sabr')||u.searchParams.get('ump')==='1')?u.href:null;}catch{return null;}}
  const maxSabrRequestBytes=131072;
  async function bodyBytes(body){
    if(body instanceof ArrayBuffer&&body.byteLength<=maxSabrRequestBytes)return new Uint8Array(body.slice(0));
    if(ArrayBuffer.isView(body)&&body.byteLength<=maxSabrRequestBytes)return new Uint8Array(body.buffer.slice(body.byteOffset,body.byteOffset+body.byteLength));
    if(typeof Blob!=='undefined'&&body instanceof Blob&&body.size<=maxSabrRequestBytes)return new Uint8Array(await Blob.prototype.arrayBuffer.call(body));
    return null;
  }
  function sabrContextCurrent(context){return !!context&&!context.retired&&current(true)===context.id&&context.generation===routeGeneration&&context.sequence>=acceptedSabrSequence;}
  function retireStreamingContext(context){
    if(!context)return;context.retired=true;
    const task=streamTasks.get(context.sequence);
    if(task?.context===context){
      streamTasks.delete(context.sequence);task.active=false;task.bytes=null;task.ready=null;task.url='';clearPlayerReadTimer(task);task.resolveStop(null);
      const detach=task.detach;task.detach=null;try{detach?.();}catch{}
    }
    sabrReads.retire(value=>value===context);requestReads.retire(value=>value===context);
  }
  async function requestBodyBytes(request,context){
    if(!sabrContextCurrent(context))return null;
    const reader=Request.prototype.clone.call(request).body?.getReader();
    if(!reader)return null;
    const entry=requestReads.open(reader,context);let ended=false;
    const bytes=new Uint8Array(maxSabrRequestBytes);
    let length=0;
    try{
      for(;;){
        const {done,value}=await reader.read();
        if(entry.cancelled||!sabrContextCurrent(context))return null;
        if(done){ended=true;return bytes.subarray(0,length);}
        if(value.byteLength>maxSabrRequestBytes-length)return null;
        bytes.set(value,length);
        length+=value.byteLength;
      }
    }finally{
      requestReads.close(reader,entry,ended);
    }
  }
  async function inspectSabr(response,task){
    const context=task.context;
    if(!context?.id||!globalThis.__udmUmpV1){retireStreamingContext(context);return;}
    const {id,capturedAt}=context,url=task.url;
    lastSabrObservation.requests++;
    let reader,entry;
    try{
      // Clone before yielding, since playback can consume the original as soon as fetch resolves.
      if(!task.active||!sabrContextCurrent(context)||!response.ok||!response.headers.get('content-type')?.includes('application/vnd.yt-ump'))return;
      reader=response.clone().body?.getReader();if(!reader)return;
      entry=sabrReads.open(reader,context);
      await Promise.race([task.ready,task.stop,entry.stop]);
      if(!task.active||entry.cancelled||!sabrContextCurrent(context))return;
      const bytes=task.bytes;
      if(!bytes?.length||!__udmUmpV1.request(bytes)){lastSabrObservation.error='No bounded streaming request body';return;}
      let prefix=new Uint8Array(0),verified=false;
      while(prefix.length<262144){const {done,value}=await reader.read();if(entry.cancelled||!sabrContextCurrent(context))return;if(done)break;const next=new Uint8Array(Math.min(262144,prefix.length+value.length));next.set(prefix);next.set(value.subarray(0,next.length-prefix.length),prefix.length);prefix=next;if(__udmUmpV1.identity(prefix,id)){verified=true;break;}}
      if(!verified||!sabrContextCurrent(context)){if(sabrContextCurrent(context))lastSabrObservation.error='Media identity not verified';return;}
      acceptedSabrSequence=context.sequence;
      retireStreamTasks(value=>value.sequence<context.sequence);
      sabrReads.retire(value=>value.sequence<context.sequence);requestReads.retire(value=>value.sequence<context.sequence);
      let raw='';for(const b of bytes)raw+=String.fromCharCode(b);sessions.set(id,{url,body:btoa(raw),videoId:id,capturedAt});document.documentElement?.setAttribute('data-udm-content-ready',id+':'+Date.now());if(sessions.size>3)sessions.delete(sessions.keys().next().value);lastSabrObservation.verified++;lastSabrObservation.error='';
    }catch(e){if(sabrContextCurrent(context))lastSabrObservation.error=String(e.message).slice(0,100);}
    finally{if(reader)sabrReads.close(reader,entry);retireStreamingContext(context);}
  }
  function lifecycle(data){
    const details=data?.videoDetails;
    return [details?.isLive===true,details?.isUpcoming===true,details?.isPostLiveDvr===true,data?.microformat?.playerMicroformatRenderer?.liveBroadcastDetails?.isLiveNow===true].join(',');
  }
  function remember(data,authority=null){
    const id=data?.videoDetails?.videoId;
    const captions=data?.captions?.playerCaptionsTracklistRenderer?.captionTracks;
    if(!/^[\w-]{11}$/.test(id||'')||(!data.streamingData&&!Array.isArray(captions)&&lifecycle(data)==='false,false,false,false'))return;
    if(authority&&lifecycle(data)!==lifecycle(authority))return;
    const streams=data.streamingData||{};
    function select(items){return (Array.isArray(items)?items:[]).slice(0,200).map(f=>({itag:f.itag,width:f.width,height:f.height,fps:f.fps,mimeType:f.mimeType,qualityLabel:f.qualityLabel,contentLength:f.contentLength,bitrate:f.bitrate,averageBitrate:f.averageBitrate,url:f.url,signatureCipher:f.signatureCipher,cipher:f.cipher,drmFamilies:f.drmFamilies,drmTrackType:f.drmTrackType,audioTrack:f.audioTrack,lastModified:f.lastModified,xtags:f.xtags}));}
    const response={videoDetails:{videoId:id,isLiveContent:data.videoDetails.isLiveContent===true,isLive:data.videoDetails.isLive===true,isUpcoming:data.videoDetails.isUpcoming===true,isPostLiveDvr:data.videoDetails.isPostLiveDvr===true,lengthSeconds:data.videoDetails.lengthSeconds},streamingData:{formats:select(streams.formats),adaptiveFormats:select(streams.adaptiveFormats)}};
    response.microformat={playerMicroformatRenderer:{liveBroadcastDetails:{isLiveNow:data.microformat?.playerMicroformatRenderer?.liveBroadcastDetails?.isLiveNow===true}}};
    const formatKey=f=>{let tags=String(f.xtags||'');if(!tags)try{tags=new URL(f.url||new URLSearchParams(f.signatureCipher||f.cipher||'').get('url')).searchParams.get('xtags')||'';}catch{}return JSON.stringify([String(f.itag),String(f.audioTrack?.id||''),tags]);};
    const prior=catalog.get(id);
    response.videoDetails.title=String(data.videoDetails.title||prior?.videoDetails.title||'').slice(0,500);
    if(Array.isArray(captions))response.captionTracks=captions.slice(0,80).map(track=>({
      baseUrl:typeof track?.baseUrl==='string'&&track.baseUrl.length<=12000?track.baseUrl:'',
      vssId:String(track?.vssId||'').slice(0,100),languageCode:String(track?.languageCode||'').slice(0,40),
      kind:track?.kind==='asr'?'asr':'',name:String(track?.name?.simpleText||
        (Array.isArray(track?.name?.runs)?track.name.runs.slice(0,12).map(run=>run?.text||'').join(''):'')).slice(0,120)
    }));
    else if(prior&&lifecycle(prior)===lifecycle(response))response.captionTracks=prior.captionTracks;
    if(prior&&lifecycle(prior)===lifecycle(response)){for(const field of ['formats','adaptiveFormats']){
      const merged=new Map(prior.streamingData[field].map(f=>[formatKey(f),f]));
      for(const next of response.streamingData[field]){
        let previous=merged.get(formatKey(next))||{};
        if(next.lastModified&&previous.lastModified&&String(next.lastModified)!==String(previous.lastModified))previous={};
        const present=Object.fromEntries(Object.entries(next).filter(([,value])=>value!==undefined));
        merged.set(formatKey(next),{...previous,...present});
      }
      response.streamingData[field]=[...merged.values()].slice(-200);
    }}
    if(authority){
      // The getter has no request/visit proof. It may supplement partial API
      // metadata, but must not roll back fields owned by an accepted response.
      response.videoDetails={...response.videoDetails,...authority.videoDetails};
      response.microformat=authority.microformat;
      if(authority.captionTracks)response.captionTracks=authority.captionTracks;
      for(const field of ['formats','adaptiveFormats']){
        const owned=new Map(authority.streamingData[field].map(f=>[formatKey(f),f]));
        const supplemented=response.streamingData[field].map(f=>{
          const known=owned.get(formatKey(f));if(!known)return f;
          if(f.lastModified&&known.lastModified&&String(f.lastModified)!==String(known.lastModified))return {...known};
          const present=Object.fromEntries(Object.entries(known).filter(([,value])=>value!==undefined));
          const merged={...f,...present};
          if(known.url||known.signatureCipher||known.cipher)for(const key of ['url','signatureCipher','cipher']){
            if(known[key]===undefined)delete merged[key];
          }
          return merged;
        });
        // Getter additions cannot evict the accepted API formats at the cap.
        const retained=new Map(owned);
        for(const format of supplemented)if(retained.has(formatKey(format))||retained.size<200)retained.set(formatKey(format),format);
        response.streamingData[field]=[...retained.values()];
      }
    }
    catalog.set(id,response);if(catalog.size>8){const oldest=catalog.keys().next().value;catalog.delete(oldest);acceptedPlayers.delete(oldest);playerAuthority.delete(oldest);}return response;
  }
  function playerContext(){const id=current(true);return id?{id,generation:routeGeneration,sequence:++playerSequence}:null;}
  function playerContextCurrent(context){return !!context&&current(true)===context.id&&context.generation===routeGeneration&&context.sequence>=(acceptedPlayers.get(context.id)||0);}
  function rememberPlayer(data,context){
    if(!playerContextCurrent(context)||data?.videoDetails?.videoId!==context.id)return;
    const response=remember(data);
    if(response){
      playerAuthority.set(context.id,response);
      acceptedPlayers.set(context.id,context.sequence);
      for(const [reader,entry] of playerReads)if(entry.context.id===context.id&&entry.context.sequence<context.sequence)cancelPlayerRead(reader);
    }
  }
  function rememberSnapshot(data){
    if(!data||typeof data!=='object')return;
    const id=data.videoDetails?.videoId,seen=snapshotVisits.get(data);
    const revisions=snapshotRevisions(data);
    if(seen&&seen.id===id&&seen.generation!==routeGeneration&&
       (current()!==id||!revisionsAdvance(seen.revisions,revisions)))return;
    snapshotVisits.set(data,{generation:routeGeneration,id,revisions});remember(data,playerAuthority.get(id));
  }
  function playerEndpoint(value){try{const u=new URL(value,location.href);return /(^|\.)youtube\.com$/.test(u.hostname)&&u.pathname==='/youtubei/v1/player';}catch{return false;}}
  function captionEndpoint(value){
    if(typeof value!=='string'||value.length>12000||!value.includes('/api/timedtext'))return null;
    try{
      const url=new URL(value,location.href);
      return url.protocol==='https:'&&!url.username&&!url.password&&!url.port&&url.pathname==='/api/timedtext'&&
        ['www.youtube.com','m.youtube.com','www.youtube-nocookie.com'].includes(url.hostname)?url:null;
    }catch{return null;}
  }
  function rememberCaptionRequest(responseUrl,contentType,length,id,generation){
    if(!id||current(true)!==id||generation!==routeGeneration||length==='0'||
       !/^(?:application\/(?:json|xml|ttml\+xml)|text\/(?:xml|vtt))\s*(?:;|$)/i.test(contentType||''))return;
    const url=captionEndpoint(responseUrl);
    if(!url||url.searchParams.getAll('v').length!==1||url.searchParams.get('v')!==id||url.searchParams.has('tlang'))return;
    captionRequests.delete(url.href);
    captionRequests.set(url.href,Date.now());
    if(captionRequests.size>16)captionRequests.delete(captionRequests.keys().next().value);
  }
  async function inspect(response,context){
    let reader,entry,ended=false;
    try{
      if(!playerContextCurrent(context))return;
      if(Number(response.headers.get('content-length'))>maxPlayerBytes)return;
      const copy=response.clone();
      reader=copy.body?.getReader();if(!reader)return;
      while(playerReads.size>=maxPlayerReads)cancelPlayerRead(playerReads.keys().next().value);
      entry={context,cancelled:false,timer:undefined};playerReads.set(reader,entry);playerReadPeak=Math.max(playerReadPeak,playerReads.size);
      if(typeof globalThis.setTimeout==='function')entry.timer=globalThis.setTimeout(()=>cancelPlayerRead(reader),30000);
      const decoder=new TextDecoder();let text='',bytes=0;
      for(;;){
        const part=await reader.read();
        if(entry.cancelled||!playerContextCurrent(context)){cancelPlayerRead(reader);return;}
        if(part.done){ended=true;break;}
        bytes+=part.value.length;if(bytes>maxPlayerBytes){cancelPlayerRead(reader);return;}
        text+=decoder.decode(part.value,{stream:true});
      }
      text+=decoder.decode();if(!entry.cancelled)rememberPlayer(JSON.parse(text),context);
    }catch{}finally{
      if(reader){
        if(!ended&&!entry?.cancelled)cancelPlayerRead(reader);
        if(playerReads.get(reader)===entry)playerReads.delete(reader);
        if(entry)clearPlayerReadTimer(entry);
        try{reader.releaseLock();}catch{}
      }
    }
  }
  const originalFetch=globalThis.fetch;
  if(typeof originalFetch==='function')globalThis.fetch=function(input,...args){
    let sabr,playerRequest;
    try{
      if(playerEndpoint(typeof input==='string'||input instanceof URL?String(input):input?.url))playerRequest=playerContext();
      const url=sabrEndpoint(typeof input==='string'||input instanceof URL?String(input):input?.url);
      if(url&&String(args[0]?.method||input?.method||'GET').toUpperCase()==='POST'){
        const id=current(true),generation=routeGeneration,capturedAt=Date.now();
        if(id){
          const context={id,generation,capturedAt,sequence:++sabrSequence};
          // A Request body becomes unusable for cloning once native fetch takes it.
          sabr=beginStreamTask(url,context,()=>args[0]?.body!=null?bodyBytes(args[0].body):requestBodyBytes(input,context));
        }
      }
    }catch{}
    let promise;try{promise=Reflect.apply(originalFetch,this,[input,...args]);}catch(error){if(sabr)retireStreamingContext(sabr.context);throw error;}
    if(sabr)promise.then(response=>void inspectSabr(response,sabr),()=>retireStreamingContext(sabr.context));
    if(playerRequest)promise.then(response=>{if(response.ok)void inspect(response,playerRequest);},()=>{});
    try{
      const caption=captionEndpoint(typeof input==='string'||input instanceof URL?String(input):input?.url);
      if(caption&&String(args[0]?.method||input?.method||'GET').toUpperCase()==='GET'){
        const id=current(true),generation=routeGeneration;
        promise.then(response=>{
          if(response.status===200)rememberCaptionRequest(response.url,response.headers.get('content-type'),
            response.headers.get('content-length'),id,generation);
        },()=>{}).catch(()=>{});
      }
    }catch{}
    return promise;
  };
  if(globalThis.XMLHttpRequest){
    const originalOpen=XMLHttpRequest.prototype.open;
    XMLHttpRequest.prototype.open=function(method,url,...args){
      const result=Reflect.apply(originalOpen,this,[method,url,...args]);
      const streaming=xhrStreamingHooks.get(this);if(streaming)retireStreamingContext(streaming.task.context);
      const previous=xhrPlayerListeners.get(this);if(previous)this.removeEventListener?.('load',previous);xhrPlayerListeners.delete(this);
      this.__udmSabr=String(method).toUpperCase()==='POST'?sabrEndpoint(url):null;
      this.__udmCaption=String(method).toUpperCase()==='GET'&&!!captionEndpoint(url);
      this.__udmPlayer=playerEndpoint(url);
      return result;
    };
    const originalSend=XMLHttpRequest.prototype.send;
    XMLHttpRequest.prototype.send=function(body){
      // A second send on an already active XHR must retain the first request's
      // observer state and preserve the native InvalidStateError.
      if(xhrStreamingHooks.get(this)?.task.active)return Reflect.apply(originalSend,this,[body]);
      const url=this.__udmSabr,id=current(true),generation=routeGeneration;
      let playerListener,streamingTask;
      if(this.__udmPlayer){
        const context=playerContext(),previous=xhrPlayerListeners.get(this);if(previous)this.removeEventListener?.('load',previous);
        playerListener=()=>{try{if(this.status<200||this.status>=300||!playerContextCurrent(context))return;const data=this.responseType==='json'?this.response:this.responseType===''||this.responseType==='text'?this.responseText:null;if(typeof data==='string'){if(data.length<=2097152)rememberPlayer(JSON.parse(data),context);}else if(data)rememberPlayer(data,context);}catch{}finally{if(xhrPlayerListeners.get(this)===playerListener)xhrPlayerListeners.delete(this);}};
        xhrPlayerListeners.set(this,playerListener);this.addEventListener('load',playerListener,{once:true});
      }
      if(url&&id)try{
        const context={id,generation,capturedAt:Date.now(),sequence:++sabrSequence};streamingTask=beginStreamTask(url,context,()=>bodyBytes(body));
        const task=streamingTask,xhr=this,hooks={task,handlers:[]};
        const detach=()=>{if(xhrStreamingHooks.get(xhr)!==hooks)return;xhrStreamingHooks.delete(xhr);for(const [name,handler] of hooks.handlers)try{xhr.removeEventListener(name,handler,true);}catch{}task.detach=null;};
        const fail=()=>retireStreamingContext(context);
        const loaded=()=>{detach();try{if(!task.active)return;if(xhr.responseType==='arraybuffer'&&xhr.response){const prefix=new Uint8Array(xhr.response,0,Math.min(xhr.response.byteLength,262144));const response=new Response(prefix,{status:xhr.status,headers:{'content-type':xhr.getResponseHeader('content-type')||''}});void inspectSabr(response,task);}else fail();}catch{fail();}};
        if(task.active){task.detach=detach;hooks.handlers=[['load',loaded],['abort',fail],['error',fail],['timeout',fail],['loadend',fail]];xhrStreamingHooks.set(xhr,hooks);for(const [name,handler] of hooks.handlers)xhr.addEventListener(name,handler,true);}
      }catch{if(streamingTask)retireStreamingContext(streamingTask.context);}
      if(this.__udmCaption&&id)this.addEventListener('load',()=>{
        if(this.status===200)rememberCaptionRequest(this.responseURL,this.getResponseHeader('content-type'),
          this.getResponseHeader('content-length'),id,generation);
      },{once:true});
      try{return Reflect.apply(originalSend,this,[body]);}catch(error){if(streamingTask)retireStreamingContext(streamingTask.context);if(playerListener){this.removeEventListener?.('load',playerListener);xhrPlayerListeners.delete(this);}throw error;}
    };
  }
  const api=Object.freeze({
    session(id){if(current(true)!==id)return null;const value=sessions.get(id);return value&&value.capturedAt>Date.now()-300000?{...value}:null;},
    visit(id){return current(true)===id?routeGeneration:null;},
    captionRequests(id){
      if(current(true)!==id)return [];
      const cutoff=Date.now()-90000;
      return [...captionRequests].filter(([,time])=>time>=cutoff).map(([url])=>url);
    },
    diagnostics(){const response=sabrReads.stats(),request=requestReads.stats();return {...lastSabrObservation,playerReads:playerReads.size,playerReadPeak,playerReadCancels,sabrReads:response.active,sabrReadPeak:response.peak,sabrReadCancels:response.cancels,requestReads:request.active,requestReadPeak:request.peak,requestReadCancels:request.cancels,streamTasks:streamTasks.size,streamTaskPeak,streamTaskBytes:[...streamTasks.values()].reduce((sum,task)=>sum+(task.bytes?.byteLength||0),0)};},
    read(id){
      current(true);
      const player=document.getElementById('movie_player');
      try{if(!acceptedPlayers.has(id))rememberSnapshot(globalThis.ytInitialPlayerResponse);}catch{}
      try{rememberSnapshot(player?.getPlayerResponse?.());}catch{}
      return catalog.get(id)||null;
    },
    prepare(id,height){
      const player=document.getElementById('movie_player');
      if(current()!==id)return false;
      const levels=player?.getAvailableQualityLevels?.()||[];
      const names={144:'tiny',240:'small',360:'medium',480:'large',720:'hd720',1080:'hd1080',1440:'hd1440',2160:'hd2160',2880:'hd2880',4320:'hd4320'};
      const choice=names[height];if(!choice||!levels.includes(choice))return false;
      try{player.setPlaybackQualityRange?.(choice,choice);player.setPlaybackQuality?.(choice);return true;}catch{return false;}
    }
  });
  current(true);
  Object.defineProperty(globalThis,'__udmCaptureV1',{value:api,configurable:false});
})();
