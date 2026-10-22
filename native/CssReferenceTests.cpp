#include "OfflineSite.hpp"
#include "GrabberScriptBudget.hpp"
#include "ScriptModules.hpp"
#include "GrabberLinks.hpp"
#include <iostream>
#include <chrono>
using namespace udm;
int main(int argc,char** argv){unsigned passed=0;try{
 if(argc==2&&std::string(argv[1])=="--asset-benchmark"){
  std::string body;while(body.size()<1024*1024)body+=R"(consume('ordinary \u0061 text');)";
  Json report=Json::array();for(bool withAsset:{false,true}){
   auto source=body+(withAsset?"image.src='tail.png';":"");std::vector<double> timings;size_t found=0;
   for(unsigned run=0;run<7;++run){auto start=std::chrono::steady_clock::now();found=scriptReferences(source).size();auto stop=std::chrono::steady_clock::now();timings.push_back(std::chrono::duration<double,std::milli>(stop-start).count());}
   std::sort(timings.begin(),timings.end());report.push_back({{"case",withAsset?"late-asset":"ordinary-strings"},{"bytes",source.size()},{"references",found},{"medianMilliseconds",timings[3]}});
  }
  std::cout<<report.dump(2)<<std::endl;return 0;
 }
 if(argc==2&&std::string(argv[1])=="--scan-benchmark"){
  std::string body;body.reserve(1024*1024);while(body.size()<1024*1024)body+="x++;";
  Json report=Json::array();for(bool withImport:{false,true}){
   auto source=body+(withImport?"import('./tail.js');":"");std::vector<double> timings;size_t found=0;
   for(unsigned run=0;run<7;++run){auto start=std::chrono::steady_clock::now();found=moduleReferences(source,false).size();auto stop=std::chrono::steady_clock::now();timings.push_back(std::chrono::duration<double,std::milli>(stop-start).count());}
   std::sort(timings.begin(),timings.end());report.push_back({{"case",withImport?"late-literal-import":"no-module-keywords"},{"bytes",source.size()},{"references",found},{"medianMilliseconds",timings[3]}});
  }
  std::cout<<report.dump(2)<<std::endl;return 0;
 }
 auto check=[&](bool ok,const char* message){if(!ok)throw std::runtime_error(message);++passed;std::cout<<"PASS "<<message<<std::endl;};
 for(const auto& source:std::vector<std::string>{"location = 'target';","location.href = 'target';","nested.image.src = 'target';","link.href = 'target';","window.open('target');","location.assign('target');","location.replace('target');","window.location.assign('target');","window.location.replace('target');","image.src=\v'target';"}){
  auto refs=scriptReferences(source);check(refs.size()==1&&refs[0].value=="target","Asset prefix fast filtering preserves every supported assignment and navigation form");
 }
 check(scriptReferences("consume('escaped \\' quote and \\u0061'); image.src='tail.png';").size()==1,"Skipping ordinary escaped strings preserves the following asset assignment");
 std::string denseScript;while(denseScript.size()<1024*1024)denseScript+="x++;";
 auto lateImports=moduleReferences(denseScript+"import('./tail.js');",false);
 check(lateImports.size()==1&&lateImports[0].value=="./tail.js","A valid-sized dense script retains imports after 262144 tokens");
 auto boundaryImports=moduleReferences(std::string(2*1024*1024-24,';')+"import('./tail.js');",false);
 check(boundaryImports.size()==1&&boundaryImports[0].value=="./tail.js","Module token capacity covers a maximally dense script within the 2 MiB body limit");
 for(const auto& source:std::vector<std::string>{
  "const t=`${import('./inside.js')}`;",
  "const t=`${(()=>{return import('./inside.js');})()}`;",
  "const t=`${`nested ${import('./inside.js')}`}`;",
  "const t=`import('./fake.js') ${import('./inside.js')}`;"}){
  auto refs=moduleReferences(source,false);check(refs.size()==1&&refs[0].value=="./inside.js","Literal imports inside template expressions are discovered without scanning template text");
  check(source.substr(refs[0].offset,refs[0].length)=="./inside.js","Template expression imports retain original source offsets");
 }
 check(rewriteModuleReferences("import('./before.js'); const t=`${import('./inside.js')}`; import('./after.js');","https://example.test/main.js",{{"https://example.test/inside.js","inside%20(1).js"}})=="import('https://example.test/before.js'); const t=`${import('./inside%20(1).js')}`; import('https://example.test/after.js');","Mixed ordinary and interpolated import replacements retain correct source order");
 for(const auto& source:std::vector<std::string>{"import('./lazy.js');","function load(){return import('./lazy.js');}","call(import('./lazy.js'));","import(/* comment */ './lazy.js', {with:{type:'json'}});"}){
  auto refs=moduleReferences(source,false);check(refs.size()==1&&refs[0].value=="./lazy.js","Literal dynamic imports are discovered in classic scripts and nested expressions");
  check(source.substr(refs[0].offset,refs[0].length)=="./lazy.js","Dynamic import source spans preserve surrounding code");
 }
 for(const auto& source:std::vector<std::string>{"object.import('./fake.js');","import(variable);","import('./prefix'+name);","import(`./${name}.js`);","import './classic-invalid.js';"})
  check(moduleReferences(source,false).empty(),"Classic discovery excludes member calls, computed paths and static declarations");
 check(rewriteModuleReferences("import('./lazy.js')","https://example.test/main.js",{{"https://example.test/lazy.js","lazy%20(1).js"}},false)=="import('./lazy%20(1).js')","Literal import calls rewrite to actual saved destinations");
 std::map<std::string,std::string> moduleFiles={{"https://example.test/modules/dep.js","dep (1).js"},{"https://example.test/shared.js","../shared.js"}};
 for(const auto& source:std::vector<std::string>{"const asset=new URL('./icon.svg', import.meta.url);", "fetch(new URL('icon.svg', import.meta.url));", "new URL(`./icon.svg`, import.meta.url);"}){
  auto refs=moduleReferences(source);check(refs.size()==1&&refs[0].asset,"Module-relative URL assets are discovered without being treated as scripts");
 }
 for(const auto& source:std::vector<std::string>{"new URL(path, import.meta.url);", "new URL('./icon.svg'+suffix, import.meta.url);", "new URL(`./${name}.svg`, import.meta.url);", "new URL('./icon.svg', document.baseURI);", "new URL('./icon.svg', import.meta.url + suffix);", "const text=\"new URL('./icon.svg', import.meta.url)\";"})check(moduleReferences(source).empty(),"Computed assets, other bases and ordinary strings are not guessed");
 auto assetTrailing=moduleReferences("new URL(/*asset*/ './icon.svg', import.meta.url,); import('./dep.js');");
 check(assetTrailing.size()==2&&assetTrailing[0].asset&&!assetTrailing[1].asset,"Asset constructors preserve following module imports and support trailing commas");
 check(rewriteModuleReferences("new URL('./icon.svg', import.meta.url)","https://example.test/modules/main.js",{{"https://example.test/modules/icon.svg","icon (1).svg"}})=="new URL('./icon%20(1).svg', import.meta.url)","Module-relative assets rewrite to their actual saved filenames");
 check(rewriteModuleReferences("import './dep.js'; export * from '../shared.js';","https://example.test/modules/main.js",moduleFiles)=="import './dep%20(1).js'; export * from '../shared.js';","Local module imports use URL-relative prefixes and actual saved filenames");
 check(rewriteModuleReferences("import './dep.js#part';","https://example.test/modules/main.js",moduleFiles)=="import './dep%20(1).js#part';","Module conversion retains fragments");
 check(rewriteModuleReferences("import './missing.js';","https://example.test/modules/main.js",moduleFiles)=="import 'https://example.test/modules/missing.js';","An unselected module keeps its original online address");
 check(rewriteModuleReferences("import 'package'; import('./dynamic'+suffix); const message=\"import './dep.js'\";","https://example.test/modules/main.js",moduleFiles)=="import 'package'; import('./dynamic'+suffix); const message=\"import './dep.js'\";","Module conversion preserves unsupported specifiers and ordinary strings");
 auto moduleHtml=rewriteGrabberDocument("<base href='/modules/'><script type='module'>import './dep.js';</script><script>const text=\"import './dep.js'\";</script>","https://example.test/page.html",false,moduleFiles);
 check(moduleHtml=="<script type='module'>import './dep%20(1).js';</script><script>const text=\"import './dep.js'\";</script>","Inline module conversion honors the original HTML base and preserves classic script text");
 check(grabberLinkDocument({{"ContentType","application/javascript; charset=utf-8"},{"FileName","endpoint"}}),"JavaScript response types participate in recoverable link conversion");
 check(!grabberLinkDocument({{"ContentType","application/json"},{"FileName","data.js"}}),"Explicit non-JavaScript data is not converted as a module");
 for(const auto& source:std::vector<std::string>{
  "import './dep.js';", "import value from './dep.js';", "import * as ns from './dep.js';",
  "import {a,b as c} from './dep.js';", "import value, {a} from './dep.js';",
  "import value, * as ns from './dep.js';", "export * from './dep.js';",
  "export * as ns from './dep.js';", "export {a as b} from './dep.js';",
  "import /* comment */ {a} /* comment */ from /* comment */ './dep.js';",
  "import './dep.js' with {type:'json'};", "const n=4/2; import './dep.js';",
  "if(ok) /import '.\\/fake.js'/; import './dep.js';",
  "const re=/import '.\\/fake.js'/; import './dep.js';",
  "const str=\"import './fake.js'\"; import './dep.js';"}){
  auto refs=moduleReferences(source);check(refs.size()==1&&refs[0].value=="./dep.js","Static module declaration resolves its literal specifier without scanning comments or strings");
  check(source.substr(refs[0].offset,refs[0].length)=="./dep.js","Module reference preserves exact source span");
 }
 for(const auto& source:std::vector<std::string>{"import('./dynamic'+suffix);", "import.meta.url;", "import 'bare-package';", "import 'bare/path.js';", "import 'data:text/javascript,1';", "// import './fake.js';", "/* export * from './fake.js'; */", "const t=`import './fake.js'`;", "const o={import:'./fake.js'};", "function f(){import(variable);}", "export const name='./fake.js';"})
  check(moduleReferences(source).empty(),"Non-static and bare module specifiers are not guessed as file URLs");

 for(const auto& source:std::vector<std::string>{"import(`./dep.js`);", "import(`./dep.js`, {with:{type:'json'}});", R"(import(`./\u0064ep.js`);)"}){
  auto refs=moduleReferences(source);check(refs.size()==1&&refs[0].value=="./dep.js","Literal template import resolves its static path");
 }
 for(const auto& source:std::vector<std::string>{"import(`./${name}.js`);", "import(`./dep.js`+suffix);", "import `./dep.js`;", "export * from `./dep.js`;", "const x=`./dep.js`;"})check(moduleReferences(source).empty(),"Computed templates and invalid static template declarations are not guessed");
 auto templateRewrite=rewriteModuleReferences("import(`./dep.js`);","https://example.test/main.js",{{"https://example.test/dep.js","module-${literal}`.js"}});
 check(templateRewrite==R"(import(`./module-\${literal}\`.js`);)","Template rewriting escapes interpolation and delimiter characters in filenames");
 auto escapedTemplate=moduleReferences(R"(import(`./literal-\${name}\`.js`);)");
 check(escapedTemplate.size()==1&&escapedTemplate[0].value=="./literal-${name}`.js","Escaped template interpolation remains a literal path");
 for(const auto& source:std::vector<std::string>{"let n=4; n++ / 2; import('./dep.js');", "let n=4; n-- / 2; import('./dep.js');", "let n=4; const text=`${n++ / 2}`; import('./dep.js');", "let n=4; const text=`${n-- / 2}`; import('./dep.js');"}){
  auto refs=moduleReferences(source);check(refs.size()==1&&refs[0].value=="./dep.js","Postfix division does not swallow subsequent module imports");
 }
 auto escapedModule=moduleReferences(R"(import './\u0064ep.js';)");
 check(escapedModule.size()==1&&escapedModule[0].value=="./dep.js","Module specifier string escapes decode before resolution");
 auto scripts=siteScripts("<script type='module'>import './inline.js';</script><script type='module' src='external.js'>import './ignored.js';</script><script src='classic.js'></script>");
 check(scripts.size()==3&&scripts[0].module&&scripts[0].source.empty()&&scripts[1].module&&scripts[1].body.empty()&&!scripts[2].module,"Module discovery distinguishes inline, linked and classic scripts");
 check(siteScripts("<script type='module' src=''>import './ignored.js';</script>").empty(),"An explicitly empty module source does not turn fallback text into an inline module");
 auto nestedTemplate=moduleReferences("import './first.js'; const t=`${fn(`export * from './fake.js'`)}`;");
 check(nestedTemplate.size()==1&&nestedTemplate[0].value=="./first.js","Nested template expressions do not invent module declarations");
 for(const auto& expression:std::vector<std::string>{
  "const t=`value ${name}`;", "const t=`${fn(`export * from './fake.js'`)}`;",
  "const t=`${({value:'}',other:`${1+2}`}).value}`;", "const t=`${/}/.test('}')}`;",
  "const t=`${(()=>{if(ok) /}/.test('}');return 1;})()}`;",
  "const t=`${1 /* } ` import './fake.js' */ + 2}`;", "const t=`escaped \\${text} and \\` quote`;"}){
  auto refs=moduleReferences(expression+" import './after.js'; import('./lazy-after.js');");
  check(refs.size()==2&&refs[0].value=="./after.js"&&refs[1].value=="./lazy-after.js","Module discovery resumes after nested template expressions without inventing imports");
 }
 check(moduleReferences("const t=`${'unterminated'; import './fake.js';").empty(),"Unterminated template expressions do not expose their text as module declarations");
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
 for(const auto& attrs:std::vector<std::string>{"src='linked.js'","src=''","src","type='module' src='linked.js'","src='' src='linked.js'"}){
  auto refs=siteReferences("<script "+attrs+">image.src='ignored.png';</script>",false,true,true);
  check(std::none_of(refs.begin(),refs.end(),[](const SiteReference& r){return r.value=="ignored.png";}),"Scripts with a src attribute never contribute ignored inline asset expressions");
 }
 for(const auto& tag:std::vector<std::string>{"textarea","title"}){
  const auto markup="<"+tag+"><img src='fake.png'><script>image.src='fake.bin';</script></"+tag+"-suffix><img src='also-fake.png'></"+tag+"><img src='real.png'>";
  auto refs=siteReferences(markup,false,true,true);
  check(refs.size()==1&&refs[0].value=="real.png","Text-only HTML content does not create assets, and closing-tag prefixes do not terminate it");
  check(rewriteGrabberDocument(markup,"https://example.test/",false,{{"https://example.test/real.png","local.png"}})==markup.substr(0,markup.rfind("real.png"))+"local.png'>","Saved conversion preserves literal markup inside text-only HTML elements");
  check(siteScriptSources("<"+tag+"></"+tag+"-suffix><script src='fake.js'></script></"+tag+"><script src='real.js'></script>")==std::vector<std::string>{"real.js"},"Script discovery respects text-only closing-tag boundaries");
 }
 const std::string scriptBoundary=R"html(<script>const text="</script-suffix><img src='fake.png'>";image.src='real.png';</SCRIPT ><img src='after.png'>)html";
 auto boundaryRefs=siteReferences(scriptBoundary,false,true,true);
 check(boundaryRefs.size()==2&&boundaryRefs[0].value=="real.png"&&boundaryRefs[1].value=="after.png","Script closing-tag prefixes do not truncate real asset discovery or expose fake HTML");
 check(rewriteSiteDocument(scriptBoundary,"https://example.test/",false,{{"https://example.test/after.png","local.png"}})==rewriteSiteDocument("<img src='after.png'>","https://example.test/",false,{{"https://example.test/after.png","local.png"}}),"Static archive conversion removes the complete script through its real closing tag");
 auto boundaryScripts=siteScripts(scriptBoundary);
 check(boundaryScripts.size()==1&&boundaryScripts[0].body.find("image.src='real.png'")!=std::string::npos,"Module discovery receives the complete script body after closing-tag lookalikes");
 const std::string styleBoundary=R"html(<style>.a{content:"</style-suffix><img src='fake.png'>"}.b{background:url(real.png)}</STYLE ><img src='after.png'>)html";
 auto styleBoundaryRefs=siteReferences(styleBoundary,false,true);
 check(styleBoundaryRefs.size()==2&&styleBoundaryRefs[0].value=="real.png"&&styleBoundaryRefs[1].value=="after.png","Style closing-tag prefixes preserve subsequent CSS URLs without exposing fake HTML");
 auto normal=siteReferences("<script>location='next.html';</script>");
 check(normal.size()==1&&normal[0].remove,"Offline sanitization does not rewrite or execute discovered script expressions");
 auto preserved=siteReferences("<script>location='next.html';</script>",false,true);
 check(preserved.empty(),"Ordinary preserved-script conversion does not alter script source");
 auto sources=siteScriptSources(R"html(<!-- <script src="fake.js"></script> --><script src="a.js?x=1&amp;y=2"></script><script type="application/json" src="data.json"></script><textarea><script src="text.js"></script></textarea><script type="module" src="module.js"></script>)html");
 check(sources==std::vector<std::string>{"a.js?x=1&y=2","module.js"},"External script discovery respects HTML entities, comments, raw text and script type");
 check(scriptReferences("image.src='asset.png';").size()==1,"External JavaScript text uses the same non-executing literal discovery");
 for(const auto& type:std::vector<std::string>{"text/javascript","TEXT/JAVASCRIPT","Application/JavaScript"}){
  auto typedRefs=siteReferences("<script type=\""+type+"\">image.src='typed.png';</script>",false,false,true);
  check(std::count_if(typedRefs.begin(),typedRefs.end(),[](const SiteReference& r){return !r.remove&&r.value=="typed.png";})==1,"JavaScript MIME casing does not suppress inline URL discovery");
  check(siteScriptSources("<script type=\""+type+"\" src='typed.js'></script>")==std::vector<std::string>{"typed.js"},"Inline and external JavaScript MIME recognition agree");
 }
 for(const auto& type:std::vector<std::string>{"application/ecmascript","application/x-ecmascript","application/x-javascript","text/ecmascript","text/javascript1.0","text/javascript1.1","text/javascript1.2","text/javascript1.3","text/javascript1.4","text/javascript1.5","text/jscript","text/livescript","text/x-ecmascript","text/x-javascript"}){
  auto refs=siteReferences("<script type='"+type+"'>image.src='legacy.png';</script>",false,false,true);
  check(std::any_of(refs.begin(),refs.end(),[](const SiteReference& r){return !r.remove&&r.value=="legacy.png";}),"Recognized legacy JavaScript type discovers inline literal assets");
  check(siteScriptSources("<script type='"+type+"' src='legacy.js'></script>")==std::vector<std::string>{"legacy.js"},"Recognized legacy JavaScript type discovers linked sources");
  check(javascriptMimeEssence(type),"HTTP and HTML JavaScript MIME classifiers agree");
 }
 for(const auto& type:std::vector<std::string>{"application/json","application/ld+json","importmap","speculationrules","text/plain","text/javascript; charset=utf-8","text/javascript-invalid"}){
  auto refs=siteReferences("<script type='"+type+"'>image.src='excluded.png';</script>",false,false,true);
  check(std::none_of(refs.begin(),refs.end(),[](const SiteReference& r){return !r.remove;}),"Data and non-essence script types do not contribute inline assets");
  check(siteScriptSources("<script type='"+type+"' src='excluded.js'></script>").empty(),"Data and non-essence script types do not contribute linked scripts");
 }
 const std::vector<std::pair<std::string,bool>> scriptAttributes={
  {"type=' \tTEXT/JAVASCRIPT\r\n\f'",true},
  {"type=' text/javascript; charset=utf-8 '",false},
  {"type='module'",true},{"type='MoDuLe'",true},{"type=' module '",false},
  {"language='JavaScript1.2'",true},{"language='VBScript'",false},
  {"language=' javascript '",false},{"language=''",true},
  {"language='VBScript' type='text/javascript'",true},
  {"language='JavaScript' type='application/json'",false},
  {"language='VBScript' type=''",true},{"language='VBScript' type",true},
  {"type='text/javascript' type='application/json'",true},
  {"type='application/json' type='text/javascript'",false}
 };
 for(const auto& item:scriptAttributes){
  auto refs=siteReferences("<script "+item.first+">image.src='classified.png';</script>",false,false,true);
  check(std::any_of(refs.begin(),refs.end(),[](const SiteReference& r){return !r.remove&&r.value=="classified.png";})==item.second,"HTML script classification respects type, whitespace and language precedence");
  check((siteScriptSources("<script "+item.first+" src='classified.js'></script>").size()==1)==item.second,"Linked script classification matches inline script classification");
 }
 auto jsonRefs=siteReferences("<script type='APPLICATION/JSON'>image.src='not-script.png';</script>",false,false,true);
 check(std::none_of(jsonRefs.begin(),jsonRefs.end(),[](const SiteReference& r){return !r.remove;}),"Mixed-case JSON script data stays excluded from discovery");
 const std::vector<std::pair<std::string,std::string>> scriptEscapes={
  {R"(image.src='\x61sset.png';)","asset.png"},
  {R"(image.src='\u0061sset.png';)","asset.png"},
  {R"(location='\u{61}sset.html';)","asset.html"},
  {R"(image.src='\uD83D\uDE00.png';)","\xf0\x9f\x98\x80.png"},
  {R"(image.src='\u{1f600}.png';)","\xf0\x9f\x98\x80.png"},
  {"image.src='as\\\r\nset.png';","asset.png"}
 };
 for(const auto& item:scriptEscapes){auto refs=scriptReferences(item.first);check(refs.size()==1&&refs[0].value==item.second,"JavaScript escaped URL matches its decoded literal");}
 for(const auto& invalidScript:std::vector<std::string>{R"(image.src='\xZZ.png';)",R"(image.src='\u12Q4.png';)",R"(image.src='\u{}.png';)",R"(image.src='\u{110000}.png';)",R"(image.src='\uD800.png';)",R"(image.src='\uDC00.png';)",R"(image.src='\x00.png';)",R"(image.src='\x01.png';)",R"(image.src='\x7f.png';)",R"(image.src='\u000a.png';)",R"(image.src='\x61'+suffix;)"}){
  check(scriptReferences(invalidScript).empty(),"Malformed, control-character and dynamic escaped URLs stay excluded");
 }
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
