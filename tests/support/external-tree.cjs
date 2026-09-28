'use strict';
// Independent depth-first ISO-BMFF writer: child reference sizes include their
// descendant boxes, whereas every first_offset names a position in media only.
const {box,inspect}=require('./sidx-tree.cjs');
function externalTree(bytes,{mixed=false,version=1}={}){
 const old=inspect(bytes),parts=old.parts.map(p=>({...p,start:p.start-old.length}));
 const leaves=parts.map(part=>({items:[{part}]})),mid=items=>({items:items.map(node=>({node}))});
 const half=Math.ceil(leaves.length/2),root=mixed?{items:[{part:parts[0]},{node:mid(leaves.slice(1,-1))},{part:parts.at(-1)}]}:mid([mid(leaves.slice(0,half)),mid(leaves.slice(half))]);
 function measure(node){
  for(const i of node.items)if(i.node)measure(i.node);
  node.first=node.items[0].part||node.items[0].node.first;
  node.duration=node.items.reduce((n,i)=>n+(i.part?.duration??i.node.duration),0);
  node.length=(version?40:32)+12*node.items.length;
  node.extent=node.length+node.items.reduce((n,i)=>n+(i.node?.extent||0),0);
 }measure(root);
 const nodes=[];function place(node,start){node.start=start;nodes.push(node);let next=start+node.length;for(const i of node.items)if(i.node){place(i.node,next);next+=i.node.extent;}}
 place(root,16);const prefix=Buffer.alloc(16);prefix.writeUInt32BE(16);prefix.write('free',4);
 const chunks=nodes.map(n=>box({id:old.id,scale:old.scale,time:n.first.time,offset:n.first.start,version,parts:n.items.map(i=>i.part||{index:true,length:i.node.extent,duration:i.node.duration})}));
 return {bytes:Buffer.concat([bytes.subarray(0,old.start),bytes.subarray(old.start+old.length)]),indexBytes:Buffer.concat([prefix,...chunks]),index:{start:16,length:root.length},initLength:old.start,ranges:parts.map(({start,length})=>({start,length})),nodes:nodes.map(n=>({start:n.start,length:n.length,extent:n.extent})),indexReads:[{start:16,length:root.length},...root.items.filter(i=>i.node).map(i=>({start:i.node.start,length:i.node.extent}))]};
}
function fixture(options){const init=Buffer.alloc(100);init.writeUInt32BE(100);init.write('free',4);const data=Buffer.alloc(400);data.writeUInt32BE(400);data.write('mdat',4);return externalTree(Buffer.concat([init,box({parts:Array.from({length:4},()=>({length:100,duration:2000}))}),data]),options);}
module.exports={externalTree,fixture};
