#pragma once
#include "Core.hpp"
namespace udm {
inline bool usesPacScript(const Json& prefs){return str(prefs,"ProxyMode")=="Use automatic configuration script";}
inline bool usesWindowsProxy(const Json& prefs){return str(prefs,"ProxyMode",str(prefs,"Proxy").empty()?"Use Windows proxy / PAC settings":"Use a proxy server")=="Use Windows proxy / PAC settings";}
void validatePacSettings(const Json&);
// Resolve one exact URL. Each returned setting represents a route in script order.
// An error never synthesizes a DIRECT alternative.
std::vector<Json> resolvePacRoutes(const Json&,const Url&,const Cancel&);
// Resolve system, per-user and connection-manager settings for this exact URL.
// The returned explicit routes are used for both admission and the transport.
std::vector<Json> resolveWindowsProxyRoutes(const Json&,const Url&,const Cancel&);
Json proxyRouteSettings(const Json&,const WINHTTP_PROXY_RESULT_ENTRY&);
bool retryPacConnection(DWORD);
}
