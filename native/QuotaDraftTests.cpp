#include "OptionsModel.hpp"
#include <iostream>
using namespace udm;
int main(){unsigned passed=0;auto check=[&](bool ok){if(!ok)throw std::runtime_error("Quota draft regression");++passed;};try{
 Json original={{"QuotaMb",512},{"QuotaHours",6},{"Other","keep"}};auto prefs=original;
 collectQuotaDraft(prefs,false,"bad","bad");check(num(prefs,"QuotaMb")==0&&num(prefs,"QuotaLastMb")==512&&num(prefs,"QuotaHours")==6);
 auto disabled=prefs;collectQuotaDraft(prefs,false,"1024","12");check(prefs==disabled);
 for(const auto& pair:std::vector<std::pair<std::string,std::string>>{{"8abc","6"},{"8.5","6"},{"0","6"},{"100000001","6"},{"512","0"},{"512","169"},{"512","6abc"},{"512","999999999999999999999"},{"","6"}}){auto draft=original;bool rejected=false;try{collectQuotaDraft(draft,true,pair.first,pair.second);}catch(...){rejected=true;}check(rejected&&draft==original);}
 collectQuotaDraft(prefs,true," 1024 ","12");check(num(prefs,"QuotaMb")==1024&&num(prefs,"QuotaLastMb")==1024&&num(prefs,"QuotaHours")==12&&str(prefs,"Other")=="keep");
 collectQuotaDraft(prefs,true,"100000000","168");check(num(prefs,"QuotaMb")==100000000&&num(prefs,"QuotaHours")==168);
 std::cout<<passed<<" passed, 0 failed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
