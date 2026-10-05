#pragma once
#include "Core.hpp"
namespace udm {
inline i64 rememberedGlobalLimit(const Json& settings){
 auto current=num(settings,"LimitKbps");
 return std::clamp<i64>(current>0?current:num(settings,"GlobalLimitRememberedKbps",1000),1,1000000);
}
inline Json globalLimiterSettings(Json settings,bool enabled,i64 rate){
 if(rate<1||rate>1000000)throw std::runtime_error("Enter a speed limit from 1 to 1,000,000 KB/s.");
 settings["GlobalLimitRememberedKbps"]=rate;settings["LimitKbps"]=enabled?rate:0;return settings;
}
}
