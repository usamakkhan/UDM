'use strict';
const api=globalThis.browser||chrome;
const $=id=>document.getElementById(id);
const status=text=>{$('status').textContent=text;};
let activeTab,videoChoices=[];
(async()=>{
  [activeTab]=await api.tabs.query({active:true,currentWindow:true});
  $('youtube').hidden=!/^https:\/\/(www\.|m\.)?youtube\.com\/watch\?/.test(activeTab?.url||'');
  const stored=await api.storage.local.get(['settings','lastError']);
  const s=stored.settings||{};
  $('selected-links').value=s.selectedLinks||'all';$('selected-link-hosts').value=(s.selectedLinkHosts||[]).join(' ');$('selected-links-mini').checked=!!s.selectedLinksMini;
  $('capture').checked=!!s.capture;$('cookies').checked=!!s.cookies;
  $('extensions').value=(s.extensions||['zip','7z','rar','iso','exe','msi','pdf','mp4','mkv','mp3','flac']).join(' ');
  $('excluded').value=(s.excluded||[]).join(' ');
  if(stored.lastError)status(stored.lastError);
  if(!$('youtube').hidden)await loadQualities();
})().catch(e=>status(e.message));
async function loadQualities(){
  $('video-download').disabled=true;$('quality').disabled=true;$('refresh-qualities').disabled=true;$('quality').replaceChildren();videoChoices=[];
  status('Reading this video’s available qualities…');
  try {
    const r=await api.runtime.sendMessage({action:'formats',url:activeTab.url,tabId:activeTab.id});
    if(!r?.ok)throw new Error(r?.error||'No response from UDM.');
    videoChoices=r.choices||[];
    for(const choice of videoChoices){const option=document.createElement('option');option.value=choice.key;option.textContent=choice.label;$('quality').append(option);}
    $('video-download').disabled=!videoChoices.length;$('quality').disabled=!videoChoices.length;status(videoChoices.length?'Choose an available quality.':'No downloadable qualities detected.');
  }catch(e){status(e.message);}finally{$('refresh-qualities').disabled=false;}
}
$('refresh-qualities').onclick=loadQualities;
$('video-download').onclick=async()=>{
  const chosen=videoChoices.find(x=>x.key===$('quality').value);if(!chosen)return;
  $('video-download').disabled=true;
  try{status('Sending video to UDM…');const r=await api.runtime.sendMessage({action:'media',url:activeTab.url,tabId:activeTab.id,title:activeTab.title.replace(/ - YouTube$/,''),height:chosen.height,formatKey:chosen.key});status(r?.ok?'Video added. Check UDM’s Download File Info window.':r?.error||'No response.');}catch(e){status(e.message);}finally{$('video-download').disabled=false;}
};
async function download(url,filename){
  if(/\.(m3u8|mpd)(?:[?#]|$)/i.test(url)){status('This is a streaming playlist. Use Download this video on its player to assemble the video.');return;}
  status('Sending to UDM…');
  const result=await api.runtime.sendMessage({action:'download',url,filename,referrer:activeTab?.url});
  status(result?.ok?'Added to UDM.':result?.error||'No reply from UDM.');
}
$('add').addEventListener('submit',e=>{e.preventDefault();download($('url').value).catch(e=>status(e.message));});
$('connect').onclick=async()=>{try{const r=await api.runtime.sendMessage({action:'ping'});status(r?.ok?'UDM is connected and ready.':r?.error||'UDM is unavailable.');}catch(e){status(e.message);}};
$('save').onclick=async()=>{
  try{
    if($('cookies').checked){
      const granted=await api.permissions.request({permissions:['cookies'],origins:['http://*/*','https://*/*']});
      if(!granted){$('cookies').checked=false;status('Cookie permission was not granted.');return;}
    }
    const split=id=>$(id).value.toLowerCase().split(/[ ,;]+/).map(s=>s.trim()).filter(Boolean);
    await api.storage.local.set({settings:{...(await api.storage.local.get('settings')).settings,selectedLinks:$('selected-links').value,selectedLinkHosts:split('selected-link-hosts'),selectedLinksMini:$('selected-links-mini').checked,capture:$('capture').checked,cookies:$('cookies').checked,extensions:split('extensions'),excluded:split('excluded')}});
    status('Integration settings saved.');
  }catch(e){status(e.message);}
};
async function enablePanels(origins){try{const ok=await api.permissions.request({origins});if(!ok){status('Permission was not granted.');return;}const result=await api.runtime.sendMessage({action:'sync-video-panels'});status(result?.ok?'Video panels enabled. Play a video; refresh the page if needed.':result?.error||'Could not activate video panels.');}catch(e){status(e.message);}}
$('observe').onclick=()=>enablePanels(['http://*/*','https://*/*']);
$('enable-site').onclick=()=>{try{const u=new URL(activeTab?.url);if(!/^https?:$/.test(u.protocol))throw Error('Open a website first.');enablePanels([u.origin+'/*']);}catch(e){status(e.message);}};
$('discover').onclick=async()=>{
  try{
    if(!activeTab?.id)throw new Error('Select a web page first.');
    const results=await api.scripting.executeScript({target:{tabId:activeTab.id},func:()=>Array.from(document.querySelectorAll('a[href],video,audio,source')).map(e=>({url:e.currentSrc||e.src||e.href,text:e.textContent?.trim().slice(0,80)||e.type||''})).filter(x=>/^https?:\/\//.test(x.url))});
    const key='media:'+activeTab.id;
    const captured=(await api.storage.session.get(key))[key]||[];
    const links=[...results.flatMap(r=>r.result||[]),...captured];
    $('media').replaceChildren();
    const unique=[...new Map(links.map(x=>[x.url,x])).values()].slice(0,100);
    for(const link of unique){const b=document.createElement('button');b.textContent=link.text||decodeURIComponent(new URL(link.url).pathname.split('/').pop())||link.url;b.title=link.url;b.onclick=()=>download(link.url).catch(e=>status(e.message));$('media').append(b);}
    status(unique.length+' links and direct media URLs found. Select one to download.');
  }catch(e){status(e.message);}
};
