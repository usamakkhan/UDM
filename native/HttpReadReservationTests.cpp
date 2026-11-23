#include "Core.hpp"
#include "HttpReadReservation.hpp"
#include <iostream>
#include <tuple>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2||fs::exists(argv[1]))return 2;
 const auto root=fs::absolute(argv[1]);fs::create_directories(root);
 Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  constexpr i64 k=1024;
  const auto pending=reserveHttpBodyRead(256*k,2048*k,1024*k,true);
  const i64 oldBoundary=256*k+(2048*k-256*k)/2;
  check(oldBoundary<pending.protectedEnd,"The old write-only split fence intersects an already issued read");
  const i64 boundary=pending.protectedEnd+(2048*k-pending.protectedEnd)/2;
  check(boundary>=pending.protectedEnd&&boundary<2048*k,"A helper starts after the complete pending read and still receives a tail");
  check(pending.count==1024*k&&pending.protectedEnd==1280*k,"Pipelined read reservation includes the bytes after the current write");
  const auto shortened=reserveHttpBodyRead(256*k,768*k,1024*k,true);
  check(shortened.count==512*k&&shortened.protectedEnd==768*k,"A split before read admission bounds the actual request to its new owner");
  const auto spent=reserveHttpBodyRead(768*k,768*k,1024*k,true);
  check(!spent.count,"An exhausted shortened assignment cannot issue another body read");
  const auto revoked=reserveHttpBodyRead(1024*k,768*k,1024*k,true);
  check(!revoked.count,"A cursor beyond a shortened boundary cannot reclaim another worker's bytes");
  const auto eof=reserveHttpBodyRead(768*k,768*k,1024*k,false);
  check(eof.count==1&&eof.protectedEnd==768*k,"Declared-body completion still performs a one-byte EOF check without claiming extra data");
  const auto extended=reserveHttpBodyRead(768*k,1536*k,1024*k,true);
  check(extended.count==768*k,"Restoring a refused helper's tail makes those unread bytes available again");
  const auto maximum=std::numeric_limits<i64>::max();
  const auto last=reserveHttpBodyRead(maximum-17,maximum,1024*k,true);
  check(last.count==17&&last.protectedEnd==maximum,"A final read near the signed offset limit does not overflow");
  check(reserveHttpBodyRead(maximum,maximum,1024*k,false).protectedEnd==maximum,"EOF validation at the largest cursor does not add an offset");
  bool matrix=true;
  for(i64 end=0;end<64;++end)for(i64 cursor=0;cursor<80;++cursor)for(size_t window:{1u,3u,16u,128u}){
   const auto result=reserveHttpBodyRead(cursor,end,window,true);
   if(cursor>=end){matrix&=result.count==0;continue;}
   matrix&=result.count>0&&result.count<=window&&result.protectedEnd<=end&&result.protectedEnd>cursor;
  }
  check(matrix,"Boundary matrix never requests bytes outside the live assignment");
  for(const auto bad:std::array<std::tuple<i64,i64,size_t>,3>{{{-1,1,1},{0,-1,1},{0,1,0}}}){
   bool rejected=false;try{reserveHttpBodyRead(std::get<0>(bad),std::get<1>(bad),std::get<2>(bad),true);}catch(const std::invalid_argument&){rejected=true;}
   check(rejected,"Invalid cursor, bound or window is rejected before an HTTP read");
  }
 }catch(const std::exception& problem){error=problem.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
