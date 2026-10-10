#include "Core.hpp"
#include "../drivers/signed-network/DurableResponseSink.hpp"
#include <iostream>
using namespace udm;
int main(){int passed=0,failed=0;auto root=fs::temp_directory_path()/wide("udm-response-catalog-"+guid());
 auto check=[&](bool ok,const char* message){(ok?++passed:++failed);std::cout<<(ok?"PASS ":"FAIL ")<<message<<std::endl;};
 auto rejects=[&](auto fn,const char* message){try{fn();check(false,message);}catch(const std::exception&){check(true,message);}};
 Json offer={{"Id",uint64_t(1)},{"Method","GET"},{"Status",200},{"WasInterceptable",true},{"Host","example.invalid"},{"Target","/original.bin"},{"Length",uint64_t(4)}};
 Json capture={{"Session",std::string(32,'a')},{"Generation",uint64_t(1)},{"ResponseAdmissionEnabled",true},{"PendingResponses",Json::array({offer})}};
 try{
  for(unsigned mode=0;mode<6;++mode){auto data=root/std::to_wstring(mode);std::string id;
   {Manager m(data);auto settings=m.state["Settings"];settings["DownloadFolder"]=utf8((data/L"files").wstring());m.setSettings(settings);
    {auto reservation=m.reserveOriginalResponse(capture,offer);id=reservation.job->id();auto saved=Json::parse(readText(data/L"state.json"));
     check(saved["Downloads"].size()==1&&saved["Downloads"][0].contains("OriginalResponse")&&!yes(saved["Downloads"][0],"QueueMember"),"Job identity is durable before any response is accepted");
     m.recoverOriginalResponses();check(str(reservation.job->data["OriginalResponse"],"State")=="Reserved","Recovery does not disturb open response file handles");
     rejects([&]{m.reserveOriginalResponse(capture,offer);},"Repeated offer cannot create a second reservation");
     if(mode!=2){auto body=reservation.body.h,receipt=reservation.receipt.h;reservation.body.h=reservation.receipt.h=INVALID_HANDLE_VALUE;
      udmnet::DurableResponseSink sink(body,receipt,true,4);sink.write(mode==1?"ab":"abcd");if(mode!=1)sink.complete();}
    }
    if(mode==3||mode==5)fs::resize_file(data/L"responses"/wide(id)/L"receipt",mode==3?80:0);
    if(mode==4)fs::remove(data/L"responses"/wide(id)/L"receipt");
   }
   {Manager m(data);auto job=m.jobs.at(0);auto state=str(job->data["OriginalResponse"],"State");
    check(state==(mode==0?"Ready":mode==2?"Unaccepted":mode>=4?"Recovery required":"Interrupted"),"Restart distinguishes received, interrupted and unaccepted response ownership");
    check(str(job->data,"Status")=="Paused"&&!yes(job->data,"QueueMember"),"Recovered original response cannot start through the queue");
    check(num(job->data,"Received")== (mode==1?2:mode==2?0:4),"Recovery preserves the actual received byte count");
    rejects([&]{m.resume(job);},"Resume cannot replay the original browser request");
    check(readText(data/L"responses"/wide(id)/L"body")== (mode==1?"ab":mode==2?"":"abcd"),"Recovery preserves every received body byte");
   }
  }
  {Manager m(root/L"rollback");m.save();rejects([&]{m.reserveOriginalResponse(capture,offer,[](const char* phase){if(std::string(phase)=="before-commit")throw std::runtime_error("fixture save failure");});},"Failed catalog reservation is reported");check(m.jobs.empty()&&Json::parse(readText(m.root/L"state.json"))["Downloads"].empty(),"Failed reservation leaves no in-memory or persisted job");}
  {auto data=root/L"lost-ack";{Manager m(data);rejects([&]{m.reserveOriginalResponse(capture,offer,[](const char* phase){if(std::string(phase)=="committed")throw std::runtime_error("fixture lost reply");});},"Lost reservation acknowledgement is simulated after commit");}Manager reopened(data);check(reopened.jobs.size()==1&&str(reopened.jobs[0]->data["OriginalResponse"],"State")=="Unaccepted","Lost acknowledgement retains the durable reservation for recovery");}
 }catch(const std::exception& e){++failed;std::cout<<"FAIL "<<e.what()<<std::endl;}
 std::cout<<passed<<" passed, "<<failed<<" failed; evidence "<<utf8(root.wstring())<<std::endl;return failed?1:0;
}
