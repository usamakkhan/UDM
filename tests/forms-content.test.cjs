'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
let count=0;
class WrappedFormData extends FormData {
 entries(){const iterator=super.entries();return {next:()=>iterator.next()};}
}
class Form {
 constructor(){this.action='https://fixture.test/result';this.method='post';this.enctype='multipart/form-data';this.acceptCharset='utf-8';}
}
function fixture(family){
 const listeners={},timers=[],messages=[];
 const scope=vm.createContext({
  browser:{runtime:{id:'fixture',sendMessage:async message=>messages.push(message)}},
  document:{characterSet:'UTF-8',addEventListener:(type,listener)=>listeners[type]=listener},
  location:{href:'https://fixture.test/form'},performance:{now:()=>100},
  HTMLFormElement:Form,FormData:WrappedFormData,File,Blob,Uint8Array,TextEncoder,URL,atob,btoa,
  setTimeout:fn=>timers.push(fn)
 });
 for(const name of ['multipart.js','forms.js'])vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser',family,name),'utf8'),scope);
 const form=new Form(),data=new WrappedFormData();
 // Simulate page-level replacements: the observer must use the pristine method.
 data.forEach=()=>{throw Error('Page replacement must not run');};
 const submit=(trusted=true)=>listeners.submit({isTrusted:trusted,target:form,submitter:null});
 const event=(trusted=true)=>listeners.formdata({isTrusted:trusted,target:form,formData:data});
 const flush=async()=>{for(const timer of timers.splice(0))await timer();await Promise.resolve();};
 return {form,data,submit,event,flush,messages};
}
(async()=>{
 for(const family of ['chromium','firefox']){
  const test=async(name,fn)=>{await fn();count++;console.log('PASS '+family+': '+name);};
  await test('Trusted wrapped FormData preserves duplicate order and File bytes',async()=>{
   const x=fixture(family);x.data.append('z','first');x.data.append('upload',new File([new Uint8Array([0,255,128])],'binary.bin'));x.data.append('z','last');
   x.submit();x.event();await x.flush();assert.equal(x.messages.length,1);
   assert.equal(JSON.stringify(x.messages[0].fields),JSON.stringify([['z','first'],['upload',{kind:'file',name:'binary.bin',type:'',body:'AP+A'}],['z','last']]));
  });
  await test('Final formdata mutations retain both initial and final snapshots',async()=>{
   const x=fixture(family);x.data.append('z','before');x.submit();x.event();x.data.append('z','after');await x.flush();
   assert.equal(JSON.stringify(x.messages[0].fields),JSON.stringify([['z','before'],['z','after']]));
   assert.equal(JSON.stringify(x.messages[0].priorFields),JSON.stringify([['z','before']]));
  });
  await test('Untrusted submit and formdata events cannot publish a snapshot',async()=>{
   for(const [submitTrusted,eventTrusted] of [[false,true],[true,false]]){const x=fixture(family);x.data.append('z','value');x.submit(submitTrusted);x.event(eventTrusted);await x.flush();assert.equal(x.messages.length,0);}
  });
  await test('A formdata event without a corresponding submit cannot publish',async()=>{
   const x=fixture(family);x.data.append('z','value');x.event();await x.flush();assert.equal(x.messages.length,0);
  });
  await test('Non-UTF-8 and oversized file forms remain browser-owned',async()=>{
   const charset=fixture(family);charset.form.acceptCharset='windows-1252';charset.data.append('z','value');charset.submit();charset.event();await charset.flush();assert.equal(charset.messages.length,0);
   const large=fixture(family);large.data.append('upload',new File([new Uint8Array(1048577)],'large.bin'));large.submit();large.event();await large.flush();assert.equal(large.messages.length,0);
  });
  await test('Over-limit entry count aborts without sending a partial snapshot',async()=>{
   const x=fixture(family);for(let i=0;i<16385;i++)x.data.append('z','v');x.submit();x.event();await x.flush();assert.equal(x.messages.length,0);
  });
 }
 console.log('ALL '+count+' FORM CONTENT CHECKS PASSED');
})().catch(error=>{console.error(error);process.exitCode=1;});
