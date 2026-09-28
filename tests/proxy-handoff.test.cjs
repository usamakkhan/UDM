'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const url='http://127.0.0.1:8080/file.zip',route={url,type:'http',host:'127.0.0.1',port:8888};let passed=0;
const pass=name=>{passed++;console.log('PASS '+name);};
function harness(preferences,proxy=route,{explicit=false,incognito=false}={}){
 const calls=[],observations=[],event={addListener(){}};
 const api={runtime:{onInstalled:event,onMessage:event,sendNativeMessage:async(_,m)=>{calls.push(m);return m.action==='preferences'?preferences:{ok:true,id:'file'};}},permissions:{contains:async()=>false},
 storage:{local:{get:async()=>({settings:{cookies:false}}),set:async()=>{}},session:{get:async()=>({}),set:async()=>{},remove:async()=>{}}},
 tabs:{get:async()=>({url:'http://127.0.0.1:8080/page',incognito}),onRemoved:event,onUpdated:event},action:{setBadgeText:async()=>{}},contextMenus:{onClicked:event},downloads:{onCreated:event},webRequest:{onHeadersReceived:event}};
 const context=vm.createContext({chrome:api,URL,URLSearchParams,navigator:{userAgent:'Fixture'},setTimeout,clearTimeout,console,
  UdmRequestContext:{create:()=>({install(){},resolve:()=>explicit?null:{method:'GET',headers:{},proxy}})},
  UdmChromiumProxy:{create:()=>({install(){},observe(address,privateMode){observations.push({address,privateMode});return {ready:Promise.resolve(),value:proxy};},resolve:ob=>ob.value})}});
 vm.runInContext(fs.readFileSync(path.join(__dirname,'../browser/chromium/background.js'),'utf8'),context);
 return {calls,observations,run:()=>context.handoff({url,tabId:7,browserDownload:!explicit})};
}
(async()=>{
 for(const preferences of [{ok:true,browserProxy:1},{ok:false,browserProxy:1,explicitProxyTransport:1},{ok:true,explicitProxyTransport:1}]){
  const h=harness(preferences);await assert.rejects(h.run(),/Update the UDM desktop/);assert(!h.calls.some(x=>x.action==='add'));pass('Incompatible desktop cannot accept an explicitly proxied request');
 }
 let h=harness({ok:true,browserProxy:1,explicitProxyTransport:1});await h.run();assert.deepEqual(JSON.parse(JSON.stringify(h.calls.find(x=>x.action==='add').browserProxy)),route);pass('Compatible desktop receives the exact captured proxy route');
 h=harness({ok:true,browserProxy:1},{url,type:'direct'});await h.run();assert(h.calls.some(x=>x.action==='add'));pass('Direct routes remain compatible with earlier desktop builds');
 h=harness({ok:true,browserProxy:1,explicitProxyTransport:1},{unsupported:true});await assert.rejects(h.run(),/cannot reproduce/);assert(!h.calls.some(x=>x.action==='add'));pass('Unsupported browser routing cannot create a native download');
 for(const incognito of [false,true]){h=harness({ok:true,browserProxy:1,explicitProxyTransport:1},route,{explicit:true,incognito});await h.run();assert.deepEqual(h.observations,[{address:url,privateMode:incognito}]);pass('Explicit link command reads the '+(incognito?'InPrivate':'regular')+' tab configuration');}
 for(const proxy of [{...route,type:'https'},{...route,type:'socks4',proxyDNS:false},{...route,type:'socks5',proxyDNS:false}]){
  h=harness({ok:true,browserProxy:1,explicitProxyTransport:1},proxy);await assert.rejects(h.run(),/Update the UDM desktop/);assert(!h.calls.some(x=>x.action==='add'));pass('Old native transport cannot accept '+proxy.type+' with extended routing');
  h=harness({ok:true,browserProxy:1,explicitProxyTransport:1,browserProxyTypes:2},proxy);await h.run();assert.deepEqual(JSON.parse(JSON.stringify(h.calls.find(x=>x.action==='add').browserProxy)),proxy);pass('Extended native transport receives '+proxy.type+' with exact DNS and encryption semantics');
 }
 console.log('ALL '+passed+' PROXY HANDOFF CHECKS PASSED');
})().catch(e=>{console.error(e);process.exitCode=1;});
