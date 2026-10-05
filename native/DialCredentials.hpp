#pragma once
#include "DialUp.hpp"
#include <ras.h>
namespace udm {
struct DialCredentialInfo {std::string userName;bool savedPassword=false,sessionPassword=false;};
struct DialCredentialChange {std::string userName,protectedPassword;bool passwordEdited=false,savePassword=false;};
std::pair<std::wstring,std::wstring> dialCredentialName(const std::string&);
void validateDialCredentialChange(const DialCredentialChange&);
class DialCredentialStore {
public:
 virtual ~DialCredentialStore()=default;
 virtual DialCredentialInfo read(const DialEntry&)=0;
 virtual void write(const DialEntry&,const DialCredentialChange&)=0;
};
// The default store is shared only inside this process. Unsaved passwords are
// DPAPI-protected in memory and never copied into settings/history or exports.
std::shared_ptr<DialCredentialStore> dialCredentialStore();
void dialConnectionCredentials(const DialEntry&,RASDIALPARAMSW&);
}
