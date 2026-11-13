#include "Core.hpp"
#include "ProgressRateHistory.hpp"
#include <iostream>
using namespace udm;
int main(int argc,char** argv){
 Json checks=Json::array();fs::path output=argc>1?fs::path(wide(argv[1])):fs::path{};bool passed=false;std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  using namespace std::chrono;const auto start=ProgressRateHistory::Clock::time_point{}+seconds(100);
  ProgressRateHistory chart;
  check(!chart.record(start,std::nullopt)&&chart.values().empty(),"No synthetic samples are added before a transfer starts");
  check(chart.record(start,100)&&chart.values().size()==1,"First real rate begins history");
  check(!chart.at(start-seconds(1)),"Hover before first measurement has no rate");
  check(!chart.record(start+milliseconds(100),900)&&chart.values().size()==1,"Rapid UI ticks do not add samples or request extra paints");
  chart.record(start+seconds(1),200);chart.record(start+seconds(2),std::nullopt);chart.record(start+seconds(8),400);
  check(chart.at(start+seconds(1))->rate==200,"Measured transfer value is retained");
  check(!chart.at(start+seconds(4))->rate,"Hover in a paused interval cannot borrow a neighboring rate");
  check(!chart.at(start+seconds(7))->rate,"A future resumed sample does not fill the preceding gap");
  check(chart.at(start+seconds(8))->rate==400,"A resumed interval begins at its measured sample");
  check(!chart.at(start+seconds(9)),"Hover cannot read beyond the observed timeline");
  check(!chart.record(start+seconds(7),600)&&chart.end()==start+seconds(8),"Out-of-order ticks cannot rewind the timeline");
  chart.record(start+seconds(9),std::nullopt);chart.record(start+seconds(45),std::nullopt);
  check(chart.end()==start+seconds(45),"The timeline advances throughout a long pause");
  check(chart.values().size()==1&&!chart.values().front().rate,"A long pause expires earlier rate samples while retaining gap state");
  check(!chart.at(start+seconds(20))->rate,"The visible paused window remains an explicit gap after pruning");
  chart.record(start+seconds(46),0);
  check(chart.at(start+seconds(46))->rate==0,"Measured zero rate remains distinct from inactivity");
  chart.record(start+seconds(47),std::numeric_limits<double>::infinity());
  check(!chart.at(start+seconds(47))->rate,"Nonfinite rate is a gap rather than a false plotted value");
  chart.record(start+seconds(48),-1);
  check(!chart.values().back().rate,"Negative rate does not enter the chart");
  ProgressRateHistory bounded;
  for(int i=0;i<10000;++i)bounded.record(start+milliseconds(500*i),i);
  check(bounded.values().size()<=62,"A long download retains only the visible history and one boundary anchor");
  check(!bounded.at(bounded.end()-seconds(31)),"Hover outside retained history has no rate");
  ProgressRateHistory transition;transition.record(start,123);transition.record(start,std::nullopt);
  check(!transition.at(start)->rate,"Same-tick pause replaces the active interpretation immediately");
  check(transition.values().size()==1&&!transition.values().front().rate,"Same-tick pause removes the superseded plotted rate and peak");
  transition.record(start,456);
  check(transition.values().size()==1&&transition.values().front().rate==456,"Same-tick resume retains only the latest drawable state");
  transition.record(start+seconds(1),std::nullopt);
  check(transition.values().size()==2&&transition.values().front().rate==456&&!transition.values().back().rate,
   "A later pause preserves the earlier real measurement and a separate gap boundary");
  ProgressRateHistory burst;
  for(int i=0;i<10000;++i)burst.record(start,i%2?std::optional<double>{}:std::optional<double>{123});
  check(burst.values().size()==1&&!burst.values().front().rate,"Repeated same-timestamp transitions cannot grow history or retain superseded rates");
  ProgressRateHistory clipped;
  clipped.record(start,1000);clipped.record(start+seconds(10),100);clipped.record(start+seconds(35),200);
  const auto visible=clipped.visibleValues();
  check(visible.size()==3&&visible.front().time==start+seconds(5),"Visible history clips the retained anchor to the left boundary");
  check(visible.front().rate==550,"Boundary rate follows the original segment instead of retaining an expired peak");
  check(clipped.values().front().rate==1000,"Clipping leaves the retained source measurement unchanged");
  ProgressRateHistory expired;
  expired.record(start,9000);expired.record(start+seconds(35),std::nullopt);
  const auto paused=expired.visibleValues();
  check(paused.size()==1&&!paused.front().rate,"Expired rate before a pause cannot produce a visible spike or peak");
  expired.record(start+seconds(36),42);
  const auto resumed=expired.visibleValues();
  check(resumed.size()==2&&!resumed.front().rate&&resumed.back().rate==42,"Resume preserves the visible gap without borrowing an expired rate");
  check(ProgressRateHistory{}.visibleValues().empty(),"Empty visible history stays empty");
  const auto inactive=chart.visibleValues();
  check(!inactive.empty()&&!inactive.front().rate,"A long inactive history retains a visible pause marker instead of returning to waiting state");
  ProgressRateHistory exact;exact.record(start,123);exact.record(start+seconds(30),456);
  check(exact.visibleValues().front().time==start&&exact.visibleValues().front().rate==123,"An exact boundary measurement remains unchanged");
  passed=true;
 }catch(const std::exception& e){error=e.what();}
 if(!output.empty()){fs::create_directories(output);atomicText(output/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks}}.dump(2),false);}
 std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
