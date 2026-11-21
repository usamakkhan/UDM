#include "Core.hpp"
#include <iostream>
using namespace udm;
int main(){Json rows=Json::array();for(int prefix:{138,139}){const auto input=std::string(prefix,'a')+"🙂"+std::string(30,'b')+".mp4";Json row={{"prefixAsciiUnits",prefix},{"inputIsValidUtf8",utf8(wide(input))==input},{"requestedExtension",".mp4"}};try{const auto result=safeName(input);row["accepted"]=true;row["result"]=result;}catch(const std::exception& e){row["accepted"]=false;row["error"]=e.what();}rows.push_back(row);}std::cout<<Json{{"engineVersion","8.56"},{"cases",rows}}.dump(2)<<"\n";}
