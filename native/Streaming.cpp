#include "MediaStorage.hpp"
#include "Streaming.hpp"
#include "StreamCache.hpp"
#include "StreamProgress.hpp"
#include <algorithm>
#include <regex>
#include <fstream>
#include <cmath>
#include <future>
#include <array>
namespace udm {
static unsigned long long varint(const Bytes& b,size_t& at){unsigned long long n=0;for(unsigned i=0;i<10;++i){if(at>=b.size())throw std::runtime_error("Truncated streaming metadata.");auto v=b[at++];if(i==9&&v>1)throw std::runtime_error("Streaming integer overflow.");n|=(unsigned long long)(v&127)<<(7*i);if(!(v&128))return n;}throw std::runtime_error("Invalid streaming integer.");}
static void putVar(Bytes& b,unsigned long long n){while(n>127){b.push_back((unsigned char)((n&127)|128));n>>=7;}b.push_back((unsigned char)n);}
Proto Proto::parse(const Bytes& b){if(b.size()>262144)throw std::runtime_error("Streaming metadata is too large.");Proto p;size_t at=0;while(at<b.size()){auto tag=varint(b,at);if(tag>UINT32_MAX||(tag>>3)==0)throw std::runtime_error("Invalid streaming field.");Field f;f.id=(unsigned)(tag>>3);f.wire=(unsigned)(tag&7);if(f.wire==0)f.number=varint(b,at);else{size_t n;if(f.wire==2){auto length=varint(b,at);if(length>262144)throw std::runtime_error("Streaming field is too large.");n=(size_t)length;}else if(f.wire==1)n=8;else if(f.wire==5)n=4;else throw std::runtime_error("Unsupported streaming wire type.");if(n>b.size()-at)throw std::runtime_error("Truncated streaming field.");f.data.assign(b.begin()+at,b.begin()+at+n);at+=n;}p.fields.push_back(std::move(f));if(p.fields.size()>8192)throw std::runtime_error("Too many streaming fields.");}return p;}
Bytes Proto::encode()const{Bytes b;for(auto& f:fields){putVar(b,((unsigned long long)f.id<<3)|f.wire);if(!f.wire)putVar(b,f.number);else{if(f.wire==2)putVar(b,f.data.size());b.insert(b.end(),f.data.begin(),f.data.end());}}return b;}
bool Proto::has(unsigned id)const{return std::any_of(fields.begin(),fields.end(),[&](const Field& f){return f.id==id;});}
unsigned long long Proto::number(unsigned id,unsigned long long fallback)const{for(auto it=fields.rbegin();it!=fields.rend();++it)if(it->id==id&&it->wire==0)return it->number;return fallback;}
Bytes Proto::data(unsigned id)const{for(auto it=fields.rbegin();it!=fields.rend();++it)if(it->id==id&&it->wire==2)return it->data;return {};}
std::string Proto::text(unsigned id)const{auto b=data(id);return std::string(b.begin(),b.end());}
Proto& Proto::remove(std::initializer_list<unsigned> ids){fields.erase(std::remove_if(fields.begin(),fields.end(),[&](const Field& f){return std::find(ids.begin(),ids.end(),f.id)!=ids.end();}),fields.end());return *this;}
Proto& Proto::add(unsigned id,const Bytes& b){fields.push_back({id,2,0,b});return *this;}
Proto& Proto::set(unsigned id,const Bytes& b){remove({id});return add(id,b);}
Proto& Proto::set(unsigned id,const std::string& s){return set(id,Bytes(s.begin(),s.end()));}
Proto& Proto::set(unsigned id,unsigned long long n){remove({id});fields.push_back({id,0,n,{}});return *this;}
Proto& Proto::floating(unsigned id,float n){remove({id});Bytes b(sizeof(n));memcpy(b.data(),&n,sizeof(n));fields.push_back({id,5,0,b});return *this;}
std::vector<unsigned long long> Proto::numbers(unsigned id)const{std::vector<unsigned long long> r;for(auto& f:fields)if(f.id==id){if(f.wire==0)r.push_back(f.number);else if(f.wire==2){size_t at=0;while(at<f.data.size())r.push_back(varint(f.data,at));}}return r;}
unsigned umpInteger(const Bytes& bytes){if(bytes.empty())throw std::runtime_error("Missing UMP integer.");unsigned first=bytes[0],count=first<128?1:first<192?2:first<224?3:first<240?4:5;if(bytes.size()!=count)throw std::runtime_error("Invalid UMP integer.");if(count==5){unsigned n=0;memcpy(&n,bytes.data()+1,4);return n;}unsigned bits=8-count,n=first&((1u<<bits)-1),multiplier=1u<<bits;for(unsigned i=1;i<count;++i){n+=bytes[i]*multiplier;multiplier*=256;}return n;}
Bytes formatIdentity(const Json& f){auto id=str(f,"id"),lmt=str(f,"lastModified");if(!std::regex_match(id,std::regex("[0-9]{1,6}"))||!std::regex_match(lmt,std::regex("[0-9]{1,20}"))||std::stoull(id)==0||std::stoull(lmt)==0)throw std::runtime_error("The browser did not capture a complete stream identity.");auto p=Proto().set(1,std::stoull(id)).set(2,std::stoull(lmt));auto tags=str(f,"xtags");if(tags.size()>2048)throw std::runtime_error("Invalid stream tags.");if(!tags.empty())p.set(3,tags);return p.encode();}
bool sameFormat(const Bytes& a,const Bytes& b){auto x=Proto::parse(a),y=Proto::parse(b);return x.number(1)==y.number(1)&&x.number(2)==y.number(2)&&x.text(3)==y.text(3);}
void validateSabr(const Json& s,const std::string& source,bool audioOnly){validateSource(source);validateStream(str(s,"url"));Url u(source);auto q=query(source);auto id=u.host=="youtu.be"?u.path.substr(1):q["v"];if(id!=str(s,"videoId")||!std::regex_match(id,std::regex("[A-Za-z0-9_-]{11}")))throw std::runtime_error("The captured stream belongs to a different video.");if(num(s,"durationMs")<=0||num(s,"durationMs")>86400000)throw std::runtime_error("Only recorded videos up to 24 hours are supported.");double when=real(s,"capturedAt");if(!std::isfinite(when)||when<epoch()-300000||when>epoch()+30000)throw std::runtime_error("The browser streaming session expired. Refresh the video and capture it again.");auto body=str(s,"body");if(body.empty()||body.size()>175000)throw std::runtime_error("Invalid streaming request.");auto b=unb64(body);if(b.empty()||b.size()>131072)throw std::runtime_error("Invalid streaming request.");auto p=Proto::parse(b);if(p.data(5).empty()||p.data(19).empty())throw std::runtime_error("Incomplete browser streaming session.");if(!s.contains("audio")||str(s["audio"],"mime").rfind("audio/mp4",0)!=0)throw std::runtime_error("Choose an MP4 audio format.");formatIdentity(s["audio"]);if(audioOnly){if(s.contains("video")||str(s["audio"],"mime").find("mp4a")==std::string::npos)throw std::runtime_error("Audio-only streaming requires one AAC audio track and no video.");}else{if(!s.contains("video")||str(s["video"],"mime").rfind("video/mp4",0)!=0)throw std::runtime_error("Choose an MP4 video format.");formatIdentity(s["video"]);}}
namespace {
struct SabrSeekUnsupported:std::runtime_error{using runtime_error::runtime_error;};
struct Track {bool video=false;Json format;Bytes identity;JobPtr child;bool initialized=false,hasInit=false;i64 startTime=0,endTime=0,expectedEnd=0,endSequence=-1,rangeStart=0,rangeEnd=0;std::map<int,std::pair<i64,i64>> times;int last=-1,first=-1;std::map<int,fs::path> files;fs::path init;bool complete()const{return hasInit&&!files.empty()&&endTime>=std::min(rangeEnd,expectedEnd)-(rangeEnd>=expectedEnd?100:0)&&(rangeEnd<expectedEnd||endSequence<0||last>=endSequence);}Bytes buffered()const{return Proto().set(1,identity).set(2,(unsigned long long)startTime).set(3,(unsigned long long)std::max<i64>(0,endTime-startTime)).set(4,(unsigned long long)std::max(0,first)).set(5,(unsigned long long)std::max(0,last)).set(6,Proto().set(1,(unsigned long long)startTime).set(2,(unsigned long long)std::max<i64>(0,endTime-startTime)).set(3,1000ULL).encode()).encode();}};
struct Pending {Track* track=nullptr;Handle file;fs::path path;bool init=false,skip=false;int sequence=0;i64 start=0,duration=0,expected=0,received=0;};
struct Reader {StreamRead read;const Cancel& cancel;Bytes buffer;size_t at=0;int byte(){if(at==buffer.size()){buffer.resize(65536);auto n=read(buffer.data(),buffer.size(),cancel);buffer.resize(n);at=0;if(!n)return -1;}return buffer[at++];}Bytes exact(size_t n){Bytes out;out.reserve(n);while(n){if(at==buffer.size()){int first=byte();if(first<0)throw std::runtime_error("Truncated UMP part.");out.push_back((unsigned char)first);--n;}else{auto count=std::min(n,buffer.size()-at);out.insert(out.end(),buffer.begin()+at,buffer.begin()+at+count);at+=count;n-=count;}}return out;}std::optional<unsigned> integer(){int first=byte();if(first<0)return {};unsigned count=first<128?1:first<192?2:first<224?3:first<240?4:5;auto b=exact(count-1);b.insert(b.begin(),(unsigned char)first);return umpInteger(b);}};
}
static void sabrTransferImpl(Manager& m,JobPtr job,const std::shared_ptr<Cancel>& c,StreamTransport transport,int forcedConnections){Json j,prefs;Headers headers;{Lock l(m.mutex);j=job->data;prefs=m.state["Settings"];headers=readHeaders(j);}const auto offer=Json::parse(reveal(str(j,"ProtectedSabr")));const bool audioOnly=str(j,"MediaOutput")=="audio";validateSabr(offer,str(j,"SourceUrl"),audioOnly);if(!job->audio||(audioOnly?bool(job->video):!job->video))throw std::runtime_error("Captured streaming tracks are missing or inconsistent.");const int trackCount=audioOnly?1:2;const i64 duration=num(offer,"durationMs");StreamCache cache(mediaWorkingDirectory(m,job),offer,audioOnly,*c);
 // Independent timeline windows own their protocol cookies, contexts and files.
 // Short clips avoid more workers than useful 30-second windows.
 const int count=forcedConnections?forcedConnections:(int)std::min<i64>(std::clamp<i64>(num(j,"Connections",8),1,8),std::max<i64>(1,duration/30000));
 auto group=std::make_shared<Cancel>();group->parent=c;
 Rate rate;std::atomic<int> requestSerial{0};
 std::vector<std::vector<Track>> lanes(count,std::vector<Track>(trackCount));
 const auto progressStarted=std::chrono::steady_clock::now();
 {Lock lock(m.mutex);
  job->workers.assign(count,{});job->data["SabrConnections"]=count;if(!forcedConnections)job->data.erase("SabrFallbackReason");
  job->data["SabrActiveRequests"]=0;job->data["SabrMaxConcurrentRequests"]=0;job->data["SabrRequestCount"]=0;
  job->data["SabrRanges"]=Json::array();job->data["Size"]=-1;job->data["RangeSupported"]=false;
  job->data["SabrResumeSupported"]=true;job->data["SabrReusedBytes"]=cache.bytes();job->data["SabrRetainedBytes"]=cache.bytes();job->data["MediaTracksReady"]=false;job->data["MediaPhase"]="Receiving streams";job->data["StreamDurationMs"]=duration;job->data["StreamCompletedMs"]=0;job->data["StreamElapsedSeconds"]=0;
  for(auto child:{job->video,job->audio})if(child){child->data["Received"]=cache.bytes(child==job->video?"video":"audio");child->data["Size"]=-1;child->data["Status"]="Downloading";child->workers.assign(count,{});}
  for(int w=0;w<count;++w){
   const i64 start=duration*w/count,end=duration*(w+1)/count;
   job->workers[w]={w+1,start,end-1,start,0,"Connecting: "+mediaClock(start)+" - "+mediaClock(end)};
   job->data["SabrRanges"].push_back({{"Start",start},{"End",end-1},{"Done",0}});
   for(int i=0;i<trackCount;++i){auto& t=lanes[w][i];t.video=!audioOnly&&i==0;t.format=offer[t.video?"video":"audio"];t.identity=formatIdentity(t.format);t.child=t.video?job->video:job->audio;
    t.expectedEnd=duration;t.rangeStart=t.startTime=t.endTime=start;t.rangeEnd=end;
    fs::create_directories(t.child->target().parent_path());
    t.child->workers[w]={w+1,start,end-1,start,0,t.video?"Video: waiting for media":"Audio: waiting for media"};
    const auto kind=t.video?"video":"audio";auto meta=cache.metadata(kind);auto saved=cache.parts(kind);
    if(meta.contains("duration")&&num(meta,"duration")>0&&std::abs(num(meta,"duration")-duration)<=5000&&saved.count(-1)){
     t.initialized=t.hasInit=true;t.init=saved[-1].second;t.expectedEnd=num(meta,"duration");t.endSequence=num(meta,"endSequence",-1);
     for(auto& entry:saved){if(entry.first<0)continue;const auto at=num(entry.second.first,"start"),length=num(entry.second.first,"duration");
      if(t.last<0){if(at>start+250)break;if(length<=0||at+length<=start)continue;}
      else if(entry.first!=t.last+1||std::abs(at-t.endTime)>250)break;
      if(t.first<0){t.first=entry.first;t.startTime=at;}t.last=entry.first;t.endTime=at+length;t.files[entry.first]=entry.second.second;t.times[entry.first]={at,length};
      if(t.complete())break;
     }
    }
   }
  }
 }
 {Lock lock(m.mutex);i64 resumed=0;for(auto& lane:lanes){i64 covered=lane[0].rangeEnd;for(auto& t:lane)covered=std::min(covered,std::min(t.rangeEnd,t.endTime));resumed+=covered-lane[0].rangeStart;}job->data["StreamResumedMs"]=resumed;}
 auto cleanupTracks=[&]{for(auto& lane:lanes)for(auto& t:lane){std::error_code ec;auto staging=t.child->target();staging+=L".assembling";fs::remove(staging,ec);}};
 auto work=[&](int worker){auto& tracks=lanes[worker];const auto& c=group;
 auto complete=[&]{return std::all_of(tracks.begin(),tracks.end(),[](const Track& t){return t.complete();});};
 auto endSum=[&]{i64 total=0;for(auto& t:tracks)total+=t.endTime;return total;};
 auto nextTime=[&]{i64 time=duration;for(auto& t:tracks)time=std::min(time,t.endTime);return (unsigned long long)time;};
 auto original=Proto::parse(unb64(str(offer,"body")));auto context=Proto::parse(original.data(19));auto cookie=context.data(3);std::map<unsigned long long,Bytes> contexts;std::set<unsigned long long> active;for(auto f:context.fields)if(f.id==5&&f.wire==2){auto v=Proto::parse(f.data);if(v.has(2)){contexts[v.number(1)]=v.data(2);active.insert(v.number(1));}}std::string endpoint=str(offer,"url");std::map<int,std::unique_ptr<Pending>> pending;int requestNumber=0,backoff=0,stalled=0;
 const auto session=transport?std::shared_ptr<HttpSession>{}:std::make_shared<HttpSession>(prefs);
 auto publishProgress=[&]{Lock lock(m.mutex);
  i64 covered=tracks[0].rangeEnd;
  for(auto& t:tracks){auto& row=t.child->workers[worker];
   auto end=std::min(t.rangeEnd,t.expectedEnd);
   row.position=t.complete()?t.rangeEnd:std::clamp(t.endTime,t.rangeStart,end);
   row.state=std::string(t.video?"Video: ":"Audio: ")+(t.complete()?"received ":"receiving ")+mediaClock(row.position)+" / "+mediaClock(end);
   covered=std::min(covered,row.position);
  }
  auto& row=job->workers[worker];row.position=covered;
  row.state=std::string(complete()?"Received: ":"Receiving: ")+mediaClock(row.start)+" - "+mediaClock(row.end+1);
  job->data["SabrRanges"][worker]["Done"]=covered-row.start;
  i64 done=0;for(const auto& r:job->workers)done+=r.position-r.start;
  job->data["StreamCompletedMs"]=done;job->data["StreamElapsedSeconds"]=std::chrono::duration<double>(std::chrono::steady_clock::now()-progressStarted).count();
 };
 auto identity=[&](const std::string& video){if(!video.empty()&&video!=str(offer,"videoId"))throw std::runtime_error("Streaming response belongs to another video. Output was not published.");};auto find=[&](const Bytes& id)->Track*{for(auto& t:tracks)if(sameFormat(t.identity,id))return &t;return nullptr;};auto timing=[](const Proto& p,unsigned field,unsigned rangeField)->i64{if(p.has(field)){auto v=p.number(field);if(v>LLONG_MAX)throw std::runtime_error("Streaming timing overflow.");return (i64)v;}auto range=Proto::parse(p.data(15));auto scale=range.number(3);auto n=range.number(rangeField);if(n>(unsigned long long)LLONG_MAX/1000)throw std::runtime_error("Streaming timing overflow.");return scale?(i64)(1000*n/scale):0;};
 auto cleanup=[&]{for(auto& [id,p]:pending){if(p->file){CloseHandle(p->file.h);p->file.h=INVALID_HANDLE_VALUE;}if(!p->path.empty()){std::error_code ec;fs::remove(p->path,ec);}}pending.clear();};
 publishProgress();
 try{while(!complete()){c->check();if(requestNumber>20000)throw std::runtime_error("Streaming request limit reached.");i64 before=endSum();auto request=original;request.remove({2,3,16,17,18});for(auto& t:tracks){request.add(t.video?17:16,t.identity);if(t.initialized)request.add(2,t.identity);if(!t.files.empty())request.add(3,t.buffered());}auto time=nextTime();request.set(4,time);auto player=Proto::parse(request.data(1));player.set(16,(unsigned long long)num(j,"MediaHeight")).set(21,1ULL).set(22,0ULL).set(26,3ULL).set(28,time).set(40,3ULL).floating(35,1);request.set(1,player.encode());auto ctx=Proto::parse(request.data(19));ctx.remove({5,6});if(!cookie.empty())ctx.set(3,cookie);for(auto& [id,data]:contexts){if(active.count(id))ctx.add(5,Proto().set(1,id).set(2,data).encode());else ctx.fields.push_back({6,0,id,{}});}request.set(19,ctx.encode());auto body=request.encode();if(body.size()>262144)throw std::runtime_error("Streaming context is too large.");auto address=std::regex_replace(endpoint,std::regex("([?&])rn=[^&]*&?"),"$1");address+=(address.find('?')==std::string::npos?"?":"&");address+="rn="+std::to_string(requestSerial++);++requestNumber;Headers h;for(auto& [key,value]:headers)if(lower(key)=="user-agent"||lower(key)=="referer")h[key]=value;{struct ActiveRequest {
  Manager& m;JobPtr job;
  ActiveRequest(Manager& manager,JobPtr parent):m(manager),job(parent){Lock lock(m.mutex);auto n=num(job->data,"SabrActiveRequests")+1;job->data["SabrActiveRequests"]=n;job->data["SabrMaxConcurrentRequests"]=std::max(n,num(job->data,"SabrMaxConcurrentRequests"));job->data["SabrRequestCount"]=num(job->data,"SabrRequestCount")+1;}
  ~ActiveRequest(){Lock lock(m.mutex);job->data["SabrActiveRequests"]=num(job->data,"SabrActiveRequests")-1;}
 } activeRequest(m,job);
 std::unique_ptr<Http> response;StreamRead read;if(transport)read=transport(address,body);else{response=std::make_unique<Http>(address,h,prefs,*c,std::nullopt,std::nullopt,"",&body,false,session);if(response->status<200||response->status>=300)throw std::runtime_error("HTTP "+std::to_string(response->status)+": the server rejected the browser streaming session.");auto content=response->header(L"Content-Type");if(content.substr(0,content.find(';'))!="application/vnd.yt-ump")throw std::runtime_error("The streaming server returned an unexpected content type.");read=[&](void* bytes,size_t size,const Cancel& cancel){return response->read(bytes,size,cancel);};}Reader reader{read,*c,{},0};int parts=0;while(auto type=reader.integer()){if(++parts>100000)throw std::runtime_error("Too many streaming response parts.");auto length=reader.integer();if(!length||*length>32*1024*1024)throw std::runtime_error("Invalid UMP part size.");auto data=reader.exact(*length);if(*type==12)throw std::runtime_error("Encrypted media is not supported.");if(*type==21){if(data.size()<2||!pending.count(data[0]))throw std::runtime_error("Media arrived without a matching header.");auto& p=*pending[data[0]];if(!p.skip){if(p.received+(i64)data.size()-1>p.expected)throw std::runtime_error("Media exceeds its declared segment length.");for(size_t offset=1;offset<data.size();){auto n=std::min<size_t>(65536,data.size()-offset);m.charge(n,*c,rate,job);DWORD written=0;if(!WriteFile(p.file.h,data.data()+offset,(DWORD)n,&written,nullptr)){const DWORD code=GetLastError();throw std::runtime_error(mediaStorageError(code,p.path,"write streaming segment"));}if(written!=n)throw std::runtime_error(mediaStorageError(ERROR_WRITE_FAULT,p.path,"write the complete streaming segment"));offset+=n;p.received+=n;Lock l(m.mutex);p.track->child->data["TransferredBytes"]=num(p.track->child->data,"TransferredBytes")+n;p.track->child->data["Received"]=num(p.track->child->data,"Received")+n;p.track->child->workers[worker].received+=n;job->workers[worker].received+=n;}}continue;}
 if(*type==22){if(data.size()!=1||!pending.count(data[0]))throw std::runtime_error("Invalid media segment terminator.");auto p=std::move(pending[data[0]]);pending.erase(data[0]);if(p->skip)continue;if(p->received!=p->expected)throw std::runtime_error("Streaming segment is incomplete.");if(!FlushFileBuffers(p->file.h)){const DWORD code=GetLastError();throw std::runtime_error(mediaStorageError(code,p->path,"flush streaming segment"));}CloseHandle(p->file.h);p->file.h=INVALID_HANDLE_VALUE;auto& t=*p->track;if(p->init){t.init=cache.commit(t.video?"video":"audio",-1,0,0,p->path);t.hasInit=true;}else{if(t.last<0&&t.rangeStart>0&&p->duration>0&&p->start>=0&&p->start<=LLONG_MAX-p->duration&&(p->start>t.rangeStart+250||p->start+p->duration<=t.rangeStart)){std::error_code ec;fs::remove(p->path,ec);throw SabrSeekUnsupported("The server did not honor independent media sections; using one connection.");}if(p->duration<=0||p->start<0||p->start>LLONG_MAX-p->duration||(t.last<0?(p->start>t.rangeStart+250||p->start+p->duration<=t.rangeStart):(std::abs(p->start-t.endTime)>250||p->sequence!=t.last+1))){std::error_code ec;fs::remove(p->path,ec);throw std::runtime_error("Streaming response has a gap or an out-of-order segment.");}if(t.first<0){t.first=p->sequence;t.startTime=p->start;}t.last=p->sequence;t.endTime=p->start+p->duration;t.files[p->sequence]=cache.commit(t.video?"video":"audio",p->sequence,p->start,p->duration,p->path);t.times[p->sequence]={p->start,p->duration};}{Lock lock(m.mutex);job->data["SabrRetainedBytes"]=cache.bytes();}publishProgress();continue;}
 if(*type!=20&&*type!=35&&*type!=42&&*type!=43&&*type!=44&&*type!=46&&*type!=57&&*type!=58&&*type!=59)continue;auto p=Proto::parse(data);switch(*type){case 42:{if(p.text(1).empty())throw std::runtime_error("Streaming format did not identify the current video. Output was not published.");identity(p.text(1));auto id=p.data(2);if(id.empty())break;auto t=find(id);if(!t)break;if(p.text(5).rfind(t->video?"video/mp4":"audio/mp4",0)!=0)throw std::runtime_error("Streaming format changed unexpectedly.");t->initialized=true;if(p.number(3)>0)t->expectedEnd=(i64)p.number(3);else if(p.number(10)>0&&p.number(9)>0){if(p.number(9)>(unsigned long long)LLONG_MAX/1000)throw std::runtime_error("Streaming duration overflow.");t->expectedEnd=(i64)(1000*p.number(9)/p.number(10));}if(t->expectedEnd<=0||std::abs(t->expectedEnd-num(offer,"durationMs"))>5000)throw std::runtime_error("Streaming duration does not match the selected video.");if(p.has(4))t->endSequence=(i64)p.number(4);cache.metadata(t->video?"video":"audio",t->expectedEnd,t->endSequence);break;}
 case 20:{identity(p.text(2));auto id=p.data(13);if(id.empty())id=Proto().set(1,p.number(3)).set(2,p.number(4)).set(3,p.text(5)).encode();auto t=find(id);if(p.number(1)>255)throw std::runtime_error("Invalid media header identifier.");int header=(int)p.number(1);if(pending.count(header))throw std::runtime_error("Overlapping media segment identifiers.");if(p.number(7))throw std::runtime_error("Compressed streaming segments are not supported.");auto segment=std::make_unique<Pending>();if(!t){segment->skip=true;pending[header]=std::move(segment);break;}if(!t->initialized)throw std::runtime_error("Media arrived before format initialization.");segment->track=t;segment->init=p.number(8)!=0;if(p.number(9)>INT_MAX||p.number(14)==0||p.number(14)>64*1024*1024)throw std::runtime_error("Invalid media segment metadata.");segment->sequence=(int)p.number(9);segment->expected=(i64)p.number(14);segment->start=timing(p,11,1);segment->duration=timing(p,12,2);segment->skip=segment->init?t->hasInit:t->files.count(segment->sequence)!=0;if(!segment->skip){segment->path=t->child->target().parent_path()/wide(std::string(t->video?"video-":"audio-")+(count>1?"w"+std::to_string(worker+1)+"-":"")+(segment->init?"init":std::to_string(segment->sequence))+".sabrpart");segment->file.h=CreateFileW(segment->path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,0,nullptr);if(!segment->file){const DWORD code=GetLastError();throw std::runtime_error(mediaStorageError(code,segment->path,"create streaming segment"));}}pending[header]=std::move(segment);break;}
 case 35:identity(p.text(8));if(p.has(7))cookie=p.data(7);backoff=(int)std::min<unsigned long long>(60000,p.number(4));break;
 case 43:validateStream(p.text(1));endpoint=p.text(1);break;
 case 44:throw std::runtime_error("Streaming server error "+std::to_string(p.number(2))+". Refresh playback in the browser.");
 case 46:throw std::runtime_error("The streaming server requested a fresh browser session.");
 case 58:if(p.number(1)==3)throw std::runtime_error("The streaming server requires browser attestation. Refresh playback in the browser.");break;
 case 57:{auto id=p.number(1);if(p.has(3)&&!(p.number(5)==2&&contexts.count(id))){if(contexts.size()>=100&&!contexts.count(id))throw std::runtime_error("Too many streaming context updates.");contexts[id]=p.data(3);if(p.number(4))active.insert(id);}break;}
 case 59:for(auto id:p.numbers(1))active.insert(id);for(auto id:p.numbers(2))active.erase(id);for(auto id:p.numbers(3)){contexts.erase(id);active.erase(id);}break;
 }}}if(!pending.empty())throw std::runtime_error("Streaming response ended inside a media segment.");stalled=endSum()>before?0:stalled+1;if(stalled>=8)throw std::runtime_error("The browser streaming session stopped supplying media. Refresh playback and retry.");if(backoff&&(!complete()))c->wait(backoff);}
 }catch(...){cleanup();group->stop=true;throw;}
 };
 try{
  std::vector<std::future<void>> tasks;std::exception_ptr failure;
  try{for(int w=0;w<count;++w)tasks.push_back(std::async(std::launch::async,[&,w]{try{work(w);}catch(...){group->stop=true;throw;}}));}catch(...){group->stop=true;failure=std::current_exception();}
  for(auto& task:tasks)try{task.get();}catch(const Cancelled&){if(!failure)failure=std::current_exception();}catch(...){failure=std::current_exception();}
  c->check();if(failure)std::rethrow_exception(failure);
  // A seek can overlap a segment at a window boundary. Compare duplicate bytes,
  // then validate the complete ordered sequence before publishing either track.
  std::vector<std::vector<fs::path>> ordered(trackCount);
  for(int i=0;i<trackCount;++i){
   auto& first=lanes[0][i];if(!first.hasInit)throw std::runtime_error("Streaming response is missing required initialization.");
   const auto initHash=fileHash(first.init);std::map<int,std::pair<fs::path,std::pair<i64,i64>>> segments;
   for(auto& lane:lanes){auto& t=lane[i];
    if(!t.complete()||t.expectedEnd!=first.expectedEnd||t.endSequence!=first.endSequence||fileHash(t.init)!=initHash)throw std::runtime_error("Parallel streaming format or initialization changed.");
    for(auto& entry:t.files){auto timing=t.times.at(entry.first);auto existing=segments.find(entry.first);
     if(existing!=segments.end()){if(existing->second.second!=timing||fileHash(existing->second.first)!=fileHash(entry.second))throw std::runtime_error("Overlapping streaming segments contain different media.");}
     else segments.emplace(entry.first,std::make_pair(entry.second,timing));
    }
   }
   i64 end=0;int last=-1;ordered[i].push_back(first.init);
   for(auto& entry:segments){auto start=entry.second.second.first,length=entry.second.second.second;
    if(std::abs(start-end)>250||(last>=0&&entry.first!=last+1))throw std::runtime_error("Parallel streaming response has a gap or an out-of-order segment.");
    end=start+length;last=entry.first;ordered[i].push_back(entry.second.first);
   }
   if(segments.empty()||end<first.expectedEnd-100||(first.endSequence>=0&&last<first.endSequence))throw std::runtime_error("Parallel streaming response is incomplete.");
  }
  for(int i=0;i<trackCount;++i){auto child=lanes[0][i].child;auto staging=child->target();staging+=L".assembling";
   {std::ofstream out(staging,std::ios::binary|std::ios::trunc);
    for(auto& path:ordered[i]){std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("Streaming segment missing.");char b[65536];
     while(input){c->check();input.read(b,sizeof(b));out.write(b,input.gcount());}if(!input.eof())throw std::runtime_error("Cannot read streaming segment.");
    }out.flush();if(!out)throw std::runtime_error("Cannot assemble streaming tracks.");
   }
   c->check();if(!MoveFileExW(staging.c_str(),child->target().c_str(),MOVEFILE_WRITE_THROUGH|MOVEFILE_REPLACE_EXISTING))throw std::runtime_error("Cannot publish internal media track.");
   Lock lock(m.mutex);child->data["Size"]=fs::file_size(child->target());child->data["Received"]=child->data["Size"];child->data["Sha256"]=fileHash(child->target());child->data["Status"]="Complete";m.save();
  }
 }catch(const SabrSeekUnsupported& e){
  cleanupTracks();if(count==1)throw;c->check();
  {Lock lock(m.mutex);job->data["SabrFallbackReason"]=e.what();}
  sabrTransferImpl(m,job,c,transport,1);return;
 }catch(...){cleanupTracks();Lock lock(m.mutex);for(auto child:{job->video,job->audio})if(child){child->data["Status"]=c->cancelled()?"Paused":"Failed";for(auto& worker:child->workers)worker.state=c->cancelled()?"Paused":"Stopped";}for(auto& worker:job->workers)worker.state=c->cancelled()?"Paused":"Stopped";throw;}
 {Lock lock(m.mutex);job->data["MediaTracksReady"]=true;job->data["MediaPhase"]="Streams retained";m.save();}
 cleanupTracks();
}
void sabrTransfer(Manager& m,JobPtr job,const std::shared_ptr<Cancel>& c,StreamTransport transport){sabrTransferImpl(m,job,c,std::move(transport),0);}
}
