#pragma once
#include "Core.hpp"
#include "OptionsModel.hpp"
#include "OfflineSite.hpp"
#include "GrabberFilters.hpp"
#include "GrabberAuth.hpp"
#include <algorithm>
#include <regex>
namespace udm {
inline std::string grabberSaveMode(const Json& project){
 return str(project,"SaveMode",trim(str(project,"Folder")).empty()?"Categories":"Folder");
}
inline fs::path grabberRelativeFile(const std::string& address){
 Url url(address);if(url.scheme!="http"&&url.scheme!="https")throw std::runtime_error("Grabber destinations require an HTTP or HTTPS address.");
 auto raw=url.path;if(raw.find('\\')!=std::string::npos)throw std::runtime_error("The website path contains a backslash.");
 fs::path relative;size_t begin=0;int count=0;
 while(begin<raw.size()){
  auto end=raw.find('/',begin);if(end==std::string::npos)end=raw.size();auto piece=raw.substr(begin,end-begin);begin=end+1;if(piece.empty())continue;
  piece=unescape(piece);if(piece=="."||piece==".."||piece.find_first_of("/\\")!=std::string::npos||std::any_of(piece.begin(),piece.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("The website path contains an unsafe folder component.");
  if(++count>48)throw std::runtime_error("The website folder hierarchy is too deep.");auto safe=safeName(piece);if(safe.empty()||safe=="."||safe=="..")throw std::runtime_error("The website path has no usable file name.");relative/=wide(safe);
 }
 if(relative.empty()||raw.empty()||raw.back()=='/')relative/=L"index.html";
 if(relative.is_absolute()||relative.has_root_path()||relative.generic_wstring().size()>1800)throw std::runtime_error("The website folder hierarchy is too long.");return relative;
}
inline void validateGrabberDestination(const Json& project,const Json& settings){
 for(const char* key:{"SaveMode","SaveCategory","Folder"})if(project.contains(key)&&!project[key].is_string())throw std::runtime_error("Invalid Grabber destination setting.");
 if(project.contains("OriginalSubfolders")&&!project["OriginalSubfolders"].is_boolean())throw std::runtime_error("Invalid original-subfolders setting.");
 const auto mode=grabberSaveMode(project),folder=trim(str(project,"Folder"));if(mode!="Categories"&&mode!="Category"&&mode!="Folder")throw std::runtime_error("Choose a Grabber destination mode.");
 if(mode=="Folder"&&!folder.empty()&&!fs::path(wide(folder)).is_absolute())throw std::runtime_error("Choose an absolute project download folder.");
 if(mode=="Folder"&&folder.empty())throw std::runtime_error("Choose a folder for the Grabber project.");
 if(mode=="Category"){auto categories=optionCategories(settings);if(std::find(categories.begin(),categories.end(),str(project,"SaveCategory"))==categories.end())throw std::runtime_error("Choose an existing category for this project.");}
 if(yes(project,"OriginalSubfolders")&&(mode!="Folder"||str(project,"Template")=="Offline website (ZIP)"))throw std::runtime_error("Original subfolders require a folder destination for collected files.");
}
inline void validateGrabberProject(const Json& project,const Json& settings){
 grabberLogin(project);validateGrabberBrowserLogin(project);validateGrabberDestination(project,settings);if(str(project,"Template")=="Offline website (ZIP)")validateOfflineProject(project);GrabberFilters filters(project);
 if(!std::regex_match(str(project,"Id"),std::regex("[a-fA-F0-9]{32}"))||trim(str(project,"Name")).empty()||!project.contains("Links")||!project["Links"].is_array()||project["Links"].size()>2000)throw std::runtime_error("Invalid grabber project.");
 for(const auto& link:project["Links"]){Url url(str(link,"Url"));if(url.scheme!="http"&&url.scheme!="https"||(filters.legacy&&url.origin!=filters.source.origin))throw std::runtime_error("Project files must belong to the allowed websites.");if(link.contains("Selected")&&!link["Selected"].is_boolean())throw std::runtime_error("Invalid project file selection.");if(link.contains("FileName")&&(!link["FileName"].is_string()||safeName(str(link,"FileName"))!=str(link,"FileName")))throw std::runtime_error("Invalid collected file name.");if(link.contains("SaveName")&&(!link["SaveName"].is_string()||str(link,"SaveName").empty()||safeName(str(link,"SaveName"))!=str(link,"SaveName")))throw std::runtime_error("Invalid collected save name.");if(link.contains("Description")&&(!link["Description"].is_string()||str(link,"Description").size()>1024))throw std::runtime_error("Invalid collected file description.");if(link.contains("Size")&&(!link["Size"].is_number_integer()||num(link,"Size")< -1))throw std::runtime_error("Invalid collected file size.");if(!str(link,"Referrer").empty())grabberCanonical(str(link,"Referrer"));if(!str(link,"DiscoveredUrl").empty())grabberCanonical(str(link,"DiscoveredUrl"));if(yes(project,"OriginalSubfolders"))grabberRelativeFile(url.full);}
}
struct GrabberDestination {std::string folder,name,category,root;};
inline GrabberDestination grabberDestination(const Json& project,const Json& settings,const std::string& address,const std::string& filename=""){
 validateGrabberDestination(project,settings);GrabberDestination result;const auto mode=grabberSaveMode(project);Url url(address);result.name=safeName(filename.empty()?unescape(url.path):filename);result.category=downloadCategory(result.name,url.host,settings);
 if(mode=="Categories")result.folder=categoryFolder(settings,result.category);
 else if(mode=="Category"){result.category=str(project,"SaveCategory");result.folder=categoryFolder(settings,result.category);}
 else if(mode=="Folder")result.folder=trim(str(project,"Folder"));
 if(yes(project,"OriginalSubfolders")){
  auto relative=grabberRelativeFile(address);if(!filename.empty())relative.replace_filename(wide(safeName(filename)));auto root=fs::path(wide(result.folder)).lexically_normal();auto target=(root/relative).lexically_normal();auto inside=target.lexically_relative(root);
  if(inside.empty()||inside.is_absolute()||*inside.begin()==L"..")throw std::runtime_error("The website path leaves the selected destination.");
  result.root=utf8(root.wstring());result.folder=utf8(target.parent_path().wstring());result.name=utf8(target.filename().wstring());result.category=downloadCategory(result.name,url.host,settings);
 }return result;
}
// Website-created subfolders must not redirect writes outside the selected root.
// The root itself is an explicit user destination and can be a mapped folder.
inline void validateGrabberFolder(const Json& job){
 auto value=str(job,"ProjectDestinationRoot");if(value.empty())return;auto root=fs::path(wide(value)).lexically_normal(),folder=fs::path(wide(str(job,"Folder"))).lexically_normal();
 if(!root.is_absolute()||!folder.is_absolute())throw std::runtime_error("Invalid saved Grabber destination.");auto relative=folder.lexically_relative(root);
 if(relative.empty()&&folder!=root)throw std::runtime_error("The Grabber destination is outside its selected root.");if(relative.is_absolute()||(!relative.empty()&&*relative.begin()==L".."))throw std::runtime_error("The Grabber destination is outside its selected root.");
 auto inspect=[](const fs::path& path,bool subfolder){auto attributes=GetFileAttributesW(path.c_str());if(attributes==INVALID_FILE_ATTRIBUTES){auto error=GetLastError();if(error!=ERROR_FILE_NOT_FOUND&&error!=ERROR_PATH_NOT_FOUND)throw std::runtime_error("Cannot inspect a Grabber destination folder.");}else if((subfolder&&(attributes&FILE_ATTRIBUTE_REPARSE_POINT))||!(attributes&FILE_ATTRIBUTE_DIRECTORY))throw std::runtime_error("A website subfolder is redirected or occupied by a file. Choose another project folder.");};
 inspect(root,false);auto current=root;for(const auto& component:relative){if(component==L".")continue;if(component==L"..")throw std::runtime_error("Invalid Grabber destination traversal.");current/=component;inspect(current,true);}
}
inline void validateGrabberPlan(const std::vector<JobPtr>& jobs,size_t first){
 std::set<std::string> targets,added;for(size_t i=0;i<jobs.size();++i){auto target=lower(utf8(jobs[i]->target().lexically_normal().wstring()));targets.insert(target);if(i>=first)added.insert(target);if(!str(jobs[i]->data,"PreviousPath").empty())targets.insert(lower(utf8(fs::path(wide(str(jobs[i]->data,"PreviousPath"))).lexically_normal().wstring())));}
 for(size_t i=0;i<jobs.size();++i){const auto& conflicts=i<first?added:targets;auto parent=jobs[i]->target().parent_path().lexically_normal();while(!parent.empty()){if(conflicts.count(lower(utf8(parent.wstring()))))throw std::runtime_error("A collected file would occupy another download's folder. Change the selection or destination.");auto next=parent.parent_path();if(next==parent)break;parent=next;}}
}
}
