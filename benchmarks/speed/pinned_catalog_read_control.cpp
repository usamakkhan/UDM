#include "PinnedCatalogRead.hpp"
#include <iostream>
using namespace udm;

int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 3;
 fs::create_directories(root);atomicText(root/L".udm-pinned-read-control","owned cached-catalog comparison",false);
 Json result,rows=Json::array();std::string error;
 try{
  for(const auto count:{128,7500,30000}){
   Json expected={{"Schema",1},{"Settings",{{"Connections",8},{"Parallel",4}}},{"Downloads",Json::array()}};
   for(int index=0;index<count;++index)expected["Downloads"].push_back({
    {"Id","fixture-"+std::to_string(index)},{"Url","http://127.0.0.1/fixture/"+std::to_string(index)},
    {"FileName","record-"+std::to_string(index)+".bin"},{"Folder","fixture-downloads"},
    {"Status","Complete"},{"Size",67108864},{"Received",67108864},{"QueueMember",false},
    {"Segments",Json::array()}});
   const auto text=expected.dump();if(text.size()>32*1048576)throw std::runtime_error("Benchmark exceeded the production catalog bound");
   const auto path=root/(L"records-"+std::to_wstring(count)+L".json");atomicText(path,text,false);
   Handle pinned(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
   if(!pinned)throw std::runtime_error("Cannot pin the owned catalog");
   const auto proof=[&](bool usePinned){
    // The old production helper is unchanged in this build. Both paths retain
    // this same pin and perform the same parse, equality and temporary cleanup.
    return Json::parse(usePinned?readPinnedCatalogText(pinned.h,32*1048576):readText(path,32*1048576))==expected;
   };
   if(!proof(false)||!proof(true))throw std::runtime_error("Warmup proof mismatch");
   Json measurements=Json::array();
   for(int trial=0;trial<10;++trial)for(int order=0;order<2;++order){
    const bool usePinned=(trial+order)%2==0;ULONG64 before=0,after=0;
    if(!QueryThreadCycleTime(GetCurrentThread(),&before))throw std::runtime_error("Cannot read thread cycles");
    const auto started=std::chrono::steady_clock::now();const bool accepted=proof(usePinned);
    const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
    if(!QueryThreadCycleTime(GetCurrentThread(),&after)||!accepted)throw std::runtime_error("Measured proof failed");
    measurements.push_back({{"trial",trial+1},{"reader",usePinned?"pinned":"path-reopen"},
      {"seconds",elapsed},{"threadCycles",after-before},{"accepted",true}});
   }
   rows.push_back({{"records",count},{"bytes",text.size()},{"sourceSha256",fileHash(path)},
     {"measurements",measurements}});
   std::cout<<count<<" records; "<<text.size()<<" bytes; 20 exact proofs passed\n"<<std::flush;
  }
 }catch(const std::exception& exception){error=exception.what();}
 result={{"passed",error.empty()},{"error",error},{"cases",rows},
   {"scope","Warm cached catalog read, JSON parse, equality and temporary destruction; common read pin excluded"},
   {"completeApplicationTiming",false},{"pairedIdmComparison",false}};
 atomicText(root/L"result.json",result.dump(2),false);return error.empty()?0:1;
}
