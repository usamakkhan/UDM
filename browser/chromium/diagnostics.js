'use strict';
const api=globalThis.browser||chrome;
async function inspect(){
  const output=[];
  for(const tab of await api.tabs.query({url:['https://www.youtube.com/watch*','https://m.youtube.com/watch*']})){
    if(tab.incognito)continue;
    const result=await api.scripting.executeScript({target:{tabId:tab.id},world:'MAIN',func:()=>{
      const player=document.getElementById('movie_player'),id=new URL(location.href).searchParams.get('v');
      const raw=player?.getPlayerResponse?.(),early=globalThis.__udmCaptureV1?.read(id);
      function summarize(response){return {videoId:response?.videoDetails?.videoId,streamKeys:Object.keys(response?.streamingData||{}),formats:[...(response?.streamingData?.formats||[]),...(response?.streamingData?.adaptiveFormats||[])].map(f=>({itag:f.itag,height:f.height,quality:f.qualityLabel,mime:f.mimeType,url:!!f.url,cipher:!!(f.signatureCipher||f.cipher),urlKeys:(()=>{try{return [...new URL(f.url||new URLSearchParams(f.signatureCipher||f.cipher||'').get('url')).searchParams.keys()];}catch{return [];}})()}))};}
      const playback=performance.getEntriesByType('resource').filter(e=>{try{return new URL(e.name).hostname.endsWith('.googlevideo.com');}catch{return false;}}).slice(-12).map(e=>{const u=new URL(e.name);return {path:u.pathname,itag:u.searchParams.get('itag'),mime:u.searchParams.get('mime'),ump:u.searchParams.get('ump'),keys:[...u.searchParams.keys()]};});
      const panels=Array.from(document.querySelectorAll('[id^="udm-video-panel-"]')).slice(0,12).map(e=>{const r=e.getBoundingClientRect();return {display:getComputedStyle(e).display,left:r.left,top:r.top,width:r.width,height:r.height};});const videos=Array.from(document.querySelectorAll('video')).slice(0,12).map(e=>{const r=e.getBoundingClientRect();return {panelMarker:e.hasAttribute('data-udm-player'),width:r.width,height:r.height,ad:!!e.closest('.ad-showing,.ad-interrupting'),paused:e.paused};});
      return {panels,videos,videoId:id,playerId:player?.getVideoData?.()?.video_id,observerPresent:!!globalThis.__udmCaptureV1,sabr:globalThis.__udmCaptureV1?.diagnostics?.(),sabrSession:!!globalThis.__udmCaptureV1?.session?.(id),levels:player?.getAvailableQualityLevels?.(),player:summarize(raw),early:summarize(early),playback};
    }});
    const key='media:'+tab.id,stored=(await api.storage.session.get(key))[key]||[];
    const network=await api.runtime.sendMessage({action:'capture-diagnostics',tabId:tab.id});
    output.push({tabId:tab.id,browserCapture:network?.counts||{},...result[0]?.result,observed:stored.map(s=>({itag:s.itag,type:s.type,size:s.size,ageSeconds:Math.round((Date.now()-s.observedAt)/1000)}))});
  }
  document.getElementById('report').textContent=JSON.stringify(output.map(x=>({version:api.runtime.getManifest().version,videoId:x.videoId,playerId:x.playerId,observerPresent:x.observerPresent,sabr:x.sabr,sabrSession:x.sabrSession,browserCapture:x.browserCapture,panels:x.panels,videos:x.videos,formats:x.early?.formats?.length,playback:x.playback})),null,2)+'\n\n'+JSON.stringify(output,null,2);
}
document.getElementById('refresh').onclick=()=>inspect().catch(e=>document.getElementById('report').textContent=e.message);
document.getElementById('reload').onclick=()=>api.runtime.reload();
inspect().catch(e=>document.getElementById('report').textContent=e.message);
