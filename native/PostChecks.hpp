#pragma once
class FormFixture {
 SOCKET listener=INVALID_SOCKET;std::thread server;std::vector<std::thread> clients;std::atomic_bool stopping{false};std::mutex mutex;
 static std::string field(const std::string& message,const std::string& key){std::istringstream lines(message);std::string line;while(std::getline(lines,line)){if(line=="\r"||line.empty())break;auto at=line.find(':');if(at!=std::string::npos&&lower(line.substr(0,at))==lower(key))return trim(line.substr(at+1));}return {};}
 static void sendAll(SOCKET socket,const std::string& data){size_t at=0;while(at<data.size()){auto n=::send(socket,data.data()+at,(int)std::min<size_t>(65536,data.size()-at),0);if(n<=0)return;at+=n;}}
 void serve(SOCKET socket){try{
  DWORD timeout=3000;setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));std::string message;char buffer[8192];size_t end=std::string::npos,required=0;
  for(;;){auto n=recv(socket,buffer,sizeof(buffer),0);if(n<=0)break;message.append(buffer,n);end=message.find("\r\n\r\n");if(end!=std::string::npos){std::smatch match;required=end+4;if(std::regex_search(message,match,std::regex("Content-Length: ([0-9]+)",std::regex::icase)))required+=std::stoull(match[1]);if(message.size()>=required)break;}if(message.size()>MaxBrowserPostBytes+32768)break;}
  auto a=message.find(' '),b=message.find(' ',a+1);auto method=message.substr(0,a),path=message.substr(a+1,b-a-1);auto body=end==std::string::npos?"":message.substr(end+4);auto headers=lower(message.substr(0,end));
  {std::lock_guard<std::mutex> lock(mutex);records.push_back({{"method",method},{"path",path},{"body",b64(Bytes(body.begin(),body.end()))},{"range",headers.find("range:")!=std::string::npos},{"referer",field(message,"Referer")},{"cookie",field(message,"Cookie")},{"agent",field(message,"User-Agent")}});}
  if(path=="/303"||path=="/307"||path=="/cross307"){
   auto code=path=="/303"?"303 See Other":"307 Temporary Redirect";auto target=path=="/303"?"/file":path=="/307"?"/echo":"http://localhost:"+std::to_string(port)+"/echo";
   sendAll(socket,std::string("HTTP/1.1 ")+code+"\r\nLocation: "+target+"\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
  }else if(path=="/busy")sendAll(socket,"HTTP/1.1 503 Service Unavailable\r\nRetry-After: 0\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
  else if(path=="/short")sendAll(socket,"HTTP/1.1 200 OK\r\nContent-Length: 50\r\nConnection: close\r\n\r\nshort");
  else if(path=="/file"){
   std::smatch match;bool range=std::regex_search(message,match,std::regex("Range: bytes=([0-9]+)-([0-9]+)",std::regex::icase));size_t first=range?(size_t)std::stoull(match[1]):0,last=range?(size_t)std::stoull(match[2]):payload.size()-1;
   auto text=std::string("HTTP/1.1 ")+(range?"206 Partial Content":"200 OK")+"\r\nETag: \"post-fixture\"\r\nContent-Length: "+std::to_string(last-first+1)+"\r\nConnection: close\r\n";
   if(range)text+="Content-Range: bytes "+std::to_string(first)+"-"+std::to_string(last)+"/"+std::to_string(payload.size())+"\r\n";
   sendAll(socket,text+"\r\n"+payload.substr(first,last-first+1));
  }else sendAll(socket,"HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: "+std::to_string(body.size())+"\r\nConnection: close\r\n\r\n"+body);
 }catch(...){}shutdown(socket,SD_BOTH);closesocket(socket);}
public:
 unsigned short port=0;std::string payload;Json records=Json::array();
 FormFixture(){payload.resize(3*1024*1024+17);for(size_t i=0;i<payload.size();++i)payload[i]=(char)((i*37+11)%251);listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(bind(listener,(sockaddr*)&a,sizeof(a))||listen(listener,32))throw std::runtime_error("Form fixture bind failed.");int n=sizeof(a);getsockname(listener,(sockaddr*)&a,&n);port=ntohs(a.sin_port);server=std::thread([this]{while(!stopping){fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval wait{0,100000};if(select(0,&set,nullptr,nullptr,&wait)>0){auto s=accept(listener,nullptr,nullptr);if(s!=INVALID_SOCKET)clients.emplace_back([this,s]{serve(s);});}}});}
 ~FormFixture(){stopping=true;server.join();closesocket(listener);for(auto& client:clients)client.join();}
 std::string url(const char* path){return "http://127.0.0.1:"+std::to_string(port)+path;}
 Json snapshot(){std::lock_guard<std::mutex> lock(mutex);return records;}
};
static void postChecks(const fs::path& root){
 FormFixture fixture;Manager m(root/L"form-downloads");m.state["Settings"]["DownloadFolder"]=utf8((root/L"post-files").wstring());m.state["Settings"]["CategoryFolders"]=false;m.state["Settings"]["ProxyMode"]="Connect directly";
 std::string body="export=monthly&token=fixture-secret&name=%E2%9C%93";Json request={{"method","POST"},{"contentType","application/x-www-form-urlencoded"},{"body",b64(Bytes(body.begin(),body.end()))}};
 auto msg=Json{{"action","add"},{"url",fixture.url("/echo")},{"filename","form.bin"},{"request",request},{"downloadLater",true}};
 auto job=m.receive(msg);check(readPostRequest(job->data)["body"]==request["body"]&&job->data.dump().find("fixture-secret")==std::string::npos&&job->data.dump().find(str(request,"body"))==std::string::npos,"POST request survives encrypted browser handoff without plaintext body storage");
 m.beginPrefetch(job);check(!m.isActive(job)&&fixture.snapshot().empty(),"File Info never prefetches or submits a POST");
 auto other=request;other["body"]=b64(Bytes{'x'});auto different=m.offerDownload(fixture.url("/echo"),"","other.bin","Main queue",true,{},other);auto get=m.offerDownload(fixture.url("/echo"),"","get.bin");check(str(different->data,"DuplicateOf").empty()&&str(get->data,"DuplicateOf").empty(),"POST duplicate identity includes method and body, independently of GET");
 job->data["Status"]="Awaiting confirmation";check(m.receive(msg)==job,"Repeated handoff of one pending POST reuses its download record");job->data["Status"]="Paused";
 auto malformed=request;malformed["contentType"]="text/plain\r\nInjected: yes";rejects([&]{validatePostRequest(malformed,fixture.url("/echo"));},"POST rejects injected Content-Type");malformed=request;malformed["body"]=b64(Bytes(MaxBrowserPostBytes+1,1));rejects([&]{validatePostRequest(malformed,fixture.url("/echo"));},"POST rejects bodies larger than 4 MiB");malformed=request;malformed["body"]="!";rejects([&]{validatePostRequest(malformed,fixture.url("/echo"));},"POST rejects malformed base64");
 rejects([&]{m.configure(job,{{"Url",fixture.url("/other")}});},"POST properties cannot move a stored form body to a different endpoint");check(!m.canRefreshAddress(job),"Generic URL refresh cannot turn a form download into GET");
 transfer(m,job,std::make_shared<Cancel>());auto first=fixture.snapshot();check(readText(job->target())==body&&first.size()==1&&str(first[0],"method")=="POST"&&!yes(first[0],"range")&&!yes(job->data,"RangeSupported"),"One exact POST streams and publishes without probes, ranges or repeat submissions");

 for(size_t size:{size_t(65537),size_t(262145),size_t(2*1024*1024+17),MaxBrowserPostBytes}){
  Bytes data(size);for(size_t i=0;i<size;++i)data[i]=(unsigned char)((i*37+17)%256);
  auto large=request;large["body"]=b64(data);large["contentType"]="application/octet-stream";
  auto largeJob=m.add(fixture.url("/echo"),"","large-form.bin","Main queue",true,{},"",large);
  auto start=fixture.snapshot().size();transfer(m,largeJob,std::make_shared<Cancel>());
  auto rows=fixture.snapshot();auto output=readText(largeJob->target(),MaxBrowserPostBytes+1);
  check(Bytes(output.begin(),output.end())==data&&rows.size()==start+1&&str(rows[start],"body")==str(large,"body")&&!yes(rows[start],"range"),"Large binary POST preserves exact bytes with one native request");
  Manager reopened(m.root);auto saved=std::find_if(reopened.jobs.begin(),reopened.jobs.end(),[&](const auto& x){return x->id()==largeJob->id();});
  check(saved!=reopened.jobs.end()&&readPostRequest((*saved)->data)["body"]==large["body"],"Large encrypted POST body survives reopening the saved download catalog");
 }

 {Manager full(root/L"post-catalog-limit");full.state["Settings"]["DownloadFolder"]=utf8((root/L"post-catalog-files").wstring());full.save();auto saved=readText(full.root/L"state.json");
  full.state["FixturePadding"]=std::string(32*1024*1024-512,'x');
  rejects([&]{full.add(fixture.url("/echo"),"","overflow.bin","Main queue",true,{},"",request);},"A POST that would exceed reloadable history size is rejected before acknowledgement");
  check(full.jobs.empty()&&readText(full.root/L"state.json")==saved&&Manager(full.root).jobs.empty(),"Oversized history keeps the previous catalog readable and rolls back the new job");
  full.state.erase("FixturePadding");
 }
 auto copy=m.redownload(job);check(readPostRequest(copy->data)==readPostRequest(job->data),"Explicit redownload retains the encrypted form request");
 auto normal=exportCatalog(m,{job});check(yes(normal["Downloads"][0],"RequiresRequestCapture")&&!normal["Downloads"][0].contains("ProtectedRequest"),"Catalog omits POST bodies unless encrypted credentials are explicitly included");
 Manager imported(root/L"post-import");importCatalog(imported,normal,utf8((root/L"post-import-files").wstring()));rejects([&]{imported.resume(imported.jobs[0]);},"Catalog without a form body requires browser recapture rather than GET");auto protectedCopy=exportCatalog(m,{job},true);Manager restored(root/L"post-import-encrypted");importCatalog(restored,protectedCopy,utf8((root/L"post-import-encrypted-files").wstring()),true);check(readPostRequest(restored.jobs[0]->data)==readPostRequest(job->data),"Encrypted catalog restores the exact form request for the same Windows account");
 auto make=[&](const char* route){return m.add(fixture.url(route),"","post.bin","Main queue",true,{},"",request);};
 auto redirect=make("/303");size_t before=fixture.snapshot().size();transfer(m,redirect,std::make_shared<Cancel>());auto records=fixture.snapshot();int posts=0,gets=0;for(size_t i=before;i<records.size();++i)(str(records[i],"method")=="POST"?posts:gets)++;check(posts==1&&gets>=3&&yes(redirect->data,"RangeSupported")&&readText(redirect->target())==fixture.payload,"303 switches once to GET and uses verified parallel ranges");
 auto same=make("/307");before=fixture.snapshot().size();transfer(m,same,std::make_shared<Cancel>());records=fixture.snapshot();check(records.size()==before+2&&str(records[before],"body")==str(records[before+1],"body")&&readText(same->target())==body,"Same-origin 307 preserves exact form body");
 auto cross=make("/cross307");before=fixture.snapshot().size();rejects([&]{transfer(m,cross,std::make_shared<Cancel>());},"Cross-origin 307 does not disclose the form body");check(fixture.snapshot().size()==before+1,"Cross-origin POST redirect sends no request to the new origin");
 auto busy=make("/busy");before=fixture.snapshot().size();rejects([&]{transfer(m,busy,std::make_shared<Cancel>());},"Failed form submission surfaces its HTTP error");check(fixture.snapshot().size()==before+1,"503 does not cause automatic form resubmission");rejects([&]{transfer(m,busy,std::make_shared<Cancel>());},"Repeated worker invocation cannot replay an attempted form");check(fixture.snapshot().size()==before+1,"Attempt marker persists before the first network submission");
 busy->data["Status"]="Failed";m.resume(busy);check(!yes(busy->data,"PostAttempted"),"Explicit Start re-arms a failed form submission");m.pause(busy);
 auto shortJob=make("/short");rejects([&]{transfer(m,shortJob,std::make_shared<Cancel>());},"Truncated POST response is never published as complete");check(!fs::exists(shortJob->target()),"Truncated form response preserves the output destination");
}
static void completionActionChecks(const fs::path& root){
 Manager m(root/L"completion-actions");auto job=m.add("https://example.test/file.bin",utf8((root/L"completion-files").wstring()));
 check(m.takeDownloadCompletion(job).empty(),"Completion actions are off by default");rejects([&]{m.setCompletionAction(job,"Launch command",30);},"Completion action rejects arbitrary commands");rejects([&]{m.setCompletionAction(job,"Restart",1);},"Completion actions require a cancellable countdown");
 m.setCompletionAction(job,"Exit UDM",30,true);check(m.takeDownloadCompletion(job).empty()&&yes(job->data,"CompletionActionArmed"),"Incomplete downloads cannot trigger their completion action");job->data["Status"]="Complete";auto spec=m.takeDownloadCompletion(job);check(str(spec,"Action")=="Exit UDM"&&num(spec,"DelaySeconds")==30&&yes(spec,"WaitForOthers"),"Successful completion produces the explicitly selected action and wait policy");check(m.takeDownloadCompletion(job).empty()&&!yes(job->data,"CompletionActionArmed"),"Completion action is consumed and saved exactly once");
 rejects([&]{m.setCompletionAction(job,"Shut down");},"Already completed downloads cannot be armed after the fact");m.setCompletionAction(job,"None");check(!exportCatalog(m,{job},true)["Downloads"][0].contains("CompletionActionArmed"),"Catalogs never import armed system actions");
}
