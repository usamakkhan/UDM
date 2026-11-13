#pragma once
#include <chrono>
#include <cmath>
#include <deque>
#include <optional>

namespace udm {
class ProgressRateHistory {
public:
 using Clock=std::chrono::steady_clock;
 static constexpr auto window=std::chrono::seconds(30);
 static constexpr auto interval=std::chrono::milliseconds(500);
 struct Sample {Clock::time_point time;std::optional<double> rate;};
private:
 std::deque<Sample> samples;
 std::optional<Clock::time_point> latest;
public:
 bool record(Clock::time_point now,std::optional<double> rate){
  if(rate&&(!std::isfinite(*rate)||*rate<0))rate.reset();
  if((samples.empty()&&!rate)||(latest&&now<*latest))return false;
  const bool stateChanged=samples.empty()||samples.back().rate.has_value()!=rate.has_value();
  if(!stateChanged&&latest&&now-*latest<interval)return false;
  latest=now;
  // A zero-duration state transition supersedes the old point. Keeping both
  // makes the renderer draw a rate that the hover/current state has retired.
  if(!samples.empty()&&samples.back().time==now)samples.back().rate=rate;
  else if(stateChanged||
     (rate&&now-samples.back().time>=interval))samples.push_back({now,rate});
  // Keep one boundary anchor so a pause spanning the left edge remains a gap.
  while(samples.size()>1&&samples[1].time<=now-window)samples.pop_front();
  return true;
 }
 const std::deque<Sample>& values()const{return samples;}
 std::deque<Sample> visibleValues()const{
  auto visible=samples;
  const auto begin=end()-window;
  if(visible.empty()||visible.front().time>=begin)return visible;
  // The retained anchor is for interpolation, not an extra visible spike.
  // Never carry it across an inactive interval or into the peak label.
  const auto anchor=visible.front();visible.pop_front();
  if(!visible.empty()&&anchor.rate&&visible.front().rate){
   const double fraction=std::chrono::duration<double>(begin-anchor.time).count()/
    std::chrono::duration<double>(visible.front().time-anchor.time).count();
   visible.push_front({begin,*anchor.rate+(*visible.front().rate-*anchor.rate)*fraction});
  }
  else if(!anchor.rate)visible.push_front({begin,std::nullopt});
  return visible;
 }
 Clock::time_point end()const{return latest.value_or(Clock::time_point{});}
 const Sample* at(Clock::time_point time)const{
  if(samples.empty()||time<end()-window||time>end()||time<samples.front().time)return nullptr;
  const Sample* previous=nullptr;
  for(const auto& sample:samples){
   if(sample.time>time){
    // A null sample starts an inactive interval. Never select a rate across it.
    if(!previous||!previous->rate||!sample.rate)return previous;
    return time-previous->time<=sample.time-time?previous:&sample;
   }
   previous=&sample;
  }
  return previous;
 }
};
}
