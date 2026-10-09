#pragma once
#include "BrokerProtocol.hpp"
#include "CaptureSession.hpp"
static int desktopBroker(const fs::path& runtime,DWORD parent,const std::wstring& nonce) {
    if(parent<=4||parent==GetCurrentProcessId()||!admin())throw std::runtime_error("The desktop network broker requires an elevated helper and a live parent.");
    udmbroker::Handle owner(OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,parent));
    if(!owner.h||WaitForSingleObject(owner.h,0)!=WAIT_TIMEOUT)throw std::runtime_error("The desktop network broker parent is unavailable.");
    auto name=udmbroker::pipeName(nonce);
    udmbroker::Handle pipe(CreateFileW(name.c_str(),FILE_READ_DATA|FILE_WRITE_DATA|FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|SYNCHRONIZE,
        0,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED|SECURITY_SQOS_PRESENT|SECURITY_IDENTIFICATION,nullptr));
    if(pipe.h==INVALID_HANDLE_VALUE)throw udmbroker::failure("Cannot connect to the desktop network broker");
    ULONG server=0;if(!GetNamedPipeServerProcessId(pipe.h,&server)||server!=parent)throw std::runtime_error("Network broker server identity mismatch.");
    Library api(runtime);
    std::string nonceText;for(auto c:nonce)nonceText.push_back(static_cast<char>(c));
    udmbroker::send(pipe.h,{{"Version",udmbroker::Version},{"Nonce",nonceText},{"ProcessId",GetCurrentProcessId()}},3000,owner.h);
    std::unique_ptr<ProcessWatch> watch;udmbroker::CaptureSession<TcpGateway> capture;
    std::vector<DWORD> roots;Json rows=Json::object();uint64_t generation=0,evicted=0;
    while(WaitForSingleObject(owner.h,0)==WAIT_TIMEOUT) {
        auto command=udmbroker::receive(pipe.h,600000,owner.h);
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
                if(requested!=roots){std::unique_ptr<ProcessWatch> next;if(!requested.empty())next=std::make_unique<ProcessWatch>(api,requested,true);capture.stop();watch=std::move(next);roots=requested;rows=Json::object();evicted=0;++generation;}
            } else if(action=="Capture") {
                auto ports=udmbroker::capturePorts(command.at("Ports"));
                capture.configure(ports,!roots.empty(),[&](const auto& selected){return std::make_unique<TcpGateway>(api,roots,selected,true);});
                response["CaptureEnabled"]=capture.get()!=nullptr;response["CaptureGeneration"]=capture.currentGeneration();
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
                if(capture.get())captured["Gateway"]=gatewayJson(capture.get()->snapshot());
                response["Snapshot"]["Capture"]=std::move(captured);
            } else if(action=="Stop"){capture.stop();watch.reset();finish=true;}
            else throw std::runtime_error("Unsupported network broker command.");
        }catch(const std::exception& e){response["Ok"]=false;response["Error"]=e.what();}
        udmbroker::send(pipe.h,response,3000,owner.h);if(finish)break;
    }
    return 0;
}

