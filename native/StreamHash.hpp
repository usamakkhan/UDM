#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace udm {
class StreamHash {
 BCRYPT_ALG_HANDLE algorithm=nullptr;
 BCRYPT_HASH_HANDLE hash=nullptr;
public:
 StreamHash(){
  if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)
   throw std::runtime_error("Cannot initialize SHA-256.");
  if(BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)<0){
   BCryptCloseAlgorithmProvider(algorithm,0);
   throw std::runtime_error("Cannot initialize SHA-256.");
  }
 }
 StreamHash(const StreamHash&)=delete;
 StreamHash& operator=(const StreamHash&)=delete;
 ~StreamHash(){
  BCryptDestroyHash(hash);
  BCryptCloseAlgorithmProvider(algorithm,0);
 }
 void update(const void* data,std::size_t count){
  if(count>MAXDWORD||(count&&BCryptHashData(hash,(PUCHAR)data,(ULONG)count,0)<0))
   throw std::runtime_error("SHA-256 failed.");
 }
 std::string finish(){
  BYTE digest[32];
  if(BCryptFinishHash(hash,digest,sizeof(digest),0)<0)throw std::runtime_error("SHA-256 failed.");
  std::ostringstream text;
  for(auto byte:digest)text<<std::hex<<std::setfill('0')<<std::setw(2)<<(int)byte;
  return text.str();
 }
};
}
