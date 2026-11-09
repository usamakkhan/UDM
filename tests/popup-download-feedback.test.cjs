'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const flush=()=>new Promise(resolve=>setImmediate(resolve));

async function check(browser){
 const folder=path.join(__dirname,'../browser',browser);
 const html=fs.readFileSync(path.join(folder,'popup.html'),'utf8');
 const elements=new Map([...html.matchAll(/id="([^"]+)"/g)].map(match=>[match[1],{
  value:'',textContent:'',disabled:false,append(){},replaceChildren(){},addEventListener(name,listener){this[name]=listener;},
 }]));
 const $=id=>elements.get(id),requests=[],timers=new Map(),storageListeners=[];
 let nextTimer=0;
 const api={
  tabs:{
   query:async()=>[{id:1,url:'https://www.youtube.com/watch?v=fixture'}],
   sendMessage:async()=>({ok:true,choices:[{key:'video',height:1080,label:'1080p'}]}),
  },
  permissions:{contains:async()=>false},
  storage:{local:{get:async()=>({settings:{}})},session:{get:async()=>({automaticDownloadNotice:'excluded'})},
   onChanged:{addListener:listener=>storageListeners.push(listener)}},
  runtime:{sendMessage:message=>{
   if(!['download','media'].includes(message.action))return Promise.resolve({ok:true,projects:[]});
   return new Promise((resolve,reject)=>requests.push({message,resolve,reject}));
  }},
 };
 vm.runInNewContext(fs.readFileSync(path.join(folder,'popup.js'),'utf8'),{
  chrome:api,URL,document:{getElementById:$,createElement:()=>({})},
  UdmFileRecognition:require(path.join(folder,'file-recognition.js')),
  setTimeout:callback=>{const id=++nextTimer;timers.set(id,callback);return id;},
  clearTimeout:id=>timers.delete(id),
 });
 await flush();
 assert.equal($('download-notice').hidden,false);
 assert.match($('download-notice').textContent,/kept in the browser.*exclusion/);
 storageListeners[0]({automaticDownloadNotice:{newValue:'<unrecognized>'}},'session');
 assert.equal($('download-notice').hidden,true);
 assert.equal($('download-notice').textContent,'');
 storageListeners[0]({automaticDownloadNotice:{newValue:'desktop-disabled'}},'session');
 assert.match($('download-notice').textContent,/desktop Browser Integration/);
 storageListeners[0]({automaticDownloadNotice:{newValue:''}},'session');
 assert.equal($('download-notice').hidden,true);

 $('url').value='https://example.test/fixture.mp4';
 const submit=()=>$('add').submit({preventDefault(){}});
 submit();
 submit();
 assert.equal(requests.length,1,'A repeated link click must share its pending handoff');
 assert.equal($('status').textContent,'','The pending link does not display a sending banner');
 requests[0].reject(new Error('Desktop unavailable'));
 await flush();
 assert.equal($('status').textContent,'Desktop unavailable');
 submit();
 assert.equal(requests.length,2,'A failed link can be retried');
 requests[1].resolve({ok:true});
 await flush();
 assert.equal($('status').textContent,'Added to UDM.');

 $('quality').value='video';
 const video=$('video-download');
 const selection=video.onclick();
 await video.onclick();
 for(const callback of timers.values())callback();
 assert.equal(requests.length,3,'A pending quality is sent once');
 assert.equal(requests[2].message.height,1080);
 assert.equal($('video-status').textContent,'','The pending video has no preparing banner');
 assert.equal(video.disabled,true);
 requests[2].resolve({ok:true});
 await selection;
 assert.equal(video.disabled,false);
 assert.match($('video-status').textContent,/Video added/);
 console.log('PASS '+browser+' quiet single handoffs, duplicate clicks, and retry feedback');
}

(async()=>{for(const browser of ['chromium','firefox'])await check(browser);})()
 .catch(error=>{console.error(error);process.exitCode=1;});
