'use strict';
// Independent media fixtures. Production code is never used to construct the expected ranges.
const {box,inspect}=require('./sidx-tree.cjs');
function externalize(bytes){
 const original=inspect(bytes),prefix=Buffer.alloc(16);prefix.writeUInt32BE(16);prefix.write('free',4);
 const data=box({id:original.id,scale:original.scale,time:original.time,offset:original.parts[0].start-original.length,parts:original.parts});
 return {bytes:Buffer.concat([bytes.subarray(0,original.start),bytes.subarray(original.start+original.length)]),indexBytes:Buffer.concat([prefix,data]),index:{start:prefix.length,length:data.length},initLength:original.start,ranges:original.parts.map(({start,length})=>({start:start-original.length,length}))};
}
module.exports={externalize};
