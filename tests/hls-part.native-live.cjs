'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),{execFileSync}=require('node:child_process');
module.exports=async function({page,session,clickAX,until,jobs,pass,project}){
 const before=new Set(jobs().map(job=>job.Id));
 await page.goto('http://127.0.0.1:43821/part-hls');
 await page.waitForFunction(()=>document.querySelector('video')?.videoHeight===360);
 await until(()=>page.locator('[id^="udm-video-panel-"]').count());
 await clickAX(session,name=>name==='Download this video with UDM');
 const chosen=await clickAX(session,name=>name.includes('MP4 · 360p'));
 const job=await until(()=>{const current=jobs().find(item=>!before.has(item.Id));if(current?.Status==='Failed')throw Error(current.Error);return current?.Status==='Complete'&&current;},60000);
 const output=path.join(job.Folder,job.FileName),tools=path.join(project,'release/tools');
 const probe=JSON.parse(execFileSync(path.join(tools,'ffprobe.exe'),['-v','error','-show_entries','stream=codec_type,height:format=duration','-of','json',output],{windowsHide:true}));
 assert(probe.streams.some(stream=>stream.codec_type==='video'&&stream.height===360));assert(probe.streams.some(stream=>stream.codec_type==='audio'));assert(Math.abs(Number(probe.format.duration)-8)<0.25);
 execFileSync(path.join(tools,'ffmpeg.exe'),['-v','error','-i',output,'-f','null','-'],{windowsHide:true});
 const stats=await (await fetch('http://127.0.0.1:43821/part-stats')).json();assert.equal(stats.missingPartRequests,0);
 const digest=crypto.createHash('sha256').update(fs.readFileSync(output)).digest('hex');assert.equal(job.Sha256.toLowerCase(),digest);
 pass('Recorded HLS downloads complete segments once and ignores duplicate partial URLs',{chosen,segments:job.AdaptiveCompletedSegments,missingPartRequests:stats.missingPartRequests,sha256:digest});
};
