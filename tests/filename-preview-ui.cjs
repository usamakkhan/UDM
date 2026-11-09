'use strict';
// Local HTTP and isolated native windows only; never opens the user's catalog.
const http=require('node:http'),fs=require('node:fs'),path=require('node:path'),{spawn}=require('node:child_process');
const [exe,output,crossFolder]=process.argv.slice(2);
if(!exe||!output||fs.existsSync(output))throw Error('Pass the fixture executable and a fresh output directory');
const requests=[],gets=new Map();
const refreshExpiredRequests=[],consumedReplacementUrls=new Set();
const oneUseReplacements=new Set(['/refresh-expired-replacement','/refresh-expired-replacement-capacity-short','/refresh-expired-replacement-capacity-long','/refresh-expired-replacement-chunked']);
const refreshModified='Wed, 21 Oct 2015 07:28:00 GMT';
const refreshServerDate='Fri, 23 Oct 2015 07:28:00 GMT';
const retryStatuses=new Map([['/verification-retry',200],['/forbidden-retry',403],['/expired-retry',410]]);
const streamChunk=256*1024;
// A stored ZIP with one file, readme.txt.
const previewZip=Buffer.from('UEsDBBQAAAAAAAAAIQCLBu9iGAAAABgAAAAKAAAAcmVhZG1lLnR4dFVETSBaSVAgcHJldmlldyBmaXh0dXJlClBLAQIUABQAAAAAAAAAIQCLBu9iGAAAABgAAAAKAAAAAAAAAAAAAACAAQAAAAByZWFkbWUudHh0UEsFBgAAAAABAAEAOAAAAEAAAAAAAA==','base64');
const prefixedAudio=Buffer.concat([Buffer.from('<html><body>Your download will start shortly.</body></html>\n'),
 Buffer.from([0,0,0,24]),Buffer.from('ftypM4A '),Buffer.alloc(streamChunk*2+1-12,1)]);
const server=http.createServer((req,res)=>{
 requests.push({method:req.method,path:req.url});
 if(req.url.startsWith('/zip-prefetch-once')){
  if(requests.filter(request=>request.path===req.url).length!==1||req.method!=='GET'){
   res.writeHead(410,{'Content-Length':0});res.end();return;
  }
  res.writeHead(200,{'Content-Length':previewZip.length,'Content-Type':'application/zip'});
  if(req.url.endsWith('-stream')){
   res.write(previewZip.subarray(0,64));
   setTimeout(()=>res.end(previewZip.subarray(64)),2200);
  }else res.end(previewZip);
  return;
 }
 if(req.url.startsWith('/refresh-expired-')){
  refreshExpiredRequests.push({method:req.method,path:req.url,authorization:!!req.headers.authorization,range:req.headers.range||'',ifRange:req.headers['if-range']||''});
  if(req.url.startsWith('/refresh-expired-original-date')&&refreshExpiredRequests.filter(request=>request.path===req.url).length===1){
   res.writeHead(200,{'Content-Length':streamChunk*2,'Accept-Ranges':'bytes','Content-Type':'application/zip',
    'Last-Modified':refreshModified,Date:req.url.endsWith('-legacy')?refreshModified:refreshServerDate,Connection:'close'});
   res.end(Buffer.alloc(streamChunk,1));return;
  }
  if(req.url.startsWith('/refresh-expired-original')){
   res.writeHead(410,{'Content-Length':'0'});res.end();return;
  }
  if(oneUseReplacements.has(req.url)){
   const body=Buffer.alloc(4097,1);
   const headers={ETag:'"one-use"', 'Accept-Ranges':'bytes', 'Content-Type':'application/zip'};
   if(consumedReplacementUrls.has(req.url)){
    res.writeHead(410,{'Content-Length':0});res.end();return;
   }
   consumedReplacementUrls.add(req.url);
   if(req.method==='HEAD'){
    res.writeHead(200,{...headers,'Content-Length':body.length});res.end();return;
   }
   if(req.headers.range==='bytes=0-0'){
    res.writeHead(206,{...headers,'Content-Length':1,'Content-Range':'bytes 0-0/'+body.length});res.end(body.subarray(0,1));return;
   }
   if(req.headers.range){
    res.writeHead(416,{'Content-Length':0});res.end();return;
   }
   if(req.url.endsWith('-capacity-short')){
    res.writeHead(200,{...headers,'Content-Length':body.length});res.flushHeaders();
    setTimeout(()=>res.end(body),1400);return;
   }
   if(req.url.endsWith('-chunked')){
    res.writeHead(200,headers);res.write(body.subarray(0,1024));
    setTimeout(()=>res.end(body.subarray(1024)),1000);return;
   }
   res.writeHead(200,{...headers,'Content-Length':body.length});res.end(body);return;
  }
  const dateOnly=req.url.endsWith('-date')||req.url.endsWith('-date-legacy');
  if(dateOnly||req.url==='/refresh-expired-replacement-partial'||req.url.startsWith('/refresh-expired-changed-')){
   const match=/^bytes=(\d+)-(\d+)$/.exec(req.headers.range||'');
   if(!match){res.writeHead(416);res.end();return;}
   const start=Number(match[1]),end=Number(match[2]),total=streamChunk*2;
   const body=Buffer.alloc(end-start+1,1);
   if(req.url.endsWith('changed-overlap'))body[0]=2;
   const tag=req.url.endsWith('changed-tag')?'"other-fixture"':'"refresh-fixture"';
   const identity=dateOnly?{
    'Last-Modified':req.url.endsWith('changed-date')?'Thu, 22 Oct 2015 07:28:00 GMT':refreshModified,
    Date:req.url.endsWith('weak-date')?refreshModified:refreshServerDate
   }:{ETag:tag};
   res.writeHead(206,{'Content-Length':body.length,'Content-Range':'bytes '+start+'-'+end+'/'+total,...identity,'Content-Type':'application/zip'});
   res.end(body);return;
  }
 }
 if(retryStatuses.has(req.url)&&req.method==='GET'){
  const count=(gets.get(req.url)||0)+1;gets.set(req.url,count);
  if(count===1){
   const status=retryStatuses.get(req.url);
   const body=status===200?'<!doctype html><title>Verify you are human</title>':'Download unavailable';
   res.writeHead(status,{'Content-Type':'text/html',...(status===200?{'Cf-Mitigated':'challenge'}:{}),'Content-Length':Buffer.byteLength(body)});
   res.end(body);
   return;
  }
 }
 if(req.url==='/unknown-length-browser'&&req.method==='GET'){
  setTimeout(()=>{
   res.writeHead(200,{'Content-Type':'application/zip'});
   res.write(Buffer.alloc(1024,1));
   setTimeout(()=>res.end(Buffer.alloc(3073,1)),850);
  },500);
  return;
 }
 if(req.url.startsWith('/prefixed-audio')){
  if(req.url.endsWith('-refreshed')&&
   (req.method!=='GET'||req.headers.range||requests.filter(request=>request.path===req.url).length!==1)){
   res.writeHead(410,{'Content-Length':0});res.end();return;
  }
  const hasLength=!req.url.includes('-unknown');
  res.writeHead(200,{'Content-Type':'audio/mp4',...(hasLength?{'Content-Length':prefixedAudio.length}:{})});
  res.flushHeaders();
  setTimeout(()=>{
   res.write(prefixedAudio.subarray(0,streamChunk));
   setTimeout(()=>res.end(prefixedAudio.subarray(streamChunk)),2400);
  },550);
  return;
 }
 if((req.url.startsWith('/start-')||req.url.startsWith('/later-'))&&req.method==='GET'){const count=(gets.get(req.url)||0)+1;gets.set(req.url,count);if(count>1){res.writeHead(410,{'Content-Length':'0'});res.end();return;}}
 const interrupted=req.url==='/capacity-interrupted';
 const capacityUnknown=req.url==='/capacity-unknown'||interrupted;
 setTimeout(()=>{
  const streaming=req.url.includes('stream');
  const length=capacityUnknown&&req.method==='HEAD'?'4001':streaming?String(streamChunk*2):'4097';
  res.writeHead(200,{'Content-Length':length,'Content-Type':'application/zip',...(req.url==='/missing'?{}:{'Content-Disposition':'attachment; filename=server.zip'})});
  if(req.method!=='GET')res.end();
  else if(interrupted){
   res.write(Buffer.alloc(1024,1));
   setTimeout(()=>res.destroy(),150);
  }
  else if(streaming){
   // Let the native reader commit a full buffer before withholding the tail.
   res.write(Buffer.alloc(streamChunk,1));
   setTimeout(()=>res.end(Buffer.alloc(streamChunk,1)),3000);
  }else res.end(Buffer.alloc(4097,1));
 },capacityUnknown&&req.method==='GET'?1500:400);
});
server.listen(0,'127.0.0.1',()=>{
 const child=spawn(path.resolve(exe),[path.resolve(output),`http://127.0.0.1:${server.address().port}`,...(crossFolder?[path.resolve(crossFolder)]:[])],{stdio:'inherit'});
 const timer=setTimeout(()=>{child.kill();},150000);
 child.on('error',error=>{console.error(error);clearTimeout(timer);server.close();process.exitCode=1;});
 child.on('exit',code=>{
  clearTimeout(timer);server.close();
  const file=path.join(output,'results.json');
  const result=fs.existsSync(file)?JSON.parse(fs.readFileSync(file,'utf8').replace(/^\uFEFF/,'')):{passed:false,error:'No native result'};
  result.httpRequests=requests;
  const previewRequests=requests.filter(r=>r.path!=='/prefetch'&&!r.path.startsWith('/zip-prefetch-once')&&!retryStatuses.has(r.path)&&!r.path.startsWith('/refresh-expired-')&&r.path!=='/unknown-length-browser'&&!r.path.startsWith('/prefixed-audio')&&!r.path.startsWith('/start-')&&!r.path.startsWith('/later-')&&!r.path.startsWith('/capacity-')&&!r.path.startsWith('/refresh-size-')&&!r.path.startsWith('/durable-')&&!r.path.startsWith('/browser-dismiss-')&&!r.path.startsWith('/schedule-limits-')&&!r.path.startsWith('/browser-metadata-'));
  const countRequests=url=>requests.filter(r=>r.path===url).length;
  result.headOnly=previewRequests.length>=6&&previewRequests.every(r=>r.method==='HEAD');
  result.prefetchUsedGet=requests.some(r=>r.path==='/prefetch'&&r.method==='GET');
  result.singleUseStarts=['/later-complete','/later-checksum','/start-stream','/start-complete','/start-move-stream',...(crossFolder?['/start-cross-stream']:[])]
   .every(url=>countRequests(url)===1&&gets.get(url)===1);
  // An invalid saved body requires a fresh GET. The origin rejects it with 410.
  result.invalidPrefetchRefetch=['/later-damaged','/later-missing']
   .every(url=>countRequests(url)===2&&gets.get(url)===2);
  const methods=url=>requests.filter(r=>r.path===url).map(r=>r.method).join(',');
  result.prefetchedZipSingleGet=['','-stream'].every(suffix=>{
   const savedZip=path.join(output,'zip-prefetch-files'+suffix,'checked.zip');
   return methods('/zip-prefetch-once'+suffix)==='GET'&&fs.existsSync(savedZip)&&fs.readFileSync(savedZip).equals(previewZip);
  });
  result.capacityRequests=methods('/capacity-reported')==='GET'&&methods('/capacity-unknown')==='HEAD,GET'&&
   methods('/capacity-interrupted')==='HEAD,GET'&&methods('/capacity-short-wait')==='GET'&&
   methods('/capacity-cancel-wait')==='';
  result.refreshSizeRequests=methods('/refresh-size-manual-start')==='GET'&&methods('/refresh-size-prefetch')==='GET'&&
   !requests.some(r=>r.path.startsWith('/old-refresh-size-'));
  result.unknownLengthSingleGet=methods('/unknown-length-browser')==='GET';
  result.browserMetadataRequests=methods('/browser-metadata-cancel')===''&&
   methods('/browser-metadata-start')==='GET'&&methods('/browser-metadata-manual')==='HEAD'&&
   methods('/browser-metadata-size-only')==='HEAD';
  result.fileInfoExplicitRetry=[...retryStatuses.keys()].every(url=>methods(url)==='GET,GET');
  result.expiredAddressRequests=refreshExpiredRequests;
  result.expiredAddressSingleGet=methods('/refresh-expired-original')==='GET'&&methods('/refresh-expired-replacement')==='GET'&&
   refreshExpiredRequests[0].authorization&&!refreshExpiredRequests[1].authorization&&!refreshExpiredRequests[1].range;
  result.expiredAddressCapacity=methods('/refresh-expired-replacement-capacity-short')==='GET'&&
   methods('/refresh-expired-replacement-capacity-long')==='GET'&&
   methods('/refresh-expired-replacement-chunked')==='GET'&&
   refreshExpiredRequests.some(r=>r.path==='/refresh-expired-replacement-chunked'&&!r.range&&!r.authorization)&&
   ['short','long'].every(wait=>methods('/refresh-expired-original-capacity-'+wait)==='GET'&&
    refreshExpiredRequests.filter(r=>r.path==='/refresh-expired-replacement-capacity-'+wait).every(r=>!r.authorization&&!r.range));
  result.partialRefreshSingleRange=['/refresh-expired-replacement-partial','/refresh-expired-changed-tag','/refresh-expired-changed-overlap'].every(url=>{
   const matching=refreshExpiredRequests.filter(request=>request.path===url);
   return matching.length===1&&matching[0].method==='GET'&&matching[0].range==='bytes=262080-524287'&&
    matching[0].ifRange==='"refresh-fixture"'&&!matching[0].authorization;
  });
  result.dateRefreshSingleRange=['/refresh-expired-replacement-date','/refresh-expired-changed-date','/refresh-expired-weak-date'].every(url=>{
   const matching=refreshExpiredRequests.filter(request=>request.path===url);
   return matching.length===1&&matching[0].method==='GET'&&matching[0].range==='bytes=262080-524287'&&
    matching[0].ifRange===''&&!matching[0].authorization;
  });
  const legacyDateRequests=refreshExpiredRequests.filter(request=>request.path==='/refresh-expired-replacement-date-legacy');
  result.legacyDateProbed=legacyDateRequests.length===2&&legacyDateRequests[0].range==='bytes=0-0'&&
   legacyDateRequests[1].range==='bytes=262080-524287'&&legacyDateRequests.every(request=>!request.ifRange&&!request.authorization);
  result.prefixedAudioSingleGet=['','-unknown','-refreshed','-unknown-refreshed']
   .every(suffix=>methods('/prefixed-audio'+suffix)==='GET');
  result.durableOfferSingleGet=methods('/durable-offer')==='GET'&&methods('/durable-changed')==='GET';
  result.browserDismissalSingleGet=['button','escape','close-stream','existing-escape','existing-close']
   .every(mode=>methods('/browser-dismiss-'+mode)==='GET');
  result.scheduledLimitsRequests=methods('/schedule-limits-cold')==='HEAD,GET'&&methods('/schedule-limits-prefetch')==='GET';
  fs.writeFileSync(file,JSON.stringify(result,null,2)+'\n');
  console.log(JSON.stringify({exitCode:code,passed:result.passed,checks:result.checks?.length,headOnly:result.headOnly,singleUseStarts:result.singleUseStarts,invalidPrefetchRefetch:result.invalidPrefetchRefetch,capacityRequests:result.capacityRequests,browserMetadataRequests:result.browserMetadataRequests,refreshSizeRequests:result.refreshSizeRequests,prefixedAudioSingleGet:result.prefixedAudioSingleGet,unknownLengthSingleGet:result.unknownLengthSingleGet,fileInfoExplicitRetry:result.fileInfoExplicitRetry,expiredAddressSingleGet:result.expiredAddressSingleGet,expiredAddressCapacity:result.expiredAddressCapacity,partialRefreshSingleRange:result.partialRefreshSingleRange,durableOfferSingleGet:result.durableOfferSingleGet,error:result.error}));
  process.exitCode=code===0&&result.passed&&result.headOnly&&result.prefetchUsedGet&&result.prefetchedZipSingleGet&&result.singleUseStarts&&result.invalidPrefetchRefetch&&result.capacityRequests&&result.browserMetadataRequests&&result.refreshSizeRequests&&result.prefixedAudioSingleGet&&result.unknownLengthSingleGet&&result.fileInfoExplicitRetry&&result.expiredAddressSingleGet&&result.expiredAddressCapacity&&result.partialRefreshSingleRange&&result.dateRefreshSingleRange&&result.legacyDateProbed&&result.durableOfferSingleGet&&result.browserDismissalSingleGet&&result.scheduledLimitsRequests?0:1;
 });
});
