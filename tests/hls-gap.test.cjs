'use strict';
const assert=require('node:assert/strict');let checks=0;
for(const family of ['chromium','firefox']){
 const media=require('../browser/'+family+'/media.js'),base='https://media.test/recording.m3u8';
 const recorded='#EXTM3U\n#EXT-X-MAP:URI="init.mp4"\n#EXTINF:2,\nfirst.m4s\n#EXT-X-GAP\n#EXTINF:3,\nmissing.m4s\n#EXTINF:2,\nlast.m4s\n#EXT-X-ENDLIST\n';
 const parsed=media.hls(recorded,base);
 assert.deepEqual(parsed.segments.map(segment=>segment.url),['https://media.test/init.mp4','https://media.test/first.m4s','https://media.test/last.m4s']);
 assert.equal(parsed.segments.at(-1).timeline,5);checks++;
 const ranged='#EXTM3U\n#EXT-X-BYTERANGE:4@0\n#EXTINF:2,\nall.ts\n#EXT-X-GAP\n#EXT-X-BYTERANGE:4\n#EXTINF:2,\nall.ts\n#EXT-X-BYTERANGE:4\n#EXTINF:2,\nall.ts\n#EXT-X-ENDLIST\n';
 assert.deepEqual(media.hls(ranged,base).segments.map(segment=>[segment.start,segment.length]),[[0,4],[8,4]]);checks++;
 const live=media.hls('#EXTM3U\n#EXT-X-TARGETDURATION:2\n#EXT-X-GAP\n#EXTINF:2,\nmissing.ts\n#EXTINF:2,\ngood.ts\n',base,{allowLive:true});
 assert.deepEqual(live.segments.map(segment=>!!segment.gap),[true,false]);checks++;
 for(const invalid of ['#EXT-X-GAP\n#EXT-X-GAP\n#EXTINF:2,\na.ts\n','#EXTINF:2,\na.ts\n#EXT-X-GAP\n']){assert.throws(()=>media.hls('#EXTM3U\n'+invalid+'#EXT-X-ENDLIST\n',base),/gap/);checks++;}
}
console.log('PASS '+checks+' HLS gap parsing and handoff checks');
