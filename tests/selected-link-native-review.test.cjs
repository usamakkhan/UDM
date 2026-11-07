'use strict';
const assert=require('node:assert/strict'),{harness}=require('./browser.test.cjs');
const page='https://page.test/articles',file='https://files.test/archive.zip';
const framePage='https://frame.test/gallery',picture='https://frame.test/photo.jpg';
const sender={id:'test-extension',url:page,tab:{id:7,url:page},frameId:2,documentId:'source-document'};
const selection={action:'selected-links-review',page,links:[{url:file+'#part',title:'Archive'},{url:file,title:'Repeated archive'}]};

async function fixture(bundle,{capability=2,behavior='accept',method='GET',legacyFrames=false,observed=null,cookieSnapshot=null,unobserved=false}={}){
  const requests=[],tabs=[],validations=[],collections=[],collectionTargets=[];
  const reads={settings:0,permissions:0,tabs:0,preferences:0,proxy:0,proxySnapshots:0,proxyVerification:0,sessionVerification:0};
  let permitted=cookieSnapshot!==null,validatingDispatch=false;
  const app=await harness({cookies:cookieSnapshot!==null},false,{},null,null,bundle,['browser-controls.js'],{
    crypto:require('node:crypto'),structuredClone,TextEncoder,
    UdmRequestContext:{create:()=>({
      install(){},
      resolve(item,credentials){
        if(unobserved)return null;
        return {method,tabId:item.tabId,url:item.url,headers:{Accept:'application/zip',...(credentials?{Cookie:'private=fixture'}:{})},...observed};
      }
    })},
    UdmChromiumProxy:unobserved||behavior==='proxy-changed-before-send'?{create:()=>({
      install(){},
      beginBatch(){
        reads.proxySnapshots++;
        return {
          async resolve(url){
            reads.proxy++;
            if(behavior==='proxy-changed-during-prep')throw Object.assign(Error('The browser proxy settings changed.'),{batchProxyChanged:true});
            return {url,type:'direct'};
          },
          verify(){
            reads.proxyVerification++;
            if(behavior==='proxy-changed-before-send'&&validatingDispatch)throw Error('The browser proxy settings changed.');
          }
        };
      }
    })}:undefined,
    UdmDownloadSession:{create:()=>({
      beginBatch:()=>({
        capture:async()=>{
          if(behavior==='session-source-changed')throw Object.assign(Error('The download page changed.'),{batchSourceChanged:true});
          return cookieSnapshot===null?null:{cookies:cookieSnapshot};
        },
        verify:async()=>{reads.sessionVerification++;}
      })
    })},
    UdmNativeBridge:{create:()=>({request:async(message,timeout,beforeSend,validate)=>{
      if(message.action==='preferences'){reads.preferences++;return {ok:true,browserBatchReview:capability,browserProxy:1,postDownloads:true,stagedPostBodyLimit:4096};}
      if(behavior==='revoked')permitted=false;
      validatingDispatch=true;
      await validate?.();beforeSend?.();requests.push(message);
      if(behavior==='lost')throw Error('Connection closed after sending');
      return behavior==='reject'?{ok:false,error:'Finish the pending browser batch reviews before opening another.'}:{ok:true,count:message.links.length};
    }})}
  });
  const storageGet=app.api.storage.local.get;
  app.api.storage.local.get=async key=>{if(key==='settings')reads.settings++;return storageGet(key);};
  app.api.permissions.contains=async()=>{reads.permissions++;return permitted;};
  app.api.tabs.get=async id=>{
    reads.tabs++;
    return {id,url:page,incognito:false,cookieStoreId:behavior==='changed-store'&&reads.tabs>1?'other-store':'fixture-store'};
  };
  app.api.tabs.create=async tab=>tabs.push(tab.url);
  app.api.runtime.getURL=name=>'extension://fixture/'+name;
  app.api.scripting.executeScript=async({target,args})=>{
    if(args){
      collections.push(args[0]);
      collectionTargets.push(JSON.parse(JSON.stringify(target)));
      const frames=[
        {frameId:0,documentId:legacyFrames?undefined:'top-document',result:[{url:file+'?top',name:'Top selection',linkText:'Top selection',page}]},
        {frameId:2,documentId:legacyFrames?undefined:'source-document',result:[{url:file,name:'Archive',linkText:'Archive',page}]},
        {frameId:5,documentId:legacyFrames?undefined:'frame-document',result:[{url:picture,name:'Photo',imageOnPage:true,page:framePage}]}
      ];
      if(target.allFrames)return frames.slice(1);
      return frames.filter(frame=>target.frameIds.includes(frame.frameId)).map(frame=>({
        ...frame,result:args[1]&&args[1]!==frame.result[0].page?null:frame.result
      }));
    }
    validations.push(JSON.parse(JSON.stringify(target)));
    return [
      {frameId:0,documentId:'top-document',result:page},
      {frameId:2,documentId:behavior==='stale'?'other-document':'source-document',result:page},
      {frameId:5,documentId:'frame-document',result:behavior==='stale-frame'?framePage+'/new':framePage}
    ];
  };
  const send=(message=selection,source=sender)=>new Promise(resolve=>app.events.message(message,source,resolve));
  async function click(menuItemId,info={}){
    app.events.context({menuItemId,...info},sender.tab);
    for(let i=0;i<40;i++){
      if(requests.length||tabs.length||app.store.lastError)return app.store.lastError;
      await new Promise(resolve=>setImmediate(resolve));
    }
    throw Error('Menu action did not finish');
  }
  return {send,click,requests,tabs,validations,collections,collectionTargets,reads,session:app.session};
}

(async()=>{
  for(const bundle of ['chromium','firefox']){
    let f=await fixture(bundle);
    assert.equal((await f.send()).count,1);
    assert.deepEqual(f.validations,[{tabId:7,documentIds:['source-document']}]);
    assert.equal(f.requests.length,1);
    assert.equal(f.requests[0].action,'browser-batch-review');
    assert.deepEqual(JSON.parse(JSON.stringify(f.requests[0].links)),[
      {url:file,headers:{Accept:'application/zip'},cookies:'',referrer:page,userAgent:'test',page,linkText:'Archive'}
    ]);
    assert.equal(f.tabs.length,0);
    console.log('PASS '+bundle+' sends a single native review with request context and no download or browser tab');

    const observed={reportedSize:42048,response:{mime:'application/zip',filename:'server.zip',ambiguous:false}};
    f=await fixture(bundle,{observed});
    assert.equal((await f.send()).ok,true);
    assert.deepEqual(JSON.parse(JSON.stringify(f.requests[0].links[0].responsePreview)),{size:42048,contentType:'application/zip',serverName:'server.zip'});
    console.log('PASS '+bundle+' forwards complete response details for an observed GET to native review');

    for(const response of [
      {...observed,response:{...observed.response,ambiguous:true}},
      {...observed,reportedSize:0},
      {...observed,response:{...observed.response,mime:''}},
      {...observed,pending:true},
      {...observed,url:file+'?different=1'},
      {...observed,requiresCredentials:true},
    ]){
      f=await fixture(bundle,{observed:response});
      assert.equal((await f.send()).ok,true);
      assert.equal(f.requests[0].links[0].responsePreview,undefined);
    }
    console.log('PASS '+bundle+' leaves incomplete, ambiguous, pending, mismatched, and credential-bound details for native lookup');

    f=await fixture(bundle,{observed:{...observed,requiresCredentials:true,headers:{Cookie:'account=old'}},cookieSnapshot:'account=current'});
    assert.equal((await f.send()).ok,true);
    assert.equal(f.requests[0].links[0].cookies,'account=current');
    assert.equal(f.requests[0].links[0].responsePreview,undefined);
    console.log('PASS '+bundle+' leaves response details for lookup when the captured cookies changed');

    const largeSelection={...selection,links:Array.from({length:100},(_,i)=>({url:file+'?part='+i,title:'Archive '+i}))};
    f=await fixture(bundle,{unobserved:true});
    assert.equal((await f.send(largeSelection)).count,100);
    assert.deepEqual(f.reads,{settings:1,permissions:0,tabs:2,preferences:1,proxy:100,proxySnapshots:1,proxyVerification:2,sessionVerification:0});
    assert(f.requests[0].links.every(link=>link.browserProxy.url===link.url));
    console.log('PASS '+bundle+' shares batch settings and tab reads while keeping each of 100 proxy routes bound to its URL');

    for(const behavior of ['proxy-changed-during-prep','proxy-changed-before-send']){
      f=await fixture(bundle,{behavior,unobserved:behavior==='proxy-changed-during-prep'});
      assert.match((await f.send(largeSelection)).error,/proxy settings changed/);
      assert.equal(f.requests.length+f.tabs.length,0);
      console.log('PASS '+bundle+' stops batch review when proxy settings change '+behavior.split('changed-')[1]);
    }

    f=await fixture(bundle,{cookieSnapshot:'account=current'});
    assert.equal((await f.send(largeSelection)).count,100);
    assert.deepEqual(f.reads,{settings:2,permissions:2,tabs:2,preferences:1,proxy:0,proxySnapshots:0,proxyVerification:0,sessionVerification:1});
    console.log('PASS '+bundle+' revalidates the shared session once before sending 100 links');

    f=await fixture(bundle,{behavior:'revoked',cookieSnapshot:'account=current'});
    assert.match((await f.send(largeSelection)).error,/Cookie sharing changed/);
    assert.equal(f.requests.length+f.tabs.length,0);
    console.log('PASS '+bundle+' stops native review if cookie permission is revoked before dispatch');

    f=await fixture(bundle,{behavior:'changed-store'});
    assert.match((await f.send()).error,/source page changed/);
    assert.equal(f.requests.length+f.tabs.length,0);
    console.log('PASS '+bundle+' stops native review if the source tab changed cookie stores');

    f=await fixture(bundle,{capability:0});
    assert.equal((await f.send()).ok,true);
    assert.equal(f.requests.length,0);assert.match(f.tabs[0],/\/links\.html#/);
    assert.equal(f.session.linkBatches[0][1].presetSelection,true);
    console.log('PASS '+bundle+' falls back to checked browser review with an older desktop');

    f=await fixture(bundle,{method:'POST'});
    assert.equal((await f.send()).ok,true);
    assert.equal(f.requests.length,0);assert.equal(f.tabs.length,1);
    console.log('PASS '+bundle+' keeps an observed POST on the existing browser review path');

    f=await fixture(bundle,{method:'POST',observed:{request:{bytes:Uint8Array.of(1,2,3),contentType:'application/octet-stream'}}});
    assert.equal((await f.send()).ok,true);
    assert.equal(f.requests.length,0);assert.equal(f.tabs.length,1);
    console.log('PASS '+bundle+' opens browser review without staging an observed form body');

    f=await fixture(bundle,{behavior:'session-source-changed',cookieSnapshot:'account=current'});
    assert.match((await f.send()).error,/page changed/);
    assert.equal(f.requests.length+f.tabs.length,0);
    console.log('PASS '+bundle+' does not reopen stale links in browser review when session scope changes');

    f=await fixture(bundle,{behavior:'stale'});
    assert.match((await f.send()).error,/source page changed/);
    assert.equal(f.requests.length+f.tabs.length,0);
    console.log('PASS '+bundle+' rejects a replaced source document before dispatch');

    for(const behavior of ['lost','reject']){
      f=await fixture(bundle,{behavior});
      assert.match((await f.send()).error,behavior==='lost'?/Check for the desktop batch dialog/:/Finish the pending browser batch reviews/);
      assert.equal(f.requests.length,1);assert.equal(f.tabs.length,0);
      console.log('PASS '+bundle+' does not open a second review after native '+behavior);
    }

    f=await fixture(bundle);
    assert.match((await f.send(selection,{...sender,tab:{...sender.tab,incognito:true}})).error,/regular web page/);
    assert.equal(f.requests.length+f.tabs.length,0);
    console.log('PASS '+bundle+' keeps private tabs out of batch review');

    f=await fixture(bundle);
    await f.click('udm-all-links');
    assert.deepEqual(f.collections,[false]);
    assert.deepEqual(f.validations,[{tabId:7,documentIds:['source-document','frame-document']}]);
    assert.deepEqual(Array.from(f.requests[0].links,link=>[link.url,link.page,!!link.imageOnPage]),[[file,page,false],[picture,framePage,true]]);
    assert.equal(f.tabs.length,0);
    console.log('PASS '+bundle+' right-click all-links sends one native batch across frames and preserves the image flag');

    f=await fixture(bundle);
    await f.click('udm-selected-links',{frameId:2,frameUrl:page});
    assert.deepEqual(f.collections,[true]);
    assert.deepEqual(f.collectionTargets,[{tabId:7,frameIds:[2]}]);
    assert.deepEqual(f.validations,[{tabId:7,documentIds:['source-document']}]);
    assert.equal(f.requests[0].links.length,1);assert.equal(f.tabs.length,0);
    console.log('PASS '+bundle+' right-click selected-links sends only selected rows to native review');

    f=await fixture(bundle);
    await f.click('udm-selected-links');
    assert.deepEqual(f.collectionTargets,[{tabId:7,frameIds:[0]}]);
    assert.equal(f.requests[0].links[0].url,file+'?top');
    console.log('PASS '+bundle+' a top-level selection does not include other frames');

    f=await fixture(bundle);
    assert.match(await f.click('udm-selected-links',{frameId:5,frameUrl:framePage+'/before-navigation'}),/source page changed/);
    assert.equal(f.requests.length+f.tabs.length,0);
    assert.equal(f.reads.preferences,0);
    console.log('PASS '+bundle+' rejects a frame that navigated before collecting the selection');

    f=await fixture(bundle,{capability:0,behavior:'stale'});
    assert.match((await f.send()).error,/source page changed/);
    assert.equal(f.requests.length+f.tabs.length,0);
    console.log('PASS '+bundle+' rejects a stale source before opening browser fallback review');

    f=await fixture(bundle,{legacyFrames:true});
    await f.click('udm-all-links');
    assert.deepEqual(f.validations,[{tabId:7,frameIds:[2,5]}]);
    assert.equal(f.requests[0].links.length,2);
    console.log('PASS '+bundle+' validates each source frame when document IDs are unavailable');

    f=await fixture(bundle,{behavior:'stale-frame'});
    assert.match(await f.click('udm-all-links'),/source page changed/);
    assert.equal(f.requests.length+f.tabs.length,0);
    console.log('PASS '+bundle+' rejects the entire native batch when a contributing frame changes');

    f=await fixture(bundle,{capability:1});
    await f.click('udm-all-links');
    assert.equal(f.requests.length,0);assert.equal(f.tabs.length,1);
    assert.equal(f.session.linkBatches[0][1].presetSelection,false);
    assert.equal(f.session.linkBatches[0][1].rows[1].imageOnPage,true);
    assert.equal((await f.send()).ok,true);assert.equal(f.requests.length,1);
    console.log('PASS '+bundle+' keeps the browser menu fallback with the previous native capability while the strip stays native');
  }
})().catch(error=>{console.error(error);process.exitCode=1;});
