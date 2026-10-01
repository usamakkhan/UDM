(()=>{
 'use strict';
 let api,runtimeIdentity;try{api=[globalThis.browser,globalThis.chrome].find(value=>value?.runtime?.id&&value?.storage?.local);runtimeIdentity=api?.runtime?.id;}catch{return;}
 if(!api||!runtimeIdentity)return; // A reload can schedule injection after the old extension API is invalidated.
 if(globalThis.__udmPanelsV2)return;globalThis.__udmPanelsV2=true;
 const replaceEvent='udm-panel-owner-replaced-v1';document.dispatchEvent(new Event(replaceEvent));
 try{if(api.runtime?.id!==runtimeIdentity){globalThis.__udmPanelsV2=false;return;}}catch{globalThis.__udmPanelsV2=false;return;}
 const panels=new Map(),abort=new AbortController(),options={signal:abort.signal},site=location.origin;
 let disposed=false;
 const contextAlive=()=>{try{return !runtimeIdentity||api.runtime.id===runtimeIdentity;}catch{return false;}};
 let tabDisabled=false,preferences={},excluded=[],integration={},scheduled=false,scanTimer=0,lastPolicy=0,pointer=null;
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
   const rootStyle=getComputedStyle(document.documentElement);
   const viewportOverflow=node===document.documentElement||(node===document.body&&rootStyle.overflowX==='visible'&&rootStyle.overflowY==='visible'&&rootStyle.contain==='none'&&style.contain==='none');
   if(viewportOverflow)continue;
   const paint=/paint|strict|content/.test(style.contain),x=paint||/hidden|clip|scroll|auto/.test(style.overflowX),y=paint||/hidden|clip|scroll|auto/.test(style.overflowY);
   if(x||y){const r=node.getBoundingClientRect(),sx=node.offsetWidth?r.width/node.offsetWidth:1,sy=node.offsetHeight?r.height/node.offsetHeight:1,left=r.left+node.clientLeft*sx,top=r.top+node.clientTop*sy;clips.push({left,top,right:left+node.clientWidth*sx,bottom:top+node.clientHeight*sy,x,y});}
   if(node===document.fullscreenElement)break;
  }
  watchGeometry(p,nodes);return {visible,clips};
 }
 const youtubeId=()=>{const u=new URL(location.href);return /(^|\.)youtube(?:-nocookie)?\.com$/.test(u.hostname)?u.searchParams.get('v')||/^\/(?:embed|shorts)\/([\w-]{11})/.exec(u.pathname)?.[1]||'':'';};
 function element(tag,text,className){const e=document.createElement(tag);if(text)e.textContent=text;if(className)e.className=className;return e;}
 const css=':host{all:initial;position:fixed!important;display:block;font:11px Tahoma,Arial,sans-serif!important;color:#162736!important;z-index:2147483000!important;pointer-events:auto!important;color-scheme:light!important}*{box-sizing:border-box}button,select{font:11px Tahoma,Arial,sans-serif;color:#162736}button{cursor:pointer}button:focus-visible,select:focus-visible{outline:2px solid #267dd2;outline-offset:1px}.bar{height:24px;display:flex;align-items:center;border:1px solid #8facc3;border-radius:3px;background:linear-gradient(#fff,#dcebf6);box-shadow:0 1px 4px #0004;white-space:nowrap;overflow:hidden}.toggle{display:flex;gap:5px;align-items:center;border:0;background:none;height:22px;padding:0 7px;flex:1;min-width:0}.toggle:hover{background:#c7e5fa}.icon{width:14px;height:14px;background:#2678b9;color:#fff;border-radius:2px;text-align:center;font:bold 13px Arial}.caption{overflow:hidden;text-overflow:ellipsis}.arrow{margin-left:auto;font-size:9px}.grip{width:11px;height:22px;border:0;background:none;color:#668297;cursor:move;padding:0;touch-action:none}.mini .caption,.mini .arrow,.mini .grip{display:none}.mini .toggle{padding:0 7px}.menu{position:absolute;right:0;top:27px;width:420px;max-width:calc(100vw - 8px);padding:7px;display:flex;flex-direction:column;overflow:hidden;border:1px solid #9bacba;background:#fff;border-radius:3px;box-shadow:0 3px 12px #0004}.head{display:flex;align-items:center;gap:5px;margin-bottom:5px;flex:none}.head strong{flex:1;font-size:12px}.small{border:1px solid #bccbd6;background:#f4f8fc;border-radius:2px;padding:2px 6px}.choices{min-height:28px;overflow:auto;flex:1 1 auto;overscroll-behavior:contain}.choice{display:block;width:100%;text-align:left;border:0;border-bottom:1px solid #e5ebf0;background:#fff;padding:7px 6px;line-height:1.35;overflow-wrap:anywhere}.choice:hover{background:#e6f2fc}.source{display:block;color:#687f90;font-size:10px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}.status{font-size:11px;line-height:1.4;margin:7px 0 2px;overflow-wrap:anywhere;max-height:52px;overflow:auto;flex:none}.foot{display:flex;gap:6px;margin-top:5px;flex:none;flex-wrap:wrap}.bar>.small{width:17px;height:20px;padding:0;margin-right:2px;flex:none;font-size:13px}.menu{top:25px;width:max-content;min-width:300px;max-width:min(580px,calc(100vw - 8px));padding:5px;border-radius:0;font-size:12px}.head{display:none}.head.compact{display:flex;justify-content:flex-end}.head strong{display:none}.choice{font-size:12px;padding:5px 12px;line-height:1.35;border:0;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}.choice:focus-visible{outline-offset:-2px}.status:empty{display:none}.foot{padding-top:4px;border-top:1px solid #ddd}.status{padding:0 7px}.choices{min-height:0}@media(prefers-color-scheme:dark){.menu{background:#2b2b2b;color:#eee;border-color:#666}.menu button{color:#eee;background:#2b2b2b}.choice:hover,.choice:focus-visible{background:#454545}.foot{border-color:#555}.foot button{border-color:#666}.menu .status{color:#ddd}}[hidden]{display:none!important}';
 async function save(){try{await api.storage.local.set({videoPanelSites:preferences});}catch{}}
 function updateSettings(data){
  const reset=Number(data.desktopPolicy?.panelReset||0),was=Number(data.videoPanelSites?.__reset||0);
  integration=UdmMedia.policy.merge(data.settings||{},data.desktopPolicy||{});excluded=integration.excluded;
  if(data.videoPanelSites)preferences=data.videoPanelSites;
  if(reset>was){preferences={__reset:reset};for(const p of panels.values()){p.offset={x:0,y:0};p.hidden=false;}save();}
  scan();
 }
 async function settings(){try{updateSettings(await api.storage.local.get(['settings','desktopPolicy','videoPanelSites']));}catch{}}
 function refreshPolicy(){if(!api.runtime.id||Date.now()-lastPolicy<30000)return;lastPolicy=Date.now();api.runtime.sendMessage({action:'integration-state'}).then(r=>{if(r?.ok){tabDisabled=!!r.disabled;schedule();}}).catch(()=>{});}
 const storageChanged=(changes,area)=>{if(area==='local'&&(changes.settings||changes.desktopPolicy||changes.videoPanelSites))settings();};
 api.storage.onChanged?.addListener(storageChanged);
 document.addEventListener('click',event=>{
  if(!event.isTrusted||event.button!==0||!api.runtime.id)return;const intent=UdmMedia.policy.intent({...keyState,altKey:event.altKey,ctrlKey:event.ctrlKey,shiftKey:event.shiftKey,metaKey:event.metaKey},integration);if(!intent)return;
  const anchor=event.composedPath().find(e=>e?.tagName==='A'&&e.href);if(tabDisabled||!anchor||UdmMedia.policy.blocked(location.href,integration)||UdmMedia.policy.blocked(anchor.href,integration))return;
  api.runtime.sendMessage({action:'capture-intent',url:anchor.href,intent}).catch(()=>{});
 },{...options,capture:true});
 // Never forward typed text or synthetic page events. Only modifier/Insert/Delete state is kept.
 let keyState={},keyTimer=0;
 const sendKeys=()=>{if(!contextAlive())return;const intent=UdmMedia.policy.intent(keyState,integration);void api.runtime.sendMessage({action:'capture-keys',keys:intent?keyState:null}).catch(()=>{});};
 const clearKeys=()=>{keyState={};clearInterval(keyTimer);keyTimer=0;sendKeys();};
 const capturePlayers=()=>{if(!contextAlive()||tabDisabled||!integration.capture||integration.captureAllowed===false)return;const intent=UdmMedia.policy.intent(keyState,integration);if(intent==='bypass'||(!integration.captureWebPlayers&&!(intent==='force'&&integration.forceClick===false)))return;
  for(const [video,p] of panels)if(!video.paused&&!video.ended&&!video.mediaKeys&&!video.hasAttribute('data-udm-encrypted'))void api.runtime.sendMessage({action:'capture-player',token:video.getAttribute('data-udm-player')}).catch(()=>{});
 };
 for(const eventName of ['keydown','keyup'])document.addEventListener(eventName,event=>{
  if(!event.isTrusted)return;if(event.composedPath().some(e=>e?.matches?.('input,textarea,select,[contenteditable]:not([contenteditable="false"])'))){clearKeys();return;}
  keyState={...keyState,altKey:event.altKey,ctrlKey:event.ctrlKey,shiftKey:event.shiftKey,metaKey:event.metaKey};if(event.key==='Insert')keyState.insertKey=eventName==='keydown';if(event.key==='Delete')keyState.deleteKey=eventName==='keydown';
  sendKeys();clearInterval(keyTimer);keyTimer=0;if(UdmMedia.policy.intent(keyState,integration)){keyTimer=setInterval(()=>{if(!document.hasFocus()||document.visibilityState!=='visible'){clearKeys();return;}sendKeys();capturePlayers();},500);capturePlayers();}
 },{...options,capture:true});
 addEventListener('blur',clearKeys,options);document.addEventListener('visibilitychange',()=>{if(document.visibilityState!=='visible')clearKeys();},options);
 document.addEventListener('playing',capturePlayers,{...options,capture:true});
 // Bound the whole extension message, including worker startup and delivery.
 // An uncertain download acknowledgement must never cause an automatic resend.
 function requestPanelMessage(message,download=false){
  let timer;
  const timeout=download?65000:12000;
  const hint=download?'UDM has not confirmed this download. Check its download list before trying again.':'UDM could not read the formats in time. Refresh this page and try again.';
  return Promise.race([Promise.resolve().then(()=>api.runtime.sendMessage(message)),new Promise((_,reject)=>{timer=setTimeout(()=>reject(Error(hint)),timeout);})])
   .catch(error=>{if(/Receiving end does not exist|Could not establish connection|Extension context invalidated/i.test(error.message))throw Error('UDM browser connection was lost. Reload UDM in Extensions, then refresh this page.');throw error;})
   .finally(()=>clearTimeout(timer));
 }
 function create(video){
  const token=crypto.randomUUID();video.setAttribute('data-udm-player',token);video.setAttribute('data-udm-epoch',String(Date.now()));
  const host=element('div');host.id='udm-video-panel-'+token;const shadow=host.attachShadow({mode:'closed'}),style=element('style');style.textContent=css;
  const bar=element('div',null,'bar'),toggle=element('button',null,'toggle'),icon=element('span','↓','icon'),caption=element('span','Download this video','caption'),arrow=element('span','▼','arrow'),grip=element('button','⋮','grip');
  toggle.title='Download this video with UDM';toggle.setAttribute('aria-label','Download this video with UDM');toggle.setAttribute('aria-expanded','false');grip.title='Drag to reposition';grip.setAttribute('aria-label','Move UDM video panel');
  toggle.append(icon,caption,arrow);bar.append(toggle,grip);
  const menu=element('div',null,'menu');menu.hidden=true;const head=element('div',null,'head'),title=element('strong','Download with UDM'),mini=element('button','−','small'),close=element('button','×','small');
  mini.title='Toggle compact icon mode';mini.setAttribute('aria-label',mini.title);close.title='Close menu';close.setAttribute('aria-label','Close download menu');head.append(title);bar.append(mini,close);
  const all=element('button','Download all','small');all.hidden=true;all.style.cssText='text-align:left;margin-bottom:4px;flex:none;border:0;border-bottom:1px solid #bbb;border-radius:0;padding:5px 12px';const choices=element('div',null,'choices'),status=element('p',null,'status'),foot=element('div',null,'foot'),refresh=element('button','Refresh','small'),reset=element('button','Reset position','small'),hide=element('button','Hide here','small');
  status.setAttribute('role','status');foot.append(refresh,reset,hide);menu.append(head,all,choices,status,foot);shadow.append(style,bar,menu);
  const p={video,host,menu,toggle,bar,head,mini,close,token,offset:{...(preferences[site]?.offset||{x:0,y:0})},loading:false,sending:false,hidden:false,lastSource:video.currentSrc,cleanup:[]};
  function change(){p.hidden=false;video.setAttribute('data-udm-epoch',String(Date.now()));menu.hidden=true;toggle.setAttribute('aria-expanded','false');choices.replaceChildren();all.hidden=true;p.lastSource=video.currentSrc;schedule();}
  function encrypted(){video.setAttribute('data-udm-encrypted','');change();}
  for(const [event,fn] of [['loadstart',change],['emptied',change],['encrypted',encrypted]]){video.addEventListener(event,fn);p.cleanup.push(()=>video.removeEventListener(event,fn));}
  async function load(){
   if(p.loading||p.sending)return;p.loading=true;all.hidden=true;choices.replaceChildren();status.textContent='Reading available video formats…';refresh.disabled=true;
   const page=location.href,stamp=video.getAttribute('data-udm-epoch'),id=youtubeId();
   try{
    if(video.mediaKeys||video.hasAttribute('data-udm-encrypted'))throw Error('This player uses encrypted or DRM-protected media.');
    const response=await requestPanelMessage(id?{action:'formats',url:'https://www.youtube.com/watch?v='+id}:{action:'site-formats',page,token});
    if(page!==location.href||stamp!==video.getAttribute('data-udm-epoch')||!host.isConnected)return;
    if(!response?.ok)throw Error(response?.error||'UDM did not respond.');
    status.textContent=response.note||(response.choices?.length||response.audioChoices?.length?'':'Play the video, then refresh.');
    async function sendChoices(selected){
     if(p.sending)return;p.sending=true;refresh.disabled=true;all.disabled=true;
     for(const b of choices.querySelectorAll('button,select'))b.disabled=true;let added=0,refreshed=0;
     try{for(const choice of selected){
      if(page!==location.href||stamp!==video.getAttribute('data-udm-epoch'))throw Error('The video changed. Refresh the list.');
      status.textContent='Sending to UDM'+(selected.length>1?' '+(added+1)+' / '+selected.length:'')+'…';
      const result=await requestPanelMessage(id?{action:choice.output==='audio'?'youtube-audio':'media',url:'https://www.youtube.com/watch?v='+id,height:choice.height,formatKey:choice.key,audioKey:choice.audioKey,title:document.title.replace(/ - YouTube$/,'')}:{action:'site-download',page,token,key:choice.key,...(choice.audioKey!==undefined?{audioKey:choice.audioKey}:{}),...(choice.subtitleKey!==undefined?{subtitleKey:choice.subtitleKey}:{}),...(choice.output?{output:choice.output}:{})},true);
      if(!result?.ok)throw Error(result?.error||'UDM did not respond.');added++;if(result.refreshPending)refreshed++;
     }status.textContent=refreshed?'Fresh streams received. Review Refresh media session in UDM.':added>1?'Added '+added+' formats. Review them in UDM.':'Added. Review Download File Info in UDM.';
     }catch(error){status.textContent=(added?'Added '+added+'. ':'')+error.message;}finally{p.sending=false;refresh.disabled=false;all.disabled=false;for(const b of choices.querySelectorAll('button,select'))b.disabled=false;}
    }
    const catalog=response.choices||[];
    function renderChoices(){
     choices.replaceChildren();all.hidden=catalog.length<2;all.textContent=catalog.some(c=>c.audioOptions?.length>1)?'Download all (default audio)':'Download all';
     all.onclick=e=>{if(e.isTrusted){e.stopPropagation();sendChoices(catalog);}};
     for(const [index,choice] of catalog.entries()){const button=element('button',(index+1)+'.  '+choice.label,'choice');button.setAttribute('aria-label',choice.label);button.title=[choice.detail||choice.label,choice.source].filter(Boolean).join(' — ');
      button.addEventListener('click',e=>{if(!e.isTrusted)return;e.stopPropagation();if(!id&&(choice.audioOptions?.length>1||choice.subtitleOptions?.length))chooseAudio(choice);else sendChoices([choice]);});choices.append(button);
     }
     const audioChoice=id?(response.audioChoices?.length?{label:'YouTube audio · M4A',audioOptions:response.audioChoices,audioOnlyAvailable:true,onlyAudio:true}:null):catalog.find(c=>c.audioOnlyAvailable);if(audioChoice){const button=element('button','Audio only (M4A)…','choice');button.addEventListener('click',e=>{if(e.isTrusted){e.stopPropagation();chooseAudio(audioChoice);}});choices.append(button);}
     schedule();
    }
    function chooseAudio(choice){
     if(p.sending)return;choices.replaceChildren();all.hidden=true;
     const name=element('p',choice.label),label=element('label','Audio track'),select=element('select'),actions=element('div',null,'foot'),back=element('button','Back','small'),download=element('button','Download video','small');
     name.style.cssText='white-space:normal;overflow-wrap:anywhere;margin:5px 7px';label.style.cssText='display:block;margin:8px 7px';select.style.cssText='display:block;width:calc(100% - 14px);margin:5px 7px;padding:5px;max-width:540px';select.setAttribute('aria-label','Audio track');label.htmlFor='udm-audio-'+token;select.id=label.htmlFor;
     let selectedDefault=false;for(const track of choice.audioOptions||[]){const option=element('option',track.label+(track.default?' — default':''));option.value=track.key;option.selected=!!track.default&&!selectedDefault;if(option.selected)selectedDefault=true;select.append(option);}
     const subtitles=element('select'),subtitleLabel=element('label','Subtitles');subtitles.setAttribute('aria-label','Subtitles');subtitles.id='udm-subtitle-'+token;subtitleLabel.htmlFor=subtitles.id;subtitleLabel.style.cssText=label.style.cssText;subtitles.style.cssText=select.style.cssText;
     const none=element('option','None');none.value='';subtitles.append(none);for(const track of choice.subtitleOptions||[]){const option=element('option',track.label);option.value=track.key;subtitles.append(option);}
     const audio=element('button','Download audio (M4A)','small');audio.addEventListener('click',e=>{if(e.isTrusted){e.stopPropagation();sendChoices([{...choice,...(select.value?{audioKey:select.value}:{}),output:'audio'}]);}});
     back.addEventListener('click',e=>{if(e.isTrusted&&!p.sending){e.stopPropagation();renderChoices();choices.querySelector('button')?.focus();}});
     download.addEventListener('click',e=>{if(e.isTrusted){e.stopPropagation();sendChoices([{...choice,...(select.value?{audioKey:select.value}:{}),subtitleKey:subtitles.value,output:'video'}]);}});
     actions.append(back);if(!choice.onlyAudio)actions.append(download);if(choice.audioOnlyAvailable)actions.append(audio);choices.append(name);if(choice.audioOptions?.length)choices.append(label,select);if(choice.subtitleOptions?.length)choices.append(subtitleLabel,subtitles);choices.append(actions);(choice.audioOptions?.length?select:download).focus();schedule();
    }
    renderChoices();
   }catch(error){status.textContent=error.message;}finally{p.loading=false;refresh.disabled=false;schedule();}
  }
  function open(value){menu.hidden=!value;toggle.setAttribute('aria-expanded',String(value));if(value)load();schedule();}
  toggle.addEventListener('click',e=>{e.stopPropagation();if(e.isTrusted)open(menu.hidden);});
  close.addEventListener('click',()=>open(false));refresh.addEventListener('click',e=>{if(e.isTrusted)load();});
  mini.addEventListener('click',()=>{open(false);toggle.focus();preferences[site]={...preferences[site],mini:!(preferences[site]?.mini??integration.panelCompact)};save();schedule();});
  reset.addEventListener('click',()=>{p.offset={x:0,y:0};preferences[site]={...preferences[site],offset:p.offset};save();status.textContent='Panel position reset.';schedule();});
  hide.addEventListener('click',()=>{p.hidden=true;host.style.setProperty('display','none','important');});
  menu.addEventListener('click',e=>e.stopPropagation());menu.addEventListener('keydown',e=>{e.stopPropagation();if(e.key==='Escape'){open(false);toggle.focus();}});
  let drag=null;grip.addEventListener('pointerdown',e=>{if(!e.isTrusted||e.button!==0)return;e.preventDefault();e.stopPropagation();drag={x:e.clientX,y:e.clientY,offset:{...p.offset}};grip.setPointerCapture(e.pointerId);});
  grip.addEventListener('pointermove',e=>{if(!drag)return;p.offset={x:drag.offset.x+e.clientX-drag.x,y:drag.offset.y+e.clientY-drag.y};schedule();});
  const stopDrag=()=>{if(!drag)return;drag=null;preferences[site]={...preferences[site],offset:p.offset};save();};grip.addEventListener('pointerup',stopDrag);grip.addEventListener('pointercancel',stopDrag);
  document.documentElement.append(host);panels.set(video,p);intersectionObserver?.observe(video);return p;
 }
 function schedule(){if(disposed||scheduled)return;scheduled=true;requestAnimationFrame(()=>{scheduled=false;layout();});}
 function layout(){
  if(disposed)return;if(!contextAlive()){dispose();return;}
  for(const [video,p] of panels){if(!video.isConnected){p.cleanup.forEach(fn=>fn());watchGeometry(p,[]);intersectionObserver?.unobserve(video);p.host.remove();panels.delete(video);continue;}
   const rect=video.getBoundingClientRect(),full=document.fullscreenElement,settings=preferences[site]||{};const compact=settings.mini??integration.panelCompact;
   const ad=video.closest('.ad-showing,.ad-interrupting,[data-ad-playing="true"]');
   const shape=geometry(p),view=globalThis.visualViewport;
   const viewport=view?{left:view.offsetLeft,top:view.offsetTop,width:view.width,height:view.height}:{width:innerWidth,height:innerHeight};
   const offset={...p.offset},corner=integration.panelPosition||'Top right',panelWidth=compact?30:206;
   if(corner.endsWith('left'))offset.x=(offset.x||0)-(rect.width-panelWidth-8);
   if(corner.startsWith('Bottom'))offset.y=(offset.y||0)+rect.height-8;
   const position=UdmMedia.placement(rect,viewport,panelWidth,24,offset,shape.clips);
   const hovered=(pointer&&pointer.x>=rect.left&&pointer.x<=rect.right&&pointer.y>=rect.top&&pointer.y<=rect.bottom)||p.host.matches(':hover')||p.host.matches(':focus-within')||!p.menu.hidden;
   const visible=!tabDisabled&&integration.panelEnabled!==false&&(!integration.panelTypes||Object.values(integration.panelTypes).some(v=>v===true))&&!(integration.panelShowProtected===false&&(video.mediaKeys||video.hasAttribute('data-udm-encrypted')))&&!UdmMedia.policy.panelBlocked(location.href,integration)&&(!integration.panelHover||hovered)&&shape.visible&&position.visible&&p.intersecting!==false&&!p.hidden&&!ad&&document.pictureInPictureElement!==video;
   // Native fullscreen on a video element has no DOM overlay surface.
   if(!visible||(full&&(full===video||!p.geometryNodes.includes(full)))){p.host.style.setProperty('display','none','important');continue;}
   const parent=full?.shadowRoot||full||document.documentElement;if(p.host.parentNode!==parent)parent.append(p.host);
   p.host.style.setProperty('display','block','important');p.host.style.setProperty('left',position.x+'px','important');p.host.style.setProperty('top',position.y+'px','important');
   p.host.style.setProperty('width',position.width+'px','important');p.bar.classList.toggle('mini',compact||position.compact);p.head.classList.toggle('compact',compact||position.compact);const controlsParent=compact||position.compact?p.head:p.bar;if(p.mini.parentNode!==controlsParent)controlsParent.append(p.mini,p.close);
   const left=viewport.left||0,top=viewport.top||0,menuWidth=Math.min(Math.max(260,Math.min(640,Number(integration.panelMenuWidth)||420)),viewport.width-8);
   const menuX=Math.max(left+4,Math.min(position.x+position.width-menuWidth,left+viewport.width-menuWidth-4));
   p.menu.style.width=menuWidth+'px';p.menu.style.left=(menuX-position.x)+'px';p.menu.style.right='auto';
   const below=top+viewport.height-position.y-31,above=position.y-top-8;
   const upward=below<280&&above>below;
   p.menu.style.maxHeight=Math.max(90,Math.min(390,upward?above:below))+'px';
   p.menu.style.top=upward?'auto':'27px';p.menu.style.bottom=upward?'27px':'auto';
  }
 }
 function scan(){scanTimer=0;if(disposed)return;if(!contextAlive()){dispose();return;}if(UdmMedia.policy.blocked(location.href,integration)){schedule();return;}refreshPolicy();
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
 document.addEventListener('pointermove',e=>{pointer={x:e.clientX,y:e.clientY};schedule();},{...options,passive:true});
 document.addEventListener('pointerdown',e=>{for(const p of panels.values())if(!e.composedPath().includes(p.host)){p.menu.hidden=true;p.toggle.setAttribute('aria-expanded','false');}},options);
 const playerCaptureTimer=setInterval(capturePlayers,2000);
 const timer=setInterval(scan,1500);
 settings().then(scan).catch(scan);
 const panelMessage=(message,sender,respond)=>{if(message.action==='tab-integration'&&(!sender.id||sender.id===api.runtime.id)){tabDisabled=!!message.disabled;schedule();respond?.({ok:true});return;}if(message.action==='restore-panels'&&(!sender.id||sender.id===api.runtime.id)){preferences={};for(const p of panels.values()){p.hidden=false;p.offset={x:0,y:0};p.menu.hidden=true;p.toggle.setAttribute('aria-expanded','false');}save();scan();respond?.({ok:true});}};api.runtime.onMessage?.addListener(panelMessage);
 function dispose(){
  if(disposed)return;disposed=true;clearInterval(playerCaptureTimer);clearKeys();clearInterval(timer);clearTimeout(scanTimer);observer.disconnect();resizeObserver?.disconnect();intersectionObserver?.disconnect();resizeTargets.clear();
  try{api.storage.onChanged?.removeListener(storageChanged);api.runtime.onMessage?.removeListener(panelMessage);}catch{}
  abort.abort();document.removeEventListener(replaceEvent,dispose);
  for(const p of panels.values()){p.cleanup.forEach(fn=>fn());p.host.remove();if(p.video.getAttribute('data-udm-player')===p.token){p.video.removeAttribute('data-udm-player');p.video.removeAttribute('data-udm-epoch');}}
  panels.clear();
 }
 document.addEventListener(replaceEvent,dispose,{once:true});
 addEventListener('pagehide',event=>{if(!event.persisted)dispose();},{signal:abort.signal});
 addEventListener('pageshow',schedule);
})();
