#pragma once
// HTTP/2 to HTTP/1.1 fallback. A download that fails while an HTTP/2 exchange
// is in progress (stream reset, GOAWAY, transport error) is retried once with
// HTTP/1.1 only, and keeps that choice for later connections and resumes.
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
}
