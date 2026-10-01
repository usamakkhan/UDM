'use strict';
const api=globalThis.browser||chrome,$=id=>document.getElementById(id);
const confirmations={dismiss:'I have handled both copies and want to clear only this recovery notice.',browser:'I have stopped or removed any matching download in UDM.',native:'I found this download in UDM and want to cancel its browser copy.'};
const make=(tag,text)=>{const node=document.createElement(tag);if(text)node.textContent=text;return node;};
let busy=false;
async function request(message){const result=await api.runtime.sendMessage(message);if(!result?.ok)throw Error(result?.error||'UDM did not reply.');return result;}
function status(text,error=false){$('status').textContent=text;$('status').className=error?'error':'';}
async function load(){
 if(busy)return;busy=true;$('refresh').disabled=true;
 try{const result=await request({action:'capture-legacy-list'});$('entries').replaceChildren();
  for(const item of result.items){
   const card=make('section');card.className='entry';const heading=make('h2',item.name||'Interrupted download');
   const meta=make('p',[item.host,new Date(item.started).toLocaleString(),item.paused?'Paused in browser':item.state].filter(Boolean).join(' · '));meta.className='meta';
   const choice=make('select');choice.setAttribute('aria-label','Recovery action for '+item.name);
   for(const [value,text] of [['','Choose what to keep…'],['browser','Resume in browser'],['native','Keep in UDM; cancel browser copy'],['dismiss','Clear notice; I have handled both copies']]){const option=make('option',text);option.value=value;option.disabled=value==='browser'&&!item.resumable;choice.append(option);}
   const label=make('label'),confirm=make('input'),text=make('span','Choose an action, then confirm after checking UDM.');confirm.type='checkbox';confirm.disabled=true;label.append(confirm,text);
   const apply=make('button',item.decision?'Retry saved choice':'Apply choice');apply.className='apply';apply.disabled=true;
   const local=make('p');local.setAttribute('role','status');if(!item.resumable)local.textContent='The browser cannot resume this response. Check browser Downloads to retry it if needed, then clear this notice after handling both copies.';
   choice.onchange=()=>{confirm.checked=false;confirm.disabled=!choice.value;text.textContent=confirmations[choice.value]||'Choose an action, then confirm after checking UDM.';apply.disabled=true;};
   confirm.onchange=()=>{apply.disabled=!confirm.checked||!choice.value;};
   if(item.decision){
    const unavailable=item.decision==='browser'&&!item.resumable;
    choice.value=unavailable?'':item.decision;for(const option of choice.options)option.disabled=(option.value!==item.decision&&option.value!=='dismiss')||(option.value==='browser'&&!item.resumable);choice.onchange();
    local.textContent=unavailable?'The saved resume choice cannot continue because the browser cannot resume this response. Use browser Downloads to retry it if needed, then choose Clear notice after handling both copies.':'This choice was saved but could not finish. Retry it when the browser and UDM are ready, or handle both copies and clear the notice.';
   }
   apply.onclick=async()=>{
    if(!confirm.checked||!choice.value)return;apply.disabled=true;choice.disabled=true;confirm.disabled=true;$('refresh').disabled=true;busy=true;
    try{const result=await request({action:'capture-legacy-resolve',token:item.token,decision:choice.value,confirmed:true});card.remove();status(result.owner==='dismiss'?'Recovery notice cleared. Neither download was changed.':result.owner==='native'?'UDM keeps this download. Its browser copy is canceled.':'The browser keeps this download. No request was sent again by UDM.');if(!$('entries').childElementCount)$('entries').append(make('p','No interrupted downloads need a decision.'));}
    catch(error){local.textContent=error.message;local.className='error';apply.textContent='Retry saved choice';apply.disabled=false;confirm.disabled=false;choice.disabled=false;for(const option of choice.options)option.disabled=option.value!==choice.value&&option.value!=='dismiss';}
    finally{busy=false;$('refresh').disabled=false;}
   };
   card.append(heading,meta,choice,label,apply,local);$('entries').append(card);
  }
  status(result.items.length?result.items.length+' interrupted downloads need review.':'No interrupted downloads need a decision.');
 }catch(error){status(error.message,true);}finally{busy=false;$('refresh').disabled=false;}
}
$('refresh').onclick=load;
$('desktop').onclick=async()=>{try{await request({action:'capture-legacy-show'});status('UDM opened. Check whether this download is already in its list.');}catch(error){status(error.message,true);}};
async function showFirefoxDownloads(){
 const button=$('downloads'),panel=$('browser-history'),message=$('browser-status');
 button.disabled=true;panel.hidden=false;button.setAttribute('aria-expanded','true');message.className='';message.textContent='Reading Firefox downloads…';$('browser-records').replaceChildren();
 try{
  const items=await api.downloads.search({limit:101,orderBy:['-startTime']});
  for(const item of items.slice(0,100)){
   const row=make('li'),name=(item.filename||'Unnamed download').split(/[\\/]/).pop();
   const heading=make('h3',name);let host='';try{host=new URL(item.url).hostname;}catch{}
   const state=item.state==='complete'?'Complete':item.paused?'Paused':item.state==='in_progress'?'Downloading':item.error==='USER_CANCELED'?'Canceled':'Interrupted';
   const received=Number.isFinite(item.bytesReceived)?Math.max(0,item.bytesReceived).toLocaleString()+' bytes received':'';
   const started=Number.isFinite(Date.parse(item.startTime))?new Date(item.startTime).toLocaleString():'';
   const details=make('p',[state,host,started,received].filter(Boolean).join(' · '));details.className='meta';
   const filename=make('p',item.filename||'No saved filename');filename.className='meta';
   row.append(heading,details,filename);$('browser-records').append(row);
  }
  message.textContent=items.length>100?'Showing the 100 most recent Firefox downloads.':items.length?items.length+' Firefox downloads.':'Firefox has no downloads in this profile.';
 }catch(error){message.textContent='Could not read Firefox downloads: '+error.message;message.className='error';}
 finally{button.disabled=false;button.textContent='Refresh Firefox downloads';panel.focus();}
}
if(location.protocol==='moz-extension:'){
 $('downloads').textContent='Show Firefox downloads';$('downloads').setAttribute('aria-controls','browser-history');$('downloads').setAttribute('aria-expanded','false');
 $('browser-shortcut').textContent='Press '+(/Mac/.test(navigator.platform)?'Command+J':'Ctrl+J')+' to open Firefox’s full Downloads window. This list does not change any download; use the recovery choices below after checking both copies.';
 $('downloads').onclick=showFirefoxDownloads;
}else $('downloads').onclick=()=>api.tabs.create({url:/Edg\//.test(navigator.userAgent)?'edge://downloads/all':'chrome://downloads/'}).catch(error=>status(error.message,true));
load();
