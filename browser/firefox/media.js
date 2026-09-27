/* Original, bounded parsers for clear, recorded HLS and DASH presentations. */
(function(root){
 'use strict';
 const MAX=1200;
 function url(value,base){const u=new URL(value,base);if(!/^https?:$/.test(u.protocol)||u.username||u.password)throw Error('Only HTTP(S) media URLs are supported.');u.hash='';return u.href;}
 function kind(value,type=''){let p='';try{p=new URL(value).pathname.toLowerCase();}catch{return '';}
  if(/mpegurl/i.test(type)||/\.m3u8?$/.test(p))return 'hls';
  if(/dash\+xml/i.test(type)||/\.mpd$/.test(p))return 'dash';
  if(/\.m4s$|(?:^|\/)(?:init|seg(?:ment)?[-_0-9])/.test(p))return 'fragment';
  if(/^video\//i.test(type)||/\.(mp4|webm|mov|m4v|ogv)$/.test(p))return 'direct';
  return '';
 }
 function attrs(s){const out={};let at=0;const re=/([A-Z0-9-]+)=("(?:[^"]*)"|[^,]*)(?:,|$)/gy;
  while(at<s.length){re.lastIndex=at;const m=re.exec(s);if(!m)throw Error('Invalid playlist attributes.');out[m[1]]=m[2].replace(/^"|"$/g,'');at=re.lastIndex;}return out;
 }
 function range(value,previous){const m=/^(\d+)(?:@(\d+))?$/.exec(value||'');if(!m)throw Error('Invalid byte range.');const length=Number(m[1]),start=m[2]===undefined?previous:Number(m[2]);
  if(!Number.isSafeInteger(length)||length<1||!Number.isSafeInteger(start)||start<0||!Number.isSafeInteger(start+length))throw Error('Unbounded byte range.');return {start,length};
 }
 function hls(text,base){
  if(text.length>2e6||!text.trimStart().startsWith('#EXTM3U'))throw Error('Invalid HLS playlist.');
  const lines=text.split(/\r?\n/).map(x=>x.trim()).filter(Boolean),variants=[],audio=[],segments=[];
  let next=null,byteRange=null,previousEnd=0,previousUrl='',init=null,hasEnd=false;
  for(const line of lines){
   if(/^#EXT-X-(SESSION-)?KEY:/.test(line)){if(attrs(line.slice(line.indexOf(':')+1)).METHOD!=='NONE')throw Error('Encrypted or DRM-protected streams are not supported.');}
   else if(/^#EXT-X-(DISCONTINUITY(?::|$)|GAP(?::|$)|DEFINE:|SKIP:|PART:|I-FRAMES-ONLY)/.test(line))throw Error('This HLS playlist uses unsupported live or discontinuity features.');
   else if(line.startsWith('#EXT-X-STREAM-INF:'))next=attrs(line.slice(18));
   else if(line.startsWith('#EXT-X-MEDIA:')){const a=attrs(line.slice(13));if(a.TYPE==='AUDIO')audio.push({...a,url:a.URI?url(a.URI,base):''});}
   else if(line.startsWith('#EXT-X-MAP:')){const a=attrs(line.slice(11));const part={url:url(a.URI,base),...(a.BYTERANGE?range(a.BYTERANGE,0):{})};if(init&&JSON.stringify(init)!==JSON.stringify(part))throw Error('Changing initialization segments are not supported.');if(!init){init=part;segments.push(part);}}
   else if(line.startsWith('#EXT-X-BYTERANGE:'))byteRange=line.slice(17);
   else if(line==='#EXT-X-ENDLIST')hasEnd=true;
   else if(!line.startsWith('#')){
    const target=url(line,base);
    if(next){const size=/^(\d+)x(\d+)$/.exec(next.RESOLUTION||'');variants.push({url:target,height:size?Number(size[2]):0,bandwidth:Number(next.BANDWIDTH)||0,audioGroup:next.AUDIO||'',codecs:next.CODECS||''});next=null;}
    else {let part={url:target};if(byteRange){if(!byteRange.includes('@')&&previousUrl!==target)throw Error('Byte range offset is ambiguous.');Object.assign(part,range(byteRange,previousEnd));previousEnd=part.start+part.length;byteRange=null;}else previousEnd=0;previousUrl=target;segments.push(part);}
   }
   if(segments.length>MAX||variants.length>100||audio.length>128)throw Error('This playlist exceeds UDM’s current segment limit.');
  }
  if(next)throw Error('Missing HLS variant URL.');
  if(variants.length)return {kind:'master',variants,audio};
  if(!hasEnd)throw Error('This is a live playlist. Recorded videos are supported.');
  if(segments.length<1||(init&&segments.length<2)||byteRange)throw Error('The playlist has no complete media segments.');
  return {kind:'media',segments};
 }
 function audioLabel(track,index=0){
  const name=String(track.name||track.NAME||'Audio '+(index+1)),language=String(track.language||track.LANGUAGE||'');
  if(name.length>160||/[\x00-\x1f\x7f]/.test(name)||language&&!/^[a-zA-Z]{2,8}(?:-[a-zA-Z0-9]{1,8})*$/.test(language)||language.length>63)throw Error('Invalid audio track description.');
  return name+(language?' ('+language+')':'')+(track.bandwidth>0?' · '+Math.round(track.bandwidth/1000)+' kbps':'');
 }
 function hlsAudio(master,variant){
  if(!variant.audioGroup)return [];
  const tracks=master.audio.filter(a=>a['GROUP-ID']===variant.audioGroup);
  if(!tracks.length)throw Error('The selected quality refers to a missing audio group.');
  if(tracks.length>32||new Set(tracks.map(a=>a.NAME)).size!==tracks.length||tracks.some(a=>!a.NAME)||tracks.filter(a=>a.DEFAULT==='YES').length>1)throw Error('Ambiguous or oversized HLS audio group.');
  if(tracks.filter(a=>!a.url).length>1)throw Error('Multiple in-band HLS audio tracks need additional support.');
  const selected=tracks.findIndex(a=>a.DEFAULT==='YES'),fallback=tracks.findIndex(a=>a.AUTOSELECT==='YES');
  return tracks.map((a,i)=>({key:'audio-'+i,name:a.NAME,language:a.LANGUAGE||'',label:audioLabel(a,i),url:a.url,default:i===(selected<0?(fallback<0?0:fallback):selected)}));
 }
 // Minimal XML tree: no DTD, external entities, processing instructions or expansion.
 function xml(text){
  if(text.length>2e6||/<!DOCTYPE|<!ENTITY/i.test(text))throw Error('Unsupported XML declaration.');
  const document={name:'document',attrs:{},children:[],text:''},stack=[document];
  const decode=s=>s.replace(/&(?:amp|lt|gt|quot|apos);|&#(?:x[0-9a-f]+|\d+);/gi,x=>{const named={'&amp;':'&','&lt;':'<','&gt;':'>','&quot;':'"','&apos;':"'"};if(named[x])return named[x];const n=x[2]==='x'?parseInt(x.slice(3,-1),16):Number(x.slice(2,-1));if(n<1||n>0x10ffff)throw Error('Invalid XML character.');return String.fromCodePoint(n);});
  const tokens=text.match(/<!--[\s\S]*?-->|<\?[\s\S]*?\?>|<!\[CDATA\[[\s\S]*?\]\]>|<[^>]+>|[^<]+/g)||[];let count=0;
  for(const token of tokens){if(token.startsWith('<!--')||token.startsWith('<?'))continue;
   if(token.startsWith('</')){if(stack.length===1||stack.at(-1).qualified!==token.slice(2,-1).trim())throw Error('Invalid MPD XML.');stack.pop();}
   else if(token.startsWith('<![CDATA['))stack.at(-1).text+=token.slice(9,-3);
   else if(token.startsWith('<')){const m=/^<([\w:.-]+)((?:\s+[\s\S]*?)?)\/?>$/.exec(token);if(!m||++count>12000||stack.length>32)throw Error('Invalid or oversized MPD XML.');
    const node={name:m[1].split(':').pop(),qualified:m[1],attrs:{},children:[],text:''};const a=m[2].replace(/\/$/,'');const rx=/\s+([\w:.-]+)\s*=\s*(?:"([^"]*)"|'([^']*)')/g;let pos=0,match;while((match=rx.exec(a))){if(a.slice(pos,match.index).trim())throw Error('Invalid MPD attributes.');node.attrs[match[1]]=decode(match[2]??match[3]);pos=rx.lastIndex;}if(a.slice(pos).trim())throw Error('Invalid MPD attributes.');stack.at(-1).children.push(node);if(!token.endsWith('/>'))stack.push(node);
   }else stack.at(-1).text+=decode(token);
  }if(stack.length!==1||document.children.length!==1)throw Error('Invalid MPD XML.');return document.children[0];
 }
 const child=(n,name)=>n?.children.find(x=>x.name===name),children=(n,name)=>n?.children.filter(x=>x.name===name)||[];
 function duration(value){const m=/^P(?:(\d+(?:\.\d+)?)D)?(?:T(?:(\d+(?:\.\d+)?)H)?(?:(\d+(?:\.\d+)?)M)?(?:(\d+(?:\.\d+)?)S)?)?$/.exec(value||'');return m?Number(m[1]||0)*86400+Number(m[2]||0)*3600+Number(m[3]||0)*60+Number(m[4]||0):0;}
 function dash(text,base){
  const mpd=xml(text);if(mpd.name!=='MPD'||(mpd.attrs.type&&mpd.attrs.type!=='static'))throw Error('Only recorded DASH presentations are supported.');
  const periods=children(mpd,'Period');if(periods.length!==1)throw Error('Multi-period DASH needs additional support.');
  function protectedNode(n){return n.name==='ContentProtection'||n.children.some(protectedNode);}if(protectedNode(mpd))throw Error('Encrypted or DRM-protected streams are not supported.');
  const period=periods[0],seconds=duration(period.attrs.duration)||duration(mpd.attrs.mediaPresentationDuration);const tracks=[];
  for(const set of children(period,'AdaptationSet'))for(const rep of children(set,'Representation')){
   const lineage=[mpd,period,set,rep];let target=base;
   for(const node of lineage){const bases=children(node,'BaseURL');if(bases.length>1)throw Error('Multiple DASH base URLs need an explicit selection.');if(bases[0])target=url(bases[0].text.trim(),target);}
   const mime=rep.attrs.mimeType||set.attrs.mimeType||'',type=rep.attrs.contentType||set.attrs.contentType||mime.split('/')[0];if(!['video','audio'].includes(type))continue;
   if(mime&&!/^(video|audio)\/mp4$/.test(mime))continue;
   let template={},timeline=null,list=null;
   for(const n of lineage){const t=child(n,'SegmentTemplate');if(t){template={...template,...t.attrs};timeline=child(t,'SegmentTimeline')||timeline;}list=child(n,'SegmentList')||list;if(child(n,'SegmentBase'))throw Error('DASH SegmentBase needs additional support.');}
   const segments=[];const push=x=>{segments.push(x);if(segments.length>MAX)throw Error('This playlist exceeds UDM’s current segment limit.');};
   const inclusive=value=>{const m=/^(\d+)-(\d+)$/.exec(value||'');if(!m||Number(m[2])<Number(m[1]))throw Error('Invalid DASH range.');return range((Number(m[2])-Number(m[1])+1)+'@'+m[1],0);};
   if(list){const init=child(list,'Initialization');if(init)push({url:url(init.attrs.sourceURL||target,target),...(init.attrs.range?inclusive(init.attrs.range):{})});for(const s of children(list,'SegmentURL'))push({url:url(s.attrs.media||target,target),...(s.attrs.mediaRange?inclusive(s.attrs.mediaRange):{})});}
   else if(template.media){
    const timescale=Number(template.timescale||1),start=Number(template.startNumber||1),offset=Number(template.presentationTimeOffset||0);if(!Number.isSafeInteger(timescale)||timescale<1||!Number.isSafeInteger(start)||start<0||!Number.isSafeInteger(offset)||offset<0)throw Error('Invalid DASH timing.');
    function expand(pattern,number,time){const result=pattern.replace(/\$\$|\$(RepresentationID|Bandwidth|Number|Time)(?:%0(\d+)d)?\$/g,(all,key,width)=>{if(all==='$$')return '$';const v={RepresentationID:rep.attrs.id,Bandwidth:rep.attrs.bandwidth,Number:number,Time:time}[key];if(v===undefined)throw Error('Missing DASH template value.');if(width&&Number(width)>12)throw Error('Invalid DASH template width.');return width?String(v).padStart(Number(width),'0'):String(v);});if(/\$[A-Za-z]/.test(result))throw Error('Unsupported DASH template.');return url(result,target);}
    if(template.initialization)push({url:expand(template.initialization,start,0)});
    let number=start;
    if(timeline){let time=0;const entries=children(timeline,'S');for(let i=0;i<entries.length;i++){const a=entries[i].attrs;time=a.t===undefined?time:Number(a.t);const d=Number(a.d),r=Number(a.r||0);if(!Number.isSafeInteger(time)||time<0||!Number.isSafeInteger(d)||d<1||!Number.isSafeInteger(r)||r< -1)throw Error('Invalid DASH timeline.');
      const boundary=entries[i+1]?.attrs.t!==undefined?Number(entries[i+1].attrs.t):seconds*timescale+offset;const repeat=r===-1?Math.ceil((boundary-time)/d)-1:r;if(repeat<0||repeat>MAX||!Number.isFinite(repeat))throw Error('Unbounded DASH timeline.');for(let n=0;n<=repeat;n++){push({url:expand(template.media,number++,time)});time+=d;if(!Number.isSafeInteger(time))throw Error('DASH timeline overflow.');}
    }}else{const d=Number(template.duration);if(!seconds||!Number.isSafeInteger(d)||d<1)throw Error('DASH duration is missing.');const count=Math.ceil(seconds*timescale/d);if(count>MAX)throw Error('This playlist exceeds UDM’s current segment limit.');for(let i=0;i<count;i++)push({url:expand(template.media,number++,i*d)});}
   }else if(target!==base)push({url:target});else continue;
   if(!segments.length)continue;
   const roles=children(set,'Role').map(x=>x.attrs.value).filter(Boolean),name=(child(rep,'Label')||child(set,'Label'))?.text.trim()||roles.filter(x=>x!=='main').join(', ')||(set.attrs.lang?'Audio':'Audio '+(tracks.filter(t=>t.kind==='audio').length+1));
   tracks.push({kind:type,id:rep.attrs.id||String(tracks.length),name,main:roles.includes('main'),height:Number(rep.attrs.height||set.attrs.height)||0,bandwidth:Number(rep.attrs.bandwidth)||0,language:rep.attrs.lang||set.attrs.lang||'',segments});
  }
  const audioTracks=tracks.filter(t=>t.kind==='audio').sort((a,b)=>Number(b.main)-Number(a.main)||b.bandwidth-a.bandwidth);
  if(audioTracks.length>32)throw Error('This DASH presentation exceeds the 32-audio-track limit.');
  const audioOptions=audioTracks.map((track,i)=>({key:'audio-'+i,name:track.name,language:track.language,label:audioLabel(track,i),default:i===0,track}));
  const audio=audioTracks[0];
  const choices=tracks.filter(t=>t.kind==='video').map(v=>({height:v.height,bandwidth:v.bandwidth,audioOptions,label:(v.height?v.height+'p':'Original quality')+' · DASH'+(audio?' · '+(audio.language||'audio'):''),plan:{type:'dash',height:v.height,audioExpected:!!audio,tracks:[v,...(audio?[audio]:[])]}}));
  if(!choices.length)throw Error('No supported MP4 video representations were found.');return choices.sort((a,b)=>b.height-a.height||b.bandwidth-a.bandwidth);
 }
 function placement(rect,viewport,width=168,height=24,offset={x:0,y:0},clips=[]){
  const bounds={left:viewport.left||0,top:viewport.top||0,right:(viewport.left||0)+viewport.width,bottom:(viewport.top||0)+viewport.height};
  for(const clip of clips){
   if(clip.x!==false){bounds.left=Math.max(bounds.left,clip.left);bounds.right=Math.min(bounds.right,clip.right);}
   if(clip.y!==false){bounds.top=Math.max(bounds.top,clip.top);bounds.bottom=Math.min(bounds.bottom,clip.bottom);}
  }
  const visibleWidth=Math.min(rect.right,bounds.right)-Math.max(rect.left,bounds.left),visibleHeight=Math.min(rect.bottom,bounds.bottom)-Math.max(rect.top,bounds.top);
  const compact=visibleWidth<230,actualWidth=compact?30:width;
  let x=Math.min(rect.right,bounds.right)-actualWidth-4+Number(offset.x||0),y=(rect.top>=(viewport.top||0)+height?rect.top-height+8:rect.top+4)+Number(offset.y||0);
  x=Math.max(bounds.left+4,Math.min(bounds.right-actualWidth-4,x));y=Math.max(bounds.top+4,Math.min(bounds.bottom-height-4,y));
  return {x,y,width:actualWidth,compact,visible:visibleWidth>=120&&visibleHeight>=70};
 }

 const policy={
  blocked(address,settings={}){try{const u=new URL(address);if(!/^https?:$/.test(u.protocol)||u.username||u.password)return true;
   if((settings.excluded||[]).some(h=>u.hostname===h||u.hostname.endsWith('.'+h)))return true;
   return (settings.excludedUrls||[]).some(pattern=>{if(typeof pattern!=='string'||pattern.length>2048)return false;const m=/^(https?):\/\/(\*\.)?([a-z0-9.-]+(?::\d{1,5})?)(\/[^\s#\\]*)$/i.exec(pattern);if(!m)return false;
    if(u.protocol!==m[1].toLowerCase()+':'||!(u.host===m[3].toLowerCase()||(m[2]&&u.host.endsWith('.'+m[3].toLowerCase()))))return false;
    const expression=m[4].split('*').map(p=>p.replace(/[.*+?^{}$()|[\]\\]/g,'\\$&')).join('.*');return new RegExp('^'+expression+'$').test(u.pathname+u.search);
   });}catch{return true;}},
  key(event,name){const expected={'Alt':[1,0,0],'Ctrl':[0,1,0],'Shift':[0,0,1],'Ctrl+Shift':[0,1,1],'Alt+Shift':[1,0,1]}[name];return !!expected&&!event.metaKey&&expected.every((v,i)=>!!v===[!!event.altKey,!!event.ctrlKey,!!event.shiftKey][i]);},
  intent(event,settings={}){if(this.key(event,settings.bypassKey||'Ctrl'))return 'bypass';if(this.key(event,settings.forceKey||'Alt'))return 'force';return '';},
  merge(local={},desktop={}){const p={...local,...desktop};p.capture=!!local.capture;p.cookies=!!local.cookies;p.excluded=[...(local.excluded||[]),...(desktop.excluded||[])];p.excludedUrls=[...(local.excludedUrls||[]),...(desktop.excludedUrls||[])];return p;}
 };

 const exported={url,kind,hls,hlsAudio,audioLabel,dash,placement,policy,MAX};root.UdmMedia=exported;if(typeof module!=='undefined')module.exports=exported;
})(globalThis);
