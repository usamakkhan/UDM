// Chromium is the canonical configuration. Preserve its capabilities and identity.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const root=process.argv[2]?path.resolve(process.argv[2]):__dirname;
const dir=path.join(root,'chromium'),manifestPath=path.join(dir,'manifest.json');
const m=JSON.parse(fs.readFileSync(manifestPath));
if(!m.key){m.key=crypto.generateKeyPairSync('rsa',{modulusLength:2048}).publicKey.export({type:'spki',format:'der'}).toString('base64');fs.writeFileSync(manifestPath,JSON.stringify(m,null,2)+'\n');}
const id=crypto.createHash('sha256').update(Buffer.from(m.key,'base64')).digest('hex').slice(0,32).replace(/[0-9a-f]/g,c=>String.fromCharCode(97+parseInt(c,16)));
fs.writeFileSync(path.join(root,'extension-id.txt'),id+'\n');
const firefox=path.join(root,'firefox');fs.mkdirSync(firefox,{recursive:true});
const priorPath=path.join(firefox,'manifest.json'),prior=fs.existsSync(priorPath)?JSON.parse(fs.readFileSync(priorPath)):{};
const f={...m,background:{scripts:['formats.js','media.js','native-bridge.js','capture-recovery.js','navigation.js','grabber-session.js','download-session.js','chromium-proxy.js','request-context.js','browser-controls.js','key-capture.js','sites.js','ump.js','streaming-capture.js','background.js']},browser_specific_settings:prior.browser_specific_settings||{gecko:{id:'udm@local.example',strict_min_version:'128.0'}}};delete f.key;
fs.writeFileSync(priorPath,JSON.stringify(f,null,2)+'\n');
for(const name of fs.readdirSync(dir)){if(name==='manifest.json')continue;const source=path.join(dir,name);if(fs.statSync(source).isFile())fs.copyFileSync(source,path.join(firefox,name));}
console.log('Prepared UDM '+m.version+' extension '+id);
