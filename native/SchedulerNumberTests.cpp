#include "SchedulerNumber.hpp"
#include <iostream>
#include <vector>
using namespace udm;
int main(){unsigned checks=0;try{
 auto check=[&](bool ok){if(!ok)throw std::runtime_error("Scheduler number regression");++checks;};
 for(auto bounds:std::vector<std::pair<long long,long long>>{{1,16},{0,10},{0,20},{1,86400},{15,3600}}){
  auto low=bounds.first,high=bounds.second;
  check(schedulerNumber(std::to_string(low),low,high,"test")==low);
  check(schedulerNumber(" \t"+std::to_string(high)+"\r\n",low,high,"test")==high);
  check(schedulerNumber("000"+std::to_string(low),low,high,"test")==low);
  for(auto input:std::vector<std::string>{""," ","3abc","1.5","1e2","+3","-1","3 4",std::to_string(high+1),"999999999999999999999999999999"}){
   bool rejected=false;try{schedulerNumber(input,low,high,"test");}catch(const std::runtime_error&){rejected=true;}check(rejected);
  }
  if(low>0){bool rejected=false;try{schedulerNumber(std::to_string(low-1),low,high,"test");}catch(const std::runtime_error&){rejected=true;}check(rejected);}
 }
 std::cout<<checks<<" passed, 0 failed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
