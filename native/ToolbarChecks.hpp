#pragma once
#include "ToolbarModel.hpp"
static void toolbarChecks(const fs::path& root){
 check(toolbarLayout(Json::object())==defaultToolbarLayout(),"Toolbar defaults contain the eleven download commands");
 Json legacy={{"ToolbarOrder",{10,0,1,10,999,-1}},{"ToolbarHidden",{5,9}}};
 check(toolbarLayout(legacy)==std::vector<int>({10,0,1,2,3,4,6,7,8}),"Legacy toolbar order/visibility migrates without losing remaining commands");
 Json modern=legacy;saveToolbarLayout(modern,{10,-1,0,-1,-1,8});
 check(toolbarLayout(modern)==std::vector<int>({10,-1,0,-1,-1,8}),"Toolbar keeps repeated separators and chosen command order");
 check(modern["ToolbarHidden"].size()==8&&modern["ToolbarOrder"][0]==10,"Toolbar layout also writes compatible legacy order and hidden commands");
 modern["ToolbarLayout"]=Json::array();check(toolbarLayout(modern).empty(),"An explicitly empty toolbar is preserved");
 for(auto bad:{Json{0,0},Json{-2},Json{11},Json{0.5},Json{true},Json{"0"},Json::object()}){
  check(!validToolbarLayout(bad),"Invalid toolbar entries cannot become button commands");
 }
 check(!validToolbarLayout(Json::array({UINT64_MAX})),"Unsigned toolbar values cannot wrap into separator markers");
 Json many=Json::array();for(int i=0;i<65;++i)many.push_back(-1);check(!validToolbarLayout(many),"Toolbar layout bounds prevent unbounded separator allocation");many.erase(many.end()-1);check(validToolbarLayout(many),"Maximum-length separator layout is representable");
 for(auto pair:std::vector<std::pair<std::string,Json>>{{"ToolbarStyle","Bad"},{"ToolbarSize","Giant"},{"HideToolbar","false"},{"ToolbarSkin","relative.tbi"},{"ToolbarSkin","C:\\icons.png"},{"ToolbarSkin",2}}){
  Json p;p[pair.first]=pair.second;rejects([&]{validateToolbarSettings(p);},"Toolbar preferences reject unsupported or malformed fields");
 }
 Manager manager(root/L"toolbar-settings");auto prefs=manager.state["Settings"];saveToolbarLayout(prefs,{0,-1,8,10});prefs["ToolbarStyle"]="Icons only";prefs["ToolbarSize"]="Small";manager.setSettings(prefs);
 {Manager reopened(manager.root);check(toolbarLayout(reopened.state["Settings"])==std::vector<int>({0,-1,8,10}),"Toolbar separators and order survive catalog restart");}
 // Small independent BMP layout: twelve cells, two rows; bottom-up 24-bit data.
 Bytes bmp(54+12*3*2,0);auto put=[&](size_t at,uint32_t n,int count){for(int i=0;i<count;++i)bmp[at+i]=(BYTE)(n>>(8*i));};
 put(0,0x4d42,2);put(2,(uint32_t)bmp.size(),4);put(10,54,4);put(14,40,4);put(18,12,4);put(22,2,4);put(26,1,2);put(28,24,2);
 for(size_t i=54;i<bmp.size();i+=3){bmp[i]=19;bmp[i+1]=37;bmp[i+2]=83;}bmp[54]=bmp[55]=bmp[56]=192;
 auto decoded=decodeToolbarBitmap(bmp);check(decoded.width==12&&decoded.height==2&&decoded.pixels[0]==19&&decoded.pixels[2]==83,"Toolbar BMP preserves BGR pixels and row orientation");
 check(decoded.pixels[12*4+3]==0&&decoded.pixels[3]==255,"Toolbar's documented grey key becomes transparent");
 auto malformed=bmp;malformed.resize(60);rejects([&]{decodeToolbarBitmap(malformed);},"Truncated toolbar pixels rejected");
 malformed=bmp;malformed[30]=1;rejects([&]{decodeToolbarBitmap(malformed);},"Unsupported BMP compression rejected");
 malformed=bmp;malformed[18]=13;rejects([&]{decodeToolbarBitmap(malformed);},"Strip width must divide into twelve published cells");
 auto folder=root/L"toolbar-fixture";fs::create_directories(folder);{std::ofstream file(folder/L"icons.bmp",std::ios::binary);file.write((char*)bmp.data(),bmp.size());}
 auto path=folder/L"test.tbi";atomicText(path,"v=2\nname=Fixture\nlarge=icons.bmp\nlargeHot=icons.bmp\nsmall=icons.bmp\nhdpi=icons.bmp\n",false);
 auto skin=loadToolbarSkin(path);check(skin.name=="Fixture"&&skin.images.size()==4,"Published TBI fields load independent BMP strips");
 check(toolbarSkinGroup(skin,"Small",192)=="small"&&toolbarSkinGroup(skin,"Large",144)=="hdpi"&&toolbarSkinGroup(skin,"Large",120)=="large","Toolbar chooses small and high-DPI variants according to size");
 for(const auto& text:std::vector<std::string>{"v=1\nname=Test\nlarge=icons.bmp","v=2\nname=Test\nlarge=../icons.bmp","v=2\nname=Test\nlarge=C:\\icons.bmp","v=2\nname=Test\nlarge=icons.bmp\nLARGE=icons.bmp","v=2\nname=Test","v=2\nname=\nlarge=icons.bmp"}){
  atomicText(path,text,false);rejects([&]{loadToolbarSkin(path);},"Malformed toolbar descriptor or image path rejected");
 }
 atomicText(path,"v=3\nname=High DPI\nlarge=icons.bmp\nhdpi=icons.bmp",false);check(loadToolbarSkin(path).images.size()==2,"Installed reference version-three fields are accepted");
 auto files=toolbarSkinFiles({folder,folder});check(files.size()==1&&files[0]==path,"Toolbar discovery de-duplicates repeated folders");
}

