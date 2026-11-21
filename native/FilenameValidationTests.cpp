#include "Core.hpp"
#include <iostream>
using namespace udm;
static Json checks=Json::array();
static void check(bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);}
int wmain(int argc,wchar_t**argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);std::string error;
 try{
  // Each process reaches its first safeName calls together, before any Manager.
  constexpr int workers=12;std::mutex gate;std::condition_variable ready;int waiting=0;bool release=false;std::vector<int> outcomes(workers);std::vector<std::thread> threads;
  for(int i=0;i<workers;++i)threads.emplace_back([&,i]{std::unique_lock<std::mutex> lock(gate);++waiting;ready.notify_all();ready.wait(lock,[&]{return release;});lock.unlock();try{bool ok=true;for(int n=0;n<64;++n)ok&=safeName(n%2?"../CON.txt":"folder\\lpt9.mp4")== (n%2?"_CON.txt":"_lpt9.mp4");outcomes[i]=ok?1:-1;}catch(...){outcomes[i]=-1;}});
  {std::unique_lock<std::mutex> lock(gate);ready.wait(lock,[&]{return waiting==workers;});release=true;}ready.notify_all();for(auto& thread:threads)thread.join();for(auto result:outcomes)check(result==1,"Simultaneous first-use filename normalization returns exact results");
  std::vector<std::pair<std::string,std::string>> cases={{"","download.bin"},{".","download.bin"},{"..","download.bin"},{" ","download.bin"},{"../normal.bin","normal.bin"},{"C:\\folder\\file.bin","file.bin"},{"folder/","download.bin"},{"  file name.bin. ","file name.bin"},{"a:b?*.mp4","a_b__.mp4"},{"a<b>c|d\"e.bin","a_b_c_d_e.bin"},{"CON","_CON"},{"con.txt","_con.txt"},{"cOn.a.b","_cOn.a.b"},{"COM0","COM0"},{"COM10.txt","COM10.txt"},{"LPT0","LPT0"},{"LPT10.mp4","LPT10.mp4"},{"console.txt","console.txt"},{"auxiliary.bin","auxiliary.bin"},{"normal.CON","normal.CON"},{"xcon","xcon"},{"con_.txt","con_.txt"},{"con..txt","_con..txt"},{" con ","_con"},{"日本語🙂.mp4","日本語🙂.mp4"},{"résumé—video.mkv","résumé—video.mkv"},{std::string("a\0b",3),"a_b"},{std::string(160,'a'),std::string(160,'a')},{std::string(161,'a'),std::string(140,'a')},{std::string(200,'a')+".mp4",std::string(140,'a')+".mp4"},{std::string(200,'b')+"."+std::string(40,'z'),std::string(140,'b')+"."+std::string(18,'z')},{std::string(139,'a')+"🙂"+std::string(30,'b'),std::string(139,'a')}};
  for(const auto* device:{"con","prn","aux","nul"})for(const std::string suffix:{"",".bin"})cases.emplace_back(std::string(device)+suffix,"_"+std::string(device)+suffix);
  for(int i=1;i<=9;++i)for(const std::string prefix:{"COM","lpt"}){const auto name=prefix+std::to_string(i)+".bin";cases.emplace_back(name,"_"+name);}
  for(int c=1;c<32;++c)cases.emplace_back("a"+std::string(1,(char)c)+"b","a_b");
  // Independent expected boundaries: keep a complete pair or omit it wholly.
  for(const std::string symbol:{"🙂","𝄞",u8"\U00010000",u8"\U0010FFFF"}){
   cases.emplace_back(std::string(137,'a')+symbol+std::string(30,'b')+".mp4",std::string(137,'a')+symbol+"b.mp4");
   cases.emplace_back(std::string(138,'a')+symbol+std::string(30,'b')+".mp4",std::string(138,'a')+symbol+".mp4");
   cases.emplace_back(std::string(139,'a')+symbol+std::string(30,'b')+".mp4",std::string(139,'a')+".mp4");
   cases.emplace_back(std::string(140,'a')+symbol+std::string(30,'b')+".mp4",std::string(140,'a')+".mp4");
   cases.emplace_back(std::string(141,'a')+symbol+std::string(30,'b')+".mp4",std::string(140,'a')+".mp4");
   cases.emplace_back(std::string(158,'a')+symbol,std::string(158,'a')+symbol);
   cases.emplace_back(std::string(159,'a')+symbol,std::string(140,'a'));
   cases.emplace_back(std::string(180,'a')+"."+std::string(16,'z')+symbol+std::string(20,'q'),std::string(140,'a')+"."+std::string(16,'z')+symbol);
   cases.emplace_back(std::string(180,'a')+"."+std::string(17,'z')+symbol+std::string(20,'q'),std::string(140,'a')+"."+std::string(17,'z'));
   cases.emplace_back(std::string(180,'a')+"."+std::string(18,'z')+symbol+std::string(20,'q'),std::string(140,'a')+"."+std::string(18,'z'));
   cases.emplace_back(std::string(139,'a')+symbol+std::string(30,'b')+"."+std::string(17,'z')+symbol+std::string(20,'q'),std::string(139,'a')+"."+std::string(17,'z'));
   cases.emplace_back("CON."+std::string(134,'a')+symbol+std::string(30,'b')+".mp4","_CON."+std::string(134,'a')+".mp4");
  }
  for(const auto& item:cases){const auto normalized=safeName(item.first);check(normalized==item.second,"Filename normalization preserves explicit device, path, Unicode, control and truncation rules");check(safeName(normalized)==normalized,"Normalized filename is stable on a second admission");}
  for(const std::string invalid:{std::string(1,(char)0xff),std::string("\xc0\xaf",2),std::string("\xed\xa0\x80",3),std::string("\xf0\x9f\x99",3)}){bool refused=false;try{safeName(invalid);}catch(const std::exception&){refused=true;}check(refused,"Malformed UTF-8 filenames remain refused");}
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
