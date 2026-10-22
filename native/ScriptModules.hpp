#pragma once
#include "OfflineSite.hpp"
#include <string_view>
namespace udm {
// Skip template text and nested interpolation expressions without treating
// their string contents as declarations. The caller resumes after the literal.
inline size_t moduleTemplateEnd(const std::string& code,size_t at,std::vector<std::pair<size_t,size_t>>* expressions=nullptr){
 struct Frame{bool text;unsigned braces=0;bool regex=true;std::string previous;std::vector<bool> parens;size_t start=0;};
 std::vector<Frame> stack{{true}};
 auto word=[](unsigned char c){return std::isalnum(c)||c=='_'||c=='$'||c>=128;};
 while(at<code.size()&&!stack.empty()){
  auto& frame=stack.back();char c=code[at++];
  if(frame.text){
   if(c=='\\'&&at<code.size()){++at;continue;}
   if(c=='`'){stack.pop_back();if(!stack.empty())stack.back().regex=false;continue;}
   if(c=='$'&&at<code.size()&&code[at]=='{'){++at;if(stack.size()>=256)return std::string::npos;stack.push_back({false});stack.back().start=at;}
   continue;
  }
  if(std::isspace(static_cast<unsigned char>(c)))continue;
  if(c=='/'&&at<code.size()&&code[at]=='/'){auto end=code.find('\n',at+1);at=end==std::string::npos?code.size():end+1;continue;}
  if(c=='/'&&at<code.size()&&code[at]=='*'){auto end=code.find("*/",at+1);if(end==std::string::npos)return end;at=end+2;continue;}
  if(c=='`'){if(stack.size()>=256)return std::string::npos;stack.push_back({true});continue;}
  if(c=='\''||c=='"'){
   bool closed=false;while(at<code.size()){char n=code[at++];if(n=='\\'&&at<code.size()){++at;continue;}if(n==c){closed=true;break;}}
   if(!closed)return std::string::npos;frame.regex=false;frame.previous="literal";continue;
  }
  if(c=='/'&&frame.regex){
   bool bracket=false,closed=false;while(at<code.size()){char n=code[at++];if(n=='\\'&&at<code.size()){++at;continue;}if(n=='[')bracket=true;else if(n==']')bracket=false;else if(n=='/'&&!bracket){closed=true;break;}else if(n=='\n'||n=='\r')break;}
   if(!closed)return std::string::npos;while(at<code.size()&&word(static_cast<unsigned char>(code[at])))++at;frame.regex=false;frame.previous="literal";continue;
  }
  if(word(static_cast<unsigned char>(c))){auto begin=at-1;while(at<code.size()&&word(static_cast<unsigned char>(code[at])))++at;frame.previous=code.substr(begin,at-begin);const auto& w=frame.previous;frame.regex=w=="return"||w=="throw"||w=="case"||w=="delete"||w=="void"||w=="typeof"||w=="yield"||w=="await"||w=="else"||w=="do";continue;}
  // Prefix ++/-- still expect an operand; postfix ++/-- still end one.
  // Preserve the preceding context instead of treating their final sign as
  // a binary operator that would allow a regular-expression literal.
  if((c=='+'||c=='-')&&at<code.size()&&code[at]==c){++at;frame.previous=std::string(2,c);continue;}
  if(c=='{')++frame.braces;
  else if(c=='}'){if(!frame.braces){if(expressions&&stack.size()==2&&expressions->size()<10000)expressions->push_back({frame.start,at-1-frame.start});stack.pop_back();continue;}--frame.braces;}
  if(c=='('){frame.parens.push_back(frame.previous=="if"||frame.previous=="while"||frame.previous=="for"||frame.previous=="with"||frame.previous=="switch"||frame.previous=="catch");frame.regex=true;}
  else if(c==')'){frame.regex=!frame.parens.empty()&&frame.parens.back();if(!frame.parens.empty())frame.parens.pop_back();}
  else frame.regex=c!=']'&&c!='.';
  frame.previous=std::string(1,c);
 }
 return stack.empty()?at:std::string::npos;
}
// Static declarations, literal import() calls and literal module-relative URL
// assets. No script is evaluated. Bare import specifiers still need import maps;
// bare URL-constructor paths resolve relative to import.meta.url instead.
struct ModuleReference{size_t offset=0,length=0;std::string value;bool asset=false;};
inline std::vector<ModuleReference> moduleReferences(const std::string& code,bool declarations=true,unsigned templateDepth=0){
 // The fetcher already limits scripts to 2 MiB. Avoid building a token
 // vector at all when neither applicable module keyword occurs.
 if(code.find("import")==std::string::npos&&(!declarations||code.find("export")==std::string::npos))return {};
 if(code.size()>UINT32_MAX)return {};
 struct Token{uint32_t start,end;char kind;};std::vector<Token> tokens;std::vector<ModuleReference> nested;
 auto word=[](unsigned char c){return std::isalnum(c)||c=='_'||c=='$'||c>=128;};
 auto text=[&](const Token& token){return std::string_view(code).substr(token.start,token.end-token.start);};
 bool regexAllowed=true;std::vector<bool> controlParens;std::string previous;
 for(size_t i=0;i<code.size()&&tokens.size()<2*1024*1024;){
  if(std::isspace(static_cast<unsigned char>(code[i]))){++i;continue;}
  if(code.compare(i,2,"//")==0){auto end=code.find('\n',i+2);i=end==std::string::npos?code.size():end+1;continue;}
  if(code.compare(i,2,"/*")==0){auto end=code.find("*/",i+2);i=end==std::string::npos?code.size():end+2;continue;}
  size_t start=i;char c=code[i++],kind='p';
  if(c=='`'){
   std::vector<std::pair<size_t,size_t>> expressions;auto end=moduleTemplateEnd(code,i,&expressions);if(end==std::string::npos)break;
   if(templateDepth<32)for(const auto& span:expressions){if(nested.size()>=10000)break;for(auto ref:moduleReferences(code.substr(span.first,span.second),false,templateDepth+1)){if(nested.size()>=10000)break;ref.offset+=span.first;nested.push_back(std::move(ref));}}
   i=end;kind=expressions.empty()?'t':'o';regexAllowed=false;
  }
  else if(c=='\''||c=='"'){
   kind='s';bool closed=false;
   while(i<code.size()){char next=code[i++];if(next=='\\'&&i<code.size()){++i;continue;}
    if(next==c){closed=true;break;}}
   if(!closed)break;regexAllowed=false;
  }else if(c=='/'&&regexAllowed){
   bool bracket=false,closed=false;while(i<code.size()){char next=code[i++];if(next=='\\'&&i<code.size()){++i;continue;}if(next=='[')bracket=true;else if(next==']')bracket=false;else if(next=='/'&&!bracket){closed=true;break;}else if(next=='\n'||next=='\r')break;}
   if(!closed)break;while(i<code.size()&&word(static_cast<unsigned char>(code[i])))++i;kind='o';regexAllowed=false;
  }else if(word(static_cast<unsigned char>(c))){
   while(i<code.size()&&word(static_cast<unsigned char>(code[i])))++i;kind='w';auto value=code.substr(start,i-start);
   regexAllowed=value=="return"||value=="throw"||value=="case"||value=="delete"||value=="void"||value=="typeof"||value=="yield"||value=="await"||value=="else"||value=="do";
  }else if((c=='+'||c=='-')&&i<code.size()&&code[i]==c){++i;}
  else if(c=='('){controlParens.push_back(previous=="if"||previous=="while"||previous=="for"||previous=="with"||previous=="switch"||previous=="catch");regexAllowed=true;}
  else if(c==')'){regexAllowed=!controlParens.empty()&&controlParens.back();if(!controlParens.empty())controlParens.pop_back();}
  else regexAllowed=c!=']'&&c!='.';
  tokens.push_back({static_cast<uint32_t>(start),static_cast<uint32_t>(i),kind});previous=kind=='w'?code.substr(start,i-start):std::string(1,c);
 }
 std::vector<ModuleReference> refs;int braces=0,parens=0,brackets=0;
 auto is=[&](size_t at,const char* value){return at<tokens.size()&&text(tokens[at])==value;};
 auto name=[&](size_t at){return at<tokens.size()&&tokens[at].kind=='w';};
 for(size_t i=0;i<tokens.size()&&refs.size()<10000;++i){
  auto value=text(tokens[i]);
  if(value=="{"){++braces;continue;}if(value=="}"){braces=std::max(0,braces-1);continue;}
  if(value=="("){++parens;continue;}if(value==")"){parens=std::max(0,parens-1);continue;}
  if(value=="["){++brackets;continue;}if(value=="]"){brackets=std::max(0,brackets-1);continue;}
  const bool asset=value=="new"&&is(i+1,"URL")&&is(i+2,"(");
  if((value!="import"&&value!="export"&&!asset)||(i&&is(i-1,".")))continue;
  const bool dynamic=value=="import"&&is(i+1,"(");
  if(!dynamic&&!asset&&(!declarations||braces||parens||brackets))continue;
  size_t at=i+(asset?3:1);bool importing=value=="import";
  if(at==tokens.size())break;
  if(asset){
   if(!is(at+1,",")||!is(at+2,"import")||!is(at+3,".")||!is(at+4,"meta")||!is(at+5,".")||!is(at+6,"url")||(!is(at+7,")")&&!(is(at+7,",")&&is(at+8,")"))))continue;
  }
  else if(dynamic){++at;if(at>=tokens.size()||(tokens[at].kind!='s'&&tokens[at].kind!='t')||(!is(at+1,")")&&!is(at+1,",")))continue;}
  else if(!(importing&&tokens[at].kind=='s')){
   if(importing&&name(at)){++at;if(is(at,","))++at;else if(!is(at,"from"))continue;}
   if(is(at,"*")){++at;if(is(at,"as")){++at;if(!name(at)&&(importing||at>=tokens.size()||tokens[at].kind!='s'))continue;++at;}else if(importing)continue;}
   else if(is(at,"{")){
    ++at;while(at<tokens.size()&&!is(at,"}")){if(tokens[at].kind!='w'&&tokens[at].kind!='s'&&!is(at,","))break;++at;}
    if(!is(at,"}"))continue;++at;
   }
   if(!is(at,"from"))continue;++at;
  }
  if(at>=tokens.size()||(tokens[at].kind!='s'&&!((dynamic||asset)&&tokens[at].kind=='t')))continue;
  const auto& token=tokens[at];auto literal=std::string(text(token));
  if(token.kind=='t'){
   // A template without substitutions has a known value. Normalize its
   // delimiter escapes for the existing non-evaluating string decoder.
   std::string quoted="\"";
   for(size_t n=1;n+1<literal.size();++n){char c=literal[n];
    if(c=='\\'&&n+2<literal.size()){char escaped=literal[++n];if(escaped!='`'&&escaped!='$')quoted+='\\';quoted+=escaped;}
    else{if(c=='"')quoted+='\\';quoted+=c;}
   }
   literal=quoted+'"';
  }
  auto decoded=scriptReferences("image.src="+literal+";");
  if(decoded.size()!=1)continue;auto url=decoded[0].value;
  auto scheme=lower(url);if(!asset&&url.rfind("./",0)!=0&&url.rfind("../",0)!=0&&url.rfind("/",0)!=0&&scheme.rfind("https://",0)!=0&&scheme.rfind("http://",0)!=0)continue;
  refs.push_back({token.start+1,token.end-token.start-2,url,asset});
  if(dynamic||asset)++parens; // The opening parenthesis was consumed with the specifier.
  i=at;
 }
 for(auto& ref:nested)refs.push_back(std::move(ref));
 std::sort(refs.begin(),refs.end(),[](const ModuleReference& a,const ModuleReference& b){return a.offset<b.offset;});
 if(refs.size()>10000)refs.resize(10000);
 return refs;
}
inline std::string rewriteModuleReferences(const std::string& source,const std::string& base,const std::map<std::string,std::string>& files,bool declarations=true){
 auto refs=moduleReferences(source,declarations);auto output=source;
 for(auto it=refs.rbegin();it!=refs.rend();++it)try{
  Url resolved(combineUrl(base,it->value));if(resolved.scheme!="http"&&resolved.scheme!="https")continue;
  auto key=resolved.origin+resolved.path+resolved.query;auto file=files.find(key);
  std::string value=file==files.end()?key:file->second;
  // A local filename without ./ is a bare module specifier, not a URL.
  if(file!=files.end()&&value.rfind("./",0)!=0&&value.rfind("../",0)!=0&&value.rfind("/",0)!=0&&value.find(':')==std::string::npos)value="./"+value;
  auto fragment=it->value.find('#');if(fragment!=std::string::npos)value+=it->value.substr(fragment);
  std::string encoded;const char* hex="0123456789ABCDEF";
  const bool templateLiteral=it->offset&&source[it->offset-1]=='`';
  for(unsigned char c:value){if(templateLiteral&&(c=='`'||c=='$')){encoded+='\\';encoded+=static_cast<char>(c);continue;}if(c<=32||c==39||c==34||c==92||c==60||c==62){encoded+='%';encoded+=hex[c>>4];encoded+=hex[c&15];}else encoded+=static_cast<char>(c);}
  output.replace(it->offset,it->length,encoded);
 }catch(const std::exception&){}
 return output;
}
}
