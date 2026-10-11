#include "GrabberRendered.hpp"
#include <iostream>
using namespace udm;
int main(){int passed=0;try{
 Json page={{"version",1},{"url","https://example.test/page"},{"baseUrl","https://example.test/assets/"},{"title","Page"},{"documentId","document"},{"tabId",1},{"frameId",0},{"visited",12},{"truncated",false},{"references",Json::array({Json{{"url","https://example.test/file.zip"},{"kind","link"},{"description","File"}}})}};
 auto check=[&](bool ok){if(!ok)throw std::runtime_error("Rendered validation assertion failed");++passed;};
 check(validateRenderedGrabberSnapshot(page,str(page,"url"))==page);
 auto reject=[&](Json value){try{validateRenderedGrabberSnapshot(value,str(page,"url"));}catch(const std::exception&){++passed;return;}throw std::runtime_error("Invalid page was accepted");};
 for(auto change:std::vector<Json>{{{"version",2}},{{"url","https://other.test/page"}},{{"documentId",""}},{{"tabId",-1}},{{"frameId",1}},{{"visited",10001}},{{"visited",0}},{{"truncated",1}},{{"cookies","unexpected"}},{{"baseUrl","file:///C:/secret"}}}){auto copy=page;copy.update(change);reject(copy);}
 for(const auto& address:{"file:///C:/secret","javascript:alert(1)","https://user:password@example.test/file"}){auto copy=page;copy["references"][0]["url"]=address;reject(copy);}
 auto copy=page;copy["references"][0]["kind"]="unknown";reject(copy);copy=page;copy["references"][0]["description"]=std::string(4097,'x');reject(copy);
 copy=page;for(int i=0;i<2000;++i)copy["references"].push_back(page["references"][0]);reject(copy);
 copy=page;copy["references"].push_back(page["references"][0]);check(validateRenderedGrabberSnapshot(copy,str(page,"url"))["references"].size()==1);
 copy=page;copy["truncated"]=true;check(yes(validateRenderedGrabberSnapshot(copy,str(page,"url")),"truncated"));
 std::cout<<"PASS "<<passed<<" native rendered-page validation checks\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
