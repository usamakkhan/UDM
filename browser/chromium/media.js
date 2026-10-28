/* Original, bounded parsers for clear, recorded HLS and DASH presentations. */
(function(root){
 'use strict';
 const MAX=10000,MAX_PLAN_BYTES=4*1024*1024;
 function validatePlanBudget(plan){
  if(!plan||!Array.isArray(plan.tracks)||plan.tracks.some(track=>!Array.isArray(track.segments)))throw Error('Invalid streaming plan.');
  if(plan.tracks.reduce((sum,track)=>sum+track.segments.length,0)>MAX)throw Error('This selection exceeds the current segment limit.');
  if(new TextEncoder().encode(JSON.stringify(plan)).length>MAX_PLAN_BYTES)throw Error('This playlist is too large for the current browser handoff.');
 }
 function url(value,base){const u=new URL(value,base);if(!/^https?:$/.test(u.protocol)||u.username||u.password)throw Error('Only HTTP(S) media URLs are supported.');u.hash='';return u.href;}
 // Asset-ID matching adapted from Awesome Downloader (MIT); see awesome-downloader-LICENSE.txt.
 function twitterAsset(value){
  try{const u=new URL(value);if(u.protocol!=='https:'||u.username||u.password||!/(^|\.)twimg\.com$/.test(u.hostname))return '';
   const m=/^\/(amplify_video|ext_tw_video|tweet_video)(?:_thumb)?\/([0-9]{1,30})\//.exec(u.pathname);
   return m?m[1]+':'+m[2]:'';
  }catch{return '';}
 }
 // Adapted fragment filtering from Awesome Downloader extension/background.js (MIT).
 // Only used for observed feed requests, not explicit user-selected TS/AAC files.
 function feedMediaFragment(value){
  try{const p=new URL(value).pathname;return /\.(?:m4s|ts|aac)$/i.test(p)||/\/init[^/]*\.mp4$/i.test(p);}catch{return true;}
 }
 function fullHlsUrl(value){const source=url(value),mark=source.indexOf('?');if(mark<0)return source;
  const entries=source.slice(mark+1).split('&'),kept=entries.filter(item=>!['_HLS_skip','_HLS_msn','_HLS_part'].includes(item.split('=',1)[0]));
  return kept.length===entries.length?source:source.slice(0,mark)+(kept.length?'?'+kept.join('&'):'');
 }
 function kind(value,type=''){let p='';try{p=new URL(value).pathname.toLowerCase();}catch{return '';}
  if(/mpegurl/i.test(type)||/\.m3u8?$/.test(p))return 'hls';
  if(/dash\+xml/i.test(type)||/\.mpd$/.test(p))return 'dash';
  if(/\.m4s$|(?:^|\/)(?:init|seg(?:ment)?[-_0-9])/.test(p))return 'fragment';
  if(/^video\//i.test(type)||/\.(mp4|webm|mov|m4v|ogv)$/.test(p))return 'direct';
  return '';
 }
 function attrs(s,substitute){const out={};let at=0;const re=/([A-Z0-9-]+)=("(?:[^"]*)"|[^,]*)(?:,|$)/gy;
  while(at<s.length){re.lastIndex=at;const m=re.exec(s);if(!m||Object.hasOwn(out,m[1]))throw Error('Invalid or duplicate playlist attributes.');const quoted=m[2].startsWith('"');out[m[1]]=quoted?substitute?substitute(m[2].slice(1,-1)):m[2].slice(1,-1):m[2];at=re.lastIndex;}return out;
 }
 function range(value,previous){const m=/^(\d+)(?:@(\d+))?$/.exec(value||'');if(!m)throw Error('Invalid byte range.');const length=Number(m[1]),start=m[2]===undefined?previous:Number(m[2]);
  if(!Number.isSafeInteger(length)||length<1||!Number.isSafeInteger(start)||start<0||!Number.isSafeInteger(start+length))throw Error('Unbounded byte range.');return {start,length};
 }
 function hlsQuery(base,name){for(const entry of new URL(base).search.slice(1).split('&')){const at=entry.indexOf('=');if(at<0)continue;try{if(decodeURIComponent(entry.slice(0,at))===name)return decodeURIComponent(entry.slice(at+1));}catch{throw Error('Invalid HLS query parameter encoding.');}}return null;}
 function hls(text,base,options={}){
  if(text.length>2e6||!text.trimStart().startsWith('#EXTM3U'))throw Error('Invalid HLS playlist.');
  const lines=text.split(/\r?\n/).map(x=>x.trim()).filter(Boolean),variants=[],audio=[],subtitles=[],segments=[];
  const variables=Object.create(null),imports=options.imports||{};
  const substitute=value=>{if(value.replace(/\{\$[A-Za-z0-9_-]+\}/g,'').includes('{$'))throw Error('Invalid HLS playlist variable reference.');return value.replace(/\{\$([A-Za-z0-9_-]+)\}/g,(_,name)=>{if(!Object.hasOwn(variables,name))throw Error('Undefined HLS playlist variable.');return variables[name];});};
  const live=options.allowLive===true&&!lines.includes('#EXT-X-ENDLIST')&&!lines.some(line=>line.startsWith('#EXT-X-STREAM-INF:'));
  let next=null,byteRange=null,previousEnd=0,previousUrl='',init=null,initIndex=-1,hasEnd=false,elapsed=0,nextDuration=null,imported=false,usesVariables=false,gap=false,pendingDiscontinuity=false;
  for(const line of lines){
   if(/^#EXT-X-(SESSION-)?KEY:/.test(line)){if(attrs(line.slice(line.indexOf(':')+1)).METHOD!=='NONE')throw Error('Encrypted or DRM-protected streams are not supported.');}
   else if(line.startsWith('#EXT-X-DEFINE:')){const a=attrs(line.slice(14)),names=['NAME','IMPORT','QUERYPARAM'].filter(key=>Object.hasOwn(a,key));if(names.length!==1||Object.keys(a).some(key=>!['NAME','VALUE','IMPORT','QUERYPARAM'].includes(key))||(names[0]==='NAME'?!Object.hasOwn(a,'VALUE'):Object.hasOwn(a,'VALUE')))throw Error('Invalid HLS variable definition.');const name=a[names[0]];if(!/^[A-Za-z0-9_-]+$/.test(name)||Object.hasOwn(variables,name)||Object.keys(variables).length>=64)throw Error('Invalid or duplicate HLS variable name.');let value;if(names[0]==='NAME')value=a.VALUE;else if(names[0]==='IMPORT'){if(!Object.hasOwn(imports,name))throw Error('Missing imported HLS variable.');value=imports[name];imported=true;}else{value=hlsQuery(base,name);if(!value)throw Error('Missing HLS query parameter.');}if(typeof value!=='string'||value.length>16384||/["\r\n\x00-\x1f\x7f]/.test(value))throw Error('Invalid HLS variable value.');variables[name]=value;usesVariables=true;}
   else if(/^#EXT-X-(SKIP:|I-FRAMES-ONLY)/.test(line))throw Error('This HLS playlist uses unsupported delta or trick-play features.');
   else if(line==='#EXT-X-DISCONTINUITY'){if(!live)pendingDiscontinuity=true;}
   else if(line.startsWith('#EXT-X-STREAM-INF:'))next=attrs(line.slice(18),substitute);
   else if(line.startsWith('#EXT-X-MEDIA:')){const a=attrs(line.slice(13),substitute);if(a.TYPE==='AUDIO')audio.push({...a,url:a.URI?url(a.URI,base):''});else if(a.TYPE==='SUBTITLES')subtitles.push({...a,url:a.URI?url(a.URI,base):''});}
   else if(line.startsWith('#EXT-X-MAP:')){const a=attrs(line.slice(11),substitute);const part={url:url(a.URI,base),...(a.BYTERANGE?range(a.BYTERANGE,0):{})};if(!init||init.url!==part.url||init.start!==part.start||init.length!==part.length){if(!live)part.initialization=true;init=part;initIndex=segments.length;segments.push(part);}}
   else if(line.startsWith('#EXTINF:')){const value=line.slice(8).split(',')[0];if(!/^[0-9]+(?:\.[0-9]+)?$/.test(value)||!Number.isFinite(Number(value))||Number(value)<=0||Number(value)>86400)throw Error('Invalid HLS segment duration.');nextDuration=Number(value);}
   else if(line.startsWith('#EXT-X-BYTERANGE:'))byteRange=line.slice(17);
   else if(line==='#EXT-X-GAP'){if(gap)throw Error('Repeated HLS gap marker.');gap=true;}
   else if(line==='#EXT-X-ENDLIST')hasEnd=true;
   else if(!line.startsWith('#')){
    const target=url(substitute(line),base);
    if(next){const size=/^(\d+)x(\d+)$/.exec(next.RESOLUTION||'');variants.push({url:target,height:size?Number(size[2]):0,bandwidth:Number(next.BANDWIDTH)||0,frameRate:/^[0-9]+(?:\.[0-9]+)?$/.test(next['FRAME-RATE']||'')&&Number.isFinite(Number(next['FRAME-RATE']))&&Number(next['FRAME-RATE'])>0?Number(next['FRAME-RATE']):0,audioGroup:next.AUDIO||'',subtitleGroup:next.SUBTITLES||'',codecs:next.CODECS||''});next=null;}
    else {let part={url:target};if(byteRange){if(!byteRange.includes('@')&&previousUrl!==target)throw Error('Byte range offset is ambiguous.');Object.assign(part,range(byteRange,previousEnd));previousEnd=part.start+part.length;byteRange=null;}else previousEnd=0;previousUrl=target;if(nextDuration!==null){part.timeline=elapsed;part.duration=nextDuration;elapsed+=nextDuration;nextDuration=null;if(elapsed>7*86400)throw Error('The playlist duration exceeds seven days.');}if(init&&!live)part.mapIndex=initIndex;if(pendingDiscontinuity&&!live)part.discontinuity=true;if(gap&&live)part.gap=true;if(!gap||live){segments.push(part);pendingDiscontinuity=false;}gap=false;}
   }
   if(segments.length>MAX||variants.length>100||audio.length>128||subtitles.length>128)throw Error('This playlist exceeds UDM’s current segment limit.');
  }
  if(next)throw Error('Missing HLS variant URL.');if(gap)throw Error('The HLS gap marker has no segment.');
  if(variants.length){if(imported)throw Error('HLS masters cannot import playlist variables.');return {kind:'master',variants,audio,subtitles,variables};}
  if(!hasEnd&&!live)throw Error('This is a live playlist. Recorded videos are supported.');
  if(live&&!lines.some(line=>/^#EXT-X-TARGETDURATION:[1-9][0-9]*$/.test(line)))throw Error('The live playlist has no valid target duration.');
  if(segments.length<1||(init&&segments.length<2)||byteRange)throw Error('The playlist has no complete media segments.');
  return {kind:'media',segments,hasInit:!!init,usesVariables,...(init?{initializationIndex:initIndex}:{}),...(live?{live:true,playlist:url(base,base)}:{})};
 }
 async function hlsAudioPlaylist(first,load,signal,imports={}){
  const visited=new Set();let current=first;
  for(let depth=0;depth<=6;depth++){
   if(signal?.aborted)throw Error('Reading the audio playlist was canceled.');
   const address=url(current.url,current.url);
   if(visited.has(address))throw Error('The audio playlist contains a cycle.');
   visited.add(address);
   const parsed=hls(current.text,address,{allowLive:true,imports});
   if(parsed.kind==='media')return {...parsed,playlist:address,imports};
   if(depth===6)throw Error('The audio playlist exceeds six nested masters.');
   // Stay within the selected rendition. Never descend into a video or another audio group.
   const choices=parsed.variants.filter(v=>!v.height&&!v.audioGroup&&!v.subtitleGroup&&v.codecs&&
    v.codecs.split(',').every(codec=>/^(mp4a|ac-3|ec-3|opus|vorbis|flac)(\.|$)/i.test(codec.trim())));
   if(!choices.length)throw Error('The nested audio playlist has no declared audio-only variant.');
   choices.sort((a,b)=>(b.bandwidth||0)-(a.bandwidth||0));
   const next=choices[0].url;
   if(visited.has(next))throw Error('The audio playlist contains a cycle.');
   imports=parsed.variables;current=await load(next,signal);
   if(signal?.aborted)throw Error('Reading the audio playlist was canceled.');
   if(current.url!==next)visited.add(next);
  }
  throw Error('Unsupported audio playlist.');
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
  return tracks.map((a,i)=>({key:'audio-'+i,name:a.NAME,language:a.LANGUAGE||'',label:audioLabel(a,i),url:a.url,variables:master.variables||{},default:i===(selected<0?(fallback<0?0:fallback):selected)}));
 }
 function hlsSubtitles(master,variant){
  if(!variant.subtitleGroup)return [];
  const tracks=(master.subtitles||[]).filter(s=>s['GROUP-ID']===variant.subtitleGroup);
  if(!tracks.length||tracks.length>32||new Set(tracks.map(s=>s.NAME)).size!==tracks.length||tracks.some(s=>!s.NAME||!s.url))throw Error('Missing, ambiguous or oversized HLS subtitle group.');
  return tracks.map((s,i)=>({key:'subtitle-'+i,name:s.NAME,language:s.LANGUAGE||'',label:audioLabel(s,i)+(s.FORCED==='YES'?' · forced':''),url:s.url,variables:master.variables||{}}));
 }
 // Compatibility traversal for providers that put a master behind a variant URI.
 // Rendition group IDs are local to their declaring playlist, never global.
 async function hlsCatalog(first,load,signal){
  const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),15000);
  const abort=()=>controller.abort();signal?.addEventListener('abort',abort,{once:true});if(signal?.aborted)abort();
  const cache=new Map(),covered=new Set(),notes=new Set(),choices=[],waiting=[];
  let active=0,reads=0,bytes=0,edges=0;
  function stopped(){if(controller.signal.aborted)throw Error('Reading the HLS playlist catalog timed out.');}
  function note(error){if(notes.size<16)notes.add(error.message||String(error));}
  async function slot(fn){
   if(active>=4)await new Promise(resolve=>waiting.push(resolve));else ++active;
   try{stopped();return await fn();}finally{const next=waiting.shift();if(next)next();else --active;}
  }
  function decode(fetched,imports){
   stopped();if(!fetched||typeof fetched.text!=='string')throw Error('Invalid HLS playlist response.');
   bytes+=new TextEncoder().encode(fetched.text).length;if(bytes>8*1024*1024){controller.abort();throw Error('The HLS catalog exceeds its eight MiB playlist limit.');}
   const address=url(fetched.url);covered.add(address);return {url:address,parsed:hls(fetched.text,address,{allowLive:true,imports})};
  }
  function read(address,imports){
   address=url(address);covered.add(address);stopped();
   const key=address+'\n'+JSON.stringify(imports||{});if(!cache.has(key)){
    if(++reads>64)throw Error('The HLS catalog exceeds its 64-playlist limit.');
    cache.set(key,slot(async()=>decode(await load(address,controller.signal),imports)));
   }
   return cache.get(key);
  }
  async function visit(item,inherited,ancestors,depth,requested=item.url){
   stopped();if(depth>6)throw Error('The HLS catalog exceeds six nested master playlists.');
   if(ancestors.has(item.url)||ancestors.has(requested))throw Error('The HLS playlist catalog contains a cycle.');
   const branch=new Set(ancestors);branch.add(item.url);branch.add(requested);
   if(item.parsed.kind==='media'){
    const choice={...inherited,url:item.url,live:!!item.parsed.live};
    const identity=JSON.stringify(choice);if(!choices.some(c=>JSON.stringify(c)===identity))choices.push(choice);
    return;
   }
   await Promise.all(item.parsed.variants.map(async variant=>{
    try{
     if(++edges>256)throw Error('The HLS catalog exceeds its 256-reference limit.');
     const audioOptions=variant.audioGroup?hlsAudio(item.parsed,variant):inherited.audioOptions||[];
     let subtitleOptions=inherited.subtitleOptions||[];
     if(variant.subtitleGroup){try{subtitleOptions=hlsSubtitles(item.parsed,variant);}catch(e){note(e);subtitleOptions=[];}}
     for(const track of [...audioOptions,...subtitleOptions])if(track.url)covered.add(track.url);
     const metadata={height:variant.height||inherited.height||0,bandwidth:variant.bandwidth||inherited.bandwidth||0,frameRate:variant.frameRate||inherited.frameRate||0,codecs:variant.codecs||inherited.codecs||'',audioOptions,subtitleOptions,hlsVariables:item.parsed.variables};
     if(branch.has(variant.url))throw Error('The HLS playlist catalog contains a cycle.');
     const child=await read(variant.url,item.parsed.variables);await visit(child,metadata,branch,depth+1,variant.url);
    }catch(e){note(e);}
   }));
  }
  try{
   const root=decode(first);cache.set(root.url+'\n{}',Promise.resolve(root));reads=1;
   await visit(root,{height:0,bandwidth:0,codecs:'',audioOptions:[],subtitleOptions:[]},new Set(),0);
   return {choices,covered:[...covered],notes:[...notes],reads,bytes};
  }finally{controller.abort();clearTimeout(timer);signal?.removeEventListener('abort',abort);}
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

 // ISO-BMFF Segment Index boxes identify ranges within a recorded DASH resource.
 // Keep offsets exact; tree expansion separately bounds depth, requests and total bytes.
 function sidxInfo(data,indexStart,resourceSize,externalMediaSize,externalTree=false){
  const external=externalMediaSize!==undefined;
  if(external&&(!Number.isSafeInteger(externalMediaSize)||externalMediaSize<1))throw Error('Invalid external DASH media bounds.');
  if(!(data instanceof Uint8Array)||!data.length||data.length>524288||!Number.isSafeInteger(indexStart)||indexStart<0||!Number.isSafeInteger(resourceSize)||resourceSize<1||!Number.isSafeInteger(indexStart+data.length)||indexStart+data.length>resourceSize)throw Error('Invalid DASH index bounds.');
  const view=new DataView(data.buffer,data.byteOffset,data.byteLength),parts=[];let at=0,found=false,info;
  const integer64=position=>{const n=view.getUint32(position)*4294967296+view.getUint32(position+4);if(!Number.isSafeInteger(n))throw Error('DASH index offset exceeds exact integer range.');return n;};
  while(at<data.length){
   if(data.length-at<8)throw Error('Truncated DASH index box.');
   let size=view.getUint32(at),header=8;const type=String.fromCharCode(...data.subarray(at+4,at+8));
   if(size===1){if(data.length-at<16)throw Error('Truncated DASH index header.');size=integer64(at+8);header=16;}
   if(size<header||size>data.length-at)throw Error('Invalid DASH index box size.');
   const end=at+size;
   if(type==='sidx'){
    if(found)throw Error('Multiple DASH indexes need additional support.');found=true;
    let p=at+header;if(end-p<24)throw Error('Truncated DASH segment index.');
    const version=view.getUint8(p);if(version>1||view.getUint32(p)%16777216)throw Error('Unsupported DASH index version or flags.');p+=4;
    const referenceId=view.getUint32(p),timescale=view.getUint32(p+4);if(!timescale)throw Error('Invalid DASH index timescale.');p+=8;
    if(version===1&&end-p<20)throw Error('Truncated 64-bit DASH index.');
    const earliest=version?integer64(p):view.getUint32(p);p+=version?8:4;
    const firstOffset=version?integer64(p):view.getUint32(p);p+=version?8:4;
    if(view.getUint16(p)!==0)throw Error('Invalid DASH index reserved bits.');
    const count=view.getUint16(p+2);p+=4;
    if(!count||count>=MAX||p+count*12!==end)throw Error('Invalid or oversized DASH reference table.');
    let offset=external?firstOffset:indexStart+end+firstOffset,time=earliest,indexOffset=indexStart+end;
    if(!Number.isSafeInteger(offset)||(!external&&offset<indexStart+data.length))throw Error('Invalid DASH media offset.');
    for(let i=0;i<count;++i,p+=12){
     const reference=view.getUint32(p),length=reference%2147483648,duration=view.getUint32(p+4);
     const kind=reference>=2147483648?'index':'media';
     if(external&&kind==='index'&&!externalTree)throw Error('Hierarchical external DASH indexes require bounded expansion.');
     if(kind==='index'&&length>524288)throw Error('DASH child index exceeds 512 KiB.');
     const start=externalTree&&kind==='index'?indexOffset:offset;
     if(!length||length>256*1024*1024||!duration||!Number.isSafeInteger(start+length)||(!(externalTree&&kind==='media')&&start+length>(external&&kind==='media'?externalMediaSize:resourceSize))||!Number.isSafeInteger(time+duration))throw Error('Invalid DASH media reference.');
     parts.push({kind,start,length,time,duration});
     if(externalTree&&kind==='index')indexOffset+=length;else offset+=length;
     time+=duration;
    }
    info={referenceId,timescale,earliest,duration:time-earliest,parts};
    if(externalTree){if(firstOffset>=externalMediaSize)throw Error('Invalid external DASH media offset.');return {...info,firstOffset,box:{start:indexStart+at,length:size}};}
   }
   at=end;
  }
  if(!found)throw Error('The DASH range contains no segment index.');
  return info;
 }
 function sidxExternal(data,indexStart,indexSize,mediaSize){
  if(!Number.isSafeInteger(mediaSize)||mediaSize<1)throw Error('Invalid external DASH media bounds.');
  return sidxInfo(data,indexStart,indexSize,mediaSize).parts.map(({start,length})=>({start,length}));
 }
 function sidx(data,indexStart,resourceSize){
  const info=sidxInfo(data,indexStart,resourceSize);
  if(info.parts.some(part=>part.kind==='index'))throw Error('Nested DASH indexes require bounded expansion.');
  return info.parts.map(({start,length})=>({start,length}));
 }
 // Each reference names one bounded child index range in the same recorded file.
 async function resolveSidx(spec,read,budget={requests:0,bytes:0,references:0}){
  const indexes=[],media=[],ranges=[];let total;
  const overlaps=(a,b)=>a.start<b.start+b.length&&b.start<a.start+a.length;
  const equalTime=(a,scaleA,b,scaleB)=>BigInt(a)*BigInt(scaleB)===BigInt(b)*BigInt(scaleA);
  async function visit(span,depth,parent){
   if(depth>8)throw Error('DASH index nesting exceeds eight levels.');
   if(!Number.isSafeInteger(span.start)||span.start<0||!Number.isSafeInteger(span.length)||span.length<1||span.length>524288||!Number.isSafeInteger(span.start+span.length))throw Error('Invalid DASH child index bounds.');
   if(indexes.some(x=>overlaps(x,span))||media.some(x=>overlaps(x,span)))throw Error('DASH indexes overlap another referenced range.');
   if(++budget.requests>64||(budget.bytes+=span.length)>2*1024*1024)throw Error('DASH index tree exceeds its request or byte limit.');
   indexes.push(span);
   const response=await read(span);
   if(total!==undefined&&response.total!==total)throw Error('DASH media changed while reading its indexes.');
   total=response.total;
   if(response.bytes.length!==span.length)throw Error('The DASH child index response was truncated.');
   const view=new DataView(response.bytes.buffer,response.bytes.byteOffset,response.bytes.byteLength),sidx=[];let at=0;
   while(at<response.bytes.length){
    if(response.bytes.length-at<8)throw Error('Truncated DASH index box.');
    let length=view.getUint32(at),header=8;const type=String.fromCharCode(...response.bytes.subarray(at+4,at+8));
    if(length===1){if(response.bytes.length-at<16)throw Error('Truncated DASH index header.');length=view.getUint32(at+8)*4294967296+view.getUint32(at+12);header=16;}
    if(!Number.isSafeInteger(length)||length<header||length>response.bytes.length-at)throw Error('Invalid DASH index box size.');
    if(type==='sidx')sidx.push({at,length});at+=length;
   }
   if(!sidx.length)throw Error('The DASH range contains no segment index.');
   if(sidx.length>64)throw Error('DASH index range has too many roots.');
   let previous;
   for(const box of sidx){
    const data=response.bytes.subarray(box.at,box.at+box.length),info=sidxInfo(data,span.start+box.at,total);
    if(previous&&(info.referenceId!==previous.referenceId||!equalTime(info.earliest,info.timescale,previous.end,previous.timescale)))throw Error('Independent DASH index identity or timing is discontinuous.');
    if(!previous&&parent&&(info.referenceId!==parent.referenceId||!equalTime(info.earliest,info.timescale,parent.time,parent.timescale)))throw Error('DASH child index identity or timing does not match its parent.');
    if((budget.references+=info.parts.length)>2*MAX)throw Error('DASH index tree has too many references.');
    for(const part of info.parts){
     if(part.kind==='index')await visit(part,depth+1,{referenceId:info.referenceId,timescale:info.timescale,time:part.time,duration:part.duration});
     else{
      if(indexes.some(x=>overlaps(x,part))||media.some(x=>overlaps(x,part))||(ranges.length&&part.start<ranges.at(-1).start+ranges.at(-1).length))throw Error('DASH media ranges overlap or are out of order.');
      if(ranges.length>=MAX-1)throw Error('This playlist exceeds the current segment limit.');
      media.push(part);ranges.push({start:part.start,length:part.length});
     }
    }
    previous={referenceId:info.referenceId,timescale:info.timescale,end:info.earliest+info.duration};
   }
   if(parent&&!equalTime(previous.end,previous.timescale,parent.time+parent.duration,parent.timescale))throw Error('DASH child index identity or timing does not match its parent.');
  }
  await visit(spec,0,null);return ranges;
 }

 // External indexes have two address spaces: child boxes follow their parent in
 // the index file; first_offset and media continuity belong to the media file.
 async function resolveExternalSidx(spec,initial,mediaSize,read,budget={requests:0,bytes:0,references:0}){
  const total=initial.total,start=spec.start??0,chunks=[],boxes=[],ranges=[];
  if(!Number.isSafeInteger(mediaSize)||mediaSize<1||!(initial.bytes instanceof Uint8Array)||!initial.bytes.length||initial.bytes.length>524288||!Number.isSafeInteger(total)||total<1||!Number.isSafeInteger(start)||start<0||!Number.isSafeInteger(start+initial.bytes.length)||start+initial.bytes.length>total||(spec.length!==undefined&&spec.length!==initial.bytes.length)||(spec.start===undefined&&initial.bytes.length!==total))throw Error('Invalid external DASH index bounds.');
  const overlaps=(a,b)=>a.start<b.start+b.length&&b.start<a.start+a.length;
  const equalTime=(a,scaleA,b,scaleB)=>BigInt(a)*BigInt(scaleB)===BigInt(b)*BigInt(scaleA);
  chunks.push({start,bytes:initial.bytes});
  async function bytesFor(span){
   const cached=chunks.find(c=>span.start>=c.start&&span.start+span.length<=c.start+c.bytes.length);
   if(cached)return cached.bytes.subarray(span.start-cached.start,span.start-cached.start+span.length);
   // Partial overlap would mix snapshots of the same index bytes.
   if(chunks.some(c=>overlaps(span,{start:c.start,length:c.bytes.length})))throw Error('External DASH index reads partially overlap.');
   if(++budget.requests>64||(budget.bytes+=span.length)>2*1024*1024)throw Error('DASH index tree exceeds its request or byte limit.');
   const response=await read(span);
   if(response.total!==total)throw Error('DASH index file changed while reading its children.');
   if(!(response.bytes instanceof Uint8Array)||response.bytes.length!==span.length)throw Error('The DASH child index response was truncated.');
   chunks.push({start:span.start,bytes:response.bytes});return response.bytes;
  }
  async function visit(span,depth,parent,expectedStart){
   if(depth>8)throw Error('DASH index nesting exceeds eight levels.');
   if(!Number.isSafeInteger(span.start)||span.start<0||!Number.isSafeInteger(span.length)||span.length<8||span.length>524288||!Number.isSafeInteger(span.start+span.length)||span.start+span.length>total)throw Error('Invalid external DASH child index bounds.');
   const data=await bytesFor(span),info=sidxInfo(data,span.start,total,mediaSize,true);
   if(parent&&(info.box.start!==span.start||info.referenceId!==parent.referenceId||!equalTime(info.earliest,info.timescale,parent.time,parent.timescale)||!equalTime(info.duration,info.timescale,parent.duration,parent.timescale)))throw Error('DASH child index identity or timing does not match its parent.');
   if(boxes.some(b=>overlaps(b,info.box)))throw Error('External DASH index boxes overlap or repeat.');
   boxes.push(info.box);
   if(expectedStart!==undefined&&info.firstOffset!==expectedStart)throw Error('External DASH media is not contiguous with its parent.');
   if((budget.references+=info.parts.length)>2*MAX)throw Error('DASH index tree has too many references.');
   let cursor=info.firstOffset;
   for(const part of info.parts){
    if(part.kind==='index')cursor=await visit(part,depth+1,{referenceId:info.referenceId,timescale:info.timescale,time:part.time,duration:part.duration},cursor);
    else{
     if(!Number.isSafeInteger(cursor+part.length)||cursor+part.length>mediaSize||(ranges.length&&cursor!==ranges.at(-1).start+ranges.at(-1).length))throw Error('External DASH media ranges overlap, have gaps or exceed the media file.');
     if(ranges.length>=MAX-1)throw Error('This playlist exceeds the current segment limit.');
     ranges.push({start:cursor,length:part.length});cursor+=part.length;
    }
   }
   return cursor;
  }
  const indexBoxes=chunk=>{
   const found=[];
   const view=new DataView(chunk.bytes.buffer,chunk.bytes.byteOffset,chunk.bytes.byteLength);let at=0;
   while(at<chunk.bytes.length){
    if(chunk.bytes.length-at<8)throw Error('Truncated external DASH index box.');
    let length=view.getUint32(at),header=8;const type=String.fromCharCode(...chunk.bytes.subarray(at+4,at+8));
    if(length===1){if(chunk.bytes.length-at<16)throw Error('Truncated external DASH index header.');length=view.getUint32(at+8)*4294967296+view.getUint32(at+12);header=16;}
    if(!Number.isSafeInteger(length)||length<header||length>chunk.bytes.length-at)throw Error('Invalid external DASH index box size.');
    if(type==='sidx')found.push({start:chunk.start+at,length});
    at+=length;
   }
   return found;
  };
  const roots=indexBoxes(chunks[0]);if(!roots.length)throw Error('The DASH range contains no segment index.');
  if(roots.length>64)throw Error('External DASH index has too many roots.');
  let cursor,previous;
  for(const span of roots){
   if(boxes.some(b=>b.start===span.start&&b.length===span.length))continue;
   const bytes=initial.bytes.subarray(span.start-start,span.start-start+span.length);
   const info=sidxInfo(bytes,span.start,total,mediaSize,true);
   if(previous&&(info.referenceId!==previous.referenceId||!equalTime(info.earliest,info.timescale,previous.end,previous.timescale)))throw Error('Independent DASH index identity or timing is discontinuous.');
   cursor=await visit(span,0,null,cursor);
   previous={referenceId:info.referenceId,timescale:info.timescale,end:info.earliest+info.duration};
  }
  // Every SIDX in each fetched range must belong to a validated root or child.
  for(const chunk of chunks)for(const span of indexBoxes(chunk)){
   if(!boxes.some(b=>b.start===span.start&&b.length===span.length))throw Error('Unreferenced external DASH index.');
  }
  return ranges;
 }

 // DASH frameRate is an integer or rational; keep the numeric value precise
 // for stream identity and round only the menu presentation.
 function dashFrameRate(value){
  const m=/^(\d+)(?:\/(\d+))?$/.exec(value||'');if(!m)return 0;
  const numerator=Number(m[1]),denominator=Number(m[2]||1);
  return Number.isSafeInteger(numerator)&&Number.isSafeInteger(denominator)&&numerator>0&&denominator>0?numerator/denominator:0;
 }
 // Resolve Period clocks before expanding any segment addresses. A missing duration
 // is different from an explicitly empty Period; never turn zero into MPD duration.
 function dashPeriodLayout(mpd){
  const read=(value,label)=>{
   if(value===undefined)return null;
   if(!/^P(?=.+)(?:(\d+(?:\.\d+)?)D)?(?:T(?=.+)(?:(\d+(?:\.\d+)?)H)?(?:(\d+(?:\.\d+)?)M)?(?:(\d+(?:\.\d+)?)S)?)?$/.test(value))throw Error('Invalid DASH '+label+'.');
   const seconds=duration(value);if(!Number.isFinite(seconds)||seconds<0||seconds>7*86400)throw Error('DASH '+label+' exceeds the supported timeline.');return seconds;
  };
  const nodes=children(mpd,'Period');if(!nodes.length||nodes.length>128)throw Error('DASH requires between 1 and 128 Periods.');
  const total=read(mpd.attrs.mediaPresentationDuration,'presentation duration'),ids=new Set();
  const layout=nodes.map((node,index)=>{
   const id=node.attrs.id;if(id!==undefined){if(ids.has(id))throw Error('Duplicate DASH Period identity.');ids.add(id);}
   return {node,index,id:id??null,start:read(node.attrs.start,'Period start'),duration:read(node.attrs.duration,'Period duration')};
  });
  if(layout[0].start===null)layout[0].start=0;
  for(let i=0;i<layout.length;i++){
   const item=layout[i],previous=layout[i-1],next=layout[i+1];
   if(item.start===null&&previous&&previous.duration!==null)item.start=previous.start+previous.duration;
   if(item.start===null)throw Error('DASH Period start cannot be resolved.');
   if(item.duration===null){
    if(next&&next.start!==null)item.duration=next.start-item.start;
    else if(!next&&total!==null)item.duration=total-item.start;
   }
   if(item.duration!==null&&(item.duration<0||item.start+item.duration>7*86400))throw Error('Invalid DASH Period interval.');
   if(previous&&previous.duration!==null&&Math.abs(item.start-previous.start-previous.duration)>0.000001)throw Error('DASH Period gaps or overlaps need timeline assembly support.');
   if(layout.length>1&&item.duration===null)throw Error('DASH Period duration cannot be resolved.');
  }
  const last=layout.at(-1);if(total!==null&&last.duration!==null&&Math.abs(last.start+last.duration-total)>0.000001)throw Error('DASH presentation and Period durations disagree.');
  const active=layout.filter(item=>item.duration!==0);if(!active.length)throw Error('DASH presentation contains no nonempty Period.');return active;
 }
 function dashRoot(text){
  const mpd=xml(text);if(mpd.name!=='MPD'||(mpd.attrs.type&&!['static','OnDemand'].includes(mpd.attrs.type)))throw Error('Only recorded DASH presentations are supported.');
  function protectedNode(n){return n.name==='ContentProtection'||n.children.some(protectedNode);}if(protectedNode(mpd))throw Error('Encrypted or DRM-protected streams are not supported.');return mpd;
 }
 function dash(text,base){
  const mpd=dashRoot(text),periods=dashPeriodLayout(mpd);if(periods.length!==1)throw Error('Multi-period DASH needs native Period assembly support.');
  return dashPeriod(mpd,periods[0].node,base,periods[0].duration??0);
 }
 // Separate per-Period catalogs retain inherited MPD/Period addressing without
 // flattening initialization data. Native assembly must opt in before handoff.
 function dashPresentation(text,base){
  const mpd=dashRoot(text),layout=dashPeriodLayout(mpd),periods=[];let bytes=0;
  for(const item of layout){
   const choices=dashPeriod(mpd,item.node,base,item.duration??0,true);
   const entry={index:item.index,id:item.id,start:item.start,duration:item.duration,choices};
   bytes+=new TextEncoder().encode(JSON.stringify(entry)).length;if(bytes>MAX_PLAN_BYTES)throw Error('DASH Period catalogs exceed the current browser budget.');periods.push(entry);
  }
  return {type:'dash',periods};
 }
 // Construct a complete selection without assuming Representation IDs survive a
 // Period transition. This draft is not a native handoff until assembly is wired.
 function dashPeriodSelection(presentation,options={}){
  const periods=presentation?.periods;if(presentation?.type!=='dash'||!Array.isArray(periods)||!periods.length||periods.length>128)throw Error('Invalid DASH Period catalog.');
  const first=periods[0],quality=options.qualityIndex??0;
  if(!Number.isInteger(quality)||quality<0||quality>=first.choices.length)throw Error('DASH quality is no longer available.');
  const choice=first.choices[quality],audioOnly=!!choice.onlyAudio||options.audioOnly===true;
  const audioOptions=choice.audioOptions||[],audio=options.audioKey===undefined?(audioOptions.find(x=>x.default)||audioOptions[0]):audioOptions.find(x=>x.key===options.audioKey);
  if(options.audioKey!==undefined&&!audio)throw Error('DASH audio track is no longer available.');
  const subtitle=options.subtitleKey?(choice.subtitleOptions||[]).find(x=>x.key===options.subtitleKey):null;
  if(options.subtitleKey&&!subtitle)throw Error('DASH subtitle track is no longer available.');
  if(audioOnly&&(subtitle||!audio))throw Error('Invalid DASH audio-only selection.');
  const selected=[...(audioOnly?[]:[choice.plan.tracks[0]]),...(audio?[audio.track]:[]),...(subtitle?[subtitle.track]:[])];
  const fingerprint=track=>{
   const s=track.selection;if(!s||(track.kind!=='subtitle'&&!s.codecs))throw Error('DASH Period matching requires declared codecs.');
   return JSON.stringify([track.kind,s.mime,s.codecs,track.height,s.width,track.frameRate,(track.language||'').trim().toLowerCase(),s.roles,[...s.channels].sort((a,b)=>JSON.stringify(a).localeCompare(JSON.stringify(b))),s.sampleRate,s.label]);
  };
  const match=(seed,period)=>{
   const candidates=new Set();for(const c of period.choices){for(const t of c.plan.tracks)candidates.add(t);for(const o of [...(c.audioOptions||[]),...(c.subtitleOptions||[])])candidates.add(o.track);}
   const identity=fingerprint(seed),compatible=[...candidates].filter(t=>t.kind===seed.kind&&t.selection&&(t.kind==='subtitle'||t.selection.codecs)&&fingerprint(t)===identity);
   if(!compatible.length)throw Error('The selected DASH '+seed.kind+' track is missing or changes format in Period '+(period.index+1)+'.');
   const exact=compatible.filter(t=>seed.selection.representationId!==null&&t.selection.representationId===seed.selection.representationId&&t.selection.adaptationId===seed.selection.adaptationId);
   const pool=exact.length?exact:compatible;
   const distance=t=>Math.abs(t.bandwidth-seed.bandwidth);pool.sort((a,b)=>distance(a)-distance(b));
   if(pool.length>1&&distance(pool[0])===distance(pool[1]))throw Error('DASH track matching is ambiguous in Period '+(period.index+1)+'.');return pool[0];
  };
  let segments=0;
  const result={type:'dash',audioOnly,height:audioOnly?0:choice.height,container:audioOnly?'m4a':'mp4',audioExpected:!!audio,periods:periods.map((period,i)=>{
   if(!Number.isFinite(period.start)||period.start<0||!Number.isFinite(period.duration)||period.duration<=0)throw Error('DASH Period assembly needs finite timing.');
   const tracks=selected.map(seed=>i?match(seed,period):seed);for(const track of tracks){fingerprint(track);segments+=track.segments.length;}
   if(segments>MAX)throw Error('This DASH selection exceeds the current segment limit.');
   return {index:period.index,id:period.id,start:period.start,duration:period.duration,tracks};
  })};
  if(new TextEncoder().encode(JSON.stringify(result)).length>MAX_PLAN_BYTES)throw Error('This DASH selection exceeds the current handoff budget.');return result;
 }
 function dashPeriod(mpd,period,base,seconds,describe=false){
  const tracks=[];
  for(const set of children(period,'AdaptationSet'))for(const rep of children(set,'Representation')){
   const lineage=[mpd,period,set,rep];let target=base;
   for(const node of lineage){const bases=children(node,'BaseURL');if(bases.length>1)throw Error('Multiple DASH base URLs need an explicit selection.');if(bases[0])target=url(bases[0].text.trim(),target);}
   const mime=rep.attrs.mimeType||set.attrs.mimeType||'',type=mime==='text/vtt'?'subtitle':rep.attrs.contentType||set.attrs.contentType||mime.split('/')[0];if(!['video','audio','subtitle'].includes(type))continue;
   if(mime&&!/^(video|audio)\/mp4$/.test(mime)&&mime!=='text/vtt')continue;
   let template={},timeline=null,list=null,segmentBase=null,initialization=null,representationIndex=null;
   for(const n of lineage){const t=child(n,'SegmentTemplate');if(t){template={...template,...t.attrs};timeline=child(t,'SegmentTimeline')||timeline;}const lists=children(n,'SegmentList');if(lists.length>1)throw Error('Multiple DASH segment lists are ambiguous.');if(lists[0]){const next=lists[0];if(children(next,'Initialization').length>1)throw Error('Multiple DASH initialization segments are ambiguous.');const replaced=new Set(next.children.map(c=>c.name));list={...next,attrs:{...(list?.attrs||{}),...next.attrs},children:[...(list?.children||[]).filter(c=>!replaced.has(c.name)),...next.children]};}const b=child(n,'SegmentBase');if(b){if(children(n,'SegmentBase').length!==1||children(b,'RepresentationIndex').length>1)throw Error('Multiple DASH index locators are ambiguous.');segmentBase={...segmentBase,...b.attrs};initialization=child(b,'Initialization')||initialization;representationIndex=child(b,'RepresentationIndex')||representationIndex;}}
   if(type==='subtitle'&&(segmentBase||list||template.media||template.initialization))continue; // Standalone WebVTT; segmented XML/MP4 text requires a separate decoder.
   const segments=[];let index;const push=x=>{segments.push(x);if(segments.length>MAX)throw Error('This playlist exceeds UDM’s current segment limit.');};
   const inclusive=value=>{const m=/^(\d+)-(\d+)$/.exec(value||'');if(!m||Number(m[2])<Number(m[1]))throw Error('Invalid DASH range.');return range((Number(m[2])-Number(m[1])+1)+'@'+m[1],0);};
   if(segmentBase){
    if(list||template.media||template.initialization)throw Error('Conflicting DASH segment addressing modes.');
    if(target===base||(!segmentBase.indexRange&&!representationIndex)||!initialization)throw Error('DASH indexed media needs a file URL, index locator and initialization.');
    if(representationIndex&&segmentBase.indexRange)throw Error('Conflicting DASH index locators.');
    const indexUrl=representationIndex?url(representationIndex.attrs.sourceURL||target,target):target;
    const indexRange=representationIndex?representationIndex.attrs.range:segmentBase.indexRange;
    if(indexUrl===target&&!indexRange)throw Error('An in-file DASH index needs a byte range.');
    const span=indexRange?inclusive(indexRange):null;if(span&&span.length>524288)throw Error('DASH index exceeds 512 KiB.');
    index={url:indexUrl,...(span||{}),...(indexUrl!==target?{mediaUrl:target}:{})};const initialUrl=url(initialization.attrs.sourceURL||target,target);
    if(!initialization.attrs.range&&initialUrl===target)throw Error('DASH initialization range is missing.');
    push({url:initialUrl,...(initialization.attrs.range?inclusive(initialization.attrs.range):{})});
   }
   else if(list){const init=child(list,'Initialization');if(init)push({url:url(init.attrs.sourceURL||target,target),...(init.attrs.range?inclusive(init.attrs.range):{})});for(const s of children(list,'SegmentURL'))push({url:url(s.attrs.media||target,target),...(s.attrs.mediaRange?inclusive(s.attrs.mediaRange):{})});}
   else if(template.media){
    const timescale=Number(template.timescale||1),start=Number(template.startNumber||1),offset=Number(template.presentationTimeOffset||0);if(!Number.isSafeInteger(timescale)||timescale<1||!Number.isSafeInteger(start)||start<0||!Number.isSafeInteger(offset)||offset<0)throw Error('Invalid DASH timing.');
    // Validate source tokens while expanding; escaped literal dollars must not be reparsed.
    function expand(pattern,number,time){
     const result=pattern.replace(/\$\$|\$(RepresentationID|Bandwidth|Number|Time)(?:%0(\d+)([diouxX]))?\$|\$/g,(all,key,width,format)=>{
      if(all==='$$')return '$';if(all==='$')throw Error('Unsupported DASH template.');
      const value={RepresentationID:rep.attrs.id,Bandwidth:rep.attrs.bandwidth,Number:number,Time:time}[key];
      if(value===undefined)throw Error('Missing DASH template value.');
      if(width&&Number(width)>12)throw Error('Invalid DASH template width.');
      if(key==='RepresentationID')return String(value);
      const numeric=Number(value);if(!Number.isSafeInteger(numeric)||numeric<0)throw Error('Invalid DASH numeric template value.');
      const radix=format==='o'?8:/^[xX]$/.test(format||'')?16:10;
      let text=numeric.toString(radix);if(format==='X')text=text.toUpperCase();
      return width?text.padStart(Number(width),'0'):text;
     });return url(result,target);
    }
    if(template.initialization)push({url:expand(template.initialization,start,0)});
    let number=start;
    if(timeline){let time=0;const entries=children(timeline,'S');for(let i=0;i<entries.length;i++){const a=entries[i].attrs;time=a.t===undefined?time:Number(a.t);const d=Number(a.d),r=Number(a.r||0);if(!Number.isSafeInteger(time)||time<0||!Number.isSafeInteger(d)||d<1||!Number.isSafeInteger(r)||r< -1)throw Error('Invalid DASH timeline.');
      const boundary=entries[i+1]?.attrs.t!==undefined?Number(entries[i+1].attrs.t):seconds*timescale+offset;const repeat=r===-1?Math.ceil((boundary-time)/d)-1:r;if(repeat<0||repeat>MAX||!Number.isFinite(repeat))throw Error('Unbounded DASH timeline.');for(let n=0;n<=repeat;n++){push({url:expand(template.media,number++,time)});time+=d;if(!Number.isSafeInteger(time))throw Error('DASH timeline overflow.');}
    }}else{const d=Number(template.duration);if(!seconds||!Number.isSafeInteger(d)||d<1)throw Error('DASH duration is missing.');const count=Math.ceil(seconds*timescale/d);if(count>MAX)throw Error('This playlist exceeds UDM’s current segment limit.');for(let i=0;i<count;i++)push({url:expand(template.media,number++,i*d)});}
   }else if(target!==base)push({url:target});else continue;
   if(!segments.length)continue;
   const roles=children(set,'Role').map(x=>x.attrs.value).filter(Boolean),name=(child(rep,'Label')||child(set,'Label'))?.text.trim()||roles.filter(x=>x!=='main').join(', ')||(type==='subtitle'?'Subtitles':set.attrs.lang?'Audio':'Audio '+(tracks.filter(t=>t.kind==='audio').length+1));
   // Selection metadata is confined to the new Period catalog contract. Legacy
   // single-Period plans retain their existing refresh identity byte for byte.
   const selection=describe?{
    mime,codecs:(rep.attrs.codecs||set.attrs.codecs||'').trim().toLowerCase(),
    width:Number(rep.attrs.width||set.attrs.width)||0,
    roles:children(set,'Role').map(x=>[x.attrs.schemeIdUri||'',x.attrs.value||'']).sort((a,b)=>JSON.stringify(a).localeCompare(JSON.stringify(b))),
    channels:children(rep,'AudioChannelConfiguration').length?children(rep,'AudioChannelConfiguration').map(x=>[x.attrs.schemeIdUri||'',x.attrs.value||'']):children(set,'AudioChannelConfiguration').map(x=>[x.attrs.schemeIdUri||'',x.attrs.value||'']),
    sampleRate:rep.attrs.audioSamplingRate||set.attrs.audioSamplingRate||'',
    label:(child(rep,'Label')||child(set,'Label'))?.text.trim()||'',
    adaptationId:set.attrs.id??null,representationId:rep.attrs.id??null
   }:null;
   const addressing=segmentBase||list||template;
   const clock=describe?{timescale:Number(addressing.timescale||1),presentationTimeOffset:Number(addressing.presentationTimeOffset||0)}:null;
   if(clock&&(!Number.isSafeInteger(clock.timescale)||clock.timescale<1||!Number.isSafeInteger(clock.presentationTimeOffset)||clock.presentationTimeOffset<0))throw Error('Invalid DASH Period media clock.');
   tracks.push({kind:type,id:rep.attrs.id||String(tracks.length),name,main:roles.includes('main'),height:Number(rep.attrs.height||set.attrs.height)||0,frameRate:type==='video'?dashFrameRate(rep.attrs.frameRate??set.attrs.frameRate):0,bandwidth:Number(rep.attrs.bandwidth)||0,language:rep.attrs.lang||set.attrs.lang||'',segments,...(index?{index}:{}),...(describe?{selection,clock}:{})});
  }
  const audioTracks=tracks.filter(t=>t.kind==='audio').sort((a,b)=>Number(b.main)-Number(a.main)||b.bandwidth-a.bandwidth);
  if(audioTracks.length>32)throw Error('This DASH presentation exceeds the 32-audio-track limit.');
  const audioOptions=audioTracks.map((track,i)=>({key:'audio-'+i,name:track.name,language:track.language,label:audioLabel(track,i),default:i===0,track}));
  const subtitleTracks=tracks.filter(t=>t.kind==='subtitle');if(subtitleTracks.length>32)throw Error('Too many subtitle tracks.');const subtitleOptions=subtitleTracks.map((track,i)=>({key:'subtitle-'+i,name:track.name,language:track.language,label:audioLabel(track,i),track}));
  const audio=audioTracks[0];
  const choices=tracks.filter(t=>t.kind==='video').map(v=>({height:v.height,bandwidth:v.bandwidth,frameRate:v.frameRate,audioOptions,subtitleOptions,label:(v.height?v.height+'p':'Original quality')+' · DASH'+(audio?' · '+(audio.language||'audio'):''),plan:{type:'dash',height:v.height,audioExpected:!!audio,...(Number.isFinite(seconds)&&seconds>0&&seconds<=7*86400?{durationMs:Math.max(1,Math.round(seconds*1000))}:{}),tracks:[v,...(audio?[audio]:[])]}}));
  // Audio-only MPDs use the same validated segment/index path and native M4A muxer.
  if(!choices.length&&audio)choices.push({height:0,bandwidth:audio.bandwidth,onlyAudio:true,audioOptions,subtitleOptions:[],label:'Audio only \u00b7 DASH',plan:{type:'dash',height:0,audioExpected:true,audioOnly:true,container:'m4a',...(Number.isFinite(seconds)&&seconds>0&&seconds<=7*86400?{durationMs:Math.max(1,Math.round(seconds*1000))}:{}),tracks:[audio]}});
  if(!choices.length)throw Error('No supported MP4 audio or video representations were found.');return choices.sort((a,b)=>b.height-a.height||b.bandwidth-a.bandwidth);
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
   return (settings.excludedUrls||[]).some(pattern=>{if(typeof pattern!=='string'||pattern.length>16385)return false;if(pattern.startsWith('=')){try{const exact=new URL(pattern.slice(1));if(!/^https?:$/.test(exact.protocol)||exact.username||exact.password)return false;exact.hash='';const current=new URL(u.href);current.hash='';return current.href===exact.href;}catch{return false;}}if(pattern.length>2048)return false;const m=/^(https?):\/\/(\*\.)?((?:[a-z0-9.-]+|\[[a-f0-9:.]+\])(?::\d{1,5})?)(\/[^\s#\\]*)$/i.exec(pattern);if(!m)return false;
    if(u.protocol!==m[1].toLowerCase()+':'||!(u.host===m[3].toLowerCase()||(m[2]&&u.host.endsWith('.'+m[3].toLowerCase()))))return false;
    const expression=m[4].split('*').map(p=>p.replace(/[.*+?^{}$()|[\]\\]/g,'\\$&')).join('.*');return new RegExp('^'+expression+'$').test(u.pathname+u.search);
   });}catch{return true;}},
  key(event,name){
   if(typeof name!=='string'||name==='None')return false;const parts=name.split('+'),allowed=['Alt','Ctrl','Shift','Ins','Del'];
   if(!parts.length||parts.length>4||new Set(parts).size!==parts.length||parts.some(k=>!allowed.includes(k))||event.metaKey)return false;
   return allowed.every(k=>parts.includes(k)===!!event[{Alt:'altKey',Ctrl:'ctrlKey',Shift:'shiftKey',Ins:'insertKey',Del:'deleteKey'}[k]]);
  },
  intent(event,settings={}){if(this.key(event,settings.bypassKey||'Alt'))return 'bypass';if(this.key(event,settings.forceKey||'Ctrl'))return 'force';return '';},
  panelBlocked(address,settings={}){if(this.blocked(address,settings))return true;try{const host=new URL(address).hostname;return (settings.panelExcluded||[]).some(pattern=>typeof pattern==='string'&&pattern.length<=253&&/^[a-z0-9*.-]+$/i.test(pattern)&&new RegExp('^'+pattern.split('*').map(s=>s.replace(/[.*+?^${}()|[\]\\]/g,'\\$&')).join('.*')+'$','i').test(host));}catch{return true;}},
  mediaType(choice={}){if(choice.container)return String(choice.container).toLowerCase();try{const ext=/\.([a-z0-9]{1,16})$/i.exec(new URL(choice.url).pathname)?.[1];if(ext)return ext.toLowerCase();}catch{}
   return {'video/mp4':'mp4','video/webm':'webm','video/ogg':'ogv','video/quicktime':'mov','audio/mp4':'m4a','audio/mpeg':'mp3','audio/aac':'aac','audio/ogg':'ogg','audio/wav':'wav','audio/webm':'webm'}[String(choice.mime||'').split(';')[0].toLowerCase()]||'mp4';},
  panelAllowed(choice,settings={}){const type=this.mediaType(choice),types=settings.panelTypes;
   if(types&&(!Object.prototype.hasOwnProperty.call(types,type)||types[type]!==true))return false;
   const min=Number(settings.panelMinKb?.[type]),size=Number(choice.size);return !(Number.isFinite(min)&&min>0&&Number.isSafeInteger(size)&&size>0&&size<min*1024);
  },
  responseSize(event){const header=name=>event.responseHeaders?.find(h=>h.name.toLowerCase()===name)?.value||'';
   if(header('content-encoding')&&!/^identity$/i.test(header('content-encoding')))return 0;
   const range=/^bytes (\d+)-(\d+)\/(\d+)$/i.exec(header('content-range'));
   const value=event.statusCode===206&&range&&Number(range[1])<=Number(range[2])&&Number(range[2])<Number(range[3])?range[3]:event.statusCode===200?header('content-length'):'';
   const n=Number(value);return /^\d+$/.test(value)&&Number.isSafeInteger(n)&&n>0?n:0;
  },
  webResource(item){const mime=String(item.mime||'').split(';')[0].toLowerCase();if(/^(text\/(html|css|javascript)|application\/(xhtml\+xml|javascript|x-javascript|json)|image\/)/.test(mime))return true;
   try{return /\.(html?|xhtml|css|[cm]?js|json|svg|png|jpe?g|gif|webp|ico|avif)$/i.test((item.filename||new URL(item.url).pathname).split(/[\\/]/).pop());}catch{return true;}
  },
  merge(local={},desktop={}){const p={...local,...desktop};p.capture=!!local.capture;p.cookies=!!local.cookies;p.excluded=[...(local.excluded||[]),...(desktop.excluded||[])];p.excludedUrls=[...(local.excludedUrls||[]),...(desktop.excludedUrls||[])];return p;}
 };

 const exported={feedMediaFragment,twitterAsset,url,fullHlsUrl,kind,hls,hlsAudioPlaylist,hlsCatalog,hlsAudio,hlsSubtitles,audioLabel,dash,dashPresentation,dashPeriodSelection,sidx,sidxInfo,sidxExternal,resolveSidx,resolveExternalSidx,placement,policy,MAX,MAX_PLAN_BYTES,validatePlanBudget};root.UdmMedia=exported;if(typeof module!=='undefined')module.exports=exported;
})(globalThis);
