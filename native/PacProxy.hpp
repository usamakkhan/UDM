#pragma once
#include "Core.hpp"
namespace udm {
inline bool usesPacScript(const Json& prefs){return str(prefs,"ProxyMode")=="Use automatic configuration script";}
void validatePacSettings(const Json&);
// Resolve one exact URL. Each returned setting represents a route in script order.
// An error never synthesizes a DIRECT alternative.
std::vector<Json> resolvePacRoutes(const Json&,const Url&,const Cancel&);
bool retryPacConnection(DWORD);
}
