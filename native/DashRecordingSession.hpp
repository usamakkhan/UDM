#pragma once
#include "DashManifest.hpp"
#include <cmath>
#include <iomanip>
#include <regex>
#include <sstream>

namespace udm {
// The session owns ordering and immutable selection, not media receipt verification.
// A Period index must be reconciled before checkpoint() is called.
class DashRecordingSession {
 fs::path root;
 Json original,data;
 static void plain(const fs::path& path){
  const auto flags=GetFileAttributesW(path.c_str());
  if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("DASH session storage must not be redirected.");
 }
 void paths()const{
  for(auto path=root;!path.empty();){plain(path);auto parent=path.parent_path();if(parent==path)break;path=std::move(parent);}
  plain(root/L"session.json");plain(root/L"session.json.tmp");
 }
 static std::string folderName(size_t index){
  if(!index)return ".";std::ostringstream text;text<<"period-"<<std::setfill('0')<<std::setw(4)<<index;return text.str();
 }
 static double clock(const Json& value,const char* key,bool positive=false){
  if(!value.contains(key)||!value[key].is_number())throw std::runtime_error("DASH session clock is missing.");
  const double number=value[key].get<double>();if(!std::isfinite(number)||number<0||number>7*86400||(positive&&number<=0))throw std::runtime_error("Invalid DASH session clock.");return number;
 }
 static std::optional<double> optionalClock(const Json& value,const char* key){
  if(!value.contains(key)||value[key].is_null())return {};return clock(value,key,true);
 }
 static void stableId(const std::string& value){
  if(value.empty()||value.size()>512||std::any_of(value.begin(),value.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("Invalid DASH session track identity.");
 }
 void validate(const Json& value)const{
  if(!value.is_object()||num(value,"schema")!=1||!value.contains("plan")||value["plan"]!=original||!value.contains("periods")||!value["periods"].is_array()||value["periods"].size()>128)throw std::runtime_error("DASH recording session selection changed.");
  const auto phase=str(value,"phase");if(phase!="recording"&&phase!="advancing"&&phase!="saving")throw std::runtime_error("Invalid DASH recording session phase.");
  if(phase!="recording"&&value["periods"].empty())throw std::runtime_error("DASH session has no Periods.");
  std::set<std::string> ids;Json firstTracks;std::string presentation;double previousStart=0;std::optional<double> previousDuration,previousEnd;
  for(size_t p=0;p<value["periods"].size();++p){const auto& entry=value["periods"][p];
   if(str(entry,"folder")!=folderName(p)||!entry.contains("tracks")||!entry["tracks"].is_array()||entry["tracks"].size()!=original.at("tracks").size()||entry["tracks"].empty()||!entry.contains("sealed")||!entry["sealed"].is_boolean())throw std::runtime_error("Invalid DASH session Period descriptor.");
   const bool sealed=yes(entry,"sealed");if((phase=="saving"||p+1<value["periods"].size())&&!sealed)throw std::runtime_error("DASH session advances past an unfinished Period.");
   const auto& first=entry["tracks"].front();const auto id=str(first,"period"),currentPresentation=str(first,"presentation");stableId(id);
   if(!ids.insert(id).second||!std::regex_match(currentPresentation,std::regex("[0-9a-f]{64}")))throw std::runtime_error("Invalid DASH session presentation identity.");
   const double start=clock(first,"periodStart"),begin=clock(entry,"captureStart");const auto duration=optionalClock(entry,"periodDuration"),end=optionalClock(entry,"captureEnd");
   if(phase=="advancing"&&p+1==value["periods"].size()&&!duration)throw std::runtime_error("DASH advancing session has no Period end.");
   if((end&&(*end<=begin||(duration&&*end>*duration+0.000001)))||(sealed&&!end)||(duration&&begin>=*duration)||start+end.value_or(begin)>7*86400)throw std::runtime_error("Invalid DASH session Period interval.");
   if(p){if(currentPresentation!=presentation||!previousDuration||!previousEnd||start<=previousStart||std::abs(previousStart+*previousDuration-start)>0.000001||std::abs(*previousEnd-*previousDuration)>0.000001||std::abs(begin)>0.000001)throw std::runtime_error("DASH session Period boundaries are not adjacent.");}
   else{presentation=currentPresentation;firstTracks=entry["tracks"];}
   std::set<std::pair<std::string,std::string>> tracks;
   for(size_t t=0;t<entry["tracks"].size();++t){const auto& identity=entry["tracks"][t];const auto adaptation=str(identity,"adaptation"),representation=str(identity,"representation");stableId(adaptation);stableId(representation);
    if(!tracks.emplace(adaptation,representation).second||num(identity,"schema")!=1||str(identity,"period")!=id||str(identity,"presentation")!=presentation||std::abs(clock(identity,"periodStart")-start)>0.000001||num(identity,"timescale")<=0||num(identity,"presentationTimeOffset",-1)<0||!identity.contains("format")||!identity["format"].is_object()||identity.dump().size()>65536)throw std::runtime_error("Invalid DASH session track descriptor.");
    if(identity["format"]!=firstTracks[t]["format"])throw std::runtime_error("DASH session selected track format changed.");
    if(!p){const auto& selected=original["tracks"][t];if(id!=str(selected,"periodId")||adaptation!=str(selected,"adaptationId")||representation!=str(selected,"representationId"))throw std::runtime_error("DASH session initial selection changed.");}
   }
   previousStart=start;previousDuration=duration;previousEnd=end;
  }
 }
 void commit(Json next,const Cancel& cancel){
  cancel.check();validate(next);if(next==data)return;const auto text=next.dump();if(text.size()>8*1024*1024)throw std::runtime_error("DASH session exceeds its storage limit.");
  const auto envelope=Json{{"schema",1},{"protected",protect(text)}}.dump();if(envelope.size()>16*1024*1024)throw std::runtime_error("DASH protected session exceeds its storage limit.");
  paths();cancel.check();atomicText(root/L"session.json",envelope,false);data=std::move(next);
 }
public:
 DashRecordingSession(const fs::path& directory,const Json& plan,const Cancel& cancel):root(fs::absolute(directory).lexically_normal()),original(plan){
  cancel.check();paths();if(!plan.is_object()||!plan.contains("tracks")||!plan["tracks"].is_array()||plan["tracks"].empty()||plan["tracks"].size()>2)throw std::runtime_error("Invalid DASH session selection.");
  data={{"schema",1},{"plan",plan},{"phase","recording"},{"periods",Json::array()}};
  if(fs::exists(root/L"session.json")){const auto envelope=Json::parse(readText(root/L"session.json",16*1024*1024));if(num(envelope,"schema")!=1)throw std::runtime_error("Unsupported DASH recording session.");const auto text=reveal(str(envelope,"protected"));if(text.size()>8*1024*1024)throw std::runtime_error("DASH recording session exceeds its storage limit.");data=Json::parse(text);}validate(data);
 }
 std::string currentFolder()const{return data["periods"].empty()?".":str(data["periods"].back(),"folder");}
 bool hasPeriods()const{return !data["periods"].empty();}
 bool saving()const{return str(data,"phase")=="saving";}
 bool advancing()const{return str(data,"phase")=="advancing";}
 fs::path currentDirectory()const{const auto name=currentFolder();auto path=name=="."?root:root/wide(name);plain(path);return path;}
 Json currentPlan()const{
  auto plan=original;if(!data["periods"].empty()){const auto& tracks=data["periods"].back()["tracks"];for(size_t t=0;t<tracks.size();++t){plan["tracks"][t]["periodId"]=str(tracks[t],"period");plan["tracks"][t]["adaptationId"]=str(tracks[t],"adaptation");plan["tracks"][t]["representationId"]=str(tracks[t],"representation");}}return plan;
 }
 std::string nextFolder()const{if(data["periods"].size()>=128)throw std::runtime_error("DASH session Period limit reached.");return folderName(data["periods"].size());}
 void checkpoint(const std::string& folder,const Json& index,const Cancel& cancel){
  cancel.check();if(!index.is_object()||!index.contains("tracks")||!index["tracks"].is_array()||index["tracks"].empty())throw std::runtime_error("Invalid DASH session recording index.");
  Json entry={{"folder",folder},{"tracks",Json::array()},{"captureStart",clock(index,"captureStart")},{"captureEnd",index.value("captureEnd",Json())},{"sealed",yes(index,"sealed")},{"periodDuration",nullptr}};
  std::optional<double> duration;for(const auto& track:index["tracks"]){entry["tracks"].push_back(track.at("identity"));const auto current=optionalClock(track,"periodDuration");if(current){if(duration&&std::abs(*duration-*current)>0.000001)throw std::runtime_error("DASH session track durations disagree.");duration=current;}}if(duration)entry["periodDuration"]=*duration;
  auto next=data;auto& periods=next["periods"];
  if(!periods.empty()&&folder==str(periods.back(),"folder")){const auto& previous=periods.back();if(entry["tracks"]!=previous["tracks"]||entry["captureStart"]!=previous["captureStart"]||(yes(previous,"sealed")&&!yes(entry,"sealed")))throw std::runtime_error("DASH session recording identity or start changed.");
   for(const char* key:{"captureEnd","periodDuration"})if(previous.contains(key)&&!previous[key].is_null()){if(entry[key].is_null()||std::abs(clock(entry,key,true)-clock(previous,key,true))>0.000001)throw std::runtime_error("DASH session recorded boundary changed or disappeared.");}periods.back()=std::move(entry);
  }else{if(saving()||folder!=nextFolder())throw std::runtime_error("DASH session Period order changed.");periods.push_back(std::move(entry));next["phase"]="recording";}
  commit(std::move(next),cancel);
 }
 void beginSaving(const Cancel& cancel){auto next=data;next["phase"]="saving";commit(std::move(next),cancel);}
 void beginAdvance(const Cancel& cancel){if(saving())throw std::runtime_error("DASH saving session cannot resume capture.");auto next=data;next["phase"]="advancing";commit(std::move(next),cancel);}
 bool discardEmptyTail(const Json& index,const Cancel& cancel){
  if(data["periods"].size()<2||saving()||yes(index,"sealed")||!index.contains("tracks")||index["tracks"].size()!=data["periods"].back()["tracks"].size())return false;
  for(size_t t=0;t<index["tracks"].size();++t){const auto& track=index["tracks"][t];if(track.at("identity")!=data["periods"].back()["tracks"][t])throw std::runtime_error("DASH empty tail identity changed.");for(const auto& segment:track.at("segments"))if(yes(segment,"committed"))return false;}
  auto next=data;next["periods"].erase(next["periods"].size()-1);commit(std::move(next),cancel);return true;
 }
 std::vector<std::string> folders()const{std::vector<std::string> result;for(const auto& entry:data["periods"])result.push_back(str(entry,"folder"));if(result.empty())throw std::runtime_error("DASH recording session has no captured Periods.");return result;}
};
}
