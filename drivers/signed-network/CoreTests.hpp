#pragma once
static std::string httpFixtureReply() {
    auto bytes=payload(192*1024);
    return "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: "+std::to_string(bytes.size())+"\r\n\r\n"+std::string(bytes.begin(),bytes.end());
}
static void streamChild(int family,unsigned port,unsigned mode) {
    Socket client(socket(family,SOCK_STREAM,IPPROTO_TCP));if(client.s==INVALID_SOCKET)throw error("Stream fixture socket",WSAGetLastError());timeout(client.s);
    auto address=loopback(family,port);if(connect(client.s,(sockaddr*)&address,addressSize(family)))throw error("Stream fixture connect",WSAGetLastError());
    auto bytes=payload(192*1024);
    std::string request=mode==2?std::string(bytes.begin(),bytes.end()):"GET /fixture HTTP/1.1\r\nHost: udm-fixture.invalid\r\n\r\n";
    writeAll(client.s,std::vector<char>(request.begin(),request.end()));shutdown(client.s,SD_SEND);
    std::string received;char buffer[16384];
    for(;;){int n=recv(client.s,buffer,sizeof(buffer),0);if(n<0)throw error("Stream fixture read",WSAGetLastError());if(!n)break;
        received.append(buffer,size_t(n));if(received.size()>1024*1024)throw std::runtime_error("Fixture exceeded byte limit.");}
    auto expected=mode==2?request:mode==1?HttpConversation::interceptedReply():httpFixtureReply();
    if(received!=expected)throw std::runtime_error("Stream fixture exact-content mismatch: received "+std::to_string(received.size())+" expected "+std::to_string(expected.size()));
    Sleep(100);
}
static DWORD launchStreamChild(int family,unsigned port,unsigned mode) {
    auto exe=executableDirectory()/L"Udm.Network.exe";
    auto command=L"\""+exe.wstring()+L"\" --stream-child "+std::to_wstring(family)+L" "+std::to_wstring(port)+L" "+std::to_wstring(mode);
    STARTUPINFOW si{sizeof(si)};PROCESS_INFORMATION pi{};
    if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,executableDirectory().c_str(),&si,&pi))throw error("Cannot start stream fixture child");
    CloseHandle(pi.hThread);DWORD wait=WaitForSingleObject(pi.hProcess,20000),code=1;
    if(wait!=WAIT_OBJECT_0){TerminateProcess(pi.hProcess,1);WaitForSingleObject(pi.hProcess,5000);}else GetExitCodeProcess(pi.hProcess,&code);
    DWORD pid=pi.dwProcessId;CloseHandle(pi.hProcess);
    if(wait!=WAIT_OBJECT_0||code)throw std::runtime_error("Stream fixture child failed (code "+std::to_string(code)+").");return pid;
}
static std::function<void()> originResponder(SOCKET listenerSocket,unsigned mode) {
    return [listenerSocket,mode]{
        fd_set ready;FD_ZERO(&ready);FD_SET(listenerSocket,&ready);timeval wait{8,0};
        if(select(0,&ready,nullptr,nullptr,&wait)!=1)throw std::runtime_error("Origin accept timeout.");
        Socket peer(accept(listenerSocket,nullptr,nullptr));if(peer.s==INVALID_SOCKET)throw error("Origin accept",WSAGetLastError());timeout(peer.s);
        if(mode==2){auto data=readAll(peer.s,192*1024);if(data!=payload(data.size()))throw std::runtime_error("Opaque fixture content changed.");writeAll(peer.s,data);}
        else {
            std::string request;char c=0;
            while(request.find("\r\n\r\n")==std::string::npos){int n=recv(peer.s,&c,1,0);if(n!=1||request.size()>1024)throw std::runtime_error("Fixture request missing.");request+=c;}
            auto response=httpFixtureReply();auto boundary=response.find("\r\n\r\n")+4;
            // Deliberately split the header across transport reads.
            for(size_t i=0;i<boundary;i+=7){auto part=response.substr(i,std::min<size_t>(7,boundary-i));writeAll(peer.s,std::vector<char>(part.begin(),part.end()));Sleep(1);}
            if(mode!=1)writeAll(peer.s,std::vector<char>(response.begin()+boundary,response.end()));
        }
        shutdown(peer.s,SD_SEND);
    };
}
static Json gatewayFixture(Library& api,int family,unsigned mode,bool includeChildren) {
    Socket origin(socket(family,SOCK_STREAM,IPPROTO_TCP));unsigned port=bindFixture(origin.s,family);if(listen(origin.s,4))throw error("Origin listen",WSAGetLastError());
    ProcessWatch watcher(api,{GetCurrentProcessId()},true);
    TcpGateway gateway(api,{GetCurrentProcessId()},{port},includeChildren,[mode](const DownloadCandidate& c){return mode==1&&c.request.host=="udm-fixture.invalid";});
    std::exception_ptr serverError,clientError;std::thread server([&]{try{originResponder(origin.s,mode)();}catch(...){serverError=std::current_exception();}});
    DWORD child=0;try{child=launchStreamChild(family,port,mode);}catch(...){clientError=std::current_exception();}
    server.join();Sleep(100);gateway.stop();watcher.stop();
    if(serverError)std::rethrow_exception(serverError);if(clientError)std::rethrow_exception(clientError);
    auto stats=gateway.snapshot();auto events=watcher.take();bool attributed=false,timestamped=false;
    for(const auto& e:events)if(e.process==child){attributed=true;timestamped|=e.timestamp>0&&e.processCreated>0&&e.parentProcess==GetCurrentProcessId();}
    if(includeChildren&&(stats.routed!=1||stats.completed!=1||stats.error||stats.relayFailures||stats.connectFailures))throw std::runtime_error("Gateway did not complete the expected route: "+std::to_string(stats.routed)+"/"+std::to_string(stats.completed));
    if(!includeChildren&&stats.routed)throw std::runtime_error("Excluded child was redirected.");
    if(includeChildren&&mode==1&&stats.intercepted!=1)throw std::runtime_error("Interception callback did not execute.");
    if(!attributed||!timestamped)throw std::runtime_error("Child identity or event timestamp missing.");
    auto observed=gateway.observations();
    if(includeChildren&&mode!=2){
        if(observed.entries.size()!=1||observed.observed!=1)throw std::runtime_error("Gateway capture observation missing.");
        const auto& item=observed.entries.front();
        if(item.process!=child||!item.processCreated||!item.connection||item.candidate.request.target!="/fixture"||item.candidate.request.host!="udm-fixture.invalid")throw std::runtime_error("Gateway capture identity mismatch.");
    }else if(!observed.entries.empty())throw std::runtime_error("Opaque or excluded traffic produced capture records.");
    return {{"Family",family==AF_INET?"IPv4":"IPv6"},{"Mode",mode==0?"HTTP exact forwarding":mode==1?"HTTP interception response":"Opaque exact forwarding"},{"ChildIncluded",includeChildren},
        {"Routed",stats.routed},{"Completed",stats.completed},{"RewrittenPackets",stats.rewritten},{"Candidates",stats.candidates},{"Intercepted",stats.intercepted},{"ChildAttributed",attributed},{"Timestamped",timestamped},{"CaptureRecords",observed.entries.size()}};
}
struct CoreSocketPair {
    Socket client,peer;
    CoreSocketPair() {
        Socket listener(socket(AF_INET,SOCK_STREAM,IPPROTO_TCP));unsigned port=bindFixture(listener.s,AF_INET);
        if(listen(listener.s,1))throw error("Core pair listen",WSAGetLastError());
        client.s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);auto address=loopback(AF_INET,port);
        if(connect(client.s,(sockaddr*)&address,sizeof(sockaddr_in)))throw error("Core pair connect",WSAGetLastError());
        peer.s=accept(listener.s,nullptr,nullptr);if(peer.s==INVALID_SOCKET)throw error("Core pair accept",WSAGetLastError());
        timeout(client.s);timeout(peer.s);
    }
};
static Json coreTests(bool live,const fs::path& runtime) {
    Json report={{"Component","UDM signed network backend 0.3"},{"Live",live},{"Results",Json::array()},{"Fixtures",Json::array()}};
    auto test=[&](const std::string& name,const std::function<bool()>& check){try{report["Results"].push_back({{"Name",name},{"Passed",check()}});}catch(const std::exception& e){report["Results"].push_back({{"Name",name},{"Passed",false},{"Error",e.what()}});}};
    auto request=std::string("GET /movie HTTP/1.1\r\nHost: fixture.invalid\r\n\r\n");
    auto header=std::string("HTTP/1.1 200 OK\r\nContent-Type: video/mp4\r\nContent-Length: 4\r\n\r\n");
    const std::string multipartBody="\r\n--udm-boundary\r\nContent-Type: video/mp4\r\nContent-Range: bytes 0-3/12\r\n\r\ndata\r\n--udm-boundary\r\nContent-Type: video/mp4\r\nContent-Range: bytes 8-11/12\r\n\r\nlast\r\n--udm-boundary--\r\n";
    auto multipartReply=[&](std::string body,bool chunked=false){
        auto h=std::string("HTTP/1.1 206 Partial Content\r\nContent-Type: multipart/byteranges; boundary=\"udm-boundary\"\r\n");
        if(!chunked)return h+"Content-Length: "+std::to_string(body.size())+"\r\n\r\n"+body;
        std::string wire=h+"Transfer-Encoding: chunked\r\n\r\n";
        for(char c:body)wire+="1\r\n"+std::string(1,c)+"\r\n";
        return wire+"0\r\n\r\n";
    };
    const std::string rangeRequest="GET /movie HTTP/1.1\r\nHost: fixture.invalid\r\nRange: bytes=0-3,8-11\r\n\r\n";
    test("Multipart ranges survive one-byte fragmentation and quoted boundary",[&]{
        std::vector<DownloadCandidate> values;HttpConversation h([&](const auto& c){values.push_back(c);return true;});
        h.feed(true,rangeRequest);for(char c:multipartReply(multipartBody))h.feed(false,std::string(1,c));h.finish(false);
        return !h.opaque()&&h.rangeParts==2&&values.size()==2&&values[0].length==4&&values[1].contentRange=="bytes 8-11/12"&&!h.intercepted;
    });
    test("Chunked multipart decoding and following response stay aligned",[&]{
        HttpConversation h;h.feed(true,rangeRequest+request);h.feed(false,multipartReply(multipartBody,true)+header+"data");
        return !h.opaque()&&h.rangeParts==2&&h.responses==2&&h.candidates==3;
    });
    test("Multipart close-delimited response completes at EOF",[&]{
        HttpConversation h;h.feed(true,rangeRequest);
        h.feed(false,"HTTP/1.0 206 Partial Content\r\nContent-Type: multipart/byteranges; boundary=udm-boundary\r\n\r\n"+multipartBody);h.finish(false);
        return !h.opaque()&&h.rangeParts==2;
    });
    test("Multipart final delimiter permits omitted trailing CRLF",[&]{
        HttpConversation h;h.feed(true,rangeRequest);h.feed(false,multipartReply(multipartBody.substr(0,multipartBody.size()-2)));
        return !h.opaque()&&h.rangeParts==2;
    });
    test("Malformed and overflowing byte ranges are rejected",[&]{
        HttpByteRange range;
        for(auto text:{"bytes 3-1/9","bytes 0-9/9","bytes 0-18446744073709551615/*","bytes 0-3/no","bytes 0-3/4x","items 0-3/8","bytes -1-3/8"})if(parseContentRange(text,range))return false;
        return parseContentRange("bytes 3-5/*",range)&&!range.totalKnown&&parseContentRange("bytes 0-3/8",range)&&range.total==8;
    });
    test("Invalid or duplicate multipart boundary becomes opaque",[&]{
        for(auto content:{"multipart/byteranges","multipart/byteranges; boundary=a; boundary=b","multipart/byteranges; boundary=\"bad \"","multipart/byteranges; boundary=\"unterminated"}){
            HttpConversation h;h.feed(true,rangeRequest);h.feed(false,std::string("HTTP/1.1 206 Partial Content\r\nContent-Length: 0\r\nContent-Type: ")+content+"\r\n\r\n");if(!h.opaque())return false;
        }return true;
    });
    test("Wrong part length and missing final boundary disable interpretation",[&]{
        auto bad=multipartBody;auto pos=bad.find("data");bad.replace(pos,4,"toolong");
        for(auto value:{bad,multipartBody.substr(0,multipartBody.size()-12)}){
            HttpConversation h;h.feed(true,rangeRequest);h.feed(false,multipartReply(value));h.finish(false);if(!h.opaque())return false;
        }return true;
    });
    test("Body data that resembles a boundary remains opaque payload",[&]{
        std::string data="--udm-boundary\r\n";
        std::string body="--udm-boundary\r\nContent-Type: video/mp4\r\nContent-Range: bytes 0-"+std::to_string(data.size()-1)+"/"+std::to_string(data.size())+"\r\n\r\n"+data+"\r\n--udm-boundary--\r\n";
        HttpConversation h;h.feed(true,rangeRequest);h.feed(false,multipartReply(body));
        return !h.opaque()&&h.rangeParts==1&&h.candidates==1;
    });
    test("Empty duplicate boundary and part framing fields are rejected",[&]{
        if(!multipartBoundary("multipart/byteranges; boundary=\"\"; boundary=x").empty())return false;
        auto body=multipartBody;auto pos=body.find("Content-Range:");body.insert(pos,"Content-Range: \r\n");
        HttpConversation h;h.feed(true,rangeRequest);h.feed(false,multipartReply(body));return h.opaque();
    });
    test("Whitespace before a boundary cannot terminate a multipart part",[&]{
        auto body=multipartBody;auto pos=body.find("--udm-boundary--");body.insert(pos," ");
        HttpConversation h;h.feed(true,rangeRequest);h.feed(false,multipartReply(body));return h.opaque();
    });
    test("Multipart parts have a strict observation limit",[&]{
        std::string body;for(int i=0;i<129;++i)body+="--udm-boundary\r\nContent-Range: bytes 0-0/1\r\n\r\nx\r\n";
        body+="--udm-boundary--\r\n";HttpConversation h;h.feed(true,rangeRequest);h.feed(false,multipartReply(body));
        return h.opaque()&&h.rangeParts==128;
    });
    test("Single-request interception stops parsing buffered bytes immediately",[&]{
        HttpConversation h([](const auto&){return true;});h.feed(true,request);h.feed(false,header+"data"+header+"more");
        return h.intercepted&&h.candidates==1&&h.responses==1;
    });


    test("Fragmented HTTP request/response preserves candidate metadata",[&]{DownloadCandidate candidate;HttpConversation h([&](const auto& c){candidate=c;return false;});
        for(char c:request)h.feed(true,std::string(1,c));for(char c:header+"data")h.feed(false,std::string(1,c));
        return !h.opaque()&&h.candidates==1&&candidate.request.target=="/movie"&&candidate.lengthKnown&&candidate.length==4;});
    test("Pipelined responses and fixed bodies stay aligned",[&]{HttpConversation h;h.feed(true,request+request);h.feed(false,header+"data"+header+"more");return !h.opaque()&&h.candidates==2&&h.responses==2;});
    test("Chunked bodies/extensions/trailers stay aligned",[&]{HttpConversation h;h.feed(true,request+request);
        auto response=std::string("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nContent-Type: video/mp4\r\n\r\n4;x=y\r\ndata\r\n0\r\nX-End: yes\r\n\r\n")+header+"more";
        for(char c:response)h.feed(false,std::string(1,c));return !h.opaque()&&h.candidates==2;});
    test("HEAD response does not consume the next response body",[&]{HttpConversation h;h.feed(true,"HEAD /movie HTTP/1.1\r\nHost: fixture.invalid\r\n\r\n"+request);h.feed(false,header+header+"data");return !h.opaque()&&h.responses==2;});
    test("Informational response retains request association",[&]{HttpConversation h;h.feed(true,request);h.feed(false,"HTTP/1.1 100 Continue\r\n\r\n"+header+"data");return h.candidates==1&&!h.opaque();});
    test("Range metadata is retained without automatic takeover",[&]{DownloadCandidate c;HttpConversation h([&](const auto& value){c=value;return true;});
        h.feed(true,"GET /movie HTTP/1.1\r\nHost: fixture.invalid\r\nRange: bytes=0-3\r\n\r\n");
        h.feed(false,"HTTP/1.1 206 Partial Content\r\nContent-Type: video/mp4\r\nContent-Length: 4\r\nContent-Range: bytes 0-3/8\r\n\r\ndata");
        return h.candidates==1&&!h.intercepted&&c.contentRange=="bytes 0-3/8"&&c.request.range=="bytes=0-3";});
    test("Only explicit decision enables first-response interception",[&]{HttpConversation h([](const auto&){return true;});h.feed(true,request);h.feed(false,header+"data");return h.intercepted;});
    test("Ordinary HTML does not become a download candidate",[&]{HttpConversation h;h.feed(true,request);h.feed(false,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n");return !h.candidates&&!h.opaque();});
    for(const auto& mime:std::vector<std::string>{"application/vnd.apple.mpegurl","application/x-mpegurl","application/dash+xml","Application/Vnd.Apple.MpegURL; charset=UTF-8"}) {
        test("Adaptive playlist MIME becomes an observation: "+mime,[&,mime]{
            DownloadCandidate c;HttpConversation h([&](const auto& value){c=value;return false;});
            h.feed(true,request);
            auto reply="HTTP/1.1 200 OK\r\nContent-Type: "+mime+"\r\nContent-Length: 4\r\n\r\ndata";
            for(char byte:reply)h.feed(false,std::string(1,byte));h.finish(false);
            return !h.opaque()&&!h.intercepted&&h.candidates==1&&c.contentType==mime&&c.request.target=="/movie"&&c.length==4;
        });
    }
    for(const auto& disposition:std::vector<std::string>{"attachmentish","attachment-file; filename=x"}) {
        test("Attachment prefix is not a disposition token: "+disposition,[&,disposition]{
            HttpConversation h;h.feed(true,request);
            h.feed(false,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Disposition: "+disposition+"\r\nContent-Length: 0\r\n\r\n");
            return !h.opaque()&&h.candidates==0;
        });
    }
    test("Attachment token accepts whitespace before parameters",[&]{
        HttpConversation h;h.feed(true,request);
        h.feed(false,"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Disposition: Attachment ; filename=x\r\nContent-Length: 0\r\n\r\n");
        return !h.opaque()&&h.candidates==1&&!h.intercepted;
    });
    test("Authorization and cookies are not retained in parsed fields",[&]{HttpHead value;HttpDecoder h(false,[&](const auto& head){value=head;});
        h.feed("GET / HTTP/1.1\r\nHost: fixture.invalid\r\nAuthorization: secret\r\nCookie: secret\r\n\r\n");return value.fields.size()==1&&value.get("host")=="fixture.invalid";});
    for(const auto& bad:std::vector<std::string>{
        "Content-Length: 4\r\nTransfer-Encoding: chunked\r\n","Content-Length: 4\r\nContent-Length: 4\r\n",
        "Content-Length: 18446744073709551616\r\n","Transfer-Encoding: gzip, chunked\r\n",
        "Content-Length : 4\r\n"," Folded: bad\r\n","Transfer-Encoding:\r\n"}) {
        test("Ambiguous/invalid HTTP framing disables interpretation: "+bad.substr(0,bad.find('\r')),[&,bad]{HttpConversation h;h.feed(true,request);h.feed(false,"HTTP/1.1 200 OK\r\n"+bad+"\r\n");return h.opaque()&&!h.candidates;});
    }
    test("Invalid chunk terminator disables further interpretation",[&]{HttpConversation h;h.feed(true,request);h.feed(false,"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n1\r\nxZZ");return h.opaque();});
    test("Framing headers in trailers are rejected",[&]{HttpConversation h;h.feed(true,request);h.feed(false,"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\nContent-Length: 4\r\n\r\n");return h.opaque();});
    test("Header memory is bounded",[&]{HttpConversation h;h.feed(true,request);h.feed(false,"HTTP/1.1 200 OK\r\nX: "+std::string(70000,'x'));return h.opaque();});
    test("Request queue is bounded",[&]{HttpConversation h;for(int i=0;i<65;i++)h.feed(true,request);return h.opaque();});
    test("TLS bytes remain opaque",[&]{HttpConversation h;h.feed(true,std::string("\x16\x03\x03\0\x20",5));return h.opaque()&&!h.candidates;});
    test("HTTP/2 preface remains opaque",[&]{HttpConversation h;h.feed(true,"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n");return h.opaque();});
    test("WebSocket upgrade switches to opaque forwarding",[&]{HttpConversation h;h.feed(true,request);h.feed(false,"HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nUpgrade: websocket\r\n\r\n");return h.opaque()&&!h.candidates;});
    test("Truncated fixed-length body is detected",[&]{HttpConversation h;h.feed(true,request);h.feed(false,header+"x");h.finish(false);return h.opaque();});
    test("Close-delimited response is accepted",[&]{HttpConversation h;h.feed(true,request);h.feed(false,"HTTP/1.0 200 OK\r\nContent-Type: video/mp4\r\n\r\ndata");h.finish(false);return !h.opaque()&&h.candidates==1;});
    test("Root process identity includes creation time",[&]{ProcessScope scope({GetCurrentProcessId()},true);return scope.matches(GetCurrentProcessId())&&scope.creation(GetCurrentProcessId())>0&&!scope.matches(4);});
    test("HTTP redirect metadata remains paired with its request",[&]{
        HttpConversation h;h.feed(true,request+request);
        h.feed(false,"HTTP/1.1 302 Found\r\nLocation: /new\r\nContent-Length: 0\r\n\r\n"+header+"data");
        return h.redirectCount==1&&h.redirects.front().location=="/new"&&h.redirects.front().request.target=="/movie"&&h.candidates==1&&!h.opaque();
    });
    test("Seeded varied segmentation preserves body boundaries",[&]{
        uint32_t random=12345;auto next=[&]{random=random*1664525+1013904223;return random;};
        for(unsigned i=0;i<100;i++){
            auto length=next()%4096;std::string body(length,'x');for(auto& c:body)c=char(next()%256);
            HttpConversation h;h.feed(true,request+request);
            auto response="HTTP/1.1 200 OK\r\nContent-Type: video/mp4\r\nContent-Length: "+std::to_string(length)+"\r\n\r\n"+body+header+"data";
            for(size_t offset=0;offset<response.size();){size_t n=std::min<size_t>(1+next()%97,response.size()-offset);h.feed(false,std::string_view(response).substr(offset,n));offset+=n;}
            if(h.opaque()||h.responses!=2||h.candidates!=2)return false;
        }return true;
    });
    test("Idle ordered relay cancels promptly",[&]{
        CoreSocketPair downstream,upstream;std::atomic_bool stop{false};RelayResult result;
        std::thread relay([&]{result=relayStream(downstream.peer.s,upstream.client.s,stop);});
        Sleep(30);auto begin=Clock::now();stop=true;relay.join();return result.cancelled&&Clock::now()-begin<std::chrono::seconds(2);
    });
    test("Idle ordered relay has a finite timeout",[&]{
        CoreSocketPair downstream,upstream;std::atomic_bool stop{false};
        auto result=relayStream(downstream.peer.s,upstream.client.s,stop,{},1);return result.timedOut&&!result.cancelled;
    });
    test("Client reset is distinguished from an upstream failure",[&]{
        CoreSocketPair downstream,upstream;std::atomic_bool stop{false};linger reset{1,0};
        if(setsockopt(downstream.client.s,SOL_SOCKET,SO_LINGER,(char*)&reset,sizeof(reset)))return false;
        closesocket(downstream.client.s);downstream.client.s=INVALID_SOCKET;
        auto result=relayStream(downstream.peer.s,upstream.client.s,stop,{},2);
        return result.errorAtClient&&(result.socketError==WSAECONNRESET||result.socketError==WSAECONNABORTED);
    });
    test("Upstream reset stays an upstream failure",[&]{
        CoreSocketPair downstream,upstream;std::atomic_bool stop{false};linger reset{1,0};
        if(setsockopt(upstream.peer.s,SOL_SOCKET,SO_LINGER,(char*)&reset,sizeof(reset)))return false;
        closesocket(upstream.peer.s);upstream.peer.s=INVALID_SOCKET;
        auto result=relayStream(downstream.peer.s,upstream.client.s,stop,{},2);
        return !result.errorAtClient&&(result.socketError==WSAECONNRESET||result.socketError==WSAECONNABORTED);
    });
    test("CONNECT response ends HTTP interpretation",[&]{HttpConversation h;h.feed(true,"CONNECT fixture.invalid:443 HTTP/1.1\r\nHost: fixture.invalid\r\n\r\n");h.feed(false,"HTTP/1.1 200 Connection Established\r\n\r\n");return h.opaque()&&!h.candidates;});
    test("Relay forwards 100 Continue before waiting for the request body",[&]{
        CoreSocketPair downstream,upstream;std::atomic_bool stop{false};RelayResult result;std::exception_ptr serverError,clientError;
        std::string interim="HTTP/1.1 100 Continue\r\n\r\n",finalResponse="HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        std::thread relay([&]{result=relayStream(downstream.peer.s,upstream.client.s,stop);});
        std::thread server([&]{try{
            std::string head;char c=0;while(head.find("\r\n\r\n")==std::string::npos){if(recv(upstream.peer.s,&c,1,0)!=1)throw std::runtime_error("Missing Expect header.");head+=c;}
            writeAll(upstream.peer.s,std::vector<char>(interim.begin(),interim.end()));
            auto body=readAll(upstream.peer.s,4);if(std::string(body.begin(),body.end())!="data")throw std::runtime_error("Expect body changed.");
            writeAll(upstream.peer.s,std::vector<char>(finalResponse.begin(),finalResponse.end()));shutdown(upstream.peer.s,SD_SEND);
        }catch(...){serverError=std::current_exception();}});
        try {
            std::string head="POST / HTTP/1.1\r\nHost: fixture.invalid\r\nExpect: 100-continue\r\nContent-Length: 4\r\n\r\n";
            writeAll(downstream.client.s,std::vector<char>(head.begin(),head.end()));
            auto reply=readAll(downstream.client.s,interim.size());if(std::string(reply.begin(),reply.end())!=interim)throw std::runtime_error("Interim response changed.");
            writeAll(downstream.client.s,{'d','a','t','a'});shutdown(downstream.client.s,SD_SEND);
            reply=readAll(downstream.client.s,finalResponse.size());if(std::string(reply.begin(),reply.end())!=finalResponse)throw std::runtime_error("Final response changed.");
        }catch(...){clientError=std::current_exception();}
        stop=true;relay.join();server.join();
        if(clientError)std::rethrow_exception(clientError);if(serverError)std::rethrow_exception(serverError);return result.socketError==0;
    });
    test("Failed handoff decision preserves HTTP parsing and disables further decisions",[&]{
        unsigned calls=0;HttpConversation h([&](const auto&)->bool{++calls;throw std::runtime_error("Offer receiver disconnected");});
        h.feed(true,request+request);h.feed(false,header+"data"+header+"more");h.finish(false);
        return !h.opaque()&&!h.intercepted&&h.responses==2&&h.candidates==2&&calls==1&&h.decisionFailures==1;
    });
    test("Non-standard handoff exception preserves original response",[&]{
        HttpConversation h([](const auto&)->bool{throw 1;});try{h.feed(true,request);h.feed(false,header+"data");h.finish(false);}catch(...){return false;}
        return !h.opaque()&&!h.intercepted&&h.candidates==1;
    });
    test("Multipart observer failure does not abort range parsing",[&]{
        unsigned calls=0;HttpConversation h([&](const auto&)->bool{++calls;throw std::runtime_error("Receiver stopped");});
        h.feed(true,rangeRequest);h.feed(false,multipartReply(multipartBody));h.finish(false);
        return !h.opaque()&&!h.intercepted&&h.rangeParts==2&&h.candidates==2&&calls==1&&h.decisionFailures==1;
    });
    test("Relay forwards exact original bytes when the offer receiver fails",[&]{
        CoreSocketPair downstream,upstream;std::atomic_bool stop{false};RelayResult result;
        std::exception_ptr relayError,serverError,clientError;std::string original=header+"data";
        std::thread relay([&]{try{result=relayStream(downstream.peer.s,upstream.client.s,stop,
            [](const auto&)->bool{throw std::runtime_error("Offer receiver disconnected");});}
            catch(...){relayError=std::current_exception();shutdown(downstream.peer.s,SD_BOTH);shutdown(upstream.client.s,SD_BOTH);}});
        std::thread server([&]{try{
            auto received=readAll(upstream.peer.s,request.size());if(std::string(received.begin(),received.end())!=request)throw std::runtime_error("Request changed");
            writeAll(upstream.peer.s,std::vector<char>(original.begin(),original.end()));shutdown(upstream.peer.s,SD_SEND);
        }catch(...){serverError=std::current_exception();}});
        try{
            writeAll(downstream.client.s,std::vector<char>(request.begin(),request.end()));shutdown(downstream.client.s,SD_SEND);
            auto received=readAll(downstream.client.s,original.size());if(std::string(received.begin(),received.end())!=original)throw std::runtime_error("Response changed");
        }catch(...){clientError=std::current_exception();}
        stop=true;relay.join();server.join();
        return !relayError&&!serverError&&!clientError&&!result.socketError&&!result.intercepted&&result.candidates==1&&result.decisionFailures==1;
    });
    test("Relay preserves both pipelined responses despite takeover request",[&]{
        CoreSocketPair downstream,upstream;std::atomic_bool stop{false};RelayResult result;
        std::exception_ptr relayError,serverError,clientError;std::string original=header+"data"+header+"more",pipelined=request+request;
        std::thread relay([&]{try{result=relayStream(downstream.peer.s,upstream.client.s,stop,
            [](const auto&)->bool{return true;});}
            catch(...){relayError=std::current_exception();shutdown(downstream.peer.s,SD_BOTH);shutdown(upstream.client.s,SD_BOTH);}});
        std::thread server([&]{try{
            auto received=readAll(upstream.peer.s,pipelined.size());if(std::string(received.begin(),received.end())!=pipelined)throw std::runtime_error("Request changed");
            writeAll(upstream.peer.s,std::vector<char>(original.begin(),original.end()));shutdown(upstream.peer.s,SD_SEND);
        }catch(...){serverError=std::current_exception();}});
        try{
            writeAll(downstream.client.s,std::vector<char>(pipelined.begin(),pipelined.end()));shutdown(downstream.client.s,SD_SEND);
            auto received=readAll(downstream.client.s,original.size());if(std::string(received.begin(),received.end())!=original)throw std::runtime_error("Response changed");
        }catch(...){clientError=std::current_exception();}
        stop=true;relay.join();server.join();
        return !relayError&&!serverError&&!clientError&&!result.socketError&&!result.intercepted&&result.candidates==2&&result.decisionFailures==0;
    });
    test("Only a first full GET response is eligible for interception",[&]{
        bool eligible=false;HttpConversation h([&](const auto& c){eligible=c.canIntercept;return c.canIntercept;});
        h.feed(true,request);h.feed(false,header+"data");return eligible&&h.intercepted&&h.decisionFailures==0;
    });
    test("HEAD observations explicitly reject response ownership",[&]{
        bool observed=false,eligible=true;HttpConversation h([&](const auto& c){observed=true;eligible=c.canIntercept;return true;});
        h.feed(true,"HEAD /movie HTTP/1.1\r\nHost: fixture.invalid\r\n\r\n");h.feed(false,header);h.finish(false);
        return observed&&!eligible&&!h.intercepted&&!h.opaque();
    });
    test("Range observations explicitly reject response ownership",[&]{
        bool observed=false,eligible=true;HttpConversation h([&](const auto& c){observed=true;eligible=c.canIntercept;return true;});
        h.feed(true,rangeRequest);h.feed(false,"HTTP/1.1 206 Partial Content\r\nContent-Type: video/mp4\r\nContent-Length: 4\r\nContent-Range: bytes 0-3/12\r\n\r\ndata");h.finish(false);
        return observed&&!eligible&&!h.intercepted&&!h.opaque();
    });
    test("A range request answered with 200 is still observation only",[&]{
        bool observed=false,eligible=true;HttpConversation h([&](const auto& c){observed=true;eligible=c.canIntercept;return true;});
        h.feed(true,rangeRequest);h.feed(false,header+"data");h.finish(false);
        return observed&&!eligible&&!h.intercepted&&!h.opaque();
    });
    test("Later pipelined response cannot claim the original stream",[&]{
        unsigned calls=0;bool first=false,later=true;HttpConversation h([&](const auto& c){if(++calls==1){first=c.canIntercept;return false;}later=c.canIntercept;return true;});
        h.feed(true,request+request);h.feed(false,header+"data"+header+"more");h.finish(false);
        return calls==2&&!first&&!later&&!h.intercepted&&!h.opaque();
    });
    test("Queued follow-up request prevents first-response takeover",[&]{
        unsigned calls=0;bool eligible=false;HttpConversation h([&](const auto& c){++calls;eligible|=c.canIntercept;return true;});
        h.feed(true,request+request);h.feed(false,header+"data"+header+"more");h.finish(false);
        return calls==2&&!eligible&&!h.intercepted&&!h.opaque();
    });
    test("Partial follow-up headers prevent first-response takeover",[&]{
        bool eligible=true;HttpConversation h([&](const auto& c){eligible=c.canIntercept;return true;});
        h.feed(true,request+"GET /next HTTP/1.1\r\nHost: fixture.invalid\r\n");h.feed(false,header+"data");
        return !eligible&&!h.intercepted&&!h.opaque();
    });
    test("Unfinished GET request body prevents response takeover",[&]{
        bool eligible=true;HttpConversation h([&](const auto& c){eligible=c.canIntercept;return true;});
        h.feed(true,"GET /movie HTTP/1.1\r\nHost: fixture.invalid\r\nContent-Length: 4\r\n\r\nab");h.feed(false,header+"data");
        return !eligible&&!h.intercepted&&!h.opaque();
    });
    test("Multipart range parts are observations without response ownership",[&]{
        unsigned calls=0;bool eligible=false;HttpConversation h([&](const auto& c){++calls;eligible|=c.canIntercept;return true;});
        h.feed(true,rangeRequest);h.feed(false,multipartReply(multipartBody));h.finish(false);
        return calls==2&&!eligible&&!h.intercepted&&!h.opaque();
    });
    test("Capture observations retain connection and process creation identity",[&]{
        CaptureObservations log;DownloadCandidate c;c.request={"GET","/movie?part=2","fixture.invalid",""};c.contentType="video/mp4";c.canIntercept=true;
        auto id=log.append(123,456,789,c);auto s=log.snapshot();
        return id==1&&s.observed==1&&s.entries.size()==1&&s.entries[0].process==123&&s.entries[0].processCreated==456&&
            s.entries[0].connection==789&&s.entries[0].candidate.request.target==c.request.target&&s.entries[0].candidate.canIntercept;
    });
    test("Capture snapshot is repeatable and independent of caller mutation",[&]{
        CaptureObservations log;DownloadCandidate c;c.request.host="fixture.invalid";log.append(123,1,1,c);
        auto first=log.snapshot();first.entries[0].candidate.request.host="changed";
        auto second=log.snapshot();return second.entries.size()==1&&second.entries[0].id==1&&second.entries[0].candidate.request.host=="fixture.invalid";
    });
    test("Capture count eviction preserves newest records and monotonic identifiers",[&]{
        CaptureObservations log;DownloadCandidate c;for(unsigned i=0;i<70;++i)log.append(123,1,i+1,c);
        auto s=log.snapshot();return s.observed==70&&s.evicted==6&&s.entries.size()==64&&s.entries.front().id==7&&s.entries.back().id==70;
    });
    test("Capture byte budget bounds long request metadata",[&]{
        CaptureObservations log;DownloadCandidate c;c.request.target=std::string(20000,'x');
        for(unsigned i=0;i<5;++i)log.append(123,1,i+1,c);
        auto s=log.snapshot();return s.observed==5&&s.evicted==3&&s.entries.size()==2&&s.retainedBytes==40000&&s.entries.front().id==4;
    });
    test("Oversized capture is rejected without discarding prior evidence",[&]{
        CaptureObservations log;DownloadCandidate c;log.append(123,1,1,c);c.request.target=std::string(CaptureObservations::MaxBytes+1,'x');
        auto id=log.append(123,1,2,c);auto s=log.snapshot();return id==0&&s.observed==1&&s.rejected==1&&s.entries.size()==1&&s.entries[0].id==1;
    });
    test("Capture rejects missing or system process identity",[&]{
        CaptureObservations log;DownloadCandidate c;
        return !log.append(4,1,1,c)&&!log.append(123,0,1,c)&&!log.append(123,1,0,c)&&log.snapshot().rejected==3&&log.snapshot().entries.empty();
    });
    test("Concurrent capture writers preserve unique ordered observations",[&]{
        CaptureObservations log;std::vector<std::thread> writers;
        for(unsigned worker=0;worker<4;++worker)writers.emplace_back([&,worker]{DownloadCandidate c;for(unsigned i=0;i<20;++i)log.append(100+worker,worker+1,i+1,c);});
        for(auto& writer:writers)writer.join();auto s=log.snapshot();
        bool ordered=true;for(size_t i=1;i<s.entries.size();++i)ordered&=s.entries[i].id==s.entries[i-1].id+1;
        return ordered&&s.observed==80&&s.evicted==16&&s.entries.size()==64&&s.entries.front().id==17&&s.entries.back().id==80;
    });
    test("Multipart observations retain one connection with distinct ranges",[&]{
        CaptureObservations log;HttpConversation h([&](const auto& c){log.append(123,456,789,c);return false;});
        h.feed(true,rangeRequest);h.feed(false,multipartReply(multipartBody));h.finish(false);auto s=log.snapshot();
        return !h.opaque()&&!h.intercepted&&s.entries.size()==2&&s.entries[0].connection==789&&s.entries[1].connection==789&&
            s.entries[0].candidate.contentRange!=s.entries[1].candidate.contentRange&&!s.entries[0].candidate.canIntercept;
    });
    test("Capture diagnostic JSON keeps non-UTF8 HTTP field octets",[&]{
        CaptureObservations log;DownloadCandidate c;c.request.target="/movie";c.contentDisposition="attachment; filename="+std::string(1,'\xFF');
        log.append(123,456,789,c);auto wire=captureObservationsJson(log.snapshot()).dump();auto decoded=Json::parse(wire);
        const auto& item=decoded["Items"][0];const auto& encoded=item["ContentDisposition"]["bytes"];
        return item["Target"]=="/movie"&&encoded.size()==c.contentDisposition.size()&&encoded.back()==255&&item["ResponseRetained"]==false;
    });
    test("Capture diagnostic JSON reports unknown lengths and eviction",[&]{
        CaptureObservations log;DownloadCandidate c;for(unsigned i=0;i<66;++i)log.append(123,456,i+1,c);
        auto value=captureObservationsJson(log.snapshot());return value["Observed"]==66&&value["Evicted"]==2&&value["Items"].size()==64&&value["Items"][0]["Length"].is_null();
    });
    if(live) {
        test("Normal code integrity and Test Mode off",[&]{return admin()&&policy()["TestMode"]==false&&policy()["CodeIntegrityEnabled"]==true;});
        if(admin()) {
            Library api(runtime);
            for(int family:{AF_INET,AF_INET6})for(unsigned mode:{0u,1u,2u})
                test(std::string(family==AF_INET?"IPv4 ":"IPv6 ")+(mode==0?"HTTP scoped gateway":mode==1?"HTTP intercepted response":"Opaque duplex gateway"),[&,family,mode]{report["Fixtures"].push_back(gatewayFixture(api,family,mode,true));return true;});
            test("Excluded child traffic is left unchanged",[&]{report["Fixtures"].push_back(gatewayFixture(api,AF_INET,0,false));return true;});
            test("Four concurrent child downloads preserve exact response ownership",[&]{
                Socket origin(socket(AF_INET,SOCK_STREAM,IPPROTO_TCP));unsigned port=bindFixture(origin.s,AF_INET);if(listen(origin.s,8))throw error("Parallel origin listen",WSAGetLastError());
                TcpGateway gateway(api,{GetCurrentProcessId()},{port},true);
                std::exception_ptr originError;std::thread server([&]{try{for(int i=0;i<4;i++)originResponder(origin.s,0)();}catch(...){originError=std::current_exception();}});
                std::array<std::exception_ptr,4> errors{};std::vector<std::thread> clients;
                for(size_t i=0;i<4;i++)clients.emplace_back([&,i]{try{launchStreamChild(AF_INET,port,0);}catch(...){errors[i]=std::current_exception();}});
                for(auto& child:clients)child.join();server.join();Sleep(100);gateway.stop();
                if(originError)std::rethrow_exception(originError);for(const auto& e:errors)if(e)std::rethrow_exception(e);
                auto result=gateway.snapshot();auto observed=gateway.observations();std::set<uint32_t> sources;std::set<uint64_t> connections;for(const auto& item:observed.entries){sources.insert(item.process);connections.insert(item.connection);}return observed.entries.size()==4&&sources.size()==4&&connections.size()==4&&result.routed==4&&result.completed==4&&result.candidates==4&&!result.error&&!result.relayFailures&&!result.connectFailures;
            });
        }
    }
    size_t pass=0;for(const auto& r:report["Results"])if(r["Passed"]==true)++pass;
    report["Passed"]=pass;report["Failed"]=report["Results"].size()-pass;return report;
}
