#pragma once
#include "CaptureObservations.hpp"
#include "third_party/json.hpp"
#include <wincrypt.h>
#pragma comment(lib,"crypt32.lib")
namespace udmnet {
using ObservationJson=nlohmann::json;
// HTTP field octets are not necessarily UTF-8. Preserve non-UTF-8 fields as
// the JSON library's binary {bytes:[...],subtype:null} representation.
inline ObservationJson captureWireBytes(const std::string& bytes) {
    ObservationJson value=bytes;
    try{value.dump();return value;}
    catch(const ObservationJson::type_error&){return ObservationJson::binary(std::vector<uint8_t>(bytes.begin(),bytes.end()));}
}
inline std::string protectedReplayHeaders(const HttpRequest& request){
    if(request.replayHeaders.empty())return {};
    auto plain=ObservationJson{{"Host",request.host},{"Target",request.target},{"Headers",request.replayHeaders}}.dump();
    DATA_BLOB input{static_cast<DWORD>(plain.size()),reinterpret_cast<BYTE*>(plain.data())},output{};
    const bool protectedOk=CryptProtectData(&input,L"UDM captured request",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output)!=FALSE;
    SecureZeroMemory(plain.data(),plain.size());
    if(!protectedOk)throw std::runtime_error("Cannot protect captured request metadata.");
    DWORD length=0;bool ok=CryptBinaryToStringA(output.pbData,output.cbData,CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,nullptr,&length)!=FALSE;
    std::string encoded(length,0);
    if(ok)ok=CryptBinaryToStringA(output.pbData,output.cbData,CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,encoded.data(),&length)!=FALSE;
    SecureZeroMemory(output.pbData,output.cbData);LocalFree(output.pbData);
    if(!ok)throw std::runtime_error("Cannot encode protected request metadata.");
    encoded.resize(length);if(!encoded.empty()&&encoded.back()=='\0')encoded.pop_back();return encoded;
}
inline ObservationJson captureObservationsJson(const CaptureObservationSnapshot& snapshot,bool includeReplayHeaders=false) {
    ObservationJson rows=ObservationJson::array();
    for(const auto& observation:snapshot.entries){const auto& c=observation.candidate;
        rows.push_back({{"Id",observation.id},{"ConnectionId",observation.connection},{"ProcessId",observation.process},
            {"ProcessCreated100ns",observation.processCreated},{"Method",captureWireBytes(c.request.method)},{"Host",captureWireBytes(c.request.host)},
            {"Target",captureWireBytes(c.request.target)},{"Range",captureWireBytes(c.request.range)},{"Status",c.status},{"ContentType",captureWireBytes(c.contentType)},
            {"ContentDisposition",captureWireBytes(c.contentDisposition)},{"ContentRange",captureWireBytes(c.contentRange)},{"Location",captureWireBytes(c.location)},
            {"ETag",captureWireBytes(c.etag)},{"LastModified",captureWireBytes(c.lastModified)},
            {"Length",c.lengthKnown?ObservationJson(c.length):ObservationJson(nullptr)},
            {"WasInterceptable",c.canIntercept},{"RequestBodyFramed",c.request.bodyFramed},{"RequestBodyRetained",false},{"ResponseRetained",false}});
        if(includeReplayHeaders&&!c.request.replayHeaders.empty())rows.back()["ProtectedRequestHeaders"]=protectedReplayHeaders(c.request);
    }
    return {{"Observed",snapshot.observed},{"Evicted",snapshot.evicted},{"Rejected",snapshot.rejected},
        {"RetainedFieldBytes",snapshot.retainedBytes},{"Items",rows}};
}
}
