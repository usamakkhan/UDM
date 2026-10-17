'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
async function run(variant){
 let timeout,reply,reads=0;const box={URL,setTimeout(fn,ms){assert.equal(ms,5000);timeout=fn;return 1;},clearTimeout(){}};
 vm.createContext(box);vm.runInContext(fs.readFileSync('browser/'+variant+'/chromium-proxy.js','utf8'),box);
 const api={runtime:{},proxy:{settings:{get(options,cb){reads++;reply=cb;}}}};
 const client=box.UdmChromiumProxy.create(api),url='https://fixture.test/file.zip';
 const first=client.observe(url),late=reply;timeout();await first.ready;assert(client.resolve(first).unsupported);
 const second=client.observe(url);assert.equal(reads,2);reply({value:{mode:'direct'}});await second.ready;
 late({value:{mode:'fixed_servers',rules:{singleProxy:{host:'stale.test'}}}});
 const cached=client.observe(url);await cached.ready;assert.equal(reads,2);assert.equal(client.resolve(cached).type,'direct');
 console.log('PASS '+variant+' timeout does not poison cache and late reply cannot replace a recovered route');
 const failed=box.UdmChromiumProxy.create(api);let observation=failed.observe(url);api.runtime.lastError={message:'Temporarily unavailable'};reply({value:{mode:'direct'}});delete api.runtime.lastError;await observation.ready;assert(failed.resolve(observation).unsupported);
 observation=failed.observe(url);assert.equal(reads,4);reply({value:{mode:'system'}});await observation.ready;assert.equal(failed.resolve(observation),undefined);
 console.log('PASS '+variant+' API errors permit fresh reads without guessing a direct route');
}
(async()=>{for(const variant of ['chromium','firefox'])await run(variant);})().catch(error=>{console.error(error);process.exitCode=1;});
