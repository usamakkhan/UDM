// UDM's original, page-local observer. It does not alter requests or response bodies.
(()=>{
  'use strict';
  if(globalThis.__udmCaptureV1)return;
  const catalog=new Map(),sessions=new Map();
  let lastSabrObservation={requests:0,verified:0,error:''};
  function current(){const id=new URL(location.href).searchParams.get('v'),player=document.getElementById('movie_player');return /^[\w-]{11}$/.test(id||'')&&player?.getVideoData?.()?.video_id===id&&!player?.classList?.contains('ad-showing')?id:null;}
  function sabrEndpoint(value){try{const u=new URL(value,location.href);return u.protocol==='https:'&&u.hostname.endsWith('.googlevideo.com')&&u.pathname==='/videoplayback'&&(u.searchParams.has('sabr')||u.searchParams.get('ump')==='1')?u.href:null;}catch{return null;}}
  async function bodyBytes(body){if(body instanceof ArrayBuffer)return new Uint8Array(body.slice(0));if(ArrayBuffer.isView(body))return new Uint8Array(body.buffer.slice(body.byteOffset,body.byteOffset+body.byteLength));if(typeof Blob!=='undefined'&&body instanceof Blob&&body.size<=131072)return new Uint8Array(await body.arrayBuffer());return null;}
  async function inspectSabr(response,url,body,id){
    if(!id||!globalThis.__udmUmpV1)return;
    lastSabrObservation.requests++;
    try{
      const bytes=await body;if(!bytes?.length||bytes.length>131072||!__udmUmpV1.request(bytes)){lastSabrObservation.error='No bounded streaming request body';return;}
      if(!response.ok||!response.headers.get('content-type')?.includes('application/vnd.yt-ump'))return;
      const reader=response.clone().body?.getReader();if(!reader)return;let prefix=new Uint8Array(0),verified=false;
      try{while(prefix.length<262144){const {done,value}=await reader.read();if(done)break;const next=new Uint8Array(Math.min(262144,prefix.length+value.length));next.set(prefix);next.set(value.subarray(0,next.length-prefix.length),prefix.length);prefix=next;if(__udmUmpV1.identity(prefix,id)){verified=true;break;}}}finally{void reader.cancel().catch(()=>{});reader.releaseLock();}
      if(!verified||current()!==id){lastSabrObservation.error='Media identity not verified';return;}
      let raw='';for(const b of bytes)raw+=String.fromCharCode(b);sessions.set(id,{url,body:btoa(raw),videoId:id,capturedAt:Date.now()});if(sessions.size>3)sessions.delete(sessions.keys().next().value);lastSabrObservation.verified++;lastSabrObservation.error='';
    }catch(e){lastSabrObservation.error=String(e.message).slice(0,100);}
  }
  function remember(data){
    const id=data?.videoDetails?.videoId;
    if(!/^[\w-]{11}$/.test(id||'')||!data.streamingData)return;
    const streams=data.streamingData;
    function select(items){return (Array.isArray(items)?items:[]).slice(0,200).map(f=>({itag:f.itag,width:f.width,height:f.height,fps:f.fps,mimeType:f.mimeType,qualityLabel:f.qualityLabel,url:f.url,signatureCipher:f.signatureCipher,cipher:f.cipher,drmFamilies:f.drmFamilies,drmTrackType:f.drmTrackType,audioTrack:f.audioTrack,lastModified:f.lastModified,xtags:f.xtags}));}
    const response={videoDetails:{videoId:id,isLiveContent:!!data.videoDetails.isLiveContent,lengthSeconds:data.videoDetails.lengthSeconds},streamingData:{formats:select(streams.formats),adaptiveFormats:select(streams.adaptiveFormats)}};
    const prior=catalog.get(id);
    if(prior){for(const field of ['formats','adaptiveFormats']){
      const merged=new Map(prior.streamingData[field].map(f=>[String(f.itag),f]));
      for(const next of response.streamingData[field]){
        const previous=merged.get(String(next.itag))||{};
        const present=Object.fromEntries(Object.entries(next).filter(([,value])=>value!==undefined));
        merged.set(String(next.itag),{...previous,...present});
      }
      response.streamingData[field]=[...merged.values()].slice(-200);
    }}
    catalog.set(id,response);if(catalog.size>8)catalog.delete(catalog.keys().next().value);
  }
  function playerEndpoint(value){try{const u=new URL(value,location.href);return /(^|\.)youtube\.com$/.test(u.hostname)&&u.pathname==='/youtubei/v1/player';}catch{return false;}}
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
    const promise=Reflect.apply(originalFetch,this,[input,...args]);
    let target=false;try{target=playerEndpoint(typeof input==='string'||input instanceof URL?String(input):input?.url);}catch{}
    if(target)promise.then(response=>{if(response.ok)void inspect(response);},()=>{});
    try{const url=sabrEndpoint(typeof input==='string'||input instanceof URL?String(input):input?.url),method=String(args[0]?.method||input?.method||'GET').toUpperCase(),id=current();if(url&&method==='POST'&&id){const body=args[0]?.body!==undefined?bodyBytes(args[0].body):input?.clone?input.clone().arrayBuffer().then(b=>new Uint8Array(b)):Promise.resolve(null);promise.then(response=>void inspectSabr(response,url,body,id),()=>{});}}catch{}
    return promise;
  };
  if(globalThis.XMLHttpRequest){
    const originalOpen=XMLHttpRequest.prototype.open;
    XMLHttpRequest.prototype.open=function(method,url,...args){
      this.__udmSabr=String(method).toUpperCase()==='POST'?sabrEndpoint(url):null;
      if(playerEndpoint(url))this.addEventListener('load',()=>{try{const data=this.responseType==='json'?this.response:this.responseType===''||this.responseType==='text'?this.responseText:null;if(typeof data==='string'){if(data.length<=2097152)remember(JSON.parse(data));}else if(data)remember(data);}catch{}},{once:true});
      return Reflect.apply(originalOpen,this,[method,url,...args]);
    };
    const originalSend=XMLHttpRequest.prototype.send;
    XMLHttpRequest.prototype.send=function(body){const url=this.__udmSabr,id=current();if(url&&id){const bytes=bodyBytes(body);this.addEventListener('load',()=>{try{if(this.responseType==='arraybuffer'&&this.response){const response=new Response(this.response,{status:this.status,headers:{'content-type':this.getResponseHeader('content-type')||''}});void inspectSabr(response,url,bytes,id);}}catch{}},{once:true});}return Reflect.apply(originalSend,this,[body]);};
  }
  const api=Object.freeze({
    session(id){const value=sessions.get(id);return current()===id&&value&&value.capturedAt>Date.now()-300000?{...value}:null;},
    diagnostics(){return {...lastSabrObservation};},
    read(id){
      const player=document.getElementById('movie_player');
      try{remember(player?.getPlayerResponse?.());}catch{}
      try{remember(globalThis.ytInitialPlayerResponse);}catch{}
      return catalog.get(id)||null;
    },
    prepare(id,height){
      const player=document.getElementById('movie_player');
      if(new URL(location.href).searchParams.get('v')!==id||player?.getVideoData?.()?.video_id!==id)return false;
      const levels=player?.getAvailableQualityLevels?.()||[];
      const names={144:'tiny',240:'small',360:'medium',480:'large',720:'hd720',1080:'hd1080',1440:'hd1440',2160:'hd2160',2880:'hd2880',4320:'hd4320'};
      const choice=names[height];if(!choice||!levels.includes(choice))return false;
      try{player.setPlaybackQualityRange?.(choice,choice);player.setPlaybackQuality?.(choice);return true;}catch{return false;}
    }
  });
  Object.defineProperty(globalThis,'__udmCaptureV1',{value:api,configurable:false});
})();
