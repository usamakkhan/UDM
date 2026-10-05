#pragma once
#include "Core.hpp"
#include <fstream>
#include <set>
#include <array>
#include <algorithm>
#include <sstream>
namespace udm {
constexpr int ToolbarCommandCount=12,ToolbarSkinColumns=12;
inline const wchar_t* toolbarLabel(int index){static const wchar_t* values[]={L"Add URL",L"Resume",L"Stop",L"Stop All",L"Delete",L"Delete completed",L"Options",L"Scheduler",L"Start Queue",L"Stop Queue",L"Grabber",L"Tell a friend"};return index>=0&&index<ToolbarCommandCount?values[index]:L"Separator";}
inline const char* toolbarAsset(int index){static const char* values[]={"add","resume","stop","stopall","remove","complete","settings","schedule","startqueue","stopqueue","grabber","link"};return values[index];}
inline std::vector<int> defaultToolbarLayout(){std::vector<int> result;for(int i=0;i<ToolbarCommandCount;++i)result.push_back(i);return result;}
inline bool validToolbarLayout(const Json& input){if(!input.is_array()||input.size()>64)return false;std::set<int> seen;for(const auto& item:input){if(!item.is_number_integer()||(item.is_number_unsigned()&&item.get<uint64_t>()>=ToolbarCommandCount))return false;auto n=item.get<i64>();if(n< -1||n>=ToolbarCommandCount||(n>=0&&!seen.insert((int)n).second))return false;}return true;}
inline std::vector<int> toolbarLayout(const Json& prefs){
 auto found=prefs.find("ToolbarLayout");if(found!=prefs.end()&&validToolbarLayout(*found))return found->get<std::vector<int>>();
 std::vector<int> order;std::set<int> seen,hidden;
 for(const char* key:{"ToolbarOrder","ToolbarHidden"}){auto values=prefs.find(key);if(values==prefs.end()||!values->is_array())continue;for(const auto& item:*values){if(!item.is_number_integer())continue;auto n=item.get<i64>();if(n<0||n>=ToolbarCommandCount)continue;if(std::string(key)=="ToolbarHidden")hidden.insert((int)n);else if(seen.insert((int)n).second)order.push_back((int)n);}}
 for(int i=0;i<ToolbarCommandCount;++i)if(seen.insert(i).second)order.push_back(i);
 order.erase(std::remove_if(order.begin(),order.end(),[&](int i){return hidden.count(i)!=0;}),order.end());return order;
}
inline void saveToolbarLayout(Json& prefs,const std::vector<int>& layout){
 if(!validToolbarLayout(Json(layout)))throw std::runtime_error("Invalid toolbar button layout.");
 prefs["ToolbarLayout"]=layout;prefs["ToolbarOrder"]=Json::array();prefs["ToolbarHidden"]=Json::array();std::set<int> shown;
 for(int i:layout)if(i>=0){shown.insert(i);prefs["ToolbarOrder"].push_back(i);}
 for(int i=0;i<ToolbarCommandCount;++i)if(!shown.count(i)){prefs["ToolbarOrder"].push_back(i);prefs["ToolbarHidden"].push_back(i);}
}
inline void validateToolbarSettings(const Json& prefs){
 if(prefs.contains("ToolbarStyle")&&(!prefs["ToolbarStyle"].is_string()||(str(prefs,"ToolbarStyle")!="Icons and text"&&str(prefs,"ToolbarStyle")!="Icons only"&&str(prefs,"ToolbarStyle")!="Text only")))throw std::runtime_error("Choose a supported toolbar display mode.");
 if(prefs.contains("ToolbarSize")&&(!prefs["ToolbarSize"].is_string()||(str(prefs,"ToolbarSize")!="Small"&&str(prefs,"ToolbarSize")!="Medium"&&str(prefs,"ToolbarSize")!="Large")))throw std::runtime_error("Choose a supported toolbar size.");
 if(prefs.contains("HideToolbar")&&!prefs["HideToolbar"].is_boolean())throw std::runtime_error("Invalid toolbar visibility setting.");
 if(prefs.contains("ToolbarLayout")&&!validToolbarLayout(prefs["ToolbarLayout"]))throw std::runtime_error("Invalid toolbar button layout.");
 if(prefs.contains("ToolbarSkin")){if(!prefs["ToolbarSkin"].is_string())throw std::runtime_error("Invalid toolbar skin path.");auto name=str(prefs,"ToolbarSkin");if(name.size()>8192||(!name.empty()&&(!fs::path(wide(name)).is_absolute()||lower(utf8(fs::path(wide(name)).extension().wstring()))!=".tbi"||name.find('\0')!=std::string::npos)))throw std::runtime_error("Choose a toolbar information (.tbi) file.");}
}
struct ToolbarBitmap {int width=0,height=0;Bytes pixels;};
inline Bytes toolbarFileBytes(const fs::path& path,size_t limit){
 std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)throw std::runtime_error("Cannot read toolbar file: "+utf8(path.filename().wstring()));
 auto size=in.tellg();if(size<0||(uint64_t)size>limit)throw std::runtime_error("Toolbar file is too large.");Bytes data((size_t)size);in.seekg(0);if(!data.empty())in.read((char*)data.data(),(std::streamsize)data.size());if(!in)throw std::runtime_error("Cannot read the complete toolbar file.");return data;
}
inline ToolbarBitmap decodeToolbarBitmap(const Bytes& bytes){
 auto value=[&](size_t at,size_t count)->uint32_t{if(at>bytes.size()||count>bytes.size()-at)throw std::runtime_error("Truncated toolbar bitmap.");uint32_t n=0;for(size_t i=0;i<count;++i)n|=(uint32_t)bytes[at+i]<<(8*i);return n;};
 if(bytes.size()<54||bytes.size()>16*1024*1024||value(0,2)!=0x4d42)throw std::runtime_error("Choose an uncompressed toolbar BMP image.");
 auto dib=value(14,4),offset=value(10,4),declared=value(2,4),width=value(18,4);int32_t signedHeight=(int32_t)value(22,4);auto bits=value(28,2);
 if((dib!=40&&dib!=108&&dib!=124)||offset<14+dib||offset>bytes.size()||value(26,2)!=1||(bits!=24&&bits!=32)||value(30,4)!=0||width==0||width>ToolbarSkinColumns*256||width%ToolbarSkinColumns||signedHeight==0||signedHeight< -256||signedHeight>256)throw std::runtime_error("Unsupported toolbar bitmap dimensions or encoding.");
 int height=abs(signedHeight);size_t stride=((size_t)width*bits+31)/32*4,total=stride*height;
 if(total>bytes.size()-offset||(declared&&(declared>bytes.size()||declared<offset+total)))throw std::runtime_error("Truncated toolbar bitmap pixels.");
 bool alpha=false;if(bits==32)for(int y=0;y<height&&!alpha;++y)for(uint32_t x=0;x<width;++x)if(bytes[offset+y*stride+x*4+3]){alpha=true;break;}
 ToolbarBitmap result{(int)width,height,Bytes((size_t)width*height*4)};
 for(int y=0;y<height;++y)for(uint32_t x=0;x<width;++x){auto at=offset+(signedHeight<0?y:height-1-y)*stride+x*(bits/8),out=((size_t)y*width+x)*4;auto b=bytes[at],g=bytes[at+1],r=bytes[at+2];result.pixels[out]=b;result.pixels[out+1]=g;result.pixels[out+2]=r;result.pixels[out+3]=(b==192&&g==192&&r==192)?0:alpha?bytes[at+3]:255;}
 return result;
}
struct ToolbarSkin {std::string name;fs::path path;std::map<std::string,ToolbarBitmap> images;};
inline ToolbarSkin loadToolbarSkin(const fs::path& path){
 if(!path.is_absolute()||lower(utf8(path.extension().wstring()))!=".tbi")throw std::runtime_error("Choose a toolbar information (.tbi) file.");
 auto bytes=toolbarFileBytes(path,16384);std::string text(bytes.begin(),bytes.end());if(text.rfind("\xef\xbb\xbf",0)==0)text.erase(0,3);
 if(utf8(wide(text))!=text||text.find('\0')!=std::string::npos)throw std::runtime_error("Toolbar information must be UTF-8 text.");
 std::map<std::string,std::string> fields;std::istringstream lines(text);std::string line;
 while(std::getline(lines,line)){line=trim(line);if(line.empty()||line[0]==';'||line[0]=='#')continue;auto eq=line.find('=');if(eq==std::string::npos)throw std::runtime_error("Invalid toolbar information line.");auto key=lower(trim(line.substr(0,eq))),v=trim(line.substr(eq+1));if(!fields.emplace(key,v).second)throw std::runtime_error("Duplicate toolbar information field.");}
 if(fields["v"]!="2"&&fields["v"]!="3")throw std::runtime_error("This toolbar information version is not supported.");
 ToolbarSkin skin;skin.path=path;skin.name=fields["name"];if(skin.name.empty()||wide(skin.name).size()>128||std::any_of(skin.name.begin(),skin.name.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("The toolbar needs a valid name.");
 if(fields["large"].empty())throw std::runtime_error("The toolbar needs a large image strip.");
 for(const char* key:{"large","largehot","largedisabled","small","smallhot","smalldisabled","hdpi","hdpihot","hdpidisabled"}){
  auto name=fields[key];if(name.empty())continue;fs::path relative(wide(name));
  if(relative.has_root_path()||relative.filename()!=relative||name.find_first_of("/\\:")!=std::string::npos||lower(utf8(relative.extension().wstring()))!=".bmp"||safeName(name)!=name)throw std::runtime_error("Toolbar images must be BMP files beside the information file.");
  skin.images[key]=decodeToolbarBitmap(toolbarFileBytes(path.parent_path()/relative,16*1024*1024));
 }
 for(const char* group:{"large","small","hdpi"})for(const char* suffix:{"hot","disabled"}){auto variant=skin.images.find(std::string(group)+suffix);if(variant==skin.images.end())continue;auto normal=skin.images.find(group);if(normal==skin.images.end()||normal->second.width!=variant->second.width||normal->second.height!=variant->second.height)throw std::runtime_error("Toolbar state images must have matching dimensions.");}
 return skin;
}
inline std::string toolbarSkinGroup(const ToolbarSkin& skin,const std::string& size,int dpi){
 if(size=="Small"&&skin.images.count("small"))return "small";if(size=="Large"&&dpi>=144&&skin.images.count("hdpi"))return "hdpi";return "large";
}
inline std::vector<fs::path> toolbarSkinFiles(const std::vector<fs::path>& folders){
 std::vector<fs::path> result;std::set<std::wstring> seen;
 for(const auto& folder:folders){std::error_code e;for(auto it=fs::directory_iterator(folder,e);!e&&it!=fs::directory_iterator();it.increment(e)){if(result.size()>=64)break;auto path=it->path();if(it->is_regular_file(e)&&lower(utf8(path.extension().wstring()))==".tbi"&&seen.insert(wide(lower(utf8(path.lexically_normal().wstring())))).second)result.push_back(path);}}
 std::sort(result.begin(),result.end());return result;
}
}
