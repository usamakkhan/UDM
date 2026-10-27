'use strict';
const {chromium}=require('playwright'),fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const output=process.argv[2];if(!output)throw Error('Pass a fresh output directory.');
fs.mkdirSync(output,{recursive:false});const results=[];let browser;
(async()=>{
 browser=await chromium.launch({channel:'chrome',headless:true});
 for(const variant of ['chromium','firefox']){
  const page=await browser.newPage();
  await page.route('**/*',route=>route.fulfill({contentType:'text/html',body:'<!doctype html><title>Feed capture fixture</title><video data-udm-player="one" data-udm-epoch="1" style="width:300px;height:180px"></video><video data-udm-player="two" data-udm-epoch="1" style="width:300px;height:180px"></video>'}));
  await page.goto('https://x.com/udm-fixture/status/123');
  await page.addScriptTag({path:path.resolve('browser',variant,'media.js')});
  await page.addScriptTag({content:fs.readFileSync(path.resolve('browser',variant,'sites.js'),'utf8')+'\nglobalThis.feedSites=UdmSites;'});
  const observed=await page.evaluate(async()=>{
   const feed='https://video.twimg.com/amplify_video/123/',other='https://video.twimg.com/amplify_video/456/';
   const videos=[...document.querySelectorAll('video')];
   for(let i=0;i<videos.length;i++){videos[i].src=URL.createObjectURL(new Blob([],{type:'video/mp4'}));videos[i].poster='https://pbs.twimg.com/amplify_video_thumb/'+(i?456:123)+'/img/a.jpg';}
   const session={},sent=[],event={addListener(){}},api={storage:{local:{get:async()=>({settings:{}})},session:{get:async key=>({[key]:session[key]}),set:async values=>Object.assign(session,values)}},tabs:{get:async id=>({id,url:location.href,incognito:false})},scripting:{executeScript:async args=>[{frameId:0,documentId:'doc',result:args.func(...args.args)}]},permissions:{contains:async()=>true,onAdded:event,onRemoved:event},runtime:{onStartup:event}};
   globalThis.handoff=async item=>{sent.push(item);return {ok:true};};feedSites.install(api);
   const row=(url,kind='hls')=>({url,kind,frameId:0,documentId:'doc',page:location.href,time:Date.now()});
   session['site-media:7']=[row(feed+'init-a.mp4','direct'),row(feed+'part.ts','direct'),row(feed+'movie.mp4','direct'),row(other+'movie.mp4','direct')];
   const sender={tab:{id:7},frameId:0,documentId:'doc',url:location.href},message={page:location.href,token:'one'};
   const direct=await feedSites.list(message,sender);await feedSites.download({...message,key:direct.choices[0].key},sender);
   session['site-media:7'].push(row(feed+'master.m3u8'));
   globalThis.fetch=async address=>new Response(address.endsWith('master.m3u8')?'#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=1000000,RESOLUTION=1280x720,CODECS="avc1.42c01e,mp4a.40.2"\n720.m3u8\n':'#EXTM3U\n#EXTINF:2,\npart.ts\n#EXT-X-ENDLIST');
   const rich=await feedSites.list(message,sender);
   videos[0].poster='https://pbs.twimg.com/amplify_video_thumb/456/img/a.jpg';let stale=false;try{await feedSites.download({...message,key:rich.choices[0].key},sender);}catch(e){stale=/changed/.test(e.message);}
   videos[0].poster='https://pbs.twimg.com/amplify_video_thumb/123/img/a.jpg';globalThis.fetch=async()=>{throw Error('Unavailable');};const fallback=await feedSites.list(message,sender);
   const originalSent=sent.map(x=>x.url);
   videos[0].poster='https://pbs.twimg.com/media/custom?format=webp&name=medium';
   Object.defineProperty(videos[0],'duration',{value:59.893,configurable:true});
   const record={media_url_https:'https://pbs.twimg.com/media/custom.jpg',video_info:{duration_millis:59893,variants:[{url:feed+'movie.mp4',content_type:'video/mp4'}]}};
   videos[0].__reactProps$fixture={media:record};
   const custom=await feedSites.list(message,sender);await feedSites.download({...message,key:custom.choices[0].key},sender);
   const customSent=sent.at(-1).url;
   record.video_info.variants[0].url=other+'movie.mp4';let changed=false;
   try{await feedSites.download({...message,key:custom.choices[0].key},sender);}catch(e){changed=/changed/.test(e.message);}
   videos[0].__reactProps$fixture={records:[record,{...record,video_info:{...record.video_info,variants:[{url:feed+'movie.mp4',content_type:'video/mp4'}]}}]};
   const ambiguous=feedSites.readPlayer('one').twitter===null;
   videos[0].__reactProps$fixture={media:record};record.video_info.duration_millis=840;
   const durationRejected=feedSites.readPlayer('one').twitter===null;
   record.video_info.duration_millis=59893;record.video_info.variants[0].url='https://video.twimg.com.evil.test/movie.mp4';
   const hostRejected=feedSites.readPlayer('one').twitter===null;
   record.video_info.variants=[{url:feed+'vid/avc1/1280x720/full.mp4',content_type:'video/mp4'},{url:feed+'master.m3u8',content_type:'application/x-mpegURL'}];
   globalThis.fetch=async address=>new Response(address.endsWith('master.m3u8')?'#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=1000000,RESOLUTION=1280x720,CODECS="avc1.42c01e,mp4a.40.2"\n720.m3u8\n':'#EXTM3U\n#EXTINF:2,\npart.ts\n#EXT-X-ENDLIST');
   const originalCatalog=await feedSites.list(message,sender);
   await feedSites.download({...message,key:originalCatalog.choices[0].key},sender);
   const originalQuality={height:originalCatalog.choices[0].height,label:originalCatalog.choices[0].label,url:sent.at(-1).url,count:originalCatalog.choices.length};
   record.video_info.variants=[{url:feed+'movie.mp4',content_type:'video/mp4'}];
   delete videos[0].__reactProps$fixture;
   record.video_info.variants[0].url=feed+'movie.mp4';
   const script=document.createElement('script');script.type='text/plain';script.setAttribute('data-tsr-stream-part','');
   script.textContent='data='+JSON.stringify(JSON.stringify({data:{media_entities2:[record]}}))+';throw Error("must never execute");';document.body.append(script);
   const serialized=feedSites.readPlayer('one').twitter?.sources[0].url;
   record.media_url_https='https://pbs.twimg.com/media/wrong.jpg';script.textContent='data='+JSON.stringify(JSON.stringify({media:[record]}));
   const wrongSerialized=feedSites.readPlayer('one').twitter===null;
   return {direct:direct.choices.length,sent:originalSent,rich:rich.choices.map(x=>({height:x.height,container:x.container})),stale,fallback:fallback.choices.length,customSent,changed,ambiguous,durationRejected,hostRejected,serialized,wrongSerialized,originalQuality};
  });
  for(const [name,run] of [
   ['actual DOM poster binds the selected player among two videos',()=>assert.equal(observed.direct,1)],
   ['only the complete selected video reaches handoff',()=>assert.deepEqual(observed.sent,['https://video.twimg.com/amplify_video/123/movie.mp4'])],
   ['rich catalog replaces raw offers with MP4 and TS qualities',()=>assert.deepEqual(observed.rich,[{height:720,container:'mp4'},{height:720,container:'ts'}])],
   ['poster mutation invalidates the saved offer',()=>assert.equal(observed.stale,true)],
   ['unavailable master retains direct fallback',()=>assert.equal(observed.fallback,1)],
   ['custom thumbnail metadata selects the complete intended video',()=>assert.equal(observed.customSent,'https://video.twimg.com/amplify_video/123/movie.mp4')],
   ['changed custom media metadata invalidates prior offers',()=>assert.equal(observed.changed,true)],
   ['conflicting custom thumbnail records are rejected',()=>assert.equal(observed.ambiguous,true)],
   ['different playback duration is rejected',()=>assert.equal(observed.durationRejected,true)],
   ['spoofed media hosts are rejected',()=>assert.equal(observed.hostRejected,true)],
   ['serialized X page JSON matches extensionless and JPG thumbnails without executing code',()=>assert.equal(observed.serialized,'https://video.twimg.com/amplify_video/123/movie.mp4')],
   ['serialized page metadata for a different thumbnail is rejected',()=>assert.equal(observed.wrongSerialized,true)],
   ['complete MP4 precedes equal-resolution HLS and uses direct native transfer',()=>{assert.equal(observed.originalQuality.height,720);assert.match(observed.originalQuality.label,/Original file/);assert.equal(observed.originalQuality.url,'https://video.twimg.com/amplify_video/123/vid/avc1/1280x720/full.mp4');assert.equal(observed.originalQuality.count,3);}]]){run();results.push({variant,name,passed:true});console.log('PASS '+variant+' '+name);}
  await page.close();
 }
})().catch(e=>{results.push({passed:false,error:e.message});console.error(e);process.exitCode=1;}).finally(async()=>{await browser?.close();fs.writeFileSync(path.join(output,'results.json'),JSON.stringify({engine:'Headless Chrome; both extension source variants; native handoff mocked',results},null,2));});
