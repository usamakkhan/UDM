'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
module.exports=async function({page,browser,worker,clickAX,firefox=false,background,command,firefoxPanelElement,firefoxClick,driverRequest,driverSession,until,jobs,pass,base,root,probe,run,requests}){
 const hello=firefox?await background({action:'hello'}):await worker.evaluate(()=>nativeRequest({action:'hello'}));
 assert(hello.capabilities.includes('live-hls-subtitles'));pass('Native host advertises live subtitle recording');
 const before=new Set(jobs().map(j=>j.Id));let session;
 if(firefox){
  await command('/url',{url:base+'/live-subtitles'});
  await until(()=>command('/execute/sync',{script:'return document.querySelector("video")?.videoHeight===360 && document.body.dataset.ready==="yes" && !!document.querySelector("video")?.getAttribute("data-udm-player")',args:[]}));
  await firefoxClick(await firefoxPanelElement('button','Download this video with UDM'));
  await firefoxClick(await firefoxPanelElement('button','Audio/subtitle options for '));
  const select=await firefoxPanelElement('select','Subtitles');
  const options=await command('/element/'+select['element-6066-11e4-a52e-4f735466cecf']+'/elements',{using:'css selector',value:'option'});
  let selected;for(const option of options)if(await command('/execute/sync',{script:'return arguments[0].textContent.includes("Spanish captions");',args:[option]}))selected=option;
  assert(selected);await firefoxClick(selected);
  assert(await command('/execute/sync',{script:'return arguments[0].selectedOptions[0].textContent.includes("Spanish captions");',args:[select]}));
  fs.writeFileSync(path.join(root,'live-subtitle-choice.png'),Buffer.from(await driverRequest('GET','/session/'+driverSession+'/screenshot'),'base64'));
 }else{
  await page.goto(base+'/live-subtitles');await page.waitForFunction(()=>document.querySelector('video')?.videoHeight===360&&document.body.dataset.ready==='yes');await until(()=>page.locator('[id^="udm-video-panel-"]').count());session=await browser.newCDPSession(page);
  await clickAX(session,'button',name=>name==='Download this video with UDM');await clickAX(session,'button',name=>name.startsWith('Audio/subtitle options for ')&&name.includes('MP4')&&name.includes('360p'));
  await clickAX(session,'combobox',name=>name==='Subtitles');await page.keyboard.press('End');await page.keyboard.press('Enter');const tree=await session.send('Accessibility.getFullAXTree');assert.match(tree.nodes.find(n=>!n.ignored&&n.role?.value==='combobox'&&n.name?.value==='Subtitles').value.value,/Spanish captions/);
  await page.screenshot({path:path.join(root,'live-subtitle-choice.png')});
 }
 pass((firefox?'Firefox':'Edge')+' panel offers and selects the live subtitle language');
 if(firefox)await firefoxClick(await firefoxPanelElement('button','Download video'));else await clickAX(session,'button',name=>name==='Download video');
 const job=await until(()=>{const j=jobs().find(j=>!before.has(j.Id));if(j?.Status==='Failed')throw Error(j.Error);return j?.Status==='Complete'&&j;},60000);
 assert.equal(job.LiveRecording,true);const output=path.join(job.Folder,job.FileName),metadata=probe(output),captions=metadata.streams.filter(s=>s.codec_type==='subtitle');
 assert.equal(captions.length,1);assert.equal(captions[0].codec_name,'mov_text');assert.equal(captions[0].tags.language,'spa');assert.equal(captions[0].tags.handler_name,'Spanish captions');assert(Math.abs(Number(metadata.format.duration)-8)<.15);
 assert.equal(metadata.streams.filter(s=>s.codec_type==='video').length,1);assert.equal(metadata.streams.filter(s=>s.codec_type==='audio').length,1);
 const text=run(['-i',output,'-map','0:s:0','-f','webvtt','pipe:1']).toString();fs.writeFileSync(path.join(root,'live-captions.vtt'),text);for(let i=0;i<4;i++)assert(text.includes('Caption '+i));assert(/00:00\.(9[0-9][0-9]) --> 00:01\./.test(text)||text.includes('00:01.000 --> 00:01.700'));
 run(['-v','error','-xerror','-i',output,'-f','null','-']);pass('Live MP4 preserves all caption text, synchronized timing, language and track name',{duration:Number(metadata.format.duration),sha256:job.Sha256});
 const polls=requests.filter(r=>r.type==='live-playlist');assert(polls.some(r=>!r.ended)&&polls.some(r=>r.ended));assert.equal(new Set(requests.filter(r=>r.type==='live-subtitle').map(r=>r.name)).size,4);assert(job.LiveCapturedSeconds>=7.9);pass('Growing live caption playlist is recorded through ENDLIST');if(session)await session.detach();
};
