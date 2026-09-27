#pragma once
#include <bcrypt.h>
static std::string authMd5(const std::string& value){
 BCRYPT_ALG_HANDLE algorithm=nullptr;if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_MD5_ALGORITHM,nullptr,0)<0)throw std::runtime_error("MD5 fixture unavailable.");
 unsigned char digest[16]{};auto status=BCryptHash(algorithm,nullptr,0,(PUCHAR)value.data(),(ULONG)value.size(),digest,sizeof(digest));BCryptCloseAlgorithmProvider(algorithm,0);if(status<0)throw std::runtime_error("MD5 fixture failed.");
 const char* digits="0123456789abcdef";std::string result;for(auto b:digest){result+=digits[b>>4];result+=digits[b&15];}return result;
}
class AuthenticationFixture {
 SOCKET listener=INVALID_SOCKET;std::thread server;std::vector<std::thread> clients;std::atomic_bool stopping{false};
 static void sendAll(SOCKET socket,const std::string& value){for(size_t at=0;at<value.size();){auto n=send(socket,value.data()+at,(int)(value.size()-at),0);if(n<=0)break;at+=n;}}
 void serve(SOCKET socket){try{
  DWORD timeout=3000;setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));std::string request;char buffer[4096];
  while(request.find("\r\n\r\n")==std::string::npos&&request.size()<32768){int n=recv(socket,buffer,sizeof(buffer),0);if(n<=0)break;request.append(buffer,n);}
  auto begin=request.find(' '),end=request.find(' ',begin+1);auto path=request.substr(begin+1,end-begin-1);++requests;
  auto respond=[&](const std::string& code,const std::string& headers,const std::string& body){sendAll(socket,"HTTP/1.1 "+code+"\r\n"+headers+"Content-Length: "+std::to_string(body.size())+"\r\nConnection: close\r\n\r\n"+body);};
  if(path=="/cross"||path=="/same"){respond("302 Found","Location: "+(path=="/cross"?std::string("http://localhost:")+std::to_string(port):url(""))+"/basic\r\n","");}
  else{
   std::smatch match;std::string auth;if(std::regex_search(request,match,std::regex("Authorization: *([^\\r\\n]*)",std::regex::icase)))auth=match[1];
   bool digest=path=="/digest"||path=="/bad-digest",valid=auth=="Basic "+udm::b64(udm::Bytes{'u',':','p',':','2'});
   if(digest){valid=false;if(auth.rfind("Digest ",0)==0){++digestHeaders;std::map<std::string,std::string> fields;std::regex pairs("([a-zA-Z0-9_-]+)=(?:\"([^\"]*)\"|([^, ]+))");for(auto it=std::sregex_iterator(auth.begin(),auth.end(),pairs);it!=std::sregex_iterator();++it)fields[(*it)[1]]=(*it)[2].matched?(*it)[2].str():(*it)[3].str();
     auto ha1=authMd5("u:udm-fixture:p:2"),ha2=authMd5("GET:"+path);auto expected=authMd5(ha1+":nonce-fixture:"+fields["nc"]+":"+fields["cnonce"]+":auth:"+ha2);
     valid=fields["username"]=="u"&&fields["realm"]=="udm-fixture"&&fields["nonce"]=="nonce-fixture"&&fields["uri"]==path&&fields["qop"]=="auth"&&fields["response"]==expected;
    }}
   if(path=="/bearer")respond("401 Unauthorized","WWW-Authenticate: Bearer realm=\"fixture\"\r\n","");
   else if(!valid||path=="/bad-digest"){++challenges;respond("401 Unauthorized",digest?"WWW-Authenticate: Digest realm=\"udm-fixture\", nonce=\"nonce-fixture\", algorithm=MD5, qop=\"auth\"\r\n":"WWW-Authenticate: Basic realm=\"udm-fixture\"\r\n","");}
   else{++accepted;bool ranged=std::regex_search(request,match,std::regex("Range: bytes=([0-9]+)-([0-9]*)",std::regex::icase));size_t from=ranged?std::stoull(match[1]):0,to=ranged&&!match[2].str().empty()?std::stoull(match[2]):payload.size()-1;to=std::min(to,payload.size()-1);std::string extra="ETag: \"auth-fixture\"\r\n";if(ranged)extra+="Content-Range: bytes "+std::to_string(from)+"-"+std::to_string(to)+"/"+std::to_string(payload.size())+"\r\n";respond(ranged?"206 Partial Content":"200 OK",extra,payload.substr(from,to-from+1));}
  }
 }catch(...){}shutdown(socket,SD_BOTH);closesocket(socket);}
public:
 unsigned short port=0;std::atomic<int> requests{0},challenges{0},accepted{0},digestHeaders{0};std::string payload;
 AuthenticationFixture(){payload.resize(1024*1024+71);for(size_t i=0;i<payload.size();++i)payload[i]=(char)((i*29+5)%251);listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(bind(listener,(sockaddr*)&address,sizeof(address))||listen(listener,32))throw std::runtime_error("Authentication fixture failed.");int size=sizeof(address);getsockname(listener,(sockaddr*)&address,&size);port=ntohs(address.sin_port);server=std::thread([&]{while(!stopping){fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval timeout{0,100000};if(select(0,&set,nullptr,nullptr,&timeout)>0){auto client=accept(listener,nullptr,nullptr);if(client!=INVALID_SOCKET)clients.emplace_back([this,client]{serve(client);});}}});}
 ~AuthenticationFixture(){stopping=true;server.join();closesocket(listener);for(auto& client:clients)client.join();}
 std::string url(const char* path)const{return "http://127.0.0.1:"+std::to_string(port)+path;}
};
static void authenticationChecks(const udm::fs::path& root){
 using namespace udm;AuthenticationFixture server;Manager manager(root/L"authentication-state");auto settings=manager.state["Settings"];settings["DownloadFolder"]=utf8((root/L"authentication-files").wstring());settings["CategoryFolders"]=false;settings["Connections"]=4;settings["Retries"]=0;manager.setSettings(settings);
 auto runUntilStopped=[&](JobPtr job){manager.resume(job);auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);for(;;){manager.tick();{Lock lock(manager.mutex);if(!manager.isActive(job)&&str(job->data,"Status")!="Queued")return;}if(std::chrono::steady_clock::now()>deadline)throw std::runtime_error("Authentication workflow did not finish.");Sleep(10);}};
 auto basic=manager.add(server.url("/basic"),"","basic.bin");runUntilStopped(basic);
 check(str(basic->data,"Status")=="Failed"&&canRequestLogin(basic->data)&&yes(basic->data,"AuthenticationPromptPending"),"HTTP Basic challenge requests a login for the actual server");
 check(str(basic->data,"Error").find("expired")==std::string::npos,"HTTP 401 is distinguished from an expired download link");
 manager.setDownloadLogin(basic,"u","wrong");runUntilStopped(basic);check(str(basic->data,"Status")=="Failed"&&yes(basic->data,"AuthenticationPromptPending")&&!fs::exists(basic->target()),"Incorrect credentials do not publish a file or cause an automatic retry loop");
 manager.setDownloadLogin(basic,"u","p:2");runUntilStopped(basic);check(str(basic->data,"Status")=="Complete"&&readText(basic->target())==server.payload,"Login and retry downloads exact bytes through parallel authenticated requests");
 check(str(basic->data,"ProtectedHeaders").find("p:2")==std::string::npos&&basicLogin(readHeaders(basic->data)).second=="p:2","Per-download password is protected at rest and preserves colons");
 auto digest=manager.add(server.url("/digest"),"","digest.bin");manager.setDownloadLogin(digest,"u","p:2");runUntilStopped(digest);
 check(str(digest->data,"Status")=="Complete"&&readText(digest->target())==server.payload&&server.digestHeaders>=2,"WinHTTP Digest challenge-response is cryptographically verified by the server for probe and workers");
 auto denied=manager.add(server.url("/bad-digest"),"","denied-digest.bin");manager.setDownloadLogin(denied,"u","p:2");auto requests=server.requests.load();runUntilStopped(denied);
 check(str(denied->data,"Status")=="Failed"&&canRequestLogin(denied->data)&&server.requests-requests==2,"Rejected Digest credentials stop after one challenge retry");
 auto cross=manager.add(server.url("/cross"),"","cross-origin.bin");manager.setDownloadLogin(cross,"u","p:2");runUntilStopped(cross);
 check(str(cross->data,"Status")=="Failed"&&num(cross->data,"LastHttpStatus")==401&&!canRequestLogin(cross->data),"Cross-origin redirect does not forward credentials or mislabel the login destination");
 rejects([&]{manager.setDownloadLogin(cross,"u","p:2");},"Login editor rejects a challenge for a different origin");
 auto same=manager.add(server.url("/same"),"","same-origin.bin");manager.setDownloadLogin(same,"u","p:2");runUntilStopped(same);
 check(str(same->data,"Status")=="Complete"&&readText(same->target())==server.payload&&reveal(str(same->data,"ProtectedResolvedUrl"))==server.url("/basic"),"Same-origin redirects retain authentication and record the resolved address privately");
 auto bearer=manager.add(server.url("/bearer"),"","bearer.bin");runUntilStopped(bearer);check(!canRequestLogin(bearer->data)&&!yes(bearer->data,"AuthenticationPromptPending"),"Bearer or browser-session authentication does not trigger a Basic password prompt");
 auto saved=manager.add("https://downloads.example.test/file","","remembered.bin");manager.setDownloadLogin(saved,"fixture-user","fixture-secret",true);
 auto sameSite=manager.add("https://downloads.example.test/next","","remembered-next.bin");auto otherSite=manager.add("https://cdn.downloads.example.test/next","","other-origin.bin");auto otherPort=manager.add("https://downloads.example.test:444/next","","other-port.bin");
 check(basicLogin(readHeaders(sameSite->data)).first=="fixture-user"&&headerValue(readHeaders(otherSite->data),"Authorization").empty()&&headerValue(readHeaders(otherPort->data),"Authorization").empty(),"Remembered logins apply only to their exact HTTPS origin");
 check(manager.state["Settings"]["SiteLogins"][0]["ProtectedPassword"]!="fixture-secret","Saved site password uses Windows protection");
 rejects([&]{manager.setDownloadLogin(bearer,"u","p:2",true);},"Remembered passwords reject a plain HTTP origin");
 auto prior=saved->data,priorSettings=manager.state["Settings"];auto blocker=manager.root/L"state.json.tmp";fs::create_directory(blocker);
 rejects([&]{manager.setDownloadLogin(saved,"changed","new-secret",true);},"Login storage failure is reported");check(saved->data==prior&&manager.state["Settings"]==priorSettings,"Failed login save rolls back both per-file and remembered credentials");fs::remove(blocker);
 Headers h={{"Cookie","fixture-cookie"}};rejects([&]{setBasicLogin(h,"bad:name","password");},"Login names with colons are rejected consistently");setBasicLogin(h,"u","p:2");setBasicLogin(h,"","",false);check(h.size()==1&&headerValue(h,"Cookie")=="fixture-cookie","Removing a login retains unrelated browser headers");
 const std::string source="https://www.youtube.com/watch?v=abcdefghijk",video="https://r1.googlevideo.com/videoplayback?itag=137&signature=fixture",audio="https://r2.googlevideo.com/videoplayback?itag=140&signature=fixture";
 Json media={{"Url",source},{"SourceUrl",source},{"Video",{{"Url",video}}},{"Audio",{{"Url",audio}}}};auto links=downloadLinks(media);
 check(downloadAddress(media)==video&&links.size()==2&&links[1].address==audio,"Captured direct video shows the real video and audio links instead of the webpage");
 media["ProtectedSabr"]=protect(Json{{"url",video}}.dump());check(downloadAddress(media)==video&&downloadLinks(media).size()==1&&mediaAddressNote(media).find("not a standalone file link")!=std::string::npos,"SABR displays its actual shared endpoint and identifies its session requirement");
 auto unchanged=media;downloadLinks(media);check(media==unchanged&&str(media,"Url")==source,"Viewing media URLs does not replace the canonical webpage or mutate captured sessions");
 Json adaptive={{"Url","https://player.example.test/watch"},{"ProtectedAdaptive",protect(Json{{"tracks",Json::array({{{"kind","video"},{"segments",Json::array({{{"url","https://cdn.example.test/part-1.m4s"}}})}}})}}.dump())}};
 check(downloadAddress(adaptive)=="https://cdn.example.test/part-1.m4s"&&downloadLinks(adaptive)[0].label=="First video segment","Segmented video addresses are labelled as segments rather than standalone full files");
 check(downloadLinks(same->snapshot()).size()==2&&downloadLinks(same->snapshot())[1].address==server.url("/basic"),"Link details distinguish requested and resolved HTTP addresses");
 manager.updateCompleted(same,{{"Url",server.url("/replacement")}});check(str(same->data,"ProtectedResolvedUrl").empty()&&downloadLinks(same->snapshot()).size()==1,"Editing a completed file address clears the previous server redirect");
 auto captured=manager.add(source,"","captured.mp4");captured->data["SourceUrl"]=source;captured->data["Status"]="Complete";
 rejects([&]{manager.updateCompleted(captured,{{"Url",video}});},"Completed media properties cannot replace the webpage with a raw streaming endpoint");
 rejects([&]{manager.setDownloadLogin(captured,"u","p");},"Browser-captured media keeps its browser sign-in workflow");
}
