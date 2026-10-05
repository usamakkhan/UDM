#pragma once
#include "Core.hpp"
#include <bcrypt.h>
#include <fstream>
#include <sstream>
#include <iomanip>
namespace udm {
using RecoveryProgress=std::function<void(i64,i64)>;
struct RecoveryCopyContext{
 const Cancel& cancel;const RecoveryProgress& progress;std::exception_ptr error;
 static DWORD CALLBACK report(LARGE_INTEGER total,LARGE_INTEGER done,LARGE_INTEGER,LARGE_INTEGER,DWORD,DWORD,HANDLE,HANDLE,LPVOID opaque)noexcept{
  auto& self=*static_cast<RecoveryCopyContext*>(opaque);
  try{self.cancel.check();if(self.progress)self.progress(done.QuadPart,total.QuadPart);self.cancel.check();return PROGRESS_CONTINUE;}
  catch(...){self.error=std::current_exception();return PROGRESS_CANCEL;}
 }
};
inline void recoveryCopy(const fs::path& source,const fs::path& target,const Cancel& cancel,const RecoveryProgress& progress={}){
 cancel.check();RecoveryCopyContext context{cancel,progress,{}};
 auto ok=CopyFileExW(source.c_str(),target.c_str(),RecoveryCopyContext::report,&context,nullptr,COPY_FILE_FAIL_IF_EXISTS);
 auto error=GetLastError();if(context.error)std::rethrow_exception(context.error);cancel.check();
 if(!ok)throw std::runtime_error("Cannot copy recovery file (Windows error "+std::to_string(error)+").");
}
inline std::string recoveryHash(const fs::path& path,const Cancel& cancel,const RecoveryProgress& progress={}){
 cancel.check();BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Cannot initialize SHA-256.");
 try{
  if(BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)<0)throw std::runtime_error("Cannot initialize SHA-256.");
  auto total=fs::file_size(path);std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("Cannot open recovery file.");
  char buffer[65536];i64 done=0;
  while(input){cancel.check();input.read(buffer,sizeof(buffer));auto count=input.gcount();
   if(count&&BCryptHashData(hash,reinterpret_cast<PUCHAR>(buffer),(ULONG)count,0)<0)throw std::runtime_error("SHA-256 failed.");
   done+=count;if(progress)progress(done,(i64)total);cancel.check();
  }
  if(!input.eof())throw std::runtime_error("Cannot read recovery file.");
  BYTE digest[32];if(BCryptFinishHash(hash,digest,32,0)<0)throw std::runtime_error("SHA-256 failed.");
  BCryptDestroyHash(hash);hash=nullptr;BCryptCloseAlgorithmProvider(algorithm,0);algorithm=nullptr;
  std::ostringstream output;for(auto byte:digest)output<<std::hex<<std::setfill('0')<<std::setw(2)<<(int)byte;return output.str();
 }catch(...){if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);throw;}
}
}
