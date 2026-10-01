'use strict';
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),{execFileSync}=require('node:child_process');
const fixture=path.resolve(process.argv[2]||'benchmarks/extension-preparation');fs.mkdirSync(fixture,{recursive:true});
fs.cpSync(path.resolve(__dirname,'../browser/chromium'),path.join(fixture,'chromium'),{recursive:true});
fs.mkdirSync(path.join(fixture,'firefox'),{recursive:true});
fs.copyFileSync(path.resolve(__dirname,'../browser/firefox/manifest.json'),path.join(fixture,'firefox/manifest.json'));
const p=path.join(fixture,'chromium/manifest.json'),m=JSON.parse(fs.readFileSync(p));m.version='99.1.2';m.optional_host_permissions.push('https://preparation-regression.invalid/*');fs.writeFileSync(p,JSON.stringify(m));
const prepare=path.resolve(__dirname,'../browser/prepare.cjs');
for(let i=0;i<2;i++){
 execFileSync(process.execPath,[prepare,fixture],{windowsHide:true});assert.deepEqual(JSON.parse(fs.readFileSync(p)),m);
 const firefox=JSON.parse(fs.readFileSync(path.join(fixture,'firefox/manifest.json')));assert.equal(firefox.version,m.version);assert.deepEqual(firefox.content_scripts,m.content_scripts);assert.deepEqual(firefox.host_permissions,m.host_permissions);assert.deepEqual(firefox.optional_host_permissions,m.optional_host_permissions);assert.ok(firefox.background.scripts.includes('key-capture.js')&&firefox.background.scripts.includes('media.js')&&firefox.background.scripts.includes('sites.js')&&firefox.background.scripts.includes('ump.js')&&firefox.background.scripts.includes('streaming-capture.js'));
 assert(firefox.permissions.includes('webNavigation'));assert(firefox.background.scripts.indexOf('navigation.js')<firefox.background.scripts.indexOf('request-context.js'));assert(firefox.background.scripts.includes('navigation.js'));
 assert(firefox.background.scripts.indexOf('file-recognition.js')<firefox.background.scripts.indexOf('request-context.js'));assert(firefox.background.scripts.includes('multipart.js'));
 for(const name of ['file-recognition.js','multipart.js','mime-db-LICENSE.txt'])assert.equal(fs.readFileSync(path.join(fixture,'chromium',name),'utf8'),fs.readFileSync(path.join(fixture,'firefox',name),'utf8'));
 for(const name of ['navigation.js','key-capture.js','content.js','media.js','sites.js','capture.js','ump.js','streaming-capture.js','background.js'])assert.equal(fs.readFileSync(path.join(fixture,'chromium',name),'utf8'),fs.readFileSync(path.join(fixture,'firefox',name),'utf8'));
 assert.equal(fs.readFileSync(path.join(fixture,'extension-id.txt'),'utf8'),fs.readFileSync(path.resolve(__dirname,'../browser/extension-id.txt'),'utf8'));
}
console.log('PASS repeated extension preparation preserves identity, version, capabilities and shared sources');
