#pragma once
#include "Core.hpp"
namespace udm {
struct SiteReference {size_t offset=0,length=0;std::string value;bool page=false,html=false,remove=false,navigation=false,css=false;std::string description;};
std::vector<SiteReference> siteReferences(const std::string&,bool css=false,bool preserveActive=false);
std::string rewriteSiteDocument(const std::string&,const std::string& base,bool css,const std::map<std::string,std::string>& files);
void validateOfflineProject(const Json&);
void offlineTransfer(Manager&,JobPtr,const std::shared_ptr<Cancel>&);
}
