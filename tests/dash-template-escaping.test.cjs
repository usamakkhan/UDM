'use strict';
const assert=require('node:assert/strict');let passed=0;
for(const family of ['chromium','firefox']){
 const M=require('../browser/'+family+'/media.js');
 function segments(media,initialization='init.mp4'){
  return M.dash('<MPD mediaPresentationDuration="PT4S"><Period><AdaptationSet mimeType="video/mp4"><SegmentTemplate duration="2" initialization="'+initialization+'" media="'+media+'"/><Representation id="video" height="720" bandwidth="1234"/></AdaptationSet></Period></MPD>','https://fixture.test/manifest.mpd')[0].plan.tracks[0].segments.map(s=>s.url);
 }
 function test(name,fn){fn();passed++;console.log('PASS '+family+' '+name);}
 test('escaped variable-like text remains literal',()=>assert.deepEqual(segments('$$Number$$-$Number$.m4s'),['https://fixture.test/init.mp4','https://fixture.test/$Number$-1.m4s','https://fixture.test/$Number$-2.m4s']));
 test('literal dollar prefix beside real variable',()=>assert.equal(segments('$$$RepresentationID$-$Number%03d$.m4s')[1],'https://fixture.test/$video-001.m4s'));
 test('escaped initialization filename',()=>assert.equal(segments('s-$Number$.m4s','$$RepresentationID$$.mp4')[0],'https://fixture.test/$RepresentationID$.mp4'));
 test('ordinary formatted variables retained',()=>assert.equal(segments('$RepresentationID$-$Bandwidth$-$Time$-$Number%02d$.m4s')[2],'https://fixture.test/video-1234-2-02.m4s'));
 test('unknown variable remains rejected',()=>assert.throws(()=>segments('$Unknown$.m4s'),/Unsupported DASH template/));
 test('unterminated variable rejected',()=>assert.throws(()=>segments('$Number.m4s'),/Unsupported DASH template/));
 test('unescaped trailing dollar rejected',()=>assert.throws(()=>segments('segment$.m4s'),/Unsupported DASH template/));
 test('escaped numeric filename retained',()=>assert.equal(segments('$$5-$Number$.m4s')[1],'https://fixture.test/$5-1.m4s'));
}
console.log(passed+' passed, 0 failed');
