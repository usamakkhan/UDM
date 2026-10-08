'use strict';
const assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),M=require('../browser/chromium/media.js');
const base='https://media.test/',page='https://player.test/watch',leaf='#EXTM3U\n#EXTINF:2,\nsegment.ts\n#EXT-X-ENDLIST\n';
const variant=(uri,height=360)=>'#EXT-X-STREAM-INF:BANDWIDTH=1000000,RESOLUTION=640x'+height+',CODECS="avc1.42c01e,mp4a.40.2"\n'+uri+'\n';
const master=(...entries)=>'#EXTM3U\n'+entries.join('');let checks=0;
function pass(name){checks++;console.log('PASS '+name);}
function fixture(roots,responses,{deniedOrigin='',changeAfterFetch=false,family='chromium',liveSubtitles=true}={}){
 const session={},fetches=[],canceled=[],sent=[],event=()=>({addListener(){}});
 const player={page,token:'test-player',stamp:'1',current:'blob:'+page,sources:[],height:360,width:640,videoCount:1,title:'Catalog fixture',encrypted:false,manifests:roots.map(url=>({url,type:'application/vnd.apple.mpegurl',page,time:Date.now()}))};
 const api={storage:{local:{get:async()=>({settings:{}})},session:{get:async k=>({[k]:session[k]}),set:async v=>Object.assign(session,v),remove:async()=>{}}},tabs:{get:async id=>({id,url:page,incognito:false})},scripting:{executeScript:async()=>[{frameId:0,documentId:'doc',result:structuredClone(player)}]},permissions:{contains:async({origins})=>!origins.some(x=>deniedOrigin&&x.startsWith(deniedOrigin)),onAdded:event(),onRemoved:event()},runtime:{onStartup:event(),sendNativeMessage:async(_,message)=>{sent.push(message);return message.action==='hello'?{ok:true,capabilities:['live-hls',...(liveSubtitles?['live-hls-subtitles']:[])]}:{ok:true,id:'captured-live'};}}};
 const sandbox={navigator:{userAgent:'test'},UdmMedia:require(path.join(__dirname,'../browser',family,'media.js')),URL,Date,crypto,AbortController,TextEncoder,TextDecoder,setTimeout,clearTimeout,fetch:async(address,{signal})=>{
  assert(!signal.aborted);fetches.push(address);const result=typeof responses==='function'?responses(address):responses[address];assert(result,address);const value=typeof result==='string'?{url:address,text:result}:result;
  if(changeAfterFetch)player.stamp='2';
  return {ok:true,url:value.url,body:new ReadableStream({start(c){c.enqueue(new TextEncoder().encode(value.text));c.close();},cancel(){canceled.push(address);}})};
 }};
 vm.createContext(sandbox);vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser',family,'sites.js'),'utf8')+'\nglobalThis.sites=UdmSites;',sandbox);sandbox.sites.install(api);
 return {fetches,canceled,session,sent,download:(choice,subtitleKey)=>sandbox.sites.download({page,token:player.token,key:choice.key,subtitleKey},{tab:{id:7},frameId:0,documentId:'doc',url:page}),list:()=>sandbox.sites.list({page,token:player.token},{tab:{id:7},frameId:0,documentId:'doc',url:page})};
}
(async()=>{
 const table={[base+'outer.m3u8']:master(variant('inner.m3u8')), [base+'inner.m3u8']:master(variant('360.m3u8'),variant('720.m3u8',720)),[base+'360.m3u8']:leaf,[base+'720.m3u8']:leaf};
 for(const order of [['outer','inner'],['inner','outer']]){
  const f=fixture(order.map(x=>base+x+'.m3u8'),table),catalog=await f.list();assert.equal(catalog.choices.length,4);assert.equal(new Set(f.fetches).size,4);assert.equal(f.fetches.length,4);assert.equal(catalog.choices.filter(x=>x.height===720).length,2);
  pass('Observed parent/child playlists deduplicate in '+order.join('-')+' arrival order');
 }
 // Root paths and child paths are deliberately separate so all reads are independent.
 let f=fixture(Array.from({length:8},(_,i)=>base+'root'+i+'.m3u8'),address=>/\/root\d+\.m3u8$/.test(address)?master(...Array.from({length:60},(_,i)=>variant('leaf'+new URL(address).pathname.slice(5).split('.')[0]+'-'+i+'.m3u8'))):leaf);
 let catalog=await f.list();assert.equal(f.fetches.length,64);assert.equal(new Set(f.fetches).size,64);assert.match(catalog.note,/64-playlist/);assert(catalog.choices.length>0);pass('Multiple observed roots share one 64-playlist read budget');
 const huge=leaf+'#'+'x'.repeat(1500000);
 f=fixture(Array.from({length:8},(_,i)=>base+'root'+i+'.m3u8'),address=>/\/root\d+\.m3u8$/.test(address)?master(variant('leaf'+new URL(address).pathname.slice(5))):huge);
 catalog=await f.list();assert.match(catalog.note,/eight MiB/);assert(f.fetches.length<16);assert(catalog.choices.length<16);pass('Multiple observed roots share one eight MiB text budget');
 f=fixture([base+'root.m3u8'],{[base+'root.m3u8']:master(variant('denied.m3u8')),[base+'denied.m3u8']:{url:'https://denied.test/leaf.m3u8',text:leaf}},{deniedOrigin:'https://denied.test'});
 catalog=await f.list();assert.equal(catalog.choices.length,0);assert.match(catalog.note,/redirected media host needs site permission/);assert(!f.fetches.some(x=>x.startsWith('https://denied.test')));pass('A redirected nested playlist still requires its media-host permission');
 f=fixture([base+'root.m3u8'],{[base+'root.m3u8']:master(variant('leaf.m3u8')),[base+'leaf.m3u8']:leaf},{changeAfterFetch:true});
 await assert.rejects(f.list,/video changed/);assert(!f.session['site-offers:7']);pass('Navigation during nested discovery cannot publish stale offers');
 const controller=new AbortController();controller.abort();let reads=0;
 await assert.rejects(()=>M.hlsCatalog({url:base+'root.m3u8',text:master(variant('leaf.m3u8'))},async()=>{reads++;return {url:base+'leaf.m3u8',text:leaf};},controller.signal),/timed out/);assert.equal(reads,0);pass('An already canceled catalog performs no playlist reads');
 const liveMaster='#EXTM3U\n#EXT-X-MEDIA:TYPE=SUBTITLES,GROUP-ID="captions",NAME="English",LANGUAGE="en",URI="captions.m3u8"\n#EXT-X-STREAM-INF:BANDWIDTH=1000000,RESOLUTION=640x360,CODECS="avc1.42c01e,mp4a.40.2",SUBTITLES="captions"\nlive.m3u8\n';
 for(const family of ['chromium','firefox']){
  const live=fixture([base+'live-master.m3u8'],{[base+'live-master.m3u8']:liveMaster,[base+'live.m3u8']:'#EXTM3U\n#EXT-X-TARGETDURATION:2\n#EXTINF:2,\nsegment.ts\n',[base+'captions.m3u8']:'#EXTM3U\n#EXT-X-TARGETDURATION:2\n#EXTINF:2,\ncaption.vtt\n'},{family});
  const result=await live.list();assert(result.choices.some(choice=>choice.label.includes('Live')));const choice=result.choices.find(choice=>choice.container==='mp4');assert.equal(choice.subtitleOptions.length,1);assert(result.choices.filter(choice=>choice.container==='ts').every(choice=>!choice.subtitleOptions?.length));pass(family+' live subtitles are available only for MP4');
  await live.download(choice,choice.subtitleOptions[0].key);const submitted=live.sent.find(message=>message.action==='adaptive');assert(submitted?.plan.live);assert.equal(submitted.plan.tracks.at(-1).kind,'subtitle');assert.equal(submitted.plan.tracks.at(-1).playlist,base+'captions.m3u8');assert.equal(submitted.plan.subtitleLanguage,'en');pass(family+' live subtitle handoff includes selected playlist and language');
  const oldDesktop=fixture([base+'live-master.m3u8'],{[base+'live-master.m3u8']:liveMaster,[base+'live.m3u8']:'#EXTM3U\n#EXT-X-TARGETDURATION:2\n#EXTINF:2,\nsegment.ts\n'},{family,liveSubtitles:false});const oldList=await oldDesktop.list(),oldChoice=oldList.choices.find(choice=>choice.container==='mp4');await assert.rejects(()=>oldDesktop.download(oldChoice,oldChoice.subtitleOptions[0].key),/Update.*live subtitles/);assert(!oldDesktop.sent.some(message=>message.action==='adaptive'));pass(family+' old desktop capability cannot receive live subtitle jobs');
 }
 console.log(checks+' HLS site integration checks passed');
})().catch(error=>{console.error(error);process.exitCode=1;});
