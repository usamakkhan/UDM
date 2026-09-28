#pragma once
// Local real-media acceptance for separately clocked renditions and encoder
// restarts. This server exposes only its immutable in-memory fixture map.
class LiveTracksFixture {
 SOCKET listener=INVALID_SOCKET;std::thread server;std::vector<std::thread> clients;std::atomic_bool stopping{false};
 std::map<std::string,std::string> files;
 void serve(SOCKET socket){
  try{DWORD timeout=3000;setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));std::string request;char buffer[4096];while(request.find("\r\n\r\n")==std::string::npos&&request.size()<32768){int count=recv(socket,buffer,sizeof(buffer),0);if(count<=0)break;request.append(buffer,count);}auto begin=request.find(' '),end=request.find(' ',begin+1);auto path=request.substr(begin+1,end-begin-1);bool found=files.count(path)!=0;std::string body=found?files.at(path):"missing";
   auto response=std::string(found?"HTTP/1.1 200 OK\r\n":"HTTP/1.1 404 Not Found\r\n")+"Content-Length: "+std::to_string(body.size())+"\r\nConnection: close\r\n\r\n"+body;size_t at=0;while(at<response.size()){int sent=send(socket,response.data()+at,(int)std::min<size_t>(65536,response.size()-at),0);if(sent<=0)break;at+=(size_t)sent;}
  }catch(...){}shutdown(socket,SD_BOTH);closesocket(socket);
 }
public:
 unsigned short port=0;
 explicit LiveTracksFixture(std::map<std::string,std::string> content):files(std::move(content)){
  listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(listener==INVALID_SOCKET)throw std::runtime_error("Live track fixture socket failed.");sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(bind(listener,(sockaddr*)&address,sizeof(address))||listen(listener,32))throw std::runtime_error("Live track fixture bind failed.");int size=sizeof(address);getsockname(listener,(sockaddr*)&address,&size);port=ntohs(address.sin_port);
  server=std::thread([this]{while(!stopping){fd_set sockets;FD_ZERO(&sockets);FD_SET(listener,&sockets);timeval timeout{0,100000};if(select(0,&sockets,nullptr,nullptr,&timeout)>0){auto client=accept(listener,nullptr,nullptr);if(client!=INVALID_SOCKET)clients.emplace_back([this,client]{serve(client);});}}});
 }
 ~LiveTracksFixture(){stopping=true;if(server.joinable())server.join();closesocket(listener);for(auto& client:clients)if(client.joinable())client.join();}
 std::string url(const std::string& path)const{return "http://127.0.0.1:"+std::to_string(port)+path;}
};
static void liveHlsTracksChecks(const fs::path& root){
 auto tools=appDir()/L"tools",source=root/L"live-track-source";fs::create_directories(source);Cancel cancel;
 auto generate=[&](const std::wstring& name,bool audio,const std::wstring& level){
  auto folder=source/name;fs::create_directories(folder);std::vector<std::wstring> args={L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y",L"-f",L"lavfi",L"-i",audio?L"sine=frequency=440:sample_rate=48000":L"testsrc2=size=320x180:rate=10",L"-t",L"4"};
  if(audio)args.insert(args.end(),{L"-c:a",L"aac",L"-b:a",L"64k"});else args.insert(args.end(),{L"-c:v",L"libx264",L"-preset",L"ultrafast",L"-level:v",level,L"-g",L"20",L"-sc_threshold",L"0"});
  // FFmpeg's HLS muxer locates its relative init file using '/' separators.
  args.insert(args.end(),{L"-f",L"hls",L"-hls_time",L"2",L"-hls_list_size",L"0",L"-hls_segment_type",L"fmp4",L"-hls_fmp4_init_filename",L"init.mp4",L"-hls_segment_filename",(folder/L"part%d.m4s").generic_wstring(),(folder/L"source.m3u8").generic_wstring()});execute(tools/L"ffmpeg.exe",args,30,cancel);if(!fs::exists(folder/L"init.mp4"))throw std::runtime_error("Generated live fixture initialization is missing.");
 };
 generate(L"video",false,L"3.0");generate(L"changed",false,L"3.1");generate(L"audio",true,L"");
 std::map<std::string,std::string> files;for(const std::wstring kind:{L"video",L"changed",L"audio"})for(const auto& entry:fs::directory_iterator(source/kind))if(entry.is_regular_file())files["/"+utf8(kind)+"/"+utf8(entry.path().filename().wstring())]=readText(entry.path());
 Manager manager(root/L"live-tracks-state");manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"]["DownloadFolder"]=utf8((root/L"live-track-outputs").wstring());
 auto receive=[&](LiveTracksFixture& server,const std::string& name,bool paired){
  Json tracks=Json::array({{{"kind","video"},{"playlist",server.url("/video/source.m3u8")},{"segments",Json::array({{{"url",server.url("/video/part0.m4s")}}})}}});if(paired)tracks.push_back({{"kind","audio"},{"playlist",server.url("/audio/source.m3u8")},{"segments",Json::array({{{"url",server.url("/audio/part0.m4s")}}})}});
  return manager.receive({{"action","adaptive"},{"url",server.url("/player")},{"filename",name},{"plan",{{"type","hls"},{"live",true},{"height",180},{"audioExpected",paired},{"container","mp4"},{"tracks",tracks}}}});
 };
 auto metadata=[&](JobPtr job){return Json::parse(execute(tools/L"ffprobe.exe",{L"-v",L"error",L"-show_entries",L"stream=codec_type,start_time,duration,height:format=duration",L"-of",L"json",job->target().wstring()},20,cancel));};
 for(bool dated:{false,true}){
  auto content=files;if(dated){for(const auto& item:std::vector<std::pair<std::string,std::string>>{{"video","2026-09-28T00:00:00.000Z"},{"audio","2026-09-28T00:00:00.500Z"}}){auto& playlist=content["/"+item.first+"/source.m3u8"];playlist.insert(playlist.find("#EXTINF:"),"#EXT-X-PROGRAM-DATE-TIME:"+item.second+"\n");}}
  LiveTracksFixture server(content);auto job=receive(server,dated?"live-tracks-program-clock":"live-tracks-media-clock",true);adaptiveTransfer(manager,job,std::make_shared<Cancel>());auto info=metadata(job);double video=-100,audio=-100;
  for(const auto& stream:info["streams"]){if(str(stream,"codec_type")=="video")video=std::stod(str(stream,"start_time"));if(str(stream,"codec_type")=="audio")audio=std::stod(str(stream,"start_time"));}
  std::cout<<"Live track offset ("<<(dated?"program clock":"media clock")<<"): "<<audio-video<<" s"<<std::endl;
  check(video>-100&&audio>-100&&std::abs((audio-video)-(dated?.5:0))<.08,dated?"Separate live audio/video preserve their program-date offset":"Separate live audio/video align using their media start clocks");
  execute(tools/L"ffmpeg.exe",{L"-v",L"error",L"-xerror",L"-i",job->target().wstring(),L"-f",L"null",L"-"},20,cancel);check(str(job->data,"Status")=="Complete"&&std::stod(str(info["format"],"duration"))>=3.9,"Separate live video and audio produce a complete fully decodable recording");
 }
 {
  auto content=files;content["/video/source.m3u8"]="#EXTM3U\n#EXT-X-VERSION:7\n#EXT-X-TARGETDURATION:2\n#EXT-X-MEDIA-SEQUENCE:0\n#EXT-X-MAP:URI=\"init.mp4\"\n#EXTINF:2,\npart0.m4s\n#EXTINF:2,\npart1.m4s\n#EXT-X-DISCONTINUITY\n#EXT-X-MAP:URI=\"../changed/init.mp4\"\n#EXTINF:2,\n../changed/part0.m4s\n#EXTINF:2,\n../changed/part1.m4s\n#EXT-X-ENDLIST\n";
  check(content["/video/init.mp4"]!=content["/changed/init.mp4"],"Live encoder-restart fixture has distinct initialization bytes");LiveTracksFixture server(content);auto job=receive(server,"live-encoder-restart",false);adaptiveTransfer(manager,job,std::make_shared<Cancel>());auto info=metadata(job);
  auto duration=std::stod(str(info["format"],"duration"));std::cout<<"Discontinuous live output duration: "<<duration<<" s"<<std::endl;
  check(duration>=7.9&&duration<=8.1&&num(job->data,"AdaptiveCompletedSegments")==4,"Live encoder restart preserves all media across a timestamp reset and initialization change");
  execute(tools/L"ffmpeg.exe",{L"-v",L"error",L"-xerror",L"-i",job->target().wstring(),L"-f",L"null",L"-"},20,cancel);check(true,"Live output fully decodes across an initialization change");
 }
}
