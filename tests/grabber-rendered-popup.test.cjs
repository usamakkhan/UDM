const assert=require('node:assert/strict');
(async()=>{for(const browser of ['chromium','firefox']){
 const html=require('node:fs').readFileSync(require('node:path').join(__dirname,'../browser',browser,'popup.html'),'utf8');
 const ids=[...html.matchAll(/\bid="([^"]+)"/g)].map(m=>m[1]);assert.equal(new Set(ids).size,ids.length,'Popup element IDs must be unique');
 const fs=require('node:fs'),path=require('node:path'),base=path.join(__dirname,'../browser',browser);
 const manifest=JSON.parse(fs.readFileSync(path.join(base,'manifest.json'),'utf8'));
 const background=fs.readFileSync(path.join(base,'background.js'),'utf8');
 if(browser==='firefox')assert(manifest.background.scripts.includes('grabber-rendered.js'));
 else assert(background.split('\n')[1].includes("'grabber-rendered.js'"));
 for(const script of manifest.background.scripts||[manifest.background.service_worker])assert(fs.existsSync(path.join(base,script)));
 assert(background.includes("message?.action==='grabber-rendered-capture'"));
 const {create}=require('../browser/'+browser+'/grabber-rendered.js');
 const calls=[],id='a'.repeat(32),url='https://example.test/page';let tab={url,status:'complete'},matches=true;
 const api={runtime:{id:'extension',getURL:p=>'chrome-extension://extension/'+p},tabs:{get:async()=>tab}};
 const sender={id:'extension',url:api.runtime.getURL('popup.html')};
 const collector=create(api,null,async message=>{calls.push(message);if(message.action==='grabber-rendered-projects')return {ok:true,projects:matches?[{projectId:id,name:'Project'}]:[]};throw Error('Unexpected native request');});
 for(const invalid of [{}, {...sender,url:url},{...sender,id:'other'}])await assert.rejects(collector.projects({tabId:1},invalid));
 assert.equal(calls.length,0);
 for(const invalid of [{url,status:'loading'},{url,incognito:true},{url:'file:///C:/secret'},{url:'https://user:pass@example.test'}]){tab=invalid;await assert.rejects(collector.projects({tabId:1},sender));}
 assert.equal(calls.length,0);tab={url,status:'complete'};
 const reply=await collector.projects({tabId:1},sender);assert.equal(reply.projects[0].projectId,id);assert.deepEqual(calls.pop(),{action:'grabber-rendered-projects',url});
 matches=false;await assert.rejects(collector.complete({tabId:1,projectId:id},sender),/no longer matches/);assert.equal(calls.length,1);assert.equal(calls[0].action,'grabber-rendered-projects');
 console.log('PASS '+browser+' popup authorization, regular-page eligibility and stale project rejection');
}})().catch(e=>{console.error(e);process.exitCode=1;});
