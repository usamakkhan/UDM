'use strict';
// Execute the real automatic capture listener and check browser ownership and popup notices.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');let passed=0;
async function probe(family,url,localSettings,desktop,{incognito=false,accept=false,previous}={}){
 const events={},calls=[],event=name=>({addListener:f=>(events[name]??=[]).push(f)});
 const local={settings:{capture:true,cookies:false,extensions:['zip'],...localSettings},desktopPolicy:{}};
 const session={};
 if(previous!==undefined)session.automaticDownloadNotice=previous;
 const api={
  runtime:{id:'fixture',onInstalled:event('installed'),onStartup:event('startup'),onMessage:event('message'),getURL:f=>'extension://fixture/'+f,
   sendNativeMessage:async(host,message)=>{
    calls.push(message.action);
    if(message.action==='preferences'||message.action==='preferences-if-running')return {ok:true,captureAllowed:true,extensions:['zip'],...desktop};
    if(message.action==='add'&&accept)return {ok:true};
    throw Error('Unexpected native message');
   }},
  storage:{local:{get:async()=>local,set:async()=>{}},session:{get:async()=>session,set:async patch=>Object.assign(session,patch),remove:async()=>{}},onChanged:event('storage')},
  contextMenus:{onClicked:event('click'),removeAll:async()=>{},create:()=>{}},
  tabs:{onRemoved:event('removed'),onUpdated:event('updated'),onActivated:event('activated'),query:async()=>[],get:async()=>({id:7,url:'https://page.test/'})},
  permissions:{contains:async()=>false},action:{setBadgeText:async()=>{}},
  downloads:{onCreated:event('download'),pause:async()=>calls.push('pause'),resume:async()=>calls.push('resume'),cancel:async()=>calls.push('cancel'),
   search:async()=>{calls.push('eligible');if(accept)return [{state:'in_progress',paused:true}];throw Error('Fixture stops before Add');}},
  webRequest:{onHeadersReceived:event('headers')},scripting:{executeScript:async()=>[]},
 };
 const box={chrome:api,...(family==='firefox'?{browser:api}:{}),URL,console:{error(){},warn(){},log(){}},crypto:require('node:crypto'),structuredClone,navigator:{userAgent:'fixture'},setTimeout,clearTimeout};vm.createContext(box);
 for(const file of ['file-recognition.js','media.js','background.js'])vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser',family,file),'utf8'),box,{filename:file});
 for(const fn of events.download||[])await fn({id:1,url,filename:'file.zip',mime:'application/zip',state:'in_progress',incognito});
 return {calls,session};
}
(async()=>{
 const exact='https://cdn.test/file.zip?literal=*';
 for(const family of ['chromium','firefox']){
  const check=async(description,args,calls,notice)=>{
   const result=await probe(family,...args);
   assert.deepEqual(result,{calls,session:notice===undefined?{}:{automaticDownloadNotice:notice}},description);
   console.log('PASS '+family+' '+description);passed++;
  };
  await check('local exact exclusion leaves the browser untouched and records only a reason',[exact,{excludedUrls:['='+exact]},{}],[],'excluded');
  await check('fresh desktop exact exclusion leaves the browser untouched',[exact,{},{excludedUrls:['='+exact]}],['preferences'],'excluded');
  await check('a literal asterisk does not exclude other query values',['https://cdn.test/file.zip?literal=other',{},{excludedUrls:['='+exact]}],['preferences','pause','eligible','resume'],'');
  await check('fresh desktop site exclusion leaves a subdomain untouched',['https://sub.cdn.test/file.zip',{},{excluded:['cdn.test']}],['preferences'],'excluded');
  await check('extension capture off needs no desktop request',[exact,{capture:false},{}],[],'disabled');
  await check('desktop capture off is distinct from extension capture off',[exact,{},{captureAllowed:false}],['preferences'],'desktop-disabled');
  await check('a file type outside the desktop list leaves the browser untouched',[exact,{},{extensions:['pdf']}],['preferences'],'type');
  await check('an unavailable desktop is shown for a matching file type',[exact,{},{ok:false}],['preferences'],'desktop-unavailable');
  await check('private download preserves the previous regular notice',[exact,{capture:false},{},{incognito:true,previous:'excluded'}],[],'excluded');
  await check('a successful transfer clears the previous notice after native acknowledgement',[exact,{},{},{accept:true,previous:'excluded'}],['preferences','pause','eligible','add','cancel'],'');
 }
 console.log(passed+' passed, 0 failed');
})().catch(error=>{console.error(error);process.exitCode=1;});
