'use strict';
const {chromium}=require('playwright');
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const root=path.resolve(process.argv[2]),checks=[];assert(!fs.existsSync(root),'Use a fresh result directory');fs.mkdirSync(root,{recursive:true});let browser;
(async()=>{
 browser=await chromium.launch({channel:'msedge',headless:true});
 for(const family of ['chromium','firefox'])for(const scenario of ['retain','audio-removed','subtitle-removed','restore','choose-new','implicit-audio','format-return','audio-permission']){
  const page=await browser.newPage({viewport:{width:1200,height:800}}),cdp=await page.context().newCDPSession(page);
  try{
   await page.route('https://panel-tracks.test/',route=>route.fulfill({contentType:'text/html',body:'<!doctype html><video style="position:absolute;left:50px;top:100px;width:800px;height:450px;background:#456"></video>'}));await page.goto('https://panel-tracks.test/');
   await page.evaluate(()=>{
    globalThis.offer={key:'video',label:'720p fixture',height:720,container:'mp4',audioOnlyAvailable:true,audioOptions:[{key:'en',label:'English',default:true},{key:'es',label:'Spanish'}],subtitleOptions:[{key:'sub-en',label:'English captions'},{key:'sub-es',label:'Spanish captions'}]};
    globalThis.handoffs=[];let lookup,progress,sequence=0;
    globalThis.chrome={storage:{local:{get:async()=>({}),set:async()=>{}}},runtime:{id:'udm-fixture',onMessage:{addListener(callback){progress=callback;}},sendMessage:message=>{
     if(message.action==='site-formats'){lookup=message;return new Promise(()=>{});}
     if(message.action==='site-download')handoffs.push(message);
     return Promise.resolve({ok:true});
    }}};
    globalThis.update=()=>progress({action:'site-formats-progress',token:lookup.token,progressId:lookup.progressId,sequence:++sequence,result:{ok:true,moreFormats:true,choices:[offer]}},{id:'udm-fixture'});
    globalThis.lookupReady=()=>!!lookup;
   });
   for(const name of ['media.js','content.js'])await page.addScriptTag({path:path.join(__dirname,'../browser',family,name)});
   async function node(name){for(let i=0;i<60;++i){const tree=await cdp.send('Accessibility.getFullAXTree'),item=tree.nodes.find(n=>!n.ignored&&n.name?.value===name&&n.backendDOMNodeId);if(item)return item;await page.waitForTimeout(20);}throw Error('Missing '+name);}
   async function focus(name){await cdp.send('DOM.focus',{backendNodeId:(await node(name)).backendDOMNodeId});}
   async function activate(name){await focus(name);await page.keyboard.press('Enter');}
   async function state(name){const object=await cdp.send('DOM.resolveNode',{backendNodeId:(await node(name)).backendDOMNodeId});try{return (await cdp.send('Runtime.callFunctionOn',{objectId:object.object.objectId,functionDeclaration:'function(){return {value:this.value,disabled:this.disabled,options:Array.from(this.options||[],o=>({value:o.value,text:o.text,disabled:o.disabled}))}}',returnByValue:true})).result.value;}finally{await cdp.send('Runtime.releaseObject',{objectId:object.object.objectId});}}
   await activate('Download this video with UDM');await page.waitForFunction(()=>lookupReady());await page.evaluate(()=>update());await activate('Audio/subtitle options for 720p fixture');
   await focus('Audio track');await page.keyboard.press('End');await focus('Subtitles');await page.keyboard.press('End');
   assert.equal((await state('Audio track')).value,'es');assert.equal((await state('Subtitles')).value,'sub-es');
   if(scenario==='retain'){
    await page.evaluate(()=>{offer={...offer,label:'720p updated',audioOptions:[{key:'fr',label:'French',default:true},{key:'es',label:'Spanish updated'},{key:'en',label:'English'}],subtitleOptions:[{key:'sub-fr',label:'French captions'},{key:'sub-es',label:'Spanish captions updated'}]};update();});
    const audio=await state('Audio track'),subtitles=await state('Subtitles');assert.equal(audio.value,'es');assert.equal(subtitles.value,'sub-es');assert(audio.options.some(o=>o.value==='fr'));assert(audio.options.some(o=>o.text==='Spanish updated'));assert(subtitles.options.some(o=>o.text==='Spanish captions updated'));assert(!(await state('Download video')).disabled);await page.screenshot({path:path.join(root,family+'-updated-chooser.png')});
   }else if(scenario==='audio-removed'){
    await focus('Download video');await page.evaluate(()=>{offer={...offer,audioOptions:[{key:'en',label:'English'}]};update();});
    assert((await state('Download video')).disabled);assert((await state('Download audio (M4A)')).disabled);assert.equal((await state('Audio track')).value,'es');assert.deepEqual(await page.evaluate(()=>handoffs),[]);
   }else if(scenario==='subtitle-removed'){
    await page.evaluate(()=>{offer={...offer,subtitleOptions:[{key:'sub-en',label:'English captions'}]};update();});
    assert((await state('Download video')).disabled);assert(!(await state('Download audio (M4A)')).disabled);
    await focus('Subtitles');await page.keyboard.press('Home');assert(!(await state('Download video')).disabled);await activate('Download video');await page.waitForFunction(()=>handoffs.length===1);const sent=await page.evaluate(()=>handoffs[0]);assert.equal(sent.audioKey,'es');assert.equal(sent.subtitleKey,'');
   }else if(scenario==='restore'){
    await page.evaluate(()=>{globalThis.original=offer;offer={...offer,audioOptions:[{key:'en',label:'English'}]};update();});assert((await state('Download video')).disabled);
    await page.evaluate(()=>{offer=original;update();});assert(!(await state('Download video')).disabled);assert.equal((await state('Audio track')).value,'es');assert.equal((await state('Subtitles')).value,'sub-es');
   }else if(scenario==='implicit-audio'){
    await page.evaluate(()=>{offer={...offer,audioOptions:[]};update();});assert((await state('Download video')).disabled);
    await focus('Audio track');await page.keyboard.press('Home');assert(!(await state('Download video')).disabled);await activate('Download video');await page.waitForFunction(()=>handoffs.length===1);assert.equal((await page.evaluate(()=>handoffs[0])).audioKey,undefined);
   }else if(scenario==='format-return'){
    await page.evaluate(()=>{globalThis.original=offer;offer={...offer,key:'replacement'};update();});assert((await state('Download video')).disabled);
    await page.evaluate(()=>{offer={...original,audioOptions:[...original.audioOptions,{key:'fr',label:'French'}]};update();});assert(!(await state('Download video')).disabled);assert.equal((await state('Audio track')).value,'es');assert((await state('Audio track')).options.some(o=>o.value==='fr'));
   }else if(scenario==='audio-permission'){
    await page.evaluate(()=>{offer={...offer,audioOnlyAvailable:false};update();});assert(!(await state('Download video')).disabled);assert(!(await cdp.send('Accessibility.getFullAXTree')).nodes.some(n=>!n.ignored&&n.name?.value==='Download audio (M4A)'));assert.deepEqual(await page.evaluate(()=>handoffs),[]);
   }else{
    await page.evaluate(()=>{offer={...offer,audioOptions:[{key:'fr',label:'French',default:true}]};update();});assert((await state('Download video')).disabled);
    await focus('Audio track');await page.keyboard.press('End');assert.equal((await state('Audio track')).value,'fr');assert(!(await state('Download video')).disabled);await activate('Download video');await page.waitForFunction(()=>handoffs.length===1);const sent=await page.evaluate(()=>handoffs[0]);assert.equal(sent.audioKey,'fr');assert.equal(sent.subtitleKey,'sub-es');assert.equal(sent.key,'video');
   }
   checks.push({name:family+' '+scenario,passed:true});
  }catch(error){checks.push({name:family+' '+scenario,passed:false,error:error.stack});}finally{await page.close();}console.log(JSON.stringify(checks.at(-1)));
 }
})().catch(error=>checks.push({name:'fixture',passed:false,error:error.stack})).finally(async()=>{await browser?.close();const failed=checks.filter(c=>!c.passed).length;fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({passed:!failed,passedChecks:checks.length-failed,failedChecks:failed,checks,scope:'Owned headless Edge pages; both bundle sources; trusted keyboard input and actual outgoing extension message payloads. No personal browser, desktop handoff or video transfer.'},null,2));if(failed)process.exitCode=1;console.log(JSON.stringify({passed:checks.length-failed,failed}));});
