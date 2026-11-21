#include "BatchRefreshProjection.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;const auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");manager.save();const auto before=manager.snapshot(),settings=before["Settings"];const auto disk=readText(manager.root/L"state.json");
  std::vector<std::string> urls{"http://127.0.0.1:1/first.bin","http://127.0.0.1:1/other.bin"};std::vector<BrowserBatchRow> browser;for(const auto& url:urls){BrowserBatchRow source;source.address=url;source.observedSize=4096;source.observedType="application/octet-stream";browser.push_back(source);}
  const auto initial=projectBatchRows(urls,&browser,{},settings).rows;auto rows=initial;
  const auto prepare=[&](std::optional<int> changed=std::nullopt,const std::set<int>& added=std::set<int>{}){return projectBatchRefresh(*captureBatchRefresh(initial,added,changed,settings),&browser,{},settings);};
  auto result=prepare();check(batchRefreshMatches(result,rows,{},{}),"An unchanged complete request cohort matches its prepared refresh");
  rows[0].metadata={{"Status","Error"}};rows[0].folder="owned-folder";rows[0].description="owned-description";rows[0].checked=true;rows[0].selectionTouched=true;
  check(applyBatchRefresh(result,rows,{},{}),"A matching refresh publishes its prepared metadata");
  check(num(rows[0].metadata,"Size")==4096&&rows[0].browserName=="first.bin","A matching refresh publishes real size and naming evidence");
  check(rows[0].folder=="owned-folder"&&rows[0].description=="owned-description"&&rows[0].checked&&rows[0].selectionTouched,"Applying metadata preserves current destination description and explicit selection");
  rows=initial;result=prepare();rows[1].address="http://127.0.0.1:1/replaced.bin";rows[0].metadata={{"Marker","untouched"}};const auto old=rows[0].metadata;
  check(!batchRefreshMatches(result,rows,{},{}),"A superseding address rejects the older request cohort");
  check(!applyBatchRefresh(result,rows,{},{} )&&rows[0].metadata==old&&num(result.values[0].row.metadata,"Size")==4096,"A rejected cohort neither partially changes earlier rows nor consumes their prepared evidence");
  rows=initial;result=prepare();rows[0].name="new-user-name.mp4";check(!applyBatchRefresh(result,rows,{},{}),"A superseding explicit name rejects old derived naming evidence");
  rows=initial;result=prepare();rows[0].protectedRequestOverrides="owned-new-protected-context";check(!applyBatchRefresh(result,rows,{},{}),"A superseding protected request context rejects the old response evidence");
  rows=initial;result=prepare();Headers different{{"Authorization","Basic owned-new-context"}};check(!applyBatchRefresh(result,rows,different,{}),"Changed batch authorization rejects the whole old request cohort");
  rows=initial;result=prepare();rows.pop_back();check(!applyBatchRefresh(result,rows,{},{}),"A changed source count rejects old positional identities");
  rows=initial;result=prepare();check(!applyBatchRefresh(result,rows,{},{1}),"A source accepted after capture rejects the older advisory cohort");
  rows=initial;result=prepare();result.values[1].index=0;check(!applyBatchRefresh(result,rows,{},{}),"Duplicated result identities cannot overwrite one row twice");
  rows=initial;result=prepare();result.values[1].index=2;check(!applyBatchRefresh(result,rows,{},{}),"An out-of-range result identity is rejected before indexing current rows");
  rows=initial;result=prepare();result.values.pop_back();check(!applyBatchRefresh(result,rows,{},{}),"An incomplete bulk result cannot be presented as a complete cohort");
  rows=initial;result=prepare(std::nullopt,{0});check(applyBatchRefresh(result,rows,{},{0})&&result.values.size()==1&&result.values[0].index==1,"A complete bulk cohort may intentionally omit previously added rows");
  rows=initial;result=prepare(1);rows[0].metadata={{"Marker","keep-other-slot"}};check(applyBatchRefresh(result,rows,{},{} )&&str(rows[0].metadata,"Marker")=="keep-other-slot"&&num(rows[1].metadata,"Size")==4096,"A current single-row result changes only its captured source slot");
  rows=initial;result=prepare(1);result.changed=0;check(!applyBatchRefresh(result,rows,{},{}),"A changed-slot descriptor cannot borrow another row's prepared response");
  rows=initial;result=prepare(1);result.values.clear();check(!applyBatchRefresh(result,rows,{},{}),"A missing changed-row result cannot silently clear request suppression");
  rows=initial;result=prepare(1);result.changed=2;check(!applyBatchRefresh(result,rows,{},{}),"An invalid changed-slot descriptor is rejected");
  rows=initial;result=prepare();std::reverse(result.values.begin(),result.values.end());check(applyBatchRefresh(result,rows,{},{} )&&num(rows[0].metadata,"Size")==4096&&num(rows[1].metadata,"Size")==4096,"Result publication uses stable identities rather than result vector order");
  check(manager.snapshot()==before&&readText(manager.root/L"state.json")==disk&&!manager.fileInfoDecisionPending,"Advisory cohort validation and application never change catalog or command ownership");manager.stop();
 }catch(const std::exception& failure){error=failure.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
