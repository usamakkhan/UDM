'use strict';
const assert=require('node:assert/strict'),Context=require('../browser/chromium/request-context.js');let passed=0;
const event={requestId:'a',url:'https://download.test/file.zip',method:'GET',tabId:1,frameId:0,documentId:'doc',documentUrl:'https://download.test/'};
function fixture(proxyInfo,firefox=true){const c=Context.create(firefox?{runtime:{getBrowserInfo(){}}}:{});c.begin(event);c.finish({...event,statusCode:200,proxyInfo});return c;}
const result=c=>c.resolve({url:event.url,browserDownload:true});
function test(name,fn){fn();console.log('PASS '+name);passed++;}
test('Firefox captures an HTTP proxy without unrelated metadata',()=>{const r=result(fixture({type:'http',host:'proxy.test',port:8080,proxyAuthorizationHeader:'private'}));assert.deepEqual(r.proxy,{url:event.url,type:'http',host:'proxy.test',port:8080});assert(!JSON.stringify(r).includes('private'));});
test('Firefox absence of proxyInfo explicitly records a direct route',()=>assert.equal(result(fixture()).proxy.type,'direct'));
test('Chromium absence of proxyInfo is unknown, never guessed to be direct',()=>assert.equal(result(fixture(undefined,false)).proxy,undefined));
test('SOCKS5 and SOCKS4 preserve remote DNS',()=>{for(const type of ['socks','socks4'])assert.equal(result(fixture({type,host:'[::1]',port:1080,proxyDNS:true})).proxy.proxyDNS,true);});
test('Unsupported encrypted proxies and local-DNS SOCKS stay browser-owned',()=>{for(const p of [{type:'https',host:'proxy.test',port:443},{type:'quic',host:'proxy.test',port:443},{type:'socks',host:'proxy.test',port:1080,proxyDNS:false}])assert(result(fixture(p)).proxy.unsupported);});
test('Proxy usernames cannot imply available passwords or get copied',()=>{const r=result(fixture({type:'http',host:'proxy.test',port:8080,username:'private-user'}));assert(r.proxy.unsupported);assert(!JSON.stringify(r).includes('private-user'));});
test('Proxy endpoint validation rejects invalid ports and injection',()=>{for(const p of [{host:'proxy.test',port:0},{host:'proxy.test',port:65536},{host:'proxy.test',port:'8080'},{host:'a\r\nb',port:8080},{host:'user@proxy.test',port:8080}])assert(result(fixture({type:'http',...p})).proxy.unsupported);});
test('Ambiguous routes for the same live document are rejected',()=>{const c=fixture({type:'http',host:'one.test',port:8080});c.begin({...event,requestId:'b'});c.finish({...event,requestId:'b',statusCode:200,proxyInfo:{type:'http',host:'two.test',port:8080}});assert(result(c).proxy.unsupported);});
test('Redirect continuation captures the final request route',()=>{const c=fixture({type:'http',host:'one.test',port:8080});const e={...event,url:'https://download.test/final.zip'};c.begin(e);c.finish({...e,statusCode:200,proxyInfo:{type:'http',host:'two.test',port:8080}});const r=c.resolve({url:e.url});assert.equal(r.proxy.host,'two.test');assert.equal(r.proxy.url,e.url);});
test('Navigation clears ephemeral routing context',()=>{const c=fixture({type:'http',host:'proxy.test',port:8080});c.clear(1);assert.equal(result(c),null);});
test('Firefox cross-site navigation binds the download to its originUrl page',()=>{const c=Context.create({runtime:{getBrowserInfo(){}}}),e={...event,documentUrl:undefined,originUrl:'https://source.test/page'};c.begin(e);c.headers({...e,requestHeaders:[{name:'Referer',value:'https://source.test/'}]});c.finish({...e,statusCode:200,proxyInfo:{type:'http',host:'proxy.test',port:8080}});assert.equal(c.resolve({url:e.url,referrer:e.originUrl}).proxy.host,'proxy.test');});
console.log(passed+' browser proxy checks passed');
