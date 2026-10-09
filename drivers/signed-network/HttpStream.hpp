#pragma once
#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace udmnet {
inline std::string lowerAscii(std::string s) {for(auto& c:s)if(c>='A'&&c<='Z')c=char(c+('a'-'A'));return s;}
inline std::string trimHttp(std::string s) {
    auto a=s.find_first_not_of(" \t");if(a==std::string::npos)return {};
    return s.substr(a,s.find_last_not_of(" \t")-a+1);
}
inline bool httpToken(std::string_view s) {
    if(s.empty())return false;
    for(unsigned char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==96||std::string_view("!#$%&'*+-.^_|~").find(char(c))!=std::string_view::npos))return false;
    return true;
}
inline bool numberHttp(std::string_view s,uint64_t& value,unsigned base=10) {
    if(s.empty())return false;value=0;
    for(char c:s){unsigned n=c>='0'&&c<='9'?unsigned(c-'0'):c>='a'&&c<='f'?unsigned(c-'a'+10):c>='A'&&c<='F'?unsigned(c-'A'+10):99;
        if(n>=base||value>(UINT64_MAX-n)/base)return false;value=value*base+n;}return true;
}
inline bool downloadMime(const std::string& type) {
    return type.rfind("video/",0)==0||type.rfind("audio/",0)==0||type=="application/octet-stream"||
        type=="application/zip"||type=="application/x-iso9660-image"||
        type=="application/vnd.apple.mpegurl"||type=="application/x-mpegurl"||type=="application/dash+xml";
}
inline bool attachmentDisposition(const std::string& value) {
    return lowerAscii(trimHttp(value.substr(0,value.find(';'))))=="attachment";
}
struct HttpHead {
    bool response=false;unsigned status=0;
    std::string method,target;
    std::map<std::string,std::string> fields;
    std::string get(const char* key) const{auto it=fields.find(key);return it==fields.end()?std::string{}:it->second;}
};
struct HttpRequest {std::string method,target,host,range;};
struct HttpRedirect {HttpRequest request;unsigned status=0;std::string location;};
struct DownloadCandidate {
    HttpRequest request;unsigned status=0;
    std::string contentType,contentDisposition,contentRange,location;
    uint64_t length=0;bool lengthKnown=false;
    // False observations must never claim ownership of the browser response.
    bool canIntercept=false;
};
#include "MultipartRanges.hpp"
// Strict framing, bounded headers, no retained body or credentials. Opaque mode
// disables interpretation but leaves the original transport bytes untouched.
class HttpDecoder {
public:
    enum class State {Header,Fixed,ChunkSize,ChunkData,ChunkEnd,Trailer,UntilClose,Opaque};
private:
    State state=State::Header;bool response=false;uint64_t remaining=0,consumed=0;
    std::string buffer;size_t trailerBytes=0;unsigned trailerCount=0;
    std::function<std::string()> responseMethod;
    std::function<void(const HttpHead&)> onHead;
    std::function<void(std::string_view)> onBody;
    std::function<void()> onComplete;
    std::string reason;
    void fail(const char* why){state=State::Opaque;buffer.clear();reason=why;}
    bool readHead(const std::string& text,HttpHead& h) {
        h.response=response;size_t e=text.find("\r\n");if(e==std::string::npos)return false;
        auto first=text.substr(0,e);
        if(response) {
            if(first.size()<12||(first.compare(0,9,"HTTP/1.1 ")&&first.compare(0,9,"HTTP/1.0 ")))return false;
            if(first.size()>12&&first[12]!=' ')return false;
            uint64_t status=0;if(!numberHttp(std::string_view(first).substr(9,3),status)||status<100||status>599)return false;h.status=unsigned(status);
        }else {
            auto a=first.find(' '),b=first.rfind(' ');
            if(a==std::string::npos||a==b||!httpToken(std::string_view(first).substr(0,a)))return false;
            auto version=first.substr(b+1);if(version!="HTTP/1.0"&&version!="HTTP/1.1")return false;
            h.method=first.substr(0,a);h.target=first.substr(a+1,b-a-1);if(h.target.empty())return false;
            for(unsigned char c:h.target)if(c<=32||c==127)return false;
        }
        unsigned count=0;size_t pos=e+2;
        while(pos<text.size()) {
            e=text.find("\r\n",pos);if(e==std::string::npos)return false;if(e==pos)break;
            auto line=text.substr(pos,e-pos);pos=e+2;if(++count>128)return false;
            auto colon=line.find(':');if(colon==std::string::npos||!httpToken(std::string_view(line).substr(0,colon)))return false;
            auto key=lowerAscii(line.substr(0,colon)),value=trimHttp(line.substr(colon+1));
            for(unsigned char c:value)if((c<32&&c!=9)||c==127)return false;
            static const std::string allowed="|host|content-encoding|content-length|transfer-encoding|content-type|content-range|range|content-disposition|location|connection|upgrade|";
            if(allowed.find("|"+key+"|")==std::string::npos)continue;
            if(h.fields.count(key))return false;
            h.fields.emplace(std::move(key),std::move(value));
        }
        if(!response&&h.get("host").empty())return false;
        if(h.fields.count("content-length")&&h.fields.count("transfer-encoding"))return false;
        return true;
    }
    bool framing(const HttpHead& h) {
        remaining=0;auto te=lowerAscii(h.get("transfer-encoding"));auto cl=h.get("content-length");
        uint64_t length=0;if(h.fields.count("content-length")&&!numberHttp(cl,length))return false;
        if(h.fields.count("transfer-encoding")&&te!="chunked")return false;
        auto method=response&&responseMethod?responseMethod():std::string{};
        if(response&&(h.status==101||(method=="CONNECT"&&h.status>=200&&h.status<300))){state=State::Opaque;return true;}
        if(response&&(method=="HEAD"||h.status<200||h.status==204||h.status==304)){state=State::Header;return true;}
        if(!te.empty()){state=State::ChunkSize;return true;}
        if(h.fields.count("content-length")){remaining=length;state=length?State::Fixed:State::Header;return true;}
        state=response?State::UntilClose:State::Header;return true;
    }
public:
    HttpDecoder(bool isResponse,std::function<void(const HttpHead&)> callback,std::function<std::string()> method={},std::function<void(std::string_view)> body={},std::function<void()> complete={})
        :response(isResponse),responseMethod(std::move(method)),onHead(std::move(callback)),onBody(std::move(body)),onComplete(std::move(complete)){}
    bool opaque()const{return state==State::Opaque;}
    const std::string& failure()const{return reason;}
    State currentState()const{return state;}
    uint64_t bytePosition()const{return consumed;}
    void disable(const char* why){fail(why);}
    void feed(std::string_view input) {
        while(!input.empty()&&!opaque()) {
            if(state==State::UntilClose){if(onBody)onBody(input);consumed+=input.size();return;}
            if(state==State::Fixed||state==State::ChunkData) {
                auto take=std::min<uint64_t>(remaining,input.size());if(onBody)onBody(input.substr(0,size_t(take)));remaining-=take;input.remove_prefix(size_t(take));consumed+=take;
                if(!remaining){if(state==State::Fixed){state=State::Header;if(onComplete)onComplete();}else state=State::ChunkEnd;}continue;
            }
            if(state==State::Header) {
                auto c=static_cast<unsigned char>(input.front());
                if(buffer.empty()&&(c<'A'||c>'Z')){fail("Non-HTTP/1 stream");return;}
                buffer.push_back(input.front());input.remove_prefix(1);++consumed;
                if(buffer.size()>65536){fail("Header limit");return;}
                if(buffer.size()>=4&&buffer.compare(buffer.size()-4,4,"\r\n\r\n")==0) {
                    HttpHead h;auto header=std::move(buffer);buffer.clear();
                    if(!readHead(header,h)||!framing(h)){fail("Invalid or ambiguous HTTP framing");return;}
                    onHead(h);if(state==State::Header&&onComplete)onComplete();
                }
                continue;
            }
            buffer.push_back(input.front());input.remove_prefix(1);++consumed;
            if(buffer.size()>8192){fail("Chunk/trailer line limit");return;}
            if(state==State::ChunkEnd) {
                if((buffer.size()==1&&buffer!="\r")||(buffer.size()==2&&buffer!="\r\n")){fail("Invalid chunk terminator");return;}
                if(buffer.size()==2){buffer.clear();state=State::ChunkSize;}continue;
            }
            if(buffer.size()<2||buffer.compare(buffer.size()-2,2,"\r\n"))continue;
            auto line=buffer.substr(0,buffer.size()-2);buffer.clear();
            if(state==State::ChunkSize) {
                auto semicolon=line.find(';');auto size=line.substr(0,semicolon);
                if(!numberHttp(size,remaining,16)){fail("Invalid chunk size");return;}
                for(unsigned char c:line)if(c<32||c==127){fail("Invalid chunk extension");return;}
                state=remaining?State::ChunkData:State::Trailer;trailerBytes=0;trailerCount=0;
            }else {
                if(line.empty()){state=State::Header;if(onComplete)onComplete();continue;}
                trailerBytes+=line.size()+2;
                auto colon=line.find(':');
                if(++trailerCount>128||trailerBytes>65536||colon==std::string::npos||!httpToken(std::string_view(line).substr(0,colon))){fail("Invalid trailer");return;}
                auto key=lowerAscii(line.substr(0,colon));
                if(key=="content-length"||key=="transfer-encoding"||key=="host"){fail("Framing field in trailer");return;}
                for(unsigned char c:line)if((c<32&&c!=9)||c==127){fail("Invalid trailer value");return;}
            }
        }
    }
    void finish() {
        if(state==State::UntilClose){state=State::Header;if(onComplete)onComplete();return;}
        if(state!=State::Opaque&&(state!=State::Header||!buffer.empty()))fail("Truncated HTTP message");
    }
};
class HttpConversation {
    std::deque<HttpRequest> pending;bool disabled=false,decisionFailed=false;
    std::function<bool(const DownloadCandidate&)> decide;
    std::unique_ptr<MultipartRanges> multipart;
    bool offer(const DownloadCandidate& candidate) noexcept {
        if(!decide||decisionFailed)return false;
        try{return decide(candidate);}
        catch(...){decisionFailed=true;++decisionFailures;return false;}
    }
    void body(std::string_view bytes){if(multipart){multipart->feed(bytes);if(multipart->failed())disabled=true;}}
    void completeBody(){if(multipart){multipart->finish();if(multipart->failed())disabled=true;multipart.reset();}}
    void request(const HttpHead& h) {
        if(pending.size()>=64){disabled=true;return;}
        pending.push_back({h.method,h.target,h.get("host"),h.get("range")});
    }
    void response(const HttpHead& h) {
        if(pending.empty()){disabled=true;return;}
        if(h.status<200&&h.status!=101){interimEnd=responsesDecoder.bytePosition();return;}
        auto req=pending.front();pending.pop_front();++responses;
        if(multipart){multipart->finish();if(multipart->failed())disabled=true;multipart.reset();}
        if(h.status>=300&&h.status<400&&!h.get("location").empty()) {
            ++redirectCount;if(redirects.size()==32)redirects.pop_front();
            redirects.push_back({req,h.status,h.get("location")});
        }
        auto type=lowerAscii(h.get("content-type"));auto semi=type.find(';');type=trimHttp(type.substr(0,semi));
        if(type=="multipart/byteranges"&&h.status==206&&req.method=="GET"){
            auto boundary=multipartBoundary(h.get("content-type"));
            if(boundary.empty()||!h.get("content-range").empty()||(!h.get("content-encoding").empty()&&lowerAscii(h.get("content-encoding"))!="identity")){disabled=true;return;}
            multipart=std::make_unique<MultipartRanges>(boundary,[this,req](const HttpRangePart& part){
                ++rangeParts;
                auto mime=lowerAscii(trimHttp(part.contentType.substr(0,part.contentType.find(';'))));
                if(downloadMime(mime)){
                    DownloadCandidate c{req,206,part.contentType,"",part.contentRange,""};
                    c.length=part.range.last-part.range.first+1;c.lengthKnown=true;++candidates;offer(c);
                }
            });
        }
        bool file=downloadMime(type)||attachmentDisposition(h.get("content-disposition"));
        if(file&&(h.status==200||h.status==206)&&(req.method=="GET"||req.method=="HEAD")) {
            DownloadCandidate c{req,h.status,h.get("content-type"),h.get("content-disposition"),h.get("content-range"),h.get("location")};
            c.lengthKnown=h.fields.count("content-length")!=0;if(c.lengthKnown)numberHttp(h.get("content-length"),c.length);
            ++candidates;
            c.canIntercept=responses==1&&req.method=="GET"&&req.range.empty()&&h.status==200;
            if(offer(c)&&c.canIntercept){intercepted=true;responsesDecoder.disable("Download intercepted");}
        }
        if(h.status==101||(req.method=="CONNECT"&&h.status>=200&&h.status<300))disabled=true;
    }
    HttpDecoder requests,responsesDecoder;
public:
    size_t candidates=0,responses=0,redirectCount=0,rangeParts=0,decisionFailures=0;uint64_t interimEnd=0;bool intercepted=false;
    std::deque<HttpRedirect> redirects;
    explicit HttpConversation(std::function<bool(const DownloadCandidate&)> callback={})
        :decide(std::move(callback)),
         requests(false,[this](const auto& h){request(h);}),
         responsesDecoder(true,[this](const auto& h){response(h);},[this]{return pending.empty()?std::string{}:pending.front().method;},[this](auto bytes){body(bytes);},[this]{completeBody();}){}
    void feed(bool fromClient,std::string_view bytes) {
        if(disabled||intercepted)return;
        if(fromClient)requests.feed(bytes);else responsesDecoder.feed(bytes);
        if(requests.opaque()||responsesDecoder.opaque()||disabled){disabled=true;requests.disable("Opaque conversation");responsesDecoder.disable("Opaque conversation");}
    }
    void finish(bool fromClient){if(fromClient)requests.finish();else{responsesDecoder.finish();if(multipart)completeBody();}}
    bool opaque()const{return disabled||requests.opaque()||responsesDecoder.opaque();}
    static std::string interceptedReply(){return "HTTP/1.1 204 No Content\r\nConnection: close\r\nCache-Control: no-store\r\nContent-Length: 0\r\n\r\n";}
};
}
