// Included inside udmnet after the HTTP metadata types.
struct HttpByteRange {uint64_t first=0,last=0,total=0;bool totalKnown=false;};
inline bool parseContentRange(const std::string& text,HttpByteRange& r) {
    if(text.rfind("bytes ",0))return false;
    auto dash=text.find('-',6),slash=text.find('/',6);
    if(dash==std::string::npos||slash==std::string::npos||dash>=slash)return false;
    if(!numberHttp(std::string_view(text).substr(6,dash-6),r.first)||!numberHttp(std::string_view(text).substr(dash+1,slash-dash-1),r.last)||r.last<r.first||r.last-r.first==UINT64_MAX)return false;
    auto total=std::string_view(text).substr(slash+1);r.totalKnown=total!="*";
    return !r.totalKnown||(numberHttp(total,r.total)&&r.total>r.last);
}
inline std::string multipartBoundary(const std::string& contentType) {
    auto semicolon=contentType.find(';');if(lowerAscii(trimHttp(contentType.substr(0,semicolon)))!="multipart/byteranges"||semicolon==std::string::npos)return {};
    std::string result;bool boundarySeen=false;size_t pos=semicolon+1;
    while(pos<contentType.size()){
        while(pos<contentType.size()&&(contentType[pos]==' '||contentType[pos]=='\t'))++pos;
        size_t equal=contentType.find('=',pos);if(equal==std::string::npos)return {};
        auto key=lowerAscii(trimHttp(contentType.substr(pos,equal-pos)));if(!httpToken(key))return {};pos=equal+1;
        while(pos<contentType.size()&&(contentType[pos]==' '||contentType[pos]=='\t'))++pos;
        std::string value;
        if(pos<contentType.size()&&contentType[pos]=='"'){
            ++pos;bool closed=false;while(pos<contentType.size()){char c=contentType[pos++];if(c=='"'){closed=true;break;}if(c=='\\'){if(pos==contentType.size())return {};c=contentType[pos++];}value+=c;}if(!closed)return {};
            while(pos<contentType.size()&&(contentType[pos]==' '||contentType[pos]=='\t'))++pos;
            if(pos<contentType.size()&&contentType[pos]!=';')return {};
        }else{auto end=contentType.find(';',pos);value=trimHttp(contentType.substr(pos,end==std::string::npos?end:end-pos));pos=end==std::string::npos?contentType.size():end;if(!httpToken(value))return {};}
        if(key=="boundary"){if(boundarySeen)return {};boundarySeen=true;result=value;}
        if(pos<contentType.size())++pos;
    }
    if(result.empty()||result.size()>70||result.back()==' ')return {};
    for(unsigned char c:result)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||std::string_view("'()+_,-./:=? ").find(char(c))!=std::string_view::npos))return {};
    return result;
}
struct HttpRangePart {HttpByteRange range;std::string contentType,contentRange;};
class MultipartRanges {
    enum class State {First,Headers,Data,DataEnd,Boundary,Done,Invalid};
    State state=State::First;std::string boundary,line;HttpRangePart part;
    uint64_t remaining=0;size_t preamble=0,headerBytes=0;unsigned fields=0,parts=0;
    bool rangeSeen=false,typeSeen=false;
    static std::string padding(std::string value){while(!value.empty()&&(value.back()==' '||value.back()=='\t'))value.pop_back();return value;}
    std::function<void(const HttpRangePart&)> callback;
    void fail(){state=State::Invalid;line.clear();}
    void delimiter(const std::string& value,bool first) {
        bool final=value=="--"+boundary+"--";
        if(value!="--"+boundary&&!final){if(first){preamble+=value.size()+2;if(preamble>8192)fail();return;}fail();return;}
        if(!first){if(++parts>128){fail();return;}callback(part);}
        if(final){if(first)fail();else state=State::Done;}
        else {state=State::Headers;part={};headerBytes=0;fields=0;rangeSeen=typeSeen=false;}
    }
public:
    MultipartRanges(std::string value,std::function<void(const HttpRangePart&)> observe):boundary(std::move(value)),callback(std::move(observe)){if(boundary.empty())fail();}
    bool failed()const{return state==State::Invalid;}
    bool complete()const{return state==State::Done;}
    size_t count()const{return parts;}
    void feed(std::string_view bytes) {
        while(!bytes.empty()&&state!=State::Invalid&&state!=State::Done){
            if(state==State::Data){size_t n=static_cast<size_t>(std::min<uint64_t>(remaining,bytes.size()));bytes.remove_prefix(n);remaining-=n;if(!remaining)state=State::DataEnd;continue;}
            line+=bytes.front();bytes.remove_prefix(1);if(line.size()>8192){fail();return;}
            if(line.size()<2||line.compare(line.size()-2,2,"\r\n"))continue;
            auto value=line.substr(0,line.size()-2);line.clear();
            if(state==State::DataEnd){if(!value.empty()){fail();return;}state=State::Boundary;continue;}
            if(state==State::First||state==State::Boundary){delimiter(padding(value),state==State::First);continue;}
            headerBytes+=value.size()+2;if(headerBytes>16384){fail();return;}
            if(value.empty()){
                if(!parseContentRange(part.contentRange,part.range)){fail();return;}
                remaining=part.range.last-part.range.first+1;state=State::Data;continue;
            }
            auto colon=value.find(':');if(++fields>64||colon==std::string::npos||!httpToken(std::string_view(value).substr(0,colon))){fail();return;}
            auto key=lowerAscii(value.substr(0,colon)),field=trimHttp(value.substr(colon+1));
            for(unsigned char c:field)if((c<32&&c!=9)||c==127){fail();return;}
            if(key=="content-range"){if(rangeSeen){fail();return;}rangeSeen=true;part.contentRange=field;}
            if(key=="content-type"){if(typeSeen){fail();return;}typeSeen=true;part.contentType=field;}
        }
    }
    void finish(){if(state==State::Boundary&&padding(line)=="--"+boundary+"--"){delimiter(padding(line),false);line.clear();}if(state!=State::Done)fail();}
};

