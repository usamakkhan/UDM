'use strict';
const assert=require('node:assert/strict'),{create,select,wire}=require('../browser/chromium/download-session.js');
const url='https://files.example.test/private/report.bin';let passed=0;
function check(name,test){test();console.log('PASS '+name);passed++;}
const cookie=(name,value,path='/')=>({name,value,path,domain:'files.example.test',hostOnly:true,secure:true,storeId:'container'});
function handoffHarness({enabled=true,permission=true,modern=true,navigate=false,revoke=false}={}){
 const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),calls=[];let captures=0,reads=0;
 const event={addListener(){}};
 const api={runtime:{onInstalled:event,onMessage:event,sendNativeMessage:async(_,m)=>{calls.push(m);return m.action==='preferences'?{ok:true,...(modern?{browserSession:1}:{})}:{ok:true,id:'captured'};}},permissions:{contains:async()=>permission},
 storage:{local:{get:async()=>({settings:{cookies:enabled&&!(revoke&&++reads>1)}}),set:async()=>{}},session:{get:async()=>({}),set:async()=>{},remove:async()=>{}}},tabs:{onRemoved:event,onUpdated:event},action:{setBadgeText:async()=>{}},contextMenus:{onClicked:event},downloads:{onCreated:event},webRequest:{onHeadersReceived:event}};
 const context=vm.createContext({chrome:api,URL,URLSearchParams,navigator:{userAgent:'Fixture'},setTimeout,clearTimeout,console,UdmRequestContext:{create:()=>({install(){},resolve:()=>({method:'GET',headers:{Cookie:'session=secret'}})})},UdmDownloadSession:{create:()=>({capture:async()=>{captures++;return {session:{fixture:true},cookies:'session=secret',verify:async()=>{if(navigate)throw Error('page changed');}};}})}});
 vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser/chromium/file-recognition.js'),'utf8')+'\n'+fs.readFileSync(path.join(__dirname,'../browser/chromium/background.js'),'utf8'),context);
 return {calls,captures:()=>captures,run:()=>context.handoff({url,tabId:7})};
}
const a=cookie('session','secret','/private'),b=cookie('root','value'),key={topLevelSite:'https://example.test',hasCrossSiteAncestor:false};
check('Observed request selects exact scoped cookies',()=>assert.equal(wire(select([a,b],url,'container','session=secret; root=value',key)),'session=secret; root=value'));
check('Unsent SameSite cookies do not enter the session',()=>assert.deepEqual(select([a,b],url,'container','session=secret',key).map(c=>c.name),['session']));
check('Empty observed headers stay anonymous',()=>assert.equal(select([a,b],url,'container','',key).length,0));
for(const [name,change] of [['other container',{storeId:'other'}],['foreign domain',{domain:'evil.test'}],['other path',{path:'/elsewhere'}],['expired',{expirationDate:1}],['first-party isolation',{firstPartyDomain:'elsewhere.test'}],['foreign partition',{partitionKey:{topLevelSite:'https://elsewhere.test'}}],['ancestor mismatch',{partitionKey:{...key,hasCrossSiteAncestor:true}}]])check('Reject '+name,()=>assert.equal(select([{...a,...change}],url,'container','session=secret',key),null));
check('Secure cookies never enter HTTP handoffs',()=>assert.equal(select([a],url.replace('https:','http:'),'container','session=secret',key),null));
check('Known partition preserves partitioned identity',()=>assert.equal(select([{...a,partitionKey:key}],url,'container','session=secret',key)[0].partitioned,true));
check('Unknown partition falls back without inventing scope',()=>assert.equal(select([{...a,partitionKey:key}],url,'container','session=secret'),null));
check('Same name and value at multiple paths is ambiguous',()=>assert.equal(select([a,{...a,path:'/'}],url,'container','session=secret',key),null));
check('Changed browser token does not silently replace the observed account',()=>assert.equal(select([{...a,value:'new'}],url,'container','session=secret',key),null));
check('Expired and foreign cookies are excluded from explicit same-origin links',()=>assert.equal(select([a,{...b,domain:'evil.test'}],url,'container',undefined,key).length,1));
check('Exact 16384-byte Cookie header fits',()=>assert.equal(Buffer.byteLength(wire(select([cookie('a','x'.repeat(8192)),cookie('b','y'.repeat(8186))],url,'container',undefined,key))),16384));
check('Separators count toward the wire-byte limit',()=>assert.throws(()=>select([cookie('a','x'.repeat(8192)),cookie('b','y'.repeat(8187))],url,'container',undefined,key),/too much/));
check('UTF-8 bytes count toward the wire-byte limit',()=>assert.throws(()=>select([cookie('a','é'.repeat(8192))],url,'container',undefined,key),/too much/));
function harness({navigationChanged=false,privateTab=false,store='container',observed=true,crossOrigin=false,firstReadChanged=false}={}){
 let reads=0,epoch=1;const filters=[];
 const tab={id:7,url:crossOrigin?'https://other.test/':url,cookieStoreId:store,incognito:privateTab};
 const api={runtime:{getBrowserInfo:async()=>({})},tabs:{get:async()=>{if(++reads>1&&navigationChanged)epoch++;return {...tab};}},cookies:{getAllCookieStores:async()=>[{id:'container',tabIds:[7]}],getAll:async f=>{filters.push(f);return [a,b];}}};
 const nav={supported:true,token:()=>epoch,valid:(record,stamp)=>stamp===epoch};
 const context=observed?{tabId:7,frameId:0,stamp:firstReadChanged?0:1,headers:{Cookie:'session=secret','User-Agent':'Fixture'}}:null;
 return {run:()=>create(api,nav).capture({url,tabId:7},context),filters};
}
(async()=>{
 let h=harness();let result=await h.run();assert.equal(result.session.Cookies.length,1);assert.equal(h.filters[0].storeId,'container');assert.equal(result.cookies,'session=secret');console.log('PASS Capture uses the request tab container and observed cookie');passed++;
 for(const options of [{navigationChanged:true},{privateTab:true},{firstReadChanged:true}]){await assert.rejects(harness(options).run(),/changed|private/);passed++;console.log('PASS Changed document or private window blocks credential capture');}
 assert.equal(await harness({store:'unknown'}).run(),null);passed++;console.log('PASS An unverified store cannot fall back to the default profile');
 assert.equal(await harness({observed:false,crossOrigin:true}).run(),null);passed++;console.log('PASS Unobserved foreign-origin links cannot acquire cookies');
 for(const options of [{enabled:false},{permission:false}]){const h=handoffHarness(options);await h.run();const sent=h.calls.find(x=>x.action==='add');assert.equal(h.captures(),0);assert(!sent.browserSession&&!sent.cookies&&!sent.headers.Cookie);passed++;console.log('PASS Current preference and cookie permission both gate session sharing');}
 let modern=handoffHarness();await modern.run();assert(modern.calls.find(x=>x.action==='add').browserSession);passed++;console.log('PASS A capable native app receives the managed session');
 let legacy=handoffHarness({modern:false});await legacy.run();const sent=legacy.calls.find(x=>x.action==='add');assert(!sent.browserSession&&sent.cookies==='session=secret');passed++;console.log('PASS Older native versions receive the compatible static header');
 for(const options of [{navigate:true},{revoke:true}]){const h=handoffHarness(options);await assert.rejects(h.run(),/changed|disabled/);assert(!h.calls.some(x=>x.action==='add'));passed++;console.log('PASS Navigation or revoked consent before submission prevents handoff');}
 console.log(passed+' passed, 0 failed');
})().catch(e=>{console.error(e);process.exitCode=1;});
