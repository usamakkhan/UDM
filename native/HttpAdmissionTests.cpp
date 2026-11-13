#include "HttpAdmission.hpp"
#include "PacProxy.hpp"
#include "QueueRetry.hpp"
#include <future>
#include <iostream>
using namespace udm;
static Json checks=Json::array();
static void check(bool passed,const char* name){checks.push_back({{"name",name},{"passed",passed}});if(!passed)throw std::runtime_error(name);}
struct PendingAdmission {
 std::shared_ptr<Cancel> cancel=std::make_shared<Cancel>();
 std::future<std::unique_ptr<HttpAdmissionLease>> result;
 PendingAdmission(std::shared_ptr<HttpAdmissionContext> context,std::shared_ptr<void> transport){
  result=std::async(std::launch::async,[context,transport,token=cancel]{return std::make_unique<HttpAdmissionLease>(context,transport,*token);});
 }
 ~PendingAdmission(){cancel->stop=true;if(result.valid())result.wait();}
 bool ready(int milliseconds){return result.wait_for(std::chrono::milliseconds(milliseconds))==std::future_status::ready;}
};
struct BusyOrigin {
 std::shared_ptr<HttpAdmissionContext> context=std::make_shared<HttpAdmissionContext>();
 std::shared_ptr<void> first=std::make_shared<int>(1),second=std::make_shared<int>(2);
 Cancel cancel;
 std::unique_ptr<HttpAdmissionLease> a=std::make_unique<HttpAdmissionLease>(context,first,cancel);
 std::unique_ptr<HttpAdmissionLease> b=std::make_unique<HttpAdmissionLease>(context,second,cancel);
 BusyOrigin(){
  a->accepted();b->accepted();a->progress();b->progress();
  auto third=std::make_shared<int>(3);
  {HttpAdmissionLease refused(context,third,cancel);check(!refused.refused(),"One refused open does not establish capacity");}
  {HttpAdmissionLease refused(context,third,cancel);check(refused.refused()==context,"Repeated refusal with productive peers establishes capacity");}
 }
};
static int postRecoveryFixture(const fs::path& output,const fs::path& input){
 const auto config=Json::parse(readText(input));const auto url=str(config,"url"),mode=str(config,"mode"),method=str(config,"method","POST");
 Url address(url);if(address.host!="127.0.0.1"&&address.host!="udm-post-recovery.invalid")throw std::runtime_error("A local recovery fixture is required.");
 Manager manager(output/L"catalog");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((output/L"files").wstring());prefs["CategoryFolders"]=false;prefs["PrefetchFileInfo"]=false;prefs["StopOnWebPage"]=false;prefs["Connections"]=1;prefs["Retries"]=method=="GET"?0:3;prefs["ServerResponseTimeoutSeconds"]=1;
 const auto proxy=str(config,"proxy");prefs["ProxyMode"]=proxy.empty()?"Connect directly":"Use a proxy server";prefs["Proxy"]=proxy;prefs["ProxyBypass"]="";
 if(!proxy.empty()){Url endpoint("http://"+proxy+"/");if(endpoint.host!="127.0.0.1")throw std::runtime_error("The fixture proxy must be loopback.");prefs["ProxyUser"]="fixture-user";prefs["ProxySecret"]=protect("fixture-secret");prefs["ProxyAuthMode"]="Automatic";}
 manager.setSettings(prefs);auto queues=manager.state["Queues"];for(auto& queue:queues)if(str(queue,"Name")=="Main queue"){queue["FileRetries"]=2;queue["FileRetryLimitEnabled"]=true;queue["RetryDelaySeconds"]=1;}manager.setQueues(queues);
 Json request=Json::object();if(method=="POST")request={{"method","POST"},{"url",url},{"contentType","application/octet-stream"},{"body",str(config,"body")}};
 auto job=manager.add(url,utf8((output/L"files").wstring()),"response.bin","Main queue",false,{},str(config,"sha256"),request);manager.queueRun("Main queue",true);
 auto waitFor=[&](bool wantComplete){const auto until=GetTickCount64()+15000;while(GetTickCount64()<until){manager.dispatchQueueWork();{Lock lock(manager.mutex);const auto status=str(job->data,"Status");if(!manager.isActive(job)&&(status=="Complete"||(!wantComplete&&status=="Failed")))return;}Sleep(10);}};
 const bool initialComplete=mode=="success"||method=="GET";waitFor(initialComplete);
 std::string initialStatus;bool attempted=false;{Lock lock(manager.mutex);initialStatus=str(job->data,"Status");attempted=yes(job->data,"PostAttempted");}
 const auto hold=GetTickCount64()+2500;while(GetTickCount64()<hold){manager.dispatchQueueWork();Sleep(10);}
 i64 automaticAttempts;std::string afterAutomatic;bool blocked;{Lock lock(manager.mutex);automaticAttempts=num(job->data,"QueueAttempts");afterAutomatic=str(job->data,"Status");blocked=queueTransferBlocked(job->data);}
 if(mode=="explicit"){manager.resume(job);manager.dispatchQueueWork();waitFor(true);}
 manager.stop();const bool exists=fs::is_regular_file(job->target()),exact=exists&&fileHash(job->target())==str(config,"sha256");const auto finalStatus=str(job->data,"Status");
 const bool initialCorrect=initialStatus==(initialComplete?"Complete":"Failed");
 const bool automaticCorrect=method=="GET"?(proxy.empty()?automaticAttempts==1:automaticAttempts<=1):initialComplete?automaticAttempts==0:automaticAttempts==0&&attempted&&blocked&&afterAutomatic=="Failed";
 const bool finalCorrect=mode=="success"||mode=="explicit"||method=="GET"?finalStatus=="Complete"&&exact:finalStatus=="Failed"&&!exists;
 const bool passed=initialCorrect&&automaticCorrect&&finalCorrect;
 atomicText(output/L"results.json",Json{{"passed",passed},{"initialStatus",initialStatus},{"postAttempted",attempted},{"automaticQueueRetries",automaticAttempts},{"afterAutomaticStatus",afterAutomatic},{"blockedAfterAutomatic",blocked},{"finalStatus",finalStatus},{"sha256_ok",exact},{"published",exists},{"error",str(job->data,"Error")}}.dump(2),false);return passed?0:1;
}
static int proxyAdmissionFixture(const fs::path& output,const fs::path& input){
 WSADATA sockets{};if(WSAStartup(MAKEWORD(2,2),&sockets))throw std::runtime_error("Fixture socket initialization failed.");
 struct Cleanup{~Cleanup(){WSACleanup();}} cleanup;
 const auto config=Json::parse(readText(input));const auto proxy=str(config,"proxy"),digest=str(config,"sha256");
 const bool timeoutCase=yes(config,"headerTimeout"),pauseWaiter=yes(config,"pauseWaiter"),pauseActive=yes(config,"pauseActive"),capReduction=yes(config,"capReduction");
 if(capReduction&&!pauseActive)throw std::runtime_error("Cap reduction requires an active pause.");
 Url endpoint("http://"+proxy+"/");if(endpoint.host!="127.0.0.1"||digest.size()!=64)throw std::runtime_error("A loopback proxy fixture and SHA-256 are required.");
 const std::string target="http://udm-account-fixture.invalid/bench/file.bin";
 struct Attempt {
  std::unique_ptr<Manager> manager;JobPtr job;Json prefs;
  ~Attempt(){if(manager)manager->stop();}
 };
 std::vector<std::unique_ptr<Attempt>> attempts;
 auto start=[&](size_t index,const char* user){
  auto item=std::make_unique<Attempt>();auto root=output/wide("job-"+std::to_string(index));
  item->manager=std::make_unique<Manager>(root/L"catalog");item->prefs=item->manager->state["Settings"];
  auto& prefs=item->prefs;prefs["ProxyMode"]="Use a proxy server";prefs["Proxy"]=proxy;prefs["ProxyBypass"]="";
  prefs["DownloadFolder"]=utf8((root/L"files").wstring());
  prefs["ProxyUser"]=user;prefs["ProxySecret"]=protect("fixture-password");prefs["ProxyAuthMode"]=str(config,"authMode","Basic");prefs["Retries"]=3;
  if(timeoutCase&&index==3){prefs["Retries"]=1;prefs["ServerResponseTimeoutSeconds"]=1;}
  prefs["Connections"]=1;prefs["CategoryFolders"]=false;prefs["PrefetchFileInfo"]=false;prefs["StopOnWebPage"]=false;
  item->manager->setSettings(prefs);item->job=item->manager->add(target+(timeoutCase&&index==3?"?stallheaders=1":""),utf8((root/L"files").wstring()),"payload.bin","Main queue",true,{},digest);
  {Lock lock(item->manager->mutex);item->manager->resume(item->job);item->manager->dispatchQueueWork();}attempts.push_back(std::move(item));
 };
 start(1,"account-a");start(2,"account-a");
 auto receiving=[&]{for(size_t i=0;i<2;++i){Lock lock(attempts[i]->manager->mutex);if(num(attempts[i]->job->data,"Received")<256*1024)return false;}return true;};
 const auto progressDeadline=GetTickCount64()+10000;while(!receiving()&&GetTickCount64()<progressDeadline)Sleep(25);
 const bool peersReceiving=receiving();start(3,"account-a");
 auto queued=[&]{Lock lock(attempts[2]->manager->mutex);for(const auto& worker:attempts[2]->job->workers)if(worker.state=="Waiting for server connection")return true;return false;};
 const auto waitDeadline=GetTickCount64()+3000;while(!queued()&&GetTickCount64()<waitDeadline)Sleep(25);
 const bool queuedBeforeIndependent=queued();bool pausedCleanly=false,resumed=false,peersActiveDuringPause=false,waiterReceivingBeforeResume=false;double pauseMilliseconds=0,slotReleaseToWaiterMilliseconds=0;i64 pausedBytes=0;
 if(pauseWaiter||pauseActive){
  const size_t pausedIndex=pauseActive?0:2;auto& item=attempts[pausedIndex];const auto began=std::chrono::steady_clock::now();item->manager->pause(item->job);
  const auto until=GetTickCount64()+1500;
  for(;;){bool active;{Lock lock(item->manager->mutex);active=item->manager->isActive(item->job);}if(!active||GetTickCount64()>=until)break;Sleep(5);}
  pauseMilliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count();
  {Lock lock(item->manager->mutex);pausedBytes=num(item->job->data,"Received");const bool progress=pauseActive?pausedBytes>0&&pausedBytes<num(item->job->data,"Size"):pausedBytes==0;pausedCleanly=!item->manager->isActive(item->job)&&str(item->job->data,"Status")=="Paused"&&progress&&pauseMilliseconds<1500;}
  peersActiveDuringPause=true;for(size_t i=0;i<2;++i)if(i!=pausedIndex){Lock lock(attempts[i]->manager->mutex);peersActiveDuringPause=peersActiveDuringPause&&attempts[i]->manager->isActive(attempts[i]->job);}
  if(pauseActive){
   const auto release=std::chrono::steady_clock::now();const auto until=GetTickCount64()+1500;
   for(;;){{Lock lock(attempts[2]->manager->mutex);waiterReceivingBeforeResume=num(attempts[2]->job->data,"Received")>0;}if(waiterReceivingBeforeResume||GetTickCount64()>=until)break;Sleep(5);}
   slotReleaseToWaiterMilliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-release).count();
  }
  Sleep(100);item->manager->resume(item->job);item->manager->dispatchQueueWork();
  {Lock lock(item->manager->mutex);resumed=item->manager->isActive(item->job);}
 }
 start(4,"account-b");
 const auto deadline=GetTickCount64()+45000;bool done=false;
 while(GetTickCount64()<deadline){done=std::all_of(attempts.begin(),attempts.end(),[](const auto& a){Lock lock(a->manager->mutex);const auto status=str(a->job->data,"Status");return (status=="Complete"||status=="Failed")&&!a->manager->isActive(a->job);});if(done)break;Sleep(25);}
 for(auto& a:attempts)a->manager->stop();
 Json records=Json::array();bool passed=peersReceiving&&(timeoutCase?!queuedBeforeIndependent:queuedBeforeIndependent)&&done&&(!(pauseWaiter||pauseActive)||(pausedCleanly&&resumed&&peersActiveDuringPause))&&(!pauseActive||(capReduction?!waiterReceivingBeforeResume:waiterReceivingBeforeResume));
 for(size_t i=0;i<attempts.size();++i){auto& a=attempts[i];Lock lock(a->manager->mutex);const bool exists=fs::is_regular_file(a->job->target());const bool exact=exists&&fileHash(a->job->target())==digest;const auto status=str(a->job->data,"Status"),error=str(a->job->data,"Error");
  const bool expected=timeoutCase&&i==2?status=="Failed"&&!exists&&(lower(error).find("timed out")!=std::string::npos||error.find("12002")!=std::string::npos):status=="Complete"&&exact;
  passed=passed&&expected;records.push_back({{"status",status},{"sha256_ok",exact},{"error",error}});}
 const bool differentCiphertexts=str(attempts[0]->prefs,"ProxySecret")!=str(attempts[1]->prefs,"ProxySecret");
 const bool sameCredentialKey=httpAdmissionKey(Url(target),attempts[0]->prefs,{})==httpAdmissionKey(Url(target),attempts[1]->prefs,{});
 atomicText(output/L"results.json",Json{{"passed",passed},{"peersReceiving",peersReceiving},{"queuedBeforeIndependent",queuedBeforeIndependent},{"differentCiphertexts",differentCiphertexts},{"sameCredentialKey",sameCredentialKey},{"pausedCleanly",pausedCleanly},{"resumed",resumed},{"peersActiveDuringPause",peersActiveDuringPause},{"pauseMilliseconds",pauseMilliseconds},{"pausedBytes",pausedBytes},{"waiterReceivingBeforeResume",waiterReceivingBeforeResume},{"slotReleaseToWaiterMilliseconds",slotReleaseToWaiterMilliseconds},{"jobs",records}}.dump(2),false);
 return passed?0:1;
}
int main(int argc,char** argv){
 fs::path output;if(argc>=2){output=argv[1];fs::create_directories(output);}
 try{
  if(argc==4&&std::string(argv[2])=="--post-recovery")return postRecoveryFixture(output,fs::path(wide(argv[3])));
  if(argc==4&&std::string(argv[2])=="--proxy-admission")return proxyAdmissionFixture(output,fs::path(wide(argv[3])));
  const Url a("http://example.test/first"),b("http://example.test/second"),other("http://example.test:8080/first");
  Json direct={{"ProxyMode","Connect directly"}};
  check(httpAdmissionKey(a,direct,{{"Authorization","one"}})==httpAdmissionKey(b,direct,{{"authorization","one"}}),"Origin admission ignores file paths and normalizes header names");
  check(httpAdmissionKey(a,direct,{})!=httpAdmissionKey(other,direct,{}),"Different origin ports have separate admission contexts");
  check(httpAdmissionKey(a,direct,{{"Cookie","one"}})!=httpAdmissionKey(a,direct,{{"Cookie","two"}}),"Different website cookies isolate capacity evidence");
  check(httpAdmissionKey(a,direct,{{"Authorization","one"}})!=httpAdmissionKey(a,direct,{{"Authorization","two"}}),"Different server credentials isolate capacity evidence");
  check(httpAdmissionKey(a,direct,{{"Cookie",std::string(1,char(0xff))}})!=httpAdmissionKey(a,direct,{{"Cookie",std::string(1,char(0xfe))}}),"Non-UTF-8 header bytes remain distinct in admission identity");
  check(httpAdmissionKey(a,direct,{{"Cookie","one"},{"cookie","two"}})!=httpAdmissionKey(a,direct,{{"Cookie","changed"},{"cookie","two"}}),"Duplicate header names retain every value in admission identity");
  auto proxy=direct;proxy["ProxyMode"]="Use a proxy server";proxy["Proxy"]="127.0.0.1:8080";
  check(httpAdmissionKey(a,direct,{})!=httpAdmissionKey(a,proxy,{}),"Direct and explicit proxy routes remain separate");
  auto alternate=proxy;alternate["Proxy"]="127.0.0.1:8081";
  check(httpAdmissionKey(a,proxy,{})!=httpAdmissionKey(a,alternate,{}),"Different resolved proxy endpoints remain separate");
  alternate=proxy;alternate["ProxySecret"]=protect("different");
  check(httpAdmissionKey(a,proxy,{})!=httpAdmissionKey(a,alternate,{}),"Different proxy credentials remain separate");
  auto account=proxy;account["ProxyUser"]="same-user";account["ProxySecret"]=protect("same-password");
  auto protectedAgain=account;protectedAgain["ProxySecret"]=protect("same-password");
  check(str(account,"ProxySecret")!=str(protectedAgain,"ProxySecret"),"The credential fixture uses independently randomized DPAPI blobs");
  check(httpAdmissionKey(a,account,{})==httpAdmissionKey(a,protectedAgain,{}),"The same proxy credential shares admission after independent encryption");
  auto otherUser=protectedAgain;otherUser["ProxyUser"]="other-user";
  check(httpAdmissionKey(a,account,{})!=httpAdmissionKey(a,otherUser,{}),"Different proxy usernames remain isolated with the same password");
  otherUser=account;otherUser["ProxySecret"]=protect("different-password");
  check(httpAdmissionKey(a,account,{})!=httpAdmissionKey(a,otherUser,{}),"A changed proxy password cannot reuse the old account's capacity evidence");
  otherUser=account;otherUser["ProxySecret"]="invalid-encrypted-setting";bool invalidCredential=false;
  try{httpAdmissionKey(a,otherUser,{});}catch(const std::exception&){invalidCredential=true;}
  check(invalidCredential,"An unreadable protected credential never collapses to an empty identity");
  check(httpAdmissionRefusal(ERROR_WINHTTP_CANNOT_CONNECT)&&httpAdmissionRefusal(ERROR_WINHTTP_CONNECTION_ERROR),"Only refused or reset opens qualify as connection-cap evidence");
  check(!httpAdmissionRefusal(ERROR_WINHTTP_TIMEOUT)&&!httpAdmissionRefusal(ERROR_WINHTTP_NAME_NOT_RESOLVED)&&!httpAdmissionRefusal(ERROR_WINHTTP_LOGIN_FAILURE),"Timeouts, DNS errors and authentication failures cannot establish a connection cap");
  {
   CurlRequestState response;response.proxyAuth=true;response.status=407;response.headersComplete=true;response.complete=true;response.result=CURLE_GOT_NOTHING;
   bool interrupted=false;try{response.check();}catch(const CurlResponseDisconnect&){interrupted=true;}
   check(interrupted,"A lost post-challenge proxy response is a receive disconnect rather than a terminal authentication rejection");
   response.proxyAuth=false;response.digest=true;response.status=401;response.result=CURLE_SEND_ERROR;interrupted=false;
   try{response.check();}catch(const CurlResponseDisconnect&){interrupted=true;}
   check(interrupted,"A lost Digest authentication continuation retains receive-disconnect classification");
   response.digest=false;response.proxyAuth=true;response.status=407;response.result=CURLE_OK;response.check();
   check(response.status==407,"A complete terminal proxy rejection remains available to authentication policy");
   response.result=CURLE_LOGIN_DENIED;bool terminal=false;
   try{response.check();}catch(const CurlResponseDisconnect&){}catch(const std::runtime_error&){terminal=true;}
   check(terminal,"Rejected proxy credentials do not become transient capacity evidence");
   response.result=CURLE_OPERATION_TIMEDOUT;bool timedOut=false;
   try{response.check();}catch(const CurlResponseDisconnect& failure){timedOut=failure.code==ERROR_WINHTTP_TIMEOUT&&!httpAdmissionRefusal(failure.code);}
   check(timedOut,"A proxy authentication response timeout retains its receive-timeout identity");
   response.proxyAuth=false;response.digest=true;response.status=401;timedOut=false;
   try{response.check();}catch(const CurlResponseDisconnect& failure){timedOut=failure.code==ERROR_WINHTTP_TIMEOUT;}
   check(timedOut,"Digest continuation timeouts remain receive failures instead of credential rejection");
   response.digest=false;response.proxyAuth=true;
   response.status=403;response.ready=true;response.result=CURLE_RECV_ERROR;bool bodyInterrupted=false;
   try{response.check();}catch(const CurlBodyDisconnect&){bodyInterrupted=true;}
   check(bodyInterrupted,"An interrupted terminal policy response keeps its existing body/status path");
   response.status=407;response.ready=false;response.result=CURLE_GOT_NOTHING;response.callbackError=std::make_exception_ptr(std::runtime_error("Invalid response headers"));bool invalidHeaders=false;
   try{response.check();}catch(const CurlResponseDisconnect&){}catch(const std::runtime_error&){invalidHeaders=true;}
   check(invalidHeaders,"A header-parser failure takes priority over authentication transport retry");
  }
  const CurlResponseDisconnect silent("Silent response",ERROR_WINHTTP_TIMEOUT),reset("Reset response");
  check(silent.code==ERROR_WINHTTP_TIMEOUT&&!httpAdmissionRefusal(silent.code)&&httpAdmissionRefusal(reset.code),"Receive timeout and actual disconnect exceptions carry distinct admission evidence");
  check(protocolFallbackError(silent),"A receive timeout preserves existing protocol fallback eligibility");
  {
   CurlRequestState post;post.submission=true;post.proxyAuth=true;
   check(CurlRequestState::prerequest(&post,nullptr,nullptr,0,0)==CURL_PREREQFUNC_OK,"The first POST request is admitted before transport submission");
   post.status=407;post.headersComplete=true;
   check(CurlRequestState::prerequest(&post,nullptr,nullptr,0,0)==CURL_PREREQFUNC_OK&&post.status==0&&!post.headersComplete,"A fresh proxy rejection permits one authentication continuation");
   check(CurlRequestState::prerequest(&post,nullptr,nullptr,0,0)==CURL_PREREQFUNC_ABORT,"A lost continuation cannot replay an empty or nonempty POST");
   bool blocked=false;try{post.check();}catch(const CurlPostReplayBlocked&){blocked=true;}
   check(blocked,"Replay prevention propagates an explicit submission-recovery error");
   CurlRequestState digest;digest.submission=true;digest.digest=true;digest.requestIssued=true;digest.status=401;digest.headersComplete=true;
   check(CurlRequestState::prerequest(&digest,nullptr,nullptr,0,0)==CURL_PREREQFUNC_OK,"A fresh Digest rejection permits its authenticated POST continuation");
   check(CurlRequestState::prerequest(&digest,nullptr,nullptr,0,0)==CURL_PREREQFUNC_ABORT,"The Digest rejection cannot authorize another submission after response loss");
   CurlRequestState get;get.requestIssued=true;get.status=200;
   check(CurlRequestState::prerequest(&get,nullptr,nullptr,0,0)==CURL_PREREQFUNC_OK&&get.status==200,"Non-submission request retry policy stays with the GET transport");
   CurlRequestState policy;policy.submission=true;policy.requestIssued=true;policy.proxyAuth=true;policy.status=403;
   check(CurlRequestState::prerequest(&policy,nullptr,nullptr,0,0)==CURL_PREREQFUNC_ABORT,"A terminal policy response cannot authorize POST replay");
  }
  auto automatic=direct;automatic["ProxyMode"]="Use Windows proxy / PAC settings";
  check(httpAdmissionKey(a,automatic,{}).empty(),"An unproven automatic route cannot join a shared context");
  automatic["HttpAdmissionSession"]="session-one";alternate=automatic;alternate["HttpAdmissionSession"]="session-two";
  check(httpAdmissionKey(a,automatic,{})!=httpAdmissionKey(a,alternate,{}),"Opaque Windows route sessions cannot share learned capacity");
  auto browser=direct;browser["ActiveBrowserSession"]={{"Origin","https://page.test"}};
  check(httpAdmissionKey(a,direct,{})!=httpAdmissionKey(a,browser,{}),"Browser session scope is part of admission identity");
  check(httpAdmissionKey(a,direct,{},Json{{"DisableHttp2",true}})!=httpAdmissionKey(a,direct,{}),"Connection policy changes isolate admission evidence");
  {
   check(usesWindowsProxy(Json::object())&&!usesWindowsProxy(direct),"Default proxy mode selects Windows while explicit direct mode does not");
   check(!usesWindowsProxy(Json{{"Proxy","127.0.0.1:8080"}}),"An implicit configured fixed proxy does not become Windows automatic mode");
   auto source=automatic;source["ProxyAutoConfigUrl"]="http://config.test/proxy.pac";source["ResolvedPacProxyScheme"]="https";
   source["ProxyUser"]="private-user";source["ProxySecret"]=protect("private-secret");source["ProtocolProxies"]={{"https",{{"Proxy","old.test:8080"}}}};
   WINHTTP_PROXY_RESULT_ENTRY entry{};
   auto resolved=proxyRouteSettings(source,entry);
   check(str(resolved,"ProxyMode")=="Connect directly"&&str(resolved,"Proxy").empty(),"A Windows direct result becomes an explicit direct transport");
   check(str(resolved,"ProxyUser").empty()&&str(resolved,"ProxySecret").empty(),"A direct route drops proxy credentials");
   check(!resolved.contains("ProtocolProxies")&&!resolved.contains("ProxyAutoConfigUrl")&&!resolved.contains("ResolvedPacProxyScheme"),"A pinned route cannot recursively resolve stale proxy configuration");
   auto independent=resolved;independent["HttpAdmissionSession"]="independent-session";
   check(httpAdmissionKey(a,resolved,{})==httpAdmissionKey(a,independent,{}),"Resolved Windows direct routes share evidence across independent sessions");
   wchar_t host[]=L"2001:db8::1";entry.fProxy=TRUE;entry.ProxyScheme=INTERNET_SCHEME_HTTPS;entry.pwszProxy=host;entry.ProxyPort=8443;
   resolved=proxyRouteSettings(source,entry);
   check(str(resolved,"Proxy")=="[2001:db8::1]:8443"&&str(resolved,"ResolvedPacProxyScheme")=="https", "A Windows HTTPS proxy result preserves scheme and IPv6 endpoint");
   check(str(resolved,"ProxyUser")=="private-user"&&str(resolved,"ProxySecret")==str(source,"ProxySecret"),"A selected proxy retains its configured credential context");
   independent=resolved;independent["ProxySecret"]=protect("different");
   check(httpAdmissionKey(a,resolved,{})!=httpAdmissionKey(a,independent,{}),"Resolved Windows routes still isolate different proxy credentials");
   entry.fBypass=TRUE;resolved=proxyRouteSettings(source,entry);
   check(str(resolved,"ProxyMode")=="Connect directly","Windows bypass results are pinned to direct transport");
   entry.fBypass=FALSE;entry.ProxyPort=0;bool rejected=false;
   try{proxyRouteSettings(source,entry);}catch(const std::exception&){rejected=true;}
   check(rejected,"A malformed selected proxy never silently becomes a direct route");
   entry.ProxyPort=8080;entry.ProxyScheme=INTERNET_SCHEME_FTP;rejected=false;
   try{proxyRouteSettings(source,entry);}catch(const std::exception&){rejected=true;}
   check(rejected,"An unsupported selected proxy scheme never silently becomes a direct route");
  }
  {
   auto context=std::make_shared<HttpAdmissionContext>();Cancel cancel;
   auto one=std::make_shared<int>(1),two=std::make_shared<int>(2);
   HttpAdmissionLease accepted(context,one,cancel);accepted.accepted();
   HttpAdmissionLease refused(context,two,cancel);
   check(!refused.refused()&&!refused.refused(),"Response headers alone do not establish productive capacity");
   accepted.progress();
   check(refused.refused()==context,"Earlier refusals become capacity evidence only after a matching peer delivers payload");
  }
  {
   BusyOrigin origin;PendingAdmission pending(origin.context,std::make_shared<int>(3));
   check(!pending.ready(100),"A new independent connection waits while proven capacity is occupied");
   const auto started=GetTickCount64();pending.cancel->stop=true;check(pending.ready(500),"Cancel promptly wakes an admission waiter");
   bool canceled=false;try{pending.result.get();}catch(const Cancelled&){canceled=true;}
   check(canceled&&GetTickCount64()-started<500,"Cancellation leaves the admission queue without opening a connection");
  }
  {
   BusyOrigin origin;PendingAdmission pending(origin.context,std::make_shared<int>(3));
   check(!pending.ready(100),"An occupied origin keeps a new transport queued");
   origin.a->eof();origin.a.reset();
   check(!pending.ready(100),"Drained keepalive responses retain capacity while their transport is alive");
   {
    HttpAdmissionLease reused(origin.context,origin.first,origin.cancel);reused.accepted();reused.progress();reused.eof();
    check(!pending.ready(100),"The same transport reuses its idle slot without admitting another transport");
   }
   origin.first.reset();check(pending.ready(1000),"Releasing an idle transport frees capacity for a queued download");
   auto admitted=pending.result.get();check(admitted!=nullptr,"The queued connection receives the released slot");
  }
  {
   BusyOrigin origin;PendingAdmission first(origin.context,std::make_shared<int>(3));PendingAdmission second(origin.context,std::make_shared<int>(4));
   check(!first.ready(100)&&!second.ready(100),"Multiple independent waiters respect the learned bound");
   origin.a.reset();
   const auto until=GetTickCount64()+1000;while(!first.ready(0)&&!second.ready(0)&&GetTickCount64()<until)Sleep(5);
   const bool firstReady=first.ready(0),secondReady=second.ready(0);
   check(firstReady!=secondReady,"One released slot admits exactly one waiter");
   auto lease=firstReady?first.result.get():second.result.get();lease.reset();
   auto& next=firstReady?second:first;check(next.ready(1000),"The next waiter advances after the first releases its slot");next.result.get();
  }
  {
   BusyOrigin origin;const auto started=GetTickCount64();
   HttpAdmissionLease helper(origin.context,origin.first,origin.cancel);
   check(GetTickCount64()-started<250,"An existing transfer retains its own worker admission policy");
   check(!helper.refused(),"An existing producer's optional worker uses within-transfer refusal recovery");
  }
  {
   BusyOrigin origin;origin.a.reset();
   {HttpAdmissionLease transition(origin.context,std::make_shared<int>(3),origin.cancel);check(transition.refused()==origin.context,"A refusal after a productive close retains shared recovery instead of spending ordinary retries");}
   PendingAdmission recovery(origin.context,std::make_shared<int>(4));
   check(!recovery.ready(40),"Recovery opens are paced during the server's close transition");
   check(recovery.ready(500),"One transient refusal cannot lower a proven two-connection bound");
   auto accepted=recovery.result.get();accepted->accepted();accepted->progress();
   origin.cancel.wait(600);accepted.reset();
   {HttpAdmissionLease fresh(origin.context,std::make_shared<int>(5),origin.cancel);check(fresh.refused()==origin.context,"A newly proven successful open invalidates older lower-capacity observations");}
   PendingAdmission repeat(origin.context,std::make_shared<int>(6));
   check(repeat.ready(500),"A later close transition requires fresh evidence before lowering capacity");repeat.result.get();
  }
  {
   BusyOrigin origin;origin.a.reset();
   {HttpAdmissionLease transition(origin.context,std::make_shared<int>(3),origin.cancel);transition.refused();}
   origin.cancel.wait(600);
   {HttpAdmissionLease sustained(origin.context,std::make_shared<int>(4),origin.cancel);check(sustained.refused()==origin.context,"Sustained fresh refusals can establish a genuinely reduced server capacity");}
   PendingAdmission next(origin.context,std::make_shared<int>(5));check(!next.ready(150),"A confirmed lower bound prevents repeated excess opens");
   origin.b.reset();check(next.ready(1000),"A lower-capacity waiter advances when its productive peer releases the slot");next.result.get();
  }
  {
   BusyOrigin origin;origin.a->eof();origin.a.reset();const auto started=GetTickCount64();
   origin.context->wait(origin.cancel,origin.first);
   check(GetTickCount64()-started<250,"A retry can immediately reuse its own transport's idle connection");
  }
  {
   BusyOrigin origin;origin.a->accepted(true);
   HttpAdmissionLease stream(origin.context,origin.first,origin.cancel);stream.accepted(true);stream.progress();
   {
    HttpAdmissionLease reset(origin.context,origin.first,origin.cancel);
    check(!reset.refused(),"A multiplexed stream failure does not establish an origin connection cap");
   }
   PendingAdmission pending(origin.context,std::make_shared<int>(3));
   check(!pending.ready(100),"A multiplexed transport still occupies one independent origin slot");
   origin.b.reset();
   check(pending.ready(1000),"Multiple streams on one transport count as one occupied connection");pending.result.get();
  }
  {
   BusyOrigin origin;origin.a->accepted(true);origin.a.reset();
   PendingAdmission pending(origin.context,std::make_shared<int>(3));
   check(!pending.ready(100),"Closing an unread multiplexed stream retains its reusable transport slot");
   origin.first.reset();check(pending.ready(1000),"Closing the multiplexed transport releases its retained slot");pending.result.get();
  }
  {
   BusyOrigin origin;PendingAdmission probe(origin.context,std::make_shared<int>(3));
   check(probe.ready(6000),"Expired capacity evidence permits a bounded recovery probe");
   auto lease=probe.result.get();PendingAdmission next(origin.context,std::make_shared<int>(4));
   check(!next.ready(100),"Only one capacity probe can be outstanding above the learned bound");
   lease->accepted();lease->progress();check(next.ready(1000),"A successful probe restores admission when server capacity increases");next.result.get();
  }
  {
   Cancel cancel;auto transport=std::make_shared<int>(0);std::vector<std::unique_ptr<HttpAdmissionLease>> held;
   for(unsigned index=0;index<256;++index)held.push_back(httpAdmission("bounded-test-"+std::to_string(index),transport,cancel));
   check(std::all_of(held.begin(),held.end(),[](const auto& lease){return lease!=nullptr;}),"The bounded context cache admits its supported live contexts");
   check(!httpAdmission("bounded-test-overflow",transport,cancel),"The context cache falls back without exceeding its live-context bound");
   held.front().reset();
   check(httpAdmission("bounded-test-replacement",transport,cancel)!=nullptr,"An unused context can be evicted for a new origin");
  }
  if(argc==3&&std::string(argv[2])=="--pac-integration"){
   WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data))throw std::runtime_error("Cannot initialize proxy route fixture sockets.");
   struct Cleanup{~Cleanup(){WSACleanup();}} cleanup;
   Json scripted={{"ProxyMode","Use automatic configuration script"},{"ProxyAutoConfigScript","function FindProxyForURL(url,host){if(url.indexOf('/second')>=0)return 'PROXY 127.0.0.1:18082; DIRECT';return 'PROXY 127.0.0.1:18081; DIRECT';}"}};
   Cancel cancel;auto first=resolvePacRoutes(scripted,a,cancel),second=resolvePacRoutes(scripted,b,cancel);
   check(first.size()==2&&second.size()==2,"The Windows resolver preserves the complete ordered PAC result");
   check(str(first[0],"Proxy")=="127.0.0.1:18081"&&str(second[0],"Proxy")=="127.0.0.1:18082","Proxy lookup uses each exact URL rather than an origin-wide route cache");
   check(str(first[1],"ProxyMode")=="Connect directly"&&str(second[1],"ProxyMode")=="Connect directly","Only explicitly returned direct alternatives are retained");
   cancel.stop=true;bool canceled=false;
   try{resolveWindowsProxyRoutes(automatic,a,cancel);}catch(const Cancelled&){canceled=true;}
   check(canceled,"A canceled Windows automatic lookup exits before route discovery");
  }
  if(!output.empty())atomicText(output/L"results.json",Json{{"passed",true},{"checks",checks}}.dump(2),false);
  std::cout<<checks.dump(2)<<"\n";return 0;
 }catch(const std::exception& error){
  if(!output.empty())atomicText(output/L"results.json",Json{{"passed",false},{"error",error.what()},{"checks",checks}}.dump(2),false);
  std::cerr<<error.what()<<"\n";return 1;
 }
}


