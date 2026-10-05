'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
let passed=0;
(async()=>{
 for(const family of ['chromium','firefox']){
  const M=require('../browser/'+family+'/media.js');
  for(const scenario of ['ordinary input','nested shadow input','inherited editable','non-editable control']){
   const leaf={isContentEditable:scenario==='inherited editable',closest:()=>scenario.includes('input')?{}:null};
   let active=leaf;if(scenario==='nested shadow input')active={closest:()=>null,shadowRoot:{activeElement:{closest:()=>null,shadowRoot:{activeElement:leaf}}}};
   const document={activeElement:{closest:()=>null},querySelectorAll:()=>[],hasFocus:()=>true,visibilityState:'visible'};
   const page='https://fixture.test/page',sender={id:'fixture',tab:{id:7},frameId:0,documentId:'doc',url:page};
   const api={runtime:{id:'fixture'},tabs:{get:async()=>({url:page})},scripting:{executeScript:async options=>[{frameId:0,documentId:'doc',result:options.func(...options.args)}]}};
   const box={document,location:{href:page},performance:{timeOrigin:1000},URL,Date,UdmMedia:M};vm.createContext(box);
   vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser',family,'key-capture.js'),'utf8'),box);
   const controller=box.UdmKeyCapture.create(api,async()=>({capture:true,forceClick:false,forceKey:'Ctrl'}),async()=>false,null,async()=>({ok:true}));
   await controller.update({keys:{ctrlKey:true}},sender);document.activeElement=active;
   const actual=await controller.intent({tabId:7,frameId:0,documentId:'doc'});
   assert.equal(actual,scenario==='non-editable control'?'force':'',family+' '+scenario);
   passed++;console.log('PASS '+family+' '+scenario);
  }
 }
 console.log(passed+' passed, 0 failed');
})().catch(e=>{console.error(e);process.exitCode=1;});
