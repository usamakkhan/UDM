#pragma once
#include <functional>
#include <memory>
#include <string>
namespace udm {
struct FileInfoInitialView;
using PreparedFileInfoSink=std::function<void(std::shared_ptr<const FileInfoInitialView>)>;
using RejectedFileInfoSink=std::function<void(const std::string&)>;
// Only entry points known not to hold an outer Manager lock opt into detached
// admission. Legacy/nested in-process callers retain synchronous semantics.
inline thread_local bool detachedBrowserAdmissionAllowed=false;
inline thread_local const PreparedFileInfoSink* preparedBrowserFileInfo=nullptr;
inline thread_local const RejectedFileInfoSink* rejectedBrowserFileInfo=nullptr;
// Called only after the owning lease has frozen new publications and resolved
// every captured write without a durable acceptance. Observers cannot change
// the persistence result or replace its original exception.
inline void notifyRejectedBrowserFileInfo(const std::string& id)noexcept{if(rejectedBrowserFileInfo)try{(*rejectedBrowserFileInfo)(id);}catch(...){}}
class BrowserAdmissionContext {
 bool previous;
 const PreparedFileInfoSink* previousSink;
 const RejectedFileInfoSink* previousRejected;
public:
 explicit BrowserAdmissionContext(bool allow=true,const PreparedFileInfoSink* sink=nullptr,const RejectedFileInfoSink* rejected=nullptr)noexcept:previous(detachedBrowserAdmissionAllowed),previousSink(preparedBrowserFileInfo),previousRejected(rejectedBrowserFileInfo){detachedBrowserAdmissionAllowed=allow;preparedBrowserFileInfo=allow?sink:nullptr;rejectedBrowserFileInfo=allow?rejected:nullptr;}
 ~BrowserAdmissionContext(){detachedBrowserAdmissionAllowed=previous;preparedBrowserFileInfo=previousSink;rejectedBrowserFileInfo=previousRejected;}
 BrowserAdmissionContext(const BrowserAdmissionContext&)=delete;
 BrowserAdmissionContext& operator=(const BrowserAdmissionContext&)=delete;
};
}
