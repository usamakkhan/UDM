#pragma once
#include "Core.hpp"
namespace udm {
// A private, authenticated loopback bridge preserves WinHTTP's TLS validation and
// range engine while SOCKS5 resolves and connects to the destination remotely.
inline bool isSocksProxy(const Json& p){auto mode=str(p,"ProxyMode");return mode=="Use a SOCKS5 proxy"||mode=="Use a SOCKS4 / 4a proxy";}
void validateSocksSettings(const Json&);
bool socksBypass(const Url&,const std::string&);
void validateSocksDestination(const Url&,const Json&);
class SocksProxy {
 struct Impl;
 std::unique_ptr<Impl> impl;
public:
 explicit SocksProxy(const Json&);
 ~SocksProxy();
 std::wstring address() const;
 void allow(const Url&);
 void credentials(HINTERNET) const;
};
}
