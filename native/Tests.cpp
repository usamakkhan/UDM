#include "MediaStorage.hpp"
#include "Core.hpp"
#include "GuiModels.hpp"
#include "StreamProgress.hpp"
#include "BrowserSettings.hpp"
#include "Catalog.hpp"
#include "ZipPreview.hpp"
#include "Streaming.hpp"
#include "Launch.hpp"
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <future>
using namespace udm;
static int passed=0,failed=0;
static void check(bool ok,const char* name){if(!ok){++failed;std::cerr<<"FAIL "<<name<<std::endl;}else{++passed;std::cout<<"PASS "<<name<<std::endl;}}
template<class F>void rejects(F f,const char* name){try{f();check(false,name);}catch(...){check(true,name);}}
#include "MediaChecks.hpp"
#include "ParallelMediaChecks.hpp"
#include "AudioStreamingChecks.hpp"
#include "StreamRecoveryChecks.hpp"
#include "DialogChecks.hpp"
#include "CompletionPlanChecks.hpp"
#include "BridgeChecks.hpp"
class Fixture {
 SOCKET listener=INVALID_SOCKET;std::thread server;std::vector<std::thread> clients;std::atomic_bool stop{false};
 static void sendAll(SOCKET s,const std::string& text){size_t at=0;while(at<text.size()){int n=::send(s,text.data()+at,(int)std::min<size_t>(65536,text.size()-at),0);if(n<=0)return;at+=n;}}
 void serve(SOCKET s){try{DWORD timeout=3000;setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));std::string req;char b[4096];while(req.find("\r\n\r\n")==std::string::npos&&req.size()<32768){int n=recv(s,b,sizeof(b),0);if(n<=0)break;req.append(b,n);}auto first=req.find(' '),last=req.find(' ',first+1);std::string path=req.substr(first+1,last-first-1);std::smatch match;bool range=std::regex_search(req,match,std::regex("Range: bytes=([0-9]+)-([0-9]*)",std::regex::icase));i64 start=range?std::stoll(match[1]):0,end=range&&!match[2].str().empty()?std::stoll(match[2]):(i64)payload.size()-1;bool probe=range&&start==0&&end==0;requests++;if(req.find("If-Match: \"fixture-v1\"")!=std::string::npos)++adaptiveMatches;if(req.find("If-Unmodified-Since: Wed, 21 Oct 2015 07:28:00 GMT")!=std::string::npos)++adaptiveDates;if(range&&start==262144)resumedPrefix=true;
 if(path=="/stall-headers"||path=="/stall-body"){++stalled;if(path=="/stall-body")sendAll(s,"HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\n");char hold;recv(s,&hold,1,0);}
 else if(path=="/redirect"){sendAll(s,"HTTP/1.1 302 Found\r\nLocation: http://localhost:"+std::to_string(port)+"/sensitive\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");}
 else if(path=="/sensitive"){sensitiveSeen=req.find("Cookie:")!=std::string::npos||req.find("Authorization:")!=std::string::npos||req.find("Referer:")!=std::string::npos;sendAll(s,"HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nok");}
 else if(path=="/expired")sendAll(s,"HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
 else if((path=="/busy-probe"&&probe&&++busyProbe==1)||(path=="/busy-worker"&&!probe&&++busyWorker==1))sendAll(s,"HTTP/1.1 503 Service Unavailable\r\nRetry-After: 1\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
 else if(path=="/forbidden-worker"&&!probe){++forbiddenWorker;sendAll(s,"HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");}
 else if(path=="/always-busy"&&probe){++alwaysBusy;sendAll(s,"HTTP/1.1 503 Service Unavailable\r\nRetry-After: 0\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");}
 else if(path=="/long-wait"&&probe){++longWait;sendAll(s,"HTTP/1.1 429 Too Many Requests\r\nRetry-After: 9999999999999999999\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");}
 else if(path=="/cancel-retry"&&probe){++cancelRetry;sendAll(s,"HTTP/1.1 429 Too Many Requests\r\nRetry-After: 60\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");}
 else if(path=="/cancel-worker"&&!probe){++cancelWorker;sendAll(s,"HTTP/1.1 429 Too Many Requests\r\nRetry-After: 60\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");}
 else if(path=="/empty")sendAll(s,"HTTP/1.1 416 Range Not Satisfiable\r\nContent-Range: bytes */0\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
 else if(path=="/unknown")sendAll(s,"HTTP/1.1 200 OK\r\nConnection: close\r\n\r\nunknown-size-body");
 else if(path=="/html")sendAll(s,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<a href='/file.zip'>file</a><a href='https://example.invalid/escape.zip'>external</a><a href='/child.html'>child</a>");
 else if(path=="/child.html")sendAll(s,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<a href='/second.pdf'>pdf</a>");
 else{bool ranged=range&&path!="/plain";if(!ranged){start=0;end=(i64)payload.size()-1;}std::string header=ranged?"HTTP/1.1 206 Partial Content\r\n":"HTTP/1.1 200 OK\r\n";i64 realStart=start;if(path=="/bad-range"&&!probe&&ranged)++start;header+="ETag: \"fixture-v1\"\r\n";if(path=="/dated")header+="Last-Modified: Wed, 21 Oct 2015 07:28:00 GMT\r\n";if(ranged)header+="Content-Range: bytes "+std::to_string(start)+"-"+std::to_string(end)+"/"+std::to_string(payload.size())+"\r\n";header+="Content-Length: "+std::to_string(end-realStart+1)+"\r\nConnection: close\r\n\r\n";sendAll(s,header);auto body=payload.substr((size_t)realStart,(size_t)(end-realStart+1));if(path=="/truncate"&&!probe&&body.size()>10)body.resize(body.size()/2);if(path=="/slow"&&!probe){for(size_t at=0;at<body.size();at+=16384){sendAll(s,body.substr(at,16384));Sleep(8);}}else sendAll(s,body);}
 }catch(...){}shutdown(s,SD_BOTH);closesocket(s);}
public:std::atomic_int adaptiveMatches{0},adaptiveDates{0};unsigned short port=0;std::string payload;std::atomic_int requests{0},stalled{0},busyProbe{0},busyWorker{0},forbiddenWorker{0},alwaysBusy{0},longWait{0},cancelRetry{0},cancelWorker{0};std::atomic_bool sensitiveSeen{false},resumedPrefix{false};
 Fixture(){payload.resize(3*1024*1024+731);for(size_t i=0;i<payload.size();++i)payload[i]=(char)((i*31+7)%251);listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(listener==INVALID_SOCKET)throw std::runtime_error("Fixture socket failed.");sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(bind(listener,(sockaddr*)&a,sizeof(a))||listen(listener,32))throw std::runtime_error("Fixture bind failed.");int n=sizeof(a);getsockname(listener,(sockaddr*)&a,&n);port=ntohs(a.sin_port);server=std::thread([this]{while(!stop){fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval wait{0,100000};if(select(0,&set,nullptr,nullptr,&wait)>0){auto s=accept(listener,nullptr,nullptr);if(s!=INVALID_SOCKET)clients.emplace_back([this,s]{serve(s);});}}});}
 ~Fixture(){stop=true;if(server.joinable())server.join();closesocket(listener);for(auto& t:clients)if(t.joinable())t.join();}
 std::string url(const char* path)const{return "http://127.0.0.1:"+std::to_string(port)+path;}
};
#include "AuthenticationChecks.hpp"
#include "PlayerChecks.hpp"
#include "AdaptiveChecks.hpp"
#include "TransferChecks.hpp"
#include "RecoveryChecks.hpp"
#include "WorkflowChecks.hpp"
#include "QuotaChecks.hpp"
#include "DuplicateChecks.hpp"
#include "OverwriteChecks.hpp"
#include "HlsPlaylistChecks.hpp"
#include "HlsRecordingChecks.hpp"
#include "LiveHlsChecks.hpp"
#include "LiveHlsTracksChecks.hpp"
#include "ReliabilityChecks.hpp"
#include "CheckpointChecks.hpp"
#include "SchedulerChecks.hpp"
#include "GuiChecks.hpp"
#include "QueueChecks.hpp"
#include "WakeChecks.hpp"
#include "CatalogChecks.hpp"
#include "PostChecks.hpp"
#include "CaptureReceiptChecks.hpp"
#include "OfflineChecks.hpp"
#include "ConnectionChecks.hpp"
#include "ProxyPolicyChecks.hpp"
#include "BrowserProxyChecks.hpp"
#include "PreviewChecks.hpp"
#include "ScannerChecks.hpp"
#include "OptionsChecks.hpp"
int main(int argc,char** argv){if(argc>=3&&std::string(argv[1])=="--scanner-fixture")return scannerFixture();WSADATA winsock{};WSAStartup(MAKEWORD(2,2),&winsock);CoInitializeEx(nullptr,COINIT_MULTITHREADED);if(argc==3&&std::string(argv[1])=="--feature-spec"){auto result=offlineFeatureSpec(fs::path(wide(argv[2])));CoUninitialize();WSACleanup();return result;}auto root=appDir()/L"test-output"/wide(guid());fs::create_directories(root);try{
 if(argc==2&&std::string(argv[1])=="--quota-checks"){Fixture fixture;quotaChecks(root,fixture);duplicateChecks(root,fixture);std::cout<<passed<<" passed, "<<failed<<" failed"<<std::endl;CoUninitialize();WSACleanup();return failed?1:0;}
 if(argc==2&&std::string(argv[1])=="--overwrite-checks"){Fixture fixture;overwriteChecks(root,fixture);duplicateChecks(root,fixture);std::cout<<passed<<" passed, "<<failed<<" failed"<<std::endl;CoUninitialize();WSACleanup();return failed?1:0;}
 if(argc==2&&std::string(argv[1])=="--parallel-media-checks"){Manager manager(root/L"parallel-state");manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"]["DownloadFolder"]=utf8((root/L"downloads").wstring());parallelMediaChecks(manager);parallelMediaChecks(manager,true);std::cout<<passed<<" passed, "<<failed<<" failed"<<std::endl;CoUninitialize();WSACleanup();return failed?1:0;}
 if(argc==2&&std::string(argv[1])=="--live-hls-track-checks"){liveHlsTracksChecks(root);std::cout<<passed<<" passed, "<<failed<<" failed"<<std::endl;CoUninitialize();WSACleanup();return failed?1:0;}
 hlsPlaylistChecks();
 hlsRecordingChecks(root);
 if(argc==2&&std::string(argv[1])=="--hls-playlist-checks"){std::cout<<passed<<" passed, "<<failed<<" failed"<<std::endl;CoUninitialize();WSACleanup();return failed?1:0;}
 liveHlsChecks(root);
 liveHlsTracksChecks(root);
 if(argc==2&&std::string(argv[1])=="--live-hls-checks"){std::cout<<passed<<" passed, "<<failed<<" failed"<<std::endl;CoUninitialize();WSACleanup();return failed?1:0;}
 optionsChecks(root);
 if(argc==2&&std::string(argv[1])=="--options-checks"){std::cout<<passed<<" passed, "<<failed<<" failed"<<std::endl;CoUninitialize();WSACleanup();return failed?1:0;}
 if(argc==2&&std::string(argv[1])=="--dialog-checks"){dialogChecks(root);completionPlanChecks(root);std::cout<<passed<<" passed, "<<failed<<" failed"<<std::endl;CoUninitialize();WSACleanup();return failed?1:0;}
 dialogChecks(root);
 completionPlanChecks(root);
 if(argc==2&&std::string(argv[1])=="--media-recovery-checks"){streamRecoveryChecks(root);std::cout<<passed<<" passed, "<<failed<<" failed"<<std::endl;CoUninitialize();WSACleanup();return failed?1:0;}
 if(argc==2&&std::string(argv[1])=="--scanner-checks"){Fixture fixture;scannerChecks(root,fixture);std::cout<<passed<<" passed, "<<failed<<" failed"<<std::endl;CoUninitialize();WSACleanup();return failed?1:0;}
 if(argc==2&&std::string(argv[1])=="--connection-checks"){Fixture fixture;connectionChecks(root);proxyPolicyChecks(root);browserProxyChecks(root,fixture);std::cout<<passed<<" passed, "<<failed<<" failed"<<std::endl;CoUninitialize();WSACleanup();return failed?1:0;}
 {
  SpeedMeter meter;check(meter.update(1000,1,true)==1000,"Speed meter uses transferred bytes and real elapsed time");
  check(meter.update(1000,1,true)==500,"Speed meter smooths a short zero-byte interval");
  meter.update(5000,3,true);check(meter.update(5000,5,true)==0,"Speed falls to zero after a full stalled window");
  check(meter.update(9000,.2,false)==0,"Paused transfer clears the display window");
  check(meter.update(10000,2,true)==500,"Resume excludes retained bytes and paused time");
  meter.reset();meter.update(4000,4,true);check(meter.update(5000,2,true)==800,"Irregular updates trim the oldest interval by elapsed time");
  check(meter.update(0,1,true)==0,"Counter reset cannot create a negative speed");
 }
 previewChecks();
 check(utf8(wide("日本語—UDM🙂"))=="日本語—UDM🙂","UTF-8 and UTF-16 round trip");check(safeName("../../CON.txt")=="_CON.txt","Windows reserved filename normalization");check(safeName("a:b?.mp4")=="a_b_.mp4","Unsafe filename characters");rejects([]{Url u("https://user:secret@example.com/file");},"Embedded URL credentials rejected");rejects([]{Url u("file:///C:/Windows/test");},"Non-network URL rejected");check(expand("https://example.com/[001-003].zip").size()==3,"Numeric URL expansion");check(expand("https://example.com/[a-c].zip")[2]=="https://example.com/c.zip","Alphabetic URL expansion");rejects([]{expand("https://example.com/[1-1001].zip");},"Batch bounds");check(hostIs("r1.googlevideo.com","googlevideo.com")&&!hostIs("evilgooglevideo.com","googlevideo.com"),"Exact media host suffix");rejects([]{validateStream("https://googlevideo.com.evil.invalid/videoplayback");},"Deceptive capture host rejected");check(parseDate(date(1700000000000LL))==1700000000000LL,"Legacy DateTime serialization");check(dictionary(Json::array({{{"Key","Archives"},{"Value","C:\\files"}}}))["Archives"]=="C:\\files","Legacy dictionary deserialization");auto secret=protect("cookies=秘密🙂");check(reveal(secret)=="cookies=秘密🙂","DPAPI current-user encryption round trip");check(secret.find("cookies")==std::string::npos,"Secrets encrypted at rest");rejects([]{validateHeaders({{"Cookie","x\r\nInjected: y"}});},"Header injection rejected");auto q=defaultQueue();q["Scheduled"]=true;q["StartMinute"]=1380;q["StopMinute"]=60;q["Days"]=1<<1;SYSTEMTIME t{};t.wYear=2026;t.wMonth=9;t.wDay=22;t.wHour=0;t.wMinute=30;FILETIME local{},utc{};SystemTimeToFileTime(&t,&local);LocalFileTimeToFileTime(&local,&utc);ULARGE_INTEGER v{};v.LowPart=utc.dwLowDateTime;v.HighPart=utc.dwHighDateTime;check(inWindow(q,(i64)(v.QuadPart/10000)-11644473600000LL),"Overnight queue belongs to start day");q["Enabled"]=false;check(!inWindow(q,0,true),"Disabled queue overrides manual run");
 auto proto=Proto().set(1,123ULL).set(5,std::string("opaque")).floating(35,1.0f).set(99,Bytes{0,1,2,255});check(Proto::parse(proto.encode()).encode()==proto.encode(),"Protobuf unknown fields preserved");rejects([]{Proto::parse(Bytes{0x08,0x80});},"Truncated protobuf rejected");check(umpInteger(Bytes{0x7f})==127&&umpInteger(Bytes{0x80,1})==64&&umpInteger(Bytes{0xf0,0x78,0x56,0x34,0x12})==0x12345678,"UMP variable integers");auto id=formatIdentity({{"id","137"},{"lastModified","12345"},{"xtags","x"}});check(sameFormat(id,id),"Full stream identity equality");check(!sameFormat(id,formatIdentity({{"id","137"},{"lastModified","12346"},{"xtags","x"}})),"Stale stream identity rejected");
 playerChecks();
 Fixture fixture;Manager manager(root/L"state");manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"]["DownloadFolder"]=utf8((root/L"downloads").wstring());manager.state["Settings"]["Retries"]=0;auto run=[&](const char* path,const char* name){auto job=manager.add(fixture.url(path),"",name);transfer(manager,job,std::make_shared<Cancel>());return job;};auto expected=root/L"expected.bin";writeBytes(expected,Bytes(fixture.payload.begin(),fixture.payload.end()));auto hash=fileHash(expected);auto ranged=run("/range","range.bin");check(fileHash(ranged->target())==hash,"Parallel range download content SHA-256");check(yes(ranged->data,"RangeSupported")&&ranged->data["Segments"].size()>1,"Validated parallel range plan");check(fs::exists(ranged->target().wstring()+L":Zone.Identifier"),"Internet zone marking");auto plain=run("/plain","plain.bin");check(fileHash(plain->target())==hash&&!yes(plain->data,"RangeSupported"),"No-range server fallback");auto empty=run("/empty","empty.bin");check(fs::file_size(empty->target())==0,"Empty resource download");auto unknown=run("/unknown","unknown.bin");check(readText(unknown->target())=="unknown-size-body","Unknown-length resource download");auto mismatch=manager.add(fixture.url("/range"),"","mismatch.bin","Main queue",true,{},std::string(64,'0'));rejects([&]{transfer(manager,mismatch,std::make_shared<Cancel>());},"SHA mismatch blocks publication");check(!fs::exists(mismatch->target()),"Mismatched hash output absent");auto bad=manager.add(fixture.url("/bad-range"),"","bad.bin");rejects([&]{transfer(manager,bad,std::make_shared<Cancel>());},"Incorrect Content-Range rejected");check(!fs::exists(bad->target()),"Incorrect range output absent");auto trunc=manager.add(fixture.url("/truncate"),"","truncate.bin");rejects([&]{transfer(manager,trunc,std::make_shared<Cancel>());},"Truncated response rejected");check(!fs::exists(trunc->target()),"Truncated output absent");auto collision=manager.add(fixture.url("/range"),"","collision.bin");writeBytes(collision->target(),Bytes{'k','e','e','p'});rejects([&]{transfer(manager,collision,std::make_shared<Cancel>());},"Existing destination not overwritten");check(readText(collision->target())=="keep","Existing file preserved");
 auto slow=manager.add(fixture.url("/slow"),"","resume.bin");auto cancel=std::make_shared<Cancel>();auto future=std::async(std::launch::async,[&]{try{transfer(manager,slow,cancel);}catch(const Cancelled&){};});for(int i=0;i<100;++i){Sleep(10);Lock l(manager.mutex);if(num(slow->data,"Received")>65536)break;}cancel->stop=true;future.get();check(num(slow->data,"Received")>0&&!fs::exists(slow->target()),"Pause preserves parts without publishing output");transfer(manager,slow,std::make_shared<Cancel>());check(fileHash(slow->target())==hash,"Resume assembles exact original bytes");Cancel c;Http redirect(fixture.url("/redirect"),{{"Cookie","secret"},{"Authorization","Basic private"},{"Referer","private"}},manager.state["Settings"],c);auto body=redirect.all(10,c);check(std::string(body.begin(),body.end())=="ok"&&!fixture.sensitiveSeen,"Cross-origin redirect strips sensitive headers");
 auto p=Json{{"Id",guid()},{"Name","Fixture project"},{"StartUrl",fixture.url("/html")},{"Extensions","zip pdf"},{"Depth",1},{"MaxPages",5},{"Links",Json::array()}};p=explore(p,manager.state["Settings"],c);check(p["Links"].size()==2,"Grabber depth and same-origin restriction");manager.saveProject(p);check(manager.addProject(p,"Main queue",true)==2&&manager.addProject(p,"Main queue",true)==0,"Saved grabber project deduplication");auto snapshot=manager.snapshot();manager.save();{Manager reopened(root/L"state");check(reopened.snapshot()["Downloads"].size()==snapshot["Downloads"].size(),"Download history reload");check(fs::exists(root/L"state"/L"state.before-native.json"),"Migration backup retained");}check(fs::exists(root/L"state"/L"state.json.bak"),"Atomic state backup");rejects([&]{manager.receive({{"action","media"},{"url","https://www.youtube.com/watch?v=abcdefghijk"},{"height",1080}});},"Browser handoff without captured stream rejected");auto incoming=manager.receive({{"action","add"},{"url",fixture.url("/browser-new")},{"filename","from-browser.bin"}});check(str(incoming->data,"Status")=="Awaiting confirmation","Browser handoff waits for confirmation");check(manager.receive({{"action","add"},{"url",fixture.url("/browser-new")},{"filename","from-browser.bin"}})->id()==incoming->id(),"Duplicate browser handoff returns same record");
 for(const char* route:{"/stall-headers","/stall-body"}){
  auto cancellation=std::make_shared<Cancel>();auto before=fixture.stalled.load();
  auto pending=std::async(std::launch::async,[&]{try{Http response(fixture.url(route),{},manager.state["Settings"],*cancellation);response.all(100,*cancellation);return false;}catch(const Cancelled&){return true;}catch(...){return false;}});
  for(int i=0;i<200&&fixture.stalled.load()==before;++i)Sleep(5);
  auto start=GetTickCount64();cancellation->stop=true;bool stopped=pending.get();
  check(stopped&&fixture.stalled.load()>before&&GetTickCount64()-start<1500,route==std::string("/stall-headers")?"Pause cancels pending HTTP headers promptly":"Pause cancels stalled HTTP body promptly");
 }
 {
  auto p=defaultSettings();auto basic=browserPreferences(p);
  check(str(basic,"forceKey")=="Ctrl"&&str(basic,"bypassKey")=="Alt","Default browser keys match the reference convention");
  check(yes(basic,"forceClick")&&yes(basic,"forceSkipWeb")&&!yes(basic,"captureWebPlayers"),"Browser capture defaults require an intentional click");
  check(basic["panelTypes"]["mp4"]==true&&basic["contextMenu"]["All"]==true,"Default panel types and browser menus remain enabled");
  validatePanelHosts("* a *.example.test");check(true,"Panel exceptions accept wildcard-all and short host patterns");
  auto custom=p;custom["CaptureForceKey"]="Alt+Ctrl+Shift+Ins";custom["CaptureBypassKey"]="Del";custom["CaptureForceClick"]=false;custom["VideoPanelTypes"]={{"mp4",true},{"webm",false}};custom["VideoPanelMinKb"]={{"mp4",1024}};custom["VideoPanelExcludedHosts"]="*.example.test media.*";custom["BrowserContextMenus"]={{"chromium",{{"Link",false},{"All",true}}},{"firefox",{{"Link",true},{"All",false}}}};
  validateBrowserSettings(custom);check(!yes(browserPreferences(custom),"forceClick")&&browserPreferences(custom)["panelMinKb"]["mp4"]==1024,"Custom panel and key rules serialize");
  check(browserPreferences(custom,"chrome.exe")["contextMenu"]["Link"]==false&&browserPreferences(custom,"FIREFOX.EXE")["contextMenu"]["All"]==false,"Context menus use the verified browser family");
  auto broken=custom;broken["CaptureForceKey"]="Ins+Ins";rejects([&]{validateBrowserSettings(broken);},"Repeated force keys rejected");
  broken=custom;broken["CaptureForceKey"]="Del";rejects([&]{validateBrowserSettings(broken);},"Force key rejects bypass-only Delete");
  broken=custom;broken["CaptureBypassKey"]="Ctrl+Alt";broken["CaptureForceKey"]="Alt+Ctrl";rejects([&]{validateBrowserSettings(broken);},"Equivalent differently ordered key combinations conflict");
  broken=custom;broken["VideoPanelMinKb"]["mp4"]=-1;rejects([&]{validateBrowserSettings(broken);},"Negative panel minimum rejected");
  broken=custom;broken["VideoPanelMinKb"]["webm"]=1.5;rejects([&]{validateBrowserSettings(broken);},"Fractional panel minimum rejected");
  broken=custom;broken["VideoPanelMinKb"]["exe"]=1;rejects([&]{validateBrowserSettings(broken);},"Panel minimum must name a configured type");
  broken=custom;broken["VideoPanelTypes"]["mp4"]="false";rejects([&]{validateBrowserSettings(broken);},"Panel file type flags must be boolean");
  broken=custom;broken["VideoPanelExcludedHosts"]="https://example.test/path";rejects([&]{validateBrowserSettings(broken);},"Panel host exceptions reject URL syntax");
  broken=custom;broken["BrowserContextMenus"]["chromium"]["Unknown"]=true;rejects([&]{validateBrowserSettings(broken);},"Unsupported menu commands rejected");
  Manager customManager(root/L"custom-browser-settings");customManager.setSettings(custom);Manager customRestored(root/L"custom-browser-settings");check(browserPreferences(customRestored.state["Settings"])==browserPreferences(custom),"Browser customization survives persistence");
  check(yes(basic,"panelEnabled")&&yes(basic,"captureAllowed")&&str(basic,"panelPosition")=="Top right","Browser settings preserve existing panel defaults");
  p["CaptureExcludedUrls"]="https://example.com/private/*\nhttps://*.example.org/files/*";
  p["VideoPanelPosition"]="Bottom left";p["VideoPanelCompact"]=true;p["CaptureForceKey"]="Alt+Shift";
  validateBrowserSettings(p);check(browserPreferences(p)["excludedUrls"].size()==2&&yes(browserPreferences(p),"panelCompact"),"Desktop panel preferences serialize without private settings");
  check(!basic.contains("ProxySecret")&&!basic.contains("SiteLogins"),"Browser settings omit stored credentials");
  auto invalid=p;invalid["CaptureExcludedUrls"]="file:///C:/secret/*";rejects([&]{validateBrowserSettings(invalid);},"Address exceptions reject non-web schemes");
  invalid=p;invalid["CaptureForceKey"]="Alt";rejects([&]{validateBrowserSettings(invalid);},"Force and bypass keys must differ");
  invalid=p;invalid["VideoPanelMenuWidth"]=900;rejects([&]{validateBrowserSettings(invalid);},"Panel menu size has explicit bounds");
  invalid=p;invalid["VideoPanelPosition"]="elsewhere";rejects([&]{validateBrowserSettings(invalid);},"Panel position validates enum");
  invalid=p;invalid["CaptureExcludedHosts"]="https://example.com";rejects([&]{validateBrowserSettings(invalid);},"Capture host exceptions reject URL syntax");
  Manager prefs(root/L"browser-settings");prefs.setSettings(p);Manager restored(root/L"browser-settings");
  check(browserPreferences(restored.state["Settings"])==browserPreferences(p),"Browser preferences survive a state reload");
 }
 catalogChecks(root);
 zipChecks(root);
 recycleChecks(root);
 queueChecks(root,fixture);wakeChecks(root);
 auto failedProject=explore({{"Id",guid()},{"Name","Error fixture"},{"StartUrl",fixture.url("/expired")},{"Extensions","zip"},{"Depth",0},{"MaxPages",1}},manager.state["Settings"],c);check(failedProject["Errors"].size()==1&&num(failedProject,"PagesVisited")==1,"Grabber records HTTP failures for its error dialog");
 authenticationChecks(root);
 guiChecks(root,fixture);
 connectionChecks(root);proxyPolicyChecks(root);browserProxyChecks(root,fixture);offlineModelChecks(root);postChecks(root);captureReceiptChecks(root);completionActionChecks(root);checkpointChecks(root);
 schedulerChecks(root,fixture);scannerChecks(root,fixture);
 reliabilityChecks(root,fixture);
 duplicateChecks(root,fixture);overwriteChecks(root,fixture);
 workflowChecks(root,fixture);quotaChecks(root,fixture);
 recoveryChecks(root,fixture);
 transferChecks(root);
 mediaChecks(manager,root);parallelMediaChecks(manager);parallelMediaChecks(manager,true);audioStreamingChecks(manager,root);streamRecoveryChecks(root);
 adaptiveChecks(manager,root);
 bridgeChecks(manager);
 auto report=Json{{"passed",passed},{"failed",failed},{"language","C++17"},{"engine","WinHTTP"},{"fixtureRequests",fixture.requests.load()},{"root",utf8(root.wstring())},{"finished",date()}};atomicText(appDir()/L"native-test-evidence.json",report.dump(2));
 }catch(const std::exception& e){++failed;std::cerr<<"UNEXPECTED "<<e.what()<<std::endl;}std::cout<<passed<<" passed, "<<failed<<" failed"<<std::endl;CoUninitialize();WSACleanup();return failed?1:0;}
