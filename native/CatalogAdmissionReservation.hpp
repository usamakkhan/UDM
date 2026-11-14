#pragma once
#include "Core.hpp"
#include "WindowsPath.hpp"
namespace udm {
// Immutable, in-process reservation for a validated protocol-3 media admission.
// This is not a token registry or an asynchronous acceptance decision. Callers
// must still reserve ownership/lifetime before using overlays concurrently.
class CatalogAdmissionReservation {
 std::string token,id;
 std::wstring destination;
 Json job,receipt;
 static std::wstring destinationKey(const Json& record){
  const auto folder=str(record,"Folder"),name=str(record,"FileName");
  if(folder.empty()||name.empty()||!fs::path(wide(folder)).is_absolute()||safeName(name)!=name)
   throw std::runtime_error("Invalid reserved media destination.");
  return windowsPath::key(fs::path(wide(folder))/wide(name));
 }
 static bool sameDestination(const std::wstring& a,const std::wstring& b)noexcept{return windowsPath::equal(a,b);}
public:
 CatalogAdmissionReservation(std::string operation,Json download,Json accepted):token(std::move(operation)),id(str(download,"Id")),job(std::move(download)),receipt(std::move(accepted)){
  if(token.empty()||id.empty()||!job.is_object()||!receipt.is_object()||num(receipt,"protocol")!=3||str(receipt,"status")!="accepted"||str(receipt,"id")!=id||str(receipt,"fingerprint").empty())
   throw std::runtime_error("Invalid reserved media receipt.");
  destination=destinationKey(job);
 }
 const std::string& operation()const noexcept{return token;}
 const std::string& jobId()const noexcept{return id;}
 const std::string& binding()const{return receipt.at("fingerprint").get_ref<const std::string&>();}
 bool exclusiveDestination()const{return str(job,"DuplicateOf").empty();}
 bool sharesDestination(const CatalogAdmissionReservation& other)const noexcept{return sameDestination(destination,other.destination);}
 bool ownsDestination(const fs::path& path)const{return sameDestination(destination,windowsPath::key(path));}
 bool requiresDirectory(const fs::path& path)const{return windowsPath::descendant(destination,windowsPath::key(path));}
 void overlay(Json& catalog)const{
  if(!catalog.is_object()||!catalog.contains("Downloads")||!catalog["Downloads"].is_array())throw std::runtime_error("Invalid admission catalog snapshot.");
  if(catalog.contains("BrowserCaptures")&&!catalog["BrowserCaptures"].is_object())throw std::runtime_error("Invalid admission receipt store.");
  // Validate every conflict before mutating the detached snapshot.
  const Json* existing=nullptr;
  for(const auto& row:catalog["Downloads"])if(str(row,"Id")==id){if(existing)throw std::runtime_error("Duplicate reserved media identity.");existing=&row;}
  if(existing&&*existing!=job)throw std::runtime_error("The reserved media download changed before publication.");
  if(!existing&&str(job,"DuplicateOf").empty())for(const auto& row:catalog["Downloads"])
   if(!str(row,"Folder").empty()&&!str(row,"FileName").empty()&&sameDestination(destinationKey(row),destination))throw std::runtime_error("The reserved media destination is already in use.");
  if(catalog.contains("BrowserCaptures")){
   const auto& rows=catalog["BrowserCaptures"];const auto found=rows.find(token);
   if(found!=rows.end()&&*found!=receipt)throw std::runtime_error("The reserved media operation changed before publication.");
  }
  if(!existing)catalog["Downloads"].push_back(job);
  if(!catalog.contains("BrowserCaptures"))catalog["BrowserCaptures"]=Json::object();
  catalog["BrowserCaptures"][token]=receipt;
 }
};
}
