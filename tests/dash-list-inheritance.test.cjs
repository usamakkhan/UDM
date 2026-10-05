'use strict';
const assert=require('node:assert/strict');let passed=0;
for(const family of ['chromium','firefox']){
 const M=require('../browser/'+family+'/media.js'),base='https://fixture.test/root/manifest.mpd';
 function segments(parent,representation,period=''){return M.dash('<MPD><Period>'+period+'<AdaptationSet mimeType="video/mp4">'+parent+'<Representation id="v" height="720"><BaseURL>video/</BaseURL>'+representation+'</Representation></AdaptationSet></Period></MPD>',base)[0].plan.tracks[0].segments;}
 function test(name,fn){fn();passed++;console.log('PASS '+family+' '+name);}
 test('child media list inherits parent initialization',()=>{const s=segments('<SegmentList><Initialization sourceURL="init.mp4"/><SegmentURL media="parent.m4s"/></SegmentList>','<SegmentList><SegmentURL media="child.m4s"/></SegmentList>');assert.deepEqual(s.map(x=>x.url),['https://fixture.test/root/video/init.mp4','https://fixture.test/root/video/child.m4s']);});
 test('child initialization overrides parent while inheriting media entries',()=>{const s=segments('<SegmentList><Initialization sourceURL="old.mp4"/><SegmentURL media="one.m4s"/><SegmentURL media="two.m4s"/></SegmentList>','<SegmentList><Initialization sourceURL="new.mp4"/></SegmentList>');assert.deepEqual(s.map(x=>x.url.split('/').pop()),['new.mp4','one.m4s','two.m4s']);});
 test('three levels preserve initialization ranges and replace media ranges',()=>{const s=segments('<SegmentList><SegmentURL media="file.mp4" mediaRange="10-19"/></SegmentList>','<SegmentList><SegmentURL media="file.mp4" mediaRange="20-39"/></SegmentList>','<SegmentList><Initialization sourceURL="file.mp4" range="0-9"/></SegmentList>');assert.deepEqual(s.map(x=>[x.start,x.length]),[[0,10],[20,20]]);});
 test('empty child list inherits complete parent list',()=>{const s=segments('<SegmentList><Initialization sourceURL="i.mp4"/><SegmentURL media="s.m4s"/></SegmentList>','<SegmentList/>');assert.equal(s.length,2);});
 test('multiple lists at one level are rejected',()=>assert.throws(()=>segments('<SegmentList><SegmentURL media="one"/></SegmentList><SegmentList><SegmentURL media="two"/></SegmentList>',''),/ambiguous|Multiple/i));
 test('multiple initialization elements are rejected',()=>assert.throws(()=>segments('<SegmentList><Initialization sourceURL="a"/><Initialization sourceURL="b"/><SegmentURL media="s"/></SegmentList>',''),/ambiguous|Multiple/i));
}
console.log(passed+' passed, 0 failed');
