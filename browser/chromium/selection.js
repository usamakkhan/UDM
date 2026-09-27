(() => {
 'use strict';
 let api,runtimeIdentity;try{api=[globalThis.browser,globalThis.chrome].find(value=>value?.runtime?.id&&value?.storage?.local);runtimeIdentity=api?.runtime?.id;}catch{return;}
 if(!api||!runtimeIdentity)return; // A reload can schedule injection after the old extension API is invalidated.
 if(globalThis.__udmSelectedLinksV1)return;globalThis.__udmSelectedLinksV1=true;
 const replacement='udm-selection-owner-replaced-v1';document.dispatchEvent(new Event(replacement));
 try{if(api.runtime?.id!==runtimeIdentity){globalThis.__udmSelectedLinksV1=false;return;}}catch{globalThis.__udmSelectedLinksV1=false;return;}
 const abort=new AbortController(),signal=abort.signal;let disposed=false;
 const alive=()=>{try{return api.runtime?.id===runtimeIdentity;}catch{return false;}};
 let tabDisabled=false,host=null,shadow=null,links=[],settings={},timer=0,working=false;
 const allowed=url=>{try{const u=new URL(url,location.href);return /^https?:$/.test(u.protocol)&&!u.username&&!u.password?u.href:'';}catch{return '';}};
 const enabled=()=>{const mode=settings.selectedLinks||'all';return !tabDisabled&&mode!=='off'&&!(globalThis.UdmMedia?.policy?.blocked(location.href,settings))&&!(settings.excluded||[]).some(h=>location.hostname===h||location.hostname.endsWith('.'+h))&&(mode!=='sites'||(settings.selectedLinkHosts||[]).some(h=>location.hostname===h||location.hostname.endsWith('.'+h)));};
 function remove(){if(host)host.remove();host=shadow=null;links=[];}
 function selected(){const selection=getSelection();if(!selection||selection.isCollapsed||!selection.rangeCount||selection.toString().length>100000)return [];const found=new Map();for(const anchor of document.querySelectorAll('a[href]')){if(found.size>=100)break;const url=allowed(anchor.href);if(!url)continue;let included=false;for(let i=0;i<selection.rangeCount&&!included;i++)try{included=selection.getRangeAt(i).intersectsNode(anchor);}catch{}if(included)found.set(url,{url,title:(anchor.textContent||'').trim().slice(0,100)||new URL(url).pathname.split('/').pop()||url});}return [...found.values()];}
 function render(){if(disposed)return;if(!alive()){dispose();return;}if(working)return;if(!enabled()){remove();return;}const next=selected();if(!next.length){remove();return;}links=next;
  if(!host){host=document.createElement('div');host.id='udm-selected-links-panel';host.style.cssText='position:fixed!important;z-index:2147483647!important;font:12px Segoe UI,Arial!important;';shadow=host.attachShadow({mode:'open'});host.addEventListener('pointerdown',e=>e.preventDefault(),{signal});document.documentElement.append(host);}
  shadow.innerHTML='<style>:host{color-scheme:light}button{font:12px Segoe UI,Arial;cursor:pointer;border:1px solid #8097a6;border-radius:4px;background:linear-gradient(#fff,#dceaf3);color:#12334c;padding:4px 8px}section{width:310px;max-height:330px;overflow:auto;background:#fff;color:#172e40;border:1px solid #8097a6;border-radius:5px;padding:9px;box-shadow:0 3px 12px #0003}label{display:flex;align-items:center;gap:5px;padding:5px 0;overflow-wrap:anywhere}input{flex:none}p{margin:7px 0}small{display:block;margin:8px 0}</style>';
  const toggle=document.createElement('button');toggle.textContent=settings.selectedLinksMini?'↓ UDM':'↓ Download selected links ('+links.length+')';toggle.title='Download selected links with UDM';toggle.setAttribute('aria-label','Download selected links with UDM');toggle.setAttribute('aria-expanded','false');shadow.append(toggle);
  const menu=document.createElement('section');menu.hidden=true;const title=document.createElement('p');title.textContent='Choose links to send to UDM';menu.append(title);const choices=[];
  for(const link of links){const row=document.createElement('label'),box=document.createElement('input'),text=document.createElement('span');box.type='checkbox';box.checked=true;text.textContent=link.title;row.title=link.url;row.append(box,text);menu.append(row);choices.push({box,link});}
  const status=document.createElement('small');status.setAttribute('role','status');const download=document.createElement('button');download.textContent='Download selected';const close=document.createElement('button');close.textContent='Close';close.style.marginLeft='8px';menu.append(status,download,close);shadow.append(menu);
  toggle.onclick=e=>{if(!e.isTrusted)return;menu.hidden=!menu.hidden;toggle.setAttribute('aria-expanded',String(!menu.hidden));position();};close.onclick=()=>{menu.hidden=true;toggle.setAttribute('aria-expanded','false');};
  download.onclick=async e=>{if(!e.isTrusted||working)return;const chosen=choices.filter(c=>c.box.checked).map(c=>c.link.url);if(!chosen.length){status.textContent='Select at least one link.';return;}working=true;download.disabled=true;status.textContent='Sending '+chosen.length+' links…';try{const result=await api.runtime.sendMessage({action:'selected-links',urls:chosen,page:location.href});if(!result?.ok)throw Error(result?.error||'UDM did not respond.');status.textContent='Added '+result.count+' links. Review them in UDM.';}catch(error){status.textContent=error.message;}finally{working=false;download.disabled=false;}};
  position();
 }
 function position(){if(!alive()){dispose();return;}if(!host)return;const selection=getSelection();let rect;try{rect=selection?.rangeCount?selection.getRangeAt(0).getBoundingClientRect():null;}catch{}if(!rect)return;const width=shadow.querySelector('section')?.hidden?(settings.selectedLinksMini?64:225):332;const height=host.getBoundingClientRect().height||30;host.style.left=Math.max(4,Math.min(innerWidth-width-4,rect.right-width))+'px';host.style.top=Math.max(4,Math.min(innerHeight-height-4,rect.bottom+5))+'px';}
 document.addEventListener('selectionchange',()=>{clearTimeout(timer);timer=setTimeout(render,180);},{signal});
 addEventListener('resize',position,{signal});addEventListener('scroll',position,{signal,capture:true});
 const readSettings=async()=>{if(!alive()){dispose();return;}try{const data=await api.storage.local.get(['settings','desktopPolicy']);settings=globalThis.UdmMedia?.policy?UdmMedia.policy.merge(data.settings||{},data.desktopPolicy||{}):data.settings||{};render();}catch{if(!alive())dispose();}};readSettings();
 const integrationChanged=(message,sender)=>{if(message.action==='tab-integration'&&(!sender.id||sender.id===api.runtime.id)){tabDisabled=!!message.disabled;render();}};api.runtime.onMessage?.addListener(integrationChanged);
 api.runtime.sendMessage({action:'integration-state'}).then(r=>{if(r?.ok){tabDisabled=!!r.disabled;render();}}).catch(()=>{});
 const changed=(changes,area)=>{if(area==='local'&&(changes.settings||changes.desktopPolicy))readSettings();};api.storage.onChanged?.addListener(changed);
 function dispose(){if(disposed)return;disposed=true;clearTimeout(timer);abort.abort();document.removeEventListener(replacement,dispose);try{api.runtime?.onMessage?.removeListener(integrationChanged);api.storage?.onChanged?.removeListener(changed);}catch{}remove();}
 document.addEventListener(replacement,dispose,{once:true});
 addEventListener('pagehide',event=>{if(!event.persisted)dispose();},{signal});
})();
