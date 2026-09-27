#pragma once
// Protocol fixture exercises real overlapping worker lifetimes, independent
// cookies, fragmented responses, boundary overlaps, assembly and cancellation.
static void parallelMediaChecks(udm::Manager& manager){
 using namespace udm;
 const std::string source="https://www.youtube.com/watch?v=abcdefghijk",endpoint="https://r1.googlevideo.com/videoplayback?id=parallel-fixture";
 auto body=Proto().set(5,Bytes{1}).set(19,Proto().set(3,Bytes{2}).encode()).encode();
 Json offer={{"url",endpoint},{"body",b64(body)},{"videoId","abcdefghijk"},{"capturedAt",epoch()},{"durationMs",200000},{"video",{{"id","137"},{"lastModified","12345"},{"mime","video/mp4"}}},{"audio",{{"id","140"},{"lastModified","67890"},{"mime","audio/mp4"}}}};
 auto makeJob=[&](const char* name,int connections){auto job=manager.add(source,"",name);job->data["SourceUrl"]=source;job->data["Connections"]=connections;job->data["MediaHeight"]=720;job->data["ProtectedSabr"]=protect(offer.dump());setStreams(manager,job,endpoint,endpoint);return job;};
 auto frame=[](Bytes& out,unsigned type,const Bytes& data){for(unsigned n:{type,(unsigned)data.size()}){out.push_back(0xf0);for(int i=0;i<4;++i)out.push_back((unsigned char)(n>>(8*i)));}out.insert(out.end(),data.begin(),data.end());};
 auto payload=[](int track,int seq){return Bytes{(unsigned char)(track?'a':'v'),(unsigned char)(seq<0?255:seq),0,1};};
 struct Evidence {std::atomic<int> arrived{0},active{0},peak{0},cookies{0};std::mutex mutex;std::set<i64> starts;};
 auto fixture=[&](std::shared_ptr<Evidence> evidence,int expected,bool corrupt=false,bool blocked=false,bool ignoreSeek=false)->StreamTransport{
  return [&,evidence,expected,corrupt,blocked,ignoreSeek](const std::string&,const Bytes& bytes)->StreamRead{
   const auto p=Proto::parse(bytes);const i64 time=(i64)p.number(4);auto cookie=Proto::parse(p.data(19)).data(3);bool first=!p.has(3);
   const i64 lane=first?time:(i64)Proto::parse(cookie).number(1);
   if(first){std::lock_guard<std::mutex> lock(evidence->mutex);evidence->starts.insert(time);++evidence->arrived;}
   else{
    ++evidence->cookies;for(auto& field:p.fields)if(field.id==3){auto buffer=Proto::parse(field.data),range=Proto::parse(buffer.data(6));
     if((i64)buffer.number(2)!=(lane/20000)*20000||buffer.number(3)!=range.number(2)||buffer.number(2)!=range.number(1))throw std::runtime_error("Incorrect nonzero buffered range.");
    }
   }
   if(p.data(16).empty()||p.data(17).empty())throw std::runtime_error("Missing selected formats.");
   int now=++evidence->active,peak=evidence->peak;while(peak<now&&!evidence->peak.compare_exchange_weak(peak,now)){}
   Bytes out;int seq=ignoreSeek&&first?0:(int)(time/20000);const int last=(int)(num(offer,"durationMs")/20000)-1;
   frame(out,35,Proto().set(7,Proto().set(1,(unsigned long long)lane).encode()).set(8,std::string("abcdefghijk")).encode());
   for(int track=0;track<2;++track){auto format=offer.at(track?"audio":"video");auto identity=formatIdentity(format);
    if(first)frame(out,42,Proto().set(1,std::string("abcdefghijk")).set(2,identity).set(3,(unsigned long long)num(offer,"durationMs")).set(4,(unsigned long long)last).set(5,str(format,"mime")).encode());
    for(int segment=first?-1:seq;segment<=seq;++segment){if(segment>=0&&segment!=seq)continue;
     unsigned id=1+track*2+(segment<0?0:1);auto header=Proto().set(1,id).set(2,std::string("abcdefghijk")).set(13,identity).set(8,segment<0?1ULL:0ULL).set(9,(unsigned long long)std::max(0,segment)).set(14,4ULL);
     if(segment>=0)header.set(11,(unsigned long long)segment*20000).set(12,20000ULL);
     frame(out,20,header.encode());auto data=payload(track,segment);if(corrupt&&lane==50000&&segment==2)data[2]=99;data.insert(data.begin(),(unsigned char)id);frame(out,21,data);frame(out,22,Bytes{(unsigned char)id});
    }
   }
   auto offset=std::make_shared<size_t>(0);auto ended=std::make_shared<bool>(false);
   return [out,offset,ended,evidence,first,expected,blocked,lane](void* target,size_t size,const Cancel& cancel){
    auto finish=[&]{if(!*ended){*ended=true;--evidence->active;}};
    try{cancel.check();if(first&&*offset==0){auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
      while(evidence->arrived<expected){cancel.wait(5);if(std::chrono::steady_clock::now()>deadline)throw std::runtime_error("Workers did not overlap.");}
      if(blocked)for(;;)cancel.wait(5);if(lane==0)cancel.wait(20);
     }
     auto count=std::min<size_t>({size,17,out.size()-*offset});memcpy(target,out.data()+*offset,count);*offset+=count;if(!count)finish();return count;
    }catch(...){finish();throw;}
   };
  };
 };
 auto evidence=std::make_shared<Evidence>();auto parallel=makeJob("parallel-stream.mp4",4);
 sabrTransfer(manager,parallel,std::make_shared<Cancel>(),fixture(evidence,4));
 check(evidence->peak==4&&evidence->active==0&&num(parallel->data,"SabrMaxConcurrentRequests")==4,"Four SABR responses actually overlap and all close");
 check(evidence->starts==std::set<i64>({0,50000,100000,150000})&&evidence->cookies>4,"Parallel SABR workers use distinct timeline windows and retain independent cookies");
 for(int track=0;track<2;++track){Bytes expected=payload(track,-1);for(int seq=0;seq<10;++seq){auto b=payload(track,seq);expected.insert(expected.end(),b.begin(),b.end());}auto content=readText((track?parallel->audio:parallel->video)->target());check(Bytes(content.begin(),content.end())==expected,track?"Parallel audio is ordered and overlap bytes appear once":"Parallel video is ordered and overlap bytes appear once");}
 check(parallel->workers.size()==4&&num(parallel->data,"StreamCompletedMs")==200000&&num(parallel->data,"SabrActiveRequests")==0,"Connection rows and verified coverage complete together");
 check(!fs::exists(parallel->target())&&num(parallel->video->data,"Received")==44,"Parallel transport waits for mux and excludes duplicated media from stored size");
 auto one=makeJob("single-stream-setting.mp4",1);auto serial=std::make_shared<Evidence>();sabrTransfer(manager,one,std::make_shared<Cancel>(),fixture(serial,1));
 check(serial->peak==1&&fileHash(one->video->target())==fileHash(parallel->video->target()),"One-connection preference uses serial transport with identical media");
 offer["durationMs"]=240000;auto capped=makeJob("capped-stream.mp4",32);auto capEvidence=std::make_shared<Evidence>();sabrTransfer(manager,capped,std::make_shared<Cancel>(),fixture(capEvidence,8));
 check(capEvidence->peak==8&&capped->workers.size()==8,"SABR concurrency remains bounded at eight requests");
 offer["durationMs"]=200000;auto mismatch=makeJob("mismatched-overlap.mp4",4);auto badEvidence=std::make_shared<Evidence>();
 rejects([&]{sabrTransfer(manager,mismatch,std::make_shared<Cancel>(),fixture(badEvidence,4,true));},"Conflicting bytes in overlapping media windows are rejected");
 check(!fs::exists(mismatch->video->target())&&!fs::exists(mismatch->target())&&fs::is_empty(mismatch->video->target().parent_path()),"Parallel validation failure leaves no published track or segment files");
 auto fallback=makeJob("ignored-seek-fallback.mp4",4);auto ignored=std::make_shared<Evidence>();
 sabrTransfer(manager,fallback,std::make_shared<Cancel>(),fixture(ignored,4,false,false,true));
 check(num(fallback->data,"SabrConnections")==1&&!str(fallback->data,"SabrFallbackReason").empty()&&fileHash(fallback->video->target())==fileHash(parallel->video->target()),"Ignored section requests fall back to one connection with exact complete media");
 check(num(fallback->data,"SabrActiveRequests")==0&&fallback->workers.size()==1,"Fallback joins parallel requests before restarting and replaces connection rows");
 auto paused=makeJob("paused-parallel-stream.mp4",4);auto waiting=std::make_shared<Evidence>();auto cancel=std::make_shared<Cancel>();
 auto task=std::async(std::launch::async,[&]{try{sabrTransfer(manager,paused,cancel,fixture(waiting,4,false,true));return false;}catch(const Cancelled&){return true;}});
 auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(waiting->arrived<4&&std::chrono::steady_clock::now()<deadline)Sleep(5);cancel->stop=true;
 check(task.get()&&waiting->active==0&&num(paused->data,"SabrActiveRequests")==0,"Pausing cancels and joins every active streaming request");
 check(!fs::exists(paused->target())&&str(paused->video->data,"Status")=="Paused"&&paused->workers[0].state=="Paused","Paused parallel media does not publish output and stops progress rows");
}
