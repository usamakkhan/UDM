// Keep one stable unpacked-extension identity across rebuilds. No private signing key is stored.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const dir=path.join(__dirname,'chromium'), manifestPath=path.join(dir,'manifest.json');
const m=JSON.parse(fs.readFileSync(manifestPath));
if(!m.key)m.key=crypto.generateKeyPairSync('rsa',{modulusLength:2048}).publicKey.export({type:'spki',format:'der'}).toString('base64');
m.version='0.5.0';m.description='Send files and YouTube video with audio to UDM. Includes a player button and quality selection.';
m.host_permissions=['https://*.youtube.com/*','https://youtube.com/*','https://*.googlevideo.com/*'];
m.content_scripts=[{matches:['https://www.youtube.com/*','https://m.youtube.com/*'],js:['ump.js','capture.js'],world:'MAIN',run_at:'document_start'},{matches:['https://www.youtube.com/*','https://m.youtube.com/*'],js:['content.js'],run_at:'document_idle'}];
fs.writeFileSync(manifestPath,JSON.stringify(m,null,2)+'\n');
const id=crypto.createHash('sha256').update(Buffer.from(m.key,'base64')).digest('hex').slice(0,32).replace(/[0-9a-f]/g,c=>String.fromCharCode(97+parseInt(c,16)));
fs.writeFileSync(path.join(__dirname,'extension-id.txt'),id+'\n');
const f={...m,background:{scripts:['formats.js','background.js']},browser_specific_settings:{gecko:{id:'udm@local.example',strict_min_version:'128.0'}}};delete f.key;
fs.writeFileSync(path.join(__dirname,'firefox','manifest.json'),JSON.stringify(f,null,2)+'\n');
for(const name of ['ump.js','capture.js','formats.js','background.js','content.js','popup.html','popup.js','popup.css','icon.png'])fs.copyFileSync(path.join(dir,name),path.join(__dirname,'firefox',name));
console.log('UDM extension '+id);
