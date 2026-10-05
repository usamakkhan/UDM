 'use strict';
const api=globalThis.browser||chrome,$=id=>document.getElementById(id);
const make=(tag,text)=>{const node=document.createElement(tag);if(text)node.textContent=text;return node;};
let busy=false;
async function request(message){const reply=await api.runtime.sendMessage(message);if(!reply?.ok)throw Error(reply?.error||'UDM did not reply.');return reply;}
function status(text,error=false){$('status').textContent=text;$('status').className=error?'error':'';}
async function load(){
 if(busy)return;busy=true;$('refresh').disabled=true;
 try{
  const result=await request({action:'media-receipt-list'});$('entries').replaceChildren();
  for(const item of result.items){
   const card=make('section');card.className='entry';card.append(make('h2',item.label||'Video download'));
   const description=item.status==='accepted'?(item.present===false?'UDM accepted this download earlier; its history record has been removed.':'UDM confirmed this download was accepted.'):item.status==='released'?'UDM did not accept this request. Its original handoff is closed.':item.status==='uncertain'?'The saved receipt is too old to prove whether this download was accepted. Review UDM before starting another copy.':'The browser did not receive a confirmed outcome. Check the receipt before retrying.';
   card.append(make('p',description),make('p',new Date(item.created).toLocaleString()));
   const check=make('button','Check receipt'),clear=make('button','Clear reviewed notice'),message=make('p');message.setAttribute('role','status');
   const known=['accepted','released'].includes(item.status),expired=Date.now()-Number(item.token.slice(0,13))>660000;
   const label=make('label'),confirm=make('input');confirm.type='checkbox';label.append(confirm,make('span','I reviewed UDM and handled any existing copy. Clear only this notice.'));label.hidden=known||!expired;
   clear.disabled=!known;confirm.onchange=()=>{clear.disabled=!confirm.checked;};
   check.onclick=async()=>{check.disabled=true;clear.disabled=true;try{await request({action:'media-receipt-check',token:item.token});await load();}catch(e){message.textContent=e.message;check.disabled=false;clear.disabled=!known&&!(expired&&confirm.checked);}};
   clear.onclick=async()=>{clear.disabled=true;check.disabled=true;try{await request({action:'media-receipt-dismiss',token:item.token,confirmed:!known&&expired&&confirm.checked});await load();}catch(e){message.textContent=e.message;check.disabled=false;clear.disabled=!known&&!(expired&&confirm.checked);}};
   card.append(check,label,clear,message);$('entries').append(card);
  }
  status(result.items.length?result.items.length+' media handoff(s) to review.':'No media handoffs need review.');
 }catch(e){status(e.message,true);}finally{busy=false;$('refresh').disabled=false;}
}
$('refresh').onclick=load;$('desktop').onclick=()=>request({action:'media-receipt-show'}).catch(e=>status(e.message,true));load();
