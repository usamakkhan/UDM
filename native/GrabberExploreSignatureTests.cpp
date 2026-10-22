#include "GrabberExploreSignature.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace udm;
int main(){int checks=0;try{
 auto check=[&](bool ok){if(!ok)throw std::runtime_error("Exploration checkpoint identity regression");++checks;};
 const std::string policy="scope|links:1",ticket="rendered-page-ticket";
 for(const auto& old:std::vector<std::string>{"","short",std::string(300,'a')}){
  for(const auto& next:std::vector<std::string>{"new",std::string(600,'b')}){
   auto initial=grabberExploreSignature(policy,true,old,ticket);
   auto checkpoint=grabberExploreSignature(policy,true,next,ticket);
   check(checkpoint==policy+"|browser:"+next+"|rendered:"+ticket);
   check(checkpoint!=initial);
   check(checkpoint!=grabberExploreSignature(policy,true,next,"different-page-ticket"));
   check(checkpoint!=grabberExploreSignature("different-policy",true,next,ticket));
   // Historical suffix replacement corrupts the rendered identity, including
   // when the original cookie snapshot is empty or larger than the ticket.
   check(initial.substr(0,initial.size()-old.size())+next!=checkpoint);
  }
 }
 check(grabberExploreSignature(policy,false,"ignored",ticket)==policy+"|rendered:"+ticket);
 check(grabberExploreSignature(policy,true,"cookies","")==policy+"|browser:cookies");
 check(grabberExploreSignature(policy,false,"","")==policy);
 std::cout<<checks<<" passed, 0 failed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
