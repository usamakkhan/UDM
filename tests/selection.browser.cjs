'use strict';
const {chromium}=require('playwright'),http=require('node:http'),fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const output=process.argv[2]?path.resolve(process.argv[2]):path.resolve(__dirname,'../benchmarks/workflows-0.13.0');fs.mkdirSync(output,{recursive:true});
const server=http.createServer((req,res)=>{res.setHeader('Content-Type','text/html');res.end('<!doctype html><html><body style="font:16px Arial;padding:30px 30px 1200px 240px"><p id="selected">Download <a href="/one.zip">First file</a>, <a href="/two.iso">Second file</a>, <a href="/one.zip">duplicate</a> and <a href="javascript:void(0)">unsafe</a> end</p><a href="/outside.zip">Outside selection</a></body></html>');});
let browser;const results=[];async function check(name,fn){await fn();results.push({name,passed:true});console.log('PASS '+name);}
(async()=>{await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));browser=await chromium.launch({channel:'chrome',headless:true});const page=await browser.newPage({viewport:{width:1100,height:700}});const errors=[];page.on('pageerror',e=>errors.push(e.message));await page.goto('http://127.0.0.1:'+server.address().port);
 await page.evaluate(()=>{window.sent=[];window.failReview=false;window.preferenceListener=null;window.preferenceStore={settings:{}};window.chrome={storage:{local:{get:async()=>structuredClone(preferenceStore)},onChanged:{addListener:fn=>window.preferenceListener=(changes,area)=>{for(const [key,value] of Object.entries(changes))preferenceStore[key]=value.newValue;fn(changes,area);},removeListener:()=>{}}},runtime:{id:'udm-fixture',onMessage:{addListener:fn=>window.integrationListener=fn,removeListener(){}},sendMessage:async message=>{if(message.action==='integration-state')return {ok:true,disabled:false};sent.push(message);return failReview?{ok:false,error:'Review unavailable'}:{ok:true,count:message.links.length};}}};});
 const bundle=process.env.UDM_TEST_BUNDLE||'chromium';
 for(const name of ['media.js','selection.js'])await page.addScriptTag({path:path.resolve(__dirname,'../browser',bundle,name)});
 const select=()=>page.evaluate(()=>{const range=document.createRange();range.selectNodeContents(document.querySelector('#selected'));const selection=getSelection();selection.removeAllRanges();selection.addRange(range);});
 const panel=page.locator('#udm-selected-links-panel');
 const download=panel.getByRole('button',{name:'Download selected links with UDM',exact:true});
 const reselect=async()=>{await page.evaluate(()=>getSelection().removeAllRanges());await page.waitForTimeout(250);await select();await panel.waitFor();};
 await check('The strip waits for release and follows a backward selection endpoint',async()=>{
  const [start,finish]=await page.evaluate(()=>{
   const paragraph=document.querySelector('#selected');
   return [paragraph.lastChild,paragraph.firstChild].map(node=>{
    const range=document.createRange();range.setStart(node,1);range.collapse(true);
    const box=range.getBoundingClientRect();
    return {x:box.left,y:box.top+box.height/2};
   });
  });
  await page.mouse.move(start.x,start.y);await page.mouse.down();
  await page.mouse.move(finish.x,finish.y,{steps:8});
  try{
   await page.waitForFunction(()=>getSelection().toString().includes('First file'));
   await page.waitForTimeout(250);
   assert.equal(await panel.count(),0);
  }finally{await page.mouse.up();}
  await panel.waitFor();
  const endpoint=await page.evaluate(()=>{
   const selection=getSelection(),range=document.createRange();
   range.setStart(selection.focusNode,selection.focusOffset);range.collapse(true);
   const box=range.getBoundingClientRect();return {x:box.left,top:box.top};
  });
  const strip=await panel.boundingBox();
  assert.ok(Math.abs(strip.x+strip.width/2-endpoint.x)<=2);
  assert.ok(strip.y+strip.height<endpoint.top);
  await page.screenshot({path:path.join(output,'backward-selected-links.png'),clip:{x:100,y:8,width:630,height:110}});
  await page.evaluate(()=>window.scrollTo(0,320));await panel.waitFor({state:'hidden'});
  await page.evaluate(()=>window.scrollTo(0,0));await panel.waitFor();
  await page.evaluate(()=>getSelection().removeAllRanges());await panel.waitFor({state:'detached'});
 });
 await check('Selected text displays the classic strip without sending a download',async()=>{await select();await panel.waitFor();assert.equal(await download.innerText(),'Download with UDM');assert.equal(await panel.getByRole('checkbox').count(),0);assert.equal((await page.evaluate(()=>sent)).length,0);await page.screenshot({path:path.join(output,'selected-links-strip.png')});});
 await check('A single click opens batch review with unique HTTP links from the selection',async()=>{await download.click();await panel.waitFor({state:'detached'});const sent=await page.evaluate(()=>window.sent);assert.equal(sent.length,1);assert.equal(sent[0].action,'selected-links-review');assert.deepEqual(sent[0].links.map(link=>new URL(link.url).pathname),['/one.zip','/two.iso']);assert.ok(await page.evaluate(()=>getSelection().toString().length));await reselect();});
 await check('Help and a failed batch review stay on the strip without sending a retry',async()=>{await panel.getByRole('button',{name:'Selected-link panel help'}).click();assert.match(await panel.getByRole('status').textContent(),/Review the selected links/);await page.evaluate(()=>failReview=true);await download.click();await panel.getByText('Review unavailable').waitFor();assert.equal(await page.evaluate(()=>sent.length),2);await page.evaluate(()=>failReview=false);});
 await check('The close button dismisses the current selection and a new selection restores the strip',async()=>{await panel.getByRole('button',{name:'Hide selected-link panel'}).click();await panel.waitFor({state:'detached'});assert.ok(await page.evaluate(()=>getSelection().toString().length));await reselect();});
 await check('Per-tab disable hides the selected-link panel and enable restores it',async()=>{await page.evaluate(()=>integrationListener({action:'tab-integration',disabled:true},{}));await panel.waitFor({state:'detached'});await page.evaluate(()=>integrationListener({action:'tab-integration',disabled:false},{}));await panel.waitFor();});
 await check('Turning the selected-link panel off removes it without changing selection',async()=>{await page.evaluate(()=>preferenceListener({settings:{newValue:{selectedLinks:'off'}}},'local'));await panel.waitFor({state:'detached'});assert.ok(await page.evaluate(()=>getSelection().toString().length));});
 await check('Site-specific mode rejects unlisted sites',async()=>{await page.evaluate(()=>preferenceListener({settings:{newValue:{selectedLinks:'sites',selectedLinkHosts:['example.com']}}},'local'));assert.equal(await panel.count(),0);});
 await check('Compact view shows the icon and close button on an allowed host',async()=>{await page.evaluate(()=>preferenceListener({settings:{newValue:{selectedLinks:'sites',selectedLinkHosts:['127.0.0.1'],selectedLinksMini:true}}},'local'));await panel.waitFor();assert.equal((await panel.boundingBox()).width,40);assert.equal(await download.innerText(),'');assert.equal(await panel.getByRole('button',{name:'Hide selected-link panel'}).isVisible(),true);});
 await check('Excluded hosts override selected-link preferences',async()=>{await page.evaluate(()=>preferenceListener({settings:{newValue:{selectedLinks:'all',excluded:['127.0.0.1']}}},'local'));await panel.waitFor({state:'detached'});});
 await check('Clearing the selection removes the panel',async()=>{await page.evaluate(()=>preferenceListener({settings:{newValue:{}}},'local'));await panel.waitFor();await page.evaluate(()=>getSelection().removeAllRanges());await panel.waitFor({state:'detached'});});
 assert.deepEqual(errors,[]);await page.screenshot({path:path.join(output,'selection-panel-test.png')});console.log('ALL '+results.length+' SELECTED-LINK BROWSER CHECKS PASSED');
})().catch(e=>{results.push({passed:false,error:e.stack});console.error(e);process.exitCode=1;}).finally(async()=>{fs.writeFileSync(path.join(output,'selection-results.json'),JSON.stringify(results,null,2));if(browser)await browser.close();server.close();});
