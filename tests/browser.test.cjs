'use strict';
const {readFileSync}=require('node:fs');
const vm=require('node:vm');
const assert=require('node:assert/strict');
const path=require('node:path');
let checks=0;
async function harness(settings={},fail=false){
  const events={}, calls=[], store={settings},session={};
  const playback=id=>'https://rr1.googlevideo.com/videoplayback?itag='+id+'&expire=4102444800';
  const player={snapshot:{videoId:'Q3TI27IN7X0',formats:[{id:'137',height:1080,fps:30,mime:'video/mp4; codecs="avc1.640028"',url:playback(137)},{id:'136',height:720,fps:30,mime:'video/mp4; codecs="avc1.4d401f"',url:''},{id:'140',mime:'audio/mp4; codecs="mp4a.40.2"',url:playback(140)}],levels:[]}};
  const event=name=>({addListener:fn=>{events[name]=fn;}});
  const api={
    runtime:{id:'test-extension',onInstalled:event('installed'),onMessage:event('message'),sendNativeMessage:async(host,msg)=>{if(msg.action==='preferences')return {ok:true,extensions:['zip','mp4'],excluded:settings.desktopExcluded||[]};calls.push(['native',msg]);if(fail)throw Error('Host disconnected');return {ok:true,id:'job-1'};}},
    storage:{local:{get:async()=>store,set:async values=>Object.assign(store,values)},session:{get:async()=>session,set:async values=>Object.assign(session,values),remove:async key=>{delete session[key];}}},
    permissions:{contains:async()=>false},cookies:{getAll:async()=>{throw Error('Cookie access should not occur');}},
    action:{setBadgeText:async()=>{}},
    downloads:{onCreated:event('download'),pause:async id=>calls.push(['pause',id]),cancel:async id=>calls.push(['cancel',id]),resume:async id=>calls.push(['resume',id])},
    contextMenus:{onClicked:event('context'),removeAll:async()=>{},create:()=>{}},
    scripting:{executeScript:async()=>[{frameId:0,result:player.snapshot}]},
    webRequest:{onHeadersReceived:event('headers')},tabs:{onRemoved:event('tabremoved'),onUpdated:event('tabupdated'),get:async()=>({url:'https://www.youtube.com/watch?v=Q3TI27IN7X0',title:'Test video',incognito:false})}
  };
  vm.runInNewContext(readFileSync(path.join(__dirname,'../browser/chromium/formats.js'),'utf8')+'\n'+readFileSync(path.join(__dirname,'../browser/chromium/background.js'),'utf8'),{chrome:api,navigator:{userAgent:'test'},URL,URLSearchParams,console,setTimeout:fn=>setImmediate(fn)});
  return {events,calls,store,session,player};
}
const check=(name)=>{checks++;console.log('PASS '+name);};
(async()=>{
  const item={id:1,url:'https://example.test/file.zip',filename:'file.zip'};
  let h=await harness();await h.events.download(item);assert.equal(h.calls.length,0);check('automatic capture is disabled by default');
  h=await harness({capture:true});await h.events.download(item);assert.deepEqual(h.calls.map(x=>x[0]),['pause','native','cancel']);check('browser download cancels only after desktop acknowledgement');
  h=await harness({capture:true},true);await h.events.download(item);assert.deepEqual(h.calls.map(x=>x[0]),['pause','native','resume']);check('failed desktop handoff resumes the browser download');
  h=await harness({capture:true,excluded:['example.test']});await h.events.download(item);assert.equal(h.calls.length,0);check('excluded hosts stay in the browser');
  h=await harness({capture:true});await h.events.download({...item,incognito:true});await h.events.download({...item,url:'blob:https://example.test/id'});assert.equal(h.calls.length,0);check('private and blob downloads are not captured');
  h=await harness({capture:true,cookies:true});await h.events.download(item);assert.equal(h.calls.find(x=>x[0]==='native')[1].cookies,'');check('cookies require explicit browser permission');
  const a=JSON.parse(readFileSync(path.join(__dirname,'../browser/chromium/manifest.json')));const b=JSON.parse(readFileSync(path.join(__dirname,'../browser/firefox/manifest.json')));assert.equal(a.manifest_version,3);assert.equal(b.browser_specific_settings.gecko.id,'udm@local.example');check('Chromium and Firefox manifests parse');
  h=await harness();
  const capture=(itag,mime,extra='')=>h.events.headers({tabId:7,url:`https://rr1.googlevideo.com/videoplayback?itag=${itag}&clen=5000000&mime=${mime}&sparams=itag,clen,mime&range=0-1000${extra}`,responseHeaders:[{name:'Content-Type',value:mime}]});
  await Promise.all([capture(137,'video/mp4'),capture(140,'audio/mp4')]);
  assert.equal(h.session['media:7'].length,2);assert.ok(h.session['media:7'].every(x=>!new URL(x.url).searchParams.has('range')));check('concurrent video/audio captures are grouped and playback range removed');
  const send=message=>new Promise(resolve=>h.events.message(message,{id:'test-extension',tab:{id:7},url:'https://www.youtube.com/watch?v=Q3TI27IN7X0'},resolve));
  let result=await send({action:'media',url:'https://www.youtube.com/watch?v=Q3TI27IN7X0',height:1080,formatKey:'137',title:'Aadat'});assert.equal(result.ok,true);let sent=h.calls.at(-1)[1];assert.equal(sent.action,'media');assert.equal(sent.url,'https://www.youtube.com/watch?v=Q3TI27IN7X0');assert.ok(sent.videoUrl.includes('itag=137'));assert.ok(sent.audioUrl.includes('itag=140'));assert.equal(sent.exactQuality,true);assert.equal(sent.formatId,'137');check('verified current-player streams reach the desktop directly with exact quality');
  await capture(136,'video/mp4','&ump=1');assert.equal(h.session['media:7'].length,2);check('encapsulated UMP responses are not treated as raw MP4');
  const directCount=h.calls.length;result=await send({action:'media',url:'https://www.youtube.com/watch?v=Q3TI27IN7X0',height:720,formatKey:'136'});assert.equal(result.ok,false);assert.match(result.error,/not captured/);assert.equal(h.calls.length,directCount);check('uncaptured streams never invoke a resolver or create a doomed desktop job');
  const before=h.calls.length;result=await send({action:'media',url:'https://www.youtube.com/watch?v=aaaaaaaaaaa',height:1080});assert.equal(result.ok,false);assert.equal(h.calls.length,before);check('stale video pages cannot reuse another video capture');
  result=await send({action:'media',url:'https://www.youtube.com/watch?v=Q3TI27IN7X0',height:999});assert.equal(result.ok,false);check('invalid quality rejected before native handoff');
  assert.equal(h.events.message({action:'media'},{id:'untrusted'},()=>{throw Error('unexpected reply');}),false);check('messages from other extensions rejected');
  h.events.tabupdated(7,{url:'https://www.youtube.com/watch?v=aaaaaaaaaaa'});await new Promise(r=>setImmediate(r));assert.equal(h.session['media:7'],undefined);check('navigation clears prior media captures');
  assert.deepEqual(a.content_scripts[0].js,['ump.js','capture.js']);assert.equal(a.content_scripts[0].world,'MAIN');assert.equal(a.content_scripts[0].run_at,'document_start');assert.deepEqual(a.content_scripts[1].js,['media.js','content.js']);assert.ok(a.host_permissions.includes('https://*.googlevideo.com/*'));assert.match(a.version,/^\d+\.\d+\.\d+$/);assert.equal(a.version,b.version);check('early main-world capture and player button are registered');
  h=await harness();
  const qualities=await send({action:'formats',url:'https://www.youtube.com/watch?v=Q3TI27IN7X0'});
  assert.equal(qualities.ok,true);assert.deepEqual(Array.from(qualities.choices,x=>x.height),[1080,720]);assert.equal(h.calls.length,0);check('1080p metadata never advertises invented 2K, 4K or 8K choices and does not invoke a resolver');
  const count=h.calls.length;const phantom=await send({action:'media',url:'https://www.youtube.com/watch?v=Q3TI27IN7X0',height:2160,formatKey:'401'});assert.equal(phantom.ok,false);assert.equal(h.calls.length,count);check('unavailable resolution is rejected before native handoff');
  h.player.snapshot={...h.player.snapshot,videoId:'aaaaaaaaaaa'};const stale=await send({action:'formats',url:'https://www.youtube.com/watch?v=Q3TI27IN7X0'});assert.equal(stale.ok,false);check('advertisement or stale player response cannot provide the current video menu');
  const {UdmFormats}=require('../browser/chromium/formats.js');
  const snapshot={videoId:'Q3TI27IN7X0',formats:[{id:'401',height:2160,fps:60,mime:'video/mp4; codecs="av01.0.12M.08"'},{id:'402',height:4320,fps:30,mime:'video/mp4; codecs="av01.0.16M.08"'}]};
  const high=UdmFormats.choices(snapshot,'Q3TI27IN7X0');assert.deepEqual(high.map(x=>x.height),[4320,2160]);assert.ok(high[0].label.includes('8K')&&high[1].label.includes('4K'));check('actual AV1 4K and 8K streams are listed by their metadata');
  assert.deepEqual(UdmFormats.choices({videoId:'Q3TI27IN7X0',levels:[]},'Q3TI27IN7X0'),[]);assert.deepEqual(UdmFormats.choices({videoId:'Q3TI27IN7X0',levels:['hd720','medium','auto']},'Q3TI27IN7X0').map(x=>x.height),[720,360]);check('player-level fallback lists only observed heights and unknown metadata stays empty');
  const expired={videoId:'Q3TI27IN7X0',formats:[{id:'18',height:360,mime:'video/mp4; codecs="avc1,mp4a"',muxed:true,url:'https://rr1.googlevideo.com/videoplayback?itag=18&expire=1'}]};
  assert.equal(UdmFormats.choices(expired,'Q3TI27IN7X0')[0].videoUrl,'');check('expired captured links require fresh browser playback');
  const fresh={...expired,formats:[{...expired.formats[0],url:'https://rr1.googlevideo.com/videoplayback?itag=18&expire=4102444800'}]};assert.ok(UdmFormats.choices(fresh,'Q3TI27IN7X0')[0].videoUrl);assert.equal(UdmFormats.choices(fresh,'Q3TI27IN7X0')[0].audioUrl,'');check('a progressive video with its own audio needs no unrelated audio capture');
  const cropped=UdmFormats.choices({...fresh,formats:[{...fresh.formats[0],height:300,qualityLabel:'360p'}]},'Q3TI27IN7X0')[0];assert.equal(cropped.height,360);assert.equal(cropped.pixelHeight,300);assert.match(cropped.label,/360p/);check('cropped video keeps its nominal quality and actual verification dimensions');
  const formatSource=readFileSync(path.join(__dirname,'../browser/chromium/formats.js'),'utf8');
  const playerPage={getVideoData:()=>({video_id:'adadadadada'}),getPlayerResponse:()=>({videoDetails:{videoId:'adadadadada'},streamingData:{}})};
  const context=vm.createContext({URL,location:{pathname:'/watch',href:'https://www.youtube.com/watch?v=Q3TI27IN7X0'},document:{getElementById:()=>playerPage}});vm.runInContext(formatSource,context);
  assert.equal(vm.runInContext("readYouTubeFormats('Q3TI27IN7X0')",context),null);check('page reader rejects an ad player identity before reading its streams');
  const owned={videoId:'Q3TI27IN7X0',formats:[{id:'137',height:1080,mime:'video/mp4; codecs="avc1"',matchUrl:'https://rr1.googlevideo.com/videoplayback?id=target&itag=137'}]};
  const observed=[{itag:137,url:'https://rr1.googlevideo.com/videoplayback?id=advertisement&itag=137',observedAt:Date.now()},{itag:137,url:'https://rr1.googlevideo.com/videoplayback?id=target&itag=137',observedAt:Date.now()}];
  const mapped=UdmFormats.attachObserved(owned,observed);assert.equal(new URL(mapped.formats[0].url).searchParams.get('id'),'target');assert.equal(UdmFormats.attachObserved(owned,[observed[0]]).formats[0].url,undefined);check('network playback URL matches the current player resource identity, never an ad with the same format');
  console.log('ALL '+checks+' BROWSER CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});
