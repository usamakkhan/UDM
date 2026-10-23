document.querySelector('#video-test').onclick=async()=>{
 const checks=[],check=(ok,name)=>{checks.push({name,passed:!!ok});if(!ok)throw Error(name);};
 const wait=(element,event,ready)=>new Promise((resolve,reject)=>{
  if(ready())return resolve();
  const timeout=setTimeout(()=>finish(Error(event+' timed out')),8000);
  const done=()=>finish(),failed=()=>finish(Error(event+' failed'));
  function finish(error){clearTimeout(timeout);element.removeEventListener(event,done);element.removeEventListener('error',failed);error?reject(error):resolve();}
  element.addEventListener(event,done,{once:true});element.addEventListener('error',failed,{once:true});
 });
 try{
  const video=document.querySelector('iframe').contentDocument.querySelector('#saved-video');
  check(!!video,'Archived video exists');
  video.currentTime=0;
  const track=video.querySelector('track');track.track.mode='hidden';
  const ended=wait(video,'ended',()=>video.ended);
  // A play/load failure can leave the independent completion timer pending.
  ended.catch(()=>{});
  await video.play();
  await wait(track,'load',()=>track.readyState===2);
  check(video.videoWidth===160&&video.videoHeight===90,'Archived video dimensions match');
  check(Math.abs(video.duration-2)<0.1,'Archived video duration matches');
  check(track.track.cues.length===1&&track.track.cues[0].text==='Saved offline caption','Video caption cue loads locally');
  await ended;
  check(video.ended&&video.currentTime>1.9,'Archived video plays to completion');
  check(video.getVideoPlaybackQuality().totalVideoFrames>=19,'Archived video frames decoded');
 }catch(error){checks.push({name:error.message,passed:false});}
 document.querySelector('#video-results').textContent=JSON.stringify({passed:checks.every(x=>x.passed),checks},null,2);
};
