'use strict';
const assert=require('node:assert/strict');let checks=0;
for(const family of ['chromium','firefox']){
 const media=require('../browser/'+family+'/media.js'),base='https://media.test/recorded.m3u8';
 const playlist='#EXTM3U\n#EXT-X-VERSION:10\n#EXT-X-PART-INF:PART-TARGET=1\n#EXT-X-MAP:URI="init.mp4"\n#EXT-X-PART:DURATION=1,URI="part0.m4s",INDEPENDENT=YES\n#EXT-X-PART:DURATION=1,URI="part1.m4s"\n#EXTINF:2,\nfull0.m4s\n#EXT-X-PART:DURATION=1,URI="part2.m4s",INDEPENDENT=YES\n#EXT-X-PART:DURATION=1,URI="part3.m4s"\n#EXTINF:2,\nfull1.m4s\n#EXT-X-ENDLIST\n';
 const parsed=media.hls(playlist,base);
 assert.deepEqual(parsed.segments.map(segment=>segment.url),['https://media.test/init.mp4','https://media.test/full0.m4s','https://media.test/full1.m4s']);checks++;
 assert.throws(()=>media.hls('#EXTM3U\n#EXT-X-PART-INF:PART-TARGET=1\n#EXT-X-PART:DURATION=1,URI="part.m4s"\n#EXT-X-ENDLIST\n',base),/no complete media segments/);checks++;
}
console.log('PASS '+checks+' HLS complete-segment and partial-only checks');
