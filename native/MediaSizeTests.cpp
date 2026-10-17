#include "MediaSize.hpp"
#include "MediaPreview.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2&&argc!=3)return 2;fs::path root=argv[1];if(fs::exists(root))return 3;fs::create_directories(root);Json checks=Json::array();
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});};
 Json data={{"SourceUrl","https://www.youtube.com/watch?v=abcdefghijk"},{"Size",-1},{"Status","Awaiting confirmation"}};
 saveMediaSizeHint(data,{{"estimatedSize",12345678}});
 check(mediaSizeHint(data)==12345678&&downloadSizeLabel(data)=="~ "+bytes(12345678),"Selected media size appears before starting");
 check(num(data,"Size")==-1&&!data.contains("Received")&&!data.contains("Segments"),"Size hint cannot change transfer or resume metadata");
 data["Size"]=456;check(downloadSizeLabel(data)==bytes(456),"Actual size takes precedence");
 data["Size"]=0;check(downloadSizeLabel(data)==bytes(0),"Actual empty file is not replaced by an estimate");data["Size"]=-1;
 data["Status"]="Complete";check(mediaSizeHint(data)==0,"Completed downloads do not show stale estimates");data["Status"]="Paused";
 for(const auto& value:Json::array({0,-1,1.5,"123",true,nullptr,9007199254740992ULL,18446744073709551615ULL})){
  saveMediaSizeHint(data,{{"estimatedSize",value}});check(mediaSizeHint(data)==0,"Malformed or unsafe size is ignored");
 }
 saveMediaSizeHint(data,{{"estimatedSize",123}});saveMediaSizeHint(data,Json::object());check(mediaSizeHint(data)==0,"Missing refreshed size clears the previous hint");
 saveMediaSizeHint(data,{{"estimatedSize",123}});data.erase("SourceUrl");check(mediaSizeHint(data)==0,"Ordinary files do not use media size hints");
 {
  Manager manager(root/L"catalog");auto job=manager.add("https://www.youtube.com/watch?v=abcdefghijk",utf8((root/L"files").wstring()),"video.mp4","Main queue",true);
  job->data["SourceUrl"]=str(job->data,"Url");saveMediaSizeHint(job->data,{{"estimatedSize",12345678}});manager.save();
 }
 {Manager reopened(root/L"catalog");check(reopened.jobs.size()==1&&mediaSizeHint(reopened.jobs[0]->data)==12345678&&num(reopened.jobs[0]->data,"Size")==-1,"Size hint survives save and reopen without becoming a transfer size");}

 Json reported={{"SourceUrl","https://www.youtube.com/watch?v=abcdefghijk"},{"Size",-1},{"Status","Awaiting confirmation"}};
 saveMediaSizeHint(reported,{{"reportedSize",12345678},{"estimatedSize",42}});
 check(reportedMediaSize(reported)==12345678&&downloadSizeLabel(reported)==bytes(12345678),"Reported stream bytes are displayed without an estimate marker");
 check(num(reported,"Size")==-1&&!reported.contains("Received")&&!reported.contains("Segments"),"Reported bytes remain independent of transfer bounds");
 reported["Size"]=789;check(downloadSizeLabel(reported)==bytes(789),"Final file size takes precedence over reported streams");reported["Size"]=-1;
 reported["Status"]="Complete";check(reportedMediaSize(reported)==0,"Completed file does not display stale stream lengths");reported["Status"]="Paused";
 for(const auto& value:Json::array({0,-1,1.5,"123",true,nullptr,9007199254740992ULL,18446744073709551615ULL})){
  saveMediaSizeHint(reported,{{"reportedSize",value}});check(reportedMediaSize(reported)==0,"Unsafe reported byte count is ignored");
 }
 saveMediaSizeHint(reported,{{"reportedSize",123}});saveMediaSizeHint(reported,Json::object());check(reportedMediaSize(reported)==0,"Refresh clears stale reported bytes");
 saveMediaSizeHint(reported,{{"reportedSize",123}});reported.erase("SourceUrl");check(reportedMediaSize(reported)==0,"Ordinary files cannot use reported media bytes");
 Json direct={{"SourceUrl","https://www.youtube.com/watch?v=abcdefghijk"},{"Video",{{"Url","https://r1.googlevideo.com/videoplayback?clen=100"}}},{"Audio",{{"Url","https://r1.googlevideo.com/videoplayback?clen=200"}}}};
 check(canPreviewMedia(direct),"Direct tracks can calculate a final mux size");direct["ProtectedSabr"]="session";check(!canPreviewMedia(direct),"Streaming sessions do not claim a complete metadata preview");direct.erase("ProtectedSabr");direct["Audio"]["Url"]="https://example.org/videoplayback?clen=200";check(!canPreviewMedia(direct),"Metadata preview is restricted to captured media origins");
 auto word=[](Bytes& b,unsigned n){for(int shift=24;shift>=0;shift-=8)b.push_back(static_cast<unsigned char>(n>>shift));};
 Bytes run;word(run,24);run.insert(run.end(),{'t','r','u','n'});word(run,0x200);word(run,2);word(run,123);word(run,456);
 check(media_preview::samples(run,0,run.size())==2,"Complete sample size table is counted");
 auto rejects=[&](Bytes b){try{media_preview::samples(b,0,b.size());return false;}catch(const std::exception&){return true;}};
 auto invalid=run;invalid.pop_back();check(rejects(invalid),"Truncated sample table is rejected");invalid=run;invalid[11]=0;invalid[10]=0;check(rejects(invalid),"Missing per-sample sizes cannot produce a calculated result");invalid=run;invalid[15]=3;check(rejects(invalid),"False sample count is rejected");invalid=run;invalid[3]=1;check(rejects(invalid),"Unsupported extended-size box is rejected");
 std::vector<unsigned> sizes;media_preview::samples(run,0,run.size(),0,&sizes);check(sizes==std::vector<unsigned>{123,456},"Sample offsets use the complete per-sample size records");
 auto delimiter=media_preview::obu(Bytes{0x12,0},2);check(delimiter.type==2&&delimiter.header==2&&delimiter.payload==0,"AV1 temporal delimiter is inspected without a fixed byte correction");
 auto frame=media_preview::obu(Bytes{0x32,0x80,0x02},259);check(frame.type==6&&frame.header==3&&frame.payload==256,"AV1 variable-length OBU payload is bounded by its sample");
 auto rejectsObu=[](Bytes b,uint64_t n){try{media_preview::obu(b,n);return false;}catch(...){return true;}};
 check(rejectsObu({0xb2,0},2)&&rejectsObu({0x30,0},2)&&rejectsObu({0x32,0x80},2)&&rejectsObu({0x32,4},5),"Invalid, unsized, truncated and overflowing AV1 OBUs are rejected");
 // Optional explicit live fixture: never consult browser profiles or the user's
 // catalog automatically. The caller supplies the selected captured job.
 if(argc==3){try{auto fixture=Json::parse(readText(argv[2]));Cancel cancel;cancel.deadline=GetTickCount64()+240000;auto before=fixture.at("download");auto result=probeMediaSize(before,fixture.at("preferences"),cancel);atomicText(root/L"preflight.json",result.dump(2),false);check(num(result,"Size")==num(fixture,"expectedBytes")&&yes(result,"CalculatedMediaSize"),"Native metadata-only mux matches the independently downloaded file byte-for-byte");check(before==fixture.at("download"),"Preflight leaves download and resume state unchanged");}catch(const std::exception& e){atomicText(root/L"failure.txt",e.what(),false);check(false,"Live metadata preflight completed successfully");}}
 bool passed=true;for(auto& c:checks)passed&=yes(c,"passed");atomicText(root/L"results.json",Json{{"passed",passed},{"checks",checks}}.dump(2),false);std::cout<<checks.dump(2)<<std::endl;return passed?0:1;
}
