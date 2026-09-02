'use strict';
// Read-only PE inspection. Never loads or executes reference binaries.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const reference='C:/Program Files (x86)/Internet Download Manager';
const output=path.resolve(__dirname,'../docs/reference');fs.mkdirSync(output,{recursive:true});
function inspect(name){
 const b=fs.readFileSync(path.join(reference,name)),pe=b.readUInt32LE(60);if(b.toString('ascii',pe,pe+4)!=='PE\0\0')throw Error('Invalid PE');
 const opt=pe+24,magic=b.readUInt16LE(opt),dirs=opt+(magic===0x20b?112:96),sections=[];
 const sectionStart=opt+b.readUInt16LE(pe+20);
 for(let i=0;i<b.readUInt16LE(pe+6);i++){const s=sectionStart+i*40;sections.push({name:b.toString('ascii',s,s+8).replace(/\0/g,''),va:b.readUInt32LE(s+12),size:Math.max(b.readUInt32LE(s+8),b.readUInt32LE(s+16)),raw:b.readUInt32LE(s+20)});}
 function offset(rva){const s=sections.find(s=>rva>=s.va&&rva<s.va+s.size);if(!s)throw Error('RVA outside sections');return s.raw+rva-s.va;}
 function cstr(at){const end=b.indexOf(0,at);return b.toString('ascii',at,end<0?at:end);}
 const imports=[];const imp=b.readUInt32LE(dirs+8);
 if(imp)for(let p=offset(imp);p+20<=b.length&&b.readUInt32LE(p+12);p+=20){const module=cstr(offset(b.readUInt32LE(p+12))),names=[];const thunk=b.readUInt32LE(p)||b.readUInt32LE(p+16),width=magic===0x20b?8:4;let t=offset(thunk);
  for(let n=0;n<20000;n++,t+=width){const value=width===8?b.readBigUInt64LE(t):BigInt(b.readUInt32LE(t));if(!value)break;const flag=width===8?0x8000000000000000n:0x80000000n;names.push(value&flag?'ordinal:'+Number(value&65535n):cstr(offset(Number(value))+2));}
  imports.push({module,names});
 }
 // Networking can be absent from the normal IAT and present only in delay imports.
 const delayImports=[],delay=b.readUInt32LE(dirs+13*8);
 if(delay){
  const imageBase=magic===0x20b?Number(b.readBigUInt64LE(opt+24)):b.readUInt32LE(opt+28);
  for(let p=offset(delay);p+32<=b.length&&b.readUInt32LE(p+4);p+=32){
   const attrs=b.readUInt32LE(p),rva=value=>(attrs&1)?value:value-imageBase;
   const module=cstr(offset(rva(b.readUInt32LE(p+4)))),names=[],table=b.readUInt32LE(p+16),width=magic===0x20b?8:4;
   if(table)for(let t=offset(rva(table)),n=0;n<20000;n++,t+=width){
    const value=width===8?b.readBigUInt64LE(t):BigInt(b.readUInt32LE(t));if(!value)break;
    const flag=width===8?0x8000000000000000n:0x80000000n;
    names.push(value&flag?'ordinal:'+Number(value&65535n):cstr(offset(rva(Number(value)))+2));
   }
   delayImports.push({module,names});
  }
 }
 const exports=[],ex=b.readUInt32LE(dirs);if(ex){const e=offset(ex),count=b.readUInt32LE(e+24),names=offset(b.readUInt32LE(e+32));for(let i=0;i<count;i++)exports.push(cstr(offset(b.readUInt32LE(names+4*i))));}
 const resources=[];const res=b.readUInt32LE(dirs+16);if(res){const base=offset(res);
  function walk(relative,ids,depth){if(depth>4)throw Error('Resource recursion');const d=base+relative,count=b.readUInt16LE(d+12)+b.readUInt16LE(d+14);
   for(let i=0;i<count;i++){const at=d+16+i*8,key=b.readUInt32LE(at),next=b.readUInt32LE(at+4);let id=key;if(key&0x80000000){const text=base+(key&0x7fffffff);id=b.toString('utf16le',text+2,text+2+b.readUInt16LE(text)*2);}const route=[...ids,id];if(next&0x80000000)walk(next&0x7fffffff,route,depth+1);else{const leaf=base+next;resources.push({ids:route,offset:offset(b.readUInt32LE(leaf)),size:b.readUInt32LE(leaf+4)});}}
  }walk(0,[],0);
 }
 function dialog(r){let p=r.offset;const end=p+r.size;const u16=()=>{const n=b.readUInt16LE(p);p+=2;return n;},i16=()=>{const n=b.readInt16LE(p);p+=2;return n;},u32=()=>{const n=b.readUInt32LE(p);p+=4;return n;};
  function text(){const first=u16();if(first===0xffff)return {ordinal:u16()};if(!first)return '';let value=String.fromCharCode(first);for(let n;(n=u16())!==0;)value+=String.fromCharCode(n);return value;}
  function align(){p=(p+3)&~3;}
  const extended=b.readUInt16LE(p)===1&&b.readUInt16LE(p+2)===0xffff;let style,exStyle,count,helpId;
  if(extended){p+=4;helpId=u32();exStyle=u32();style=u32();count=u16();}else{style=u32();exStyle=u32();count=u16();}
  const result={id:r.ids[1],language:r.ids[2],extended,style,exStyle,helpId,x:i16(),y:i16(),width:i16(),height:i16(),menu:text(),class:text(),title:text(),controls:[]};
  if(style&0x40){result.font={points:u16()};if(extended){result.font.weight=u16();result.font.italic=b[p++];result.font.charset=b[p++];}result.font.face=text();}
  for(let i=0;i<count;i++){align();if(p>=end)throw Error('Truncated control');let h,xs,s,id;if(extended){h=u32();xs=u32();s=u32();}else{s=u32();xs=u32();}const x=i16(),y=i16(),w=i16(),height=i16();id=extended?u32():u16();const cls=text(),title=text();const extra=u16();if(extra)p+=extra-2;result.controls.push({id,class:cls,title,x,y,width:w,height,style:s,exStyle:xs,helpId:h});}
  return result;
 }
 const dialogs=resources.filter(r=>r.ids[0]===5).map(r=>{try{return dialog(r);}catch(e){return {id:r.ids[1],error:e.message};}});
 const ascii=b.toString('latin1').match(/[\x20-\x7e]{6,}/g)||[];
 const signals=[...new Set(ascii.filter(s=>s.length<250&&/(?:videoplayback|youtubei|googlevideo|signatureCipher|adaptiveFormats|serverAbr|sabr|ump|Content-Range|Range:|HttpSend|WinHttp|connectNative|NamedPipe|IDMWFP|\\Device\\|CreateFileMapping)/i.test(s)))];
 return {name,sha256:crypto.createHash('sha256').update(b).digest('hex'),machine:b.readUInt16LE(pe+4).toString(16),sections,imports,delayImports,exports,resourceCounts:resources.reduce((v,r)=>(v[r.ids[0]]=(v[r.ids[0]]||0)+1,v),{}),dialogs,signals};
}
const names=['IDMan.exe','IDMMsgHost.exe','IDMNetMon.dll','IDMNetMon64.dll','idmnmcl.dll','IDMVMPrs.dll','IDMVMPrs64.dll','idmvconv.dll','idmvs.dll','idmwfp64.sys'];
const reports=names.map(inspect);fs.writeFileSync(path.join(output,'pe-analysis.json'),JSON.stringify(reports,null,2)+'\n');
for(const r of reports)console.log(JSON.stringify({name:r.name,importModules:r.imports.map(i=>i.module),delayImportModules:r.delayImports.map(i=>i.module),exportCount:r.exports.length,dialogs:r.dialogs.length,errors:r.dialogs.filter(d=>d.error)}));
console.log('Wrote read-only PE metadata to docs/reference/pe-analysis.json');
