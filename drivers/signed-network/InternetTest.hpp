#pragma once
struct HttpHandle {
    HINTERNET h=nullptr;
    explicit HttpHandle(HINTERNET handle):h(handle){if(!h)throw error("HTTPS fixture handle");}
    ~HttpHandle(){WinHttpCloseHandle(h);}
};
static void httpsFixtureChild() {
    HttpHandle session(WinHttpOpen(L"UDM-Backend-Acceptance/0.2",WINHTTP_ACCESS_TYPE_NO_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));
    if(!WinHttpSetTimeouts(session.h,5000,5000,5000,5000))throw error("HTTPS fixture timeouts");
    HttpHandle connection(WinHttpConnect(session.h,L"example.com",INTERNET_DEFAULT_HTTPS_PORT,0));
    HttpHandle request(WinHttpOpenRequest(connection.h,L"GET",L"/",nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));
    if(!WinHttpSendRequest(request.h,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(request.h,nullptr))throw error("HTTPS fixture request");
    DWORD status=0,size=sizeof(status);
    if(!WinHttpQueryHeaders(request.h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX)||status!=200)throw std::runtime_error("HTTPS fixture HTTP status was not 200.");
    uint64_t total=0;char bytes[8192];DWORD received=0;
    for(;;){if(!WinHttpReadData(request.h,bytes,sizeof(bytes),&received))throw error("HTTPS fixture response");if(!received)break;total+=received;if(total>1024*1024)throw std::runtime_error("HTTPS fixture size limit.");}
    if(!total)throw std::runtime_error("HTTPS fixture response was empty.");
}
static Json internetTest(const fs::path& runtime) {
    Json report={{"Component","UDM signed network backend 0.2"},{"Source","https://example.com/"},{"TlsCertificateValidation","Windows defaults, no overrides"},{"HttpsDecryption",false},{"Passed",0},{"Failed",1}};
    try {
        if(!admin())throw std::runtime_error("Internet gateway fixture needs administrator privileges.");
        Library api(runtime);TcpGateway gateway(api,{GetCurrentProcessId()},{443},true);
        auto exe=executableDirectory()/L"Udm.Network.exe",directory=executableDirectory();
        auto command=L"\""+exe.wstring()+L"\" --https-child";
        STARTUPINFOW si{sizeof(si)};PROCESS_INFORMATION pi{};
        if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,directory.c_str(),&si,&pi))throw error("Cannot start HTTPS fixture child");
        CloseHandle(pi.hThread);DWORD wait=WaitForSingleObject(pi.hProcess,30000),code=1;
        if(wait!=WAIT_OBJECT_0){TerminateProcess(pi.hProcess,1);WaitForSingleObject(pi.hProcess,5000);}else GetExitCodeProcess(pi.hProcess,&code);
        CloseHandle(pi.hProcess);Sleep(100);gateway.stop();report["ChildExitCode"]=code;report["ChildCompleted"]=wait==WAIT_OBJECT_0;
        auto result=gateway.snapshot();report["Gateway"]={{"Routed",result.routed},{"Completed",result.completed},{"ConnectFailures",result.connectFailures},{"RelayFailures",result.relayFailures},{"ClientResets",result.clientResets},{"LastSocketError",result.lastSocketError},{"RewrittenPackets",result.rewritten},{"ServerStreamBytes",result.serverBytes},{"Candidates",result.candidates}};
        if(wait!=WAIT_OBJECT_0||code||!result.routed||!result.completed||result.error||result.connectFailures||result.relayFailures||!result.serverBytes||result.candidates)throw std::runtime_error("External HTTPS gateway fixture did not pass.");
        report["Passed"]=1;report["Failed"]=0;
    }catch(const std::exception& e){report["Error"]=e.what();}
    return report;
}
