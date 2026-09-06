(()=>{
 'use strict';
 if(globalThis.__udmPanelsV2)return;globalThis.__udmPanelsV2=true;
 const api=globalThis.browser||chrome,panels=new Map(),abort=new AbortController(),options={signal:abort.signal},site=location.origin;
 let preferences={},excluded=[],scheduled=false,scanTimer=0;
 const resizeTargets=new Map();
 const resizeObserver=typeof ResizeObserver==='function'?new ResizeObserver(schedule):null;
 const intersectionObserver=typeof IntersectionObserver==='function'?new IntersectionObserver(entries=>{for(const entry of entries){const p=panels.get(entry.target);if(p)p.intersecting=entry.isIntersecting;}schedule();},{threshold:[0,0.01,0.1,0.5,1]}):null;
 const parentElement=node=>node.parentElement||node.getRootNode()?.host||null;
 function watchGeometry(p,nodes){
  if(p.geometryNodes?.length===nodes.length&&nodes.every((node,i)=>node===p.geometryNodes[i]))return;
  for(const node of p.geometryNodes||[]){const refs=(resizeTargets.get(node)||1)-1;if(refs)resizeTargets.set(node,refs);else{resizeTargets.delete(node);resizeObserver?.unobserve(node);}}
  p.geometryNodes=nodes;
  for(const node of nodes){const refs=resizeTargets.get(node)||0;if(!refs)resizeObserver?.observe(node);resizeTargets.set(node,refs+1);}
 }
 function geometry(p){
  const nodes=[],clips=[];let visible=true,opacity=1;
  for(let node=p.video;node&&nodes.length<100;node=parentElement(node)){
   nodes.push(node);const style=getComputedStyle(node);opacity*=Number(style.opacity||1);
   if(style.display==='none'||style.visibility==='hidden'||style.visibility==='collapse'||style.contentVisibility==='hidden'||opacity<=0.01||node.matches('.ad-showing,.ad-interrupting,[data-ad-playing="true"]'))visible=false;
   if(node===p.video)continue;
   const paint=/paint|strict|content/.test(style.contain),x=paint||/hidden|clip|scroll|auto/.test(style.overflowX),y=paint||/hidden|clip|scroll|auto/.test(style.overflowY);
   if(x||y){const r=node.getBoundingClientRect(),sx=node.offsetWidth?r.width/node.offsetWidth:1,sy=node.offsetHeight?r.height/node.offsetHeight:1,left=r.left+node.clientLeft*sx,top=r.top+node.clientTop*sy;clips.push({left,top,right:left+node.clientWidth*sx,bottom:top+node.clientHeight*sy,x,y});}
  }
  watchGeometry(p,nodes);return {visible,clips};
 }
 const youtubeId=()=>{const u=new URL(location.href);return /(^|\.)youtube(?:-nocookie)?\.com$/.test(u.hostname)?u.searchParams.get('v')||/^\/(?:embed|shorts)\/([\w-]{11})/.exec(u.pathname)?.[1]||'':'';};
 function element(tag,text,className){const e=document.createElement(tag);if(text)e.textContent=text;if(className)e.className=className;return e;}
 const css=':host{all:initial;position:fixed!important;display:block;font:11px Tahoma,Arial,sans-serif!important;color:#162736!important;z-index:2147483000!important;pointer-events:auto!important;color-scheme:light!important}*{box-sizing:border-box}button,select{font:11px Tahoma,Arial,sans-serif;color:#162736}button{cursor:pointer}button:focus-visible,select:focus-visible{outline:2px solid #267dd2;outline-offset:1px}.bar{height:24px;display:flex;align-items:center;border:1px solid #8facc3;border-radius:3px;background:linear-gradient(#fff,#dcebf6);box-shadow:0 1px 4px #0004;white-space:nowrap;overflow:hidden}.toggle{display:flex;gap:5px;align-items:center;border:0;background:none;height:22px;padding:0 7px;flex:1;min-width:0}.toggle:hover{background:#c7e5fa}.icon{width:14px;height:14px;background:#2678b9;color:#fff;border-radius:2px;text-align:center;font:bold 13px Arial}.caption{overflow:hidden;text-overflow:ellipsis}.arrow{margin-left:auto;font-size:9px}.grip{width:11px;height:22px;border:0;background:none;color:#668297;cursor:move;padding:0;touch-action:none}.mini .caption,.mini .arrow,.mini .grip{display:none}.mini .toggle{padding:0 7px}.menu{position:absolute;right:0;top:27px;width:276px;max-width:calc(100vw - 8px);padding:8px;border:1px solid #9bacba;background:#fff;border-radius:3px;box-shadow:0 3px 12px #0004}.head{display:flex;align-items:center;gap:5px;margin-bottom:6px}.head strong{flex:1;font-size:12px}.small{border:1px solid #bccbd6;background:#f4f8fc;border-radius:2px;padding:2px 6px}.choices{max-height:210px;overflow:auto}.choice{display:block;width:100%;text-align:left;border:0;border-bottom:1px solid #e5ebf0;background:#fff;padding:7px 5px;line-height:1.4}.choice:hover{background:#e6f2fc}.source{display:block;color:#687f90;font-size:10px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}.status{font-size:11px;line-height:1.4;margin:7px 0 2px;overflow-wrap:anywhere;max-height:100px;overflow:auto}.foot{display:flex;gap:6px;margin-top:7px}[hidden]{display:none!important}';
 async function save(){try{await api.storage.local.set({videoPanelSites:preferences});}catch{}}
 function create(video){
  const token=crypto.randomUUID();video.setAttribute('data-udm-player',token);video.setAttribute('data-udm-epoch',String(Date.now()));
  const host=element('div');host.id='udm-video-panel-'+token;const shadow=host.attachShadow({mode:'closed'}),style=element('style');style.textContent=css;
  const bar=element('div',null,'bar'),toggle=element('button',null,'toggle'),icon=element('span','↓','icon'),caption=element('span','Download this video','caption'),arrow=element('span','▼','arrow'),grip=element('button','⋮','grip');
  toggle.title='Download this video with UDM';toggle.setAttribute('aria-label','Download this video with UDM');toggle.setAttribute('aria-expanded','false');grip.title='Drag to reposition';grip.setAttribute('aria-label','Move UDM video panel');
  toggle.append(icon,caption,arrow);bar.append(toggle,grip);
  const menu=element('div',null,'menu');menu.hidden=true;const head=element('div',null,'head'),title=element('strong','Download with UDM'),mini=element('button','−','small'),close=element('button','×','small');
  mini.title='Toggle compact icon mode';mini.setAttribute('aria-label',mini.title);close.title='Close menu';close.setAttribute('aria-label','Close download menu');head.append(title,mini,close);
  const choices=element('div',null,'choices'),status=element('p',null,'status'),foot=element('div',null,'foot'),refresh=element('button','Refresh','small'),reset=element('button','Reset position','small'),hide=element('button','Hide here','small');
  status.setAttribute('role','status');foot.append(refresh,reset,hide);menu.append(head,choices,status,foot);shadow.append(style,bar,menu);
  const p={video,host,menu,toggle,bar,token,offset:{...(preferences[site]?.offset||{x:0,y:0})},loading:false,hidden:false,lastSource:video.currentSrc,cleanup:[]};
  function change(){video.setAttribute('data-udm-epoch',String(Date.now()));menu.hidden=true;toggle.setAttribute('aria-expanded','false');choices.replaceChildren();p.lastSource=video.currentSrc;schedule();}
  function encrypted(){video.setAttribute('data-udm-encrypted','');change();}
  for(const [event,fn] of [['loadstart',change],['emptied',change],['encrypted',encrypted]]){video.addEventListener(event,fn);p.cleanup.push(()=>video.removeEventListener(event,fn));}
  async function load(){
   if(p.loading)return;p.loading=true;choices.replaceChildren();status.textContent='Reading available video formats…';refresh.disabled=true;
   const page=location.href,stamp=video.getAttribute('data-udm-epoch'),id=youtubeId();
   try{
    if(video.mediaKeys||video.hasAttribute('data-udm-encrypted'))throw Error('This player uses encrypted or DRM-protected media.');
    const response=await api.runtime.sendMessage(id?{action:'formats',url:'https://www.youtube.com/watch?v='+id}:{action:'site-formats',page,token});
    if(page!==location.href||stamp!==video.getAttribute('data-udm-epoch')||!host.isConnected)return;
    if(!response?.ok)throw Error(response?.error||'UDM did not respond.');
    status.textContent=response.note||(response.choices?.length?'Select a format to open UDM.':'Play the video, then refresh.');
    for(const choice of response.choices||[]){const button=element('button',choice.label,'choice');if(choice.source)button.append(element('span',choice.source,'source'));
     button.addEventListener('click',async e=>{if(!e.isTrusted)return;e.stopPropagation();if(page!==location.href||stamp!==video.getAttribute('data-udm-epoch')){status.textContent='The video changed. Refresh the list.';return;}button.disabled=true;status.textContent='Sending to UDM…';
      try{const result=await api.runtime.sendMessage(id?{action:'media',url:'https://www.youtube.com/watch?v='+id,height:choice.height,formatKey:choice.key,title:document.title.replace(/ - YouTube$/,'')}:{action:'site-download',page,token,key:choice.key});status.textContent=result?.ok?'Added. Review Download File Info in UDM.':result?.error||'UDM did not respond.';}catch(error){status.textContent=error.message;}finally{button.disabled=false;}
     });choices.append(button);
    }
   }catch(error){status.textContent=error.message;}finally{p.loading=false;refresh.disabled=false;schedule();}
  }
  function open(value){menu.hidden=!value;toggle.setAttribute('aria-expanded',String(value));if(value)load();schedule();}
  toggle.addEventListener('click',e=>{e.stopPropagation();if(e.isTrusted)open(menu.hidden);});
  close.addEventListener('click',()=>open(false));refresh.addEventListener('click',e=>{if(e.isTrusted)load();});
  mini.addEventListener('click',()=>{preferences[site]={...preferences[site],mini:!preferences[site]?.mini};save();schedule();});
  reset.addEventListener('click',()=>{p.offset={x:0,y:0};preferences[site]={...preferences[site],offset:p.offset};save();schedule();});
  hide.addEventListener('click',()=>{p.hidden=true;host.style.setProperty('display','none','important');});
  menu.addEventListener('click',e=>e.stopPropagation());menu.addEventListener('keydown',e=>{e.stopPropagation();if(e.key==='Escape'){open(false);toggle.focus();}});
  let drag=null;grip.addEventListener('pointerdown',e=>{if(!e.isTrusted||e.button!==0)return;e.preventDefault();e.stopPropagation();drag={x:e.clientX,y:e.clientY,offset:{...p.offset}};grip.setPointerCapture(e.pointerId);});
  grip.addEventListener('pointermove',e=>{if(!drag)return;p.offset={x:drag.offset.x+e.clientX-drag.x,y:drag.offset.y+e.clientY-drag.y};schedule();});
  const stopDrag=()=>{if(!drag)return;drag=null;preferences[site]={...preferences[site],offset:p.offset};save();};grip.addEventListener('pointerup',stopDrag);grip.addEventListener('pointercancel',stopDrag);
  document.documentElement.append(host);panels.set(video,p);intersectionObserver?.observe(video);return p;
 }
 function schedule(){if(scheduled)return;scheduled=true;requestAnimationFrame(()=>{scheduled=false;layout();});}
 function layout(){
  for(const [video,p] of panels){if(!video.isConnected){p.cleanup.forEach(fn=>fn());watchGeometry(p,[]);intersectionObserver?.unobserve(video);p.host.remove();panels.delete(video);continue;}
   const rect=video.getBoundingClientRect(),full=document.fullscreenElement,settings=preferences[site]||{};
   const ad=video.closest('.ad-showing,.ad-interrupting,[data-ad-playing="true"]');
   const shape=geometry(p),view=globalThis.visualViewport;
   const viewport=view?{left:view.offsetLeft,top:view.offsetTop,width:view.width,height:view.height}:{width:innerWidth,height:innerHeight};
   const position=UdmMedia.placement(rect,viewport,settings.mini?30:168,24,p.offset,shape.clips);
   const visible=shape.visible&&position.visible&&p.intersecting!==false&&!p.hidden&&!ad&&document.pictureInPictureElement!==video;
   // Native fullscreen on a video element has no DOM overlay surface.
   if(!visible||(full&&(full===video||!p.geometryNodes.includes(full)))){p.host.style.setProperty('display','none','important');continue;}
   const parent=full||document.documentElement;if(p.host.parentNode!==parent)parent.append(p.host);
   p.host.style.setProperty('display','block','important');p.host.style.setProperty('left',position.x+'px','important');p.host.style.setProperty('top',position.y+'px','important');
   p.host.style.setProperty('width',position.width+'px','important');p.bar.classList.toggle('mini',settings.mini||position.compact);
   p.menu.style.right='0';p.menu.style.left='auto';if(position.x+position.width<280){p.menu.style.left='0';p.menu.style.right='auto';}
   const available=Math.max(90,innerHeight-position.y-32);p.menu.style.maxHeight=Math.min(360,available)+'px';p.menu.style.overflowY='auto';
   if(!p.menu.hidden&&available<160&&position.y>180){p.menu.style.top='auto';p.menu.style.bottom='27px';p.menu.style.maxHeight=Math.min(360,position.y-8)+'px';}else{p.menu.style.top='27px';p.menu.style.bottom='auto';}
  }
 }
 function scan(){scanTimer=0;if(excluded.some(x=>location.hostname===x||location.hostname.endsWith('.'+x)))return;
  const roots=[document],videos=[];for(let i=0;i<roots.length&&i<40;i++){videos.push(...roots[i].querySelectorAll('video'));for(const e of Array.from(roots[i].querySelectorAll('*')).slice(0,3000))if(e.shadowRoot&&roots.length<40)roots.push(e.shadowRoot);}
  updateRoots(roots);
  for(const video of videos.slice(0,30))if(!panels.has(video))create(video);schedule();
 }
 const observerOptions={childList:true,subtree:true,attributes:true,attributeFilter:['class','style','hidden','open','data-ad-playing']};
 let observedRoots=[];
 const observer=new MutationObserver(records=>{
  const relevant=records.filter(r=>!r.target.closest?.('[id^="udm-video-panel-"]'));
  if(!relevant.length)return;schedule();
  if(relevant.some(r=>r.type==='childList')&&!scanTimer)scanTimer=setTimeout(scan,250);
 });
 function updateRoots(roots){if(roots.length===observedRoots.length&&roots.every((r,i)=>r===observedRoots[i]))return;observer.disconnect();observedRoots=roots;for(const root of roots)observer.observe(root===document?document.documentElement:root,observerOptions);}
 updateRoots([document]);
 for(const name of ['scroll','resize','fullscreenchange','yt-navigate-finish','transitionend','animationend'])globalThis.addEventListener(name,schedule,{...options,capture:true,passive:true});
 for(const name of ['resize','scroll'])globalThis.visualViewport?.addEventListener(name,schedule,{...options,passive:true});
 document.addEventListener('pointerdown',e=>{for(const p of panels.values())if(!e.composedPath().includes(p.host)){p.menu.hidden=true;p.toggle.setAttribute('aria-expanded','false');}},options);
 const timer=setInterval(scan,1500);
 api.storage.local.get(['videoPanelSites','settings']).then(data=>{preferences=data.videoPanelSites||{};excluded=data.settings?.excluded||[];scan();}).catch(scan);
 addEventListener('pagehide',event=>{if(event.persisted)return;clearInterval(timer);clearTimeout(scanTimer);observer.disconnect();resizeObserver?.disconnect();intersectionObserver?.disconnect();resizeTargets.clear();abort.abort();for(const p of panels.values()){p.cleanup.forEach(fn=>fn());p.host.remove();}panels.clear();});
 addEventListener('pageshow',schedule);
})();
