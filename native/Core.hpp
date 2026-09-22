#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WINDOWS_
#include <winsock2.h>
#include <windows.h>
#endif
#include <winhttp.h>
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <memory>
#include <chrono>
#include <stdexcept>
#include "third_party/json.hpp"
namespace udm {
using Json=nlohmann::json;
namespace fs=std::filesystem;
using i64=long long;
using Bytes=std::vector<unsigned char>;
using Headers=std::map<std::string,std::string>;
using Lock=std::lock_guard<std::recursive_mutex>;
std::wstring wide(const std::string&);
std::string utf8(const std::wstring&);
std::string lower(std::string);
std::string trim(std::string);
std::string str(const Json&,const char*,std::string fallback="");
i64 num(const Json&,const char*,i64 fallback=0);
bool yes(const Json&,const char*,bool fallback=false);
double real(const Json&,const char*,double fallback=0);
std::string guid(),safeName(std::string),category(const std::string&),bytes(double);
template<class T,std::enable_if_t<std::is_integral_v<T>,int> =0>std::string bytes(T n){return bytes(static_cast<double>(n));}
std::vector<std::string> words(const std::string&);
std::vector<std::string> expand(const std::string&);
std::string b64(const Bytes&);
Bytes unb64(const std::string&);
std::string protect(const std::string&),reveal(const std::string&);
Json dictionary(const Json&),legacyDictionary(const Json&);
Headers readHeaders(const Json&);
void validateHeaders(const Headers&);
std::string date(i64 ms=0); i64 epoch(),parseDate(const Json&);
std::string readText(const fs::path&,size_t limit=32*1024*1024);
void atomicText(const fs::path&,const std::string&,bool backup=true);
void writeBytes(const fs::path&,const Bytes&);
fs::path appDir(),defaultData();
std::wstring quote(const std::wstring&);
std::string fileHash(const fs::path&);
void markZone(const fs::path&);
struct Handle {HANDLE h=INVALID_HANDLE_VALUE;Handle()=default;explicit Handle(HANDLE v):h(v){}~Handle(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);}Handle(const Handle&)=delete;Handle& operator=(const Handle&)=delete;Handle(Handle&& x)noexcept:h(x.h){x.h=INVALID_HANDLE_VALUE;}explicit operator bool()const{return h&&h!=INVALID_HANDLE_VALUE;}};
struct Cancelled:std::runtime_error{Cancelled():runtime_error("Download paused.") {}};
struct Changed:std::runtime_error{using runtime_error::runtime_error;};
// -1 = absent/malformed; -2 = beyond the automatic five-minute wait budget.
int retryAfterDelay(const std::string&,i64 now);
struct HttpRejected:std::runtime_error{
 DWORD status;int retryAfterMs;
 explicit HttpRejected(DWORD,const std::string& retryAfter="");
 bool retryable()const;int delay(int attempt)const;
};
std::string recoveryPage(const Json&);
struct Cancel {std::atomic_bool stop{false};std::shared_ptr<Cancel> parent;void check()const{if(stop||(parent&&parent->cancelled()))throw Cancelled();}bool cancelled()const{return stop||(parent&&parent->cancelled());}void wait(int ms)const;};
struct Url {std::string full,scheme,host,path,origin,query;INTERNET_PORT port=0;explicit Url(const std::string&);};
std::string combineUrl(const std::string&,const std::string&);
bool hostIs(const std::string&,const std::string&);
std::map<std::string,std::string> query(const std::string&);
std::string unescape(const std::string&);
struct Rate {std::mutex mutex;std::chrono::steady_clock::time_point next{};void wait(size_t,i64,const Cancel&);};
struct Worker{int number=0;i64 start=0,end=0,position=0,received=0;std::string state="Waiting";};
struct Job {Json data;std::shared_ptr<Job> video,audio;std::vector<Worker> workers;double speed=0;i64 prior=0;std::optional<i64> sessionLimit;explicit Job(Json j);Json snapshot()const;fs::path target()const;std::string id()const{return str(data,"Id");}};
using JobPtr=std::shared_ptr<Job>;
Json defaultSettings(),defaultQueue(std::string name="Main queue");
bool inWindow(const Json&,i64 now=0,bool manual=false);
class Manager {
 std::map<std::string,std::shared_ptr<Cancel>> active;
 std::vector<std::thread> threads;
 std::set<std::string> schedulePaused,manualQueues;
 std::chrono::steady_clock::time_point lastTick=std::chrono::steady_clock::now();
 int ticks=0;
 bool stopping=false;
 std::string refreshId; i64 refreshUntil=0;
 void start(JobPtr);
public:
 mutable std::recursive_mutex mutex;
 fs::path root;
 Json state;
 std::vector<JobPtr> jobs;
 Rate globalRate;
 std::string storageError;
 std::function<void(JobPtr,bool)> event;
 explicit Manager(fs::path);
 ~Manager();
 void save();Json snapshot()const;
 void tick();void stop();
 JobPtr add(std::string url,std::string folder="",std::string name="",std::string queue="Main queue",bool paused=true,Headers headers={},std::string expected="");
 JobPtr receive(const Json&);
 void resume(JobPtr);void pause(JobPtr);void remove(JobPtr);bool isActive(JobPtr)const;
 void queueRun(const std::string&,bool);void move(JobPtr,int);
 void configure(JobPtr,const Json&);
 void relocate(JobPtr,const fs::path&);
 void updateCompleted(JobPtr,const Json&);
 JobPtr redownload(JobPtr);
 void setMembership(JobPtr,bool,const std::string& queue="");
 void beginPrefetch(JobPtr);void endPrefetch(JobPtr);
 void recoverFileOperation();
 JobPtr findDuplicate(const std::string&,const Headers&,JobPtr ignore={})const;
 JobPtr offerDownload(const std::string&,const std::string& folder="",const std::string& name="",const std::string& queue="Main queue",bool paused=true,const Headers& headers={});
 JobPtr resolveDuplicate(JobPtr,const std::string& choice);
 void publishFile(JobPtr,const fs::path& staging,const std::string& hash);
 void recoverReplacements();
 std::vector<JobPtr> pendingOffers;
 bool canRefreshAddress(JobPtr)const;
 void beginAddressRefresh(JobPtr);void cancelAddressRefresh(JobPtr);
 JobPtr captureAddressRefresh(const std::string&,const Headers&,const std::string&,const std::string&);
 Json addressRefreshCandidate(JobPtr)const;
 void refreshAddress(JobPtr,const std::string&,std::optional<Headers> headers=std::nullopt,const std::string& sourcePage="");
 void setSettings(const Json&);void setQueue(const Json&);void deleteQueue(const std::string&);
 void saveProject(const Json&);int addProject(const Json&,const std::string&,bool);
 std::vector<std::string> categories()const;
 int retries(const std::string&)const;
 void charge(size_t,const Cancel&,Rate&,JobPtr);
 void progress(JobPtr,size_t,size_t segment,Worker*);
};
// A transfer owns its pool; request headers and authentication stay request-local.
class HttpSession {
 HINTERNET session=nullptr;
 std::mutex mutex;
 std::map<std::string,HINTERNET> connections;
public:
 explicit HttpSession(const Json&);
 ~HttpSession();
 HttpSession(const HttpSession&)=delete;HttpSession& operator=(const HttpSession&)=delete;
 HINTERNET handle()const{return session;}
 HINTERNET connect(const Url&);
};
struct HttpAsyncState;
struct Http {
private:
 std::unique_ptr<HttpAsyncState> async;
 void closeRequest() noexcept;
 void prepareOperation();
 DWORD awaitOperation(BOOL,const Cancel&,const char*);
public:
 std::shared_ptr<HttpSession> pool;
 HINTERNET session=nullptr,connection=nullptr,request=nullptr;
 DWORD status=0;std::string finalUrl;
 Http(const std::string&,const Headers&,const Json&,const Cancel&,std::optional<i64> start={},std::optional<i64> end={},std::string validator="",const Bytes* body=nullptr,bool redirects=true,std::shared_ptr<HttpSession> pool={});
 ~Http();Http(const Http&)=delete;Http& operator=(const Http&)=delete;
 std::string header(const wchar_t*)const;
 size_t read(void*,size_t,const Cancel&);
 Bytes all(size_t,const Cancel&);
};
void transfer(Manager&,JobPtr,const std::shared_ptr<Cancel>&,JobPtr limitOwner={},std::shared_ptr<Rate> rate={});
void mediaTransfer(Manager&,JobPtr,const std::shared_ptr<Cancel>&);
void validateAdaptive(const Json&);
JobPtr receiveAdaptive(Manager&,const Json&);
void adaptiveTransfer(Manager&,JobPtr,const std::shared_ptr<Cancel>&);
void validateSource(const std::string&);void validateStream(const std::string&);
void setStreams(Manager&,JobPtr,const std::string&,const std::string&);
void validateSabr(const Json&,const std::string&);
using StreamRead=std::function<size_t(void*,size_t,const Cancel&)>;
using StreamTransport=std::function<StreamRead(const std::string&,const Bytes&)>;
void sabrTransfer(Manager&,JobPtr,const std::shared_ptr<Cancel>&,StreamTransport transport={});
std::string execute(const fs::path&,const std::vector<std::wstring>&,int,const Cancel&);
Json explore(const Json&,const Json&,const Cancel&,std::function<void(std::string)> report={});
std::string pipeName();Json send(const Json&,int timeout=1500);
class PipeServer {
 Manager& manager;std::thread thread;std::atomic_bool stopping{false};HANDLE stopEvent=nullptr;std::function<void()> show;
 void listen();
public:PipeServer(Manager&,std::function<void()>);~PipeServer();
};
int nativeHost();
Json diagnostics(),endpoints();
class Monitor {
 Handle device;
public:Monitor();void watch(const std::vector<DWORD>&);Json snapshot();
};
}
