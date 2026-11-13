#include "ProgressRecordView.hpp"
#include "HttpRepresentation.hpp"
#include "GuiModels.hpp"
#include <iostream>
using namespace udm;
// Kept only as an in-memory comparison with the pre-7.48 refresh algorithm.
static bool legacyCandidate(const Json& d){
 if(!hasNonemptyString(d,"ProtectedRequest")||!yes(d,"PostResponseComplete")||yes(d,"RefreshPendingValidation"))return false;
 const auto f=d.find("CompletedPrefetch");if(f==d.end()||!f->is_object())return false;const auto& r=*f;const auto size=num(d,"Size",-1);
 return num(r,"Version")==1&&size>=0&&num(d,"Received")==size&&num(r,"Size",-1)==size&&str(r,"Url")==str(d,"Url")&&str(r,"Headers")==str(d,"ProtectedHeaders")&&!str(d,"PostResponseRequestSha256").empty()&&str(r,"RequestSha256")==str(d,"PostResponseRequestSha256")&&str(r,"ContentEncoding")==str(d,"HttpContentEncoding")&&str(r,"PostGetUrl")==str(d,"ProtectedPostGetUrl")&&str(r,"PostGetHeaders")==str(d,"ProtectedPostGetHeaders")&&r.value("Plan",Json::array())==d.value("Segments",Json::array())&&r.contains("Hashes")&&r["Hashes"].is_array()&&r["Hashes"].size()==d.value("Segments",Json::array()).size();
}
static ProgressRecordView legacyView(const Json& source,bool connections){
 ProgressRecordView v;
 for(auto it=source.begin();it!=source.end();++it){const auto& key=it.key();
  if(key=="ProtectedAdaptive"||key=="ProtectedAdaptiveDisplayAddress"||key=="ProtectedSabr"||key=="ProtectedResolvedUrl"||key=="ProtectedMediaCookies"||key=="ProtectedMediaHeaders"||key=="ProtectedBrowserSession"||key=="Video"||key=="Audio")continue;
  if(!connections&&(key=="Segments"||key=="SabrRanges"))continue;v.data[key]=it.value();
 }
 v.request={hasNonemptyString(source,"ProtectedRequest"),hasNonemptyString(source,"ProtectedPostGetUrl"),legacyCandidate(v.data)};return v;
}
int main(int argc,char** argv){
 Json checks=Json::array(),timing;bool passed=false;std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Json source={{"Status","Paused"},{"Url","https://example.invalid/owned"},{"FileName","owned.bin"},{"Size",65536000},{"Received",65536000},{"PostAttempted",true},{"PostResponseComplete",true},{"ProtectedRequest",std::string(6*1024*1024,'x')+"FORM_REQUEST_SENTINEL"},{"ProtectedHeaders",std::string(48*1024,'h')},{"ProtectedPostGetUrl","opaque-redirect"},{"ProtectedPostGetHeaders",std::string(48*1024,'r')},{"PostResponseRequestSha256",std::string(64,'a')},{"Segments",Json::array()}};
  for(int i=0;i<1000;++i)source["Segments"].push_back({{"Index",i},{"Start",i*65536LL},{"End",(i+1)*65536LL-1},{"Done",65536}});
  source["CompletedPrefetch"]={{"Version",1},{"Url",source["Url"]},{"Headers",source["ProtectedHeaders"]},{"RequestSha256",source["PostResponseRequestSha256"]},{"ContentEncoding",""},{"PostGetUrl",source["ProtectedPostGetUrl"]},{"PostGetHeaders",source["ProtectedPostGetHeaders"]},{"Size",source["Size"]},{"Plan",source["Segments"]},{"Hashes",Json::array()}};
  for(int i=0;i<1000;++i)source["CompletedPrefetch"]["Hashes"].push_back(std::string(64,'b'));
  source["ProtectedRenderedPage"]=std::string(2*1024*1024,'p');source["OfflineProject"]={{"Pages",std::string(1024*1024,'w')}};
  source["OriginalResponse"]={{"State","Reserved"},{"ProtectedRequestHeaders",std::string(128*1024,'q')}};
  source["RetainedMediaOutput"]={{"Version",1},{"Kind","LiveHls"},{"Size",12345},{"Sha256",std::string(64,'c')},{"Binding",std::string(512*1024,'z')}};
  check(completedPostResponseCandidate(source)&&legacyCandidate(source),"Large source has matching complete-response metadata");
  const auto baseline=source;
  size_t legacyBytes=0,visibleBytes=0,hiddenBytes=0;
  {auto v=legacyView(source,true);legacyBytes=v.data.dump().size();}
  for(bool details:{false,true}){
   auto v=progressRecordView(source,details);const auto bytes=v.data.dump();if(details)visibleBytes=bytes.size();else hiddenBytes=bytes.size();
   check(v.request.form&&v.request.redirect&&v.request.saved,"Display request state derives from the original record before redaction");
   check(bytes.find("FORM_REQUEST_SENTINEL")==std::string::npos&&!v.data.contains("ProtectedRequest")&&!v.data.contains("ProtectedRenderedPage")&&!v.data.contains("OfflineProject"),"Progress presentation contains no opaque form or captured-page payloads");
   check(v.data["CompletedPrefetch"]==Json{{"Size",num(source,"Size")}}&&v.data["OriginalResponse"]==Json{{"State","Reserved"}},"Only the used receipt size and captured-response state enter the display snapshot");
   check(!v.data["RetainedMediaOutput"].contains("Binding")&&savedMediaOutputSize(v.data)==savedMediaOutputSize(source),"Output receipt summary preserves displayed readiness without its large binding");
   check(v.data.contains("Segments")==details,"Only the visible connection map copies its needed segments");
   check(progressResumeCapability(v.data,&v.request)==progressResumeCapability(source)&&progressNeedsFormRestart(v.data,&v.request)==progressNeedsFormRestart(source),"Resume capability and form restart behavior survive payload redaction");
  }
  check(source==baseline,"Snapshot creation never mutates transport or recovery data");
  check(visibleBytes<legacyBytes/10&&hiddenBytes<visibleBytes,"Snapshot size falls substantially while hidden connections omit their range map");
  for(const char* key:{"Url","ProtectedHeaders","PostResponseRequestSha256","HttpContentEncoding","ProtectedPostGetUrl","ProtectedPostGetHeaders","Received","Size","Segments"}){
   auto changed=source;changed[key]=nullptr;check(completedPostResponseCandidate(changed)==legacyCandidate(changed),"Reference-based candidate checks preserve legacy missing/null field semantics");
   changed.erase(key);check(completedPostResponseCandidate(changed)==legacyCandidate(changed),"Reference-based candidate checks preserve absent-field defaults");
  }
  auto partial=source;partial["Segments"][0]["Done"]=0;check(!completedPostResponseCandidate(partial)&&!legacyCandidate(partial),"A changed range plan still invalidates the saved-response candidate");
  auto owned=progressRecordView(source,true);owned.data["Segments"][0]["Done"]=0;check(num(source["Segments"][0],"Done")==65536,"Display range-map edits cannot alter transport checkpoints");
  auto refresh=source;refresh["ProtectedRefreshOffer"]=std::string(1024*1024,'f');auto refreshed=progressRecordView(refresh,false);
  check(savedMediaOutputSize(refresh)==-1&&savedMediaOutputSize(refreshed.data)==-1&&str(refreshed.data["RetainedMediaOutput"],"Kind")=="LiveHls","Protected refresh invalidation survives redaction without losing the live receipt kind");
  auto damaged=source;damaged.erase("CompletedPrefetch");auto rejected=progressRecordView(damaged,false);check(progressNeedsFormRestart(rejected.data,&rejected.request)&&progressResumeCapability(rejected.data,&rejected.request)=="No","Rejected form response still offers explicit Restart");
  auto encoded=source;encoded["HttpContentEncoding"]="gzip";encoded["CompletedPrefetch"]["ContentEncoding"]="gzip";encoded["HttpDecodedSize"]=100;encoded["HttpDecodedReceived"]=100;
  auto decoded=progressRecordView(encoded,false);projectHttpOutput(decoded.data);check(decoded.request.saved&&progressResumeCapability(decoded.data,&decoded.request)=="Response saved"&&num(decoded.data,"Size")==100,"Decoded output projection cannot invalidate the original saved-response candidate");
  auto captured=source;captured["OriginalResponse"]["State"]="Ready";auto capturedView=progressRecordView(captured,false);check(capturedResponseSavePending(capturedView.data)&&progressResumeCapability(capturedView.data,&capturedView.request)=="Response saved","Captured-response save status survives journal reduction");
  std::vector<double> oldSamples,newSamples;size_t sink=0;
  const auto measure=[&](bool old){const auto start=std::chrono::steady_clock::now();for(int i=0;i<8;++i){auto v=old?legacyView(source,true):progressRecordView(source,true);sink+=v.data.size()+static_cast<size_t>(v.request.saved);}return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/8;};
  for(int trial=0;trial<7;++trial){if(trial%2){newSamples.push_back(measure(false));oldSamples.push_back(measure(true));}else{oldSamples.push_back(measure(true));newSamples.push_back(measure(false));}}
  auto a=oldSamples,b=newSamples;std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());timing={{"scope","Synthetic in-memory snapshot comparison, not complete UI-frame or IDM timing"},{"framesPerTrial",8},{"legacyMsPerFrame",oldSamples},{"currentMsPerFrame",newSamples},{"legacyMedianMs",a[a.size()/2]},{"currentMedianMs",b[b.size()/2]},{"legacySerializedBytes",legacyBytes},{"visibleSerializedBytes",visibleBytes},{"hiddenSerializedBytes",hiddenBytes},{"sink",sink}};

  Job addressJob(Json{{"Id",guid()},{"FileName","address.bin"},{"Url","https://example.invalid/original"}});
  const auto addressCase=[&](const Json& expected){
   auto cached=downloadAddressInputs(addressJob);
   check(cached==expected,"Address projection preserves the selected source fields and shape");
   check(downloadAddressInputsUnchanged(addressJob,cached),"Unchanged selected address inputs match without rebuilding the snapshot");
   cached["Unexpected"]=true;check(!downloadAddressInputsUnchanged(addressJob,cached),"Extra cached address fields invalidate the snapshot");
   check(!downloadAddressInputsUnchanged(addressJob,Json()),"Invalid cached address shape cannot suppress refresh");
  };
  addressCase(Json{{"Url","https://example.invalid/original"}});
  auto initial=downloadAddressInputs(addressJob);addressJob.data["Url"]="https://example.invalid/changed";
  check(!downloadAddressInputsUnchanged(addressJob,initial),"Changed ordinary URL invalidates the address snapshot");
  addressJob.data["ProtectedResolvedUrl"]="opaque-resolved";
  addressCase(Json{{"Url","https://example.invalid/changed"}});
  addressJob.data["SourceUrl"]="https://example.invalid/watch";
  addressJob.video=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","track.bin"},{"Url","https://example.invalid/video"}});
  addressJob.audio=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","track.bin"},{"Url","https://example.invalid/audio"}});
  addressCase(Json{{"SourceUrl","https://example.invalid/watch"},{"Video",{{"Url","https://example.invalid/video"}}}});
  auto tracks=downloadAddressInputs(addressJob);addressJob.audio->data["Url"]="https://example.invalid/new-audio";
  check(downloadAddressInputsUnchanged(addressJob,tracks),"Unused secondary audio changes leave the selected video address snapshot valid");
  tracks=downloadAddressInputs(addressJob);addressJob.audio.reset();
  check(downloadAddressInputsUnchanged(addressJob,tracks),"Removing unused secondary audio leaves the selected video address snapshot valid");
  addressJob.video->data["Url"]="https://example.invalid/new-video";
  check(!downloadAddressInputsUnchanged(addressJob,tracks),"Changing the selected video address invalidates the snapshot");
  addressJob.data["ProtectedAdaptive"]=std::string(6*1024*1024,'a');
  auto largeAddress=downloadAddressInputs(addressJob);
  check(largeAddress.size()==1&&largeAddress["ProtectedAdaptive"]==addressJob.data["ProtectedAdaptive"],"Legacy adaptive address retains the full selected protected plan only in the owned snapshot");
  size_t matches=0;std::vector<double> cloneSamples,matchSamples;
  const auto addressMeasure=[&](bool clone){const auto start=std::chrono::steady_clock::now();for(int i=0;i<32;++i){if(clone){auto copy=downloadAddressInputs(addressJob);matches+=copy==largeAddress;}else matches+=downloadAddressInputsUnchanged(addressJob,largeAddress);}return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/32;};
  for(int trial=0;trial<7;++trial){if(trial%2){matchSamples.push_back(addressMeasure(false));cloneSamples.push_back(addressMeasure(true));}else{cloneSamples.push_back(addressMeasure(true));matchSamples.push_back(addressMeasure(false));}}
  auto cloneSorted=cloneSamples,matchSorted=matchSamples;std::sort(cloneSorted.begin(),cloneSorted.end());std::sort(matchSorted.begin(),matchSorted.end());
  timing["addressInputs"]={{"scope","Synthetic unchanged six-MiB legacy adaptive address; copy/compare vs borrowed comparison, not full UI or IDM"},{"iterationsPerTrial",32},{"cloneMsPerCheck",cloneSamples},{"borrowedMsPerCheck",matchSamples},{"cloneMedianMs",cloneSorted[3]},{"borrowedMedianMs",matchSorted[3]}};
  check(matches==448,"Both address algorithms match the unchanged large plan in every trial");
  addressJob.data["ProtectedAdaptive"].get_ref<std::string&>().back()='b';
  check(!downloadAddressInputsUnchanged(addressJob,largeAddress),"A same-length plan change near its end invalidates the cached address");
  addressJob.data["ProtectedAdaptiveDisplayAddress"]="opaque-display";
  addressCase(Json{{"ProtectedAdaptiveDisplayAddress","opaque-display"}});
  addressJob.data["ProtectedSabr"]="opaque-sabr";
  addressCase(Json{{"ProtectedSabr","opaque-sabr"},{"MediaOutput",""}});
  addressJob.data["SabrTrackAddressBound"]=true;addressJob.data["MediaOutput"]="audio";
  addressJob.audio=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","track.bin"},{"Url","https://example.invalid/bound"}});
  addressJob.video.reset();addressCase(Json{{"Url","https://example.invalid/bound"}});
  const auto bound=downloadAddressInputs(addressJob);addressJob.video=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","track.bin"},{"Url","https://example.invalid/bound"}});
  check(!downloadAddressInputsUnchanged(addressJob,bound),"Audio-only session with an unexpected video track rejects the direct binding");
  addressJob.data["MediaOutput"]="video";addressCase(Json{{"Url","https://example.invalid/bound"}});
  addressJob.video->data["Url"]="https://example.invalid/mismatched";
  addressCase(Json{{"ProtectedSabr","opaque-sabr"},{"MediaOutput","video"}});
  addressJob.data=Json{{"Url",17},{"ProtectedResolvedUrl",nullptr}};addressJob.video.reset();addressJob.audio.reset();
  addressCase(Json{{"Url",""},{"ProtectedResolvedUrl",""}});
  passed=true;
 }catch(const std::exception& e){error=e.what();}
 if(argc>1){const auto root=fs::path(wide(argv[1]));fs::create_directories(root);atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks},{"timing",timing}}.dump(2),false);}
 std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
