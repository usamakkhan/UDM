#pragma once
#include "HlsRecording.hpp"
static void hlsRecordingChecks(const fs::path& root){
 auto progress=Json{{"LiveRecording",true},{"Status","Downloading"},{"Size",100},{"Received",100},{"AdaptiveTotalSegments",2},{"AdaptiveCompletedSegments",2},{"LiveCapturedSeconds",64.5}};
 check(downloadPermille(progress)==-1&&downloadSecondsLeft(progress,100)==-1&&streamProgressText(progress).find("1:04")!=std::string::npos,"Live progress shows recorded time without a false completion percentage or ETA");
 progress["Status"]="Complete";check(downloadPermille(progress)==1000,"A saved live recording reaches complete progress");

 const std::string source="https://media.example.test/live/index.m3u8?secret=token";
 const std::string header="#EXTM3U\n#EXT-X-TARGETDURATION:4\n#EXT-X-MEDIA-SEQUENCE:10\n";
 auto playlist=parseHlsPlaylist(header+"#EXTINF:4,\na.ts?secret=token\n#EXTINF:4,\nb.ts?secret=token\n#EXTINF:4,\nc.ts?secret=token\n",source);
 auto path=root/L"live-recording-journal";HlsRecording journal(path,{source});check(journal.accept(0,playlist)==3,"HLS journal commits the initial recording window");
 check(readText(path/L"recording.json").find("secret=token")==std::string::npos&&readText(path/L"0-10.json").find("media.example")==std::string::npos,"HLS recovery headers and segment receipts protect captured addresses");
 writeBytes(journal.mediaPath(0,10),Bytes{'o','n','e'});journal.commitMedia(0,10);
 writeBytes(journal.mediaPath(0,12),Bytes{'t','h','r','e','e'});journal.commitMedia(0,12);
 check(journal.hasMedia(0,10)&&journal.hasMedia(0,12)&&journal.ready(0).size()==1,"HLS journal retains parallel results but exposes only a contiguous playable prefix");
 rejects([&]{journal.localPlaylist(0);},"HLS finalization requires stop or server completion");
 {HlsRecording resumed(path,{source});check(resumed.hasMedia(0,10)&&resumed.hasMedia(0,12)&&!resumed.hasMedia(0,11)&&resumed.ready(0).size()==1,"HLS recovery verifies media hashes and resumes pending segments");}
 writeBytes(journal.mediaPath(0,11),Bytes{'t','w','o'});journal.commitMedia(0,11);check(journal.ready(0).size()==3,"HLS journal closes a completed parallel-download gap");
 auto changed=playlist;changed.segments[1].media.url="https://other.example.test/wrong.ts";
 auto before=readText(path/L"recording.json");rejects([&]{journal.accept(0,changed);},"HLS journal rejects an identity change before committing it");
 check(readText(path/L"recording.json")==before&&journal.ready(0).size()==3,"Rejected HLS updates preserve the journal and playable data");
 journal.seal();check(journal.isSealed(),"Stopping an HLS recording commits the save boundary");
 auto output=readText(journal.localPlaylist(0));check(output.find("0-10.part")!=std::string::npos&&output.find("0-12.part")!=std::string::npos&&output.find("#EXT-X-ENDLIST")!=std::string::npos&&output.find("http")==std::string::npos&&output.find("secret")==std::string::npos,"HLS finalization creates a closed local-only media playlist");
 {HlsRecording resumed(path,{source});check(resumed.isSealed()&&resumed.ready(0).size()==3,"Stop-and-save intent and verified segments survive app restart");}
 rejects([&]{journal.accept(0,playlist);},"A stopped recording cannot silently resume capture");
 rejects([&]{HlsRecording wrong(path,{"https://other.example.test/live.m3u8"});},"HLS journal cannot be reused for another source");
 writeBytes(journal.mediaPath(0,10),Bytes{'b','a','d'});rejects([&]{journal.localPlaylist(0);},"HLS finalization catches a same-size segment edit");
 {HlsRecording resumed(path,{source});check(!resumed.hasMedia(0,10)&&resumed.hasMedia(0,11)&&resumed.ready(0).empty(),"HLS restart marks corrupted media missing without publishing it");}
 auto recoveryPath=root/L"live-recording-commit-failure";HlsRecording failedCommit(recoveryPath,{source});failedCommit.accept(0,playlist);
 writeBytes(failedCommit.mediaPath(0,10),Bytes{'o','n','e'});
 {Handle held(CreateFileW((recoveryPath/L"0-10.json.tmp").c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));check((bool)held,"HLS segment receipt failure fixture holds the pending write");rejects([&]{failedCommit.commitMedia(0,10);},"HLS does not mark a segment complete when its receipt cannot commit");}
 check(!failedCommit.hasMedia(0,10)&&fs::exists(failedCommit.mediaPath(0,10)),"An HLS receipt failure retains untrusted bytes for a later retry");
 failedCommit.commitMedia(0,10);check(failedCommit.hasMedia(0,10),"An HLS segment can commit after transient receipt contention ends");
 auto extension=parseHlsPlaylist("#EXTM3U\n#EXT-X-TARGETDURATION:4\n#EXT-X-MEDIA-SEQUENCE:11\n#EXTINF:4,\nb.ts?secret=token\n#EXTINF:4,\nc.ts?secret=token\n#EXTINF:4,\nd.ts?secret=token\n",source);
 {Handle held(CreateFileW((recoveryPath/L"recording.json.tmp").c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));rejects([&]{failedCommit.accept(0,extension);},"A failed HLS header write cannot advance the recording boundary");}
 check(failedCommit.timeline(0).size()==3,"HLS header failure leaves the in-memory timeline unchanged");
 {HlsRecording resumed(recoveryPath,{source});check(resumed.timeline(0).size()==3&&!resumed.hasMedia(0,13),"HLS recovery ignores prepared receipts beyond the committed window");check(resumed.accept(0,extension)==1,"HLS can retry the prepared refresh after header failure");}
 auto mapPlaylist=parseHlsPlaylist(header+"#EXT-X-MAP:URI=\"init.mp4\",BYTERANGE=\"4@0\"\n#EXTINF:4,\na.m4s\n#EXT-X-DISCONTINUITY\n#EXT-X-MAP:URI=\"second.mp4\"\n#EXTINF:4,\nb.m4s\n#EXT-X-ENDLIST\n",source);
 auto mapPath=root/L"live-recording-initialization";HlsRecording maps(mapPath,{source});maps.accept(0,mapPlaylist);writeBytes(maps.mediaPath(0,10),Bytes{'m','e','d','i','a'});maps.commitMedia(0,10);
 check(maps.ready(0).empty(),"HLS media is not playable until its initialization section is committed");
 auto initialization=*mapPlaylist.segments[0].initialization;writeBytes(maps.initializationPath(0,initialization),Bytes{'i','n','i'});rejects([&]{maps.commitInitialization(0,initialization);},"HLS checks initialization byte-range length");
 writeBytes(maps.initializationPath(0,initialization),Bytes{'i','n','i','t'});maps.commitInitialization(0,initialization);check(maps.ready(0).size()==1,"HLS recognizes a complete initialized media prefix");
 auto second=*mapPlaylist.segments[1].initialization;writeBytes(maps.initializationPath(0,second),Bytes{'n','e','w'});maps.commitInitialization(0,second);writeBytes(maps.mediaPath(0,11),Bytes{'m','o','r','e'});maps.commitMedia(0,11);
 auto local=readText(maps.localPlaylist(0));check(maps.ended()&&local.find("#EXT-X-DISCONTINUITY\n")!=std::string::npos&&local.find("#EXT-X-MAP:URI=")!=std::string::npos&&local.find("BYTERANGE")==std::string::npos,"Closed HLS output preserves initialization changes and discontinuities using downloaded ranges");
 {HlsRecording resumed(mapPath,{source});check(resumed.hasInitialization(0,initialization)&&resumed.ready(0).size()==2,"HLS initialization receipts survive restart with verified hashes");}
 writeBytes(maps.initializationPath(0,second),Bytes{'b','a','d'});rejects([&]{maps.localPlaylist(0);},"HLS finalization catches modified initialization data");
 auto gapPlaylist=parseHlsPlaylist(header+"#EXTINF:4,\na.ts\n#EXT-X-GAP\n#EXTINF:4,\nb.ts\n#EXTINF:4,\nc.ts\n",source);HlsRecording gap(root/L"live-recording-gap",{source});gap.accept(0,gapPlaylist);writeBytes(gap.mediaPath(0,10),Bytes{'a'});gap.commitMedia(0,10);writeBytes(gap.mediaPath(0,11),Bytes{'b'});rejects([&]{gap.commitMedia(0,11);},"A server-declared HLS gap cannot be treated as captured media");gap.seal();check(gap.ready(0).size()==1&&readText(gap.localPlaylist(0)).find("0-11.part")==std::string::npos,"Stop and save retains the valid prefix before a declared live gap");
 auto audioSource="https://media.example.test/live/audio.m3u8";HlsRecording paired(root/L"live-recording-tracks",{source,audioSource});paired.accept(0,playlist);auto audio=parseHlsPlaylist(header+"#EXTINF:4,\na.aac\n#EXT-X-ENDLIST\n",audioSource);paired.accept(1,audio);writeBytes(paired.mediaPath(1,10),Bytes{'a','u','d','i','o'});paired.commitMedia(1,10);
 check(paired.ready(0).empty()&&paired.ready(1).size()==1&&!paired.ended(),"Live audio and video keep independent sequence spaces and completion state");
 check(real(paired.progress(),"seconds")==0,"A live recording cannot be offered for saving until every selected track has a playable prefix");
 writeBytes(paired.mediaPath(0,10),Bytes{'v','i','d','e','o'});paired.commitMedia(0,10);check(real(paired.progress(),"seconds")==4,"Live save readiness uses the common captured duration across selected tracks");
 rejects([&]{paired.accept(1,playlist);},"An audio refresh cannot use the video's playlist identity");
 auto forged=hlsSegmentData(playlist.segments[0]);forged["sequence"]=-1;rejects([&]{hlsReadSegment(forged);},"HLS recovery rejects invalid saved sequence numbers");
 auto restore=hlsReadSegment(hlsSegmentData(mapPlaylist.segments[0]));check(hlsSameSegment(restore,mapPlaylist.segments[0]),"HLS recovery preserves complete media and initialization identity");
 auto cleanup=root/L"live-cleanup-owned";fs::create_directories(cleanup/L"nested");for(const auto& name:{L"0-12.part",L"1-45.json.bak",L"recording.json",L"0.m3u8.tmp",L"notes.txt",L"nested/0-12.part"})writeBytes(cleanup/name,Bytes{1});cleanLiveHlsCache(cleanup);
 check(!fs::exists(cleanup/L"0-12.part")&&!fs::exists(cleanup/L"1-45.json.bak")&&!fs::exists(cleanup/L"recording.json")&&!fs::exists(cleanup/L"0.m3u8.tmp")&&fs::exists(cleanup/L"notes.txt")&&fs::exists(cleanup/L"nested"/L"0-12.part"),"Live cleanup removes only owned files and preserves unrelated files and child directories");
}
