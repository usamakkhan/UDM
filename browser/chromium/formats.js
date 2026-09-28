'use strict';
// Executed in the page's main world through the extension scripting API.
// Return only bounded media metadata; never trust an unrelated/stale player response.
function readYouTubeFormats(expectedId) {
  const pageId=location.pathname==='/watch'?new URL(location.href).searchParams.get('v'):/^\/(?:embed|shorts)\/([\w-]{11})/.exec(location.pathname)?.[1];
  if(pageId!==expectedId)return null;
  const player=document.getElementById('movie_player');
  const data=player?.getVideoData?.();
  if(player?.classList?.contains('ad-showing')||player?.classList?.contains('ad-interrupting'))return null;
  let response=globalThis.__udmCaptureV1?.read(expectedId)||player?.getPlayerResponse?.();
  if(response?.videoDetails?.videoId!==expectedId)response=globalThis.ytInitialPlayerResponse;
  if(data?.isLive||response?.videoDetails?.isLiveContent)return {videoId:expectedId,live:true,formats:[],levels:[]};
  if(data?.video_id&&data.video_id!==expectedId)return null;
  const same=response?.videoDetails?.videoId===expectedId;
  const stream=same?response.streamingData:null;
  const formats=[];
  for(const [items,muxed] of [[stream?.formats,true],[stream?.adaptiveFormats,false]]) {
    for(const f of (Array.isArray(items)?items:[]).slice(0,200)) {
      if(f.drmFamilies?.length||f.drmTrackType)continue;
      formats.push({id:String(f.itag||''),lastModified:String(f.lastModified||''),xtags:String(f.xtags||''),height:Number(f.height)||0,qualityLabel:String(f.qualityLabel||""),width:Number(f.width)||0,fps:Number(f.fps)||0,mime:String(f.mimeType||'').slice(0,180),url:String(f.url||'').slice(0,16000),matchUrl:String(f.url||new URLSearchParams(f.signatureCipher||f.cipher||'').get('url')||'').slice(0,16000),muxed,audioDefault:f.audioTrack?.audioIsDefault!==false,audioTrackId:String(f.audioTrack?.id||'').slice(0,128),audioName:String(f.audioTrack?.displayName||'').slice(0,120),contentLength:Number(f.contentLength)||0,bitrate:Number(f.averageBitrate||f.bitrate)||0});
    }
  }
  const levels=data?.video_id===expectedId?player?.getAvailableQualityLevels?.():[];
  return {videoId:expectedId,live:false,visitorData:String(globalThis.ytcfg?.get?.("VISITOR_DATA")||response?.responseContext?.visitorData||"").slice(0,2048),signatureTimestamp:Number(globalThis.ytcfg?.get?.("STS")||globalThis.ytplayer?.config?.sts)||0,timeOrigin:performance.timeOrigin,durationMs:Number(response?.videoDetails?.lengthSeconds||0)*1000,formats,levels:Array.isArray(levels)?levels.slice(0,30):[]};
}

const UdmFormats=(()=>{
  const heights={tiny:144,small:240,medium:360,large:480,hd720:720,hd1080:1080,hd1440:1440,hd2160:2160,hd2880:2880,hd4320:4320};
  function url(value,id) {
    try {
      const u=new URL(value);
      if(u.protocol!=='https:'||u.username||u.password||(u.port&&u.port!=='443')||!u.hostname.endsWith('.googlevideo.com')||u.pathname!=='/videoplayback'||u.searchParams.get('itag')!==String(id)||u.searchParams.get('ump')==='1'||u.searchParams.has('sabr')||u.searchParams.has('sq'))return '';
      const expiry=Number(u.searchParams.get('expire'));
      if(u.searchParams.has('expire')&&(!Number.isSafeInteger(expiry)||expiry*1000<Date.now()+120000))return '';
      const signed=(u.searchParams.get('sparams')||'').split(',');
      for(const key of ['range','rn','rbuf']){if(signed.includes(key)&&u.searchParams.has(key))return '';u.searchParams.delete(key);}
      return u.href;
    }catch{return '';}
  }
  // Player metadata may still contain an untransformed n value. Only a
  // successful, identity-matched playback observation can make that URL ready.
  function directUrl(f) {
    const value=url(f?.url,f?.id);
    return value&&(!new URL(value).searchParams.has('n')||f.observed===true)?value:'';
  }
  function size(f){if(!f)return 0;let n=Number(f.contentLength);if(!n)try{n=Number(new URL(f.url||f.matchUrl).searchParams.get('clen'));}catch{}return Number.isSafeInteger(n)&&n>0?n:0;}
  function label(height,fps,codec) {
    return height+'p'+(fps>30?' '+Math.round(fps)+' fps':'')+(height===1440?' (2K)':height===2160?' (4K)':height===4320?' (8K)':'')+' \u00b7 MP4'+(codec?' \u00b7 '+codec:'');
  }
  function attachObserved(snapshot,captured) {
    if(!snapshot||!Array.isArray(snapshot.formats))return snapshot;
    return {...snapshot,formats:snapshot.formats.map(f=>{
      try {
        const base=new URL(f.matchUrl||f.url),resource=base.searchParams.get('id');
        if(!resource||base.protocol!=='https:'||!base.hostname.endsWith('.googlevideo.com'))return f;
        const found=(captured||[]).find(c=>{
          const actual=new URL(c.url);
          return String(c.itag)===f.id&&actual.searchParams.get('id')===resource&&actual.searchParams.get('itag')===f.id&&(!/^audio\//i.test(f.mime)||(actual.searchParams.get('xtags')||'')===(base.searchParams.get('xtags')||f.xtags||'')&&(actual.searchParams.get('lmt')||'')===(base.searchParams.get('lmt')||String(f.lastModified||'')))&&c.observedAt>Date.now()-600000&&url(c.url,f.id);
        });
        return found?{...f,url:found.url,observed:true}:f;
      }catch{return f;}
    })};
  }
  function audioChoices(snapshot,id,allowStreaming=false) {
    if(!snapshot||snapshot.videoId!==id||snapshot.live)return [];
    const selected=new Map();
    for(const f of (Array.isArray(snapshot.formats)?snapshot.formats:[]).slice(0,400)) {
      if(f.muxed||!/^audio\/mp4\b/i.test(f.mime)||!/mp4a/i.test(f.mime)||!/^\d{1,6}$/.test(f.id))continue;
      let audioUrl=directUrl(f),resource='',u=audioUrl?new URL(audioUrl):null;
      if(u){resource=u.searchParams.get('id')||'';
        if(!resource||!/^\d+$/.test(u.searchParams.get('expire')||'')){audioUrl='';resource='';}
      }
      const lastModified=String(f.lastModified||''),tags=String(f.xtags||u?.searchParams.get('xtags')||'');
      const identity=/^[1-9]\d{0,19}$/.test(lastModified)&&BigInt(lastModified)<=18446744073709551615n&&tags.length<=2048;
      if(!audioUrl&&(!allowStreaming||!identity||!Number.isSafeInteger(snapshot.durationMs)||snapshot.durationMs<=0||snapshot.durationMs>86400000))continue;
      const trackId=String(f.audioTrackId||'').slice(0,128);
      const key=JSON.stringify([f.id,trackId,tags]);
      const name=String(f.audioName||trackId.split('.')[0]||'Audio').replace(/[\x00-\x1f\x7f]/g,' ').trim().slice(0,120)||'Audio';
      const rate=Number(f.bitrate),bitrate=Number.isFinite(rate)&&rate>0&&rate<10000000?Math.round(rate/1000):0;
      const label=name+' · AAC'+(bitrate?' · '+bitrate+' kbps':'')+' · M4A';
      const item={container:'m4a',size:size(f),key,label,default:f.audioDefault!==false,bitrate,audioUrl,resource,formatId:f.id,lastModified,streamFormat:identity?{id:f.id,lastModified,xtags:tags,mime:f.mime}:null};
      // Ambiguous duplicate track metadata must not silently pick a different file.
      if(selected.has(key)&&(selected.get(key)?.audioUrl!==audioUrl||selected.get(key)?.lastModified!==lastModified))selected.set(key,null);
      else if(!selected.has(key))selected.set(key,item);
    }
    return [...selected.values()].filter(Boolean).sort((a,b)=>Number(b.default)-Number(a.default)||b.bitrate-a.bitrate||a.label.localeCompare(b.label));
  }
  function choices(snapshot,id) {
    if(!snapshot||snapshot.videoId!==id||snapshot.live)return [];
    const raw=Array.isArray(snapshot.formats)?snapshot.formats.slice(0,400):[];
    const audio=raw.find(f=>f.audioDefault!==false&&/^audio\/mp4\b/i.test(f.mime)&&/mp4a/i.test(f.mime)&&/^\d{1,6}$/.test(f.id)&&directUrl(f));
    const byHeight=new Map();
    for(const f of raw) {
      const pixelHeight=Number(f.height),fps=Number(f.fps)||0;
      const height=Number(String(f.qualityLabel||'').match(/^(\d+)p/)?.[1])||pixelHeight;
      if(!/^\d{1,6}$/.test(f.id)||!Number.isInteger(height)||height<144||height>4320||!/^video\/(mp4|webm)\b/i.test(f.mime))continue;
      const codec=/avc1/i.test(f.mime)?'H.264':/av01/i.test(f.mime)?'AV1':/vp09|vp9/i.test(f.mime)?'VP9':'';
      if(!codec)continue;
      const videoUrl=directUrl(f),audioUrl=f.muxed?'':audio?directUrl(audio):'';
      const direct=!!videoUrl&&(!!f.muxed||!!audioUrl);
      // Without a ready file URL, prefer an MP4 adaptive format that supports
      // native retrieval or streaming over an unavailable combined format.
      const rank=(direct?1000:!f.muxed&&/^video\/mp4\b/i.test(f.mime)?500:0)+(codec==='H.264'?300:codec==='AV1'?200:100)+Math.min(fps,120);
      const item={container:'mp4',size:f.muxed?size(f):size(f)&&size(audio)&&Number.isSafeInteger(size(f)+size(audio))?size(f)+size(audio):0,key:String(f.id),formatId:String(f.id),height,pixelHeight,fps,codec,label:label(height,fps,codec),videoUrl:direct?videoUrl:'',audioUrl:direct?audioUrl:'',rank};
      if(!byHeight.has(height)||byHeight.get(height).rank<rank)byHeight.set(height,item);
    }
    // The player may expose only its available-quality list, with URLs hidden.
    // These are actual player choices, not a static menu of every supported height.
    if(!byHeight.size)for(const level of snapshot.levels||[]) {
      const height=heights[level];if(height)byHeight.set(height,{container:'mp4',key:'height:'+height,formatId:'',height,pixelHeight:0,fps:0,codec:'',label:label(height,0,''),videoUrl:'',audioUrl:''});
    }
    return [...byHeight.values()].sort((a,b)=>b.height-a.height);
  }
  return {choices,audioChoices,attachObserved};
})();
if(typeof module!=='undefined')module.exports={readYouTubeFormats,UdmFormats};
