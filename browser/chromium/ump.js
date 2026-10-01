// Original bounded wire reader. Protocol field references are recorded in docs/sabr.md.
(()=>{
  'use strict';
  const text=new TextDecoder();
  function fields(bytes){
    let offset=0;const output=[];
    function number(){let n=0n,shift=0n;for(let i=0;i<10;i++){if(offset>=bytes.length)throw Error('Truncated protobuf');const b=bytes[offset++];n|=BigInt(b&127)<<shift;if(!(b&128)){if(i===9&&b>1)throw Error('Integer overflow');return n;}shift+=7n;}throw Error('Invalid integer');}
    while(offset<bytes.length){const tag=Number(number()),id=tag>>>3,wire=tag&7;if(!id)throw Error('Invalid protobuf field');let value;if(wire===0)value=number();else if(wire===2){const count=Number(number());if(!Number.isSafeInteger(count)||count<0||offset+count>bytes.length)throw Error('Truncated field');value=bytes.subarray(offset,offset+count);offset+=count;}else if(wire===1||wire===5){const count=wire===1?8:4;if(offset+count>bytes.length)throw Error('Truncated fixed field');value=bytes.subarray(offset,offset+count);offset+=count;}else throw Error('Unsupported protobuf wire type');output.push({id,wire,value});if(output.length>8192)throw Error('Too many fields');}return output;
  }
  function integer(bytes,offset){if(offset>=bytes.length)return null;const first=bytes[offset],count=first<128?1:first<192?2:first<224?3:first<240?4:5;if(offset+count>bytes.length)return null;let value;if(count===5)value=bytes[offset+1]+256*(bytes[offset+2]+256*(bytes[offset+3]+256*bytes[offset+4]));else{const bits=8-count;value=first&((1<<bits)-1);let multiplier=1<<bits;for(let i=1;i<count;i++){value+=bytes[offset+i]*multiplier;multiplier*=256;}}return {value,next:offset+count};}
  function identity(bytes,expected){let offset=0,found=false;while(offset<bytes.length){const type=integer(bytes,offset);if(!type)break;const length=integer(bytes,type.next);if(!length)break;if(length.value>64*1024*1024)throw Error('Oversized UMP part');if(length.next+length.value>bytes.length)break;const payload=bytes.subarray(length.next,length.next+length.value);offset=length.next+length.value;if(type.value===12)throw Error('Encrypted media');if(type.value===20||type.value===42){const item=fields(payload).find(f=>f.id===(type.value===20?2:1)&&f.wire===2);if(item){const id=text.decode(item.value);if(id!==expected)throw Error('Different video');found=true;}}}return found;}
  function request(bytes){const f=fields(bytes);return f.some(x=>x.id===5&&x.wire===2&&x.value.length)&&f.some(x=>x.id===19&&x.wire===2&&x.value.length);}
  const api=Object.freeze({fields,integer,identity,request});
  if(typeof module!=='undefined')module.exports=api;
  else Object.defineProperty(globalThis,'__udmUmpV1',{value:api,configurable:false});
})();
