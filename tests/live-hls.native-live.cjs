'use strict';
const assert=require('node:assert/strict'),path=require('node:path'),fs=require('node:fs'),crypto=require('node:crypto'),{execFileSync}=require('node:child_process');
module.exports=async function({page,session,clickAX,until,jobs,pass,project,root}){
 const before=new Set(jobs().map(job=>job.Id));await page.goto('http://127.0.0.1:43821/live-hls');await page.waitForFunction(()=>document.querySelector('video')?.videoHeight===360);await until(()=>page.locator('[id^="udm-video-panel-"]').count());
 await clickAX(session,name=>name==='Download this video with UDM');const chosen=await clickAX(session,name=>name.includes('MP4 · 360p'));await page.screenshot({path:path.join(root,'live-hls-panel.png')});
 const job=await until(()=>{const current=jobs().find(item=>!before.has(item.Id));if(current?.Status==='Failed')throw Error(current.Error);return current?.Status==='Complete'&&current;},60000);
 assert.equal(job.LiveRecording,true);assert(job.LiveCapturedSeconds>=7.9&&job.LiveCapturedSeconds<8.2);const output=path.join(job.Folder,job.FileName),tools=path.join(project,'release/tools');
 const probe=JSON.parse(execFileSync(path.join(tools,'ffprobe.exe'),['-v','error','-show_entries','stream=codec_type,height:format=duration','-of','json',output],{windowsHide:true}));assert(probe.streams.some(stream=>stream.codec_type==='video'&&stream.height===360));assert(probe.streams.some(stream=>stream.codec_type==='audio'));assert(Math.abs(Number(probe.format.duration)-8)<0.25);
 execFileSync(path.join(tools,'ffmpeg.exe'),['-v','error','-i',output,'-f','null','-'],{windowsHide:true});const digest=crypto.createHash('sha256').update(fs.readFileSync(output)).digest('hex');assert.equal(job.Sha256.toLowerCase(),digest);
 const stats=await (await fetch('http://127.0.0.1:43821/live-stats')).json();assert(stats.nativeLivePolls>=2);assert.equal(job.AdaptiveCompletedSegments,stats.segments);
 pass('Trusted browser capture records a changing live fMP4 playlist and publishes fully decodable video with audio',{chosen,duration:probe.format.duration,capturedSeconds:job.LiveCapturedSeconds,segments:stats.segments,nativePlaylistRefreshes:stats.nativeLivePolls,sha256:digest});
};
