#include "Streaming.hpp"
#include <iostream>
#include <future>
#include <fstream>
#include "GuiModels.hpp"
#include "StreamProgress.hpp"
#include "MediaStorage.hpp"
using namespace udm;
static Json regressionChecks=Json::array();
static void check(bool ok,const std::string& name){regressionChecks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);}
template<class F>static void rejects(F f,const std::string& name){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,name);}
#include "MediaChecks.hpp"
#include "ParallelMediaChecks.hpp"
#include "AudioStreamingChecks.hpp"
int wmain(int argc,wchar_t** argv){
 if(argc!=2||fs::exists(argv[1]))return 2;
 fs::path root=fs::absolute(argv[1]);fs::create_directories(root);
 Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
 Manager manager(root/L"data");
 const std::string source="https://www.youtube.com/watch?v=abcdefghijk",endpoint="https://r1.googlevideo.com/videoplayback?id=fixture";
 Json offer={{"url",endpoint},{"body",b64(Proto().set(5,Bytes{1}).set(19,Proto().set(3,Bytes{2}).encode()).encode())},{"videoId","abcdefghijk"},{"capturedAt",epoch()},{"durationMs",1000},{"video",{{"id","137"},{"lastModified","12345"},{"mime","video/mp4"}}},{"audio",{{"id","140"},{"lastModified","67890"},{"mime","audio/mp4"}}}};
 auto job=[&](const char* name){auto j=manager.add(source,"",name);j->data["SourceUrl"]=source;j->data["ProtectedSabr"]=protect(offer.dump());setStreams(manager,j,endpoint,endpoint);return j;};
 auto frame=[](Bytes& out,unsigned type,const Bytes& data){for(unsigned n:{type,(unsigned)data.size()}){out.push_back(0xf0);for(int i=0;i<4;++i)out.push_back((unsigned char)(n>>(8*i)));}out.insert(out.end(),data.begin(),data.end());};
 Bytes first,second;Bytes savedHeader;
 for(int track=0;track<2;++track){auto format=offer[track?"audio":"video"];auto id=formatIdentity(format);auto& initial=track?second:first;
  frame(initial,42,Proto().set(1,std::string("abcdefghijk")).set(2,id).set(3,1000ULL).set(4,0ULL).set(5,str(format,"mime")).encode());
  for(int init=0;init<2;++init){unsigned h=1+track*2+init;auto p=Proto().set(1,h).set(2,std::string("abcdefghijk")).set(13,id).set(8,init?0ULL:1ULL).set(9,0ULL).set(14,4ULL);if(init)p.set(11,0ULL).set(12,1000ULL);
   auto& out=track==0&&init==0?first:second;frame(out,20,p.encode());
   if(track==0&&init==0){savedHeader=p.encode();frame(first,21,Bytes{1,'v','i'});frame(second,21,Bytes{1,'0','1'});}
   else frame(out,21,Bytes{(unsigned char)h,(unsigned char)(track?'a':'v'),(unsigned char)(init?'d':'i'),'0','1'});
   frame(second,22,Bytes{(unsigned char)h});
  }
 }
 auto transport=[](std::vector<Bytes> responses,const std::shared_ptr<int>& calls){return [responses,calls](const std::string&,const Bytes&)->StreamRead{if(*calls>=(int)responses.size())throw std::runtime_error("Fixture response budget exhausted");auto bytes=std::make_shared<Bytes>(responses[(*calls)++]);auto at=std::make_shared<size_t>(0);return [bytes,at](void* out,size_t size,const Cancel& c){c.check();auto n=std::min<size_t>({size,3,bytes->size()-*at});memcpy(out,bytes->data()+*at,n);*at+=n;return n;};};};
 auto calls=std::make_shared<int>(0);auto good=job("continued.mp4");sabrTransfer(manager,good,std::make_shared<Cancel>(),transport({first,second},calls));
 check(*calls==2,"A selected segment continues across exactly two responses");
 check(readText(good->video->target())=="vi01vd01"&&readText(good->audio->target())=="ai01ad01","Continued output matches uninterrupted track bytes");
 check(!fs::exists(good->target()),"Continuation does not publish an unmerged final file");
 auto rejects=[&](std::vector<Bytes> responses,const char* name){auto j=job(name);auto count=std::make_shared<int>(0);bool rejected=false;try{sabrTransfer(manager,j,std::make_shared<Cancel>(),transport(responses,count));}catch(const std::exception&){rejected=true;}check(rejected,name);check(!fs::exists(j->target())&&!fs::exists(j->video->target()),"Invalid continuation does not publish track or final output");};
 Bytes conflict;frame(conflict,20,savedHeader);conflict.insert(conflict.end(),second.begin(),second.end());rejects({first,conflict},"Repeated pending header is rejected");
 Bytes excess;frame(excess,21,Bytes{1,'0','1','2'});rejects({first,excess},"Continuation cannot exceed declared segment length");
 Bytes missing;frame(missing,22,Bytes{1});rejects({first,missing},"Early terminator cannot commit incomplete media");
 Bytes foreign;frame(foreign,42,Proto().set(1,std::string("wrongvideo1")).set(2,formatIdentity(offer["video"])).encode());rejects({first,foreign},"Changed video identity is rejected during continuation");
 auto emptyCalls=std::make_shared<int>(0);auto stalled=job("stalled.mp4");bool stopped=false;std::vector<Bytes> stalledResponses(10);stalledResponses[0]=first;try{sabrTransfer(manager,stalled,std::make_shared<Cancel>(),transport(stalledResponses,emptyCalls));}catch(const std::exception&){stopped=true;}
 check(stopped&&*emptyCalls==9,"Eight responses without selected bytes or completed coverage stop continuation");
  auto gzipResponses=[&](int fault){std::vector<Bytes> responses(2);const char* encoded[]={"H4sIAAAAAAAACivLNDAEADrmjqYEAAAA","H4sIAAAAAAAACitLMTAEAGl1Vq4EAAAA","H4sIAAAAAAAACkvMNDAEAByJQGsEAAAA","H4sIAAAAAAAACktMMTAEAE8amGMEAAAA"};
  for(int track=0;track<2;++track){auto format=offer[track?"audio":"video"];auto id=formatIdentity(format);frame(responses[track?1:0],42,Proto().set(1,std::string("abcdefghijk")).set(2,id).set(3,1000ULL).set(4,0ULL).set(5,str(format,"mime")).encode());
   for(int part=0;part<2;++part){unsigned header=1+track*2+part;auto bytes=unb64(encoded[track*2+part]);bool first=track==0&&part==0;
    if(first){if(fault==1)bytes[bytes.size()-8]^=1;if(fault==2)bytes.resize(bytes.size()-3);if(fault==3)bytes.push_back(42);if(fault==6){auto raw=readText(fs::path(__FILE__).parent_path()/L"test-data"/L"sabr-oversize.gz");bytes.assign(raw.begin(),raw.end());}if(fault==7){auto extra=bytes;bytes.insert(bytes.end(),extra.begin(),extra.end());}}
    auto p=Proto().set(1,header).set(2,std::string("abcdefghijk")).set(13,id).set(7,first&&fault==4?2ULL:1ULL).set(8,part?0ULL:1ULL).set(9,0ULL).set(14,(unsigned long long)bytes.size()+(first&&fault==5?1:0));if(part)p.set(11,0ULL).set(12,1000ULL);
    frame(responses[first?0:1],20,p.encode());
    if(first){auto mid=bytes.size()/2;Bytes a{(unsigned char)header},b{(unsigned char)header};a.insert(a.end(),bytes.begin(),bytes.begin()+mid);b.insert(b.end(),bytes.begin()+mid,bytes.end());frame(responses[0],21,a);frame(responses[1],21,b);}
    else{bytes.insert(bytes.begin(),(unsigned char)header);frame(responses[1],21,bytes);}
    frame(responses[1],22,Bytes{(unsigned char)header});
   }
  }return responses;
 };
 auto gz=job("gzip.mp4");auto gzCalls=std::make_shared<int>(0);sabrTransfer(manager,gz,std::make_shared<Cancel>(),transport(gzipResponses(0),gzCalls));
 check(*gzCalls==2&&readText(gz->video->target())=="vi01vd01"&&readText(gz->audio->target())=="ai01ad01","Gzip video and audio decode correctly across responses");
 for(auto fault: {1,2,3,4,5}){auto name=std::string("gzip-invalid-")+std::to_string(fault)+".mp4";rejects(gzipResponses(fault),name.c_str());}
  auto oversized=job("gzip-oversized.mp4");bool sizeRejected=false;try{sabrTransfer(manager,oversized,std::make_shared<Cancel>(),transport(gzipResponses(6),std::make_shared<int>(0)));}catch(const std::exception& e){sizeRejected=std::string(e.what()).find("Decoded streaming segment exceeds the size limit")!=std::string::npos;}
 check(sizeRejected&&!fs::exists(oversized->target())&&!fs::exists(oversized->video->target()),"Gzip expansion above 64 MiB is rejected before publication");
  bool clean=true;for(const auto& entry:fs::recursive_directory_iterator(oversized->video->target().parent_path()))if(entry.is_regular_file()&&(entry.path().extension()==L".decoded"||entry.path().extension()==L".sabrpart"))clean=false;
 check(clean,"Rejected gzip expansion leaves no compressed or decoded temporary segment");
 auto members=job("gzip-members.mp4");sabrTransfer(manager,members,std::make_shared<Cancel>(),transport(gzipResponses(7),std::make_shared<int>(0)));
 check(readText(members->video->target())=="vi01vi01vd01","Concatenated gzip members are decoded in order");
 mediaChecks(manager,root);parallelMediaChecks(manager);parallelMediaChecks(manager,true);audioStreamingChecks(manager,root);
 }catch(const std::exception& e){error=e.what();}
 for(const auto& item:regressionChecks)checks.push_back(item);
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<error<<std::endl;return error.empty()?0:1;
}
