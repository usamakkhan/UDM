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
      formats.push({id:String(f.itag||''),lastModified:String(f.lastModified||''),xtags:String(f.xtags||''),height:Number(f.height)||0,qualityLabel:String(f.qualityLabel||""),width:Number(f.width)||0,fps:Number(f.fps)||0,mime:String(f.mimeType||'').slice(0,180),url:String(f.url||'').slice(0,16000),matchUrl:String(f.url||new URLSearchParams(f.signatureCipher||f.cipher||'').get('url')||'').slice(0,16000),muxed,audioDefault:f.audioTrack?.audioIsDefault!==false});
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
      if(u.protocol!=='https:'||!u.hostname.endsWith('.googlevideo.com')||u.pathname!=='/videoplayback'||u.searchParams.get('itag')!==String(id)||u.searchParams.get('ump')==='1'||u.searchParams.has('sabr')||u.searchParams.has('sq'))return '';
      const expiry=Number(u.searchParams.get('expire'));
      if(expiry&&expiry*1000<Date.now()+120000)return '';
      const signed=(u.searchParams.get('sparams')||'').split(',');
      for(const key of ['range','rn','rbuf']){if(signed.includes(key)&&u.searchParams.has(key))return '';u.searchParams.delete(key);}
      return u.href;
    }catch{return '';}
  }
  function label(height,fps,codec) {
    return height+'p'+(fps>30?' '+Math.round(fps)+' fps':'')+(height===1440?' (2K)':height===2160?' (4K)':height===4320?' (8K)':'')+' · MP4'+(codec?' · '+codec:'');
  }
  function attachObserved(snapshot,captured) {
    if(!snapshot||!Array.isArray(snapshot.formats))return snapshot;
    return {...snapshot,formats:snapshot.formats.map(f=>{
      try {
        const base=new URL(f.matchUrl||f.url),resource=base.searchParams.get('id');
        if(!resource||base.protocol!=='https:'||!base.hostname.endsWith('.googlevideo.com'))return f;
        const found=(captured||[]).find(c=>{
          const actual=new URL(c.url);
          return String(c.itag)===f.id&&actual.searchParams.get('id')===resource&&actual.searchParams.get('itag')===f.id&&c.observedAt>Date.now()-600000&&url(c.url,f.id);
        });
        return found?{...f,url:found.url}:f;
      }catch{return f;}
    })};
  }
  function choices(snapshot,id) {
    if(!snapshot||snapshot.videoId!==id||snapshot.live)return [];
    const raw=Array.isArray(snapshot.formats)?snapshot.formats.slice(0,400):[];
    const audio=raw.find(f=>f.audioDefault!==false&&/^audio\/mp4\b/i.test(f.mime)&&/mp4a/i.test(f.mime)&&/^\d{1,6}$/.test(f.id)&&url(f.url,f.id));
    const byHeight=new Map();
    for(const f of raw) {
      const pixelHeight=Number(f.height),fps=Number(f.fps)||0;
      const height=Number(String(f.qualityLabel||'').match(/^(\d+)p/)?.[1])||pixelHeight;
      if(!/^\d{1,6}$/.test(f.id)||!Number.isInteger(height)||height<144||height>4320||!/^video\/(mp4|webm)\b/i.test(f.mime))continue;
      const codec=/avc1/i.test(f.mime)?'H.264':/av01/i.test(f.mime)?'AV1':/vp09|vp9/i.test(f.mime)?'VP9':'';
      if(!codec)continue;
      const videoUrl=url(f.url,f.id),audioUrl=f.muxed?'':audio?url(audio.url,audio.id):'';
      const direct=!!videoUrl&&(!!f.muxed||!!audioUrl);
      const rank=(direct?1000:0)+(codec==='H.264'?300:codec==='AV1'?200:100)+Math.min(fps,120);
      const item={key:String(f.id),formatId:String(f.id),height,pixelHeight,fps,codec,label:label(height,fps,codec),videoUrl:direct?videoUrl:'',audioUrl:direct?audioUrl:'',rank};
      if(!byHeight.has(height)||byHeight.get(height).rank<rank)byHeight.set(height,item);
    }
    // The player may expose only its available-quality list, with URLs hidden.
    // These are actual player choices, not a static menu of every supported height.
    if(!byHeight.size)for(const level of snapshot.levels||[]) {
      const height=heights[level];if(height)byHeight.set(height,{key:'height:'+height,formatId:'',height,pixelHeight:0,fps:0,codec:'',label:label(height,0,''),videoUrl:'',audioUrl:''});
    }
    return [...byHeight.values()].sort((a,b)=>b.height-a.height);
  }
  return {choices,attachObserved};
})();
if(typeof module!=='undefined')module.exports={readYouTubeFormats,UdmFormats};
