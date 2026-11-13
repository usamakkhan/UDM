#pragma once
#include "Core.hpp"
namespace udm {
// HTTP range offsets always describe the encoded response. UI projections may
// show decoded output, but must never feed those offsets back into transport.
inline bool pendingHttpGzip(const Json& data){return str(data,"HttpContentEncoding")=="gzip"&&str(data,"Status")!="Complete";}
inline i64 httpOutputSize(const Json& data){return pendingHttpGzip(data)?num(data,"HttpDecodedSize",-1):num(data,"Size",-1);}
inline i64 httpOutputReceived(const Json& data){return pendingHttpGzip(data)?num(data,"HttpDecodedReceived"):num(data,"Received");}
inline void projectHttpOutput(Json& data){
 if(!pendingHttpGzip(data))return;
 data["Size"]=httpOutputSize(data);data["Received"]=httpOutputReceived(data);
 data.erase("BrowserReportedDirectSize");
 data["Segments"]=Json::array();
 const auto size=num(data,"Size",-1);
 if(size>0)data["Segments"].push_back({{"Index",0},{"Start",0},{"End",size-1},{"Done",std::min(size,num(data,"Received"))}});
}
}
