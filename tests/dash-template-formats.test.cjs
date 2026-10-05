'use strict';
const assert=require('node:assert/strict');let passed=0;
for(const family of ['chromium','firefox']){
 const M=require('../browser/'+family+'/media.js');
 function urls(pattern,bandwidth='1234',id='video'){
  return M.dash('<MPD mediaPresentationDuration="PT4S"><Period><AdaptationSet mimeType="video/mp4"><SegmentTemplate duration="2" startNumber="15" initialization="init.mp4" media="'+pattern+'"/><Representation id="'+id+'" height="720" bandwidth="'+bandwidth+'"/></AdaptationSet></Period></MPD>','https://fixture.test/manifest.mpd')[0].plan.tracks[0].segments.map(s=>new URL(s.url).pathname);
 }
 function test(name,fn){fn();passed++;console.log('PASS '+family+' '+name);}
 for(const [format,expected] of [['d','0015'],['i','0015'],['u','0015'],['o','0017'],['x','000f'],['X','000F']])test(format+' numeric format',()=>assert.equal(urls('$Number%04'+format+'$.m4s')[1],'/'+expected+'.m4s'));
 test('hexadecimal crosses digit boundary',()=>assert.equal(urls('$Number%02x$.m4s')[2],'/10.m4s'));
 test('bandwidth hexadecimal and zero time',()=>assert.equal(urls('$Bandwidth%04X$-$Time%02u$.m4s')[1],'/04D2-00.m4s'));
 test('padding does not truncate',()=>assert.equal(urls('$Bandwidth%02o$.m4s')[1],'/2322.m4s'));
 test('escaped format-like text stays literal',()=>assert.equal(urls('$$Number%04X$$-$Number%04X$.m4s')[1],'/$Number%04X$-000F.m4s'));
 test('noninteger numeric format rejected',()=>assert.throws(()=>urls('$Bandwidth%04x$.m4s','1.5'),/numeric template/));
 test('unsafe numeric format rejected',()=>assert.throws(()=>urls('$Bandwidth%04x$.m4s','9007199254740993'),/numeric template/));
 test('excessive width rejected',()=>assert.throws(()=>urls('$Number%099x$.m4s'),/width/));
}
console.log(passed+' passed, 0 failed');
