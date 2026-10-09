'use strict';
const assert=require('node:assert/strict');let passed=0,failed=0;
for(const family of ['chromium','firefox']){
 const M=require('../browser/'+family+'/media.js'),base='https://media.test/master.m3u8';
 function check(name,fn){try{fn();passed++;console.log('PASS '+family+' '+name);}catch(e){failed++;console.error('FAIL '+family+' '+name+': '+e.message);}}
 const plan=n=>({type:'hls',tracks:[{kind:'video',segments:Array.from({length:n},(_,i)=>({url:'https://media.test/'+i+'.m4s'}))}]});
 check('HLS preserves all 2500 segments',()=>{const p=M.hls('#EXTM3U\n'+Array.from({length:2500},(_,i)=>'#EXTINF:2,\n'+i+'.ts\n').join('')+'#EXT-X-ENDLIST\n',base);assert.equal(p.segments.length,2500);});
 check('DASH preserves 2500 media segments and initialization',()=>{const p=M.dash('<MPD type="static" mediaPresentationDuration="PT5000S"><Period><AdaptationSet mimeType="video/mp4"><SegmentTemplate duration="2" initialization="init.mp4" media="part-$Number$.m4s"/><Representation id="v" height="360"/></AdaptationSet></Period></MPD>',base);assert.equal(p[0].plan.tracks[0].segments.length,2501);});
 check('Combined selection accepts exactly 10000 parts',()=>M.validatePlanBudget(plan(10000)));
 check('Combined tracks cannot exceed 10000 parts',()=>{const p=plan(6000);p.tracks.push({kind:'audio',segments:plan(4001).tracks[0].segments});assert.throws(()=>M.validatePlanBudget(p),/segment limit/);});
 check('Four MiB budget counts UTF-8 bytes',()=>{const p=plan(1);p.description='é'.repeat(2200000);assert(JSON.stringify(p).length<4*1024*1024);assert.throws(()=>M.validatePlanBudget(p),/handoff/);});
 check('Serialized plan above old 200 KB limit is accepted',()=>{const p=plan(2500);for(const part of p.tracks[0].segments)part.url+='?token='+'x'.repeat(128);assert(Buffer.byteLength(JSON.stringify(p))>200000);M.validatePlanBudget(p);});
 check('Per-playlist bound remains enforced',()=>assert.throws(()=>M.hls('#EXTM3U\n'+'#EXTINF:2,\np.ts\n'.repeat(10001)+'#EXT-X-ENDLIST\n',base),/segment limit/));
}
console.log(JSON.stringify({passed,failed}));process.exitCode=failed?1:0;
