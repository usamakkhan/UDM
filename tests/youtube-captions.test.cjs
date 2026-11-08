'use strict';
const assert=require('node:assert/strict');
const path=require('node:path');
const id='Q3TI27IN7X0';
const endpoint='https://www.youtube.com/api/timedtext';
const address=endpoint+'?v='+id+'&lang=en&signature=fixture%2Bsignature&sparams=v%2Clang';
const manual={id:'.en',language:'en',name:'English',auto:false,url:address};
const generated={...manual,id:'a.en',auto:true,url:address+'&kind=asr'};
let checks=0;
for(const family of ['chromium','firefox']){
 const {UdmFormats}=require(path.join(__dirname,'../browser',family,'formats.js'));
 const list=(tracks,requests=[])=>UdmFormats.captionChoices({videoId:id,captionTracks:tracks,captionRequests:requests},id);
 const check=(label,run)=>{run();checks++;console.log('PASS '+family+' '+label);};
 check('manual and ASR tracks have distinct TTML entries',()=>{
  const choices=list([manual,generated]);
  assert.deepEqual(choices.map(choice=>choice.key),['caption:.en','caption:a.en']);
  assert.match(choices[1].label,/auto-generated/);
  assert.equal(choices[0].url,address+'&fmt=ttml');
 });
 check('matched observed address preserves session and signed parameters',()=>{
  const observed=address+'&fmt=json3&pot=fixture-session&potc=1&c=WEB';
  const choices=list([manual,generated],[observed]);
  assert.equal(choices[0].url,observed.replace('fmt=json3','fmt=ttml'));
  assert.equal(new URL(choices[1].url).searchParams.has('pot'),false);
 });
 check('different video, language or signature cannot contribute a session',()=>{
  const requests=[
   address.replace(id,'aaaaaaaaaaa')+'&pot=wrong-video',
   address.replace('lang=en','lang=fr')+'&pot=wrong-language',
   address.replace('fixture%2Bsignature','other')+'&pot=wrong-signature',
  ];
  assert.equal(list([manual],requests)[0].url,address+'&fmt=ttml');
 });
 check('signed format and expired or translated tracks are rejected',()=>{
  assert.deepEqual(list([
   {...manual,url:address.replace('v%2Clang','v%2Clang%2Cfmt')+'&fmt=json3'},
   {...manual,url:address+'&expire=1'},
   {...manual,url:address+'&tlang=fr'},
  ]),[]);
 });
 check('relative YouTube caption URLs resolve and identical rows deduplicate',()=>{
  const entries=list([{...manual,url:address.slice('https://www.youtube.com'.length)},manual]);
  assert.equal(entries.length,1);
  assert.equal(entries[0].url,address+'&fmt=ttml');
 });
}
console.log('ALL '+checks+' YOUTUBE CAPTION CHECKS PASSED');
