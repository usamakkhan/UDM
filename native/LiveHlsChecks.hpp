#pragma once
#include "LiveHls.hpp"
class LiveHlsFixture {
 SOCKET listener=INVALID_SOCKET;std::thread server;std::vector<std::thread> clients;std::atomic_bool stopping{false};
 static void sendAll(SOCKET socket,const std::string& text){size_t at=0;while(at<text.size()){int sent=::send(socket,text.data()+at,(int)std::min<size_t>(65536,text.size()-at),0);if(sent<=0)return;at+=(size_t)sent;}}
 void serve(SOCKET socket){
  try{DWORD timeout=3000;setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));std::string request;char buffer[4096];while(request.find("\r\n\r\n")==std::string::npos&&request.size()<32768){int count=recv(socket,buffer,sizeof(buffer),0);if(count<=0)break;request.append(buffer,count);}auto begin=request.find(' '),end=request.find(' ',begin+1);auto path=request.substr(begin+1,end-begin-1);requests++;
   std::string body,extra;int status=200;
   if(path=="/live.m3u8"){
    int poll=++polls;int first=mode=="rolling"?std::min(2,poll-1):0;int count=mode=="rolling"?std::min(6-first,poll+1):2;bool ended=(mode=="rolling"&&poll>=3)||mode=="ended"||mode=="busy-playlist"||mode=="busy-segment";
    body="#EXTM3U\n#EXT-X-TARGETDURATION:2\n#EXT-X-MEDIA-SEQUENCE:"+std::to_string(first)+"\n";
    for(int index=first;index<first+count;++index){if(mode=="gap"&&index==1)body+="#EXT-X-GAP\n";body+="#EXTINF:2,\nsegment"+std::to_string(index)+".ts\n";}if(ended)body+="#EXT-X-ENDLIST\n";
    extra="Content-Type: application/vnd.apple.mpegurl\r\n";if(mode=="expired"&&poll>1){status=403;body.clear();}
    if(mode=="busy-playlist"&&poll==1){status=503;extra+="Retry-After: 1\r\n";body.clear();}
    if((mode=="stall-headers"||mode=="stall-body")&&poll>1){++stalled;if(mode=="stall-body")sendAll(socket,"HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\n");char ignored;recv(socket,&ignored,1,0);shutdown(socket,SD_BOTH);closesocket(socket);return;}
   }else if(path.rfind("/segment",0)==0&&path.size()==12&&path[8]>='0'&&path[8]<='5'){
    auto index=(size_t)(path[8]-'0');body=media[index];auto active=++concurrent;auto seen=peak.load();while(active>seen&&!peak.compare_exchange_weak(seen,active)){}Sleep(80);--concurrent;segments++;extra="Content-Type: video/mp2t\r\n";
    if(mode=="busy-segment"&&index==0&&++busySegments==1){status=503;extra+="Retry-After: 1\r\n";body.clear();}
   }else{status=404;body="missing";}
   sendAll(socket,"HTTP/1.1 "+std::to_string(status)+(status==200?" OK\r\n":" Error\r\n")+extra+"Content-Length: "+std::to_string(body.size())+"\r\nConnection: close\r\n\r\n"+body);
  }catch(...){}shutdown(socket,SD_BOTH);closesocket(socket);
 }
public:
 unsigned short port=0;std::vector<std::string> media;std::string mode="rolling";std::atomic_int polls{0},segments{0},requests{0},concurrent{0},peak{0},busySegments{0},stalled{0};
 explicit LiveHlsFixture(std::vector<std::string> content,std::string behavior="rolling"):media(std::move(content)),mode(std::move(behavior)){
  listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(listener==INVALID_SOCKET)throw std::runtime_error("Live fixture socket failed.");sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(bind(listener,(sockaddr*)&address,sizeof(address))||listen(listener,32))throw std::runtime_error("Live fixture bind failed.");int size=sizeof(address);getsockname(listener,(sockaddr*)&address,&size);port=ntohs(address.sin_port);
  server=std::thread([this]{while(!stopping){fd_set sockets;FD_ZERO(&sockets);FD_SET(listener,&sockets);timeval timeout{0,100000};if(select(0,&sockets,nullptr,nullptr,&timeout)>0){auto client=accept(listener,nullptr,nullptr);if(client!=INVALID_SOCKET)clients.emplace_back([this,client]{serve(client);});}}});
 }
 ~LiveHlsFixture(){stopping=true;if(server.joinable())server.join();closesocket(listener);for(auto& client:clients)if(client.joinable())client.join();}
 std::string url(const std::string& path)const{return "http://127.0.0.1:"+std::to_string(port)+path;}
};
static void liveHlsChecks(const fs::path& root){
 auto tools=appDir()/L"tools",source=root/L"live-source";fs::create_directories(source);Cancel cancel;
 execute(tools/L"ffmpeg.exe",{L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y",L"-f",L"lavfi",L"-i",L"testsrc2=size=320x180:rate=10",L"-f",L"lavfi",L"-i",L"sine=frequency=440:sample_rate=48000",L"-t",L"12",L"-c:v",L"libx264",L"-preset",L"ultrafast",L"-g",L"20",L"-sc_threshold",L"0",L"-c:a",L"aac",L"-b:a",L"64k",L"-f",L"hls",L"-hls_time",L"2",L"-hls_list_size",L"0",L"-hls_segment_filename",(source/L"segment%d.ts").wstring(),(source/L"source.m3u8").wstring()},40,cancel);
 std::vector<std::string> media;for(int index=0;index<6;++index)media.push_back(readText(source/(L"segment"+std::to_wstring(index)+L".ts")));
 Manager manager(root/L"live-native-catalog");manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"]["DownloadFolder"]=utf8((root/L"live-outputs").wstring());manager.state["Settings"]["Retries"]=0;
 auto create=[&](LiveHlsFixture& server,const std::string& name){Json plan={{"type","hls"},{"live",true},{"height",180},{"audioExpected",true},{"container","mp4"},{"tracks",Json::array({{{"kind","video"},{"playlist",server.url("/live.m3u8")},{"segments",Json::array({{{"url",server.url("/segment0.ts")}}})}}})}};return manager.receive({{"action","adaptive"},{"url",server.url("/player")},{"filename",name},{"plan",plan},{"originCookies",Json::object()}});};
 auto probe=[&](JobPtr job){return Json::parse(execute(tools/L"ffprobe.exe",{L"-v",L"error",L"-show_entries",L"format=duration:stream=codec_type,height",L"-of",L"json",job->target().wstring()},20,cancel));};
 {
  LiveHlsFixture server(media);auto job=create(server,"live-endlist");check(yes(job->data,"LiveRecording")&&str(job->data,"Status")=="Awaiting confirmation","Browser live-HLS handoff creates an unconfirmed native recording");auto control=std::make_shared<Cancel>();control->deadline=GetTickCount64()+20000;adaptiveTransfer(manager,job,control);
  auto metadata=probe(job);bool video=false,audio=false;for(const auto& stream:metadata["streams"]){video|=str(stream,"codec_type")=="video"&&num(stream,"height")==180;audio|=str(stream,"codec_type")=="audio";}
  check(str(job->data,"Status")=="Complete"&&fileHash(job->target())==str(job->data,"Sha256")&&video&&audio&&std::stod(str(metadata["format"],"duration"))>=11.9,"A rolling HLS stream publishes verified video and audio after ENDLIST");
  check(server.polls==3&&server.segments==6&&server.peak>=2&&num(job->data,"AdaptiveCompletedSegments")==6,"Live HLS downloads new segments in parallel without duplicating overlapping windows");
  execute(tools/L"ffmpeg.exe",{L"-v",L"error",L"-i",job->target().wstring(),L"-f",L"null",L"-"},20,cancel);check(true,"Recorded live HLS output fully decodes");
  check(fs::is_empty(mediaWorkingDirectory(manager,job)/L"live-hls"),"Successful live publication removes its recording cache");
 }
 manager.state["Queues"][0]["Retries"]=2;
 for(const std::string mode:{"busy-playlist","busy-segment"}){
  LiveHlsFixture server(media,mode);auto job=create(server,"live-"+mode);auto started=GetTickCount64();adaptiveTransfer(manager,job,std::make_shared<Cancel>());
  check(str(job->data,"Status")=="Complete"&&GetTickCount64()-started>=1000&&(mode=="busy-playlist"?server.polls==2:server.busySegments==2),"Live capture honors Retry-After and recovers a temporary playlist or segment rejection");
 }
 {
  LiveHlsFixture server(media,"expired");auto job=create(server,"live-no-forbidden-retry");rejects([&]{adaptiveTransfer(manager,job,std::make_shared<Cancel>());},"Live capture reports a permanent source rejection with retries enabled");
  check(server.polls==2&&real(job->data,"LiveCapturedSeconds")==4,"A forbidden live playlist is not retried and its captured media remains saveable");
 }
 for(const std::string mode:{"stall-headers","stall-body"}){
  LiveHlsFixture server(media,mode);auto job=create(server,"live-"+mode);job->data["Status"]="Downloading";auto control=std::make_shared<Cancel>();control->deadline=GetTickCount64()+15000;
  auto task=std::async(std::launch::async,[&]{adaptiveTransfer(manager,job,control);});auto limit=GetTickCount64()+6000;while(!server.stalled&&GetTickCount64()<limit)Sleep(10);
  bool stalled=server.stalled>0;auto start=GetTickCount64();if(stalled)requestLiveHlsFinish(manager,job);else control->stop=true;task.get();
  check(stalled&&GetTickCount64()-start<1500&&str(job->data,"Status")=="Complete","Stop and save cancels stalled live HTTP headers or body and publishes promptly");
 }
 {
  LiveHlsFixture server(media,"hold");auto job=create(server,"live-pause-restart");job->data["Status"]="Downloading";auto control=std::make_shared<Cancel>();control->deadline=GetTickCount64()+10000;
  auto task=std::async(std::launch::async,[&]{try{adaptiveTransfer(manager,job,control);return false;}catch(const Cancelled&){return true;}});auto limit=GetTickCount64()+5000;bool ready=false;
  while(GetTickCount64()<limit){{Lock lock(manager.mutex);ready=real(job->data,"LiveCapturedSeconds")>=4;}if(ready)break;Sleep(10);}control->stop=true;
  check(task.get()&&ready&&!fs::exists(job->target()),"Pausing a live capture retains verified segments without publishing a partial file");job->data["Status"]="Paused";manager.save();
  Manager restored(manager.root);JobPtr saved;for(auto candidate:restored.jobs)if(candidate->id()==job->id())saved=candidate;auto before=server.requests.load();requestLiveHlsFinish(restored,saved);adaptiveTransfer(restored,saved,std::make_shared<Cancel>());
  check(str(saved->data,"Status")=="Complete"&&server.requests==before&&fileHash(saved->target())==str(saved->data,"Sha256"),"Restarted live recording saves its durable prefix without contacting the source");
 }
 {
  LiveHlsFixture server(media,"ended");auto job=create(server,"live-publish-conflict");job->data["LiveRecording"]=false;fs::create_directories(job->target().parent_path());writeBytes(job->target(),Bytes{'k','e','e','p'});
  rejects([&]{adaptiveTransfer(manager,job,std::make_shared<Cancel>());},"A live recording refuses to overwrite a destination created while downloading");
  check(yes(job->data,"LiveRecording")&&readText(job->target())=="keep"&&fs::exists(mediaWorkingDirectory(manager,job)/L"live-hls"/L"recording.json"),"Protected live selection controls dispatch and a publication conflict preserves both files and receipts");
  job->data["FileName"]="live-publish-recovered.mp4";job->data["Status"]="Failed";auto requests=server.requests.load();adaptiveTransfer(manager,job,std::make_shared<Cancel>());
  check(str(job->data,"Status")=="Complete"&&server.requests==requests&&fs::is_empty(mediaWorkingDirectory(manager,job)/L"live-hls"),"Publication retry uses sealed live media offline and cleans it only after success");
 }
 {
  LiveHlsFixture server(media,"hold");auto job=create(server,"live-stop-save");job->data["Status"]="Downloading";auto control=std::make_shared<Cancel>();control->deadline=GetTickCount64()+15000;
  auto recording=std::async(std::launch::async,[&]{adaptiveTransfer(manager,job,control);});bool ready=false;auto limit=GetTickCount64()+6000;
  while(GetTickCount64()<limit){if(recording.wait_for(std::chrono::milliseconds(0))==std::future_status::ready)break;{Lock lock(manager.mutex);ready=real(job->data,"LiveCapturedSeconds")>=4;}if(ready)break;Sleep(20);}
  check(ready,"Live recording exposes a durable playable prefix while waiting for more media");
  if(ready)requestLiveHlsFinish(manager,job);else control->stop=true;recording.get();auto metadata=probe(job);auto seconds=std::stod(str(metadata["format"],"duration"));
  check(str(job->data,"Status")=="Complete"&&yes(job->data,"LiveStopRequested")&&seconds>=3.9&&seconds<5,"Stop and save publishes the captured live portion before ENDLIST");
  check(server.segments==2&&job->liveCapture==nullptr,"Stop and save releases live network control without downloading another window");
 }
 {
  LiveHlsFixture server(media,"expired");auto job=create(server,"live-expired-save");auto control=std::make_shared<Cancel>();control->deadline=GetTickCount64()+15000;
  rejects([&]{adaptiveTransfer(manager,job,control);},"An expired live playlist stops capture and preserves its media");check(real(job->data,"LiveCapturedSeconds")==4&&!fs::exists(job->target()),"Expired live capture retains a saveable prefix without claiming completion");
  job->data["Status"]="Failed";requestLiveHlsFinish(manager,job);auto requests=server.requests.load();adaptiveTransfer(manager,job,std::make_shared<Cancel>());
  check(str(job->data,"Status")=="Complete"&&server.requests==requests,"A retained live recording saves offline after its source expires");
 }
 {
  LiveHlsFixture server(media,"gap");auto job=create(server,"live-gap-save");rejects([&]{adaptiveTransfer(manager,job,std::make_shared<Cancel>());},"An explicit live-media gap halts capture instead of silently skipping content");
  check(real(job->data,"LiveCapturedSeconds")==2&&server.segments==1,"Live HLS saves the valid segment preceding an unavailable segment");job->data["Status"]="Failed";requestLiveHlsFinish(manager,job);adaptiveTransfer(manager,job,std::make_shared<Cancel>());check(str(job->data,"Status")=="Complete","Stop and save publishes the intact prefix before a live gap");
 }
 auto invalid=Json{{"type","hls"},{"live",true},{"height",180},{"tracks",Json::array({{{"kind","video"},{"playlist","file:///C:/secret"},{"segments",Json::array({{{"url","https://media.example.test/segment.ts"}}})}}})}};
 rejects([&]{validateAdaptive(invalid);},"Live browser plans reject nonnetwork playlists");
 invalid["tracks"][0]["playlist"]="https://media.example.test/live.m3u8";invalid["type"]="dash";rejects([&]{validateAdaptive(invalid);},"DASH plans cannot enter the HLS live recorder");
}
