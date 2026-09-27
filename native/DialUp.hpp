#pragma once
#include "Core.hpp"
namespace udm {
struct DialEntry {std::string name,phonebook;};
std::vector<DialEntry> dialEntries();
void validateDialSettings(const Json&);
void ensureDialConnection(const Json&,const Cancel&,std::function<void(const std::string&)> report={});
// Interface isolates OS dialing from retry/concurrency tests. Production always
// uses Windows RAS; test implementations cannot change global network settings.
class DialBackend {
public: virtual ~DialBackend()=default;
 virtual bool connected(const Json&)=0;
 virtual void connect(const Json&,const Cancel&)=0;
};
class DialUp {
 std::unique_ptr<DialBackend> backend;
 std::timed_mutex mutex;
 std::string failedKey,failedMessage;ULONGLONG failedUntil=0;
public:
 explicit DialUp(std::unique_ptr<DialBackend> implementation={});~DialUp();
 void ensure(const Json&,const Cancel&,std::function<void(const std::string&)> report={});
};
}
