'use strict';
const {chromium}=require('playwright'),fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const root=path.resolve(process.argv[2]);assert(!fs.existsSync(root));fs.mkdirSync(root,{recursive:true});let browser;const results=[];
(async()=>{
 browser=await chromium.launch({channel:'chrome',headless:true});const page=await browser.newPage();
 await page.route('**/*',route=>route.fulfill({contentType:'text/html',body:'<div class="ad-showing" id="movie_player"><video style="width:800px;height:450px"></video></div>'}));
 await page.goto('https://www.youtube.com/watch?v=abcdefghijk');
 await page.evaluate(()=>{window.chrome={storage:{local:{get:async()=>({}),set:async()=>{}}},runtime:{id:'fixture',sendMessage:async()=>({ok:true,choices:[]})}}});
 for(const file of ['media.js','content.js'])await page.addScriptTag({path:path.resolve(__dirname,'../browser/chromium',file)});
 async function visible(expected){await page.waitForFunction(expected=>{const p=document.querySelector('[id^="udm-video-panel-"]');return p&&(getComputedStyle(p).display!=='none')===expected},expected,{timeout:1500})}
 await visible(false);results.push({name:'ad-only panel remains hidden',passed:true});
 await page.evaluate(()=>document.documentElement.setAttribute('data-udm-content-ready','differentid:'+Date.now()));
 await visible(false);results.push({name:'other video readiness does not reveal panel',passed:true});
 await page.evaluate(()=>document.documentElement.setAttribute('data-udm-content-ready','abcdefghijk:'+Date.now()));
 await visible(true);results.push({name:'verified target content reveals panel before ad flag clears',passed:true});
 await page.evaluate(()=>document.documentElement.setAttribute('data-udm-content-ready','abcdefghijk:'+(Date.now()-300001)));
 await visible(false);results.push({name:'expired readiness does not reveal panel',passed:true});
 await page.evaluate(()=>document.querySelector('#movie_player').classList.remove('ad-showing'));
 await visible(true);results.push({name:'normal content still reveals panel',passed:true});
 console.log(results.length+' panel ad-transition checks passed');
})().catch(error=>{results.push({passed:false,error:error.message});console.error(error);process.exitCode=1})
.finally(async()=>{fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({results},null,2));await browser?.close()});
