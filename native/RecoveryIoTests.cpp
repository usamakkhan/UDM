#include "Backup.hpp"
#include <iostream>
using namespace udm;
static int passed=0,failed=0;
static void check(bool ok,const char* name){std::cout<<(ok?"PASS ":"FAIL ")<<name<<"\n";(ok?passed:failed)++;}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 try{
 auto source=root/L"source.bin";writeBytes(source,Bytes(16*1024*1024,0x5a));auto digest=fileHash(source);Cancel c;
 check(recoveryHash(source,c)==digest,"Cancellable SHA-256 matches existing verifier");
 writeBytes(root/L"empty",{});check(recoveryHash(root/L"empty",c)==fileHash(root/L"empty"),"Empty-file hash matches");
 Cancel hashCancel;i64 hashAt=0;bool caught=false;
 try{recoveryHash(source,hashCancel,[&](i64 done,i64){hashAt=done;if(done>0)hashCancel.stop=true;});}catch(const Cancelled&){caught=true;}
 check(caught&&hashAt>0&&hashAt<16*1024*1024,"Hash verification cancels before reading entire file");
 Cancel copyCancel;i64 copyAt=0;caught=false;
 try{recoveryCopy(source,root/L"cancel.bin",copyCancel,[&](i64 done,i64 total){if(done>0&&done<total){copyAt=done;copyCancel.stop=true;}});}catch(const Cancelled&){caught=true;}
 check(caught&&copyAt>0&&copyAt<16*1024*1024,"Large copy cancels during transfer");
 check(!fs::exists(root/L"cancel.bin"),"Windows removes cancelled copy destination");
 check(fileHash(source)==digest,"Cancelled copy preserves source bytes");
 caught=false;try{recoveryCopy(source,root/L"exception.bin",c,[](i64,i64){throw std::runtime_error("callback-failure");});}catch(const std::exception& e){caught=std::string(e.what())=="callback-failure";}
 check(caught&&!fs::exists(root/L"exception.bin"),"Callback exception is contained and rethrown after Windows returns");
 writeBytes(root/L"existing.bin",Bytes{'k','e','e','p'});caught=false;try{recoveryCopy(source,root/L"existing.bin",c);}catch(...){caught=true;}
 check(caught&&readText(root/L"existing.bin")=="keep","Copy cannot replace an existing destination");
 recoveryCopy(source,root/L"complete.bin",c);check(fileHash(root/L"complete.bin")==digest,"Successful copy retains exact bytes");
 {Manager m(root/L"data");auto job=m.add("https://example.com/large",utf8(root.wstring()),"source.bin");job->data["FileName"]="source.bin";job->data["Status"]="Complete";m.save();Cancel stop;bool mid=false;
  caught=false;try{writeRecoveryImage(m,root/L"backup",stop,[&](const char* phase){if(std::string(phase)=="copy-progress"){mid=true;stop.stop=true;}});}catch(const Cancelled&){caught=true;}
  check(caught&&mid&&!fs::exists(root/L"backup"),"Backup cancellation interrupts a large source copy without publishing");
 }
 atomicText(root/L"results.json",Json{{"passed",passed},{"failed",failed},{"copyCancelledAt",copyAt},{"hashCancelledAt",hashAt}}.dump(2),false);
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
 std::cout<<passed<<" passed, "<<failed<<" failed\n";return failed?1:0;
}
