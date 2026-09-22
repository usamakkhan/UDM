'use strict';
const assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const M=require('../browser/chromium/media.js');let passed=0;
function check(name,run){run();passed++;console.log('PASS '+name);}
const base='https://media.example.test/root/master.m3u8';
check('HLS quality catalog preserves only declared heights and audio groups',()=>{
 const p=M.hls('#EXTM3U\n#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID="en",NAME="English",DEFAULT=YES,URI="en.m3u8"\n#EXT-X-STREAM-INF:BANDWIDTH=2000000,RESOLUTION=1280x720,AUDIO="en"\n720/index.m3u8\n#EXT-X-STREAM-INF:BANDWIDTH=4000000,RESOLUTION=1920x1080,AUDIO="en"\n1080/index.m3u8',base);
 assert.deepEqual(p.variants.map(x=>x.height),[720,1080]);assert.equal(p.variants[0].url,'https://media.example.test/root/720/index.m3u8');assert.equal(p.audio[0].url,'https://media.example.test/root/en.m3u8');
});
check('HLS fMP4 initialization and explicit/implicit byte ranges',()=>{
 const p=M.hls('#EXTM3U\n#EXT-X-MAP:URI="all.mp4",BYTERANGE="100@0"\n#EXT-X-BYTERANGE:200@100\nall.mp4\n#EXT-X-BYTERANGE:300\nall.mp4\n#EXT-X-ENDLIST',base);
 assert.deepEqual(p.segments.map(x=>[x.start,x.length]),[[0,100],[100,200],[300,300]]);
});
for(const [name,text,pattern] of [
 ['HLS encrypted media','#EXTM3U\n#EXT-X-KEY:METHOD=AES-128,URI="key"\na.ts\n#EXT-X-ENDLIST',/Encrypted/],
 ['HLS live playlist','#EXTM3U\n#EXTINF:6,\na.ts',/live/],
 ['HLS discontinuity','#EXTM3U\n#EXT-X-DISCONTINUITY\na.ts\n#EXT-X-ENDLIST',/unsupported/],
 ['HLS local resource','#EXTM3U\nfile:///private.ts\n#EXT-X-ENDLIST',/HTTP/],
 ['HLS ambiguous range','#EXTM3U\n#EXT-X-BYTERANGE:50\nfirst.ts\n#EXT-X-ENDLIST',/ambiguous/],
 ['HLS changed init','#EXTM3U\n#EXT-X-MAP:URI="first.mp4"\na.m4s\n#EXT-X-MAP:URI="second.mp4"\nb.m4s\n#EXT-X-ENDLIST',/Changing/]
])check(name+' is rejected',()=>assert.throws(()=>M.hls(text,base),pattern));
const mpd='<MPD type="static" mediaPresentationDuration="PT6S"><Period><AdaptationSet mimeType="video/mp4"><SegmentTemplate timescale="1" duration="2" initialization="init-$RepresentationID$.mp4" media="part-$Number%03d$.m4s"/><Representation id="v1" height="720" bandwidth="2000000"/></AdaptationSet><AdaptationSet mimeType="audio/mp4" lang="en"><Representation id="a1"><BaseURL>audio.mp4</BaseURL></Representation></AdaptationSet></Period></MPD>';
check('DASH templates, duration, quality and separate audio',()=>{const c=M.dash(mpd,base);assert.equal(c.length,1);assert.equal(c[0].height,720);assert.equal(c[0].plan.tracks.length,2);assert.equal(c[0].plan.tracks[0].segments.length,4);assert.equal(c[0].plan.tracks[0].segments[2].url,'https://media.example.test/root/part-002.m4s');});
check('DASH negative repeat ends at period duration',()=>{const s=mpd.replace('duration="2"','').replace('/><Representation','><SegmentTimeline><S t="0" d="2" r="-1"/></SegmentTimeline></SegmentTemplate><Representation');assert.equal(M.dash(s,base)[0].plan.tracks[0].segments.length,4);});
check('DASH explicit timeline and Time templates',()=>{const text=mpd.replace('duration="2"','').replace('media="part-$Number%03d$.m4s"','media="part-$Time$.m4s"').replace('/><Representation','><SegmentTimeline><S t="10" d="2" r="1"/><S t="14" d="2" r="1"/></SegmentTimeline></SegmentTemplate><Representation');const c=M.dash(text,base);assert.equal(c[0].plan.tracks[0].segments[1].url,'https://media.example.test/root/part-10.m4s');assert.equal(c[0].plan.tracks[0].segments.length,5);});
check('DASH SegmentList and byte ranges',()=>{const text='<MPD><Period><AdaptationSet mimeType="video/mp4"><Representation height="360"><BaseURL>file.mp4?x=1&amp;y=2</BaseURL><SegmentList><Initialization range="0-99"/><SegmentURL mediaRange="100-299"/></SegmentList></Representation></AdaptationSet></Period></MPD>';const c=M.dash(text,base);assert.equal(c[0].plan.tracks[0].segments[1].length,200);assert.match(c[0].plan.tracks[0].segments[0].url,/x=1&y=2/);});
check('DASH rejects DRM',()=>assert.throws(()=>M.dash(mpd.replace('<Period>','<Period><ContentProtection/>'),base),/DRM/));
check('DASH rejects live and multiple periods',()=>{assert.throws(()=>M.dash(mpd.replace('static','dynamic'),base),/recorded/);assert.throws(()=>M.dash(mpd.replace('</MPD>','<Period/></MPD>'),base),/Multi-period/);});
check('DASH rejects XML entities and malformed trees',()=>{assert.throws(()=>M.dash('<!DOCTYPE MPD [<!ENTITY x SYSTEM "file:///x">]>'+mpd,base),/XML/);assert.throws(()=>M.dash(mpd.replace('</Period>','</Wrong>'),base),/XML/);});
check('Playlist expansion is bounded',()=>assert.throws(()=>M.dash(mpd.replace('PT6S','PT999999S'),base),/limit/));
check('Top-edge placement stays compact and aligned while scrolling',()=>{const a=M.placement({left:100,top:200,right:900,bottom:650,width:800,height:450},{width:1200,height:800});assert.equal(a.x,728);assert.equal(a.y,184);assert.equal(a.width,168);const b=M.placement({left:100,top:100,right:900,bottom:550,width:800,height:450},{width:1200,height:800});assert.equal(a.y-b.y,100);});
check('Small players use icon mode and viewport bounds',()=>{const p=M.placement({left:0,top:0,right:180,bottom:100,width:180,height:100},{width:200,height:120},168,24,{x:999,y:-999});assert.equal(p.width,30);assert.equal(p.x,166);assert.equal(p.y,4);});
check('Offscreen players hide and signed URLs stay unchanged',()=>{assert.equal(M.placement({left:0,top:-500,right:800,bottom:-50,width:800,height:450},{width:900,height:600}).visible,false);assert.equal(M.url('https://cdn.test/file.mp4?signature=a%2Fb&x=1'),'https://cdn.test/file.mp4?signature=a%2Fb&x=1');assert.equal(M.kind('https://cdn.test/part.m4s','video/mp4'),'fragment');});

(async()=>{
 const store={},session={},nativeCalls=[];let player={page:'https://player.example.test/watch',token:'token-1',stamp:'1',current:'https://cdn.example.test/movie.mp4',sources:[],height:720,width:1280,videoCount:1,title:'Movie',encrypted:false},frame=2,documentId='document-1';
 const event=()=>({addListener(){}}),api={storage:{local:{get:async()=>({settings:{}})},session:{get:async key=>({[key]:session[key]}),set:async values=>Object.assign(session,values),remove:async keys=>{for(const key of [].concat(keys))delete session[key];}}},tabs:{get:async id=>({id,url:'https://outer.example.test/',incognito:false})},scripting:{executeScript:async()=>[{frameId:frame,documentId,result:structuredClone(player)}]},permissions:{contains:async()=>true,onAdded:event(),onRemoved:event()},runtime:{onStartup:event(),sendNativeMessage:async(_,msg)=>{nativeCalls.push(msg);return {ok:true};}}};
 const sandbox={navigator:{userAgent:"test"},UdmMedia:M,console,URL,Date,crypto,AbortController,TextDecoder,setTimeout,clearTimeout,handoff:async item=>{nativeCalls.push(item);return {ok:true};},fetch:async address=>({ok:true,url:address,body:new ReadableStream({start(c){c.enqueue(new TextEncoder().encode('#EXTM3U\n#EXTINF:2,\nsegment.ts\n#EXT-X-ENDLIST'));c.close();}})})};
 vm.createContext(sandbox);vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser/chromium/sites.js'),'utf8')+'\nglobalThis.sites=UdmSites;',sandbox);sandbox.sites.install(api);
 const sender={tab:{id:7},frameId:2,documentId,url:player.page},message={page:player.page,token:'token-1'};
 const catalog=await sandbox.sites.list(message,sender);assert.equal(catalog.choices.length,1);assert.match(catalog.choices[0].label,/720p/);passed++;console.log('PASS direct video is bound to the clicked frame/player');
 await sandbox.sites.download({...message,key:catalog.choices[0].key},sender);assert.equal(nativeCalls.length,1);
 player.current='https://cdn.example.test/other.mp4';await assert.rejects(()=>sandbox.sites.download({...message,key:catalog.choices[0].key},sender),/changed/);assert.equal(nativeCalls.length,1);passed++;console.log('PASS replaced videos invalidate prior choices');
 player.current='blob:https://player.example.test/blob';player.videoCount=2;await assert.rejects(()=>sandbox.sites.list(message,sender),/Several videos/);passed++;console.log('PASS ambiguous multi-player blob capture is rejected');
 player.videoCount=1;session['site-media:7']=[{frameId:0,page:player.page,time:Date.now(),kind:'hls',url:'https://cdn.test/wrong-frame.m3u8'},{frameId:2,documentId,page:player.page,time:Date.now(),kind:'hls',url:'https://cdn.test/correct.m3u8'}];
 const hls=await sandbox.sites.list(message,sender);assert.equal(hls.choices.length,1);assert.equal(hls.choices[0].source,'correct.m3u8');passed++;console.log('PASS embedded-player catalogs exclude other frames');
 await sandbox.sites.download({...message,key:hls.choices[0].key},sender);assert.equal(nativeCalls.at(-1).action,'adaptive');assert.equal(nativeCalls.at(-1).plan.tracks[0].segments[0].url,'https://cdn.test/segment.ts');passed++;console.log('PASS HLS choice sends actual segment plan to native engine');
 // A real browser can open its panel while the observed playlist is still being saved.
 session['site-media:7']=[];const originalSet=api.storage.session.set;let unblock,entered;
 const gate=new Promise(r=>unblock=r),pendingWrite=new Promise(r=>entered=r);
 api.storage.session.set=async values=>{if(values['site-media:7']){entered();await gate;}return originalSet(values);};
 const observing=sandbox.sites.observe({tabId:7,frameId:2,documentId,statusCode:200,url:'https://cdn.test/pending.m3u8',documentUrl:player.page,responseHeaders:[{name:'Content-Type',value:'application/vnd.apple.mpegurl'}]});
 await pendingWrite;let settled=false;const listing=sandbox.sites.list(message,sender).then(result=>{settled=true;return result;});
 await new Promise(r=>setImmediate(r));const readTooEarly=settled;unblock();await observing;const pendingCatalog=await listing;api.storage.session.set=originalSet;
 assert.equal(readTooEarly,false,'Catalog must wait for the in-flight capture write');assert.equal(pendingCatalog.choices.length,1);assert.equal(pendingCatalog.choices[0].source,'pending.m3u8');passed++;console.log('PASS first HLS menu waits for the pending browser capture write');
 // Identical URLs, frame IDs and player tokens are not proof of the same document.
 session['site-media:7']=[{frameId:2,documentId:'old-document',page:player.page,time:Date.now(),kind:'hls',url:'https://cdn.test/old.m3u8'},{frameId:2,documentId,page:player.page,time:Date.now(),kind:'hls',url:'https://cdn.test/current.m3u8'}];
 const scoped=await sandbox.sites.list(message,sender);assert.equal(scoped.choices.length,1);assert.equal(scoped.choices[0].source,'current.m3u8');passed++;console.log('PASS same-URL frame navigation excludes earlier documents');
 documentId='document-2';await assert.rejects(()=>sandbox.sites.list(message,sender),/page changed/);passed++;console.log('PASS an old content-script message cannot address a new document');
 sender.documentId=documentId;await assert.rejects(()=>sandbox.sites.download({...message,key:scoped.choices[0].key},sender),/changed/);passed++;console.log('PASS document navigation invalidates saved choices even with identical player values');
 // Firefox/older implementations without document IDs must enforce navigation time.
 documentId=undefined;delete sender.documentId;player.timeOrigin=Date.now()-1000;
 session['site-media:7']=[{frameId:2,page:player.page,time:player.timeOrigin-1,kind:'hls',url:'https://cdn.test/old.m3u8'},{frameId:2,page:player.page,time:Date.now(),kind:'hls',url:'https://cdn.test/fallback.m3u8'}];
 const fallback=await sandbox.sites.list(message,sender);assert.equal(fallback.choices.length,1);assert.equal(fallback.choices[0].source,'fallback.m3u8');passed++;console.log('PASS browsers without document IDs exclude captures older than navigation');
 // Navigation can occur while storage is returning a selected direct offer.
 documentId='document-3';sender.documentId=documentId;player.current='https://cdn.test/movie.mp4';
 const direct=await sandbox.sites.list(message,sender),originalGet=api.storage.session.get,count=nativeCalls.length;
 api.storage.session.get=async key=>{const value=await originalGet(key);if(key==='site-offers:7')documentId='document-4';return value;};
 await assert.rejects(()=>sandbox.sites.download({...message,key:direct.choices[0].key},sender),/page changed/);api.storage.session.get=originalGet;assert.equal(nativeCalls.length,count);passed++;console.log('PASS navigation during direct-offer lookup prevents native handoff');
 sender.documentId=documentId;
 player.encrypted=true;await assert.rejects(()=>sandbox.sites.list(message,sender),/DRM/);passed++;console.log('PASS protected players never expose download choices');
 console.log('ALL '+passed+' CROSS-SITE CHECKS PASSED');
})().catch(error=>{console.error(error);process.exitCode=1;});
