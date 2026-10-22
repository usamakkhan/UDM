#include "Streaming.hpp"
#include <iostream>
#include <future>
#include <fstream>
#include "GuiModels.hpp"
#include "StreamProgress.hpp"
#include "MediaStorage.hpp"
#include "ProtocolFallback.hpp"
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
 {
  Manager admission(root/L"prefetch-admission");admission.state["Settings"]["PrefetchFileInfo"]=true;
  auto pending=admission.add("http://127.0.0.1:9/prefetch.bin","","prefetch.bin","Main queue",true);
  pending->data["PrefetchedFinalSize"]=123;pending->data["ReusedPrefetchedOutput"]=true;pending->data["ContinuedMediaPrefetch"]=true;
  admission.save();const auto before=pending->data;const auto persisted=readText(admission.root/L"state.json");
  bool failed=false;{Handle held(CreateFileW((admission.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
   check(bool(held),"Prefetch catalog failure fixture holds the saved record");
   try{admission.beginPrefetch(pending);}catch(const std::exception&){failed=true;}
  }
  check(failed,"Prefetch admission reports a failed catalog save");
  check(!admission.isActive(pending),"Failed prefetch admission starts no transfer");
  check(readText(admission.root/L"state.json")==persisted,"Failed prefetch admission preserves the durable record");
  check(pending->data==before,"Failed prefetch admission restores confirmation and previous metadata in memory");
 }
 Manager manager(root/L"data");
 {auto settings=manager.state["Settings"];settings["DownloadFolder"]=utf8((root/L"files").wstring());settings["CategoryFolders"]=false;manager.setSettings(settings);}
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
 {
  auto preferences=manager.state["Settings"],queues=manager.state["Queues"];for(auto& queue:manager.state["Queues"])queue.erase("Retries");manager.state["Settings"]["Retries"]=1;
  auto retried=job("connection-retry.mp4");auto count=std::make_shared<int>(0);StreamTransport responses=transport({first,second},std::make_shared<int>(0));
  StreamTransport interrupted=[&,count,responses](const std::string& url,const Bytes& body)mutable->StreamRead{
   ++*count;if(*count==2)throw InternetFailure("HTTP read",ERROR_WINHTTP_CONNECTION_ERROR);
   if(*count==3)responses=transport({first,second},std::make_shared<int>(0));
   return responses(url,body);
  };
  sabrTransfer(manager,retried,std::make_shared<Cancel>(),interrupted);
  check(*count==4&&num(retried->data,"SabrConnectionRetries")==1,"Dropped streaming connection retries within configured budget");
  check(readText(retried->video->target())=="vi01vd01"&&readText(retried->audio->target())=="ai01ad01","Connection retry discards partial data and preserves exact output");
  Bytes completeResponse=first;completeResponse.insert(completeResponse.end(),second.begin(),second.end());
  for(auto cut:std::vector<size_t>{1,first.size(),completeResponse.size()-3}){
   const auto name="read-retry-"+std::to_string(cut)+".mp4";auto recovering=job(name.c_str());int requests=0;size_t delivered=0;
   auto replay=transport({completeResponse},std::make_shared<int>(0));
   StreamTransport brokenRead=[&](const std::string& url,const Bytes& body)->StreamRead{
    if(++requests>1)return replay(url,body);
    auto offset=std::make_shared<size_t>(0);
    return [&,offset,cut](void* out,size_t capacity,const Cancel& cancel){
     cancel.check();if(*offset==cut)throw InternetFailure("HTTP read",ERROR_WINHTTP_CONNECTION_ERROR);
     auto n=std::min<size_t>({capacity,3,cut-*offset});memcpy(out,completeResponse.data()+*offset,n);*offset+=n;delivered+=n;return n;
    };
   };
   sabrTransfer(manager,recovering,std::make_shared<Cancel>(),brokenRead);
   check(delivered==cut&&requests==2&&num(recovering->data,"SabrConnectionRetries")==1,"Response read failure retries after receiving partial bytes");
   check(readText(recovering->video->target())=="vi01vd01"&&readText(recovering->audio->target())=="ai01ad01","Retry across framing, partial and committed segments preserves exact track bytes");
   check(!fs::exists(recovering->target()),"Recovered tracks are not published as an unmerged final file");
  }
  for(auto code:{ERROR_WINHTTP_CONNECTION_ERROR,ERROR_WINHTTP_SECURE_FAILURE}){
   manager.state["Settings"]["Retries"]=code==ERROR_WINHTTP_CONNECTION_ERROR?0:2;int attempts=0;bool failed=false;
   try{sabrTransfer(manager,job(code==ERROR_WINHTTP_CONNECTION_ERROR?"no-retry.mp4":"tls-failure.mp4"),std::make_shared<Cancel>(),[&](const std::string&,const Bytes&)->StreamRead{++attempts;throw InternetFailure("HTTP read",code);});}catch(const InternetFailure&){failed=true;}
   check(failed&&attempts==1,"Zero retry budget and TLS failures never replay the streaming request");
  }
  manager.state["Settings"]["Retries"]=1;
  for(auto status:{429,503}){
   auto name="server-retry-"+std::to_string(status)+".mp4";auto recovering=job(name.c_str());int requests=0;ULONGLONG rejectedAt=0,retriedAt=0;
   auto replay=transport({completeResponse},std::make_shared<int>(0));
   sabrTransfer(manager,recovering,std::make_shared<Cancel>(),[&](const std::string& url,const Bytes& body)->StreamRead{
    if(++requests==1){rejectedAt=GetTickCount64();throw HttpRejected(status,"1");}retriedAt=GetTickCount64();return replay(url,body);
   });
   check(requests==2&&retriedAt-rejectedAt>=1000&&num(recovering->data,"SabrServerRetries")==1,"Streaming honors the server Retry-After delay before retrying");
   check(readText(recovering->video->target())=="vi01vd01"&&readText(recovering->audio->target())=="ai01ad01","Temporary server rejection recovers exact audio and video tracks");
  }
  for(auto status:{401,403,410,503}){
   int requests=0;bool failed=false;auto name="server-terminal-"+std::to_string(status)+".mp4";
   try{sabrTransfer(manager,job(name.c_str()),std::make_shared<Cancel>(),[&](const std::string&,const Bytes&)->StreamRead{++requests;throw HttpRejected(status,status==503?"601":"");});}catch(const HttpRejected& e){failed=e.status==status;}
   check(failed&&requests==1,"Authentication, expired links and excessive server waits remain terminal");
  }
  {int requests=0;bool failed=false;
   try{sabrTransfer(manager,job("mixed-retry-limit.mp4"),std::make_shared<Cancel>(),[&](const std::string&,const Bytes&)->StreamRead{if(++requests==1)throw InternetFailure("HTTP read",ERROR_WINHTTP_CONNECTION_ERROR);throw HttpRejected(503,"0");});}catch(const HttpRejected&){failed=true;}
   check(failed&&requests==2,"Connection and server failures share one retry budget");
  }
  manager.state["Settings"]["Retries"]=2;
  auto exhausted=job("retry-exhausted.mp4");int attempts=0;bool exhaustedFailure=false;
  try{sabrTransfer(manager,exhausted,std::make_shared<Cancel>(),[&](const std::string&,const Bytes&)->StreamRead{++attempts;throw InternetFailure("HTTP read",ERROR_WINHTTP_TIMEOUT);});}catch(const InternetFailure& e){exhaustedFailure=e.code==ERROR_WINHTTP_TIMEOUT;}
  check(exhaustedFailure&&attempts==3,"Persistent timeout stops after the configured two retries");
  check(!fs::exists(exhausted->target())&&!fs::exists(exhausted->video->target()),"Exhausted retries do not publish incomplete media");
  auto overridden=job("queue-no-retry.mp4");bool matched=false;
  for(auto& queue:manager.state["Queues"])if(str(queue,"Name")==str(overridden->data,"Queue")){queue["Retries"]=0;matched=true;}
  attempts=0;bool overrideFailure=false;
  try{sabrTransfer(manager,overridden,std::make_shared<Cancel>(),[&](const std::string&,const Bytes&)->StreamRead{++attempts;throw InternetFailure("HTTP read",ERROR_WINHTTP_CONNECTION_ERROR);});}catch(const InternetFailure&){overrideFailure=true;}
  check(matched&&overrideFailure&&attempts==1,"Queue retry limit overrides the global retry preference");
  for(auto& queue:manager.state["Queues"])queue.erase("Retries");
  auto cancelledJob=job("cancel-retry.mp4");auto cancellation=std::make_shared<Cancel>();std::atomic_int cancelCalls{0};
  auto pending=std::async(std::launch::async,[&]{try{sabrTransfer(manager,cancelledJob,cancellation,[&](const std::string&,const Bytes&)->StreamRead{++cancelCalls;throw InternetFailure("HTTP read",ERROR_WINHTTP_CONNECTION_ERROR);});}catch(const Cancelled&){return true;}return false;});
  bool retrying=false;auto retryDeadline=GetTickCount64()+5000;
  while(GetTickCount64()<retryDeadline){{Lock lock(manager.mutex);retrying=str(cancelledJob->data,"MediaPhase")=="Retrying connection";}if(retrying)break;Sleep(1);}
  cancellation->stop=true;auto stoppedPromptly=pending.wait_for(std::chrono::milliseconds(1000))==std::future_status::ready;auto cancelled=pending.get();
  check(retrying&&cancelled&&stoppedPromptly&&cancelCalls==1,"Stop during retry backoff cancels promptly without another request");
  check(!fs::exists(cancelledJob->target())&&!fs::exists(cancelledJob->video->target()),"Cancelled retry does not publish incomplete media");
  manager.state["Settings"]=preferences;manager.state["Queues"]=queues;
 }
 auto rejects=[&](std::vector<Bytes> responses,const char* name){auto j=job(name);auto count=std::make_shared<int>(0);bool rejected=false;try{sabrTransfer(manager,j,std::make_shared<Cancel>(),transport(responses,count));}catch(const std::exception&){rejected=true;}check(rejected,name);check(!fs::exists(j->target())&&!fs::exists(j->video->target()),"Invalid continuation does not publish track or final output");};
 Bytes conflict;frame(conflict,20,savedHeader);conflict.insert(conflict.end(),second.begin(),second.end());rejects({first,conflict},"Repeated pending header is rejected");
 Bytes excess;frame(excess,21,Bytes{1,'0','1','2'});rejects({first,excess},"Continuation cannot exceed declared segment length");
 Bytes missing;frame(missing,22,Bytes{1});rejects({first,missing},"Early terminator cannot commit incomplete media");
 Bytes foreign;frame(foreign,42,Proto().set(1,std::string("wrongvideo1")).set(2,formatIdentity(offer["video"])).encode());rejects({first,foreign},"Changed video identity is rejected during continuation");
 auto emptyCalls=std::make_shared<int>(0);auto stalled=job("stalled.mp4");bool stopped=false;std::vector<Bytes> stalledResponses(10);stalledResponses[0]=first;try{sabrTransfer(manager,stalled,std::make_shared<Cancel>(),transport(stalledResponses,emptyCalls));}catch(const std::exception&){stopped=true;}
 check(stopped&&*emptyCalls==9,"Eight responses without selected bytes or completed coverage stop continuation");
  auto compressedResponses=[&](bool brotli,int fault){std::vector<Bytes> responses(2);const char* encoded[]={"H4sIAAAAAAAACivLNDAEADrmjqYEAAAA","H4sIAAAAAAAACitLMTAEAGl1Vq4EAAAA","H4sIAAAAAAAACkvMNDAEAByJQGsEAAAA","H4sIAAAAAAAACktMMTAEAE8amGMEAAAA"};
  for(int track=0;track<2;++track){auto format=offer[track?"audio":"video"];auto id=formatIdentity(format);frame(responses[track?1:0],42,Proto().set(1,std::string("abcdefghijk")).set(2,id).set(3,1000ULL).set(4,0ULL).set(5,str(format,"mime")).encode());
   for(int part=0;part<2;++part){unsigned header=1+track*2+part;const char* br[]={"iwGAdmkwMQM=","iwGAdmQwMQM=","iwGAYWkwMQM=","iwGAYWQwMQM="};auto bytes=unb64((brotli?br:encoded)[track*2+part]);bool first=track==0&&part==0;
    if(first){if(fault==1){if(brotli)bytes[0]=0xff;else bytes[bytes.size()-8]^=1;}if(fault==2)bytes.resize(bytes.size()-3);if(fault==3)bytes.push_back(42);if(fault==6||fault==8){auto raw=readText(fs::path(__FILE__).parent_path()/L"test-data"/(fault==6?(brotli?L"sabr-oversize.br":L"sabr-oversize.gz"):(brotli?L"sabr-drain.br":L"sabr-drain.gz")));bytes.assign(raw.begin(),raw.end());}if(fault==7){auto extra=bytes;bytes.insert(bytes.end(),extra.begin(),extra.end());}}
    auto p=Proto().set(1,header).set(2,std::string("abcdefghijk")).set(13,id).set(7,first&&fault==4?3ULL:(brotli?2ULL:1ULL)).set(8,part?0ULL:1ULL).set(9,0ULL).set(14,(unsigned long long)bytes.size()+(first&&fault==5?1:0));if(part)p.set(11,0ULL).set(12,1000ULL);
    frame(responses[first?0:1],20,p.encode());
    if(first){auto mid=bytes.size()/2;Bytes a{(unsigned char)header},b{(unsigned char)header};a.insert(a.end(),bytes.begin(),bytes.begin()+mid);b.insert(b.end(),bytes.begin()+mid,bytes.end());frame(responses[0],21,a);frame(responses[1],21,b);}
    else{bytes.insert(bytes.begin(),(unsigned char)header);frame(responses[1],21,bytes);}
    frame(responses[1],22,Bytes{(unsigned char)header});
   }
  }return responses;
 };
 auto gzipResponses=[&](int fault){return compressedResponses(false,fault);};auto gz=job("gzip.mp4");auto gzCalls=std::make_shared<int>(0);sabrTransfer(manager,gz,std::make_shared<Cancel>(),transport(gzipResponses(0),gzCalls));
 check(*gzCalls==2&&readText(gz->video->target())=="vi01vd01"&&readText(gz->audio->target())=="ai01ad01","Gzip video and audio decode correctly across responses");
 for(auto fault: {1,2,3,4,5}){auto name=std::string("gzip-invalid-")+std::to_string(fault)+".mp4";rejects(gzipResponses(fault),name.c_str());}
  auto oversized=job("gzip-oversized.mp4");bool sizeRejected=false;try{sabrTransfer(manager,oversized,std::make_shared<Cancel>(),transport(gzipResponses(6),std::make_shared<int>(0)));}catch(const std::exception& e){sizeRejected=std::string(e.what()).find("Decoded streaming segment exceeds the size limit")!=std::string::npos;}
 check(sizeRejected&&!fs::exists(oversized->target())&&!fs::exists(oversized->video->target()),"Gzip expansion above 64 MiB is rejected before publication");
  bool clean=true;for(const auto& entry:fs::recursive_directory_iterator(oversized->video->target().parent_path()))if(entry.is_regular_file()&&(entry.path().extension()==L".decoded"||entry.path().extension()==L".sabrpart"))clean=false;
 check(clean,"Rejected gzip expansion leaves no compressed or decoded temporary segment");
 auto members=job("gzip-members.mp4");sabrTransfer(manager,members,std::make_shared<Cancel>(),transport(gzipResponses(7),std::make_shared<int>(0)));
 check(readText(members->video->target())=="vi01vi01vd01","Concatenated gzip members are decoded in order");
  auto br=job("brotli.mp4");auto brCalls=std::make_shared<int>(0);sabrTransfer(manager,br,std::make_shared<Cancel>(),transport(compressedResponses(true,0),brCalls));
 check(*brCalls==2&&readText(br->video->target())=="vi01vd01"&&readText(br->audio->target())=="ai01ad01","Brotli video and audio decode correctly across responses");
 for(auto fault:{1,2,3,4,5}){auto name=std::string("brotli-invalid-")+std::to_string(fault)+".mp4";rejects(compressedResponses(true,fault),name.c_str());}
 auto brLarge=job("brotli-oversized.mp4");bool brSizeRejected=false;try{sabrTransfer(manager,brLarge,std::make_shared<Cancel>(),transport(compressedResponses(true,6),std::make_shared<int>(0)));}catch(const std::exception& e){brSizeRejected=std::string(e.what()).find("Decoded streaming segment exceeds the size limit")!=std::string::npos;}
 check(brSizeRejected&&!fs::exists(brLarge->target())&&!fs::exists(brLarge->video->target()),"Brotli expansion above 64 MiB is rejected before publication");
 bool brClean=true;for(const auto& entry:fs::recursive_directory_iterator(brLarge->video->target().parent_path()))if(entry.is_regular_file()&&(entry.path().extension()==L".decoded"||entry.path().extension()==L".sabrpart"))brClean=false;
 check(brClean,"Rejected Brotli expansion leaves no compressed or decoded temporary segment");
 for(bool brotli:{false,true}){auto drain=job(brotli?"brotli-drain.mp4":"gzip-drain.mp4");sabrTransfer(manager,drain,std::make_shared<Cancel>(),transport(compressedResponses(brotli,8),std::make_shared<int>(0)));check(readText(drain->video->target())==std::string(65537,'\0')+"vd01",brotli?"Brotli drains decoded output after input EOF":"Gzip drains decoded output after input EOF");}
 mediaChecks(manager,root);parallelMediaChecks(manager);parallelMediaChecks(manager,true);audioStreamingChecks(manager,root);
 }catch(const std::exception& e){error=e.what();}
 for(const auto& item:regressionChecks)checks.push_back(item);
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<error<<std::endl;return error.empty()?0:1;
}
