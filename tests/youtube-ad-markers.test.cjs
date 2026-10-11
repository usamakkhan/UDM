'use strict';
const assert=require('node:assert/strict'),path=require('node:path'),vm=require('node:vm'),fs=require('node:fs');
const id='Q3TI27IN7X0',page='https://www.youtube.com/watch?v='+id;let passed=0;
const markers=['advideo=1','advideo=0','el=adunit','el=normal&el=adunit','advideo=&advideo=yes'];
for(const browser of ['chromium','firefox']){
 const dir=path.resolve(__dirname,'../browser',browser),{UdmFormats,readYouTubeFormats}=require(path.join(dir,'formats.js'));
 const url=(itag,query='')=>`https://rr1.googlevideo.com/videoplayback?id=content&itag=${itag}&expire=4102444800&lmt=100&${query}`;
 const video=query=>({id:'137',height:1080,mime:'video/mp4; codecs="avc1.640028"',muxed:true,url:url(137,query)});
 const audio=query=>({id:'140',mime:'audio/mp4; codecs="mp4a.40.2"',url:url(140,query),lastModified:'100'});
 for(const marker of markers){
  const snap={videoId:id,formats:[video(marker)],levels:['hd1080'],durationMs:10000};
  assert.equal(UdmFormats.choices(snap,id).length,0,browser+' rejects ad-marked video '+marker);passed++;
  assert.equal(UdmFormats.audioChoices({...snap,formats:[audio(marker)]},id,true).length,0,browser+' rejects ad-marked audio including streaming fallback');passed++;
  const base={...video('n=untransformed'),muxed:true};
  const attached=UdmFormats.attachObserved({...snap,formats:[base]},[{url:url(137,marker),itag:'137',observedAt:Date.now()}]);
  assert(!attached.formats[0].observed,browser+' cannot promote an ad observation');passed++;
  for(const cipher of [false,true]){
   const format={itag:137,height:1080,mimeType:'video/mp4; codecs="avc1.640028"',...(cipher?{signatureCipher:new URLSearchParams({url:url(137,marker),s:'fixture'}).toString()}:{url:url(137,marker)})};
   const response={videoDetails:{videoId:id},streamingData:{formats:[format]}};
   const context={URL,URLSearchParams,location:new URL(page),performance:{timeOrigin:1},document:{getElementById:()=>({getVideoData:()=>({video_id:id}),getPlayerResponse:()=>response,getAvailableQualityLevels:()=>['hd1080']})}};
   const read=vm.runInNewContext('('+readYouTubeFormats.toString()+')('+JSON.stringify(id)+')',context);
   assert.equal(read.formats.length,0);assert.equal(read.levels.length,0);passed++;
  }
  const events={},context=vm.createContext({URL,Date,console});
  vm.runInContext(fs.readFileSync(path.join(dir,'streaming-capture.js'),'utf8')+'\nglobalThis.testCapture=UdmStreamingCapture;',context);
  const event=name=>({addListener:fn=>events[name]=fn});
  context.testCapture.install({tabs:{onRemoved:event('removed'),onUpdated:event('updated')},webRequest:{onBeforeRequest:event('before'),onHeadersReceived:event('headers'),onErrorOccurred:event('error')}});
  events.before({tabId:7,frameId:0,requestId:'ad',method:'POST',url:url(137,marker+'&sabr=1')});
  assert.equal(context.testCapture.diagnostics(7).seen,undefined,browser+' rejects explicit SABR ad before request storage');passed++;
 }
 for(const query of ['','advideo=','el=detailpage','el=adunit-extra','title=adunit','redirect=https%3A%2F%2Fexample.test%2F%3Fel%3Dadunit']){
  assert.equal(UdmFormats.choices({videoId:id,formats:[video(query)]},id).length,1,browser+' preserves normal content '+query);passed++;
 }
}
console.log(JSON.stringify({passed,failed:0}));
