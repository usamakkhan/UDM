'use strict';
const assert=require('node:assert/strict');let passed=0;
for(const family of ['chromium','firefox']){
 const M=require('../browser/'+family+'/media.js');
 const parse=(period='',template='duration="2"',timeline='',total='PT12S')=>M.dash(`<MPD mediaPresentationDuration="${total}"><Period ${period}><AdaptationSet mimeType="video/mp4"><SegmentTemplate ${template} initialization="init.mp4" media="$Time$-$Number$.m4s">${timeline}</SegmentTemplate><Representation id="v" height="360" bandwidth="1000"/></AdaptationSet></Period></MPD>`,'https://fixture.test/main.mpd')[0].plan.tracks[0].segments.map(s=>new URL(s.url).pathname);
 function test(name,fn){fn();passed++;console.log('PASS '+family+' '+name);}
 test('nonzero start subtracts from total presentation duration',()=>assert.equal(parse('start="PT8S"').length,3));
 test('zero/default start retains full duration',()=>assert.equal(parse().length,7));
 test('explicit period duration remains authoritative',()=>assert.equal(parse('start="PT8S" duration="PT2S"').length,2));
 test('negative repeat ends at period-local duration',()=>assert.deepEqual(parse('start="PT8S"','timescale="1"','<SegmentTimeline><S t="0" d="2" r="-1"/></SegmentTimeline>'),['/init.mp4','/0-1.m4s','/2-2.m4s']));
 test('presentation offset is added to period-local boundary',()=>assert.deepEqual(parse('start="PT8S"','timescale="1" presentationTimeOffset="10"','<SegmentTimeline><S t="10" d="2" r="-1"/></SegmentTimeline>'),['/init.mp4','/10-1.m4s','/12-2.m4s']));
 test('period beyond presentation does not fabricate duration segments',()=>assert.throws(()=>parse('start="PT14S"'),/duration/));
}
console.log(passed+' passed, 0 failed');