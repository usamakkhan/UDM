'use strict';
const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const out=path.resolve(process.argv[2]),results=[];let browser;
(async()=>{
 fs.mkdirSync(out,{recursive:true});browser=await chromium.launch({channel:process.env.UDM_TEST_BROWSER||'msedge',headless:true});
 for(const family of ['chromium','firefox']){
  const page=await browser.newPage();await page.route('https://key-focus.test/**',r=>r.fulfill({contentType:'text/html',body:'<button id="neutral">Neutral</button><input id="ordinary"><div id="host"></div><div contenteditable="true"><span id="inherited" tabindex="0">Editor</span></div>'}));await page.goto('https://key-focus.test/');
  await page.evaluate(()=>{const outer=document.querySelector('#host').attachShadow({mode:'open'});outer.innerHTML='<div id="nested"></div>';outer.querySelector('#nested').attachShadow({mode:'open'}).innerHTML='<input id="shadow">';});
  for(const file of ['media.js','key-capture.js'])await page.addScriptTag({path:path.join(__dirname,'../browser',family,file)});
  await page.evaluate(()=>{
   const sender={id:'fixture',tab:{id:7},frameId:0,documentId:'doc',url:location.href};
   const api={runtime:{id:'fixture'},tabs:{get:async()=>({url:location.href})},scripting:{executeScript:async options=>[{frameId:0,documentId:'doc',result:options.func(...options.args)}]}};
   const controller=UdmKeyCapture.create(api,async()=>({capture:true,forceClick:false,forceKey:'Ctrl'}),async()=>false,null,async()=>({ok:true}));
   globalThis.hold=()=>controller.update({keys:{ctrlKey:true}},sender);globalThis.intent=()=>controller.intent({tabId:7,frameId:0,documentId:'doc'});
  });
  for(const [selector,expected] of [['#ordinary',''],['#shadow',''],['#inherited',''],['#neutral','force']]){
   await page.locator('#neutral').focus();await page.evaluate(()=>hold());await page.locator(selector).focus();assert.equal(await page.evaluate(()=>intent()),expected);
   results.push({family,selector,passed:true});
  }
  await page.close();
 }
 console.log(results.length+' real-DOM focus checks passed');
})().catch(e=>{results.push({passed:false,error:e.stack});console.error(e);process.exitCode=1;}).finally(async()=>{fs.mkdirSync(out,{recursive:true});fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({browser:'Edge headless',api:'Mocked extension API invokes production readState against real DOM',checks:results,passed:results.every(x=>x.passed)},null,2));await browser?.close();});
