'use strict';
// Monitor an actual browser-created job. Never submit page-only media jobs.
const fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'..');
const jobId=process.argv[2],reportName=process.argv[3]||'udm-capture-benchmark.json';
if(!/^[a-f0-9]{32}$/.test(jobId||''))throw Error('Pass the ID of a job created by the browser panel.');
if(!/^udm-[a-z0-9-]+\.json$/.test(reportName))throw Error('Use a simple udm-*.json report name.');
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
(async()=>{
 const reply={id:jobId};console.log('Watching browser job '+reply.id);
 const stateFile=path.join(process.env.LOCALAPPDATA,'UDM','state.json'),samples=[];let last='',job;
 for(let i=0;i<240;i++){
  await sleep(1000);let state;try{state=JSON.parse(fs.readFileSync(stateFile,'utf8'));}catch{continue;}
  job=state.Downloads.find(j=>j.Id===reply.id);if(!job)continue;
  const line=job.Status+' | '+(job.Received/1048576).toFixed(2)+' MiB';if(line!==last){console.log(line);last=line;}
  samples.push({observedUtc:new Date().toISOString(),status:job.Status,received:job.Received});
  // The completion record is saved before the final stopwatch/manager save.
  if(['Failed','Paused'].includes(job.Status)||job.Status==='Complete'&&job.ElapsedSeconds>0)break;
 }
 if(!job||job.Status!=='Complete')throw Error(job?.Error||'Live download did not complete within four minutes.');
 const result={id:job.Id,version:"0.5",sourceUrl:job.SourceUrl,filename:job.FileName,output:path.join(job.Folder,job.FileName),format:job.FormatDescription,transferredBytes:job.TransferredBytes,outputBytes:job.Size,transferSeconds:job.TransferSeconds,resolveSeconds:job.ResolveSeconds,mergeSeconds:job.MergeSeconds,elapsedSeconds:job.ElapsedSeconds,averageMiBps:job.TransferredBytes/job.TransferSeconds/1048576,peakMiBps:job.PeakSpeed/1048576,sha256:job.Sha256,connections:{video:job.Video?.Connections,audio:job.Audio?.Connections},rangeSupport:{video:job.Video?.RangeSupported,audio:job.Audio?.RangeSupported},samples};
 const out=path.join(root,'docs',reportName);fs.writeFileSync(out,JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify({...result,samples:undefined},null,2));
})().catch(e=>{console.error(e.message);process.exitCode=1;});
