#pragma once
#include "ContentSniff.hpp"
template<class Check> void contentSniffUnicodeChecks(Check check){
 using namespace udm;
 auto classify=[](const std::string& s,bool complete=true){return sniffContent(reinterpret_cast<const uint8_t*>(s.data()),s.size(),complete);};
 for(const auto& bytes:std::vector<std::string>{"\xc2\x80","\xdf\xbf","\xe0\xa0\x80","\xed\x9f\xbf","\xee\x80\x80","\xef\xbf\xbf","\xf0\x90\x80\x80","\xf4\x8f\xbf\xbf"})
  check(classify(bytes).verdict==SniffVerdict::Text,"Valid UTF-8 scalar boundary remains text");
 for(const auto& bytes:std::vector<std::string>{"\xc0\xaf","\xc1\xbf","\xe0\x80\xaf","\xe0\x9f\xbf","\xed\xa0\x80","\xed\xbf\xbf","\xf0\x80\x80\xaf","\xf0\x8f\xbf\xbf","\xf4\x90\x80\x80","\xf4\xbf\xbf\xbf","\xf5\x80\x80\x80","\x80","\xe2\x28\xa1"}){
  auto result=classify("Notice: "+bytes);
  check(result.verdict==SniffVerdict::None,"Malformed UTF-8 stays unknown binary");
  check(!webPageInsteadOfFile(result,"zip"),"Malformed UTF-8 does not falsely block a binary download");
 }
 for(const auto& bytes:std::vector<std::string>{"\xc2","\xe2\x82","\xf0\x9f\x92"})
  check(classify("Notice: "+bytes).verdict==SniffVerdict::None,"Incomplete UTF-8 at actual EOF is not text");
 std::string edge(contentSniffWindow-1,'a');edge+="\xe2";
 check(classify(edge,false).verdict==SniffVerdict::Text,"Unfinished sniff window may end inside a valid UTF-8 sequence");
 check(classify(edge+"\x82\xac",true).verdict==SniffVerdict::Text,"Bounded window inside a complete file permits a split character");
 std::string invalidEdge(contentSniffWindow-2,'a');invalidEdge+="\xe0\x80";
 check(classify(invalidEdge,false).verdict==SniffVerdict::None,"Already-invalid partial sequence is rejected at the window boundary");
 check(classify("<html>Access denied</html>").verdict==SniffVerdict::Html,"Ordinary HTML error detection is retained");
}
