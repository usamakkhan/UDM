'use strict';
const api=globalThis.browser||chrome,$=id=>document.getElementById(id),token=location.hash.slice(1);let rows=[];
function fileLabel(url){try{return decodeURIComponent(new URL(url).pathname.split('/').pop())||'Link';}catch{return 'Link';}}
function update(){const count=rows.filter(r=>r.check.checked).length;$('count').textContent=count+' selected';$('download').disabled=count<1||count>100;}
function filter(){const q=$('filter').value.toLowerCase();for(const r of rows)r.element.hidden=!r.text.includes(q);}
function files(){for(const r of rows)r.check.checked=/\.(zip|7z|rar|iso|exe|msi|pdf|mp4|webm|mkv|mp3|flac|jpg|jpeg|png|docx?|xlsx?|pptx?)(?:[?#]|$)/i.test(r.link.url);update();}
(async()=>{const result=await api.runtime.sendMessage({action:'batch-list',token});if(!result?.ok)throw Error(result?.error||'UDM did not reply.');
 for(const link of result.links){const element=document.createElement('tr'),cell=document.createElement('td'),check=document.createElement('input');check.type='checkbox';check.setAttribute('aria-label','Select '+(link.name||link.url));check.onchange=update;cell.append(check);element.append(cell);
  for(const value of [link.name||fileLabel(link.url),link.url]){const td=document.createElement('td');td.textContent=value;element.append(td);}
  $('rows').append(element);rows.push({element,check,link,text:(link.name+' '+link.url).toLowerCase()});
 }files();
})().catch(e=>{$('status').textContent=e.message;});
$('filter').oninput=filter;$('all').onclick=()=>{for(const r of rows)if(!r.element.hidden)r.check.checked=true;update();};$('none').onclick=()=>{for(const r of rows)r.check.checked=false;update();};$('files').onclick=files;
$('download').onclick=async()=>{const indexes=rows.filter(r=>r.check.checked).map(r=>r.link.index);$('download').disabled=true;$('status').textContent='Adding selected links…';try{const r=await api.runtime.sendMessage({action:'batch-download',token,indexes,downloadLater:$('later').checked});$('status').textContent=r?.ok?r.count+' links added to UDM. You can close this tab.':r?.error||'UDM did not reply. Check its download list before trying again.';}catch(e){$('status').textContent=e.message+' Check UDM before retrying.';}};
