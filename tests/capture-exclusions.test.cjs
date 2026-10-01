'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),os=require('node:os'),{execFileSync}=require('node:child_process');let passed=0;
function test(name,fn){fn();passed++;console.log('PASS '+name);}
for(const family of ['chromium','firefox']){
 const M=require('../browser/'+family+'/media.js');
 test(family+' exact exclusions treat literal asterisks and query parameters literally',()=>{const p={excludedUrls:['=https://cdn.example.test/file.zip?q=*&token=example']};assert(M.policy.blocked('https://cdn.example.test/file.zip?q=*&token=example#part',p));for(const u of ['https://cdn.example.test/file.zip?q=other&token=example','https://cdn.example.test/file.zip?q=*&token=changed','https://cdn.example.test/other.zip?q=*&token=example','http://cdn.example.test/file.zip?q=*&token=example','https://cdn.example.test.evil/file.zip?q=*&token=example'])assert(!M.policy.blocked(u,p));});
 test(family+' exact exclusions normalize URL scheme, hostname, default port and fragment',()=>{assert(M.policy.blocked('https://EXAMPLE.test/file.zip#new',{excludedUrls:['=HTTPS://example.test:443/file.zip#old']}));assert(!M.policy.blocked('https://example.test:8443/file.zip',{excludedUrls:['=https://example.test/file.zip']}));});
 test(family+' exact exclusions preserve path case and encoding',()=>{const p={excludedUrls:['=https://example.test/File%20Name.zip']};assert(M.policy.blocked('https://example.test/File%20Name.zip',p));assert(!M.policy.blocked('https://example.test/file%20name.zip',p));});
 test(family+' existing wildcard patterns and site exceptions retain their scope',()=>{const p={excludedUrls:['https://*.example.test/private/*']};assert(M.policy.blocked('https://sub.example.test/private/a',p));assert(!M.policy.blocked('https://sub.example.test/public/a',p));assert(!M.policy.blocked('https://example.test.evil/private/a',p));assert(M.policy.blocked('http://sub.example.test:8888/file.zip',{excluded:['example.test']}));});
 test(family+' IPv6 exact, site and wildcard address exclusions work',()=>{assert(M.policy.blocked('https://[::1]:8443/a.zip?x=*',{excludedUrls:['=https://[::1]:8443/a.zip?x=*']}));assert(!M.policy.blocked('https://[::1]:8443/a.zip?x=z',{excludedUrls:['=https://[::1]:8443/a.zip?x=*']}));assert(M.policy.blocked('https://[::1]:8443/a.zip',{excluded:['[::1]']}));assert(M.policy.blocked('https://[::1]:8443/a.zip',{excludedUrls:['https://[::1]:8443/*']}));});
 test(family+' long exact addresses are supported without widening malformed inputs',()=>{const u='https://long.test/a?token='+'a'.repeat(3000);assert(M.policy.blocked(u,{excludedUrls:['='+u]}));for(const bad of ['=not-a-url','=ftp://example.test/file','=https://user:pass@example.test/file','='+'x'.repeat(17000)])assert(!M.policy.blocked('https://example.test/file',{excludedUrls:[bad]}));});
 test(family+' desktop exclusions merge with local settings',()=>{const p=M.policy.merge({capture:true,excludedUrls:['https://local.test/*']},{excludedUrls:['=https://cdn.test/a.zip']});assert(M.policy.blocked('https://local.test/a',p));assert(M.policy.blocked('https://cdn.test/a.zip',p));assert(!M.policy.blocked('https://cdn.test/b.zip',p));});
}
test('Actual browser generation includes every canonical dependency in Firefox',()=>{
 const root=path.join(__dirname,'../browser'),fixture=fs.mkdtempSync(path.join(os.tmpdir(),'udm-exclusion-generator-'));
 for(const family of ['chromium','firefox'])fs.cpSync(path.join(root,family),path.join(fixture,family),{recursive:true});
 execFileSync(process.execPath,[path.join(root,'prepare.cjs'),fixture],{windowsHide:true});
 const worker=fs.readFileSync(path.join(root,'chromium/background.js'),'utf8'),imports=[.../importScripts\(([^)]+)\)/.exec(worker)[1].matchAll(/'([^']+\.js)'/g)].map(x=>x[1]);
 const scripts=JSON.parse(fs.readFileSync(path.join(fixture,'firefox/manifest.json'))).background.scripts;assert.deepEqual(scripts,[...imports,'background.js']);assert(scripts.indexOf('multipart.js')<scripts.indexOf('request-context.js'));
});
if(process.argv[2]){
 const policy=JSON.parse(fs.readFileSync(process.argv[2]));
 for(const family of ['chromium','firefox'])test(family+' consumes actual native-generated exclusion preferences',()=>{const M=require('../browser/'+family+'/media.js');assert(M.policy.blocked('https://cdn.example.test/second.zip?literal=*&token=example',policy.exact));assert(!M.policy.blocked('https://cdn.example.test/second.zip?literal=anything&token=example',policy.exact));assert(!M.policy.blocked('https://cdn.example.test/other.zip',policy.exact));assert(M.policy.blocked('http://sub.cdn.example.test:8080/other.zip',policy.site));assert(!M.policy.blocked('https://page.example.test/other.zip',policy.site));assert(M.policy.blocked('https://[::1]:8443/other.zip',policy.ipv6));});
}
console.log(passed+' passed, 0 failed');
