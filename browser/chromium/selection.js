(() => {
 'use strict';
 let api,runtimeIdentity;try{api=[globalThis.browser,globalThis.chrome].find(value=>value?.runtime?.id&&value?.storage?.local);runtimeIdentity=api?.runtime?.id;}catch{return;}
 if(!api||!runtimeIdentity)return; // A reload can schedule injection after the old extension API is invalidated.
 if(globalThis.__udmSelectedLinksV1)return;globalThis.__udmSelectedLinksV1=true;
 const replacement='udm-selection-owner-replaced-v1';document.dispatchEvent(new Event(replacement));
 try{if(api.runtime?.id!==runtimeIdentity){globalThis.__udmSelectedLinksV1=false;return;}}catch{globalThis.__udmSelectedLinksV1=false;return;}
 const abort=new AbortController(),signal=abort.signal;let disposed=false;
 const alive=()=>{try{return api.runtime?.id===runtimeIdentity;}catch{return false;}};
 let tabDisabled=false,host=null,shadow=null,settings={},timer=0,working=false,dismissed=null,selectingPointer=null;
 const allowed=url=>{try{const u=new URL(url,location.href);return /^https?:$/.test(u.protocol)&&!u.username&&!u.password?u.href:'';}catch{return '';}};
 const enabled=()=>{const mode=settings.selectedLinks||'all';return !tabDisabled&&mode!=='off'&&!(globalThis.UdmMedia?.policy?.blocked(location.href,settings))&&!(settings.excluded||[]).some(h=>location.hostname===h||location.hostname.endsWith('.'+h))&&(mode!=='sites'||(settings.selectedLinkHosts||[]).some(h=>location.hostname===h||location.hostname.endsWith('.'+h)));};
 function remove(){if(host)host.remove();host=shadow=null;}
 function selected(selection){if(!selection||selection.isCollapsed||!selection.rangeCount||selection.toString().length>100000)return [];const found=new Map();for(const anchor of document.querySelectorAll('a[href]')){if(found.size>=100)break;const url=allowed(anchor.href);if(!url)continue;let included=false;for(let i=0;i<selection.rangeCount&&!included;i++)try{included=selection.getRangeAt(i).intersectsNode(anchor);}catch{}if(included)found.set(url,{url,title:(anchor.textContent||'').trim().slice(0,1000)||new URL(url).pathname.split('/').pop()||url});}return [...found.values()];}
 const selectionKey=selection=>[selection?.anchorNode,selection?.anchorOffset,selection?.focusNode,selection?.focusOffset];
 const sameSelection=(key,current)=>key?.every((part,i)=>part===current[i]);
 function render(){
  if(disposed)return;if(!alive()){dispose();return;}if(!enabled()){remove();return;}if(working||selectingPointer!==null)return;
  const selection=getSelection(),links=selected(selection);
  if(!links.length){dismissed=null;remove();return;}
  const key=selectionKey(selection);
  if(sameSelection(dismissed,key)){remove();return;}
  dismissed=null;
  if(!host){
   host=document.createElement('div');host.id='udm-selected-links-panel';shadow=host.attachShadow({mode:'open'});
   host.addEventListener('mousedown',e=>e.preventDefault(),{signal});document.documentElement.append(host);
  }
  const style=document.createElement('style');
  style.textContent=`
:host{all:initial;position:fixed!important;z-index:2147483647!important;color:#111;font:12px Tahoma,Arial,sans-serif;color-scheme:light;width:min(206px,calc(100vw - 8px))}
:host([data-compact]){width:40px}
*{box-sizing:border-box}button{font:inherit;cursor:pointer}button:focus-visible{outline:1px dotted #263c60;outline-offset:-2px}
${globalThis.UdmMedia.classicPanelCss}
.bar:not(.mini){height:25px;padding:1px 1px 2px 4px;border:1px solid #9eb5cf;background:linear-gradient(#edf4fc,#d7e5f4)}
.note{position:absolute;top:100%;left:0;margin:0;width:max-content;max-width:min(206px,var(--panel-space,calc(100vw - 8px)));padding:5px 7px;background:#fff;color:#111;border:1px solid #878787;box-shadow:2px 2px 4px #0006;overflow-wrap:anywhere}
.note:empty,[hidden]{display:none!important}
`;
  const bar=document.createElement('div'),download=document.createElement('button'),icon=document.createElement('span'),caption=document.createElement('span');
  const help=document.createElement('button'),close=document.createElement('button'),status=document.createElement('p');
  const compact=!!settings.selectedLinksMini;
  host.toggleAttribute('data-compact',compact);bar.className=compact?'bar mini':'bar';
  download.className='toggle';download.title='Download selected links with UDM';download.setAttribute('aria-label',download.title);
  icon.className='icon';icon.setAttribute('aria-hidden','true');caption.className='caption';caption.textContent='Download with UDM';
  download.append(icon,caption);help.className=close.className='caption-button';help.textContent='?';close.textContent='×';
  help.setAttribute('aria-label','Selected-link panel help');help.setAttribute('aria-expanded','false');help.hidden=compact;
  close.title='Hide selected-link panel';close.setAttribute('aria-label',close.title);
  status.className='note';status.setAttribute('role','status');bar.append(download,help,close);shadow.replaceChildren(style,bar,status);
  const updateStatus=text=>{status.textContent=text;position();};
  const dismiss=()=>{dismissed=key;remove();};
  help.onclick=e=>{if(e.isTrusted){const open=help.getAttribute('aria-expanded')==='false';help.setAttribute('aria-expanded',String(open));updateStatus(open?'Review the selected links before adding them to UDM.':'');}};
  close.onclick=e=>{if(e.isTrusted)dismiss();};
  download.onclick=async e=>{
   if(!e.isTrusted||working)return;
   const current=getSelection();
   if(!sameSelection(key,selectionKey(current))){render();return;}
   const chosen=selected(current);if(!chosen.length){render();return;}
   working=true;download.disabled=true;
   try{
    const result=await api.runtime.sendMessage({action:'selected-links-review',links:chosen,page:location.href});
    if(!result?.ok)throw Error(result?.error||'UDM did not respond.');
    dismiss();
   }catch(error){updateStatus(error.message);}
   finally{working=false;download.disabled=false;if(!sameSelection(key,selectionKey(getSelection())))render();}
  };
  position();
 }
 function selectionEnd(selection){
  const caret=document.createRange();
  caret.setStart(selection.focusNode,selection.focusOffset);caret.collapse(true);
  const endpoint=caret.getBoundingClientRect();
  if(endpoint.height)return {x:endpoint.left,top:endpoint.top,bottom:endpoint.bottom};
  // Element-boundary selections can have no caret rectangle.
  const first=selection.getRangeAt(0),backward=selection.focusNode===first.startContainer&&selection.focusOffset===first.startOffset;
  const boxes=selection.getRangeAt(backward?0:selection.rangeCount-1).getClientRects();
  const box=boxes[backward?0:boxes.length-1];
  return box?{x:backward?box.left:box.right,top:box.top,bottom:box.bottom}:null;
 }
 function position(){
  if(!alive()){dispose();return;}if(!host)return;
  const selection=getSelection();if(!selection?.rangeCount||selection.isCollapsed||!selection.focusNode)return;
  const end=selectionEnd(selection),view=globalThis.visualViewport;
  const leftEdge=view?.offsetLeft??0,topEdge=view?.offsetTop??0;
  const availableWidth=view?.width??innerWidth,rightEdge=leftEdge+availableWidth,bottomEdge=topEdge+(view?.height??innerHeight);
  const visible=end&&end.x>=leftEdge&&end.x<=rightEdge&&end.bottom>topEdge&&end.top<bottomEdge;
  host.style.setProperty('visibility',visible?'visible':'hidden','important');
  if(!visible)return;
  const space=Math.max(0,availableWidth-8)+'px';
  host.style.setProperty('--panel-space',space,'important');
  host.style.setProperty('max-width',space,'important');
  const note=shadow.querySelector('.note'),width=Math.max(host.offsetWidth,note?.offsetWidth||0),height=host.offsetHeight+(note?.offsetHeight||0);
  const left=Math.max(leftEdge+4,Math.min(rightEdge-width-4,end.x-host.offsetWidth/2));
  const above=end.top-height-4,below=end.bottom+4;
  const top=above>=topEdge+4?above:Math.min(bottomEdge-height-4,below);
  host.style.setProperty('left',left+'px','important');
  host.style.setProperty('top',Math.max(topEdge+4,top)+'px','important');
 }
 function scheduleRender(delay){clearTimeout(timer);timer=setTimeout(render,delay);}
 document.addEventListener('selectionchange',()=>{
  if(selectingPointer!==null){clearTimeout(timer);remove();return;}
  scheduleRender(180);
 },{signal});
 document.addEventListener('pointerdown',event=>{
  if(event.isTrusted&&event.pointerType==='mouse'&&event.button===0&&!event.composedPath().includes(host))selectingPointer=event.pointerId;
 },{capture:true,signal});
 function endSelection(event){
  if(selectingPointer===null||!event.isTrusted)return;
  if(event.type!=='blur'&&event.pointerId!==selectingPointer)return;
  selectingPointer=null;if(!host)scheduleRender(0);
 }
 for(const name of ['pointerup','pointercancel'])addEventListener(name,endSelection,{capture:true,signal});
 addEventListener('blur',endSelection,{signal});
 document.addEventListener('pointermove',event=>{if(!(event.buttons&1))endSelection(event);},{passive:true,signal});
 addEventListener('resize',position,{signal});addEventListener('scroll',position,{signal,capture:true});
 for(const name of ['resize','scroll'])globalThis.visualViewport?.addEventListener(name,position,{signal,passive:true});
 const readSettings=async()=>{if(!alive()){dispose();return;}try{const data=await api.storage.local.get(['settings','desktopPolicy']);settings=globalThis.UdmMedia?.policy?UdmMedia.policy.merge(data.settings||{},data.desktopPolicy||{}):data.settings||{};render();}catch{if(!alive())dispose();}};readSettings();
 const integrationChanged=(message,sender)=>{if(message.action==='tab-integration'&&(!sender.id||sender.id===api.runtime.id)){tabDisabled=!!message.disabled;render();}};api.runtime.onMessage?.addListener(integrationChanged);
 api.runtime.sendMessage({action:'integration-state'}).then(r=>{if(r?.ok){tabDisabled=!!r.disabled;render();}}).catch(()=>{});
 const changed=(changes,area)=>{if(area==='local'&&(changes.settings||changes.desktopPolicy))readSettings();};api.storage.onChanged?.addListener(changed);
 function dispose(){if(disposed)return;disposed=true;clearTimeout(timer);abort.abort();document.removeEventListener(replacement,dispose);try{api.runtime?.onMessage?.removeListener(integrationChanged);api.storage?.onChanged?.removeListener(changed);}catch{}remove();}
 document.addEventListener(replacement,dispose,{once:true});
 addEventListener('pagehide',event=>{if(!event.persisted)dispose();},{signal});
})();
