'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),{execFileSync}=require('node:child_process');
module.exports=async function({page,session,clickAX,until,jobs,pass,project}){
 const before=new Set(jobs().map(job=>job.Id));
 await page.goto('http://127.0.0.1:43821/gap-hls');
 await page.waitForFunction(()=>document.querySelector('video')?.videoHeight===360);
 await until(()=>page.locator('[id^="udm-video-panel-"]').count());
 await clickAX(session,name=>name==='Download this video with UDM');
 const chosen=await clickAX(session,name=>name.includes('MP4 · 360p'));
 const job=await until(()=>{const current=jobs().find(item=>!before.has(item.Id));if(current?.Status==='Failed')throw Error(current.Error);return current?.Status==='Complete'&&current;},60000);
 const output=path.join(job.Folder,job.FileName),tools=path.join(project,'release/tools');
 const probe=JSON.parse(execFileSync(path.join(tools,'ffprobe.exe'),['-v','error','-show_entries','stream=codec_type,height','-of','json',output],{windowsHide:true}));
 assert(probe.streams.some(stream=>stream.codec_type==='video'&&stream.height===360));assert(probe.streams.some(stream=>stream.codec_type==='audio'));
 execFileSync(path.join(tools,'ffmpeg.exe'),['-v','error','-i',output,'-f','null','-'],{windowsHide:true});
 const digest=crypto.createHash('sha256').update(fs.readFileSync(output)).digest('hex');assert.equal(job.Sha256.toLowerCase(),digest);
 const stats=await (await fetch('http://127.0.0.1:43821/gap-stats')).json();assert.equal(stats.missingGapRequests,0);assert.equal(job.AdaptiveCompletedSegments,4);
 pass('Recorded HLS skips an unavailable gap segment and publishes decodable video',{chosen,segments:job.AdaptiveCompletedSegments,missingGapRequests:stats.missingGapRequests,sha256:digest});
};
