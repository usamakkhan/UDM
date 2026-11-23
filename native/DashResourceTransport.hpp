#pragma once
#include "Core.hpp"
namespace udm {
class DashResourceTransportScope;
class DashResourceTransportBinding;
class DashResourceTransportContext {
 struct State;
 std::shared_ptr<State> state;
 static thread_local std::shared_ptr<State> current;
 friend class DashResourceTransportScope;
 friend class DashResourceTransportBinding;
 friend DashResourceTransportContext dashResourceTransportContext();
 friend std::shared_ptr<HttpSession> dashResourceTransport(const std::string&,const Json&,const Headers&);
public:
 explicit operator bool()const noexcept{return bool(state);}
};
// One recording owns the cache. Capture explicitly binds that owner on its
// joined workers; unrelated threads and later recordings cannot borrow it.
class DashResourceTransportScope {
 DashResourceTransportContext previous;
public:
 DashResourceTransportScope();
 ~DashResourceTransportScope();
 DashResourceTransportScope(const DashResourceTransportScope&)=delete;
 DashResourceTransportScope& operator=(const DashResourceTransportScope&)=delete;
};
class DashResourceTransportBinding {
 DashResourceTransportContext previous;
public:
 explicit DashResourceTransportBinding(const DashResourceTransportContext&);
 ~DashResourceTransportBinding();
 DashResourceTransportBinding(const DashResourceTransportBinding&)=delete;
 DashResourceTransportBinding& operator=(const DashResourceTransportBinding&)=delete;
};
DashResourceTransportContext dashResourceTransportContext();
// Bounded by 16 retained sessions and 1 MiB per complete identity key. Overflow
// uses ordinary request ownership. Credential headers are the origin's original
// identity, excluding per-resource Range/If-Match values added by the writer.
std::shared_ptr<HttpSession> dashResourceTransport(const std::string&,const Json&,const Headers&);
}
