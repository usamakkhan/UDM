'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict'),crypto=require('node:crypto'),M=require('../browser/chromium/media.js');
let passed=0;const check=(name,fn)=>{fn();passed++;console.log('PASS '+name);};
function index({version=0,offset=0,count=2,length=100,start=0,extended=false}={}){
 const h=extended?16:8,b=Buffer.alloc(h+(version?32:24)+count*12);b.writeUInt32BE(extended?1:b.length);b.write('sidx',4);if(extended)b.writeBigUInt64BE(BigInt(b.length),8);
 let p=h;b[p]=version;p+=4;b.writeUInt32BE(1,p);b.writeUInt32BE(1000,p+4);p+=8;
 if(version){b.writeBigUInt64BE(BigInt(start),p);b.writeBigUInt64BE(BigInt(offset),p+8);p+=16;}else{b.writeUInt32BE(start,p);b.writeUInt32BE(offset,p+4);p+=8;}
 b.writeUInt16BE(count,p+2);p+=4;for(let i=0;i<count;i++,p+=12){b.writeUInt32BE(length,p);b.writeUInt32BE(2000,p+4);b.writeUInt32BE(0x90000000,p+8);}return b;
}
const mpd=(base='<SegmentBase indexRange="100-155"><Initialization range="0-99"/></SegmentBase>')=>'<MPD type="static" mediaPresentationDuration="PT4S"><Period><AdaptationSet mimeType="video/mp4"><Representation id="v" height="360"><BaseURL>v.mp4</BaseURL>'+base+'</Representation></AdaptationSet><AdaptationSet mimeType="audio/mp4" lang="en"><Representation id="a"><BaseURL>a.mp4</BaseURL><SegmentBase indexRange="100-155"><Initialization range="0-99"/></SegmentBase></Representation></AdaptationSet></Period></MPD>';
check('Version-zero index resolves exact absolute ranges',()=>{const b=index();assert.deepEqual(M.sidx(b,100,1000),[{start:156,length:100},{start:256,length:100}]);});
check('Version-one offsets above 32 bits stay exact',()=>{const b=index({version:1,offset:4294967300,start:4294967301});assert.equal(M.sidx(b,100,1e11)[0].start,100+b.length+4294967300);});
check('Extended-size boxes use their full header width',()=>{const b=index({extended:true});assert.equal(M.sidx(b,100,1000)[0].start,100+b.length);});
check('A prefixed box does not shift the absolute SIDX origin',()=>{const free=Buffer.alloc(8);free.writeUInt32BE(8);free.write('free',4);const b=Buffer.concat([free,index()]);assert.equal(M.sidx(b,100,1000)[0].start,100+b.length);});
check('Typed-array subviews preserve their byte offset',()=>{const b=index(),p=Buffer.concat([Buffer.alloc(7),b,Buffer.alloc(8)]);assert.deepEqual(M.sidx(p.subarray(7,7+b.length),100,1000),M.sidx(b,100,1000));});
const bads=[
 ['box truncation',b=>b.subarray(0,b.length-1)],
 ['nested index',b=>{b.writeUInt32BE(0x80000064,32);return b;}],
 ['zero media length',b=>{b.writeUInt32BE(0,32);return b;}],
 ['zero duration',b=>{b.writeUInt32BE(0,36);return b;}],
 ['zero timescale',b=>{b.writeUInt32BE(0,16);return b;}],
 ['unknown version',b=>{b[8]=2;return b;}],
 ['nonzero flags',b=>{b[11]=1;return b;}],
 ['nonzero reserved bits',b=>{b[28]=1;return b;}],
 ['missing references',b=>{b.writeUInt16BE(0,30);return b;}],
 ['reference table mismatch',b=>{b.writeUInt16BE(3,30);return b;}],
 ['media beyond resource end',b=>{b.writeUInt32BE(900,32);return b;}],
 ['media part above native bound',b=>{b.writeUInt32BE(256*1024*1024+1,32);return b;}],
 ['size-zero box',b=>{b.writeUInt32BE(0);return b;}],
 ['missing SIDX',b=>{b.write('free',4);return b;}]
];
for(const [name,mutate] of bads)check('Rejects '+name,()=>assert.throws(()=>M.sidx(mutate(index()),100,1000)));
check('Rejects duplicate SIDX boxes',()=>assert.throws(()=>M.sidx(Buffer.concat([index({offset:56}),index()]),100,1000)));
check('Rejects unsafe 64-bit integers',()=>assert.throws(()=>M.sidx(index({version:1,offset:9007199254740992n}),100,1e16)));
check('Rejects more references than native accepts',()=>assert.throws(()=>M.sidx(index({count:M.MAX}),0,1e7)));
check('MPD exposes index metadata without inventing media segments',()=>{const t=M.dash(mpd(),'https://cdn.test/a.mpd')[0].plan.tracks[0];assert.deepEqual(t.index,{url:'https://cdn.test/v.mp4',start:100,length:56});assert.deepEqual(t.segments,[{url:'https://cdn.test/v.mp4',start:0,length:100}]);});
check('SegmentBase attributes and initialization inherit across the representation',()=>{
 const xml='<MPD><Period><AdaptationSet mimeType="video/mp4"><SegmentBase indexRange="100-155"><Initialization range="0-99"/></SegmentBase><Representation height="360"><BaseURL>v.mp4</BaseURL><SegmentBase indexRange="200-255"/></Representation></AdaptationSet></Period></MPD>';
 const t=M.dash(xml,'https://cdn.test/a.mpd')[0].plan.tracks[0];assert.equal(t.index.start,200);assert.equal(t.segments[0].length,100);
});
for(const [name,xml] of [
 ['missing initialization',mpd('<SegmentBase indexRange="100-155"/>')],
 ['missing index range',mpd('<SegmentBase><Initialization range="0-99"/></SegmentBase>')],
 ['oversized index',mpd('<SegmentBase indexRange="100-600000"><Initialization range="0-99"/></SegmentBase>')],
 ['ambiguous whole-file initialization',mpd('<SegmentBase indexRange="100-155"><Initialization/></SegmentBase>')],
 ['conflicting index locators',mpd('<SegmentBase indexRange="100-155"><RepresentationIndex sourceURL="x"/><Initialization range="0-99"/></SegmentBase>')],
 ['conflicting addressing',mpd() .replace('<SegmentBase indexRange="100-155">','<SegmentTemplate media="x"/><SegmentBase indexRange="100-155">')],
 ['protected representation',mpd().replace('<Representation id="v"','<ContentProtection/><Representation id="v"')]
])check('MPD rejects '+name,()=>assert.throws(()=>M.dash(xml,'https://cdn.test/a.mpd')));
function harness({status=206,range='bytes 100-155/1000',length='56',payload=index(),permission=true,change=false,batch=false,receipts=false,adaptiveResources=1}={}){
 const presentation=batch?mpd().replace('</Representation></AdaptationSet>',
  '</Representation><Representation id="high" height="720"><BaseURL>high.mp4</BaseURL><SegmentBase indexRange="100-155"><Initialization range="0-99"/></SegmentBase></Representation></AdaptationSet>'):mpd();
 const storage={},calls=[],fetches=[],preferences=[],player={page:'https://page.test/watch',timeOrigin:1,token:'v1',stamp:'1',current:'blob:fixture',sources:[],height:360,width:640,duration:4,videoCount:1,title:'Fixture',manifests:[{url:'https://cdn.test/a.mpd',type:'application/dash+xml',page:'https://page.test/watch',time:Date.now(),text:presentation}]};
 const local={settings:{cookies:false}};
 const api={runtime:{},storage:{local:{get:async()=>({...local}),set:async values=>Object.assign(local,values)},session:{get:async key=>({[key]:storage[key]}),set:async v=>Object.assign(storage,v)}},
 tabs:{get:async()=>({id:1,url:player.page,incognito:false})},permissions:{contains:async()=>permission},
 scripting:{executeScript:async()=>[{frameId:0,documentId:'doc',result:player}]},webRequest:{onHeadersReceived:{addListener(){}}}};
 const nativeRequest=async (m,beforeSend,validateSelection)=>{
  if(m.action==='preferences'){preferences.push(m);return {ok:true,adaptiveResources,mediaReceipts:1};}
  if(m.action==='hello')return {ok:true,capabilities:['dash-single-period-timing']};
  await validateSelection?.(m);beforeSend?.();calls.push(m);return {ok:true,id:'indexed-fixture-job'};
 };
 const sandbox={UdmMedia:M,URL,crypto,Uint8Array,DataView,AbortController,TextEncoder,TextDecoder,setTimeout,clearTimeout,navigator:{userAgent:'fixture'},nativeRequest,
 fetch:async(url,options)=>{fetches.push({url,options});if(change)player.stamp='2';return new Response(payload,{status,headers:{'Content-Range':range,'Content-Length':length}});}};
 if(receipts)sandbox.mediaHandoffs=require('../browser/chromium/media-receipts.js').create(api,nativeRequest);
 vm.createContext(sandbox);vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser/chromium/sites.js'),'utf8')+'\nglobalThis.sites=UdmSites;',sandbox);sandbox.sites.install(api);
 const message={token:'v1',page:player.page},sender={tab:{id:1},frameId:0,documentId:'doc',url:player.page};
 return {calls,fetches,preferences,run:async output=>{const list=await sandbox.sites.list(message,sender);assert.equal(fetches.length,0);await sandbox.sites.download({...message,key:list.choices[0].key,...(output?{output}:{})},sender);},
  review:async desktop=>{
   const list=await sandbox.sites.list(message,sender);
   assert.equal(list.choices.length,2);
   return sandbox.sites.reviewVideoBatch({...message,keys:list.choices.map(choice=>choice.key)},sender,async(rows,fromMenu,validate,prepare)=>{
    assert(fromMenu);
    const prepared=await Promise.all(rows.map(row=>prepare(row,desktop)));
    await validate();
    return prepared;
   });
  }};
}
(async()=>{
 let h=harness();await h.run();assert.equal(h.calls.length,1);assert.equal(h.fetches.length,2);assert(h.fetches.every(x=>x.options.headers.Range==='bytes=100-155'&&x.options.credentials==='omit'&&x.options.redirect==='error'));assert.equal(h.calls[0].plan.periods.length,1);assert(h.calls[0].plan.periods[0].tracks.every(t=>!t.index&&t.segments.length===3));passed++;console.log('PASS Only selected indexes are fetched before exact native range handoff');
 h=harness();await h.run('audio');assert.equal(h.fetches.length,1);assert.match(h.fetches[0].url,/a\.mp4$/);assert.equal(h.calls[0].plan.periods[0].tracks[0].kind,'audio');passed++;console.log('PASS Audio-only never requests the video index or media');
 h=harness({receipts:true});
 await h.run();
 assert.equal(h.preferences.length,1);
 assert.equal(h.fetches.length,2);
 assert.equal(h.calls.length,1);
 assert.match(h.calls[0].mediaToken,/^\d{13}-/);
 h=harness({receipts:true,adaptiveResources:0});
 await assert.rejects(h.run,/Update.*indexed DASH/);
 assert.equal(h.preferences.length,1);
 assert.equal(h.fetches.length,0);
 assert.equal(h.calls.length,0);
 passed++;console.log('PASS Indexed DASH and its receipt share one fresh preferences read');
 h=harness({batch:true});
 const batch=await h.review({ok:true,adaptiveResources:1});
 assert.equal(batch.length,2);
 assert(batch.every(request=>request.plan.periods[0].tracks.every(track=>!track.index)));
 assert.equal(h.preferences.length,0);
 h=harness({batch:true});
 await assert.rejects(()=>h.review({ok:true,adaptiveResources:0}),/Update.*indexed DASH/);
 assert.equal(h.preferences.length,0);
 assert.equal(h.fetches.length,0);
 passed++;console.log('PASS Indexed DASH batch uses the desktop preferences already read for review');
 for(const [name,options] of [['ignored range',{status:200}],['wrong range',{range:'bytes 99-154/1000'}],['truncated index',{payload:index().subarray(0,55)}],['wrong length',{length:'55'}],['missing site permission',{permission:false}],['navigation during index fetch',{change:true}]]){
  h=harness(options);await assert.rejects(h.run());assert.equal(h.calls.length,0);passed++;console.log('PASS '+name+' creates no native download');
 }
 assert.equal(fs.readFileSync(path.join(__dirname,'../browser/chromium/media.js'),'utf8'),fs.readFileSync(path.join(__dirname,'../browser/firefox/media.js'),'utf8'));
 assert.equal(fs.readFileSync(path.join(__dirname,'../browser/chromium/sites.js'),'utf8'),fs.readFileSync(path.join(__dirname,'../browser/firefox/sites.js'),'utf8'));
 passed++;console.log('PASS Firefox and Chromium share the exact indexed-media implementation');
 console.log('ALL '+passed+' DASH INDEX CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});

