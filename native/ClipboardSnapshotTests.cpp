#include "ClipboardSnapshot.hpp"
#include <iostream>
#include <stdexcept>
using namespace udm;
int main(){
 int passed=0;
 auto check=[&](bool ok,const char* name){if(!ok)throw std::runtime_error(name);++passed;std::cout<<"PASS "<<name<<"\n";};
 try{
  ClipboardSnapshot cache;unsigned sequence=1;int reads=0;std::optional<std::string> value="https://example.test/file.zip";
  auto counter=[&]{return sequence;};auto reader=[&]{++reads;return value;};
  check(cache.read(counter,reader)==value&&reads==1,"Initial clipboard content is read");
  for(int i=0;i<1000;++i)cache.read(counter,reader);
  check(reads==1,"One thousand unchanged refreshes perform no additional clipboard reads");
  ++sequence;value="https://example.test/new.zip";
  check(cache.read(counter,reader)==value&&reads==2,"Changed content is read immediately");
  ++sequence;value.reset();check(!cache.read(counter,reader),"Locked clipboard is not treated as empty");
  value="https://example.test/retry.zip";check(cache.read(counter,reader)==value&&reads==4,"Failed read is retried with the same counter");
  ++sequence;value="";check(cache.read(counter,reader)==value,"Successful empty clipboard clears prior content");
  cache.read(counter,reader);check(reads==5,"Empty content is cached");
  sequence=0;cache.read(counter,reader);cache.read(counter,reader);check(reads==7,"Unavailable counter falls back to reading");
  sequence=100;cache.read(counter,[&]{++reads;++sequence;return value;});cache.read(counter,reader);check(reads==9,"Counter changing during read cannot cache stale content");
  cache.reset();cache.read(counter,reader);check(reads==10,"Re-enabling monitoring forces a fresh read");
  sequence=1;cache.read(counter,reader);check(reads==11,"Counter wraparound is detected by inequality");
  std::cout<<passed<<" passed, 0 failed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
