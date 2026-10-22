#include "SortKeyCache.hpp"
#include <iostream>
#include <stdexcept>
using namespace udm;
int main(){unsigned passed=0,reads=0;auto check=[&](bool value){if(!value)throw std::runtime_error("Sort key cache regression");++passed;};try{
 SortKeyCache cache;auto load=[&]{++reads;return std::string("first");};
 cache.begin();check(cache.get("job","encrypted-v1",load)=="first"&&reads==1);cache.finish();
 bool unchanged=true;for(unsigned i=0;i<1000;++i){cache.begin();unchanged&=cache.get("job","encrypted-v1",load)=="first";cache.finish();}check(unchanged);check(reads==1);
 cache.begin();check(cache.get("job","encrypted-v2",[&]{++reads;return std::string("edited");})=="edited"&&reads==2);cache.finish();
 cache.begin();cache.finish();cache.begin();cache.get("job","encrypted-v2",load);cache.finish();check(reads==3);
 cache.clear();cache.begin();cache.get("job","encrypted-v2",load);cache.finish();check(reads==4);
 cache.begin();bool failed=false;try{cache.get("job","bad",[]{throw std::runtime_error("bad source");return std::string();});}catch(...){failed=true;}check(failed);cache.begin();check(cache.get("job","encrypted-v2",load)=="first"&&reads==4);cache.finish();
 // A refresh abandoned after an edit must still have the last committed keys.
 cache.begin();cache.get("job","aborted",[]{return std::string("uncommitted");});
 cache.begin();check(cache.get("job","encrypted-v2",load)=="first"&&reads==4);cache.finish();
 // Large keys stay in the same allocation across unchanged refreshes.
 const std::string revision(65536,'r'),key(65536,'k');unsigned largeReads=0;
 cache.begin();const auto* original=&cache.get("large",revision,[&]{++largeReads;return key;});cache.finish();
 bool stable=true;for(unsigned i=0;i<100;++i){cache.begin();const auto& value=cache.get("large",revision,[&]{++largeReads;return key;});stable&=&value==original&&value==key;cache.finish();}
 check(stable&&largeReads==1);
 cache.begin();check(cache.get("large",revision+"changed",[]{return std::string("new key");})=="new key");cache.finish();
 cache.begin();cache.finish();cache.begin();cache.get("large",revision,[&]{++largeReads;return key;});cache.finish();check(largeReads==2);
 std::cout<<passed<<" passed, 0 failed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
