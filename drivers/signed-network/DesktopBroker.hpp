#pragma once
#include "BrokerProtocol.hpp"
#include "ResponseCaptureSession.hpp"
static int desktopBroker(const fs::path& runtime,DWORD parent,const std::wstring& nonce) {
    if(parent<=4||parent==GetCurrentProcessId()||!admin())throw std::runtime_error("The desktop network broker requires an elevated helper and a live parent.");
    udmbroker::Handle owner(OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_DUP_HANDLE,FALSE,parent));
    if(!owner.h||WaitForSingleObject(owner.h,0)!=WAIT_TIMEOUT)throw std::runtime_error("The desktop network broker parent is unavailable.");
    auto name=udmbroker::pipeName(nonce);
    udmbroker::Handle pipe(CreateFileW(name.c_str(),FILE_READ_DATA|FILE_WRITE_DATA|FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|SYNCHRONIZE,
        0,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED|SECURITY_SQOS_PRESENT|SECURITY_IDENTIFICATION,nullptr));
    if(pipe.h==INVALID_HANDLE_VALUE)throw udmbroker::failure("Cannot connect to the desktop network broker");
    ULONG server=0;if(!GetNamedPipeServerProcessId(pipe.h,&server)||server!=parent)throw std::runtime_error("Network broker server identity mismatch.");
    Library api(runtime);
    std::string nonceText;for(auto c:nonce)nonceText.push_back(static_cast<char>(c));
    udmbroker::send(pipe.h,{{"Version",udmbroker::Version},{"Nonce",nonceText},{"ProcessId",GetCurrentProcessId()}},3000,owner.h);
    std::unique_ptr<ProcessWatch> watch;ResponseAdmissions admissions;udmbroker::CaptureSession<TcpGateway> capture;bool admissionEnabled=false;
    struct CancelAdmissions{ResponseAdmissions& value;~CancelAdmissions(){value.reset(false);}} cancelAdmissions{admissions};
    std::vector<DWORD> roots;Json rows=Json::object();uint64_t generation=0,evicted=0;
    while(WaitForSingleObject(owner.h,0)==WAIT_TIMEOUT) {
        auto command=udmbroker::receive(pipe.h,3000,owner.h,INFINITE);
        Json response={{"Ok",true},{"Version",udmbroker::Version}};
        bool finish=false;
        try {
            if(command.value("Version",0u)!=udmbroker::Version)throw std::runtime_error("Unsupported network broker version.");
            auto action=command.value("Action",std::string());
            if(action=="Watch") {
                if(!command.contains("ProcessIds")||!command["ProcessIds"].is_array()||command["ProcessIds"].size()>32)throw std::runtime_error("Select up to 32 live processes.");
                std::vector<DWORD> requested;
                for(auto& p:command["ProcessIds"]){if(!p.is_number_unsigned()||p.get<uint64_t>()<=4||p.get<uint64_t>()>MAXDWORD)throw std::runtime_error("Invalid process identifier.");requested.push_back(p.get<DWORD>());}
                std::sort(requested.begin(),requested.end());requested.erase(std::unique(requested.begin(),requested.end()),requested.end());
                if(requested!=roots){
                    std::unique_ptr<ProcessWatch> next;
                    if(!requested.empty())next=std::make_unique<ProcessWatch>(api,requested,true);
                    if(requested.empty()){admissions.reset(false);admissionEnabled=false;capture.stop();}
                    else if(capture.get())capture.get()->replaceRoots(requested);
                    watch=std::move(next);roots=requested;rows=Json::object();evicted=0;++generation;
                }
                response["CaptureGeneration"]=capture.currentGeneration();
                response["ResponseAdmissionEnabled"]=admissionEnabled;
            } else if(action=="Capture") {
                auto ports=udmbroker::capturePorts(command.at("Ports"));
                bool requestedAdmission=command.value("ResponseAdmission",false)&&!ports.empty();
                udmbroker::configureResponseCapture(capture,admissions,admissionEnabled,ports,!roots.empty(),requestedAdmission,
                    [&](const auto& selected,bool retainHeaders){return std::make_unique<TcpGateway>(api,roots,selected,true,std::function<bool(const DownloadCandidate&)>{},[&](DWORD pid,uint64_t created,uint64_t connection,const DownloadCandidate& candidate){return admissions.offer(pid,created,connection,candidate);},retainHeaders);});
                response["ResponseAdmissionEnabled"]=admissionEnabled;response["CaptureEnabled"]=capture.get()!=nullptr;response["CaptureGeneration"]=capture.currentGeneration();
            } else if(action=="AdmitResponse"||action=="RejectResponse"||action=="CancelResponse"||action=="GrantResponse") {
                if(command.value("Session",std::string())!=nonceText||command.at("Generation").get<uint64_t>()!=capture.currentGeneration())throw std::runtime_error("Expired response admission session.");
                auto id=command.at("Id").get<uint64_t>();
                if(action=="GrantResponse"){response["Granted"]=admissions.grant(id,udmbroker::responseAllowance(command.at("TotalBytes")));}
                else if(action=="CancelResponse")response["Cancelled"]=admissions.cancel(id);
                else if(action=="RejectResponse")response["Rejected"]=admissions.reject(id);
                else response["Accepted"]=admissions.admit(id,[&](const DownloadCandidate& candidate){
                    auto duplicate=[&](const char* key){
                        const auto& value=command.at(key);if(!value.is_number_unsigned())throw std::runtime_error("Invalid response file handle.");
                        auto number=value.get<uint64_t>();if(!number||number>=UINTPTR_MAX)throw std::runtime_error("Invalid response file handle.");
                        HANDLE copy=INVALID_HANDLE_VALUE;if(!DuplicateHandle(owner.h,reinterpret_cast<HANDLE>(static_cast<uintptr_t>(number)),GetCurrentProcess(),&copy,0,FALSE,DUPLICATE_SAME_ACCESS))throw udmbroker::failure("Cannot receive reserved response file");return copy;
                    };
                    udmbroker::Handle body(duplicate("BodyHandle")),receipt(duplicate("ReceiptHandle"));
                    auto bodyHandle=body.h,receiptHandle=receipt.h;body.h=receipt.h=INVALID_HANDLE_VALUE;
                    return std::make_unique<DurableResponseSink>(bodyHandle,receiptHandle,candidate.lengthKnown,candidate.length);
                },command.value("FlowControlled",false));
            } else if(action=="Snapshot") {
                if(watch) {
                    if(watch->lastError())throw error("Signed process monitor stopped",watch->lastError());
                    for(auto& event:watch->take()) {
                        if(event.protocol!=IPPROTO_TCP&&event.protocol!=IPPROTO_UDP)continue;
                        auto e=eventJson(event);
                        // FLOW is authoritative when available; socket events fill bind/connect/close gaps.
                        std::string key=std::to_string(event.processCreated)+":"+std::to_string(event.process)+":"+std::to_string(event.endpoint);
                        if(!rows.contains(key)&&rows.size()>=512){rows.erase(rows.begin());++evicted;}
                        bool closed=event.type==WINDIVERT_EVENT_FLOW_DELETED||event.type==WINDIVERT_EVENT_SOCKET_CLOSE;
                        if(rows.contains(key)&&rows[key].value("TimestampQpc",INT64(0))>event.timestamp)continue;
                        rows[key]={{"ProcessId",event.process},{"ParentProcessId",event.parentProcess},{"ProcessCreated100ns",event.processCreated},
                            {"Local",e["Local"]},
                            {"Remote",e["Remote"]},
                            {"Transport",event.protocol==IPPROTO_TCP?"TCP":"UDP"},{"State",closed?"Closed":"Observed"},
                            {"Received",nullptr},{"Sent",nullptr},{"TimestampQpc",event.timestamp}};
                    }
                }
                Json flows=Json::array();for(auto& row:rows.items())flows.push_back(row.value());
                response["Snapshot"]={{"Backend","WinDivert 2.2.2-A"},{"Version","0.3"},{"Generation",generation},{"Watched",roots.size()},
                    {"Dropped",watch?watch->lost():0},{"Evicted",evicted},{"ScopeLookupFailures",watch?watch->scopeLookupFailures():0},
                    {"BrokerProcessId",GetCurrentProcessId()},{"Connections",rows.size()},{"Received",nullptr},{"Sent",nullptr},{"Flows",flows},{"IncludeChildren",true}};
                Json captured={{"Session",nonceText},{"Generation",capture.currentGeneration()},{"Enabled",capture.get()!=nullptr},
                    {"Ports",capture.destinationPorts()},{"AutomaticDownloadTakeover",false},
                    {"Observations",captureObservationsJson(capture.get()?capture.get()->observations():CaptureObservationSnapshot{})}};
                CaptureObservationSnapshot offers;offers.entries=admissions.pending();
                captured["Responses"]=Json::array();for(const auto& progress:admissions.progress()){
                    const char* state=progress.phase==ResponsePhase::Receiving?"Receiving":progress.phase==ResponsePhase::Finalizing?"Finalizing":progress.phase==ResponsePhase::Complete?"Complete":progress.phase==ResponsePhase::Cancelled?"Cancelled":"Failed";
                    captured["Responses"].push_back({{"Id",progress.id},{"Received",progress.received},{"State",state},{"TransferSeconds",progress.seconds},{"FlowControlled",progress.flowControlled},{"GrantedBytes",progress.granted}});
                }
                captured["ResponseCreditsSupported"]=true;captured["ResponseAdmissionEnabled"]=admissionEnabled;captured["PendingResponses"]=captureObservationsJson(offers,true)["Items"];
                if(capture.get())captured["Gateway"]=gatewayJson(capture.get()->snapshot());
                response["Snapshot"]["Capture"]=std::move(captured);
            } else if(action=="Stop"){admissions.reset(false);capture.stop();watch.reset();finish=true;}
            else throw std::runtime_error("Unsupported network broker command.");
        }catch(const std::exception& e){response["Ok"]=false;response["Error"]=e.what();}
        udmbroker::send(pipe.h,response,3000,owner.h);if(finish)break;
    }
    return 0;
}

