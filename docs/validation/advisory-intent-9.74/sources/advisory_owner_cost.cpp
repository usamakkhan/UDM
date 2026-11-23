#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "BodyBufferStorage.hpp"
#include "StreamHash.hpp"
#include "third_party/json.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <regex>
using Json=nlohmann::json;using namespace udm;namespace fs=std::filesystem;
using Clock=std::chrono::steady_clock;
static double elapsed(Clock::time_point begin){return std::chrono::duration<double>(Clock::now()-begin).count();}
__declspec(noinline) static bool exactHex(const std::string& value){return value.size()==64&&std::all_of(value.begin(),value.end(),[](unsigned char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');});}
struct Result{std::string digest;double hash=0,release=0,total=0;};
int wmain(int argc,wchar_t**argv){
 if(argc!=3)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);const auto expected=std::string(argv[2],argv[2]+wcslen(argv[2]));Json checks=Json::array(),rows=Json::array();std::string error;
 const auto check=[&](bool ok,const char* label){checks.push_back({{"passed",ok},{"name",label}});if(!ok)throw std::runtime_error(label);};
 try{
  for(int trial=0;trial<8;++trial)for(int index=0;index<2;++index){
   const bool releaseInside=(index+(trial%2))%2!=0;auto body=BodyBufferStorage::reserve(BodyBufferStorage::maxBytes);check(bool(body),"Each fresh sample obtains the one existing budgeted allocation");
   for(size_t i=0;i<body->size();++i)body->data()[i]=static_cast<unsigned char>((i*37+17)%256);
   std::weak_ptr<BodyBufferStorage> weak=body;
   auto task=std::async(std::launch::async,[body=std::move(body),releaseInside]()mutable{
    Result r;const auto began=Clock::now();StreamHash hash;for(size_t offset=0;offset<body->size();offset+=1048576)hash.update(body->data()+offset,std::min<size_t>(1048576,body->size()-offset));r.digest=hash.finish();r.hash=elapsed(began);
    if(releaseInside){const auto releasing=Clock::now();body.reset();r.release=elapsed(releasing);}r.total=elapsed(began);return r;
   });
   task.wait();const bool expiredBeforeGet=weak.expired();auto competing=BodyBufferStorage::reserve(8);const bool budgetAvailableBeforeGet=bool(competing);competing.reset();
   check(expiredBeforeGet==releaseInside&&budgetAvailableBeforeGet==releaseInside,"Completed task captures retain storage and budget until retrieval unless explicitly released on the worker");
   const auto getting=Clock::now();const auto r=task.get();const auto getSeconds=elapsed(getting);
   check(r.digest==expected,"Both lifetimes hash the same independently verified complete 64 MiB body");check(weak.expired(),"The sample leaves no buffer alive after task retrieval");
   rows.push_back({{"trial",trial+1},{"releaseInsideWorker",releaseInside},{"expiredBeforeGet",expiredBeforeGet},{"budgetAvailableBeforeGet",budgetAvailableBeforeGet},{"hashSeconds",r.hash},{"workerReleaseSeconds",r.release},{"workerTotalSeconds",r.total},{"getSeconds",getSeconds},{"sha256",r.digest}});
  }
  const std::regex reference("[a-f0-9]{64}");
  for(int byte=0;byte<256;++byte){auto s=expected;s[17]=static_cast<char>(byte);check(exactHex(s)==std::regex_match(s,reference),"Exact scanner and reference regex agree for every possible byte in a digest");}
  for(const auto s:{std::string(),expected.substr(1),expected+"0",expected+"\n",std::string("+")+expected})check(exactHex(s)==std::regex_match(s,reference),"Exact scanner preserves whole-string and length rejection");
  volatile unsigned accepted=0;constexpr unsigned repeats=128;auto began=Clock::now();for(unsigned i=0;i<repeats;++i)accepted+=std::regex_match(expected,std::regex("[a-f0-9]{64}"));const auto regexSeconds=elapsed(began);began=Clock::now();for(unsigned i=0;i<repeats;++i)accepted+=exactHex(expected);const auto scannerSeconds=elapsed(began);check(accepted==2*repeats,"Timed validators accept the same valid digest");
  std::ofstream(root/L"validation-cost.json")<<Json{{"iterations",repeats},{"regexTotalSeconds",regexSeconds},{"scannerTotalSeconds",scannerSeconds}}.dump(2);
 }catch(const std::exception& e){error=e.what();}
 std::ofstream(root/L"results.json")<<Json{{"passed",error.empty()},{"checks",checks},{"rows",rows},{"error",error},{"writesPayloadToDisk",false},{"nativeProductComparison",false}}.dump(2);std::cout<<checks.size()<<" checks; "<<error<<"\n";return error.empty()?0:1;
}
