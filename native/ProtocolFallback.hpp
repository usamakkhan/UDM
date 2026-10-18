#pragma once
// Protocol fallback. A download that fails while an HTTP/3 exchange is in
// progress is retried once without HTTP/3; one that fails during an HTTP/2
// exchange (stream reset, GOAWAY, transport error) is retried once with
// HTTP/1.1 only. Each choice stays with the download for later connections and resumes.
#include "Core.hpp"
#include "CurlHttp.hpp"
namespace udm {
struct InternetFailure:std::runtime_error {DWORD code;InternetFailure(const char* operation,DWORD error):runtime_error(std::string(operation)+" failed (Windows error "+std::to_string(error)+")."),code(error){}};
// Transport-level failures only; server replies and changed resources are
// handled by their own retry rules.
inline bool protocolFallbackError(const std::exception& e){
 return dynamic_cast<const InternetFailure*>(&e)||dynamic_cast<const CurlBodyDisconnect*>(&e)||dynamic_cast<const CurlResponseDisconnect*>(&e);
}
inline bool shouldDisableHttp2(const std::string& protocol,bool alreadyDisabled,const std::exception& e){
 return protocol=="HTTP/2"&&!alreadyDisabled&&protocolFallbackError(e);
}
inline bool shouldDisableHttp3(const std::string& protocol,bool alreadyDisabled,const std::exception& e){
 return protocol=="HTTP/3"&&!alreadyDisabled&&protocolFallbackError(e);
}
}
