(()=>{
  'use strict';
  const api=globalThis.browser||chrome;
  let current='',host;
  function mount(){
    const id=new URL(location.href).searchParams.get('v');
    if(location.pathname!=='/watch'||!id){host?.remove();host=null;current='';return;}
    const player=document.getElementById('movie_player');if(!player)return;
    if(host?.isConnected&&current===id)return;
    host?.remove();current=id;host=document.createElement('div');host.id='udm-media-capture';
    Object.assign(host.style,{position:'absolute',top:'12px',right:'12px',zIndex:'2147483000',fontSize:'14px'});
    const shadow=host.attachShadow({mode:'closed'});
    const style=document.createElement('style');style.textContent=':host{font:14px Segoe UI,Arial,sans-serif}button,select{font:inherit;color:#fff;background:#2160de;border:1px solid #5184e7;border-radius:7px;padding:9px 12px;cursor:pointer}button:hover{background:#164dc0}.panel{margin-top:6px;padding:14px;width:270px;color:#15223a;background:#fff;border:1px solid #d8e1f0;border-radius:10px;box-shadow:0 8px 28px #0005}select{width:100%;background:#f4f7fc;color:#15223a;border-color:#d8e1f0;margin:8px 0}p{font-size:12px;line-height:1.5;margin:8px 0}strong{font-size:15px}.status{overflow-wrap:anywhere}[hidden]{display:none}';
    const toggle=document.createElement('button');toggle.textContent='↓ Download with UDM';toggle.title='Download this video with audio';
    const panel=document.createElement('div');panel.className='panel';panel.hidden=true;
    const title=document.createElement('strong');title.textContent='Video + audio';
    const select=document.createElement('select');select.setAttribute('aria-label','UDM video quality');
    select.disabled=true;
    const note=document.createElement('p');note.textContent='Only qualities detected for this video are listed. Video and audio are saved together as MP4.';
    const download=document.createElement('button');download.textContent='↓ Download video';download.disabled=true;
    const refresh=document.createElement('button');refresh.textContent='Refresh qualities';
    const status=document.createElement('p');status.className='status';status.setAttribute('role','status');
    let choices=[],loading=false;
    async function load(){
      if(loading)return;loading=true;download.disabled=true;select.disabled=true;refresh.disabled=true;select.replaceChildren();choices=[];status.textContent='Reading this video’s available qualities…';
      try {
        const r=await api.runtime.sendMessage({action:'formats',url:'https://www.youtube.com/watch?v='+encodeURIComponent(id)});
        if(current!==id||!host?.isConnected)return;
        if(!r?.ok)throw new Error(r?.error||'UDM could not read the available qualities.');
        if(r.videoId!==id)throw new Error('The video changed. Refresh the list.');
        choices=r.choices||[];
        for(const choice of choices){const option=document.createElement('option');option.value=choice.key;option.textContent=choice.label;select.append(option);}
        select.disabled=!choices.length;download.disabled=!choices.length;status.textContent=choices.length?'Choose an available quality.':'No downloadable qualities detected.';
      }catch(err){if(current===id)status.textContent=err.message;}
      finally{loading=false;refresh.disabled=false;}
    }
    toggle.addEventListener('click',e=>{e.stopPropagation();if(e.isTrusted){panel.hidden=!panel.hidden;if(!panel.hidden)load();}});
    refresh.addEventListener('click',e=>{if(e.isTrusted)load();});
    panel.addEventListener('click',e=>e.stopPropagation());panel.addEventListener('keydown',e=>e.stopPropagation());
    download.addEventListener('click',async e=>{
      if(!e.isTrusted)return;
      const chosen=choices.find(x=>x.key===select.value);if(!chosen||current!==id)return;
      download.disabled=true;status.textContent='Sending to UDM…';
      try{const r=await api.runtime.sendMessage({action:'media',url:'https://www.youtube.com/watch?v='+encodeURIComponent(current),height:chosen.height,formatKey:chosen.key,title:(document.querySelector('h1.ytd-watch-metadata')?.textContent||document.title).replace(/ - YouTube$/,'').trim()});status.textContent=r?.ok?'Added to UDM. Check the desktop for progress.':r?.error||'UDM did not respond.';}catch(err){status.textContent=err.message;}
      finally{download.disabled=false;}
    });
    panel.append(title,select,note,download,refresh,status);shadow.append(style,toggle,panel);player.append(host);
  }
  document.addEventListener('yt-navigate-finish',mount);setInterval(mount,1500);mount();
})();
