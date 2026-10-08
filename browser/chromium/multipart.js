/* Bounded form replay, verified against the browser's original request envelope. */
(function(root){
 'use strict';
 const LIMIT=4*1024*1024;
 const crlf=value=>value.replace(/\r\n|\r|\n/g,'\r\n');
 const escape=value=>value.replace(/[\r\n"]/g,c=>c==='\r'?'%0D':c==='\n'?'%0A':'%22');
 const field=value=>escape(crlf(value));
 const text=value=>new TextEncoder().encode(value);
 function binary(value){
  if(typeof value!=='string'||value.length>Math.ceil(LIMIT/3)*4||value.length%4||!/^[A-Za-z0-9+/]*={0,2}$/.test(value))return null;
  try{const decoded=atob(value);if(decoded.length>LIMIT||btoa(decoded)!==value)return null;return Uint8Array.from(decoded,c=>c.charCodeAt(0));}catch{return null;}
 }
 function base64(bytes){
  // Indexed reads avoid Firefox's wrapped typed-array methods and iterators.
  const chunks=[];for(let i=0;i<bytes.length;i+=8192){let chunk='';const end=Math.min(i+8192,bytes.length);for(let j=i;j<end;j++)chunk+=String.fromCharCode(bytes[j]);chunks.push(chunk);}
  return btoa(chunks.join(''));
 }
 function fileValue(value){
  if(!value||typeof value!=='object'||Array.isArray(value)||value.kind!=='file'||Object.keys(value).some(k=>!['kind','name','type','body'].includes(k)))return null;
  if(typeof value.name!=='string'||value.name.length>32768||typeof value.type!=='string'||value.type.length>256||/[^\x20-\x7e]/.test(value.type))return null;
  const bytes=binary(value.body);return bytes?{value:{kind:'file',name:value.name,type:value.type,body:value.body},bytes}:null;
 }
 function capture(input){
  if(!Array.isArray(input)||input.length>16384)return null;
  let size=0,files=0;const fields=[];
  for(const pair of input){
   if(!Array.isArray(pair)||pair.length!==2||typeof pair[0]!=='string')return null;
   const [key,value]=pair;
   if(typeof value==='string'){size+=text(key+value).length;fields.push([key,value]);}
   else{const file=fileValue(value);if(!file)return null;size+=text(key+file.value.name+file.value.type).length+file.bytes.length;fields.push([key,file.value]);files++;}
   if(size>LIMIT)return null;
  }
  return {fields,size,files};
 }
 // Called only for a trusted submitted form in the isolated content script.
 // Check aggregate sizes before reading any user-selected File. No filesystem paths are read.
 async function captureEntries(input){
  if(!Array.isArray(input)||input.length>16384)return null;
  let size=0;const entries=[];
  for(const pair of input){
   if(!Array.isArray(pair)||pair.length!==2||typeof pair[0]!=='string')return null;
   const [key,value]=pair;
   if(typeof value==='string')size+=text(key+value).length;
   else if(typeof File!=='undefined'&&value instanceof File&&Number.isSafeInteger(value.size)&&value.size>=0)size+=text(key+value.name+value.type).length+value.size;
   else return null;
   if(size>LIMIT)return null;entries.push([key,value]);
  }
  const fields=[];
  try{for(const [key,value] of entries){
   if(typeof value==='string')fields.push([key,value]);
   else{const bytes=new Uint8Array(await Blob.prototype.arrayBuffer.call(value));if(bytes.length!==value.size)return null;fields.push([key,{kind:'file',name:value.name,type:value.type,body:base64(bytes)}]);}
  }}catch{return null;}
  return capture(fields);
 }
 function boundary(type){
  const m=/^multipart\/form-data\s*;\s*boundary=(?:"([A-Za-z0-9'()+_,./:=? -]{1,70})"|([A-Za-z0-9'()+_,./:=?-]{1,70}))\s*$/i.exec(type||'');
  const value=m&&(m[1]||m[2]);return value&&!value.endsWith(' ')?value:'';
 }
 function matches(snapshot,observed,decodeFilenames=true){
  if(!snapshot||!Array.isArray(observed))return false;
  const a=new Map(),b=new Map();
  // Both browsers decode field names. Firefox alone preserves escaped filenames.
  const decode=value=>decodeURIComponent(value.replace(/%(?![0-9a-f]{2})/ig,'%25'));
  for(const [key,value] of snapshot.fields){let k,v;try{k=decode(field(key));v=typeof value==='string'?crlf(value):decodeFilenames?decode(escape(value.name)):escape(value.name);}catch{return false;}if(!a.has(k))a.set(k,[]);a.get(k).push(v);}
  for(const [key,value] of observed){if(!b.has(key))b.set(key,[]);b.get(key).push(value);}
  if(a.size!==b.size)return false;
  for(const [key,values] of a){const other=b.get(key);if(!other||other.length!==values.length||values.some((v,i)=>v!==other[i]))return false;}
  return true;
 }
 function sameEncoding(a,b){
  return !!a&&!!b&&a.fields.length===b.fields.length&&a.fields.every(([key,value],i)=>{
   const [otherKey,other]=b.fields[i];if(field(key)!==field(otherKey)||typeof value!==typeof other)return false;
   return typeof value==='string'?crlf(value)===crlf(other):escape(value.name)===escape(other.name)&&(value.type||'application/octet-stream')===(other.type||'application/octet-stream')&&value.body===other.body;
  });
 }
 function identity(pairs){
  const grouped=new Map();for(const [key,value] of pairs){if(!grouped.has(key))grouped.set(key,[]);grouped.get(key).push(value);}
  return JSON.stringify([...grouped].sort(([a],[b])=>a<b?-1:a>b?1:0));
 }
 function segments(snapshot,type){
  const mark=boundary(type);if(!snapshot||!mark)return null;
  let size=0;const parts=[];
  const add=(bytes,file=false)=>{size+=bytes.length;parts.push({bytes,file});};
  for(const [name,value] of snapshot.fields){
   const head='--'+mark+'\r\nContent-Disposition: form-data; name="'+field(name)+'"';
   if(typeof value==='string')add(text(head+'\r\n\r\n'+crlf(value)+'\r\n'));
   else{const file=fileValue(value);if(!file)return null;
    add(text(head+'; filename="'+escape(value.name)+'"\r\nContent-Type: '+(value.type||'application/octet-stream')+'\r\n\r\n'));
    add(file.bytes,true);add(text('\r\n'));
   }
   if(size>LIMIT)return null;
  }
  add(text('--'+mark+'--\r\n'));return size<=LIMIT?{parts,size}:null;
 }
 function encode(snapshot,type){
  const result=segments(snapshot,type);if(!result)return null;
  const bytes=new Uint8Array(result.size);let at=0;for(const part of result.parts){bytes.set(part.bytes,at);at+=part.bytes.length;}return bytes;
 }
 function equalAt(raw,part,offset){if(offset+part.length>raw.length)return false;for(let i=0;i<part.length;i++)if(raw[offset+i]!==part[i])return false;return true;}
 function matchesRaw(snapshot,raw,type,fileParts=0){
  if(!(raw instanceof Uint8Array)||!Number.isSafeInteger(fileParts)||fileParts<0||fileParts>(snapshot?.files||0))return false;
  const result=segments(snapshot,type);if(!result)return false;
  // Chromium may omit file payloads without marking the hole. Every byte that
  // it does expose must still match: headers, text, order and all boundaries.
  let positions=new Set([0]);
  for(const part of result.parts){
   const next=new Set();for(const at of positions){if(part.file)next.add(at);if(equalAt(raw,part.bytes,at))next.add(at+part.bytes.length);}
   if(!next.size||next.size>256)return false;positions=next;
  }
  return positions.has(raw.length);
 }
 const api={capture,captureEntries,boundary,matches,matchesRaw,encode,identity,sameEncoding};root.UdmMultipart=api;if(typeof module==='object'&&module.exports)module.exports=api;
})(globalThis);
