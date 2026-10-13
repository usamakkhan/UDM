'use strict';
const assert=require('node:assert/strict');
const families=['chromium','firefox'];let checks=0;
const check=(name,fn)=>{fn();checks++;console.log('PASS '+name);};
(async()=>{
 for(const family of families){
  const M=require('../browser/'+family+'/media.js'),base='https://media.test/root.m3u8';
  const master='#EXTM3U\n#EXT-X-DEFINE:NAME="folder",VALUE="signed"\n#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID="a",NAME="English",URI="{$folder}/audio.m3u8"\n#EXT-X-STREAM-INF:BANDWIDTH=1000000,RESOLUTION=640x360,AUDIO="a"\n{$folder}/video.m3u8\n';
  const parsed=M.hls(master,base);
  check(family+' master substitutes variant and rendition URLs',()=>{assert.equal(parsed.variants[0].url,'https://media.test/signed/video.m3u8');assert.equal(M.hlsAudio(parsed,parsed.variants[0])[0].url,'https://media.test/signed/audio.m3u8');});
  const media='#EXTM3U\n#EXT-X-DEFINE:IMPORT="folder"\n#EXT-X-MAP:URI="{$folder}/init.mp4"\n#EXTINF:2,\n{$folder}/one.m4s\n#EXT-X-ENDLIST\n';
  check(family+' imported variables bind recorded media and maps',()=>{const result=M.hls(media,base,{imports:{folder:'signed'}});assert.deepEqual(result.segments.map(x=>x.url),['https://media.test/signed/init.mp4','https://media.test/signed/one.m4s']);});
  const mapped='#EXTM3U\n#EXT-X-DEFINE:NAME="bounds",VALUE="20@0"\n#EXT-X-MAP:URI="init.mp4",BYTERANGE="{$bounds}"\n#EXTINF:2,\none.m4s\n#EXT-X-ENDLIST\n';
  check(family+' quoted map byte ranges substitute before validation',()=>{const result=M.hls(mapped,base);assert.deepEqual({start:result.segments[0].start,length:result.segments[0].length},{start:0,length:20});});
  const query='#EXTM3U\n#EXT-X-DEFINE:QUERYPARAM="sig"\n#EXTINF:2,\none.ts?signature={$sig}\n#EXT-X-ENDLIST\n';
  check(family+' query variables use percent-decoded final playlist URL',()=>assert.equal(M.hls(query,base+'?sig=a%2Bb').segments[0].url,'https://media.test/one.ts?signature=a+b'));
  for(const [name,body] of [
   ['duplicate','#EXT-X-DEFINE:NAME="x",VALUE="a"\n#EXT-X-DEFINE:NAME="x",VALUE="b"\n'],
   ['missing import','#EXT-X-DEFINE:IMPORT="x"\n'],
   ['missing query','#EXT-X-DEFINE:QUERYPARAM="x"\n'],
   ['forward reference','#EXTINF:2,\n{$x}.ts\n#EXT-X-DEFINE:NAME="x",VALUE="later"\n'],
   ['invalid scheme','#EXT-X-DEFINE:NAME="x",VALUE="file:///secret"\n#EXTINF:2,\n{$x}\n'],
  ])check(family+' rejects '+name,()=>assert.throws(()=>M.hls('#EXTM3U\n'+body+'#EXTINF:2,\nok.ts\n#EXT-X-ENDLIST\n',base)));
  const leaf='#EXTM3U\n#EXT-X-DEFINE:IMPORT="folder"\n#EXTINF:2,\n{$folder}/one.ts\n#EXT-X-ENDLIST\n';
  const catalog=await M.hlsCatalog({url:base,text:master},async address=>({url:address,text:leaf}));
  check(family+' catalog carries declaring-master imports to selected leaf',()=>{assert.equal(catalog.choices.length,1);assert.equal(catalog.choices[0].hlsVariables.folder,'signed');assert.equal(catalog.choices[0].url,'https://media.test/signed/video.m3u8');});
  const audio=await M.hlsAudioPlaylist({url:'https://media.test/signed/audio.m3u8',text:leaf},()=>{throw Error('Unexpected nested read');},undefined,{folder:'signed'});
  check(family+' audio rendition imports resolve before native plan',()=>assert.equal(audio.segments[0].url,'https://media.test/signed/signed/one.ts'));
  const nested='#EXTM3U\n#EXT-X-DEFINE:NAME="next",VALUE="second.m3u8"\n#EXT-X-STREAM-INF:BANDWIDTH=64000,CODECS="mp4a.40.2"\n{$next}\n';
  const descendant='#EXTM3U\n#EXT-X-DEFINE:IMPORT="next"\n#EXTINF:2,\n{$next}.ts\n#EXT-X-ENDLIST\n';
  const nestedAudio=await M.hlsAudioPlaylist({url:base,text:nested},async address=>({url:address,text:descendant}));
  check(family+' nested audio imports use their immediate master',()=>{assert.equal(nestedAudio.segments[0].url,'https://media.test/second.m3u8.ts');assert.equal(nestedAudio.imports.next,'second.m3u8');});
 }
 console.log('ALL '+checks+' HLS VARIABLE CHECKS PASSED');
})().catch(error=>{console.error(error);process.exitCode=1;});
