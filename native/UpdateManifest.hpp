#pragma once
// Signed update manifests. The release host publishes latest.json: an envelope whose
// "manifest" is the base64 of a JSON document naming the version and the setup (URL on
// the UDM repository's releases, SHA-256, size) and whose "signature" is an ECDSA P-256
// signature over those exact bytes by the UDM update key (tools/publish-manifest.ps1).
// UDM verifies it with the public key built in (UpdateKey.hpp), downloads the setup
// through its own transfer with the manifest's SHA-256 as the expected checksum, and
// starts it only when it also carries a valid, trusted Authenticode signature.
#include "Core.hpp"
#include "Updates.hpp"
#include "UpdateKey.hpp"
#include <bcrypt.h>
#include <wintrust.h>
#include <softpub.h>
#include <shellapi.h>
#include <regex>
#pragma comment(lib,"wintrust.lib")
namespace udm {
inline constexpr const char* updateManifestUrl="https://github.com/usamakkhan/UDM/releases/latest/download/latest.json";
inline constexpr const char* updateSetupPrefix="/usamakkhan/UDM/releases/download/";
inline constexpr i64 updateCheckIntervalMs=20LL*3600*1000;
inline bool updateKeyConfigured(const unsigned char* key=updatePublicKey){for(int i=0;i<64;++i)if(key[i])return true;return false;}
struct UpdateManifest{std::string version,browserVersion,setupUrl,setupSha256,notes,published;i64 setupSize=0;bool newer=false;};
inline Bytes sha256Raw(const Bytes& data){
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Cannot initialize SHA-256.");
 Bytes digest(32);const bool ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0&&(data.empty()||BCryptHashData(hash,(PUCHAR)data.data(),(ULONG)data.size(),0)>=0)&&BCryptFinishHash(hash,digest.data(),32,0)>=0;
 if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(algorithm,0);if(!ok)throw std::runtime_error("SHA-256 failed.");return digest;
}
inline bool verifyUpdateSignature(const Bytes& data,const Bytes& signature,const unsigned char* key){
 if(signature.size()!=64)return false;
 std::vector<unsigned char> blob(sizeof(BCRYPT_ECCKEY_BLOB)+64);auto header=reinterpret_cast<BCRYPT_ECCKEY_BLOB*>(blob.data());header->dwMagic=BCRYPT_ECDSA_PUBLIC_P256_MAGIC;header->cbKey=32;std::memcpy(blob.data()+sizeof(BCRYPT_ECCKEY_BLOB),key,64);
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_KEY_HANDLE handle=nullptr;bool ok=false;
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_ECDSA_P256_ALGORITHM,nullptr,0)>=0){
  if(BCryptImportKeyPair(algorithm,nullptr,BCRYPT_ECCPUBLIC_BLOB,&handle,blob.data(),(ULONG)blob.size(),0)>=0){
   try{auto digest=sha256Raw(data);ok=BCryptVerifySignature(handle,nullptr,digest.data(),32,(PUCHAR)signature.data(),64,0)>=0;}catch(...){ok=false;}
   BCryptDestroyKey(handle);
  }
  BCryptCloseAlgorithmProvider(algorithm,0);
 }
 return ok;
}
inline UpdateManifest parseUpdateManifest(const std::string& envelopeText,const unsigned char* key=updatePublicKey,const std::string& current=currentProductVersion){
 if(!updateKeyConfigured(key))throw std::runtime_error("This build of UDM has no update signing key, so it cannot verify update manifests.");
 Json envelope;try{envelope=Json::parse(envelopeText);}catch(...){throw std::runtime_error("The update manifest is not valid JSON.");}
 if(!envelope.is_object()||str(envelope,"algorithm")!="ecdsa-p256-sha256"||!envelope.contains("manifest")||!envelope["manifest"].is_string()||!envelope.contains("signature")||!envelope["signature"].is_string())throw std::runtime_error("The update manifest has an unsupported envelope.");
 Bytes manifest,signature;try{manifest=unb64(str(envelope,"manifest"));signature=unb64(str(envelope,"signature"));}catch(...){throw std::runtime_error("The update manifest's encoding is invalid.");}
 if(!verifyUpdateSignature(manifest,signature,key))throw std::runtime_error("The update manifest's signature does not verify against UDM's update key.");
 Json body;try{body=Json::parse(manifest.begin(),manifest.end());}catch(...){throw std::runtime_error("The signed manifest is not valid JSON.");}
 if(!body.is_object()||!body.contains("setup")||!body["setup"].is_object())throw std::runtime_error("The signed manifest names no setup.");
 UpdateManifest result;result.version=str(body,"version");if(result.version.empty()||result.version.size()>32)throw std::runtime_error("The signed manifest has no usable version.");const auto parsed=releaseVersion(result.version);
 const auto& setup=body["setup"];result.setupUrl=str(setup,"url");if(result.setupUrl.size()>2048)throw std::runtime_error("The update's setup address is too long.");
 Url url(result.setupUrl);if(url.scheme!="https"||url.host!="github.com"||url.port!=443||url.path.rfind(updateSetupPrefix,0)!=0||url.path.size()==std::strlen(updateSetupPrefix)||!url.query.empty())throw std::runtime_error("The update's setup is outside the UDM repository's releases.");
 result.setupSha256=lower(str(setup,"sha256"));if(!std::regex_match(result.setupSha256,std::regex("[0-9a-f]{64}")))throw std::runtime_error("The update's setup has no SHA-256.");
 result.setupSize=num(setup,"size");if(result.setupSize<=0||result.setupSize>1024LL*1024*1024)throw std::runtime_error("The update's setup size is not plausible.");
 result.notes=str(body,"notes");if(!result.notes.empty()){if(result.notes.size()>2048)throw std::runtime_error("The release notes address is too long.");Url notes(result.notes);if(notes.scheme!="https"||notes.host!="github.com")throw std::runtime_error("The release notes are outside GitHub.");}
 result.browserVersion=str(body,"browserVersion");result.published=str(body,"published");if(result.browserVersion.size()>32||result.published.size()>32)throw std::runtime_error("The signed manifest has oversized fields.");
 result.newer=parsed>releaseVersion(current);return result;
}
inline UpdateManifest fetchUpdateManifest(const Json& settings,const Cancel& cancel,const std::string& address=updateManifestUrl,const unsigned char* key=updatePublicKey){
 Headers headers={{"Accept","application/json"},{"User-Agent",std::string("UDM/")+currentProductVersion}};
 Http request(address,headers,settings,cancel,{},{},"",nullptr,true);
 if(request.status==404)throw std::runtime_error("No update manifest is published yet.");
 if(request.status!=200)throw std::runtime_error("The update manifest returned HTTP "+std::to_string(request.status)+".");
 auto body=request.all(64*1024,cancel);cancel.check();return parseUpdateManifest(std::string(body.begin(),body.end()),key);
}
// Startup policy: on unless turned off, at most once per 20 hours, and a version the user skipped stays silent.
inline bool updateCheckDue(const Json& settings,i64 now){
 if(!yes(settings,"CheckForUpdates",true))return false;
 i64 last=0;try{if(settings.contains("UpdateCheckedAt"))last=parseDate(settings["UpdateCheckedAt"]);}catch(...){last=0;}
 return now-last>=updateCheckIntervalMs;
}
inline bool updateSkipped(const Json& settings,const std::string& version){return !version.empty()&&str(settings,"SkippedUpdateVersion")==version;}
inline std::string updateSetupFileName(const UpdateManifest& manifest){
 Url url(manifest.setupUrl);auto slash=url.path.rfind('/');auto name=slash==std::string::npos?std::string():url.path.substr(slash+1);
 if(std::regex_match(name,std::regex("[A-Za-z0-9._-]{1,120}\\.exe")))return name;return "UDM-"+manifest.version+"-Setup-x64.exe";
}
// Empty when the file carries a valid Authenticode signature chaining to a trusted root;
// otherwise the reason. Revocation is not fetched, so an offline machine can still update.
inline std::string setupSignatureProblem(const fs::path& path){
 const auto name=path.wstring();WINTRUST_FILE_INFO file{};file.cbStruct=sizeof(file);file.pcwszFilePath=name.c_str();
 WINTRUST_DATA data{};data.cbStruct=sizeof(data);data.dwUIChoice=WTD_UI_NONE;data.fdwRevocationChecks=WTD_REVOKE_NONE;data.dwUnionChoice=WTD_CHOICE_FILE;data.pFile=&file;data.dwStateAction=WTD_STATEACTION_VERIFY;data.dwProvFlags=WTD_CACHE_ONLY_URL_RETRIEVAL|WTD_REVOCATION_CHECK_NONE;
 GUID action=WINTRUST_ACTION_GENERIC_VERIFY_V2;const LONG status=WinVerifyTrust(nullptr,&action,&data);data.dwStateAction=WTD_STATEACTION_CLOSE;WinVerifyTrust(nullptr,&action,&data);
 if(status==ERROR_SUCCESS)return "";
 switch((DWORD)status){
  case (DWORD)TRUST_E_NOSIGNATURE:return "The downloaded setup is not signed.";
  case (DWORD)TRUST_E_EXPLICIT_DISTRUST:return "The downloaded setup's signature is distrusted.";
  case (DWORD)TRUST_E_SUBJECT_NOT_TRUSTED:return "The downloaded setup's publisher is not trusted.";
  case (DWORD)CERT_E_UNTRUSTEDROOT:return "The downloaded setup's certificate chains to an untrusted root.";
  case (DWORD)CERT_E_EXPIRED:return "The downloaded setup's certificate has expired.";
  case (DWORD)TRUST_E_BAD_DIGEST:return "The downloaded setup's signature does not match its content.";
  default:{std::ostringstream text;text<<"The downloaded setup's signature could not be verified (0x"<<std::hex<<(DWORD)status<<").";return text.str();}
 }
}
// Runs the setup only when it matches the signed manifest and carries a trusted publisher signature.
inline void launchDownloadedSetup(const fs::path& path,const std::string& expectedSha256,HWND owner){
 if(!fs::is_regular_file(path))throw std::runtime_error("The downloaded setup is missing.");
 if(lower(fileHash(path))!=lower(expectedSha256))throw std::runtime_error("The downloaded setup does not match the signed update manifest. It was not run.");
 const auto problem=setupSignatureProblem(path);if(!problem.empty())throw std::runtime_error(problem+" It was not run; the file is at "+utf8(path.wstring())+".");
 const auto result=ShellExecuteW(owner,L"open",path.c_str(),L"/CLOSEAPPLICATIONS",nullptr,SW_SHOWNORMAL);if((INT_PTR)result<=32)throw std::runtime_error("Windows could not start the downloaded setup.");
}
}
