#include "Core.hpp"
#include <chrono>
#include <iostream>
#include <thread>
#include <atomic>
using namespace udm;
int main(){unsigned passed=0;auto check=[&](bool ok){if(!ok)throw std::runtime_error("Date parsing regression");++passed;};try{
 check(parseDate(Json())==0);check(parseDate("/Date(1700000000123)/")==1700000000123LL);
 check(parseDate("1970-01-01T00:00:00Z")==0);check(parseDate("1970-01-01T01:30:00+01:30")==0);
 check(parseDate("1969-12-31T22:30:00-01:30")==0);check(parseDate("1970-01-01T00:00:00.1234567Z")==123);
 check(parseDate("1970-01-01T00:00:00.1Z")==100);check(parseDate("2000-02-29T00:00:00Z")==951782400000LL);
 for(const Json& bad:std::vector<Json>{42,"bad","2026-01-01T00:00:00+24:00","2026-01-01T00:00:00+00:60"}){bool rejected=false;try{parseDate(bad);}catch(...){rejected=true;}check(rejected);}
 std::atomic<bool> good{true};std::vector<std::thread> workers;
 for(int i=0;i<8;++i)workers.emplace_back([&]{try{for(int n=0;n<200;++n)if(parseDate("1970-01-01T01:30:00.123+01:30")!=123)good=false;}catch(...){good=false;}});
 for(auto& worker:workers)worker.join();check(good);
 const auto start=std::chrono::steady_clock::now();i64 sum=0;for(int i=0;i<10000;++i)sum+=parseDate("1970-01-01T01:30:00.123+01:30");check(sum==1230000);
 std::cout<<"ISO 10000 parse ms: "<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<"\n";
 std::cout<<passed<<" passed, 0 failed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
