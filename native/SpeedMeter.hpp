#pragma once
#include <deque>
#include <algorithm>
#include <cmath>
namespace udm {
// A time-weighted five-second payload window; retained parts are never traffic.
struct SpeedMeter {
 struct Sample { double seconds, bytes; };
 std::deque<Sample> samples; double duration=0,total=0,instant=0; long long previous=0;
 void reset(long long counter=0){samples.clear();duration=total=instant=0;previous=counter;}
 double update(long long counter,double seconds,bool downloading){
  if(!downloading||counter<previous){reset(counter);return 0;}
  if(!std::isfinite(seconds)||seconds<=0)return duration>0?total/duration:0;
  double delta=static_cast<double>(counter-previous);previous=counter;instant=delta/seconds;
  samples.push_back({seconds,delta});duration+=seconds;total+=delta;
  while(duration>5&&!samples.empty()){
   auto& first=samples.front();double removed=std::min(first.seconds,duration-5);
   double bytes=first.bytes*removed/first.seconds;total-=bytes;duration-=removed;
   if(removed>=first.seconds)samples.pop_front();else{first.seconds-=removed;first.bytes-=bytes;}
  }
  return duration>0?std::max(0.0,total/duration):0;
 }
};
}
