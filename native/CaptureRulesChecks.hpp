#pragma once
#include "CaptureRules.hpp"
#include "BrowserSettings.hpp"
static void captureRulesChecks(){
 using namespace udm;
 const std::string text="// comment\r\n\r\nGTSD!http*://turbo.test/download/redirect/*/*\n  S!http://s*.turbo.test/download.php?name=<F:*>&md5=*  \nG#PDF!http://cite*.example.test/viewdoc/download?doi=*&type=pdf\nhttps://~example.test/get?\n";
 auto lines=captureRuleLines(text);
 check(lines.size()==4&&lines[1]=="S!http://s*.turbo.test/download.php?name=<F:*>&md5=*"&&lines[3]=="https://~example.test/get?","Rule lines drop comments and blanks and trim spaces");
 validateCaptureRules(text);check(true,"Valid rule text is accepted");
 validateCaptureRules("");check(captureRuleLines("").empty(),"Empty rule text is valid and yields no rules");
 rejects([&]{validateCaptureRules("Z!http://x.test/a");},"Unknown flags are rejected");
 rejects([&]{validateCaptureRules("G!x.test/a");},"Patterns without a scheme are rejected");
 rejects([&]{validateCaptureRules("http://x.test/a b");},"Patterns with spaces are rejected");
 rejects([&]{validateCaptureRules("http://x.test/<Q:*>");},"Unknown capture tags are rejected");
 rejects([&]{validateCaptureRules("http://x.test/<F:*");},"Unbalanced captures are rejected");
 rejects([&]{validateCaptureRules("G#toolongextensionname!http://x.test/a");},"Over-long literal extensions are rejected");
 rejects([&]{validateCaptureRules("http://x.test/"+std::string(CaptureRuleLineLimit,'a'));},"Over-long lines are rejected");
 {std::string many;for(size_t i=0;i<=CaptureRuleCountLimit;++i)many+="http://x.test/"+std::to_string(i)+"\n";rejects([&]{validateCaptureRules(many);},"Too many rules are rejected");}
 auto prefs=defaultSettings();check(str(prefs,"CaptureRules").empty()&&browserPreferences(prefs)["captureRules"].is_array()&&browserPreferences(prefs)["captureRules"].empty(),"Default settings ship no site rules and serve an empty list");
 prefs["CaptureRules"]=text;validateBrowserSettings(prefs);auto served=browserPreferences(prefs)["captureRules"];
 check(served.size()==4&&served[0]=="GTSD!http*://turbo.test/download/redirect/*/*","Browser preferences serve the active rule lines");
 prefs["CaptureRules"]="G!x.test/a";rejects([&]{validateBrowserSettings(prefs);},"Browser settings validation covers site rules");
}
