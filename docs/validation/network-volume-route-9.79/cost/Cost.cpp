#include "FixedVolume.hpp"
#include <iostream>
using namespace udm;
struct Observed:FixedVolumeOps {
 int fallbacks=0,handles=0,closed=0;
 HANDLE open(const wchar_t* path){const auto h=FixedVolumeOps::open(path);if(h&&h!=INVALID_HANDLE_VALUE)++handles;return h;}
 bool volumePath(const wchar_t* path,wchar_t* root,DWORD capacity){++fallbacks;return FixedVolumeOps::volumePath(path,root,capacity);}
 void close(HANDLE h){++closed;FixedVolumeOps::close(h);}
};
bool legacy(const fs::path& path){wchar_t root[MAX_PATH]{};const auto full=fs::absolute(path).wstring();return GetVolumePathNameW(full.c_str(),root,MAX_PATH)&&GetDriveTypeW(root)==DRIVE_FIXED;}
int wmain(int argc,wchar_t**argv){
 if(argc!=3)return 2;
 Json rows=Json::array();
 for(int drive=1;drive<3;++drive){
  const fs::path path(argv[drive]);if(!fs::is_regular_file(path))return 3;
  for(int trial=0;trial<10;++trial){
   double oldMs=0,newMs=0;bool a=false,b=false;Observed ops;
   const auto old=[&]{auto begin=std::chrono::steady_clock::now();a=legacy(path);oldMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();};
   const auto current=[&]{auto begin=std::chrono::steady_clock::now();b=fileOnFixedVolume(path,ops);newMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();};
   if(trial%2){current();old();}else{old();current();}
   if(!a||a!=b||ops.fallbacks||ops.handles!=1||ops.closed!=1)return 4;
   rows.push_back({{"driveIndex",drive},{"trial",trial},{"legacyMs",oldMs},{"currentMs",newMs},{"sameFixedClassification",a==b},{"fallbacks",ops.fallbacks},{"handlesClosed",ops.closed}});
  }
 }
 std::cout<<Json{{"passed",true},{"readOnly",true},{"comparisons",rows},{"scope","Actual helper API calls on existing C and D files, not download or app completion timing"}}.dump(2);return 0;
}
