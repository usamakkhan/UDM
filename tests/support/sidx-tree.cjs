'use strict';
// Independent ISO-BMFF fixture writer; no production parser is used to build indexes.
function box({id=1,scale=1000,time=0,offset=0,parts=[],version=1}){
 const b=Buffer.alloc((version?40:32)+12*parts.length);b.writeUInt32BE(b.length);b.write('sidx',4);b[8]=version;b.writeUInt32BE(id,12);b.writeUInt32BE(scale,16);
 let p=20;if(version){b.writeBigUInt64BE(BigInt(time),p);b.writeBigUInt64BE(BigInt(offset),p+8);p+=16;}else{b.writeUInt32BE(time,p);b.writeUInt32BE(offset,p+4);p+=8;}
 b.writeUInt16BE(parts.length,p+2);p+=4;
 for(const part of parts){b.writeUInt32BE(part.length+(part.index?0x80000000:0),p);b.writeUInt32BE(part.duration,p+4);b.writeUInt32BE(0x90000000,p+8);p+=12;}return b;
}
function inspect(bytes){
 let at=0,found;
 while(at<bytes.length){let n=bytes.readUInt32BE(at),header=8;if(n===1){n=Number(bytes.readBigUInt64BE(at+8));header=16;}if(n<8||at+n>bytes.length)throw Error('Invalid fixture MP4');
  if(bytes.toString('ascii',at+4,at+8)==='sidx'){
   if(found)throw Error('More than one source SIDX');let p=at+header;const version=bytes[p];p+=4;const id=bytes.readUInt32BE(p),scale=bytes.readUInt32BE(p+4);p+=8;
   const time=version?Number(bytes.readBigUInt64BE(p)):bytes.readUInt32BE(p);p+=version?8:4;const offset=version?Number(bytes.readBigUInt64BE(p)):bytes.readUInt32BE(p);p+=version?8:4;
   const count=bytes.readUInt16BE(p+2);p+=4;let start=at+n+offset,t=time;const parts=[];
   for(let i=0;i<count;i++,p+=12){const length=bytes.readUInt32BE(p),duration=bytes.readUInt32BE(p+4);if(length>=0x80000000)throw Error('Source must be flat');parts.push({start,length,duration,time:t});start+=length;t+=duration;}
   found={start:at,length:n,id,scale,time,parts};
  }at+=n;
 }if(!found)throw Error('Missing source SIDX');return found;
}
function hierarchize(bytes){
 const original=inspect(bytes),leaves=original.parts.map(part=>({part,children:[]})),split=Math.ceil(leaves.length/2),middle=[{children:leaves.slice(0,split)},{children:leaves.slice(split)}].filter(n=>n.children.length),root={children:middle},nodes=[root,...middle,...leaves];
 for(const node of [...nodes].reverse()){node.time=node.part?node.part.time:node.children[0].time;node.duration=node.part?node.part.duration:node.children.reduce((sum,n)=>sum+n.duration,0);node.length=40+12*(node.part?1:node.children.length);}
 let at=original.start;for(const node of nodes){node.start=at;at+=node.length;}const delta=at-original.start-original.length;
 const indexes=nodes.map(node=>{const target=node.part?node.part.start+delta:node.children[0].start;const data=box({id:original.id,scale:original.scale,time:node.time,offset:target-node.start-node.length,parts:node.part?[node.part]:node.children.map(c=>({length:c.length,duration:c.duration,index:true}))});return {start:node.start,length:node.length,data};});
 return {bytes:Buffer.concat([bytes.subarray(0,original.start),...indexes.map(x=>x.data),bytes.subarray(original.start+original.length)]),index:{start:root.start,length:root.length},indexes:indexes.map(({start,length})=>({start,length})),ranges:original.parts.map(({start,length})=>({start:start+delta,length}))};
}
function fixture(){
 const init=Buffer.alloc(100);init.writeUInt32BE(init.length);init.write('free',4);
 const b=box({parts:Array.from({length:4},()=>({length:100,duration:2000}))});
 const media=Buffer.alloc(400);media.writeUInt32BE(media.length);media.write('mdat',4);
 return hierarchize(Buffer.concat([init,b,media]));
}
module.exports={box,inspect,hierarchize,fixture};
