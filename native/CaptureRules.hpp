#pragma once
// Site capture rules: one rule per line, "[FLAGS][#EXT]!PATTERN" or a bare
// PATTERN over the full request address. The browser extension compiles and
// evaluates them (browser/*/capture-rules.js); the desktop stores, validates
// and serves the text. Flags: G/P request method, S also background requests,
// X only page navigations, D secure requests only, T allow the path extension,
// H hidden, V/A/C stream rules. <F:*> captures a file name, <T:*> an extension.
#include "Core.hpp"
#include <regex>
#include <sstream>
namespace udm {
constexpr size_t CaptureRuleLineLimit=2048,CaptureRuleCountLimit=2000;
inline std::vector<std::string> captureRuleLines(const std::string& text){
 std::vector<std::string> lines;std::string line;std::istringstream input(text);
 while(std::getline(input,line)){
  auto value=trim(line);if(value.empty()||value.rfind("//",0)==0||value.rfind("#",0)==0&&value.find('!')==std::string::npos)continue;
  lines.push_back(value);
 }
 return lines;
}
inline void validateCaptureRules(const std::string& text){
 if(text.size()>CaptureRuleCountLimit*CaptureRuleLineLimit)throw std::runtime_error("Site rules are too long.");
 static const std::regex shape("^(?:([A-Z]+)?(?:#([A-Za-z0-9]{1,16}))?!)?(.+)$"),capture("<([A-Z]):([^>]*)>");
 auto lines=captureRuleLines(text);
 if(lines.size()>CaptureRuleCountLimit)throw std::runtime_error("Use at most "+std::to_string(CaptureRuleCountLimit)+" site rules.");
 for(const auto& line:lines){
  if(line.size()>CaptureRuleLineLimit)throw std::runtime_error("A site rule is longer than "+std::to_string(CaptureRuleLineLimit)+" characters.");
  std::smatch m;if(!std::regex_match(line,m,shape))throw std::runtime_error("Site rule is not \"[FLAGS][#EXT]!PATTERN\": "+line);
  for(char flag:m[1].str())if(std::string("GPTSDVACRHX").find(flag)==std::string::npos)throw std::runtime_error(std::string("Unknown site rule flag ")+flag+": "+line);
  auto pattern=m[3].str();
  if(pattern.find('!')<pattern.find("://"))throw std::runtime_error("Site rule prefix must be flags and an optional #EXT: "+line);
  if(pattern.find("://")==std::string::npos||pattern.find_first_of(" \t")!=std::string::npos)throw std::runtime_error("Site rule pattern needs a scheme and no spaces: "+line);
  for(std::sregex_iterator it(pattern.begin(),pattern.end(),capture),end;it!=end;++it){auto tag=(*it)[1].str();if(tag!="F"&&tag!="T")throw std::runtime_error("Site rule captures must be <F:...> or <T:...>: "+line);}
  if(pattern.find('<')!=std::string::npos&&std::regex_replace(pattern,capture,"").find('<')!=std::string::npos)throw std::runtime_error("Unbalanced capture in site rule: "+line);
 }
}
}
