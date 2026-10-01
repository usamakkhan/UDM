'use strict';
// Execute the real onCreated path with a mocked browser, stopping before native Add.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');let passed=0;
async function probe(family,url,localExclusions,desktop){
 const events={},calls=[],event=name=>({addListener:f=>(events[name]??=[]).push(f)});
 const local={settings:{capture:true,cookies:false,extensions:['zip'],...localExclusions},desktopPolicy:{}};
 const api={runtime:{id:'fixture',onInstalled:event('installed'),onStartup:event('startup'),onMessage:event('message'),getURL:f=>'extension://fixture/'+f,sendNativeMessage:async(host,message)=>{if(message.action==='preferences'){calls.push('preferences');return {ok:true,captureAllowed:true,extensions:['zip'],...desktop};}throw Error('Unexpected native message');}},storage:{local:{get:async()=>local,set:async()=>{}},session:{get:async()=>({}),set:async()=>{},remove:async()=>{}},onChanged:event('storage')},contextMenus:{onClicked:event('click'),removeAll:async()=>{},create:()=>{}},tabs:{onRemoved:event('removed'),onUpdated:event('updated'),onActivated:event('activated'),query:async()=>[],get:async()=>({id:7,url:'https://page.test/'})},permissions:{contains:async()=>false},action:{setBadgeText:async()=>{}},downloads:{onCreated:event('download'),pause:async()=>calls.push('pause'),resume:async()=>calls.push('resume'),cancel:async()=>calls.push('cancel'),search:async()=>{calls.push('eligible');throw Error('Fixture stops before Add');}},webRequest:{onHeadersReceived:event('headers')},scripting:{executeScript:async()=>[]}};
 const box={chrome:api,...(family==='firefox'?{browser:api}:{}),URL,console:{error(){},warn(){},log(){}},crypto:require('node:crypto'),structuredClone,navigator:{userAgent:'fixture'},setTimeout,clearTimeout};vm.createContext(box);
 for(const file of ['file-recognition.js','media.js','background.js'])vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser',family,file),'utf8'),box,{filename:file});
 for(const fn of events.download||[])await fn({id:1,url,filename:'file.zip',mime:'application/zip',state:'in_progress',incognito:false});return calls;
}
(async()=>{
 const exact='https://cdn.test/file.zip?literal=*';
 for(const family of ['chromium','firefox']){
  assert.deepEqual(await probe(family,exact,{excludedUrls:['='+exact]},{}),[]);console.log('PASS '+family+' local exact exclusion prevents even a temporary browser pause');passed++;
  assert.deepEqual(await probe(family,exact,{}, {excludedUrls:['='+exact]}),['pause','preferences','resume']);console.log('PASS '+family+' fresh desktop exact exclusion resumes the held browser download without Add or cancel');passed++;
  assert.deepEqual(await probe(family,'https://cdn.test/file.zip?literal=other',{}, {excludedUrls:['='+exact]}),['pause','preferences','eligible','resume']);console.log('PASS '+family+' a literal asterisk does not exclude other query values in automatic capture');passed++;
  assert.deepEqual(await probe(family,'https://sub.cdn.test/file.zip',{}, {excluded:['cdn.test']}),['pause','preferences','resume']);console.log('PASS '+family+' fresh desktop site exclusion releases a subdomain download back to the browser');passed++;
 }
 console.log(passed+' passed, 0 failed');
})().catch(error=>{console.error(error);process.exitCode=1;});
