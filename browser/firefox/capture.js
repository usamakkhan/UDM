// UDM's original, page-local observer. It does not alter requests or response bodies.
(()=>{
  'use strict';
  if(globalThis.__udmCaptureV1)return;
  const catalog=new Map(),sessions=new Map(),captionRequests=new Map();
  let pageVideoId=null,routeGeneration=0;
  function invalidateRoute(){
    routeGeneration++;
    sessions.clear();
    captionRequests.clear();
    document.documentElement?.removeAttribute?.('data-udm-content-ready');
  }
  document.addEventListener?.('yt-navigate-start',invalidateRoute,true);
  globalThis.addEventListener?.('popstate',()=>current(true),true);
  let lastSabrObservation={requests:0,verified:0,error:''};
  function current(allowVerifiedContent=false){
    const id=new URL(location.href).searchParams.get('v')||/^\/(?:embed|shorts)\/([\w-]{11})/.exec(location.pathname)?.[1]||null;
    if(id!==pageVideoId){pageVideoId=id;invalidateRoute();}
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
    if(typeof Blob!=='undefined'&&body instanceof Blob&&body.size<=maxSabrRequestBytes)return new Uint8Array(await body.arrayBuffer());
    return null;
  }
  async function requestBodyBytes(request){
    const reader=Request.prototype.clone.call(request).body?.getReader();
    if(!reader)return null;
    const bytes=new Uint8Array(maxSabrRequestBytes);
    let length=0;
    try{
      for(;;){
        const {done,value}=await reader.read();
        if(done)return bytes.subarray(0,length);
        if(value.byteLength>maxSabrRequestBytes-length)return null;
        bytes.set(value,length);
        length+=value.byteLength;
      }
    }finally{
      void reader.cancel().catch(()=>{});
      reader.releaseLock();
    }
  }
  async function inspectSabr(response,url,body,id,generation,capturedAt){
    if(!id||!globalThis.__udmUmpV1)return;
    lastSabrObservation.requests++;
    let reader;
    try{
      // Clone before yielding, since playback can consume the original as soon as fetch resolves.
      if(response.ok&&response.headers.get('content-type')?.includes('application/vnd.yt-ump'))
        reader=response.clone().body?.getReader();
      const bytes=await body;if(!bytes?.length||!__udmUmpV1.request(bytes)){lastSabrObservation.error='No bounded streaming request body';return;}
      if(!reader)return;let prefix=new Uint8Array(0),verified=false;
      while(prefix.length<262144){const {done,value}=await reader.read();if(done)break;const next=new Uint8Array(Math.min(262144,prefix.length+value.length));next.set(prefix);next.set(value.subarray(0,next.length-prefix.length),prefix.length);prefix=next;if(__udmUmpV1.identity(prefix,id)){verified=true;break;}}
      if(!verified||current(true)!==id||generation!==routeGeneration){lastSabrObservation.error='Media identity not verified';return;}
      if((sessions.get(id)?.capturedAt??0)>capturedAt)return;
      let raw='';for(const b of bytes)raw+=String.fromCharCode(b);sessions.set(id,{url,body:btoa(raw),videoId:id,capturedAt});document.documentElement?.setAttribute('data-udm-content-ready',id+':'+Date.now());if(sessions.size>3)sessions.delete(sessions.keys().next().value);lastSabrObservation.verified++;lastSabrObservation.error='';
    }catch(e){lastSabrObservation.error=String(e.message).slice(0,100);}
    finally{if(reader){void reader.cancel().catch(()=>{});reader.releaseLock();}}
  }
  function lifecycle(data){
    const details=data?.videoDetails;
    return [details?.isLive===true,details?.isUpcoming===true,details?.isPostLiveDvr===true,data?.microformat?.playerMicroformatRenderer?.liveBroadcastDetails?.isLiveNow===true].join(',');
  }
  function remember(data){
    const id=data?.videoDetails?.videoId;
    const captions=data?.captions?.playerCaptionsTracklistRenderer?.captionTracks;
    if(!/^[\w-]{11}$/.test(id||'')||(!data.streamingData&&!Array.isArray(captions)&&lifecycle(data)==='false,false,false,false'))return;
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
    catalog.set(id,response);if(catalog.size>8)catalog.delete(catalog.keys().next().value);
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
  async function inspect(response){
    try{
      const copy=response.clone();
      if(Number(copy.headers.get('content-length'))>2097152)return;
      const reader=copy.body?.getReader();if(!reader)return;
      const decoder=new TextDecoder();let text='',bytes=0;
      try{for(;;){const part=await reader.read();if(part.done)break;bytes+=part.value.length;if(bytes>2097152){await reader.cancel();return;}text+=decoder.decode(part.value,{stream:true});}text+=decoder.decode();remember(JSON.parse(text));}finally{reader.releaseLock();}
    }catch{}
  }
  const originalFetch=globalThis.fetch;
  if(typeof originalFetch==='function')globalThis.fetch=function(input,...args){
    let sabr;
    try{
      const url=sabrEndpoint(typeof input==='string'||input instanceof URL?String(input):input?.url);
      if(url&&String(args[0]?.method||input?.method||'GET').toUpperCase()==='POST'){
        const id=current(true),generation=routeGeneration,capturedAt=Date.now();
        if(id){
          // A Request body becomes unusable for cloning once native fetch takes it.
          const body=args[0]?.body!=null?bodyBytes(args[0].body):requestBodyBytes(input);
          sabr={url,body:body.catch(()=>null),id,generation,capturedAt};
        }
      }
    }catch{}
    const promise=Reflect.apply(originalFetch,this,[input,...args]);
    if(sabr)promise.then(response=>void inspectSabr(response,sabr.url,sabr.body,sabr.id,sabr.generation,sabr.capturedAt),()=>{});
    let target=false;try{target=playerEndpoint(typeof input==='string'||input instanceof URL?String(input):input?.url);}catch{}
    if(target)promise.then(response=>{if(response.ok)void inspect(response);},()=>{});
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
      this.__udmSabr=String(method).toUpperCase()==='POST'?sabrEndpoint(url):null;
      this.__udmCaption=String(method).toUpperCase()==='GET'&&!!captionEndpoint(url);
      if(playerEndpoint(url))this.addEventListener('load',()=>{try{const data=this.responseType==='json'?this.response:this.responseType===''||this.responseType==='text'?this.responseText:null;if(typeof data==='string'){if(data.length<=2097152)remember(JSON.parse(data));}else if(data)remember(data);}catch{}},{once:true});
      return Reflect.apply(originalOpen,this,[method,url,...args]);
    };
    const originalSend=XMLHttpRequest.prototype.send;
    XMLHttpRequest.prototype.send=function(body){
      const url=this.__udmSabr,id=current(true),generation=routeGeneration;
      if(url&&id){const capturedAt=Date.now(),bytes=bodyBytes(body);this.addEventListener('load',()=>{try{if(this.responseType==='arraybuffer'&&this.response){const response=new Response(this.response,{status:this.status,headers:{'content-type':this.getResponseHeader('content-type')||''}});void inspectSabr(response,url,bytes,id,generation,capturedAt);}}catch{}},{once:true});}
      if(this.__udmCaption&&id)this.addEventListener('load',()=>{
        if(this.status===200)rememberCaptionRequest(this.responseURL,this.getResponseHeader('content-type'),
          this.getResponseHeader('content-length'),id,generation);
      },{once:true});
      return Reflect.apply(originalSend,this,[body]);
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
    diagnostics(){return {...lastSabrObservation};},
    read(id){
      const player=document.getElementById('movie_player');
      try{remember(globalThis.ytInitialPlayerResponse);}catch{}
      try{remember(player?.getPlayerResponse?.());}catch{}
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
  Object.defineProperty(globalThis,'__udmCaptureV1',{value:api,configurable:false});
})();
