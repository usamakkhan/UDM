'use strict';
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),assert=require('node:assert/strict'),{spawn}=require('node:child_process');
const root=path.resolve(process.argv[2]),app=process.env.UDM_APP_EXE||path.resolve(__dirname,'../release/UDM.exe'),host=path.join(path.dirname(app),'Udm.NativeHost.exe'),parent=path.basename(process.execPath).toLowerCase(),results=[];
const pause=ms=>new Promise(resolve=>setTimeout(resolve,ms));
function request(tag,message){return new Promise((resolve,reject)=>{
 const child=spawn(host,[],{windowsHide:true,env:{...process.env,UDM_INSTANCE_TAG:tag}}),chunks=[];const timer=setTimeout(()=>{child.kill();reject(Error('Native request timed out'));},5000);
 child.on('error',e=>{clearTimeout(timer);reject(e);});child.stdout.on('data',b=>chunks.push(b));child.stdin.on('error',()=>{});child.on('close',code=>{clearTimeout(timer);try{assert.equal(code,0);const data=Buffer.concat(chunks);assert.equal(data.length,4+data.readUInt32LE(0));resolve(JSON.parse(data.subarray(4)));}catch(e){reject(e);}});
 const data=Buffer.from(JSON.stringify(message)),prefix=Buffer.alloc(4);prefix.writeUInt32LE(data.length);child.stdin.end(Buffer.concat([prefix,data]));
 });}
function pass(name){results.push({name,passed:true});console.log('PASS '+name);}
(async()=>{
 fs.mkdirSync(root,{recursive:true});
 for(const enabled of [false,true]){
  const tag='options'+Date.now()+(enabled?'on':'off'),folder=path.join(root,enabled?'enabled':'disabled');fs.mkdirSync(folder,{recursive:true});const state=path.join(folder,'state.json');assert(!fs.existsSync(state),'Use fresh test output');
  fs.writeFileSync(state,JSON.stringify({Schema:1,Settings:{DownloadFolder:path.join(folder,'downloads'),BrowserCaptureTargets:{[parent]:{Name:'Native integration fixture',Enabled:enabled}},CaptureForceKey:'Ctrl+Ins',CaptureBypassKey:'Alt+Del',CaptureForceClick:false,VideoPanelTypes:{mp4:true,webm:false},VideoPanelMinKb:{mp4:1000},VideoPanelExcludedHosts:'*.blocked.test',BrowserContextMenus:{chromium:{Link:false,All:true},firefox:{Link:true,All:false}},Sound:false,SuppressCompletionDialog:true,PrefetchFileInfo:false},Queues:[{Name:'Main queue',Enabled:false,Parallel:1}],Downloads:[],Projects:[]}));
  const child=spawn(app,['--background','--data-dir',folder,'--instance-tag',tag],{windowsHide:true,stdio:'ignore'});
  let spawnError;child.on('error',e=>{spawnError=e;});
  try{
   let diagnostics;const end=Date.now()+20000;while(Date.now()<end){if(spawnError)throw spawnError;if(child.exitCode!==null)throw Error('App exited before native integration test');try{const reply=await request(tag,{action:'diagnostics'});if(reply.ok){diagnostics=reply;break;}}catch{}await pause(100);}
   assert(diagnostics?.ok);assert.equal(diagnostics.version,'0.65.0');assert.equal(path.resolve(diagnostics.dataDirectory),folder);assert.equal(diagnostics.downloads,0);pass('Running 0.63 app uses the isolated '+(enabled?'enabled':'disabled')+' test catalog');
   const reply=await request(tag,{action:'preferences-if-running',browserExecutable:'msedge.exe'});assert.equal(reply.ok,true);assert.equal(reply.captureAllowed,enabled);assert.equal(reply.panelEnabled,enabled);pass('Native parent identity '+(enabled?'enables':'disables')+' capture and video panels, ignoring a supplied Edge executable');
   assert.equal(reply.forceKey,'Ctrl+Ins');assert.equal(reply.bypassKey,'Alt+Del');assert.equal(reply.forceClick,false);assert.deepEqual(reply.panelTypes,{mp4:true,webm:false});assert.equal(reply.panelMinKb.mp4,1000);assert.deepEqual(reply.panelExcluded,['*.blocked.test']);assert.deepEqual(reply.contextMenu,{Link:false,All:true});pass('Custom keyboard, panel and browser-menu rules reach the real native host');
   assert(!('BrowserCaptureTargets' in reply)&&!('SiteLogins' in reply)&&!('ProxySecret' in reply));pass('Per-browser preference response keeps local settings and secrets private');
  }finally{if(child.exitCode===null){child.kill();await new Promise(resolve=>{child.once('close',resolve);setTimeout(resolve,5000).unref();});}}
 }
})().catch(e=>{results.push({passed:false,error:e.stack});console.error(e);process.exitCode=1;}).finally(()=>{fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({scope:'Real native desktop/host protocol; Node process emulates a named browser parent. No UI automation, browser profile or user history changes.',binarySha256:{app:crypto.createHash('sha256').update(fs.readFileSync(app)).digest('hex'),host:crypto.createHash('sha256').update(fs.readFileSync(host)).digest('hex')},results},null,2));console.log(results.filter(x=>x.passed).length+' passed, '+results.filter(x=>!x.passed).length+' failed');});
