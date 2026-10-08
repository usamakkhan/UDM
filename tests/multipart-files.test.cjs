'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
let passed=0;
const file=(body,name='binary.bin',type='application/octet-stream')=>({kind:'file',name,type,body:Buffer.from(body).toString('base64')});
const raw=b=>({raw:[{bytes:Uint8Array.from(b).buffer}]}),url='https://files.test/result',type='multipart/form-data; boundary=fixture';
const sender={tab:{id:2,incognito:false},frameId:0,documentId:'doc',url:'https://files.test/form'};
const head=Buffer.from('--fixture\r\nContent-Disposition: form-data; name="upload"; filename="binary.bin"\r\nContent-Type: application/octet-stream\r\n\r\n');
const tail=Buffer.from('\r\n--fixture--\r\n'),bytes=Buffer.from([0,128,255,13,10,1,2]);
(async()=>{
 for(const family of ['chromium','firefox']){
  const M=require('../browser/'+family+'/multipart.js'),C=require('../browser/'+family+'/request-context.js');
  const fields=[['upload',file(bytes)]],envelope=Buffer.concat([head,tail]),whole=Buffer.concat([head,bytes,tail]);
  const test=async(name,fn)=>{await fn();console.log('PASS '+family+': '+name);passed++;};
  function setup({body=raw(envelope),entries=fields,prior,source=sender,before=true,firefox=false}={}){
   const c=C.create(firefox?{runtime:{getBrowserInfo:async()=>({name:'Firefox'})}}:{}),event={requestId:'one',url,method:'POST',tabId:2,frameId:0,documentId:'doc',documentUrl:sender.url};
   const snapshot=()=>c.captureForm({url,fields:entries,...(prior?{priorFields:prior}:{})},source);
   if(before&&entries)snapshot();c.begin({...event,requestBody:body});c.headers({...event,requestHeaders:[{name:'Content-Type',value:type}]});
   c.finish({...event,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment; filename=out.zip'}]});if(!before&&entries)snapshot();
   return {c,event,read:()=>c.resolve({url,browserDownload:true})};
  }
  await test('Binary bytes, filename and media type survive multipart serialization',()=>assert.deepEqual(Buffer.from(M.encode(M.capture(fields),type)),whole));
  await test('Trusted File snapshot retains every binary byte',async()=>{
   const result=await M.captureEntries([['upload',new File([bytes],'binary.bin',{type:'application/octet-stream'})]]);
   assert.deepEqual(result.fields,fields);assert.equal(result.files,1);
  });
  await test('File encoding works with indexed typed arrays whose iterator is hidden',async()=>{
   class WrappedArray extends Uint8Array {}
   Object.defineProperty(WrappedArray.prototype,Symbol.iterator,{value:undefined});
   Object.defineProperty(WrappedArray.prototype,'constructor',{get(){throw Error('Permission denied to access property constructor');}});
   const scope=vm.createContext({TextEncoder,Uint8Array:WrappedArray,File,Blob,atob,btoa});
   vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser',family,'multipart.js'),'utf8'),scope);
   const result=await scope.UdmMultipart.captureEntries([['upload',new File([bytes],'binary.bin',{type:'application/octet-stream'})]]);
   assert(result);assert.equal(result.fields[0][1].body,bytes.toString('base64'));
  });
  await test('Empty selected File is represented as an empty file, not omitted',async()=>{
   const result=await M.captureEntries([['empty',new File([],'')]]);assert.equal(result.files,1);assert.equal(result.fields[0][1].body,'');assert(M.encode(result,type).length>0);
  });
  await test('Serialized file objects are rejected by the DOM File reader',async()=>assert.equal(await M.captureEntries(fields),null));
  await test('Oversized Files are rejected before their bytes are read',async()=>{
   const result=await M.captureEntries([['large',new File([Buffer.alloc(4*1048576+1)],'large.bin')]]);assert.equal(result,null);
  });
  await test('Invalid file metadata and malformed binary encoding are rejected',()=>{
   for(const bad of [{...file(bytes),type:'x\r\nInjected:y'},{...file(bytes),body:'A==='},{...file(bytes),body:'Zh=='},{...file(bytes),path:'C:/secret'},{kind:'file',name:'x',body:'AA=='}])assert.equal(M.capture([['upload',bad]]),null);
  });
  await test('Observed envelope with omitted file bytes authorizes exact binary replay',()=>assert.deepEqual(Buffer.from(setup().read().request.body,'base64'),whole));
  await test('Fully exposed upload bytes match the same form',()=>assert.deepEqual(Buffer.from(setup({body:raw(whole)}).read().request.body,'base64'),whole));
  await test('Explicit filesystem markers use only captured File bytes, never read a path',()=>{
   const body={raw:[{bytes:Uint8Array.from(head).buffer},{file:'C:/fixture/binary.bin'},{bytes:Uint8Array.from(tail).buffer}]};
   assert.deepEqual(Buffer.from(setup({body}).read().request.body,'base64'),whole);
  });
  await test('File markers in non-multipart requests cannot become partial raw POSTs',()=>{
   const {c,event,read}=setup({entries:null,body:{raw:[{file:'C:/fixture/binary.bin'},{bytes:Uint8Array.from(bytes).buffer}]}});
   c.headers({...event,requestHeaders:[{name:'Content-Type',value:'application/octet-stream'}]});assert.equal(read().request,null);
  });
  await test('Observed envelope without a form snapshot is insufficient',()=>assert.equal(setup({entries:null}).read().request,null));
  await test('Observed filename dictionary matches a trusted File snapshot',()=>assert.deepEqual(Buffer.from(setup({body:{formData:{upload:['binary.bin']}}}).read().request.body,'base64'),whole));
  await test('Firefox preserves filename escapes while decoding field names',()=>{
   const entries=[['name%22',file(bytes,'literal%22%GG+é.bin')]],body={formData:{'name"':['literal%22%GG+é.bin']}};
   assert.deepEqual(Buffer.from(setup({entries,body,firefox:true}).read().request.body,'base64'),Buffer.from(M.encode(M.capture(entries),type)));
  });
  await test('Chromium decoding and Firefox escaping cannot be interchanged',()=>{
   const entries=[['upload',file(bytes,'literal%22.bin')]],body={formData:{upload:['literal".bin']}};
   assert(setup({entries,body}).read().request);assert.equal(setup({entries,body,firefox:true}).read().request,null);
  });
  await test('Firefox escaped filename and decoded newline field names match exactly',()=>{
   const entries=[['a\nb',file(bytes,'quote"\rfile.bin')]],body={formData:{'a\r\nb':['quote%22%0Dfile.bin']}};
   assert(setup({entries,body,firefox:true}).read().request);
  });
  await test('Filename-only dictionary without a trusted File snapshot is insufficient',()=>assert.equal(setup({entries:null,body:{formData:{upload:['binary.bin']}}}).read().request,null));
  await test('Changed File bytes remain ambiguous with filename-only observation',()=>assert.equal(setup({entries:[['upload',file([7,7,7,7,7,7,7])]],prior:fields,body:{formData:{upload:['binary.bin']}}}).read().request,null));
  await test('Mismatched observed filename cannot authorize captured bytes',()=>assert.equal(setup({body:{formData:{upload:['other.bin']}}}).read().request,null));
  await test('Late File snapshot binds after response observation',()=>assert(setup({before:false}).read().request));
  await test('File bytes changed under identical metadata are treated as ambiguous',()=>{
   const different=[['upload',file(Buffer.from([8,8,8,8,8,8,8]))]];assert.equal(setup({entries:different,prior:fields}).read().request,null);
  });
  await test('Changed filename resolves the new form entry without using the old File',()=>{
   const different=[['upload',file(bytes,'changed.bin')]],body=raw(Buffer.from(envelope.toString().replace('binary.bin','changed.bin')));
   assert(setup({entries:different,prior:fields,body}).read().request);
  });
  await test('Mismatched names, MIME types, boundaries or exposed binary bytes are rejected',()=>{
   for(const b of [Buffer.from(envelope.toString().replace('upload','other')),Buffer.from(envelope.toString().replace('octet-stream','other')),Buffer.from(envelope.toString().replaceAll('fixture','different')),Buffer.concat([head,Buffer.from([9,9,9]),tail])])assert.equal(setup({body:raw(b)}).read().request,null);
  });
  await test('Too many file markers are rejected',()=>assert.equal(setup({body:{raw:[{bytes:Uint8Array.from(head).buffer},{file:'a'},{file:'b'},{bytes:Uint8Array.from(tail).buffer}]}}).read().request,null));
  await test('Filename newlines are escaped without normalizing the file name',()=>{
   const result=Buffer.from(M.encode(M.capture([['x',file([],'a\rb\nc"')]]),type)).toString();assert(result.includes('filename="a%0Db%0Ac%22"'));
  });
  await test('Metadata overhead counts toward the 4 MiB native limit',()=>{
   const snapshot=M.capture([['x',file(Buffer.alloc(4*1048576-100),'x')]]);assert(snapshot);assert.equal(M.encode(snapshot,type),null);
  });
  await test('The observed order of mixed text and duplicate file fields is required',()=>{
   const entries=[['z','before'],['upload',file(bytes)],['z','middle'],['upload',file([4,5,6],'second.bin')],['z','after']];
   const encoded=M.encode(M.capture(entries),type);assert(encoded);
   assert(M.matchesRaw(M.capture(entries),encoded,type));assert(!M.matchesRaw(M.capture(entries.slice().reverse()),encoded,type));
  });
  await test('Mixed exposed and omitted File payloads reconcile against the whole envelope',()=>{
   const entries=[['upload',file(bytes)],['other',file([4,5,6],'second.bin')]],snapshot=M.capture(entries),encoded=Buffer.from(M.encode(snapshot,type));
   const withoutFirst=Buffer.concat([encoded.subarray(0,head.length),encoded.subarray(head.length+bytes.length)]);
   assert(M.matchesRaw(snapshot,withoutFirst,type));
  });
  await test('File snapshots remain tied to tab, frame, document and private-mode state',()=>{
   for(const source of [{...sender,frameId:1},{...sender,documentId:'old'},{...sender,tab:{id:3}},{...sender,tab:{id:2,incognito:true}}])assert.equal(setup({source}).read().request,null);
  });
  await test('File snapshot consumption prevents later unrelated upload reuse',()=>{
   const {c,event,read}=setup();assert(read().request);const next={...event,requestId:'two'};
   c.begin({...next,requestBody:raw(envelope)});c.headers({...next,requestHeaders:[{name:'Content-Type',value:type}]});c.finish({...next,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});assert.equal(read().request,null);
  });
  await test('Same-request same-origin 307 preserves the validated File body',()=>{
   const {c,event,read}=setup(),expected=read().request;c.finish({...event,statusCode:307});const next={...event,url:'https://files.test/redirect'};
   c.begin({...next,requestBody:raw(envelope)});c.headers({...next,requestHeaders:[{name:'Content-Type',value:type}]});c.finish({...next,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});assert.deepEqual(c.resolve({url:next.url,browserDownload:true}).request,expected);
  });
  await test('Cross-origin redirect cannot inherit captured files',()=>{
   const {c,event}=setup();c.finish({...event,statusCode:307});const next={...event,url:'https://other.test/redirect'};
   c.begin({...next,requestBody:raw(envelope)});c.headers({...next,requestHeaders:[{name:'Content-Type',value:type}]});c.finish({...next,statusCode:200,responseHeaders:[{name:'Content-Disposition',value:'attachment'}]});assert.equal(c.resolve({url:next.url,browserDownload:true}).request,null);
  });
  await test('File envelopes and binary payloads are charged to the bounded cache',()=>{
   const c=C.create({});for(let i=0;i<70;i++)c.captureForm({url,fields:[['upload',file(Buffer.alloc(200000,i))]]},sender);assert(c.diagnostics().bodyBytes<=32*1024*1024);c.clear(2);assert.equal(c.diagnostics().bodyBytes,0);
  });
 }
 for(const name of ['multipart.js','forms.js','request-context.js'])assert.equal(fs.readFileSync(path.join(__dirname,'../browser/chromium',name),'utf8'),fs.readFileSync(path.join(__dirname,'../browser/firefox',name),'utf8'));
 console.log('ALL '+passed+' FILE MULTIPART CHECKS PASSED');
})().catch(error=>{console.error(error);process.exitCode=1;});
