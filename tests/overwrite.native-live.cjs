'use strict';
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),assert=require('node:assert/strict'),{spawn}=require('node:child_process');
const {fixture}=require('./support/ftp-fixture.cjs');
const root=path.resolve(process.argv[2]),exe=process.env.UDM_TEST_EXE||path.resolve(__dirname,'../release/Udm.NativeTests.exe'),results=[];
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');fs.mkdirSync(root,{recursive:true});
async function run(dir,spec){fs.mkdirSync(dir,{recursive:true});const input=path.join(dir,'input.json');fs.writeFileSync(input,JSON.stringify(spec));return await new Promise((resolve,reject)=>{const child=spawn(exe,['--feature-spec',input],{windowsHide:true}),chunks=[];let timer=setTimeout(()=>{child.kill();reject(Error('Overwrite fixture timed out'));},45000);child.on('error',e=>{clearTimeout(timer);reject(e);});child.stdout.on('data',b=>chunks.push(b));child.stderr.on('data',b=>chunks.push(b));child.on('close',code=>{clearTimeout(timer);const output=Buffer.concat(chunks).toString();fs.appendFileSync(path.join(dir,'native.log'),output+'\n');try{assert.equal(code,0,output);resolve(JSON.parse(fs.readFileSync(path.join(dir,'result.json'),'utf8')));}catch(e){reject(e);}});});}
async function test(name,fn){try{await fn();results.push({name,passed:true});console.log('PASS '+name);}catch(e){results.push({name,passed:false,error:e.stack});console.error('FAIL '+name+': '+e.message);}fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({results,passed:results.filter(r=>r.passed).length,failed:results.filter(r=>!r.passed).length},null,2));}
(async()=>{
 for(const passive of [true,false])for(const unfinished of [false,true])await test((passive?'Passive':'Active')+' FTP overwrites '+(unfinished?'unfinished saved parts':'a completed file')+' with exact new bytes',async()=>{
  const f=await fixture({delay:5}).start();try{
   const dir=path.join(root,(passive?'passive':'active')+'-'+(unfinished?'partial':'complete'));
   const before=Buffer.from(f.o.body),first=await run(dir,{url:f.url,passive,connections:4,...(unfinished?{cancelMs:300}:{})});
   assert.equal(first.status,unfinished?'Paused':'Complete',first.error);assert.ok(first.bytes>0);if(unfinished)assert.ok(first.bytes<before.length);else assert.equal(first.sha256,hash(before));
   const oldState=JSON.parse(fs.readFileSync(path.join(dir,'state/state.json'),'utf8'));const oldParts=oldState.Downloads[0].PartsFolder||path.join(dir,'state/parts',first.jobId);
   f.o.body=Buffer.from(before);for(let i=0;i<f.o.body.length;i++)f.o.body[i]^=0x5a;f.o.modified='20260928000000';const offsets=f.metrics.offsets.length;
   const second=await run(dir,{resume:true,overwrite:true,rememberOverwrite:true,passive});
   assert.equal(second.status,'Complete',second.error);assert.equal(second.path,first.path);assert.equal(second.sha256,hash(f.o.body));assert.deepEqual(fs.readFileSync(second.path),f.o.body);assert.ok(f.metrics.offsets.slice(offsets).includes(0));
   const state=JSON.parse(fs.readFileSync(path.join(dir,'state/state.json'),'utf8'));assert.equal(state.Settings.DuplicatePolicy,'Replace');
   if(unfinished){assert.equal(second.jobId,first.jobId);assert.equal(second.records,1);assert.ok(second.restartAttempt);assert.equal(fs.existsSync(oldParts)&&fs.readdirSync(oldParts).some(x=>x.endsWith('.part')),false);}
   else{assert.equal(second.records,2);const prior=state.Downloads.find(x=>x.Id===first.jobId);assert.equal(prior.PreviousVersionOf,second.jobId);assert.deepEqual(fs.readFileSync(path.join(prior.Folder,prior.FileName)),before);}
  }finally{f.close();}
 });
})().catch(e=>{console.error(e);process.exitCode=1;}).finally(()=>{if(results.some(x=>!x.passed))process.exitCode=1;console.log(results.filter(x=>x.passed).length+' passed, '+results.filter(x=>!x.passed).length+' failed');});
