'use strict';
const assert=require('node:assert/strict'),M=require('../browser/chromium/media.js'),FirefoxMedia=require('../browser/firefox/media.js');
const base='https://media.test/root.m3u8',leaf='#EXTM3U\n#EXTINF:2,\nsegment.ts\n#EXT-X-ENDLIST\n';
const variant=(uri,height=360,extra='')=>'#EXT-X-STREAM-INF:BANDWIDTH=1000000'+(height?',RESOLUTION=640x'+height:'')+extra+'\n'+uri+'\n';
const master=(...v)=>'#EXTM3U\n'+v.join('');
const group=(name,uri,id='a')=>'#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID="'+id+'",NAME="'+name+'",LANGUAGE="en",DEFAULT=YES,URI="'+uri+'"\n';
let count=0;
const pass=name=>{++count;console.log('PASS '+name);};
async function catalog(text,table,extra,parser=M){const reads=[];const result=await parser.hlsCatalog({url:base,text},async(address,signal)=>{reads.push(address);if(extra)return extra(address,signal);assert(table[address],address);return typeof table[address]==='string'?{url:address,text:table[address]}:table[address];});return {...result,requests:reads};}
(async()=>{
 let c=await catalog(master(variant('branch/master.m3u8',0)),{
  'https://media.test/branch/master.m3u8':master(group('English','en.m3u8'),variant('360.m3u8',360,',AUDIO="a",CODECS="avc1.42c01e,mp4a.40.2"'),variant('720.m3u8',720,',AUDIO="a"')),
  'https://media.test/branch/360.m3u8':leaf,'https://media.test/branch/720.m3u8':leaf});
 assert.deepEqual(c.choices.map(x=>x.height).sort((a,b)=>a-b),[360,720]);assert(c.choices.every(x=>x.audioOptions[0].url==='https://media.test/branch/en.m3u8'));assert.equal(c.requests.length,3);pass('Nested masters expose only actual leaf qualities and relative rendition URLs');
 assert(!c.requests.some(x=>/en.m3u8|segment.ts/.test(x)));pass('Catalog discovery does not fetch audio or media segments');
 c=await catalog(master(group('Parent','parent.m3u8'),variant('child.m3u8',1080,',AUDIO="a",CODECS="avc1.640028,mp4a.40.2"')),
  {'https://media.test/child.m3u8':master(variant('leaf.m3u8',720)),'https://media.test/leaf.m3u8':leaf});
 assert.equal(c.choices[0].height,720);assert.equal(c.choices[0].audioOptions[0].name,'Parent');assert.equal(c.choices[0].codecs,'avc1.640028,mp4a.40.2');pass('Leaf quality overrides parent metadata while unoverridden audio stays attached');
 c=await catalog(master(group('Parent','parent.m3u8'),variant('child.m3u8',1080,',AUDIO="a"')),
  {'https://media.test/child.m3u8':master(group('Child','child-audio.m3u8'),variant('leaf.m3u8',720,',AUDIO="a"')),'https://media.test/leaf.m3u8':leaf});
 assert.equal(c.choices[0].audioOptions[0].name,'Child');pass('A nested audio group resolves in its declaring playlist');
 c=await catalog(master(group('Parent','parent.m3u8'),variant('child.m3u8',360,',AUDIO="a"')),
  {'https://media.test/child.m3u8':master(variant('leaf.m3u8',720,',AUDIO="a"'))});
 assert.equal(c.choices.length,0);assert.match(c.notes.join(),/missing audio group/);pass('Missing child audio group cannot borrow a same-named parent group');
 c=await catalog(master(variant('same.m3u8'),variant('same.m3u8')),{'https://media.test/same.m3u8':leaf});
 assert.equal(c.requests.length,1);assert.equal(c.choices.length,1);pass('Shared identical leaves are fetched and offered once');
 c=await catalog(master(group('English','en.m3u8','en'),group('French','fr.m3u8','fr'),variant('same.m3u8',360,',AUDIO="en"'),variant('same.m3u8',360,',AUDIO="fr"')),{'https://media.test/same.m3u8':leaf});
 assert.equal(c.requests.length,1);assert.deepEqual(c.choices.map(x=>x.audioOptions[0].name).sort(),['English','French']);pass('Shared media with distinct rendition groups preserves both valid selections');
 c=await catalog(master(variant('root.m3u8'),variant('good.m3u8',720)),{'https://media.test/good.m3u8':leaf});
 assert.equal(c.choices.length,1);assert.match(c.notes.join(),/cycle/);pass('A cyclic branch cannot suppress an independent valid quality');
 c=await catalog(master(variant('redirect.m3u8')),{'https://media.test/redirect.m3u8':{url:base,text:master(variant('redirect.m3u8'))}});
 assert.equal(c.choices.length,0);assert.match(c.notes.join(),/cycle/);pass('Redirects back to an ancestor cannot hide a playlist cycle');
 c=await catalog(master(variant('redirect.m3u8')),{'https://media.test/redirect.m3u8':{url:'https://cdn.test/final/root.m3u8',text:master(variant('leaf.m3u8',480))},'https://cdn.test/final/leaf.m3u8':leaf});
 assert.equal(c.choices[0].url,'https://cdn.test/final/leaf.m3u8');pass('Redirected masters resolve relative children against the final URL');
 c=await catalog(master(variant('0.m3u8')),{},async address=>({url:address,text:master(variant((Number(new URL(address).pathname.slice(1).split('.')[0])+1)+'.m3u8'))}));
 assert.equal(c.choices.length,0);assert(c.requests.length<=7);assert.match(c.notes.join(),/six nested/);pass('Deep playlist chains stop within the traversal bound');
 const many=Array.from({length:90},(_,i)=>variant(i+'.m3u8')).join('');c=await catalog(master(many),{},async address=>({url:address,text:leaf}));
 assert(c.requests.length<=63);assert.match(c.notes.join(),/64-playlist/);pass('Wide playlist catalogs stop at the total read limit');
 let active=0,peak=0;c=await catalog(master(many),{},async address=>{peak=Math.max(peak,++active);await new Promise(r=>setTimeout(r,2));--active;return {url:address,text:leaf};});
 assert.equal(peak,4);assert.equal(active,0);pass('Playlist discovery uses at most four concurrent reads');
 const huge=leaf+'#'+('x'.repeat(1500000));c=await catalog(master(Array.from({length:8},(_,i)=>variant(i+'.m3u8')).join('')),{},async address=>({url:address,text:huge}));
 assert.match(c.notes.join(),/eight MiB/);assert(c.choices.length<8);pass('The whole catalog has a byte budget independent of each playlist limit');
 c=await catalog(master(variant('live.m3u8')),{'https://media.test/live.m3u8':'#EXTM3U\n#EXT-X-TARGETDURATION:2\n#EXTINF:2,\nseg.ts'});
 assert.equal(c.choices[0].live,true);pass('Live leaf playlists retain their live-recording identity');
 c=await catalog(master('#EXT-X-MEDIA:TYPE=SUBTITLES,GROUP-ID="s",NAME="English",LANGUAGE="en",URI="captions.m3u8"\n',variant('live.m3u8',360,',SUBTITLES="s"')),{'https://media.test/live.m3u8':'#EXTM3U\n#EXT-X-TARGETDURATION:2\n#EXTINF:2,\nseg.ts'});
 assert.equal(c.choices[0].live,true);assert.deepEqual(c.choices[0].subtitleOptions,[]);pass('Live HLS catalog omits subtitle renditions the recorder cannot save');
 c=await catalog(master('#EXT-X-MEDIA:TYPE=SUBTITLES,GROUP-ID="s",NAME="English",LANGUAGE="en",URI="captions.m3u8"\n',variant('live.m3u8',360,',SUBTITLES="s"')),{'https://media.test/live.m3u8':'#EXTM3U\n#EXT-X-TARGETDURATION:2\n#EXTINF:2,\nseg.ts'},undefined,FirefoxMedia);
 assert.equal(c.choices[0].live,true);assert.deepEqual(c.choices[0].subtitleOptions,[]);pass('Firefox live HLS catalog applies the same subtitle limit');
 c=await catalog(master(variant('encrypted.m3u8'),variant('good.m3u8',720)),{'https://media.test/encrypted.m3u8':'#EXTM3U\n#EXT-X-KEY:METHOD=SAMPLE-AES,URI="key"\n#EXTINF:2,\na.ts\n#EXT-X-ENDLIST','https://media.test/good.m3u8':leaf});
 assert.equal(c.choices.length,1);assert.match(c.notes.join(),/Encrypted/);pass('Unsupported protected branches do not become downloadable leaf offers');
 console.log(count+' HLS catalog checks passed');
})().catch(e=>{console.error(e);process.exitCode=1;});
