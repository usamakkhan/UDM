#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>
namespace udm {
// Bounded, attempt-local bytes copied only after successful positioned writes.
// The transfer serializes capture with credited progress; readers run after join.
class WrittenPrefix {
 std::vector<unsigned char> bytes,covered;size_t filled=0;
public:
 void reset(size_t limit)noexcept{filled=0;try{bytes.assign(limit,0);covered.assign(limit,0);}catch(...){bytes.clear();covered.clear();}}
 void capture(const unsigned char* input,size_t count,int64_t offset)noexcept{if(offset<0||static_cast<uint64_t>(offset)>=bytes.size()||!input)return;const auto begin=static_cast<size_t>(offset),take=std::min(count,bytes.size()-begin);std::copy_n(input,take,bytes.data()+begin);for(size_t i=begin;i<begin+take;++i)if(!covered[i]){covered[i]=1;++filled;}}
 bool read(int64_t total,std::vector<unsigned char>& output)const{if(total<0||bytes.empty())return false;const auto need=static_cast<size_t>(std::min<uint64_t>(static_cast<uint64_t>(total),bytes.size()));if(filled<need||!std::all_of(covered.begin(),covered.begin()+need,[](auto value){return value!=0;}))return false;output.assign(bytes.begin(),bytes.begin()+need);return true;}
};
}
