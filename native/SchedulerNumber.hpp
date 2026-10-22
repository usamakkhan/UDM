#pragma once
#include <string>
#include <stdexcept>
namespace udm {
inline long long schedulerNumber(const std::string& input,long long minimum,long long maximum,const char* field){
 auto first=input.find_first_not_of(" \t\r\n"),last=input.find_last_not_of(" \t\r\n");
 auto invalid=[&]{return std::runtime_error(std::string("Enter a whole number from ")+std::to_string(minimum)+" to "+std::to_string(maximum)+" for "+field+".");};
 if(first==std::string::npos)throw invalid();
 long long value=0;
 for(auto i=first;i<=last;++i){unsigned char c=input[i];if(c<'0'||c>'9')throw invalid();auto digit=c-'0';if(value>maximum/10||(value==maximum/10&&digit>maximum%10))throw invalid();value=value*10+digit;}
 if(value<minimum)throw invalid();return value;
}
}
