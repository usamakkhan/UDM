#pragma once
#include "Core.hpp"
#include <set>
#include <regex>
namespace udm {
inline fs::path setupPath(const std::string& value){
 if(value.empty()||value.size()>32700||value.find_first_of("\r\n")!=std::string::npos||value.find('\0')!=std::string::npos)throw std::runtime_error("Invalid setup path.");
 fs::path path=wide(value);if(!path.is_absolute())throw std::runtime_error("Setup paths must be absolute.");
 path=path.lexically_normal();if(path==path.root_path())throw std::runtime_error("Setup requires a dedicated directory or file.");return path;
}
inline std::string setupPathKey(const fs::path& path){return lower(utf8(fs::absolute(path).lexically_normal().wstring()));}
inline Json inspectSetupData(const Json& request){
 if(!request.is_object()||!request.contains("manifests")||!request["manifests"].is_array()||request["manifests"].size()>12)throw std::runtime_error("Invalid data migration request.");
 auto installation=setupPath(str(request,"installationDirectory")),fallback=setupPath(str(request,"fallbackDirectory"));
 Json sources=Json::array();std::set<std::string> choices,seenManifests;fs::path chosen;
 for(const auto& entry:request["manifests"]){
  if(!entry.is_string())throw std::runtime_error("Invalid registered manifest path.");auto manifest=setupPath(entry.get<std::string>());if(!seenManifests.insert(setupPathKey(manifest)).second)continue;
  auto data=Json::parse(readText(manifest,65536));
  if(!data.is_object()||str(data,"name")!="com.udm.download_manager"||str(data,"type")!="stdio")throw std::runtime_error("A registered host manifest is not a UDM host.");
  auto hostText=str(data,"path");if(hostText.empty()||hostText.find_first_of("\r\n")!=std::string::npos||hostText.find('\0')!=std::string::npos)throw std::runtime_error("Invalid registered host executable.");
  fs::path host=wide(hostText);if(host.is_relative())host=manifest.parent_path()/host;host=setupPath(utf8(host.wstring()));
  if(lower(utf8(host.filename().wstring()))!="udm.nativehost.exe"||!fs::is_directory(host.parent_path()))throw std::runtime_error("The previous UDM installation cannot be located.");
  auto location=configuredData(host.parent_path(),fallback);auto key=setupPathKey(location);choices.insert(key);chosen=location;
  sources.push_back({{"manifest",utf8(manifest.wstring())},{"host",utf8(host.wstring())},{"dataDirectory",utf8(location.wstring())}});
 }
 if(choices.size()>1)throw std::runtime_error("Registered UDM installations use different catalogs. Select one existing installation before upgrading; no history was changed.");
 const auto config=installation/L"udm-data.json";bool existing=fs::exists(config);
 if(existing){auto selected=configuredData(installation,fallback);if(!choices.empty()&&!choices.count(setupPathKey(selected)))throw std::runtime_error("The destination and registered installation use different catalogs. No configuration was overwritten.");chosen=selected;}
 else if(choices.empty())chosen=fallback;
 const bool create=!existing&&setupPathKey(chosen)!=setupPathKey(fallback);
 return {{"ok",true},{"installationDirectory",utf8(installation.wstring())},{"dataDirectory",utf8(chosen.wstring())},{"disposition",existing?"existing":create?"preserve-custom":"default"},{"createConfiguration",create},{"sources",sources}};
}
inline Json rollbackSetupData(const fs::path& receiptPath){
 auto receipt=Json::parse(readText(receiptPath,65536));if(str(receipt,"kind")!="UDM.SetupData.v1")throw std::runtime_error("Unknown setup data receipt.");
 if(!yes(receipt,"created"))return {{"ok",true},{"status","no-change"}};
 auto installation=setupPath(str(receipt,"installationDirectory"));auto operation=str(receipt,"operation");
 if(!std::regex_match(operation,std::regex("[a-fA-F0-9]{32}")))throw std::runtime_error("Invalid migration receipt identifier.");
 auto config=installation/L"udm-data.json",stage=installation/wide(".udm-setup-data-"+operation+".new");
 auto expected=str(receipt,"configuration");if(expected.empty()||expected.size()>4096)throw std::runtime_error("Invalid migration receipt contents.");auto expectedData=Json::parse(expected);if(str(expectedData,"setupMigrationId")!=operation)throw std::runtime_error("Migration receipt identity does not match.");setupPath(str(expectedData,"dataDirectory"));
 bool preserved=false,removed=false;
 Handle owned(CreateFileW(config.c_str(),GENERIC_READ|DELETE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
 if(owned){
  LARGE_INTEGER size{};if(!GetFileSizeEx(owned.h,&size))throw std::runtime_error("Could not inspect the setup data location.");
  if(size.QuadPart==(LONGLONG)expected.size()){
   std::string actual(expected.size(),'\0');DWORD count=0;if(!ReadFile(owned.h,actual.data(),(DWORD)actual.size(),&count,nullptr)||count!=actual.size())throw std::runtime_error("Could not read the setup data location.");
   if(actual==expected){FILE_DISPOSITION_INFO disposition{TRUE};if(!SetFileInformationByHandle(owned.h,FileDispositionInfo,&disposition,sizeof(disposition)))throw std::runtime_error("Could not roll back the setup data location.");removed=true;}else preserved=true;
  }else preserved=true;
 }else if(GetLastError()!=ERROR_FILE_NOT_FOUND&&GetLastError()!=ERROR_PATH_NOT_FOUND)throw std::runtime_error("The setup data location is in use; rollback was deferred.");
 for(const auto& path:{stage,fs::path(stage.wstring()+L".tmp")})if(fs::exists(path)&&!DeleteFileW(path.c_str()))throw std::runtime_error("Could not remove the setup staging file.");
 return {{"ok",true},{"status",preserved?"preserved-changed-config":removed?"removed-created-config":"already-absent"}};
}
inline Json prepareSetupData(const Json& request,const fs::path& receiptPath,const std::function<void(const char*)>& checkpoint={}){
 auto plan=inspectSetupData(request);auto installation=setupPath(str(plan,"installationDirectory"));
 if(!fs::is_regular_file(installation/L"UDM.exe")||!fs::is_regular_file(installation/L"Udm.NativeHost.exe"))throw std::runtime_error("The destination UDM executables are missing.");
 if(setupPathKey(receiptPath).rfind(setupPathKey(installation)+"\\",0)==0)throw std::runtime_error("Keep the migration receipt outside the application directory.");
 if(fs::exists(receiptPath))throw std::runtime_error("A setup data receipt already exists. Preserve it and finish rollback before retrying.");
 Json receipt={{"kind","UDM.SetupData.v1"},{"created",yes(plan,"createConfiguration")},{"installationDirectory",utf8(installation.wstring())}};
 if(!yes(plan,"createConfiguration")){atomicText(receiptPath,receipt.dump(2),false);return plan;}
 const auto operation=guid();const auto config=installation/L"udm-data.json",stage=installation/wide(".udm-setup-data-"+operation+".new");
 auto contents=Json{{"dataDirectory",plan["dataDirectory"]},{"setupMigrationId",operation}}.dump(2);
 if(contents.size()>4096)throw std::runtime_error("The data configuration exceeds the supported size.");
 if(fs::exists(stage)||fs::exists(fs::path(stage.wstring()+L".tmp")))throw std::runtime_error("The setup staging location is already occupied.");
 receipt["operation"]=operation;receipt["configuration"]=contents;
 atomicText(receiptPath,receipt.dump(2),false);
 try{
  if(checkpoint)checkpoint("receipt-written");
  atomicText(stage,contents,false);
  if(checkpoint)checkpoint("staged");
  // No replacement flag: a concurrently created configuration always wins.
  if(!MoveFileExW(stage.c_str(),config.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("The data configuration could not be published. Existing configuration was preserved.");
  if(checkpoint)checkpoint("published");
 }catch(...){auto failure=std::current_exception();rollbackSetupData(receiptPath);std::rethrow_exception(failure);}
 return plan;
}
}
