#pragma once
#include "Core.hpp"
namespace udm {
// Browser sizes are display hints, never transfer bounds or resume metadata.
inline i64 positiveMediaSize(const Json& value){
 if(!value.is_number_integer())return 0;
 if(value.is_number_unsigned()){auto n=value.get<unsigned long long>();return n>0&&n<=9007199254740991ULL?(i64)n:0;}
 auto n=value.get<i64>();return n>0&&n<=9007199254740991LL?n:0;
}
inline void saveMediaSizeHint(Json& data,const Json& message){
 auto reported=message.find("reportedSize");auto exact=reported==message.end()?0:positiveMediaSize(*reported);
 if(exact)data["ReportedMediaSize"]=exact;else data.erase("ReportedMediaSize");
 auto found=message.find("estimatedSize");auto size=found==message.end()?0:positiveMediaSize(*found);
 if(size)data["EstimatedMediaSize"]=size;else data.erase("EstimatedMediaSize");
}
inline i64 mediaSizeHint(const Json& data){
 if(str(data,"SourceUrl").empty()||str(data,"Status")=="Complete")return 0;
 auto found=data.find("EstimatedMediaSize");return found==data.end()?0:positiveMediaSize(*found);
}
inline i64 reportedMediaSize(const Json& data){
 if(str(data,"SourceUrl").empty()||str(data,"Status")=="Complete")return 0;
 auto found=data.find("ReportedMediaSize");return found==data.end()?0:positiveMediaSize(*found);
}
inline std::string downloadSizeLabel(const Json& data){
 auto size=num(data,"Size",-1);if(size>=0)return bytes(size);
 auto reported=reportedMediaSize(data);if(reported)return bytes(reported);
 auto estimate=mediaSizeHint(data);return estimate?"~ "+bytes(estimate):bytes(-1);
}
}
