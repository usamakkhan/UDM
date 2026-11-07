const {chromium}=require('playwright'),fs=require('fs'),path=require('path'),assert=require('assert/strict');
const root=process.argv[2];let browser;
(async()=>{fs.mkdirSync(root,{recursive:true});browser=await chromium.launch({channel:process.env.UDM_TEST_BROWSER||'chrome',headless:true});const page=await browser.newPage({viewport:{width:512,height:340}}),cdp=await page.context().newCDPSession(page);
await page.route('https://udm-layout.test/',r=>r.fulfill({contentType:'text/html',body:'<!doctype html><body style="margin:0"><video style="position:absolute;left:8px;top:40px;width:496px;height:280px;background:#243b54"></video>'}));await page.goto('https://udm-layout.test/');
await page.evaluate(()=>{globalThis.sent=[];globalThis.failAt='';globalThis.holdHandoff=false;globalThis.chrome={storage:{local:{get:async()=>({}),set:async()=>{}}},runtime:{id:'udm-fixture',onMessage:{addListener(fn){globalThis.deliverProgress=fn;}},sendMessage:async m=>{if(m.action==='site-download'){sent.push(m.key);if(holdHandoff)return new Promise(resolve=>globalThis.finishHandoff=resolve);await new Promise(r=>setTimeout(r,30));return m.key===failAt?{ok:false,error:'Fixture handoff failed'}:{ok:true};}return {ok:true,choices:[1080,720,480,288].flatMap(h=>['mp4','ts'].map(container=>({key:h+'-'+container,height:h,container,label:'Dailymotion · '+container.toUpperCase()+' · '+h+'p'+(h>=720?' HD':'')+' · '+({1080:6075,720:2098,480:816,288:449}[h])+' kbps'})))}}}};});
for(const file of ['media.js','content.js'])await page.addScriptTag({path:path.resolve('browser',process.env.UDM_TEST_BUNDLE||'chromium',file)});
async function ax(){return (await cdp.send('Accessibility.getFullAXTree')).nodes.filter(n=>!n.ignored);}
async function node(name){for(let i=0;i<40;i++){const n=(await ax()).find(n=>n.role?.value==='button'&&n.name?.value===name);if(n)return n;await page.waitForTimeout(50);}throw Error('Missing '+name);}
async function box(name){const n=await node(name);const q=(await cdp.send('DOM.getBoxModel',{backendNodeId:n.backendDOMNodeId})).model.border;return {x:q[0],y:q[1],right:q[2],bottom:q[5]};}
async function click(name){const b=await box(name);await page.mouse.click((b.x+b.right)/2,(b.y+b.bottom)/2);await page.waitForTimeout(100);}
await click('Download this video with UDM');
assert(!(await ax()).some(n=>n.role?.value==='button'&&n.name?.value==='Reset position'));
await page.screenshot({path:path.join(root,'classic-video-menu.png')});
await click('Video panel options');
for(const name of ['Refresh','Reset position','Hide here','Download all']){const b=await box(name);assert(b.x>=0&&b.y>=0&&b.right<=512&&b.bottom<=340,name+' clipped');}
await page.screenshot({path:path.join(root,'compact-eight-formats.png')});await page.emulateMedia({colorScheme:'dark'});await page.screenshot({path:path.join(root,'compact-eight-formats-dark.png')});console.log('PASS eight-format menu keeps footer controls inside a small video viewport');
await click('Download all');await page.waitForTimeout(400);assert.equal(await page.evaluate(()=>sent.length),8);assert.equal(new Set(await page.evaluate(()=>sent)).size,8);console.log('PASS Download all sends each of eight selected format offers once');
await page.evaluate(()=>{sent=[];failAt='720-mp4';});await click('Download all');await page.waitForTimeout(300);assert.deepEqual(await page.evaluate(()=>sent),['1080-mp4','1080-ts','720-mp4']);assert((await ax()).some(n=>n.name?.value==='Added 2. Fixture handoff failed'));console.log('PASS partial bulk failure stops and reports how many formats were accepted');
await click('Refresh');assert((await ax()).some(n=>n.role?.value==='button'&&n.name?.value==='Dailymotion · MP4 · 1080p HD · 6075 kbps'));assert(!(await ax()).some(n=>n.name?.value==='Added 2. Fixture handoff failed'));console.log('PASS controls recover after a failed bulk handoff');
await page.screenshot({path:path.join(root,'compact-menu-after-refresh.png')});
await page.evaluate(()=>{sent=[];failAt='';});
await click('Dailymotion · MP4 · 1080p HD · 6075 kbps');
assert.deepEqual(await page.evaluate(()=>sent),['1080-mp4']);
assert(!(await ax()).some(n=>n.name?.value==='Download all'||/Preparing the selected video|Review Download File Info/.test(n.name?.value||'')));
console.log('PASS a single format submits once and leaves the menu closed after acknowledgement');
await page.evaluate(()=>{sent=[];holdHandoff=true;});
await click('Download this video with UDM');await click('Dailymotion · MP4 · 1080p HD · 6075 kbps');
await page.mouse.click(20,20);await page.evaluate(()=>finishHandoff({ok:false,error:'The desktop did not acknowledge this selection.'}));
await page.waitForTimeout(100);assert(!(await ax()).some(n=>n.name?.value==='Download all'));
await click('Download this video with UDM');assert((await ax()).some(n=>n.name?.value==='The desktop did not acknowledge this selection.'));
assert.deepEqual(await page.evaluate(()=>sent),['1080-mp4']);
console.log('PASS an outside click prevents late failure from reopening the menu and keeps its warning for manual review');
await click('Close download menu');
await page.evaluate(()=>{
 const direct={key:'direct',container:'mp4',label:'Direct MP4 fixture'},stream={key:'stream',height:720,container:'ts',label:'Stream TS fixture'};
 sent=[];
 chrome.runtime.sendMessage=async message=>{
  if(message.action==='site-download'){sent.push(message.key);return {ok:true};}
  if(message.action!=='site-formats')return {ok:true};
  if(message.directFirst)return {ok:true,moreFormats:true,choices:[direct]};
  return new Promise(resolve=>globalThis.finishFormats=success=>resolve(success?{ok:true,choices:[stream,direct]}:{ok:false,error:'Playlist unavailable'}));
 };
});
await click('Download this video with UDM');
const early=await box('Direct MP4 fixture');
assert(!(await ax()).some(n=>n.name?.value==='Download all'||n.name?.value==='Stream TS fixture'));
await page.mouse.move((early.x+early.right)/2,(early.y+early.bottom)/2);
await page.mouse.down();
await page.evaluate(()=>finishFormats(true));await page.waitForTimeout(100);
assert(!(await ax()).some(n=>n.name?.value==='Stream TS fixture'));
await page.mouse.up();await page.waitForTimeout(100);
assert.deepEqual(await page.evaluate(()=>sent),['direct']);
assert(!(await ax()).some(n=>n.name?.value==='Stream TS fixture'));
console.log('PASS stream discovery does not move a direct row under a pressed pointer');
await click('Download this video with UDM');await node('Direct MP4 fixture');
await page.keyboard.press('ArrowDown');
await page.evaluate(()=>finishFormats(true));await node('Stream TS fixture');
assert((await node('Direct MP4 fixture')).properties?.some(property=>property.name==='focused'&&property.value?.value===true));
await click('Close download menu');
console.log('PASS a late stream catalog retains keyboard focus on the direct row');
await click('Download this video with UDM');await node('Direct MP4 fixture');
await page.evaluate(()=>finishFormats(false));await page.waitForTimeout(100);
assert((await ax()).some(n=>n.name?.value==='Additional formats could not be read. Playlist unavailable'));
await click('Direct MP4 fixture');assert.deepEqual(await page.evaluate(()=>sent),['direct','direct']);
console.log('PASS a playlist failure leaves the direct choice usable');
await page.evaluate(()=>{
 globalThis.streamChoice={key:'early-stream',height:720,container:'mp4',label:'Verified 720p stream',audioOnlyAvailable:true,audioOptions:[{key:'en',label:'English',default:true}]};
 globalThis.laterChoice={key:'late-stream',height:1080,container:'ts',label:'Verified 1080p stream'};
 chrome.runtime.sendMessage=async message=>{
  if(message.action!=='site-formats')return {ok:true};
  globalThis.streamLookup=message;
  return new Promise(resolve=>globalThis.finishStreamLookup=()=>resolve({ok:true,choices:[laterChoice,streamChoice]}));
 };
 globalThis.streamProgress=(changes,result,sender={id:'udm-fixture'})=>deliverProgress({action:'site-formats-progress',token:streamLookup.token,progressId:streamLookup.progressId,sequence:1,result,...changes},sender);
});
await click('Download this video with UDM');
await page.evaluate(()=>streamProgress({progressId:'old-request'},{ok:true,moreFormats:true,choices:[laterChoice]}));
assert(!(await ax()).some(n=>n.name?.value==='Verified 1080p stream'));
await page.evaluate(()=>streamProgress({},{ok:true,moreFormats:true,choices:[streamChoice]}));
await node('Verified 720p stream');
await click('Audio only (M4A)…');
assert((await ax()).some(n=>n.name?.value==='Audio track'));
await page.evaluate(()=>finishStreamLookup());await page.waitForTimeout(100);
assert((await ax()).some(n=>n.name?.value==='Audio track'));
await click('Back');await node('Verified 1080p stream');
await page.evaluate(()=>streamProgress({sequence:3},{ok:true,moreFormats:true,choices:[streamChoice]}));
assert((await ax()).some(n=>n.name?.value==='Verified 1080p stream'));
console.log('PASS only current stream updates render, and completed catalogs keep an open audio chooser intact');
fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({passed:10},null,2));
})().catch(e=>{console.error(e);process.exitCode=1;}).finally(async()=>{await browser?.close();});
