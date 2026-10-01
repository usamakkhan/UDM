'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),crypto=require('node:crypto');
const M=require('../browser/chromium/media.js'),{box}=require('./support/sidx-tree.cjs');
let passed=0;async function check(name,fn){await fn();passed++;console.log('PASS '+name);}
function external({version=1,prefix=0,offset=100,nested=false}={}){
 const index=box({version,offset,parts:[{length:100,duration:2000,index:nested},{length:120,duration:2000}]});
 const free=Buffer.alloc(prefix);if(prefix){free.writeUInt32BE(prefix);free.write('free',4);}
 return Buffer.concat([free,index]);
}
function mpd({locator='<RepresentationIndex sourceURL="https://index.test/v.idx"/>',audio=false,init='<Initialization range="0-99"/>',attrs=''}={}){
 return '<MPD type="static"><Period><AdaptationSet mimeType="video/mp4"><Representation height="360"><BaseURL>https://media.test/v.mp4</BaseURL><SegmentBase '+attrs+'>'+init+locator+'</SegmentBase></Representation></AdaptationSet>'+
 (audio?'<AdaptationSet mimeType="audio/mp4" lang="es"><Representation id="es"><BaseURL>https://media.test/a.mp4</BaseURL><SegmentBase><Initialization range="0-99"/><RepresentationIndex sourceURL="https://index.test/a.idx"/></SegmentBase></Representation></AdaptationSet>':'')+'</Period></MPD>';
}
function harness({ranged=false,credentials=false,denied='',change=0,mutate=()=>{},audio=false,capability=true,index=external()}={}){
 const stored={},calls=[],fetches=[],page='https://page.test/watch',player={page,timeOrigin:1,token:'p1',stamp:'1',current:'blob:video',sources:[],height:360,width:640,duration:4,videoCount:1,title:'External fixture'};
 player.manifests=[{url:'https://page.test/master.mpd',type:'application/dash+xml',page,time:Date.now(),text:mpd({audio,locator:'<RepresentationIndex sourceURL="https://index.test/v.idx"'+(ranged?' range="8-'+(index.length+7)+'"':'')+'/>'})}];
 const api={runtime:{},storage:{local:{get:async()=>({settings:{cookies:credentials}})},session:{get:async key=>({[key]:stored[key]}),set:async value=>Object.assign(stored,value)}},tabs:{get:async()=>({id:1,url:page,incognito:false})},permissions:{contains:async request=>!request.origins?.some(x=>x.includes(denied)&&denied)},cookies:{getAll:async()=>[]},scripting:{executeScript:async()=>[{frameId:0,documentId:'document1',result:player}]},webRequest:{onHeadersReceived:{addListener(){}}}};
 const sandbox={UdmMedia:M,URL,crypto,Uint8Array,DataView,AbortController,TextEncoder,TextDecoder,setTimeout,clearTimeout,navigator:{userAgent:'fixture'},nativeRequest:async m=>{if(m.action==='preferences')return {ok:true,adaptiveResources:capability?1:0};calls.push(m);return {ok:true};},requestContext:{resolve:({url})=>({method:'GET',headers:{Authorization:url.includes('index.test')?'Bearer index-only':'Bearer media-only'}})},
 fetch:async(url,options)=>{
  fetches.push({url,options});const isIndex=url.includes('index.test'),span=/bytes=(\d+)-(\d+)/.exec(options.headers.Range||'');
  const payload=isIndex?Buffer.from(index):Buffer.from([7]),total=isIndex?index.length+(ranged?8:0):320;
  const response={status:span?206:200,bytes:payload,headers:{'Content-Length':String(payload.length),ETag:isIndex?'"index-version"':'"media-version"',...(span?{'Content-Range':'bytes '+span[1]+'-'+span[2]+'/'+total}:{})},url};
  mutate(response,{isIndex,number:fetches.length,span});if(change===fetches.length)player.stamp='changed';
  const result=new Response(response.bytes,{status:response.status,headers:response.headers});Object.defineProperty(result,'url',{value:response.url});return result;
 }};
 vm.createContext(sandbox);vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser/chromium/sites.js'),'utf8')+'\nglobalThis.sites=UdmSites;',sandbox);sandbox.sites.install(api);
 const message={page,token:'p1'},sender={tab:{id:1},frameId:0,documentId:'document1',url:page};
 return {calls,fetches,run:async output=>{const list=await sandbox.sites.list(message,sender);assert.equal(fetches.length,0);return sandbox.sites.download({...message,key:list.choices[0].key,...(output?{output}:{})},sender);}};
}
(async()=>{
 for(const version of [0,1])await check('External version '+version+' offsets use the media-file origin',()=>{const data=external({version});assert.deepEqual(M.sidxExternal(data,37,data.length+37,320),[{start:100,length:100},{start:200,length:120}]);});
 await check('A prefixed box and nonzero index-file range do not shift media offsets',()=>{const data=external({prefix:16});assert.equal(M.sidxExternal(data,700,data.length+700,320)[0].start,100);});
 await check('Media offsets above 32 bits stay exact with a small external index',()=>{const start=4294967396,data=external({offset:start});assert.equal(M.sidxExternal(data,0,data.length,start+220)[0].start,start);});
 for(const [name,fn] of [
  ['out-of-bounds media',()=>M.sidxExternal(external(),0,64,319)],
  ['out-of-bounds index',()=>M.sidxExternal(external(),100,120,320)],
  ['nested external references',()=>M.sidxExternal(external({nested:true}),0,64,320)],
  ['multiple external indexes',()=>{const bytes=Buffer.concat([external(),external()]);M.sidxExternal(bytes,0,bytes.length,320);}],
  ['unsafe media length',()=>M.sidxExternal(external(),0,64,Number.MAX_SAFE_INTEGER+1)]
 ])await check('External index rejects '+name,()=>assert.throws(fn));
 await check('MPD describes a whole external index without pretending it is a media file',()=>{const t=M.dash(mpd(),'https://page.test/master.mpd')[0].plan.tracks[0];assert.deepEqual(t.index,{url:'https://index.test/v.idx',mediaUrl:'https://media.test/v.mp4'});});
 await check('MPD external byte range is retained exactly',()=>{const t=M.dash(mpd({locator:'<RepresentationIndex sourceURL="v.idx" range="16-79"/>'}),'https://page.test/master.mpd')[0].plan.tracks[0];assert.deepEqual(t.index,{url:'https://media.test/v.idx',start:16,length:64,mediaUrl:'https://media.test/v.mp4'});});
 await check('Same-file RepresentationIndex uses the existing integrated-index path',()=>{const t=M.dash(mpd({locator:'<RepresentationIndex range="100-163"/>'}),'https://page.test/master.mpd')[0].plan.tracks[0];assert.deepEqual(t.index,{url:'https://media.test/v.mp4',start:100,length:64});});
 await check('Representation index and initialization inherit from the adaptation set',()=>{const xml='<MPD><Period><AdaptationSet mimeType="video/mp4"><SegmentBase><Initialization range="0-99"/><RepresentationIndex sourceURL="v.idx"/></SegmentBase><Representation height="360"><BaseURL>https://media.test/v.mp4</BaseURL></Representation></AdaptationSet></Period></MPD>';assert.equal(M.dash(xml,'https://page.test/master.mpd')[0].plan.tracks[0].index.url,'https://media.test/v.idx');});
 for(const [name,options] of [
  ['ambiguous index locators',{attrs:'indexRange="100-163"'}],
  ['duplicate index elements',{locator:'<RepresentationIndex sourceURL="a"/><RepresentationIndex sourceURL="b"/>'}],
  ['index credentials',{locator:'<RepresentationIndex sourceURL="https://u:p@index.test/file"/>'}],
  ['non-network index',{locator:'<RepresentationIndex sourceURL="file:///C:/local"/>'}],
  ['oversized explicit range',{locator:'<RepresentationIndex sourceURL="a" range="0-524288"/>'}],
  ['whole same-file index',{locator:'<RepresentationIndex/>'}]
 ])await check('MPD rejects '+name,()=>assert.throws(()=>M.dash(mpd(options),'https://page.test/master.mpd')));
 for(const ranged of [false,true])await check((ranged?'Ranged':'Whole-file')+' external handoff probes only one media byte and binds the media ETag',async()=>{
  const h=harness({ranged});await h.run();assert.equal(h.fetches.length,2);assert.equal(h.fetches[0].options.headers.Range,ranged?'bytes=8-71':undefined);assert.equal(h.fetches[1].options.headers.Range,'bytes=0-0');
  const plan=h.calls[0].plan;assert.deepEqual(JSON.parse(JSON.stringify(plan.resources)),[{url:'https://media.test/v.mp4',size:320,etag:'"media-version"'}]);assert(plan.tracks[0].segments.every(s=>s.url==='https://media.test/v.mp4'));assert.equal(plan.tracks[0].segments.length,3);assert(!JSON.stringify(plan).includes('index.test'));assert(h.fetches.every(f=>f.options.credentials==='omit'&&f.options.redirect==='error'&&!f.options.headers.Authorization));
 });
 await check('Chunked whole index is bounded without requiring Content-Length',async()=>{const h=harness({mutate:(r,c)=>{if(c.isIndex)delete r.headers['Content-Length'];}});await h.run();assert.equal(h.calls.length,1);});
 await check('Credential opt-in resolves index and media authorization separately',async()=>{const h=harness({credentials:true});await h.run();assert.equal(h.fetches[0].options.headers.Authorization,'Bearer index-only');assert.equal(h.fetches[1].options.headers.Authorization,'Bearer media-only');assert(h.fetches.every(f=>f.options.credentials==='include'));});
 await check('Audio-only fetches no video index or media probe',async()=>{const h=harness({audio:true});await h.run('audio');assert.equal(h.fetches.length,2);assert(h.fetches.every(f=>/\/a\.(idx|mp4)$/.test(f.url)));assert.equal(h.calls[0].plan.tracks[0].kind,'audio');});
 for(const [name,options] of [
  ['old native capability',{capability:false}],
  ['missing index-host permission',{denied:'index.test'}],
  ['missing media-host permission',{denied:'media.test'}],
  ['page navigation after index capture',{change:1}],
  ['page navigation after media probe',{change:2}],
  ['index redirect',{mutate:(r,c)=>{if(c.isIndex){r.status=302;r.headers.Location='https://elsewhere.test/';}}}],
  ['wrong resolved index URL',{mutate:(r,c)=>{if(c.isIndex)r.url='https://elsewhere.test/';}}],
  ['media probe ignores range',{mutate:(r,c)=>{if(!c.isIndex)r.status=200;}}],
  ['wrong media probe range',{mutate:(r,c)=>{if(!c.isIndex)r.headers['Content-Range']='bytes 1-1/320';}}],
  ['media file too short',{mutate:(r,c)=>{if(!c.isIndex)r.headers['Content-Range']='bytes 0-0/319';}}],
  ['invalid probe total',{mutate:(r,c)=>{if(!c.isIndex)r.headers['Content-Range']='bytes 0-0/9007199254740992';}}],
  ['media redirect',{mutate:(r,c)=>{if(!c.isIndex)r.status=302;}}],
  ['compressed index',{mutate:(r,c)=>{if(c.isIndex)r.headers['Content-Encoding']='gzip';}}],
  ['index declares oversize',{mutate:(r,c)=>{if(c.isIndex)r.headers['Content-Length']='524289';}}],
  ['chunked index exceeds limit',{mutate:(r,c)=>{if(c.isIndex){delete r.headers['Content-Length'];r.bytes=Buffer.alloc(524289);}}}],
  ['truncated external body',{mutate:(r,c)=>{if(c.isIndex)r.bytes=r.bytes.subarray(1);}}],
  ['empty external body',{mutate:(r,c)=>{if(c.isIndex){r.bytes=Buffer.alloc(0);delete r.headers['Content-Length'];}}}],
  ['nested external hierarchy',{index:external({nested:true})}],
 ])await check(name+' produces no native job',async()=>{const h=harness(options);await assert.rejects(h.run());assert.equal(h.calls.length,0);assert(h.fetches.length<=2);});
 console.log('ALL '+passed+' EXTERNAL DASH CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});
