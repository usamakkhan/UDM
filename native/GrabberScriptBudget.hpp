#pragma once
#include "Core.hpp"
namespace udm {
// Charge bytes even when a body exceeds its limit or a later read fails.
// One look-ahead byte distinguishes an exact-limit body from truncation.
template<class Reader> Bytes readGrabberScript(Reader&& read,size_t& remaining,size_t limit=2*1024*1024){
 Bytes body;char buffer[16384];
 for(;;){const size_t request=std::min({sizeof(buffer),limit-body.size()+1,remaining+1});
  auto count=read(buffer,static_cast<DWORD>(request));if(!count)return body;
  if(count>request)throw std::runtime_error("Invalid script reader length.");
  const bool budgetExceeded=count>remaining;remaining=count>remaining?0:remaining-count;
  if(budgetExceeded||count>limit-body.size())throw std::runtime_error("Script discovery size limit reached.");
  body.insert(body.end(),buffer,buffer+count);
 }
}
}