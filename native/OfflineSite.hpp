#pragma once
#include "Core.hpp"
namespace udm {
struct SiteReference {size_t offset=0,length=0;std::string value;bool page=false,html=false,remove=false,navigation=false,css=false;std::string description;};
std::vector<SiteReference> siteReferences(const std::string&,bool css=false,bool preserveActive=false);
std::vector<SiteReference> siteReferences(const std::string&,bool css,bool preserveActive,bool discoverScriptLinks);
std::string rewriteSiteDocument(const std::string&,const std::string& base,bool css,const std::map<std::string,std::string>& files,const std::string& documentName="");
bool javascriptMimeEssence(const std::string&);
struct SiteScript {std::string source,body;bool module=false;size_t bodyOffset=0;};
std::vector<SiteScript> siteScripts(const std::string&);
std::vector<std::string> siteScriptSources(const std::string&);
std::vector<SiteReference> scriptReferences(const std::string&);
void validateOfflineProject(const Json&);
void offlineTransfer(Manager&,JobPtr,const std::shared_ptr<Cancel>&);
}
