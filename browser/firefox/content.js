(()=>{
 'use strict';
 let api,runtimeIdentity;try{api=[globalThis.browser,globalThis.chrome].find(value=>value?.runtime?.id&&value?.storage?.local);runtimeIdentity=api?.runtime?.id;}catch{return;}
 if(!api||!runtimeIdentity)return; // A reload can schedule injection after the old extension API is invalidated.
 if(globalThis.__udmPanelsV2)return;globalThis.__udmPanelsV2=true;
 const replaceEvent='udm-panel-owner-replaced-v1';document.dispatchEvent(new Event(replaceEvent));
 try{if(api.runtime?.id!==runtimeIdentity){globalThis.__udmPanelsV2=false;return;}}catch{globalThis.__udmPanelsV2=false;return;}
 const panels=new Map(),abort=new AbortController(),options={signal:abort.signal},site=location.origin;
 let disposed=false,preparing=null,preferredPanel=null,catalogPolicyRevision=0,panelPage=location.href;
 const preparedLifetime=90000,prepareRetryDelay=4500;
 const contextAlive=()=>{try{return !runtimeIdentity||api.runtime.id===runtimeIdentity;}catch{return false;}};
 let tabDisabled=false,preferences={},excluded=[],integration={},scheduled=false,scanTimer=0,lastPolicy=0,pointer=null;
 const resizeTargets=new Map();
 const resizeObserver=typeof ResizeObserver==='function'?new ResizeObserver(schedule):null;
 const intersectionObserver=typeof IntersectionObserver==='function'?new IntersectionObserver(entries=>{for(const entry of entries){const p=panels.get(entry.target);if(p)p.intersecting=entry.isIntersecting;}schedule();},{threshold:[0,0.01,0.1,0.5,1]}):null;
 const parentElement=node=>node.parentElement||node.getRootNode()?.host||null;
 const adSelector='.ad-showing,.ad-interrupting,[data-ad-playing="true"]';
 const verifiedContentReady=()=>{const value=document.documentElement.getAttribute('data-udm-content-ready')||'',split=value.lastIndexOf(':'),time=Number(value.slice(split+1));return split>0&&value.slice(0,split)===youtubeId()&&time<=Date.now()&&time>Date.now()-300000;};
 function watchGeometry(p,nodes){
  if(p.geometryNodes?.length===nodes.length&&nodes.every((node,i)=>node===p.geometryNodes[i]))return;
  for(const node of p.geometryNodes||[]){const refs=(resizeTargets.get(node)||1)-1;if(refs)resizeTargets.set(node,refs);else{resizeTargets.delete(node);resizeObserver?.unobserve(node);}}
  p.geometryNodes=nodes;
  for(const node of nodes){const refs=resizeTargets.get(node)||0;if(!refs)resizeObserver?.observe(node);resizeTargets.set(node,refs+1);}
 }
 function geometry(p,scene){
  const nodes=[],clips=[];let visible=true,opacity=1;
  for(let node=p.video;node&&nodes.length<100;node=parentElement(node)){
   nodes.push(node);
   let style=scene.styles.get(node);
   if(!style){style=getComputedStyle(node);scene.styles.set(node,style);}
   opacity*=Number(style.opacity||1);
   if(style.display==='none'||style.visibility==='hidden'||style.visibility==='collapse'||
      style.contentVisibility==='hidden'||opacity<=0.01||(!scene.contentReady&&node.matches(adSelector)))visible=false;
   if(node===p.video)continue;
   const viewportOverflow=node===scene.root||(node===document.body&&scene.rootOverflow&&style.contain==='none');
   if(viewportOverflow)continue;
   let clip=scene.clips.get(node);
   if(clip===undefined){
    const paint=/paint|strict|content/.test(style.contain);
    const x=paint||/hidden|clip|scroll|auto/.test(style.overflowX),y=paint||/hidden|clip|scroll|auto/.test(style.overflowY);
    clip=null;
    if(x||y){
     const r=node.getBoundingClientRect(),width=node.offsetWidth,height=node.offsetHeight;
     const sx=width?r.width/width:1,sy=height?r.height/height:1;
     const left=r.left+node.clientLeft*sx,top=r.top+node.clientTop*sy;
     clip={left,top,right:left+node.clientWidth*sx,bottom:top+node.clientHeight*sy,x,y};
    }
    scene.clips.set(node,clip);
   }
   if(clip)clips.push(clip);
   if(node===scene.full)break;
  }
  watchGeometry(p,nodes);return {visible,clips};
 }
 const youtubeId=()=>{const u=new URL(location.href);return /(^|\.)youtube(?:-nocookie)?\.com$/.test(u.hostname)?u.searchParams.get('v')||/^\/(?:embed|shorts)\/([\w-]{11})/.exec(u.pathname)?.[1]||'':'';};
 function element(tag,text,className){const e=document.createElement(tag);if(text)e.textContent=text;if(className)e.className=className;return e;}
 const css=`
:host{all:initial;position:fixed!important;display:none;font:12px Tahoma,Arial,sans-serif!important;color:#111!important;z-index:2147483000!important;pointer-events:auto!important;color-scheme:light!important}
*{box-sizing:border-box}button,select{font:inherit;color:inherit}button{cursor:pointer}button:focus-visible,select:focus-visible{outline:1px dotted #263c60;outline-offset:-2px}
 ${UdmMedia.classicPanelCss}
.grip{position:absolute;inset:1px auto auto 1px;width:7px;height:18px;padding:0;border:0;background:transparent;cursor:move;touch-action:none}
.menu{position:absolute;display:flex;flex-direction:column;overflow:hidden;width:max-content;min-width:min(178px,var(--menu-width-limit));max-width:var(--menu-width-limit);padding:2px;border:1px solid #878787;background:#fff;color:#111;box-shadow:2px 2px 4px #0006;font:12px/1.25 Tahoma,Arial,sans-serif}
.menu.awaiting-formats{display:none}
.toggle[aria-busy="true"]{background:linear-gradient(#9bb8d7,#d7e8fa);cursor:progress}
.panel-announcement{position:absolute;width:1px;height:1px;overflow:hidden;clip-path:inset(50%);white-space:nowrap;pointer-events:none}
.menu.compact{background:#efefef;border-color:#b5b5b5}
.menu.compact:not(.tool-menu){background:linear-gradient(90deg,#e8e8e8 0 26px,#efefef 26px);box-shadow:inset 0 0 0 1px #fafafa,2px 2px 4px #0006}
.choices{min-height:0;overflow:auto;overscroll-behavior:contain}.choice-row{display:flex;align-items:stretch}.choice-row>.choice{flex:1;min-width:0;width:auto}
.choice,.format-options,.command{border:0;background:#fff;color:#111;height:17px;padding:0 15px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.choice,.command{display:block;width:100%;text-align:left}.format-options{flex:none;width:22px;padding:0;font-weight:bold}
.choice:hover,.choice:focus-visible,.command:hover,.command:focus-visible,.format-options:hover,.format-options:focus-visible{background:#bcbeca}
.command{flex:none;border-bottom:1px solid #b2b2b2;height:20px;margin-bottom:3px}
.choice.other-qualities{border-top:1px solid #b2b2b2;margin-top:3px;height:22px}
.item-title{display:none}.choice-title{white-space:nowrap}
.compact .choice,.compact .command,.compact .format-options{background:transparent;height:22px}
.compact .choice,.compact .command{padding-left:30px;padding-right:14px}
.compact .command{height:25px;border-bottom-color:#cfcfcf;margin-bottom:3px}
.compact .choice-title{display:flex;align-items:center;gap:4px;padding-right:clamp(14px,calc((var(--menu-width-limit) - 260px) / 4),40px)}
.compact .item-number{flex:none}
.compact .item-title{display:block;overflow:hidden;text-overflow:ellipsis;min-width:0}
.compact .item-format{overflow:hidden;text-overflow:ellipsis;flex-shrink:0;max-width:70%}
.compact .choice:hover,.compact .choice:focus-visible,.compact .command:hover,.compact .command:focus-visible,.compact .format-options:hover,.compact .format-options:focus-visible{background:#72bef1}
.status{max-width:330px;max-height:46px;overflow:auto;overflow-wrap:anywhere;margin:3px 0;padding:0 7px;flex:none}.status:empty{display:none}
.foot{display:flex;gap:3px;padding:3px 4px;flex:none;flex-wrap:wrap;border-top:1px solid #c5c5c5}
.panel-tools{min-height:0;overflow:auto;overscroll-behavior:contain}
.panel-tools .tool{height:22px;padding-inline:15px;background:transparent}
.panel-tools .tool:hover,.panel-tools .tool:focus-visible{background:#bcbeca}
.compact .panel-tools .tool:hover,.compact .panel-tools .tool:focus-visible{background:#72bef1}
.tool-menu > :not(.panel-tools){display:none}
.small{border:1px solid #9b9b9b;background:linear-gradient(#fff,#e6e6e6);padding:1px 5px}
.quality-help{width:min(392px,calc(100vw - 24px));max-height:calc(100dvh - 24px);padding:0;border:1px solid #8297ad;background:#fff;color:#111;box-shadow:2px 3px 8px #0005}
.quality-help::backdrop{background:transparent}
.quality-help h2{margin:0;padding:6px 10px;background:#edf4fc;font:inherit;font-weight:bold}
.quality-help p{margin:0;padding:16px;line-height:1.4}
.quality-help form{margin:0;padding:8px 12px;background:#f0f0f0;text-align:right}
.quality-help button{min-width:72px;padding:3px 8px}
[hidden]{display:none!important}
`;
 async function save(){try{await api.storage.local.set({videoPanelSites:preferences});}catch{}}
 const catalogPolicy=prefs=>JSON.stringify([prefs.panelEnabled,prefs.panelTypes,prefs.panelMinKb,
  prefs.panelExcluded,prefs.excluded,prefs.excludedUrls,prefs.cookies]);
 function updateSettings(data){
  const reset=Number(data.desktopPolicy?.panelReset||0),was=Number(data.videoPanelSites?.__reset||0);
  const previousPolicy=catalogPolicy(integration);
  integration=UdmMedia.policy.merge(data.settings||{},data.desktopPolicy||{});excluded=integration.excluded;
  if(previousPolicy!==catalogPolicy(integration)){
   catalogPolicyRevision++;
   for(const panel of panels.values())panel.invalidatePrepared(true);
  }
  if(data.videoPanelSites)preferences=data.videoPanelSites;
  if(reset>was){preferences={__reset:reset};for(const p of panels.values()){p.offset={x:0,y:0};p.hidden=false;}save();}
  queueScan(0);
 }
 async function settings(){try{updateSettings(await api.storage.local.get(['settings','desktopPolicy','videoPanelSites']));}catch{queueScan(0);}}
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
 document.addEventListener('playing',event=>{
  // A newly inserted player can start before the mutation scan runs.
  if(event.target?.localName==='video'&&!panels.has(event.target))queueScan(0);
  capturePlayers();
 },{...options,capture:true});
 // Bound the whole extension message, including worker startup and delivery.
 // An uncertain download acknowledgement must never cause an automatic resend.
 function requestPanelMessage(message,download=false){
  let timer;
  const timeout=download?65000:12000;
  const hint=message.action==='site-other-qualities'?'UDM did not confirm the guidance request. Check for the desktop message box.':
   message.action==='site-video-batch'?'UDM has not confirmed the batch review. Check for the desktop batch dialog before trying again.':
   download?'UDM has not confirmed this download. Check its download list before trying again.':'UDM could not read the formats in time. Refresh this page and try again.';
  return Promise.race([Promise.resolve().then(()=>api.runtime.sendMessage(message)),new Promise((_,reject)=>{timer=setTimeout(()=>reject(Error(hint)),timeout);})])
   .catch(error=>{if(/Receiving end does not exist|Could not establish connection|Extension context invalidated/i.test(error.message))throw Error('UDM browser connection was lost. Reload UDM in Extensions, then refresh this page.');throw error;})
   .finally(()=>clearTimeout(timer));
 }
 function create(video){
  const token=crypto.randomUUID();video.setAttribute('data-udm-player',token);video.setAttribute('data-udm-epoch',token+':0');
  const host=element('div');host.id='udm-video-panel-'+token;const shadow=host.attachShadow({mode:'closed'}),style=element('style');style.textContent=css;
  const bar=element('div',null,'bar'),toggle=element('button',null,'toggle'),icon=element('span',null,'icon'),caption=element('span','Download this video','caption'),grip=element('button',null,'grip');
  toggle.title='Download this video with UDM';toggle.setAttribute('aria-label','Download this video with UDM');toggle.setAttribute('aria-expanded','false');grip.title='Drag to reposition';grip.setAttribute('aria-label','Move UDM video panel');
  toggle.append(icon,caption);bar.append(toggle,grip);
  const menu=element('div',null,'menu');menu.hidden=true;const help=element('button','?','caption-button'),close=element('button','×','caption-button');
  help.title='Video panel options';help.setAttribute('aria-label',help.title);help.setAttribute('aria-expanded','false');bar.append(help,close);
  const all=element('button','Download all','command');all.hidden=true;
  const choices=element('div',null,'choices'),status=element('p',null,'status'),tools=element('div',null,'panel-tools'),toolStatus=element('p',null,'status');
  const announcement=element('span',null,'panel-announcement');
  announcement.setAttribute('role','status');
  const refresh=element('button','Refresh','choice tool'),reset=element('button','Reset position','choice tool'),mini=element('button','Compact mode','choice tool'),hide=element('button','Hide here','choice tool');
  const qualityHelp=element('dialog',null,'quality-help'),qualityTitle=element('h2','Other qualities'),qualityText=element('p','Choose a different quality in the web player and let it play. Open the UDM video menu again to see the formats captured for this player.');
  qualityTitle.id='udm-quality-title';qualityText.id='udm-quality-text';
  qualityHelp.setAttribute('aria-labelledby',qualityTitle.id);qualityHelp.setAttribute('aria-describedby',qualityText.id);
  const dismissHelp=element('form'),ok=element('button','OK','small');
  dismissHelp.method='dialog';dismissHelp.append(ok);qualityHelp.append(qualityTitle,qualityText,dismissHelp);
  qualityHelp.addEventListener('keydown',e=>e.stopPropagation());qualityHelp.addEventListener('click',e=>e.stopPropagation());
  mini.setAttribute('aria-label','Toggle compact icon mode');tools.hidden=true;
  status.setAttribute('role','status');toolStatus.setAttribute('role','status');tools.append(refresh,reset,mini,hide,toolStatus);menu.append(all,choices,status,tools);shadow.append(style,bar,announcement,menu,qualityHelp);
  const sourceKey=()=>JSON.stringify([video.currentSrc,video.src,video.poster]);
  const p={video,host,menu,choices,tools,toggle,bar,help,qualityHelp,token,offset:{...(preferences[site]?.offset||{x:0,y:0})},generation:0,menuRevision:0,catalogRevision:0,metadataRefresh:false,previousWarning:'',loading:false,sending:false,hidden:false,lastSource:sourceKey(),cleanup:[]};
  // Keep the opening intent until there is a list or a useful error to show.
  const formatsOpen=()=>!menu.hidden&&tools.hidden;
  const awaitingFormats=()=>menu.classList.contains('awaiting-formats');
  function setFormatsPending(value){
   menu.classList.toggle('awaiting-formats',value);
   toggle.setAttribute('aria-expanded',String(formatsOpen()&&!value));
   if(value){
    toggle.setAttribute('aria-busy','true');
    announcement.textContent='Checking video formats.';
   }else{
    toggle.removeAttribute('aria-busy');
    announcement.textContent='';
   }
  }
  const protectedVideo=()=>!!video.mediaKeys||video.hasAttribute('data-udm-encrypted');
  const catalogScope=()=>JSON.stringify([location.href,video.currentSrc,video.src,video.poster,
   p.generation,protectedVideo(),catalogPolicyRevision]);
  const preparedFor=scope=>p.prepared?.scope===scope&&Date.now()-p.prepared.time<preparedLifetime?p.prepared.result:null;
  p.preparedCatalog=()=>preparedFor(catalogScope());
  p.catalogEvicted=keys=>{
   const matches=choices=>choices?.some(choice=>keys.includes(choice.key));
   if(matches(p.prepared?.result.choices))p.prepared=null;
   if(formatsOpen()&&!p.sending&&matches(p.visibleChoices))open(false);
  };
  function sendCatalogCancel(request){
   if(!request||!contextAlive())return;
   void Promise.resolve().then(()=>api.runtime.sendMessage({...request,action:'site-formats-cancel'})).catch(()=>{});
  }
  function cancelCatalog(){
   if(!p.loading&&!p.siteRequest)return;
   const request=p.siteRequest;
   p.siteRequest=null;p.receiveCatalog=null;p.presentCatalog=null;p.catalogRevision++;p.metadataRefresh=false;p.loading=false;
   if(preparing===p)preparing=null;
   refresh.disabled=p.sending;
   sendCatalogCancel(request);
  }
  p.cleanup.push(cancelCatalog);
  p.invalidatePrepared=(reload=false)=>{
   p.prepared=null;
   if(reload||preparing===p)cancelCatalog();
   if(reload&&formatsOpen())void load();
  };
  p.cancelPreparation=()=>{if(menu.hidden)cancelCatalog();};
  p.prepareCatalog=(intent=false)=>{
   if((preparing&&(!intent||!preparing.menu.hidden))||p.loading||p.sending||!menu.hidden||document.visibilityState!=='visible'||
      video.readyState<1||protectedVideo())return;
   if((video.paused||video.ended)&&!intent)return;
   const scope=catalogScope();
   if(preparedFor(scope))return;
   if(p.prepareScope!==scope){p.prepareScope=scope;p.prepareAttempts=0;p.prepareAfter=0;}
   if(p.prepareAttempts>=2||Date.now()<p.prepareAfter)return;
   preparing?.cancelPreparation();
   p.prepareAttempts++;
   p.prepareAfter=Date.now()+prepareRetryDelay;
   void load(false,true);
  };
  // A paused player with loaded metadata can start preparing on strip intent.
  const preferPanel=event=>{if(event.isTrusted){preferredPanel=p;schedule();}};
  bar.addEventListener('pointerenter',preferPanel);
  bar.addEventListener('focusin',preferPanel);
  p.capturedSourceReady=()=>{
   if(tabDisabled||document.hidden||!video.isConnected||p.hidden||p.sending||protectedVideo()||
      integration.panelEnabled===false||UdmMedia.policy.panelBlocked(location.href,integration))return;
   const scope=catalogScope(),opened=formatsOpen();
   if(preparedFor(scope)||(opened&&choices.childElementCount)||p.observedScope===scope||
      (!p.loading&&!opened&&p.prepareScope!==scope))return;
   // Allow one capture-driven retry for this player identity, even after the
   // time-based attempts were exhausted. Repeated live playlist updates are cheap.
   p.observedScope=scope;
   if(p.prepareScope===scope){
    p.prepareAfter=0;
    p.prepareAttempts=Math.min(p.prepareAttempts,1);
   }
   if(p.loading)p.metadataRefresh=true;
   else if(opened)void load();
  };
  function resetSource(closeFormats){
   qualityHelp.close();
   if(closeFormats||awaitingFormats())open(false);
   else cancelCatalog();
   p.prepared=null;p.keyboardFocus=null;p.formatPointer=undefined;p.afterFormatPress=null;
   p.hidden=false;p.generation++;p.metadataRefresh=false;p.loading=false;
   refresh.disabled=p.sending;
   video.setAttribute('data-udm-epoch',token+':'+p.generation);
   choices.replaceChildren();all.hidden=true;p.lastSource=sourceKey();
   if(formatsOpen())void load();
   schedule();
  }
  function change(){resetSource(sourceKey()!==p.lastSource);}
  function encrypted(){video.setAttribute('data-udm-encrypted','');resetSource(true);}
  p.resetForNavigation=()=>resetSource(true);
  p.checkSource=()=>{
   if((p.loading||p.prepared||p.hidden||!menu.hidden)&&sourceKey()!==p.lastSource)resetSource(true);
  };
  function metadataReady(){
   schedule();
   if(formatsOpen()&&!p.sending&&!choices.childElementCount){
    if(p.loading)p.metadataRefresh=true;
    else void load();
   }
  }
  const playerEvents=[['loadstart',change],['emptied',change],['encrypted',encrypted],
   ['play',schedule],['playing',schedule],['loadedmetadata',metadataReady]];
  for(const [event,fn] of playerEvents){
   video.addEventListener(event,fn);
   p.cleanup.push(()=>video.removeEventListener(event,fn));
  }
  async function load(pickSingle=false,prepare=false){
   if(p.loading||p.sending)return;
   p.lastSource=sourceKey();
   const scope=catalogScope(),cached=preparedFor(scope);
   if(!cached)p.prepared=null;
   p.loading=true;
   if(prepare)preparing=p;
   refresh.disabled=true;
   const showPending=()=>{
    all.hidden=true;
    choices.replaceChildren();
    p.visibleChoices=null;
    status.textContent=p.previousWarning;
    setFormatsPending(!p.previousWarning);
   };
   if(!prepare&&!cached)showPending();
   const generation=p.generation,revision=++p.catalogRevision,page=location.href,id=youtubeId();
   const current=()=>revision===p.catalogRevision&&scope===catalogScope()&&video.isConnected&&host.isConnected;
   const siteRequest=id||cached?null:{action:'site-formats',page,token,progressId:crypto.randomUUID(),...(prepare?{prepareOnly:true}:{})};
   const marker=/^#udm-stream-refresh=[0-9a-f]{32}\.[av]\d{1,6}$/.test(location.hash)?location.hash:'';
   const youtubeUrl=id?'https://www.youtube.com/watch?v='+id+marker:'';
   let response,sequence=0,choosingAudio=false,handoffWarmed=false,capacityDeferred=false;
   try{
    if(protectedVideo())throw Error('This player uses encrypted or DRM-protected media.');
    p.siteRequest=siteRequest;
    function showStatus(){
     setFormatsPending(false);
     const note=response.note||(response.choices?.length||response.audioChoices?.length||response.captionChoices?.length?'':'Play the video, then refresh.');
     status.textContent=[p.previousWarning,note].filter(Boolean).join(' ');
     if(response.moreFormats)announcement.textContent='Checking additional video formats.';
    }
    function rememberCatalog(result){
     if(result.moreFormats||!(result.choices?.length||result.audioChoices?.length||result.captionChoices?.length))return;
     p.prepared={scope,result,time:Date.now()};
     p.prepareAttempts=0;
    }
    if(prepare)p.presentCatalog=(selectSingle)=>{
     if(!current()){cancelCatalog();void load(selectSingle);return;}
     pickSingle=selectSingle&&!response;
     if(siteRequest?.prepareOnly){
      siteRequest.prepareOnly=false;
      void Promise.resolve().then(()=>api.runtime.sendMessage({...siteRequest,action:'site-formats-priority'})).catch(()=>{});
     }
     if(response){showStatus();renderChoices();focusRequestedFormat();}
     else showPending();
    };
    async function sendChoices(selected,reviewBatch=false){
     if(p.sending)return;
     if(!video.isConnected)return;
     if(scope!==catalogScope()){cancelCatalog();open(true);return;}p.sending=true;p.selectedCatalog=revision;p.sendGeneration=generation;p.previousWarning='';refresh.disabled=true;all.disabled=true;
     const single=selected.length===1&&!reviewBatch;
     if(single){
      const returnFocus=menu.contains(shadow.activeElement);
      open(false);
      if(returnFocus)toggle.focus({preventScroll:true});
     }
     const handoffRevision=p.menuRevision;
     for(const b of choices.querySelectorAll('button,select'))b.disabled=true;let added=0,refreshed=0,recovered=0;
     status.textContent='';
     const pending=single?null:setTimeout(()=>{
      if(generation===p.generation&&host.isConnected)status.textContent='Preparing the selected video…';
     },350);
     function feedback(message){
      if(generation!==p.generation||page!==location.href||!host.isConnected){p.previousWarning='Previous video: '+message;return;}
      status.textContent=message;
      // Keep feedback for this selection without overriding a later close or tab toggle.
      if(single&&!formatsOpen()){
       if(menu.hidden&&p.menuRevision===handoffRevision&&!p.hidden&&!tabDisabled&&host.getClientRects().length)open(true);
       else p.previousWarning=message;
      }
      schedule();
     }
     try{
      if(reviewBatch){
       const batch=id?{youtubeChoices:selected.map(({key,height})=>({key,height}))}:{keys:selected.map(choice=>choice.key)};
       const result=await requestPanelMessage({action:'site-video-batch',page,token,...batch},true);
       const sameMenu=generation===p.generation&&p.menuRevision===handoffRevision&&formatsOpen();
       if(result?.ok&&result.count===selected.length){
        if(sameMenu){
         const returnFocus=menu.contains(shadow.activeElement);
         open(false);
         if(returnFocus)toggle.focus({preventScroll:true});
        }
        return;
       }
       if(result?.unsupported!==true)throw Error(result?.error||'UDM did not confirm the batch review. Check for the desktop batch dialog before trying again.');
       if(!sameMenu)return;
      }
      for(const choice of selected){
       if(scope!==catalogScope())throw Error('The video changed. Refresh the list.');
       if(added&&generation===p.generation)status.textContent='Added '+added+' / '+selected.length+' formats…';
       const action=choice.output==='caption'?'youtube-caption':choice.output==='audio'?'youtube-audio':'media';
       const result=await requestPanelMessage(id?{action,url:youtubeUrl,page,height:choice.height,formatKey:choice.key,audioKey:choice.audioKey,title:document.title.replace(/ - YouTube$/,'')}:{action:'site-download',page,token,key:choice.key,...(choice.audioKey!==undefined?{audioKey:choice.audioKey}:{}),...(choice.subtitleKey!==undefined?{subtitleKey:choice.subtitleKey}:{}),...(choice.output?{output:choice.output}:{})},true);
       if(!result?.ok)throw Error(result?.error||'UDM did not respond.');added++;if(result.refreshPending)refreshed++;
       if(result.recoveredMedia)recovered++;if(result.mediaReceipt)void api.runtime.sendMessage({action:'media-receipt-ack',token:result.mediaReceipt}).catch(()=>{});
      }
      if(recovered)feedback('Previous media handoff confirmed in UDM. Select Download again only if you want another copy.');
      else if(refreshed)feedback('Fresh streams received. Review Refresh media session in UDM.');
      else if(!single&&generation===p.generation)status.textContent='Added '+added+(added===1?' format.':' formats.')+' Review them in UDM.';
     }catch(error){
      p.prepared=null;
      p.previousWarning=(added?'Added '+added+'. ':'')+error.message;
      feedback(p.previousWarning);
     }finally{clearTimeout(pending);p.sending=false;refresh.disabled=false;all.disabled=false;for(const b of choices.querySelectorAll('button,select'))b.disabled=false;if(generation!==p.generation&&formatsOpen())void load();}
    }
    function renderChoices(){
     if((prepare||cached)&&!handoffWarmed&&formatsOpen()&&
        (response.choices?.length||response.audioChoices?.length||response.captionChoices?.length)){
      handoffWarmed=true;
      void Promise.resolve().then(()=>api.runtime.sendMessage({action:'video-panel-opened',page,
       ...(response.warmPlayer===true&&youtubeUrl?{youtubeUrl}:{})})).catch(()=>{});
     }
     const catalog=[...(response.choices||[]),...(response.captionChoices||[])];
     const videoTitle=String(response.title||'').replace(/\s+/g,' ').trim().slice(0,500);
     p.visibleChoices=catalog;
     const focusedKey=shadow.activeElement?.dataset.formatKey;
     choices.replaceChildren();all.hidden=response.moreFormats===true||!catalog.length||(catalog.length<2&&!response.otherQualities);all.textContent=catalog.some(c=>c.audioOptions?.length>1)?'Download all (default audio)':'Download all';
     all.onclick=e=>{if(e.isTrusted){e.stopPropagation();sendChoices(catalog,response.videoBatch===true);}};
     for(const [index,choice] of catalog.entries()){
      const format=(choice.container||choice.label?.match(/\b(?:MP4|TS|WEBM|M4A|MKV|FLV|MOV)\b/i)?.[0]||'').toUpperCase();
      const resolution=choice.height?choice.height+'p'+(/\b\d+p HD\b/i.test(choice.label||'')?' HD':''):'original quality';
      const variation=[choice.label?.match(/\b(?:AV1|H\.264|H\.265|VP9)\b/i)?.[0],choice.label?.match(/\b\d+(?:\.\d+)? fps\b/i)?.[0],choice.originalFile||choice.label?.includes('Original file')?'original file':''].filter(Boolean);
      const label=format&&!choice.onlyAudio&&!choice.live?format+' file, '+(choice.height?'quality ':'')+resolution+(variation.length?' ('+variation.join(', ')+')':''):choice.label;
      const number=(index+1)+'.',button=element('button',null,'choice');
      button.dataset.formatKey=choice.key;
      if(videoTitle&&format&&!choice.onlyAudio&&!choice.live){
       button.classList.add('choice-title');
       button.append(element('span',number+' ','item-number'),element('span',videoTitle+',','item-title'),element('span',' '+label,'item-format'));
      }else button.textContent=number+'  '+label;
      button.setAttribute('aria-label',choice.label);button.title=[videoTitle,choice.detail||choice.label,choice.source].filter(Boolean).join(' — ');
      button.addEventListener('click',e=>{if(!e.isTrusted)return;e.stopPropagation();sendChoices([choice]);});
      if(!id&&(choice.audioOptions?.length>1||choice.subtitleOptions?.length)){
       const row=element('div',null,'choice-row'),options=element('button','…','format-options');
       options.title='Audio/subtitle options';options.setAttribute('aria-label','Audio/subtitle options for '+choice.label);
       options.addEventListener('click',e=>{if(!e.isTrusted)return;e.stopPropagation();chooseAudio(choice);});row.append(button,options);choices.append(row);
      }else choices.append(button);
     }
     const audioChoice=id?(response.audioChoices?.length?{label:'YouTube audio · M4A',audioOptions:response.audioChoices,audioOnlyAvailable:true,onlyAudio:true}:null):catalog.find(c=>c.audioOnlyAvailable&&!c.onlyAudio);if(audioChoice){const button=element('button','Audio only (M4A)…','choice');button.addEventListener('click',e=>{if(e.isTrusted){e.stopPropagation();chooseAudio(audioChoice);}});choices.append(button);}
     if(response.otherQualities===true){
      const format=catalog.find(choice=>!choice.onlyAudio);
      const button=element('button','Other qualities…','choice other-qualities');
      button.addEventListener('click',async e=>{
       if(!e.isTrusted||!format||!current()||!formatsOpen()||p.sending)return;
       e.stopPropagation();
       button.disabled=true;
       try{
        const result=await requestPanelMessage({action:'site-other-qualities',page,token,key:format.key});
        if(!current()||!formatsOpen())return;
        if(!result?.ok&&result?.unsupported!==true)throw Error(result?.error||'UDM did not confirm the guidance request.');
        open(false);toggle.focus({preventScroll:true});
        if(result.unsupported===true)qualityHelp.showModal();
       }catch(error){if(current()&&formatsOpen()){status.textContent=error.message;schedule();}}
       finally{if(!p.sending)button.disabled=false;}
      });
      choices.append(button);
     }
     if(focusedKey){
      const focused=[...choices.querySelectorAll('[data-format-key]')].find(button=>button.dataset.formatKey===focusedKey);
      (focused||choices.querySelector('button')||toggle).focus({preventScroll:true});
     }
     schedule();
    }
    function chooseAudio(choice){
     if(p.sending)return;choosingAudio=true;choices.replaceChildren();all.hidden=true;
     const name=element('p',choice.label),label=element('label','Audio track'),select=element('select'),actions=element('div',null,'foot'),back=element('button','Back','small'),download=element('button','Download video','small');
     name.style.cssText='white-space:normal;overflow-wrap:anywhere;margin:5px 7px';label.style.cssText='display:block;margin:8px 7px';select.style.cssText='display:block;width:calc(100% - 14px);margin:5px 7px;padding:5px;max-width:540px';select.setAttribute('aria-label','Audio track');label.htmlFor='udm-audio-'+token;select.id=label.htmlFor;
     let selectedDefault=false;for(const track of choice.audioOptions||[]){const option=element('option',track.label+(track.default?' — default':''));option.value=track.key;option.selected=!!track.default&&!selectedDefault;if(option.selected)selectedDefault=true;select.append(option);}
     const subtitles=element('select'),subtitleLabel=element('label','Subtitles');subtitles.setAttribute('aria-label','Subtitles');subtitles.id='udm-subtitle-'+token;subtitleLabel.htmlFor=subtitles.id;subtitleLabel.style.cssText=label.style.cssText;subtitles.style.cssText=select.style.cssText;
     const none=element('option','None');none.value='';subtitles.append(none);for(const track of choice.subtitleOptions||[]){const option=element('option',track.label);option.value=track.key;subtitles.append(option);}
     const audio=element('button','Download audio (M4A)','small');audio.addEventListener('click',e=>{if(e.isTrusted){e.stopPropagation();sendChoices([{...choice,...(select.value?{audioKey:select.value}:{}),output:'audio'}]);}});
     back.addEventListener('click',e=>{if(e.isTrusted&&!p.sending){e.stopPropagation();choosingAudio=false;renderChoices();choices.querySelector('button')?.focus();}});
     download.addEventListener('click',e=>{if(e.isTrusted){e.stopPropagation();sendChoices([{...choice,...(select.value?{audioKey:select.value}:{}),subtitleKey:subtitles.value,output:'video'}]);}});
     actions.append(back);if(!choice.onlyAudio)actions.append(download);if(choice.audioOnlyAvailable)actions.append(audio);choices.append(name);if(choice.audioOptions?.length)choices.append(label,select);if(choice.subtitleOptions?.length)choices.append(subtitleLabel,subtitles);choices.append(actions);(choice.audioOptions?.length?select:choice.onlyAudio?audio:download).focus();schedule();
    }
    function updateCatalog(next){
     const apply=()=>{
      if(!current()||p.sending||p.selectedCatalog===revision)return;
      response=next;
      if(!cached)rememberCatalog(next);
      if(!formatsOpen()||(next.moreFormats&&!(next.choices?.length||next.audioChoices?.length)))return;
      showStatus();
      if(!choosingAudio){renderChoices();focusRequestedFormat();}
     };
     if(p.formatPointer!==undefined)p.afterFormatPress=apply;
     else apply();
    }
    if(siteRequest)p.receiveCatalog=message=>{
     if(message.progressId!==siteRequest.progressId||!Number.isInteger(message.sequence)||message.sequence<=sequence||!message.result?.ok)return;
     sequence=message.sequence;
     updateCatalog(message.result);
    };
    const first=cached||await requestPanelMessage(id?{action:'formats',url:youtubeUrl,...(prepare?{prepareOnly:true}:{})}:{...siteRequest,directFirst:true});
    if(!current())return;
    if(!first?.ok)throw Error(first?.error||'UDM did not respond.');
    if(first.deferred){capacityDeferred=true;return;}
    // Only the original panel activation can select the sole confirmed offer.
    // Once a provisional list is shown, keep its selections under user control.
    if(pickSingle&&first.singleFormat===true&&first.choices?.length===1&&!first.otherQualities&&!first.moreFormats&&!sequence&&
       (!p.keyboardFocus||p.keyboardFocus==='activation')&&!p.metadataRefresh&&!p.previousWarning&&formatsOpen()&&host.getClientRects().length){
     response=first;
     if(!cached)rememberCatalog(first);
     showStatus();
     renderChoices(); // Keep the row available if the handoff reports a problem.
     void sendChoices(first.choices);
     return;
    }
    updateCatalog(first);
    if(siteRequest&&first.moreFormats&&(prepare||formatsOpen())){
     const complete=await requestPanelMessage(siteRequest);
     if(!complete?.ok)throw Error(complete?.error||'UDM did not respond.');
     if(complete.deferred){capacityDeferred=true;return;}
     updateCatalog(complete);
    }
   }catch(error){
    if(current()){
     sendCatalogCancel(p.siteRequest);
     if(formatsOpen()&&!p.sending&&p.selectedCatalog!==revision){
      setFormatsPending(false);
      status.textContent=[p.previousWarning,response?'Additional formats could not be read. '+error.message:error.message].filter(Boolean).join(' ');
     }
    }
   }finally{
    if(revision===p.catalogRevision){
     p.siteRequest=null;p.receiveCatalog=null;p.presentCatalog=null;p.loading=false;refresh.disabled=p.sending;
     if(preparing===p)preparing=null;
     const sourceChanged=scope!==catalogScope();
     const deferredOpening=capacityDeferred&&!response;
     const retry=(sourceChanged||((deferredOpening||p.metadataRefresh)&&!choices.childElementCount))&&
      formatsOpen()&&!p.sending&&video.isConnected&&host.isConnected;
     p.metadataRefresh=false;
     schedule();
     if(sourceChanged&&awaitingFormats())open(false);
     else if(retry)void load(deferredOpening&&pickSingle);
     else{
      if(capacityDeferred&&response&&formatsOpen()&&!p.sending&&p.selectedCatalog!==revision)
       status.textContent=[status.textContent,'Refresh to show the rest of the formats.'].filter(Boolean).join(' ');
      focusRequestedFormat();
     }
    }
   }
  }
  function focusRequestedFormat(){
   if(!p.keyboardFocus||awaitingFormats()||(p.loading&&!choices.childElementCount))return;
   const direction=p.keyboardFocus;p.keyboardFocus=null;
   if(!formatsOpen()||!host.isConnected||shadow.activeElement!==toggle||p.sending)return;
   const items=[all,...choices.querySelectorAll('button')].filter(item=>!item.disabled&&item.getClientRects().length);
   items[direction==='last'?items.length-1:0]?.focus();
  }
  function open(value,{showTools=false,pickSingle=false}={}){
   if(value)checkPanelPage();
   p.menuRevision++;
   if(!value||showTools){cancelCatalog();p.keyboardFocus=null;p.formatPointer=undefined;p.afterFormatPress=null;}
   const optionsOpen=value&&showTools;
   tools.hidden=!optionsOpen;toolStatus.textContent='';
   menu.classList.toggle('tool-menu',optionsOpen);menu.hidden=!value;
   help.setAttribute('aria-expanded',String(optionsOpen));
   setFormatsPending(false);
   close.title=optionsOpen?'Close panel options':value?'Close download menu':'Hide UDM video panel';close.setAttribute('aria-label',close.title);
   if(formatsOpen()){
    if(p.sending&&p.sendGeneration!==p.generation)status.textContent='Waiting for the previous video download confirmation…';
    if(p.presentCatalog)p.presentCatalog(pickSingle);
    else{
     if(preparing!==p&&!p.loading&&!p.sending&&!preparedFor(catalogScope()))preparing?.cancelPreparation();
     void load(pickSingle);
    }
   }
   schedule();
  }
  toggle.addEventListener('click',e=>{
   e.stopPropagation();
   if(!e.isTrusted)return;
   checkPanelPage();
   p.checkSource();
   const opening=!formatsOpen();
   if(opening&&e.detail===0&&shadow.activeElement===toggle)p.keyboardFocus='activation';
   open(opening,{pickSingle:e.detail<2});
  });
  function openTools(){
   open(true,{showTools:true});
   (refresh.disabled?reset:refresh).focus({preventScroll:true});
  }
  bar.addEventListener('contextmenu',e=>{
   if(!e.isTrusted||!bar.classList.contains('mini'))return;
   e.preventDefault();e.stopPropagation();openTools();
  });
  // Pointer activation leaves focus on the strip until the user navigates.
  toggle.addEventListener('keydown',e=>{
   if(e.key==='Enter'&&e.repeat){e.preventDefault();e.stopPropagation();return;}
   if(e.isTrusted&&bar.classList.contains('mini')&&(e.key==='ContextMenu'||(e.shiftKey&&e.key==='F10'))){e.preventDefault();e.stopPropagation();openTools();return;}
    if(e.isTrusted&&(e.key==='ArrowDown'||e.key==='ArrowUp')){e.preventDefault();e.stopPropagation();p.keyboardFocus=e.key==='ArrowUp'?'last':'first';if(!formatsOpen())open(true);focusRequestedFormat();return;}
    if(e.key==='Escape'&&!menu.hidden){e.preventDefault();e.stopPropagation();open(false);toggle.focus();}
  });
  function hidePanel(){open(false);p.hidden=true;host.style.setProperty('display','none','important');}
  help.addEventListener('click',e=>{if(e.isTrusted){e.stopPropagation();if(tools.hidden)openTools();else{open(false);help.focus();}}});
  close.addEventListener('click',e=>{if(e.isTrusted){e.stopPropagation();if(menu.hidden)hidePanel();else{open(false);toggle.focus();}}});
  close.title='Hide UDM video panel';close.setAttribute('aria-label',close.title);
  refresh.addEventListener('click',e=>{if(e.isTrusted){p.invalidatePrepared();toggle.focus({preventScroll:true});open(true);}});
  mini.addEventListener('click',()=>{open(false);toggle.focus();preferences[site]={...preferences[site],mini:!(preferences[site]?.mini??integration.panelCompact)};save();schedule();});
  reset.addEventListener('click',()=>{p.offset={x:0,y:0};preferences[site]={...preferences[site],offset:p.offset};save();toolStatus.textContent='Panel position reset.';schedule();});
  hide.addEventListener('click',e=>{if(e.isTrusted)hidePanel();});
  menu.addEventListener('click',e=>e.stopPropagation());menu.addEventListener('keydown',e=>{
   e.stopPropagation();
   if(e.key==='Enter'&&e.repeat){e.preventDefault();return;}
   if(e.key==='Escape'){e.preventDefault();const trigger=tools.hidden||help.hidden?toggle:help;open(false);trigger.focus();return;}
   if(!['ArrowDown','ArrowUp','Home','End'].includes(e.key)||e.target.matches?.('select,input,textarea,[contenteditable]'))return;
   const items=[...menu.querySelectorAll('button,select')].filter(item=>!item.disabled&&item.getClientRects().length);
   if(!items.length)return;const current=items.indexOf(e.target);let next;
   if(e.key==='Home')next=0;else if(e.key==='End')next=items.length-1;
   else next=(current+(e.key==='ArrowDown'?1:-1)+items.length)%items.length;
   e.preventDefault();items[next].focus();
  });
  choices.addEventListener('pointerdown',e=>{if(e.isTrusted&&e.button===0)p.formatPointer=e.pointerId;});
  let drag=null;grip.addEventListener('pointerdown',e=>{if(!e.isTrusted||e.button!==0)return;e.preventDefault();e.stopPropagation();drag={x:e.clientX,y:e.clientY,offset:{...p.offset}};grip.setPointerCapture(e.pointerId);});
  grip.addEventListener('pointermove',e=>{if(!drag)return;p.offset={x:drag.offset.x+e.clientX-drag.x,y:drag.offset.y+e.clientY-drag.y};schedule();});
  const stopDrag=()=>{if(!drag)return;drag=null;preferences[site]={...preferences[site],offset:p.offset};save();};grip.addEventListener('pointerup',stopDrag);grip.addEventListener('pointercancel',stopDrag);
  p.closeMenu=()=>{qualityHelp.close();open(false);};
  p.awaitingFormats=awaitingFormats;
  document.documentElement.append(host);panels.set(video,p);intersectionObserver?.observe(video);return p;
 }
 function schedule(){if(disposed||scheduled)return;scheduled=true;requestAnimationFrame(()=>{scheduled=false;layout();});}
 function checkPanelPage(){
  if(panelPage===location.href)return;
  panelPage=location.href;
  for(const p of panels.values())p.resetForNavigation();
  queueScan(0);
 }
 function layout(){
  if(disposed)return;if(!contextAlive()){dispose();return;}
  checkPanelPage();
  for(const [video,p] of panels){
   if(video.isConnected){p.checkSource();continue;}
   p.cleanup.forEach(fn=>fn());watchGeometry(p,[]);intersectionObserver?.unobserve(video);p.host.remove();panels.delete(video);
  }
  if(!panels.size)return;
  const root=document.documentElement,full=document.fullscreenElement,view=globalThis.visualViewport;
  const viewport=view?{left:view.offsetLeft,top:view.offsetTop,width:view.width,height:view.height}:{width:innerWidth,height:innerHeight};
  const compact=preferences[site]?.mini??integration.panelCompact,compactWidth=40;
  const corner=integration.panelPosition||'Top right',panelWidth=compact?compactWidth:206;
  const enabled=!tabDisabled&&integration.panelEnabled!==false&&
   (!integration.panelTypes||Object.values(integration.panelTypes).some(v=>v===true))&&
   !UdmMedia.policy.panelBlocked(location.href,integration);
  const placements=[];
  let scene;
  // Measure all players before moving any overlay. Shared ancestors only need one read.
  for(const [video,p] of panels){
   if(!enabled||p.hidden||p.intersecting===false||document.pictureInPictureElement===video||
      full===video||(integration.panelShowProtected===false&&(video.mediaKeys||video.hasAttribute('data-udm-encrypted')))){
    placements.push({p,eligible:false});
    continue;
   }
   if(!scene){
    const rootStyle=getComputedStyle(root);
    scene={root,full,contentReady:verifiedContentReady(),styles:new Map([[root,rootStyle]]),clips:new Map(),
     rootOverflow:rootStyle.overflowX==='visible'&&rootStyle.overflowY==='visible'&&rootStyle.contain==='none'};
   }
   const rect=video.getBoundingClientRect(),shape=geometry(p,scene);
   const position=UdmMedia.placement(rect,viewport,panelWidth,compact?18:20,p.offset,shape.clips,compactWidth,corner);
   const eligible=shape.visible&&position.visible&&(scene.contentReady||!video.closest(adSelector))&&
    (!full||p.geometryNodes.includes(full));
   const hovered=eligible&&integration.panelHover&&((pointer&&pointer.x>=rect.left&&pointer.x<=rect.right&&
    pointer.y>=rect.top&&pointer.y<=rect.bottom)||p.host.matches(':hover')||p.host.matches(':focus-within')||!p.menu.hidden);
   placements.push({p,position,eligible,hovered});
  }
  for(const {p,position,eligible,hovered} of placements){
   if(!eligible||(integration.panelHover&&!hovered)){
    p.host.style.setProperty('display','none','important');p.qualityHelp.close();
    if(!eligible&&p.awaitingFormats())p.closeMenu();
    else if(!eligible&&preparing===p&&p.menu.hidden)p.invalidatePrepared();
    continue;
   }
   const parent=full?.shadowRoot||full||root;if(p.host.parentNode!==parent)parent.append(p.host);
   p.host.style.setProperty('display','block','important');p.host.style.setProperty('left',position.x+'px','important');p.host.style.setProperty('top',position.y+'px','important');
   const mini=compact||position.compact;
   p.host.style.setProperty('width',position.width+'px','important');p.bar.classList.toggle('mini',mini);p.menu.classList.toggle('compact',mini);
   if(p.help.hidden!==mini){
    p.help.hidden=mini;
    p.toggle.title=mini?'Download this video with UDM. Right-click or press Shift+F10 for panel options.':'Download this video with UDM';
   }
   if(p.menu.hidden||p.awaitingFormats())continue;
   const left=viewport.left||0,top=viewport.top||0,maxWidth=Math.max(0,Math.min(Math.max(260,Math.min(640,Number(integration.panelMenuWidth)||490)),viewport.width-8));
   p.menu.style.setProperty('--menu-width-limit',maxWidth+'px');
   const menuWidth=p.menu.offsetWidth;
   const anchor=mini?position.x+position.width-menuWidth:position.x;
   const menuX=Math.max(left+4,Math.min(anchor,left+viewport.width-menuWidth-4));
   p.menu.style.left=(menuX-position.x)+'px';
   const menuTop=mini?15:19,below=top+viewport.height-position.y-menuTop-4,above=position.y-top-4;
   const menuList=p.tools.hidden?p.choices:p.tools;
   const menuHeight=Math.min(390,p.menu.scrollHeight+2+menuList.scrollHeight-menuList.clientHeight);
   const upward=menuHeight>below&&above>below;
   p.menu.style.maxHeight=Math.max(0,Math.min(390,upward?above:below))+'px';
   p.menu.style.top=upward?'auto':menuTop+'px';p.menu.style.bottom=upward?'100%':'auto';
  }
  // Let an open menu finish its lookup before preparing other players.
  if(placements.some(({p})=>!p.menu.hidden&&p.loading))return;
  if(preferredPanel&&(!placements.some(({p,eligible})=>p===preferredPanel&&eligible)||
     !preferredPanel.bar.matches(':hover,:focus-within')))preferredPanel=null;
  preferredPanel?.prepareCatalog(true);
  if(!preparing)for(const {p,eligible} of placements){
   if(eligible&&p!==preferredPanel)p.prepareCatalog();
   if(preparing)break;
  }
 }
 function queueScan(delay=250){
  if(disposed||document.hidden||(scanTimer&&delay!==0))return;
  clearTimeout(scanTimer);
  scanTimer=setTimeout(scan,delay);
 }
 function scan(){
  clearTimeout(scanTimer);
  scanTimer=0;
  if(disposed)return;
  if(!contextAlive()){dispose();return;}
  if(document.hidden)return;
  if(UdmMedia.policy.blocked(location.href,integration)){schedule();return;}
  refreshPolicy();
  const roots=[document],videos=[];
  for(let i=0;i<roots.length;i++){
   const root=roots[i];
   if(videos.length<30)for(const video of root.querySelectorAll('video')){
    videos.push(video);
    if(videos.length===30)break;
   }
   if(roots.length>=40)continue;
   // Walk only the bounded prefix needed for open-shadow discovery.
   const walker=document.createTreeWalker(root,NodeFilter.SHOW_ELEMENT);
   for(let n=0;n<3000&&roots.length<40;n++){
    const node=walker.nextNode();
    if(!node)break;
    if(node.shadowRoot)roots.push(node.shadowRoot);
   }
  }
  updateRoots(roots);
  for(const video of videos)if(!panels.has(video))create(video);
  schedule();
 }
 const observerOptions={childList:true,subtree:true,attributes:true,attributeFilter:['class','style','hidden','open','src','poster','data-ad-playing','data-udm-content-ready']};
 let observedRoots=[];
 const observer=new MutationObserver(records=>{
  const relevant=records.filter(r=>!r.target.closest?.('[id^="udm-video-panel-"]'));
  if(!relevant.length)return;schedule();
  if(relevant.some(r=>r.target===document.documentElement&&r.attributeName==='data-udm-content-ready')&&verifiedContentReady()){
   for(const p of panels.values())p.capturedSourceReady();
   if([...panels.values()].some(p=>p.sending)){
    try{void api.runtime.sendMessage({action:'youtube-stream-ready',url:location.href}).catch(()=>{});}catch{}
   }
  }
  if(relevant.some(r=>r.type==='childList'&&[...r.addedNodes,...r.removedNodes].some(
    node=>node.nodeType===Node.ELEMENT_NODE&&!node.id?.startsWith('udm-video-panel-'))))queueScan();
 });
 function updateRoots(roots){if(roots.length===observedRoots.length&&roots.every((r,i)=>r===observedRoots[i]))return;observer.disconnect();observedRoots=roots;for(const root of roots)observer.observe(root===document?document.documentElement:root,observerOptions);}
 updateRoots([document]);
 document.addEventListener('visibilitychange',()=>{
  if(document.hidden){
   if(preparing?.menu.hidden)preparing.invalidatePrepared();
   for(const p of panels.values())if(p.awaitingFormats())p.closeMenu();
  }else queueScan(0);
  schedule();
 },options);
 document.addEventListener('udm-video-route-changed-v1',checkPanelPage,options);
 for(const name of ['popstate','hashchange','yt-navigate-finish'])globalThis.addEventListener(name,()=>{
  checkPanelPage();
  queueScan(0);
 },options);
 for(const name of ['scroll','resize','fullscreenchange','transitionend','animationend'])globalThis.addEventListener(name,schedule,{...options,capture:true,passive:true});
 for(const name of ['resize','scroll'])globalThis.visualViewport?.addEventListener(name,schedule,{...options,passive:true});
 function updateHoverLayout(){if(integration.panelHover)schedule();}
 document.addEventListener('pointermove',e=>{pointer={x:e.clientX,y:e.clientY};updateHoverLayout();},{...options,passive:true});
 // CSS hover can move a player even when UDM's hover-only option is off.
 document.addEventListener('pointerover',event=>{
  // An existing element can attach an open shadow root without a DOM mutation.
  const targetRoot=event.composedPath()[0]?.getRootNode?.();
  if(targetRoot?.host&&!observedRoots.includes(targetRoot))queueScan(0);
  schedule();
 },{...options,passive:true});
 document.addEventListener('pointerout',e=>{if(!e.relatedTarget)pointer=null;schedule();},{...options,passive:true});
 for(const type of ['focusin','focusout'])document.addEventListener(type,updateHoverLayout,options);
 document.addEventListener('pointerdown',e=>{for(const p of panels.values())if((!p.menu.hidden||p.sending)&&!e.composedPath().includes(p.host))p.closeMenu();},options);
 for(const type of ['pointerup','pointercancel'])document.addEventListener(type,event=>{
  if(!event.isTrusted)return;
  for(const p of panels.values()){
   if(p.formatPointer!==event.pointerId)continue;
   p.formatPointer=undefined;
   const apply=p.afterFormatPress;p.afterFormatPress=null;
   if(apply)setTimeout(apply,0);
  }
 },{...options,capture:true});
 const playerCaptureTimer=setInterval(capturePlayers,2000);
 const layoutTimer=setInterval(()=>{
  if(document.hidden)return;
  if(!contextAlive()){dispose();return;}
  if(panels.size){refreshPolicy();schedule();}
 },1500);
 const discoveryTimer=setInterval(()=>queueScan(),8000);
 void settings();
 const panelMessage=(message,sender,respond)=>{
  if(message.action==='prepared-video-qualities'&&sender.id===api.runtime.id&&
     sender.url?.split('#')[0]===api.runtime.getURL('popup.html')){
   let catalog=null;
   if(window===window.top&&message.page===location.href&&youtubeId()&&!tabDisabled&&
      integration.panelEnabled!==false&&!UdmMedia.policy.panelBlocked(location.href,integration)){
    for(const p of panels.values()){
     if(p.hidden||!p.video.isConnected||!p.host.isConnected)continue;
     catalog=p.preparedCatalog();
     if(catalog)break;
    }
   }
   respond?.(catalog);
   return;
  }
  if(message.action==='site-playlist-observed'&&sender.id===api.runtime.id){
   if(!youtubeId()&&(message.page===location.href||message.page===location.origin)&&
      Number.isFinite(message.time)&&message.time>=performance.timeOrigin&&Math.abs(Date.now()-message.time)<15000){
    for(const p of panels.values())if(p.video.currentSrc.startsWith('blob:'))p.capturedSourceReady();
    schedule();
   }
   return;
  }
  if(message.action==='site-formats-progress'&&sender.id===api.runtime.id){
   for(const p of panels.values())if(p.token===message.token){p.receiveCatalog?.(message);break;}
   return;
  }
  if(message.action==='site-formats-evicted'&&sender.id===api.runtime.id&&Array.isArray(message.keys)){
   for(const p of panels.values())if(p.token===message.token){p.catalogEvicted(message.keys);break;}
   return;
  }
  if(message.action==='tab-integration'&&(!sender.id||sender.id===api.runtime.id)){tabDisabled=!!message.disabled;if(tabDisabled)for(const p of panels.values())p.closeMenu();schedule();respond?.({ok:true});return;}
  if(message.action==='restore-panels'&&(!sender.id||sender.id===api.runtime.id)){preferences={};for(const p of panels.values()){p.hidden=false;p.offset={x:0,y:0};p.closeMenu();}save();scan();respond?.({ok:true});}
 };api.runtime.onMessage?.addListener(panelMessage);
 function dispose(){
  if(disposed)return;disposed=true;clearInterval(playerCaptureTimer);clearKeys();clearInterval(layoutTimer);clearInterval(discoveryTimer);clearTimeout(scanTimer);observer.disconnect();resizeObserver?.disconnect();intersectionObserver?.disconnect();resizeTargets.clear();
  try{api.storage.onChanged?.removeListener(storageChanged);api.runtime.onMessage?.removeListener(panelMessage);}catch{}
  abort.abort();document.removeEventListener(replaceEvent,dispose);
  for(const p of panels.values()){p.cleanup.forEach(fn=>fn());p.host.remove();if(p.video.getAttribute('data-udm-player')===p.token){p.video.removeAttribute('data-udm-player');p.video.removeAttribute('data-udm-epoch');}}
  panels.clear();
 }
 document.addEventListener(replaceEvent,dispose,{once:true});
 addEventListener('pagehide',event=>{
  if(!event.persisted)dispose();
  else for(const p of panels.values())if(!p.menu.hidden||p.qualityHelp.open||preparing===p)p.closeMenu();
 },{signal:abort.signal});
 addEventListener('pageshow',()=>queueScan(0),options);
})();
