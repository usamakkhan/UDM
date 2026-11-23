#pragma once
#include "Core.hpp"
namespace udm {
enum class DashMetadataTransportRole { Manifest, Clock, ResourceDiscovery };
// Stack-owned on the recording worker. Requests retain shared ownership until
// their asynchronous callbacks are closed; no cache survives this scope.
class DashMetadataTransportScope {
 struct State;
 std::unique_ptr<State> state;
 static thread_local State* current;
 friend std::shared_ptr<HttpSession> dashMetadataTransport(const std::string&,const Json&,DashMetadataTransportRole);
public:
 DashMetadataTransportScope();
 ~DashMetadataTransportScope();
 DashMetadataTransportScope(const DashMetadataTransportScope&)=delete;
 DashMetadataTransportScope& operator=(const DashMetadataTransportScope&)=delete;
};
// A missing scope or a full bounded cache preserves ordinary request ownership.
// The key covers request role, canonical origin and every transport preference.
std::shared_ptr<HttpSession> dashMetadataTransport(const std::string&,const Json&);
std::shared_ptr<HttpSession> dashMetadataTransport(const std::string&,const Json&,DashMetadataTransportRole);
}
