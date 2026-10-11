#include "OfflineSite.hpp"
#include "GrabberScriptBudget.hpp"
#include <iostream>
using namespace udm;
int main(){unsigned passed=0;try{
 auto check=[&](bool ok,const char* message){if(!ok)throw std::runtime_error(message);++passed;std::cout<<"PASS "<<message<<std::endl;};
 const std::vector<std::pair<std::string,std::string>> cases={
  {R"(url(images/a\ b.png))","images/a b.png"},
  {R"(url('fonts/\66 ont.woff2'))","fonts/font.woff2"},
  {R"(@import "th\65 me.css";)","theme.css"},
  {R"(url(images/a\)b.png))","images/a)b.png"},
  {R"(url('a\'b.png'))","a'b.png"},
  {R"(url(\000061.png))","a.png"},
  {R"(url(\1f600.png))",utf8(L"\U0001f600.png")},
  {R"(url(\0.png))",utf8(L"\ufffd.png")},
  {R"(url(\d800.png))",utf8(L"\ufffd.png")},
  {R"(url(\110000.png))",utf8(L"\ufffd.png")},
  {"url('a\\\r\nb.png')","ab.png"},
  {"url(\\61\r\n.png)","a.png"}
 };
 for(const auto& item:cases){auto refs=siteReferences(item.first,true);check(refs.size()==1&&refs.front().value==item.second,"CSS asset escape decodes to its actual URL");check(refs.front().offset+refs.front().length<=item.first.size()&&refs.front().css,"Decoded reference keeps valid original-source boundaries");}
 auto source=std::string(R"(a{background:url(images/a\ b.png)} @import "th\65 me.css";)");
 auto rewritten=rewriteSiteDocument(source,"https://example.test/main.css",true,{{"https://example.test/images/a%20b.png","local.png"},{"https://example.test/images/a b.png","local.png"},{"https://example.test/theme.css","theme-local.css"}});
 check(rewritten=="a{background:url(local.png)} @import \"theme-local.css\";","Offline rewriting replaces complete escaped source spans without damaging CSS");
 check(siteReferences("a{background:url(a\\\nb.png)}",true).empty(),"Unquoted backslash-newline does not become a downloadable URL");
 check(siteReferences("url('a\nb.png')",true).empty(),"Literal newline rejects a CSS string URL");
 check(siteReferences("/* url(fake.png) */ a{content:\"url(fake2.png)\";background:url(real.png)}",true).size()==1,"Comments and ordinary strings do not create spurious asset downloads");
 auto inlineRefs=siteReferences(R"html(<div style="background:url(img/a\ b.png)"></div>)html");
 check(inlineRefs.size()==1&&inlineRefs[0].value=="img/a b.png"&&inlineRefs[0].html,"Inline CSS uses the same decoded asset discovery");
 const auto entityStyle=std::string(R"html(<div style="background:url(&quot;img/a\ b.png&quot;)"></div>)html");
 auto entityRefs=siteReferences(entityStyle);
 check(entityRefs.size()==1&&entityRefs[0].value=="img/a b.png","HTML quote entities are decoded before CSS tokenization");
 check(entityStyle.substr(entityRefs[0].offset,entityRefs[0].length)==R"(img/a\ b.png)","Inline entity decoding retains exact raw HTML URL span");
 check(rewriteSiteDocument(entityStyle,"https://example.test/",false,{{"https://example.test/img/a%20b.png","local.png"},{"https://example.test/img/a b.png","local.png"}}).find(R"html(<div style="background:url(&quot;local.png&quot;)"></div>)html")!=std::string::npos,"Inline rewrite preserves original encoded CSS quotes");
 auto ampRefs=siteReferences(R"html(<div style="background:url('a\26 amp;.png')"></div>)html");
 check(ampRefs.size()==1&&ampRefs[0].value=="a&amp;.png","CSS-produced ampersands are not decoded again as HTML entities");
 auto slashRefs=siteReferences(R"html(<div style="background:url('a&#92;20 b.png')"></div>)html");
 check(slashRefs.size()==1&&slashRefs[0].value=="a b.png","HTML numeric backslash entities participate in CSS escapes");
 check(siteReferences(R"html(<div style="background:url('a&#10;b.png')"></div>)html").empty(),"Entity-produced literal newlines invalidate CSS string URLs");
 auto scriptRefs=siteReferences(R"html(<script>window.location='next.html'; image.src="img.png"; window.open('more.html'); location.assign('assign.html'); location.replace('replace.html'); link.href='file.zip';</script>)html",false,false,true);
 scriptRefs.erase(std::remove_if(scriptRefs.begin(),scriptRefs.end(),[](const SiteReference& r){return r.remove;}),scriptRefs.end());
 check(scriptRefs.size()==6&&scriptRefs[0].value=="next.html"&&scriptRefs[1].value=="img.png"&&!scriptRefs[1].page,"Grabber discovers literal script navigation and resource assignments without evaluation");
 auto ignored=siteReferences(R"html(<script>// location='comment.html';
 /* image.src='comment.png'; */ const text="window.open('string.html')";const pattern=/location='regex.html'/; const tmpl=`window.open('template.html')`;location='prefix'+suffix; window.open('prefix'+suffix);</script><script type="application/json">{"src":"data.png"}</script>)html",false,false,true);
 check(std::none_of(ignored.begin(),ignored.end(),[](const SiteReference& r){return !r.remove;}),"Script comments, strings, regex literals, templates, JSON and dynamic expressions are not treated as links");
 auto normal=siteReferences("<script>location='next.html';</script>");
 check(normal.size()==1&&normal[0].remove,"Offline sanitization does not rewrite or execute discovered script expressions");
 auto preserved=siteReferences("<script>location='next.html';</script>",false,true);
 check(preserved.empty(),"Ordinary preserved-script conversion does not alter script source");
 auto sources=siteScriptSources(R"html(<!-- <script src="fake.js"></script> --><script src="a.js?x=1&amp;y=2"></script><script type="application/json" src="data.json"></script><textarea><script src="text.js"></script></textarea><script type="module" src="module.js"></script>)html");
 check(sources==std::vector<std::string>{"a.js?x=1&y=2","module.js"},"External script discovery respects HTML entities, comments, raw text and script type");
 check(scriptReferences("image.src='asset.png';").size()==1,"External JavaScript text uses the same non-executing literal discovery");
 auto readBody=[](const std::string& value,size_t& budget,size_t limit){size_t at=0;return readGrabberScript([&](char* buffer,DWORD requested){auto count=std::min<size_t>(requested,value.size()-at);memcpy(buffer,value.data()+at,count);at+=count;return static_cast<DWORD>(count);},budget,limit);};
 size_t budget=10;bool oversized=false;try{readBody("12345",budget,4);}catch(...){oversized=true;}
 check(oversized&&budget==5,"Oversized script bytes are charged before the body is rejected");
 auto exact=readBody("12345",budget,5);check(exact.size()==5&&budget==0,"Exact-boundary script succeeds while exhausting the shared budget");
 budget=3;oversized=false;try{readBody("12345",budget,10);}catch(...){oversized=true;}
 check(oversized&&budget==0,"Aggregate exhaustion rejects the body and leaves no reusable budget");
 budget=10;unsigned reads=0;bool failedRead=false;try{readGrabberScript([&](char* buffer,DWORD){if(reads++)throw std::runtime_error("network failure");memcpy(buffer,"abc",3);return DWORD(3);},budget,8);}catch(...){failedRead=true;}
 check(failedRead&&budget==7,"A later transport failure does not refund bytes already read");
 std::cout<<passed<<" passed, 0 failed"<<std::endl;return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}}