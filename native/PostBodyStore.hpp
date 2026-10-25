#pragma once
// Staged POST bodies. A captured form body above MaxBrowserPostBytes cannot ride inside one
// pipe frame or the 32 MB catalog, so the browser sends it in base64 chunks (post-body-begin,
// post-body-chunk, post-body-finish; each frame stays under BrowserRequestMessageLimit). The
// desktop appends the chunks to <root>/requests/<token>.part, verifies the declared size and
// SHA-256, seals the bytes DPAPI-protected as <token>.bin and hands back a small reference
// {bodyFile, bodySize, bodySha256} that the download record stores in place of the bytes.
// Bytes are never interpreted as text, so binary and non-UTF-8 producers round-trip exactly.
// Body files are pruned when no record references them; abandoned stagings expire.
#include "Core.hpp"
#include <wincrypt.h>
#include <bcrypt.h>
#include <fstream>
#include <regex>
#include <set>
namespace udm {
struct StagedPostBody{fs::path root;size_t size=0,received=0;std::string sha256,contentType;i64 started=0;};
inline std::mutex& stagedPostMutex(){static std::mutex value;return value;}
inline std::map<std::string,StagedPostBody>& stagedPostBodies(){static std::map<std::string,StagedPostBody> value;return value;}
inline bool postBodyToken(const std::string& value){return std::regex_match(value,std::regex("[0-9a-f]{32}"));}
inline fs::path postBodyPath(const fs::path& root,const std::string& token,const wchar_t* extension){return root/L"requests"/(wide(token)+extension);}
inline std::string sha256Hex(const Bytes& data){
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Cannot initialize SHA-256.");
 unsigned char digest[32]{};bool ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0&&(data.empty()||BCryptHashData(hash,(PUCHAR)data.data(),(ULONG)data.size(),0)>=0)&&BCryptFinishHash(hash,digest,32,0)>=0;
 if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(algorithm,0);if(!ok)throw std::runtime_error("SHA-256 failed.");
 static const char* hex="0123456789abcdef";std::string text;for(auto b:digest){text+=hex[b>>4];text+=hex[b&15];}return text;
}
inline Bytes protectBytes(const Bytes& data){DATA_BLOB in{(DWORD)data.size(),(BYTE*)data.data()},out{};if(!CryptProtectData(&in,L"UDM",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out))throw std::runtime_error("Cannot protect the form body.");Bytes result(out.pbData,out.pbData+out.cbData);LocalFree(out.pbData);return result;}
inline Bytes unprotectBytes(const Bytes& data){DATA_BLOB in{(DWORD)data.size(),(BYTE*)data.data()},out{};if(!CryptUnprotectData(&in,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out))throw std::runtime_error("The stored form body cannot be read by this Windows account.");Bytes result(out.pbData,out.pbData+out.cbData);LocalFree(out.pbData);return result;}
inline std::string newPostBodyToken(){unsigned char raw[16]{};if(BCryptGenRandom(nullptr,raw,sizeof(raw),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)throw std::runtime_error("Cannot create a form body token.");static const char* hex="0123456789abcdef";std::string text;for(auto b:raw){text+=hex[b>>4];text+=hex[b&15];}return text;}
inline void expireStagedPostBodies(i64 now){ // caller holds stagedPostMutex
 auto& rows=stagedPostBodies();for(auto it=rows.begin();it!=rows.end();){if(now-it->second.started>3600LL*1000){ // epoch() is in milliseconds
  std::error_code ec;fs::remove(postBodyPath(it->second.root,it->first,L".part"),ec);it=rows.erase(it);}else ++it;}
}
inline Json beginStagedPostBody(Manager& m,const Json& message){
 if(!message.is_object()||!message.contains("size")||!message["size"].is_number_integer())throw std::runtime_error("Invalid staged form body size.");
 const auto size=message["size"].get<i64>();if(size<=(i64)MaxBrowserPostBytes||size>(i64)MaxStagedPostBytes)throw std::runtime_error("Staged form bodies must be larger than the inline limit and at most 64 MiB.");
 const auto sha=str(message,"sha256"),type=str(message,"contentType");
 if(!std::regex_match(sha,std::regex("[0-9a-f]{64}"))||type.empty()||type.size()>256||std::any_of(type.begin(),type.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("Invalid staged form body description.");
 const auto token=newPostBodyToken();fs::create_directories(m.root/L"requests");
 {std::ofstream part(postBodyPath(m.root,token,L".part"),std::ios::binary);if(!part)throw std::runtime_error("Cannot stage the form body.");}
 std::lock_guard<std::mutex> lock(stagedPostMutex());const auto now=epoch();expireStagedPostBodies(now);
 if(stagedPostBodies().size()>=8){std::error_code ec;fs::remove(postBodyPath(m.root,token,L".part"),ec);throw std::runtime_error("Too many form bodies are being staged.");}
 stagedPostBodies()[token]={m.root,(size_t)size,0,sha,type,now};
 return {{"ok",true},{"bodyToken",token},{"chunkBytes",(i64)PostBodyChunkBytes}};
}
inline Json appendStagedPostBody(Manager& m,const Json& message){
 const auto token=str(message,"bodyToken");std::lock_guard<std::mutex> lock(stagedPostMutex());auto& rows=stagedPostBodies();auto it=rows.find(token);
 if(it==rows.end()||it->second.root!=m.root)throw std::runtime_error("Unknown form body token.");auto& row=it->second;
 if(!message.contains("offset")||!message["offset"].is_number_integer()||message["offset"].get<i64>()!=(i64)row.received)throw std::runtime_error("Form body chunks must arrive in order.");
 const auto encoded=str(message,"data");if(encoded.empty()||encoded.size()>((PostBodyChunkBytes+2)/3)*4)throw std::runtime_error("Invalid form body chunk.");
 const auto data=unb64(encoded);if(data.empty()||data.size()>PostBodyChunkBytes||b64(data)!=encoded||row.received+data.size()>row.size)throw std::runtime_error("Invalid form body chunk.");
 std::ofstream part(postBodyPath(row.root,token,L".part"),std::ios::binary|std::ios::app);if(!part||!part.write((const char*)data.data(),(std::streamsize)data.size()))throw std::runtime_error("Cannot stage the form body.");part.close();
 row.received+=data.size();return {{"ok",true},{"received",(i64)row.received}};
}
inline Json finishStagedPostBody(Manager& m,const Json& message){
 const auto token=str(message,"bodyToken");StagedPostBody row;
 {std::lock_guard<std::mutex> lock(stagedPostMutex());auto& rows=stagedPostBodies();auto it=rows.find(token);
  if(it==rows.end()||it->second.root!=m.root)throw std::runtime_error("Unknown form body token.");
  if(it->second.received!=it->second.size)throw std::runtime_error("The form body is incomplete.");
  row=it->second;rows.erase(it);}
 const auto part=postBodyPath(row.root,token,L".part"),sealed=postBodyPath(row.root,token,L".bin"),temporary=postBodyPath(row.root,token,L".tmp");
 try{
  const auto text=readText(part,MaxStagedPostBytes);Bytes data(text.begin(),text.end());
  if(data.size()!=row.size||sha256Hex(data)!=row.sha256)throw std::runtime_error("The form body does not match its checksum.");
  const auto stored=protectBytes(data);
  {std::ofstream out(temporary,std::ios::binary|std::ios::trunc);if(!out||!out.write((const char*)stored.data(),(std::streamsize)stored.size()))throw std::runtime_error("Cannot store the form body.");}
  fs::rename(temporary,sealed);std::error_code ec;fs::remove(part,ec);
 }catch(...){std::error_code ec;fs::remove(part,ec);fs::remove(temporary,ec);throw;}
 return {{"ok",true},{"request",{{"method","POST"},{"contentType",row.contentType},{"bodyFile",token},{"bodySize",(i64)row.size},{"bodySha256",row.sha256}}}};
}
// Resolves a stored reference to its bytes, verifying size and checksum; a missing or damaged file asks for recapture.
inline Bytes readStagedPostBody(const Manager& m,const Json& request){
 const auto token=str(request,"bodyFile");if(!postBodyToken(token))throw std::runtime_error("Invalid staged form download body.");
 const auto path=postBodyPath(m.root,token,L".bin");if(!fs::exists(path))throw std::runtime_error("The stored form body is missing. Submit the download form again in your browser.");
 const auto stored=readText(path,MaxStagedPostBytes+65536);auto data=unprotectBytes(Bytes(stored.begin(),stored.end()));
 if((i64)data.size()!=num(request,"bodySize")||sha256Hex(data)!=str(request,"bodySha256"))throw std::runtime_error("The stored form body is damaged. Submit the download form again in your browser.");
 return data;
}
// Deletes sealed bodies no record references and part files no staging owns (left by a crash).
inline void pruneStagedPostBodies(Manager& m){
 const auto directory=m.root/L"requests";std::error_code ec;if(!fs::is_directory(directory,ec))return;
 std::set<std::string> referenced;{Lock lock(m.mutex);for(auto job:m.jobs){try{auto request=readPostRequest(job->data);if(request.contains("bodyFile"))referenced.insert(str(request,"bodyFile"));}catch(const std::exception&){}}}
 std::set<std::string> staging;{std::lock_guard<std::mutex> lock(stagedPostMutex());for(auto& [token,row]:stagedPostBodies())if(row.root==m.root)staging.insert(token);}
 for(auto& entry:fs::directory_iterator(directory,ec)){
  const auto name=utf8(entry.path().filename().wstring());const auto dot=name.find('.');const auto token=name.substr(0,dot),extension=dot==std::string::npos?std::string():name.substr(dot);
  if(!postBodyToken(token))continue;
  if((extension==".bin"&&!referenced.count(token))||(extension==".part"&&!staging.count(token))||extension==".tmp"){std::error_code remove;fs::remove(entry.path(),remove);}
 }
}
}
