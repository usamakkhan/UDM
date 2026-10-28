#include "DashManifest.hpp"
#include "AdaptiveJournal.hpp"
#include "DashPeriodAssembly.hpp"
#include <fstream>
#include <xmllite.h>
#include <shlwapi.h>
#include <regex>
#include <cmath>
#include <limits>
#include <algorithm>
#include <future>

namespace udm {
namespace {
struct DashComRelease {template<class T>void operator()(T* value)const noexcept{if(value)value->Release();}};
void dashXmlCheck(HRESULT hr){if(FAILED(hr))throw std::runtime_error("Invalid or unsupported DASH XML.");}
double dashAvailabilityOffset(const std::map<std::string,std::string>& attributes){
 auto found=attributes.find("availabilityTimeOffset");if(found==attributes.end())return 0;
 if(found->second=="INF")return std::numeric_limits<double>::infinity();
 if(!std::regex_match(found->second,std::regex("[+-]?(?:[0-9]+(?:\\.[0-9]*)?|\\.[0-9]+)(?:[eE][+-]?[0-9]+)?")))throw std::runtime_error("Invalid DASH availability offset.");
 double value=std::stod(found->second);if(!std::isfinite(value)||std::abs(value)>7*86400)throw std::runtime_error("Unsupported DASH availability offset.");return value;
}
bool dashAvailabilityComplete(const std::map<std::string,std::string>& attributes){
 auto found=attributes.find("availabilityTimeComplete");if(found==attributes.end()||found->second=="true"||found->second=="1")return true;
 if(found->second=="false"||found->second=="0")return false;throw std::runtime_error("Invalid DASH availability completeness.");
}
std::string dashXmlString(IXmlReader* reader,HRESULT (STDMETHODCALLTYPE IXmlReader::*method)(LPCWSTR*,UINT*)){
 LPCWSTR value=nullptr;UINT length=0;dashXmlCheck((reader->*method)(&value,&length));return value?utf8(std::wstring(value,length)):std::string{};
}
std::optional<double> dashXmlDuration(const DashXmlNode& root,const char* name){
 auto found=root.attributes.find(name);if(found==root.attributes.end())return {};
 const auto& text=found->second;std::smatch match;
 if(!std::regex_match(text,match,std::regex("P(?:(\\d+(?:\\.\\d+)?)D)?(?:T(?:(\\d+(?:\\.\\d+)?)H)?(?:(\\d+(?:\\.\\d+)?)M)?(?:(\\d+(?:\\.\\d+)?)S)?)?"))||(!match[1].matched&&!match[2].matched&&!match[3].matched&&!match[4].matched)||text.back()=='T')throw std::runtime_error("Invalid DASH duration attribute.");
 double seconds=0;const double factors[]={86400,3600,60,1};for(size_t i=1;i<=4;++i)if(match[i].matched)seconds+=std::stod(match[i])*factors[i-1];
 if(!std::isfinite(seconds)||seconds<0||seconds>7*86400)throw std::runtime_error("DASH duration exceeds the supported timeline.");return seconds;
}
std::optional<i64> dashXmlTime(const DashXmlNode& root,const char* name){
 auto found=root.attributes.find(name);if(found==root.attributes.end())return {};
 std::smatch match;if(!std::regex_match(found->second,match,std::regex("([0-9]{4})-([0-9]{2})-([0-9]{2})T([0-9]{2}):([0-9]{2}):([0-9]{2})(?:\\.([0-9]{1,9}))?(Z|([+-])([0-9]{2}):([0-9]{2}))")))throw std::runtime_error("DASH wall-clock time requires an explicit timezone.");
 SYSTEMTIME value{};value.wYear=(WORD)std::stoi(match[1]);value.wMonth=(WORD)std::stoi(match[2]);value.wDay=(WORD)std::stoi(match[3]);value.wHour=(WORD)std::stoi(match[4]);value.wMinute=(WORD)std::stoi(match[5]);value.wSecond=(WORD)std::stoi(match[6]);
 if(value.wYear<1601||value.wMonth<1||value.wMonth>12||value.wDay<1||value.wHour>23||value.wMinute>59||value.wSecond>59)throw std::runtime_error("Invalid DASH wall-clock date.");
 FILETIME time{};if(!SystemTimeToFileTime(&value,&time))throw std::runtime_error("Invalid DASH wall-clock date.");
 SYSTEMTIME roundtrip{};if(!FileTimeToSystemTime(&time,&roundtrip)||roundtrip.wYear!=value.wYear||roundtrip.wMonth!=value.wMonth||roundtrip.wDay!=value.wDay)throw std::runtime_error("Invalid DASH calendar date.");
 ULARGE_INTEGER ticks{};ticks.LowPart=time.dwLowDateTime;ticks.HighPart=time.dwHighDateTime;
 i64 result=((i64)ticks.QuadPart-116444736000000000LL)/10;
 auto fraction=match[7].str();fraction.resize(6,'0');if(!fraction.empty())result+=std::stoll(fraction);
 if(match[9].matched){int hours=std::stoi(match[10]),minutes=std::stoi(match[11]);if(hours>14||minutes>59||(hours==14&&minutes!=0))throw std::runtime_error("Invalid DASH timezone offset.");i64 offset=(hours*3600LL+minutes*60LL)*1000000;result+=match[9].str()=="+"?-offset:offset;}
 return result;
}
}
std::string expandDashTemplate(const std::string& pattern,const DashTemplateValues& values){
 constexpr size_t limit=32768;
 if(pattern.empty()||pattern.size()>limit)throw std::runtime_error("Invalid DASH URL template length.");
 std::string result;
 auto append=[&](const std::string& text){
  if(text.size()>limit-result.size())throw std::runtime_error("Expanded DASH URL exceeds its length limit.");
  for(unsigned char c:text)if(c<32||c==127)throw std::runtime_error("Control character in DASH URL template.");
  result+=text;
 };
 for(size_t position=0;position<pattern.size();){
  if(pattern[position]!='$'){
   const auto next=pattern.find('$',position);const auto end=next==std::string::npos?pattern.size():next;
   append(pattern.substr(position,end-position));position=end;continue;
  }
  if(position+1<pattern.size()&&pattern[position+1]=='$'){append("$");position+=2;continue;}
  const auto end=pattern.find('$',position+1);if(end==std::string::npos)throw std::runtime_error("Unclosed DASH template identifier.");
  auto token=pattern.substr(position+1,end-position-1);position=end+1;
  const auto percent=token.find('%');auto name=token.substr(0,percent);size_t width=0;char format='d';
  if(percent!=std::string::npos){
   const auto spec=token.substr(percent);
   if(spec.size()<4||spec[0]!='%'||spec[1]!='0')throw std::runtime_error("Unsupported DASH template format.");
   format=spec.back();if(std::string("diouxX").find(format)==std::string::npos)throw std::runtime_error("Unsupported DASH template format.");
   for(size_t i=2;i+1<spec.size();++i){if(spec[i]<'0'||spec[i]>'9'||width>32)throw std::runtime_error("Invalid DASH template width.");width=width*10+spec[i]-'0';}
   if(width<1||width>32)throw std::runtime_error("Invalid DASH template width.");
  }
  if(name=="RepresentationID"){
   if(percent!=std::string::npos||!values.representationId||values.representationId->empty())throw std::runtime_error("Invalid DASH representation template value.");
   append(*values.representationId);continue;
  }
  std::optional<i64> value;
  if(name=="Bandwidth")value=values.bandwidth;else if(name=="Number")value=values.number;else if(name=="Time")value=values.time;
  else throw std::runtime_error("Unsupported DASH template identifier.");
  if(!value||*value<0)throw std::runtime_error("Missing or invalid DASH numeric template value.");
  const i64 radix=format=='o'?8:(format=='x'||format=='X'?16:10);const char* digits=format=='X'?"0123456789ABCDEF":"0123456789abcdef";
  i64 remaining=*value;std::string text;do{text.push_back(digits[remaining%radix]);remaining/=radix;}while(remaining);
  while(text.size()<width)text.push_back('0');std::reverse(text.begin(),text.end());append(text);
 }
 return result;
}

std::vector<DashTimelineEntry> readDashTimeline(const DashManifest& manifest,size_t index,const Cancel& cancel){
 cancel.check();
 if(index>=manifest.nodes.size()||manifest.nodes[index].name!="SegmentTimeline")throw std::runtime_error("Expected a DASH SegmentTimeline.");
 const auto& timeline=manifest.nodes[index];
 if(!timeline.namespaceUri.empty()&&timeline.namespaceUri!="urn:mpeg:dash:schema:mpd:2011")throw std::runtime_error("Unsupported DASH timeline namespace.");
 if(timeline.children.empty()||timeline.children.size()>12000)throw std::runtime_error("Invalid DASH timeline entry count.");
 auto integer=[](const std::string& text,bool repeat){
  if(repeat&&text=="-1")return i64{-1};
  if(text.empty()||text.size()>19)throw std::runtime_error("Invalid DASH timeline integer.");
  i64 value=0;for(char c:text){if(c<'0'||c>'9'||value>(std::numeric_limits<i64>::max()-(c-'0'))/10)throw std::runtime_error("Invalid DASH timeline integer.");value=value*10+c-'0';}return value;
 };
 std::vector<DashTimelineEntry> entries;entries.reserve(timeline.children.size());
 for(auto child:timeline.children){
  cancel.check();if(child>=manifest.nodes.size())throw std::runtime_error("Invalid DASH timeline node.");const auto& node=manifest.nodes[child];
  if(node.name!="S"||node.namespaceUri!=timeline.namespaceUri||!node.children.empty()||!trim(node.text).empty())throw std::runtime_error("Unsupported DASH timeline entry.");
  DashTimelineEntry entry;auto d=node.attributes.find("d");if(d==node.attributes.end()||(entry.duration=integer(d->second,false))<=0)throw std::runtime_error("DASH timeline requires a positive duration.");
  auto t=node.attributes.find("t");if(t!=node.attributes.end())entry.time=integer(t->second,false);
  auto r=node.attributes.find("r");if(r!=node.attributes.end())entry.repeat=integer(r->second,true);
  // Do not silently misinterpret extensions that alter sequence numbering.
  for(const auto& attribute:node.attributes)if(attribute.first!="t"&&attribute.first!="d"&&attribute.first!="r"&&attribute.first.find('{')!=0)throw std::runtime_error("Unsupported DASH timeline attribute.");
  entries.push_back(entry);
 }
 return entries;
}

std::vector<DashPeriodTiming> resolveDashPeriods(const DashManifest& manifest,const Cancel& cancel){
 cancel.check();if(manifest.nodes.empty()||manifest.nodes[0].name!="MPD")throw std::runtime_error("Expected a DASH presentation.");
 std::vector<DashPeriodTiming> periods;std::vector<std::optional<double>> starts;std::set<std::string> ids;
 for(auto index:manifest.nodes[0].children){
  cancel.check();if(index>=manifest.nodes.size())throw std::runtime_error("Invalid DASH Period node.");const auto& node=manifest.nodes[index];
  if(node.name!="Period"||node.namespaceUri!=manifest.nodes[0].namespaceUri)continue;
  if(periods.size()>=128)throw std::runtime_error("Too many DASH Periods.");
  DashPeriodTiming period;period.node=index;period.index=periods.size();period.duration=dashXmlDuration(node,"duration");
  auto id=node.attributes.find("id");if(id!=node.attributes.end()){if(id->second.empty()||id->second.size()>512||!ids.insert(id->second).second)throw std::runtime_error("Invalid or duplicate DASH Period ID.");period.id=id->second;}
  starts.push_back(dashXmlDuration(node,"start"));periods.push_back(std::move(period));
 }
 if(periods.empty())throw std::runtime_error("DASH presentation has no Periods.");
 if(!starts[0]&&!manifest.dynamic)starts[0]=0;
 for(size_t pass=0;pass<periods.size();++pass){
  bool changed=false;for(size_t i=0;i<periods.size();++i){cancel.check();
   if(!starts[i]&&i&&starts[i-1]&&periods[i-1].duration){starts[i]=*starts[i-1]+*periods[i-1].duration;changed=true;}
   if(!periods[i].duration&&starts[i]){
    std::optional<double> end=i+1<periods.size()?starts[i+1]:manifest.duration;
    if(end){if(*end<*starts[i])throw std::runtime_error("Invalid DASH Period boundary.");periods[i].duration=*end-*starts[i];changed=true;}
   }
  }if(!changed)break;
 }
 for(size_t i=0;i<periods.size();++i){
  cancel.check();if(!starts[i]||!std::isfinite(*starts[i])||*starts[i]<0||*starts[i]>7*86400)throw std::runtime_error("Unresolved DASH Period start.");periods[i].start=*starts[i];
  if(!periods[i].duration&&i+1<periods.size())throw std::runtime_error("Unresolved intermediate DASH Period duration.");
  if(periods[i].duration){const double end=periods[i].start+*periods[i].duration;
   if(!std::isfinite(end)||end>7*86400||(manifest.duration&&end>*manifest.duration+0.000001))throw std::runtime_error("DASH Period exceeds presentation bounds.");
   if(i+1<periods.size()&&starts[i+1]&&end>*starts[i+1]+0.000001)throw std::runtime_error("Overlapping DASH Periods are unsupported.");
  }
 }
 return periods;
}

std::vector<DashSegmentReference> expandDashTimeline(const std::vector<DashTimelineEntry>& entries,i64 startNumber,i64 windowStart,i64 windowEnd,const Cancel& cancel,std::optional<i64> endNumber){
 cancel.check();
 if(entries.empty()||entries.size()>12000||startNumber<0||windowStart<0||windowEnd<=windowStart||(endNumber&&*endNumber<startNumber))throw std::runtime_error("Invalid DASH timeline window.");
 auto add=[](i64 a,i64 b){if(a<0||b<0||a>std::numeric_limits<i64>::max()-b)throw std::runtime_error("DASH timeline integer overflow.");return a+b;};
 auto multiply=[](i64 a,i64 b){if(a<0||b<0||(b&&a>std::numeric_limits<i64>::max()/b))throw std::runtime_error("DASH timeline integer overflow.");return a*b;};
 auto ceilDivide=[](i64 value,i64 divisor){return value/divisor+(value%divisor!=0);};
 i64 cursor=0,number=startNumber;std::vector<DashSegmentReference> output;
 for(size_t i=0;i<entries.size();++i){
  cancel.check();const auto& entry=entries[i];
  if(entry.duration<=0||entry.repeat< -1||(entry.time&&*entry.time<0))throw std::runtime_error("Invalid DASH timeline entry.");
  const i64 time=entry.time.value_or(cursor);if(time<cursor)throw std::runtime_error("Overlapping DASH timeline entries are unsupported.");
  i64 count=0;
  if(entry.repeat>=0)count=add(entry.repeat,1);
  else{
   if(i+1<entries.size()&&!entries[i+1].time)throw std::runtime_error("Unbounded DASH repeat before an implicit start.");
   const i64 boundary=i+1<entries.size()?*entries[i+1].time:windowEnd;
   if(boundary<=time){if(i+1==entries.size())break;throw std::runtime_error("Invalid DASH repeat boundary.");}
   count=ceilDivide(boundary-time,entry.duration);
  }
  const i64 end=add(time,multiply(count,entry.duration));
  const i64 first=windowStart>=time?(windowStart-time)/entry.duration:0;
  i64 last=time<windowEnd?std::min(count,ceilDivide(windowEnd-time,entry.duration)):0;
  // Compare the distance first, so an inclusive maximum of INT64_MAX does not overflow.
  if(endNumber&&*endNumber-number<count-1)last=std::min(last,*endNumber-number+1);
  if(last>first){
   if(last-first>static_cast<i64>(10000-output.size()))throw std::runtime_error("DASH window exceeds the segment limit.");
   for(i64 n=first;n<last;++n){cancel.check();output.push_back({add(number,n),add(time,multiply(n,entry.duration)),entry.duration});}
  }
  if(endNumber&&*endNumber-number<=count-1)break;
  number=add(number,count);cursor=end;
 }
 return output;
}

DashTemplateAddressing resolveDashTemplate(const DashManifest& manifest,size_t representation,const std::string& manifestUrl,const Cancel& cancel){
 cancel.check();
 if(manifest.nodes.empty()||manifest.nodes.size()>12000||representation>=manifest.nodes.size())throw std::runtime_error("Invalid DASH representation selection.");
 const auto& nodes=manifest.nodes;
 std::vector<size_t> parents(nodes.size(),nodes.size());
 for(size_t i=0;i<nodes.size();++i){cancel.check();for(auto child:nodes[i].children){if(child<=i||child>=nodes.size()||parents[child]!=nodes.size())throw std::runtime_error("Invalid DASH manifest tree.");parents[child]=i;}}
 std::vector<size_t> lineage;for(size_t i=representation;;i=parents[i]){lineage.push_back(i);if(i==0)break;if(parents[i]==nodes.size()||lineage.size()>4)throw std::runtime_error("Unsupported DASH representation hierarchy.");}
 std::reverse(lineage.begin(),lineage.end());const char* names[]={"MPD","Period","AdaptationSet","Representation"};
 if(lineage.size()!=4)throw std::runtime_error("Expected a DASH representation within an adaptation set.");
 for(size_t i=0;i<4;++i)if(nodes[lineage[i]].name!=names[i]||nodes[lineage[i]].namespaceUri!=nodes[0].namespaceUri)throw std::runtime_error("Unsupported DASH representation hierarchy.");
 auto address=[](const std::string& value){for(unsigned char c:value)if(c<32||c==127)throw std::runtime_error("Invalid DASH resource URL.");Url parsed(value);if(parsed.scheme!="http"&&parsed.scheme!="https")throw std::runtime_error("DASH resources require HTTP or HTTPS.");return parsed.full;};
 DashTemplateAddressing result;result.bases.push_back(address(manifestUrl));result.baseTiming.push_back({});
 const auto periods=resolveDashPeriods(manifest,cancel);auto period=std::find_if(periods.begin(),periods.end(),[&](const DashPeriodTiming& value){return value.node==lineage[1];});
 if(period==periods.end()||(period->duration&&*period->duration==0))throw std::runtime_error("Selected DASH Period has no media duration.");result.period=*period;
 std::map<std::string,std::string> attributes;std::optional<size_t> timeline;
 for(auto index:lineage){
  cancel.check();std::vector<size_t> bases,templates;
  for(auto child:nodes[index].children){const auto& node=nodes[child];if(node.namespaceUri!=nodes[0].namespaceUri)continue;
   if(node.name=="BaseURL")bases.push_back(child);else if(node.name=="SegmentTemplate")templates.push_back(child);
   else if(node.name=="SegmentList"||node.name=="SegmentBase")throw std::runtime_error("Selected DASH representation is not template-addressed.");
  }
  if(!bases.empty()){
   if(bases.size()>32||result.bases.size()>32/bases.size())throw std::runtime_error("Too many DASH base URL alternatives.");
   std::vector<std::string> resolved;std::vector<DashTemplateAddressing::BaseTiming> timings;
   for(size_t b=0;b<result.bases.size();++b)for(auto child:bases){cancel.check();const auto& node=nodes[child];auto text=trim(node.text);if(text.empty()||text.size()>32768||!node.children.empty()||node.attributes.count("byteRange"))throw std::runtime_error("Invalid or unsupported DASH BaseURL.");auto value=address(combineUrl(result.bases[b],text));
    DashTemplateAddressing::BaseTiming timing{result.baseTiming[b].offset+dashAvailabilityOffset(node.attributes),result.baseTiming[b].complete&&dashAvailabilityComplete(node.attributes)};
    bool duplicate=false;for(size_t n=0;n<resolved.size();++n)if(resolved[n]==value&&timings[n].offset==timing.offset&&timings[n].complete==timing.complete){duplicate=true;break;}
    if(!duplicate){resolved.push_back(std::move(value));timings.push_back(timing);}
   }result.bases=std::move(resolved);result.baseTiming=std::move(timings);
  }
  if(templates.size()>1)throw std::runtime_error("Ambiguous DASH segment template.");
  if(!templates.empty()){
   const auto& node=nodes[templates.front()];for(const auto& attribute:node.attributes)attributes[attribute.first]=attribute.second;
   std::optional<size_t> localTimeline;
   for(auto child:node.children){const auto& element=nodes[child];if(element.namespaceUri!=nodes[0].namespaceUri)continue;
    if(element.name=="SegmentTimeline"){if(localTimeline)throw std::runtime_error("Ambiguous DASH segment timeline.");localTimeline=child;}
    else if(element.name=="Initialization"||element.name=="RepresentationIndex")throw std::runtime_error("DASH template child resource addressing is not supported yet.");
   }
   if(localTimeline)timeline=localTimeline;
  }
 }
 auto integer=[](const std::string& text){if(text.empty()||text.size()>19)throw std::runtime_error("Invalid DASH addressing integer.");i64 value=0;for(char c:text){if(c<'0'||c>'9'||value>(std::numeric_limits<i64>::max()-(c-'0'))/10)throw std::runtime_error("Invalid DASH addressing integer.");value=value*10+c-'0';}return value;};
 auto number=[&](const char* key,i64 fallback){auto found=attributes.find(key);return found==attributes.end()?fallback:integer(found->second);};
 for(const char* unsupported:{"eptDelta","pdDelta"})if(attributes.count(unsupported))throw std::runtime_error("Unsupported DASH template boundary adjustment.");
 result.availabilityOffset=dashAvailabilityOffset(attributes);result.availabilityComplete=dashAvailabilityComplete(attributes);
 auto media=attributes.find("media");if(media==attributes.end()||media->second.empty())throw std::runtime_error("DASH media template is missing.");result.media=media->second;
 auto initialization=attributes.find("initialization");if(initialization!=attributes.end())result.initialization=initialization->second;
 result.timescale=number("timescale",1);result.presentationTimeOffset=number("presentationTimeOffset",0);result.startNumber=number("startNumber",1);
 if(attributes.count("endNumber")){result.endNumber=number("endNumber",0);if(*result.endNumber<result.startNumber)throw std::runtime_error("DASH endNumber precedes startNumber.");}
 if(result.timescale<=0)throw std::runtime_error("DASH timescale must be positive.");
 const auto& rep=nodes[representation];auto id=rep.attributes.find("id");if(id!=rep.attributes.end())result.values.representationId=id->second;
 auto bandwidth=rep.attributes.find("bandwidth");if(bandwidth!=rep.attributes.end())result.values.bandwidth=integer(bandwidth->second);
 if(timeline)result.timeline=readDashTimeline(manifest,*timeline,cancel);
 else{const i64 duration=number("duration",0);if(duration<=0)throw std::runtime_error("DASH template requires a timeline or duration.");result.timeline.push_back({i64{0},duration,-1});}
 return result;
}

DashResourceWindow planDashTemplateWindow(const DashTemplateAddressing& addressing,i64 windowStart,i64 windowEnd,const Cancel& cancel){
 cancel.check();if(addressing.bases.empty()||addressing.bases.size()>32||addressing.timescale<=0||addressing.presentationTimeOffset<0)throw std::runtime_error("Invalid DASH template addressing.");
 auto references=expandDashTimeline(addressing.timeline,addressing.startNumber,windowStart,windowEnd,cancel,addressing.endNumber);
 DashResourceWindow result;result.timescale=addressing.timescale;result.presentationTimeOffset=addressing.presentationTimeOffset;result.period=addressing.period;
 size_t bytes=0;
 auto resources=[&](const std::string& pattern,const DashTemplateValues& values){
  std::vector<std::string> urls;auto relative=expandDashTemplate(pattern,values);
  for(const auto& base:addressing.bases){cancel.check();Url parsed(combineUrl(base,relative));if(parsed.scheme!="http"&&parsed.scheme!="https")throw std::runtime_error("DASH resources require HTTP or HTTPS.");
   if(std::find(urls.begin(),urls.end(),parsed.full)==urls.end()){if(parsed.full.size()>4*1024*1024-bytes)throw std::runtime_error("DASH resource window exceeds its URL memory limit.");bytes+=parsed.full.size();urls.push_back(std::move(parsed.full));}
  }return urls;
 };
 if(!addressing.initialization.empty())result.initialization=resources(addressing.initialization,addressing.values);
 for(const auto& reference:references){cancel.check();auto values=addressing.values;values.number=reference.number;values.time=reference.time;result.segments.push_back({reference,resources(addressing.media,values)});}
 return result;
}

DashResourceWindow planDashAvailableWindow(const DashManifest& manifest,const DashTemplateAddressing& addressing,i64 windowStart,i64 windowEnd,i64 nowUs,const Cancel& cancel){
 cancel.check();if(!manifest.dynamic||!manifest.availabilityStartUs)throw std::runtime_error("DASH availability planning requires a live presentation origin.");
 if(addressing.baseTiming.size()!=addressing.bases.size())throw std::runtime_error("DASH base availability metadata is incomplete.");
 if(!addressing.availabilityComplete||std::none_of(addressing.baseTiming.begin(),addressing.baseTiming.end(),[](const DashTemplateAddressing::BaseTiming& timing){return timing.complete;}))throw std::runtime_error("Low-latency DASH chunk availability requires streaming transfer support.");
 auto window=planDashTemplateWindow(addressing,windowStart,windowEnd,cancel);
 if(manifest.availabilityEndUs&&nowUs>=*manifest.availabilityEndUs){window.segments.clear();window.initialization.clear();return window;}
 const long double now=(static_cast<long double>(nowUs)-static_cast<long double>(*manifest.availabilityStartUs))/1000000.0L;
 const long double earliest=manifest.timeShiftBufferDepth?now-*manifest.timeShiftBufferDepth:0;
 std::vector<DashPlannedSegment> available;
 for(auto& segment:window.segments){
  cancel.check();const auto& ref=segment.reference;
  if(ref.time>std::numeric_limits<i64>::max()-ref.duration)throw std::runtime_error("DASH segment end overflows.");
  // Subtract the offset as integers before converting, preserving large shared epochs.
  const i64 startTicks=ref.time-addressing.presentationTimeOffset,endTicks=ref.time+ref.duration-addressing.presentationTimeOffset;
  constexpr i64 exactInteger=9007199254740991LL;
  if(startTicks>exactInteger||startTicks< -exactInteger||endTicks>exactInteger||endTicks< -exactInteger||addressing.timescale>exactInteger)throw std::runtime_error("DASH availability timing exceeds supported precision.");
  const long double start=addressing.period.start+static_cast<long double>(startTicks)/addressing.timescale;
  const long double end=addressing.period.start+static_cast<long double>(endTicks)/addressing.timescale;
  if(end<=addressing.period.start||(addressing.period.duration&&start>=addressing.period.start+*addressing.period.duration)||end<=earliest)continue;
  auto values=addressing.values;values.number=ref.number;values.time=ref.time;const auto relative=expandDashTemplate(addressing.media,values);std::set<std::string> ready;
  for(size_t b=0;b<addressing.bases.size();++b){
   cancel.check();const auto& timing=addressing.baseTiming[b];
   if(!timing.complete)continue;
   const long double offset=static_cast<long double>(timing.offset)+addressing.availabilityOffset;
   if(std::isnan(offset))throw std::runtime_error("Invalid DASH availability offset.");
   if(end<=now+offset)ready.insert(combineUrl(addressing.bases[b],relative));
  }
  segment.urls.erase(std::remove_if(segment.urls.begin(),segment.urls.end(),[&](const std::string& url){return !ready.count(url);}),segment.urls.end());
  if(!segment.urls.empty())available.push_back(std::move(segment));
 }
 window.segments=std::move(available);if(window.segments.empty())window.initialization.clear();return window;
}

i64 dashClockNow(const DashClockSample& sample,std::chrono::steady_clock::time_point now){
 const auto elapsed=std::chrono::duration_cast<std::chrono::microseconds>(now-sample.anchor).count();
 if(elapsed<0||elapsed>60000000||sample.uncertaintyUs<0||sample.uncertaintyUs>10000000||sample.serverUs>std::numeric_limits<i64>::max()-elapsed)throw std::runtime_error("DASH clock sample requires synchronization.");
 return sample.serverUs+elapsed;
}

namespace {
void accountDashMetadata(size_t bytes,const DashMetadataAccounting& onBytes,const std::shared_ptr<Cancel>& parent,const std::shared_ptr<Cancel>& budget){
 if(!bytes||!onBytes)return;
 const auto before=GetTickCount64();auto extend=[&]{const auto elapsed=GetTickCount64()-before;if(budget->deadline)budget->deadline=elapsed>std::numeric_limits<ULONGLONG>::max()-budget->deadline?std::numeric_limits<ULONGLONG>::max():budget->deadline+elapsed;};
 // App bandwidth/quota waits must remain cancellable without consuming the
 // network timeout. Timestamp anchors still record actual response receipt.
 try{onBytes(bytes,*parent);}catch(...){extend();throw;}extend();parent->check();
}
}

DashClockSample synchronizeDashClock(const DashManifest& manifest,const std::string& manifestUrl,const Json& prefs,std::chrono::steady_clock::time_point manifestReceived,const std::shared_ptr<Cancel>& cancel,bool allowDirect,const DashMetadataAccounting& onBytes){
 if(!cancel)throw std::runtime_error("DASH clock synchronization requires cancellation.");cancel->check();
 if(manifest.nodes.empty())throw std::runtime_error("DASH clock synchronization requires an MPD.");
 using Clock=std::chrono::steady_clock;
 auto budget=std::make_shared<Cancel>();budget->parent=cancel;budget->deadline=GetTickCount64()+10000;
 Json settings=prefs;settings.erase("ActiveBrowserSession");settings["SiteLogins"]=Json::array();settings["WindowsLoginIntranet"]=false;
 size_t sources=0,requests=0;
 for(auto index:manifest.nodes[0].children){
  budget->check();if(index>=manifest.nodes.size())throw std::runtime_error("Invalid DASH clock source node.");const auto& node=manifest.nodes[index];
  if(node.name!="UTCTiming"||node.namespaceUri!=manifest.nodes[0].namespaceUri)continue;
  if(++sources>16)throw std::runtime_error("Too many DASH clock sources.");
  auto schemeIt=node.attributes.find("schemeIdUri"),valueIt=node.attributes.find("value");if(schemeIt==node.attributes.end()||valueIt==node.attributes.end()||valueIt->second.size()>4096)continue;
  const auto& scheme=schemeIt->second;std::string method;
  for(const char* candidate:{"direct","http-head","http-xsdate","http-iso"})for(const char* year:{"2014","2012"})if(scheme==std::string("urn:mpeg:dash:utc:")+candidate+":"+year)method=candidate;
  if(method.empty())continue;
  if(method=="direct"){
   if(!allowDirect)continue;
   try{DashXmlNode value;value.attributes["time"]=trim(valueIt->second);DashClockSample sample{*dashXmlTime(value,"time"),1000000,manifestReceived,scheme};dashClockNow(sample,Clock::now());return sample;}catch(const std::exception&){budget->check();continue;}
  }
  auto endpoints=words(valueIt->second);if(endpoints.size()>8)continue;
  for(const auto& endpoint:endpoints){
   try{
    budget->check();auto address=combineUrl(manifestUrl,endpoint);Url source(address);if(source.scheme!="http"&&source.scheme!="https")continue;
    if(Url(manifestUrl).scheme=="https"&&source.scheme!="https")continue;
    for(int redirect=0;redirect<4;++redirect){
     budget->check();if(++requests>8)throw std::runtime_error("DASH clock request limit reached.");const auto sent=Clock::now();
     Http response(address,Headers{{"Cache-Control","no-cache"}},settings,*budget,{},{},"",nullptr,false,{},method=="http-head");
     if(response.status>=300&&response.status<400){auto location=response.header(L"Location");if(location.empty())break;auto next=combineUrl(address,location);Url parsed(next);if(parsed.scheme!="http"&&parsed.scheme!="https")break;if(Url(address).scheme=="https"&&parsed.scheme!="https")break;address=next;continue;}
     if(response.status!=200)break;
     const auto age=trim(response.header(L"Age"));if(!age.empty()&&age!="0")break;
     i64 serverUs=0;size_t bodyBytes=0;auto received=Clock::now();
     if(method=="http-head"){
      auto date=response.header(L"Date");SYSTEMTIME stamp{};FILETIME file{};
      if(date.empty()||date.size()>128||!WinHttpTimeToSystemTime(wide(date).c_str(),&stamp)||!SystemTimeToFileTime(&stamp,&file))break;
      ULARGE_INTEGER ticks{};ticks.LowPart=file.dwLowDateTime;ticks.HighPart=file.dwHighDateTime;if(ticks.QuadPart>static_cast<unsigned long long>(std::numeric_limits<i64>::max()))break;
      serverUs=(static_cast<i64>(ticks.QuadPart)-116444736000000000LL)/10;
     }else{auto body=response.all(256,*budget);received=Clock::now();bodyBytes=body.size();accountDashMetadata(bodyBytes,onBytes,cancel,budget);DashXmlNode value;value.attributes["time"]=trim(std::string(body.begin(),body.end()));serverUs=*dashXmlTime(value,"time");}
     const auto roundTrip=std::chrono::duration_cast<std::chrono::microseconds>(received-sent).count();
     if(roundTrip<0||roundTrip>10000000)break;
     DashClockSample sample{serverUs,roundTrip/2+1000000,sent+(received-sent)/2,scheme};dashClockNow(sample,Clock::now());return sample;
    }
   }catch(const std::exception&){budget->check();}
   if(requests>=8)throw std::runtime_error("DASH clock synchronization exhausted its request limit.");
  }
 }
 throw std::runtime_error("No declared DASH clock source could be synchronized.");
}

DashManifestSnapshot refreshDashManifest(const std::string& url,const Headers& headers,const Json& prefs,const std::shared_ptr<Cancel>& cancel,const DashManifestSnapshot* previous,const DashMetadataAccounting& onBytes){
 if(!cancel)throw std::runtime_error("DASH manifest refresh requires cancellation.");cancel->check();validateHeaders(headers);
 const Url original(url);if(original.scheme!="http"&&original.scheme!="https")throw std::runtime_error("DASH manifests require HTTP or HTTPS.");
 if(previous&&(previous->requestUrl!=url||previous->manifest.nodes.empty()))throw std::runtime_error("DASH snapshot belongs to a different request.");
 using Clock=std::chrono::steady_clock;auto budget=std::make_shared<Cancel>();budget->parent=cancel;budget->deadline=GetTickCount64()+15000;
 auto boundedHeader=[](const std::string& value,size_t limit){if(value.size()>limit||std::any_of(value.begin(),value.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("Invalid DASH cache validator.");return value;};
 std::string address=url;
 for(int redirect=0;redirect<6;++redirect){
  budget->check();Url current(address);Headers requestHeaders=current.origin==original.origin?headers:Headers{};
  for(auto it=requestHeaders.begin();it!=requestHeaders.end();){auto name=lower(it->first);if(name=="if-none-match"||name=="if-modified-since"||name=="range"||name=="if-range"||name=="cache-control")it=requestHeaders.erase(it);else ++it;}
  requestHeaders["Cache-Control"]="no-cache";bool conditional=false;
  if(previous&&address==previous->url){
   if(!previous->etag.empty()){requestHeaders["If-None-Match"]=boundedHeader(previous->etag,4096);conditional=true;}
   else if(!previous->modified.empty()){requestHeaders["If-Modified-Since"]=boundedHeader(previous->modified,128);conditional=true;}
  }
  Json settings=prefs;if(current.origin!=original.origin){settings.erase("ActiveBrowserSession");settings["SiteLogins"]=Json::array();settings["WindowsLoginIntranet"]=false;}
  const auto started=Clock::now();Http response(address,requestHeaders,settings,*budget,{},{},"",nullptr,false);
  if(response.status>=300&&response.status<400&&response.status!=304){
   const auto location=response.header(L"Location");if(location.empty())throw std::runtime_error("DASH manifest redirect has no destination.");auto next=combineUrl(address,location);Url target(next);
   if(target.scheme!="http"&&target.scheme!="https")throw std::runtime_error("Unsupported DASH redirect scheme.");if(current.scheme=="https"&&target.scheme!="https")throw std::runtime_error("DASH HTTPS downgrade redirect rejected.");address=std::move(next);continue;
  }
  const auto etag=boundedHeader(response.header(L"ETag"),4096),modified=boundedHeader(response.header(L"Last-Modified"),128);
  i64 age=0;const auto ageText=trim(response.header(L"Age"));for(char c:ageText){if(c<'0'||c>'9'||age>604800)throw std::runtime_error("Invalid DASH response age.");age=age*10+c-'0';}if(age>604800)throw std::runtime_error("DASH response is too old.");
  DashManifestSnapshot result;
  if(response.status==304){
   if(!previous||!conditional)throw std::runtime_error("Unexpected DASH not-modified response.");result=*previous;
   if(!etag.empty())result.etag=etag;if(!modified.empty())result.modified=modified;
  }else{
   if(response.status!=200)throw HttpRejected(response.status);
   const auto body=response.all(2*1024*1024,*budget);const auto received=Clock::now();accountDashMetadata(body.size(),onBytes,cancel,budget);result.document=std::string(body.begin(),body.end());result.manifest=parseDashManifest(result.document,*budget);
   if(previous){
    const auto& old=previous->manifest;const auto& next=result.manifest;
    auto identity=[](const DashManifest& value){auto found=value.nodes[0].attributes.find("id");return found==value.nodes[0].attributes.end()?std::optional<std::string>{}:std::optional<std::string>{found->second};};
    if(identity(old)!=identity(next))throw std::runtime_error("DASH presentation identity changed during refresh.");
    if(old.publishTimeUs&&(!next.publishTimeUs||*next.publishTimeUs<*old.publishTimeUs))throw std::runtime_error("DASH refresh returned an older publication.");
    if(next.dynamic&&old.availabilityStartUs!=next.availabilityStartUs)throw std::runtime_error("DASH live timeline origin changed during refresh.");
    if(!old.dynamic&&next.dynamic)throw std::runtime_error("Static DASH presentation changed back to live.");
   }
   result.requestUrl=url;result.url=address;result.etag=etag;result.modified=modified;
   result.bodyReceived=previous&&previous->document==result.document?previous->bodyReceived:received;
  }
  result.checked=Clock::now();result.responseAgeSeconds=age;result.validUntil.reset();
  if(result.manifest.dynamic&&result.manifest.minimumUpdatePeriod){
   const auto seconds=std::max(0.0,*result.manifest.minimumUpdatePeriod-static_cast<double>(age));
   result.validUntil=started+std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(seconds));
  }
  budget->check();return result;
 }
 throw std::runtime_error("DASH manifest redirect limit reached.");
}

Json selectDashLivePeriod(const std::string& url,const Headers& headers,const Json& preferences,const std::shared_ptr<Cancel>& cancel){
 if(!cancel)throw std::runtime_error("DASH Period selection requires cancellation.");Json prefs=preferences;prefs.erase("ActiveBrowserSession");prefs["SiteLogins"]=Json::array();prefs["WindowsLoginIntranet"]=false;
 const auto snapshot=refreshDashManifest(url,headers,prefs,cancel);const auto& manifest=snapshot.manifest;if(!manifest.dynamic||!manifest.availabilityStartUs)throw std::runtime_error("The DASH presentation is no longer live; refresh its formats.");
 using Clock=std::chrono::steady_clock;const auto sample=synchronizeDashClock(manifest,snapshot.url,prefs,snapshot.bodyReceived,cancel,snapshot.responseAgeSeconds==0&&Clock::now()-snapshot.bodyReceived<std::chrono::seconds(60));const auto now=Clock::now();
 if(snapshot.validUntil&&manifest.minimumUpdatePeriod&&*manifest.minimumUpdatePeriod>0&&now>=*snapshot.validUntil)throw std::runtime_error("The DASH manifest expired during Period selection; refresh its formats.");
 const long double relative=(static_cast<long double>(dashClockNow(sample,now))-*manifest.availabilityStartUs)/1000000.0L-manifest.suggestedDelay.value_or(6);
 const long double uncertainty=static_cast<long double>(sample.uncertaintyUs)/1000000.0L;const double first=static_cast<double>(std::max(0.0L,relative-uncertainty)),last=static_cast<double>(std::max(0.0L,relative+uncertainty));
 if(!std::isfinite(first)||!std::isfinite(last)||last>7*86400)throw std::runtime_error("DASH live time lies outside the supported timeline.");
 std::optional<DashPeriodTiming> selected;for(const auto& period:resolveDashPeriods(manifest,*cancel)){if(period.duration&&*period.duration==0)continue;if(period.start<=first&&(!period.duration||last<period.start+*period.duration)){if(selected)throw std::runtime_error("DASH active Period selection is ambiguous.");selected=period;}}
 if(!selected||!selected->id||selected->id->empty())throw std::runtime_error("The live stream is between Periods or lacks a stable ID; refresh its formats.");
 cancel->check();return {{"ok",true},{"periodId",*selected->id},{"manifestUrl",snapshot.url},{"periodStart",selected->start},{"periodDuration",selected->duration?Json(*selected->duration):Json()}};
}

DashSelectedTrack selectDashTrack(const DashManifestSnapshot& snapshot,size_t representation,const Cancel& cancel){
 cancel.check();DashSelectedTrack result;result.addressing=resolveDashTemplate(snapshot.manifest,representation,snapshot.url,cancel);
 const auto& nodes=snapshot.manifest.nodes;std::optional<size_t> adaptation;
 result.presentation=adaptiveSelectionDigest(Json{{"request",snapshot.requestUrl},{"id",nodes[0].attributes.count("id")?nodes[0].attributes.at("id"):std::string{}},{"origin",snapshot.manifest.availabilityStartUs?Json(*snapshot.manifest.availabilityStartUs):Json()}});
 for(auto child:nodes[result.addressing.period.node].children){cancel.check();const auto& node=nodes[child];if(node.name=="AdaptationSet"&&node.namespaceUri==nodes[0].namespaceUri&&std::find(node.children.begin(),node.children.end(),representation)!=node.children.end()){if(adaptation)throw std::runtime_error("Ambiguous DASH track ancestry.");adaptation=child;}}
 if(!adaptation)throw std::runtime_error("DASH track has no adaptation set.");
 auto identity=[](const DashXmlNode& node){auto id=node.attributes.find("id");if(id==node.attributes.end()||id->second.empty()||id->second.size()>512||std::any_of(id->second.begin(),id->second.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("Live DASH continuity requires stable IDs.");return id->second;};
 result.periodId=identity(nodes[result.addressing.period.node]);result.adaptationId=identity(nodes[*adaptation]);result.representationId=identity(nodes[representation]);
 // Reject duplicate sibling IDs before a selection can enter a recording.
 for(auto parent:{result.addressing.period.node,*adaptation}){std::set<std::string> seen;const std::string expected=parent==*adaptation?"Representation":"AdaptationSet";for(auto child:nodes[parent].children){const auto& node=nodes[child];if(node.name==expected&&node.namespaceUri==nodes[0].namespaceUri){auto found=node.attributes.find("id");if(found!=node.attributes.end()&&!seen.insert(found->second).second)throw std::runtime_error("Duplicate DASH track IDs.");}}}
 std::map<std::string,std::string> attributes;std::set<std::string> descriptors;
 for(auto index:{result.addressing.period.node,*adaptation,representation}){
  cancel.check();for(const auto& attribute:nodes[index].attributes)attributes[attribute.first]=attribute.second;
  for(auto child:nodes[index].children){const auto& node=nodes[child];if(node.namespaceUri!=nodes[0].namespaceUri)continue;
   if(node.name=="Role"||node.name=="Accessibility"||node.name=="AudioChannelConfiguration"||node.name=="EssentialProperty"||node.name=="SupplementalProperty"){
    if(!node.children.empty()||!trim(node.text).empty())throw std::runtime_error("Unsupported DASH track descriptor.");descriptors.insert(Json{{"name",node.name},{"attributes",node.attributes}}.dump());
   }else if(node.name=="Label"){if(!node.children.empty())throw std::runtime_error("Unsupported DASH track label.");descriptors.insert(Json{{"name",node.name},{"text",trim(node.text)},{"attributes",node.attributes}}.dump());}
  }
 }
 result.format=Json::object();for(const char* field:{"mimeType","contentType","codecs","width","height","frameRate","audioSamplingRate","lang","sar","scanType"}){auto found=attributes.find(field);auto value=found==attributes.end()?std::string{}:trim(found->second);if(std::string(field)=="mimeType"||std::string(field)=="contentType"||std::string(field)=="codecs"||std::string(field)=="lang")value=lower(value);result.format[field]=value;}
 if(str(result.format,"codecs").empty())throw std::runtime_error("Live DASH continuity requires declared codecs.");
 result.format["descriptors"]=descriptors;
 if(result.format.dump().size()>65536)throw std::runtime_error("DASH track metadata exceeds its limit.");
 return result;
}

DashSelectedTrack rematchDashTrack(const DashManifestSnapshot& snapshot,const DashSelectedTrack& previous,const Cancel& cancel){
 cancel.check();const auto& nodes=snapshot.manifest.nodes;if(nodes.empty())throw std::runtime_error("DASH refresh has no presentation.");
 auto find=[&](size_t parent,const char* name,const std::string& id){
  std::optional<size_t> match;for(auto child:nodes[parent].children){cancel.check();if(child>=nodes.size())throw std::runtime_error("Invalid DASH refreshed tree.");const auto& node=nodes[child];auto found=node.attributes.find("id");
   if(node.name==name&&node.namespaceUri==nodes[0].namespaceUri&&found!=node.attributes.end()&&found->second==id){if(match)throw std::runtime_error("Ambiguous DASH refreshed track identity.");match=child;}
  }if(!match)throw std::runtime_error("Selected DASH track is no longer present in the refreshed Period.");return *match;
 };
 const auto period=find(0,"Period",previous.periodId),adaptation=find(period,"AdaptationSet",previous.adaptationId),representation=find(adaptation,"Representation",previous.representationId);
 auto current=selectDashTrack(snapshot,representation,cancel);
 // A final static MPD may omit availabilityStartTime; retain the recording identity.
 if(snapshot.manifest.dynamic&&current.presentation!=previous.presentation)throw std::runtime_error("DASH recording presentation changed during refresh.");current.presentation=previous.presentation;
 if(current.format!=previous.format||current.addressing.timescale!=previous.addressing.timescale||current.addressing.presentationTimeOffset!=previous.addressing.presentationTimeOffset||std::abs(current.addressing.period.start-previous.addressing.period.start)>0.000001)throw std::runtime_error("Selected DASH track format or timeline changed during refresh.");
 if(previous.addressing.period.duration){if(current.addressing.period.duration&&std::abs(*current.addressing.period.duration-*previous.addressing.period.duration)>0.000001)throw std::runtime_error("Selected DASH Period end changed during refresh.");current.addressing.period.duration=previous.addressing.period.duration;}
 return current;
}

DashLiveState createDashLiveState(const DashManifestSnapshot& snapshot,const std::vector<size_t>& representations,const Cancel& cancel){
 cancel.check();if(!snapshot.manifest.dynamic||representations.empty()||representations.size()>3)throw std::runtime_error("Invalid DASH live track selection.");
 DashLiveState state;state.snapshot=snapshot;std::set<size_t> seen;
 for(auto representation:representations){cancel.check();if(!seen.insert(representation).second)throw std::runtime_error("Duplicate DASH live track selection.");auto track=selectDashTrack(snapshot,representation,cancel);
  if(!state.tracks.empty()&&track.periodId!=state.tracks.front().periodId)throw std::runtime_error("DASH live tracks must belong to the same Period.");state.tracks.push_back(std::move(track));
 }return state;
}

std::optional<DashPeriodTransition> planDashPeriodTransition(const DashLiveState& previous,const DashManifestSnapshot& refreshed,const Cancel& cancel){
 cancel.check();if(previous.tracks.empty()||previous.tracks.size()>3||previous.snapshot.manifest.nodes.empty()||refreshed.manifest.nodes.empty()||previous.snapshot.requestUrl!=refreshed.requestUrl)throw std::runtime_error("Invalid DASH Period transition source.");
 const auto& old=previous.snapshot.manifest;const auto& next=refreshed.manifest;
 auto rootId=[](const DashManifest& manifest){auto found=manifest.nodes[0].attributes.find("id");return found==manifest.nodes[0].attributes.end()?std::optional<std::string>{}:std::optional<std::string>{found->second};};
 if(rootId(old)!=rootId(next)||(old.publishTimeUs&&(!next.publishTimeUs||*next.publishTimeUs<*old.publishTimeUs))||((next.dynamic||next.availabilityStartUs)&&old.availabilityStartUs!=next.availabilityStartUs)||(!old.dynamic&&next.dynamic))throw std::runtime_error("DASH presentation changed during Period transition.");
 const auto& first=previous.tracks.front();auto current=first.addressing.period;
 if(first.periodId.empty()||first.presentation.empty()||!std::isfinite(current.start)||current.start<0)throw std::runtime_error("Invalid selected DASH Period.");
 for(const auto& track:previous.tracks)if(track.periodId!=first.periodId||track.presentation!=first.presentation||std::abs(track.addressing.period.start-current.start)>0.000001||track.addressing.period.duration!=current.duration)throw std::runtime_error("DASH selected tracks do not share a Period clock.");
 const auto periods=resolveDashPeriods(next,cancel);bool currentVisible=false;
 for(const auto& period:periods)if(period.id&&*period.id==first.periodId){
  currentVisible=true;
  if(std::abs(period.start-current.start)>0.000001||(current.duration&&period.duration&&std::abs(*current.duration-*period.duration)>0.000001))throw std::runtime_error("DASH current Period timing changed.");
  if(period.duration)current.duration=period.duration;
 }
 if(!current.duration){if(!currentVisible)throw std::runtime_error("DASH selected Period disappeared before its end was known.");return {};}
 const double boundary=current.start+*current.duration;if(!std::isfinite(boundary)||*current.duration<=0||boundary>7*86400)throw std::runtime_error("Invalid DASH Period end.");
 std::optional<DashPeriodTiming> following;
 for(const auto& period:periods){if(period.duration&&*period.duration==0)continue;if(period.id&&*period.id==first.periodId)continue;
  if(period.start<current.start-0.000001){if(period.duration&&period.start+*period.duration>current.start+0.000001)throw std::runtime_error("DASH refreshed history overlaps the selected Period.");continue;}if(!following||period.start<following->start)following=period;
 }
 if(!following)return {};
 if(std::abs(following->start-boundary)>0.000001)throw std::runtime_error("DASH Period transition has a gap or overlap.");
 if(!following->id||following->id->empty())throw std::runtime_error("DASH next Period requires a stable ID.");
 const auto& nodes=next.nodes;std::vector<DashSelectedTrack> candidates;size_t representations=0;
 for(auto a:nodes[following->node].children){const auto& adaptation=nodes[a];if(adaptation.name!="AdaptationSet"||adaptation.namespaceUri!=nodes[0].namespaceUri)continue;
  for(auto r:adaptation.children){cancel.check();if(nodes[r].name!="Representation"||nodes[r].namespaceUri!=nodes[0].namespaceUri)continue;if(++representations>256)throw std::runtime_error("DASH Period transition exceeds its representation budget.");
   try{auto track=selectDashTrack(refreshed,r,cancel);if(next.dynamic&&track.presentation!=first.presentation)throw std::runtime_error("DASH next Period belongs to another presentation.");track.presentation=first.presentation;candidates.push_back(std::move(track));}
   catch(const Cancelled&){throw;}catch(const std::runtime_error&){cancel.check();}
  }
 }
 DashPeriodTransition result;result.previousEnd=boundary;result.state.snapshot=refreshed;std::set<std::pair<std::string,std::string>> used;
 for(const auto& selected:previous.tracks){
  cancel.check();std::vector<const DashSelectedTrack*> compatible,exact,bandwidth;
  for(const auto& candidate:candidates){if(candidate.format!=selected.format)continue;compatible.push_back(&candidate);if(candidate.adaptationId==selected.adaptationId&&candidate.representationId==selected.representationId)exact.push_back(&candidate);if(selected.addressing.values.bandwidth&&candidate.addressing.values.bandwidth==selected.addressing.values.bandwidth)bandwidth.push_back(&candidate);}
  const auto& pool=!exact.empty()?exact:!bandwidth.empty()?bandwidth:compatible;
  if(pool.size()!=1)throw std::runtime_error(pool.empty()?"The next DASH Period has no compatible selected track.":"The next DASH Period has ambiguous selected tracks.");
  const auto& candidate=*pool.front();if(!used.emplace(candidate.adaptationId,candidate.representationId).second)throw std::runtime_error("DASH Period transition selected one track more than once.");result.state.tracks.push_back(candidate);
 }
 // A refreshed body/Period must establish its own current clock sample.
 result.state.clock.reset();return result;
}

DashLiveUpdate updateDashLivePlan(const DashLiveState& previous,const std::vector<std::pair<i64,i64>>& ranges,const Headers& headers,const Json& prefs,const std::shared_ptr<Cancel>& cancel,const DashMetadataAccounting& onBytes){
 if(!cancel)throw std::runtime_error("DASH live planning requires cancellation.");cancel->check();
 if(previous.tracks.empty()||previous.tracks.size()>3||ranges.size()!=previous.tracks.size())throw std::runtime_error("Invalid DASH live planning windows.");
 for(const auto& range:ranges)if(range.first<0||range.second<=range.first)throw std::runtime_error("Invalid DASH live media window.");
 using Clock=std::chrono::steady_clock;DashLiveUpdate result;result.state=previous;auto& state=result.state;
 if(state.snapshot.validUntil&&Clock::now()>=*state.snapshot.validUntil){
  auto refreshed=refreshDashManifest(state.snapshot.requestUrl,headers,prefs,cancel,&state.snapshot,onBytes);std::vector<DashSelectedTrack> tracks;
  for(const auto& selected:state.tracks){cancel->check();tracks.push_back(rematchDashTrack(refreshed,selected,*cancel));}
  if(refreshed.document!=state.snapshot.document)state.clock.reset();state.snapshot=std::move(refreshed);state.tracks=std::move(tracks);
 }
 result.staticPresentation=!state.snapshot.manifest.dynamic;
 if(!result.staticPresentation){
  bool synchronize=!state.clock;if(state.clock){try{dashClockNow(*state.clock,Clock::now());}catch(const std::exception&){synchronize=true;}}
  if(synchronize){
   // Direct sources cannot be made fresh merely by revalidating cached XML.
   // Their original body-receipt anchor is deliberately retained by refresh.
   state.clock=synchronizeDashClock(state.snapshot.manifest,state.snapshot.url,prefs,state.snapshot.bodyReceived,cancel,state.snapshot.responseAgeSeconds==0&&Clock::now()-state.snapshot.bodyReceived<std::chrono::seconds(60),onBytes);
  }
  if(state.snapshot.validUntil&&state.snapshot.manifest.minimumUpdatePeriod&&*state.snapshot.manifest.minimumUpdatePeriod>0&&Clock::now()>=*state.snapshot.validUntil)throw std::runtime_error("DASH manifest expired while synchronizing its clock.");
 }
 size_t count=0,bytes=0;
 for(size_t i=0;i<state.tracks.size();++i){
  cancel->check();const auto& addressing=state.tracks[i].addressing;DashResourceWindow window;
  if(result.staticPresentation){
   window=planDashTemplateWindow(addressing,ranges[i].first,ranges[i].second,*cancel);
   window.segments.erase(std::remove_if(window.segments.begin(),window.segments.end(),[&](const DashPlannedSegment& segment){
    const long double start=(static_cast<long double>(segment.reference.time)-addressing.presentationTimeOffset)/addressing.timescale;
    const long double end=start+static_cast<long double>(segment.reference.duration)/addressing.timescale;
    return end<=0||(addressing.period.duration&&start>=*addressing.period.duration);
   }),window.segments.end());
  }else{
   const i64 now=dashClockNow(*state.clock,Clock::now()),uncertainty=state.clock->uncertaintyUs;
   if(now<std::numeric_limits<i64>::min()+uncertainty||now>std::numeric_limits<i64>::max()-uncertainty)throw std::runtime_error("DASH clock uncertainty overflows.");
   window=planDashAvailableWindow(state.snapshot.manifest,addressing,ranges[i].first,ranges[i].second,now-uncertainty,*cancel);
   const auto latest=planDashAvailableWindow(state.snapshot.manifest,addressing,ranges[i].first,ranges[i].second,now+uncertainty,*cancel);
   std::map<i64,const DashPlannedSegment*> upper;for(const auto& segment:latest.segments)upper.emplace(segment.reference.number,&segment);
   window.segments.erase(std::remove_if(window.segments.begin(),window.segments.end(),[&](DashPlannedSegment& segment){
    const auto found=upper.find(segment.reference.number);if(found==upper.end())return true;
    const auto& other=*found->second;if(other.reference.time!=segment.reference.time||other.reference.duration!=segment.reference.duration)throw std::runtime_error("DASH availability plans disagree.");
    segment.urls.erase(std::remove_if(segment.urls.begin(),segment.urls.end(),[&](const std::string& url){return std::find(other.urls.begin(),other.urls.end(),url)==other.urls.end();}),segment.urls.end());return segment.urls.empty();
   }),window.segments.end());
  }
  if(window.segments.empty())window.initialization.clear();
  count+=window.segments.size();if(count>10000)throw std::runtime_error("Combined DASH live window exceeds the segment limit.");
  auto account=[&](const std::string& url){if(url.size()>4*1024*1024-bytes)throw std::runtime_error("Combined DASH live window exceeds its URL budget.");bytes+=url.size();};
  for(const auto& url:window.initialization)account(url);for(const auto& segment:window.segments)for(const auto& url:segment.urls)account(url);
  result.windows.push_back(std::move(window));
 }
 cancel->check();return result;
}

Json dashSegmentBinding(const DashSelectedTrack& track,const DashSegmentReference& segment){
 if(segment.number<0||segment.time<0||segment.duration<=0||segment.time>std::numeric_limits<i64>::max()-segment.duration)throw std::runtime_error("Invalid DASH segment identity.");
 if(track.presentation.empty())throw std::runtime_error("DASH segment has no presentation binding.");
 return {{"schema",1},{"presentation",track.presentation},{"period",track.periodId},{"adaptation",track.adaptationId},{"representation",track.representationId},{"format",track.format},{"periodStart",track.addressing.period.start},{"timescale",track.addressing.timescale},{"presentationTimeOffset",track.addressing.presentationTimeOffset},{"number",segment.number},{"time",segment.time},{"duration",segment.duration}};
}

std::optional<DashStoredResource> inspectDashResource(const fs::path& directory,const Json& binding,const Cancel& cancel){
 cancel.check();if(!binding.is_object()||binding.empty()||binding.dump().size()>65536)throw std::runtime_error("Invalid DASH resource binding.");
 auto plain=[](const fs::path& path){auto flags=GetFileAttributesW(path.c_str());if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("DASH recovery files must not be redirected.");};
 const auto folder=fs::absolute(directory).lexically_normal();for(auto parent=folder;!parent.empty();){plain(parent);auto next=parent.parent_path();if(next==parent)break;parent=std::move(next);}
 const auto key=adaptiveSelectionDigest(binding);const auto name=wide(key);const auto destination=folder/(name+L".part"),receipt=folder/(name+L".receipt.json");plain(destination);plain(receipt);
 if(!fs::is_regular_file(receipt)||!fs::is_regular_file(destination))return {};
 try{auto envelope=Json::parse(readText(receipt,256*1024));if(num(envelope,"schema")!=1)return {};auto saved=Json::parse(reveal(str(envelope,"protected")));const i64 length=num(saved,"bytes");const auto digest=str(saved,"sha256");
  if(str(saved,"binding")!=key||length<=0||length>256LL*1024*1024||!std::regex_match(digest,std::regex("[0-9a-fA-F]{64}"))||fs::file_size(destination)!=static_cast<uintmax_t>(length)||lower(fileHash(destination,[&]{cancel.check();}))!=lower(digest))return {};
  return DashStoredResource{destination,length,digest,true};
 }catch(const Cancelled&){throw;}catch(const std::exception&){cancel.check();return {};}
}

DashStoredResource storeDashResource(const fs::path& directory,const Json& binding,const std::vector<std::string>& urls,const DashResourceHeaders& headers,const Json& prefs,const std::shared_ptr<Cancel>& cancel,const std::function<void(size_t)>& onBytes){
 if(!cancel)throw std::runtime_error("DASH resource storage requires cancellation.");cancel->check();
 if(headers.size()>64)throw std::runtime_error("Too many DASH credential origins.");
 size_t headerBytes=0;for(const auto& scope:headers){const Url origin(scope.first);if((origin.scheme!="http"&&origin.scheme!="https")||origin.origin!=scope.first)throw std::runtime_error("DASH credentials require canonical HTTP origins.");validateHeaders(scope.second);for(const auto& entry:scope.second){headerBytes+=entry.first.size()+entry.second.size();if(headerBytes>256*1024)throw std::runtime_error("DASH resource headers exceed their size limit.");}}
 if(!binding.is_object()||binding.empty()||binding.dump().size()>65536||urls.empty()||urls.size()>32)throw std::runtime_error("Invalid DASH resource storage request.");
 auto plain=[](const fs::path& path){auto flags=GetFileAttributesW(path.c_str());if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("DASH recovery files must not be redirected.");};
 const auto folder=fs::absolute(directory).lexically_normal();for(auto parent=folder;!parent.empty();){plain(parent);auto next=parent.parent_path();if(next==parent)break;parent=std::move(next);}fs::create_directories(folder);plain(folder);
 const auto key=adaptiveSelectionDigest(binding);const auto destination=folder/(wide(key)+L".part"),receipt=folder/(wide(key)+L".receipt.json"),temporary=folder/(wide(key)+L".part.tmp");
 plain(destination);plain(receipt);plain(temporary);auto receiptTemp=receipt;receiptTemp+=L".tmp";plain(receiptTemp);
 constexpr i64 limit=256LL*1024*1024;
 if(auto saved=inspectDashResource(folder,binding,*cancel))return *saved;
 const auto original=Url(urls.front());std::exception_ptr failure;
 for(const auto& source:urls){
  cancel->check();try{
   std::string address=source;
   for(int hop=0;hop<6;++hop){
    cancel->check();const Url current(address);if(current.scheme!="http"&&current.scheme!="https")throw std::runtime_error("DASH resources require HTTP or HTTPS.");
    if(original.scheme=="https"&&current.scheme!="https")throw std::runtime_error("DASH resource HTTPS downgrade rejected.");
    const auto scope=headers.find(current.origin);Headers requestHeaders=scope==headers.end()?Headers{}:scope->second;for(auto it=requestHeaders.begin();it!=requestHeaders.end();){auto name=lower(it->first);if(name=="range"||name=="if-range"||name=="if-none-match"||name=="if-modified-since")it=requestHeaders.erase(it);else ++it;}
    // Every alternative and redirect resolves its own credential scope. Disable
    // implicit browser/site credentials so anonymous origins stay anonymous.
    Json settings=prefs;settings.erase("ActiveBrowserSession");settings["SiteLogins"]=Json::array();settings["WindowsLoginIntranet"]=false;
    Http response(address,requestHeaders,settings,*cancel,{},{},"",nullptr,false);
    if(response.status>=300&&response.status<400){auto location=response.header(L"Location");if(location.empty())throw std::runtime_error("DASH resource redirect has no destination.");auto next=combineUrl(address,location);if(current.scheme=="https"&&Url(next).scheme!="https")throw std::runtime_error("DASH resource HTTPS downgrade rejected.");address=std::move(next);continue;}
    if(response.status!=200)throw HttpRejected(response.status,response.header(L"Retry-After"));
    auto encoding=lower(trim(response.header(L"Content-Encoding")));if(!encoding.empty()&&encoding!="identity")throw std::runtime_error("Unexpected encoded DASH resource.");
    const auto type=lower(response.header(L"Content-Type"));if(type.find("text/html")!=std::string::npos||type.find("application/xhtml")!=std::string::npos)throw std::runtime_error("DASH resource returned a web page.");
    i64 expected=-1;const auto length=trim(response.header(L"Content-Length"));if(!length.empty()){expected=0;for(char c:length){if(c<'0'||c>'9'||expected>(limit-(c-'0'))/10)throw std::runtime_error("Invalid DASH resource length.");expected=expected*10+c-'0';}}
    plain(temporary);i64 written=0;{
     std::ofstream output(temporary,std::ios::binary|std::ios::trunc);if(!output)throw std::runtime_error("Cannot create DASH resource file.");char buffer[65536];
     for(;;){cancel->check();const auto bytes=response.read(buffer,sizeof(buffer),*cancel);if(!bytes)break;if(bytes>static_cast<size_t>(limit-written)||(expected>=0&&bytes>static_cast<size_t>(expected-written)))throw std::runtime_error("DASH resource exceeds its declared size.");if(onBytes)onBytes(bytes);output.write(buffer,bytes);if(!output)throw std::runtime_error("Cannot write DASH resource file.");written+=bytes;}
     output.flush();if(!output)throw std::runtime_error("Cannot flush DASH resource file.");
    }
    if(written<=0||(expected>=0&&written!=expected))throw std::runtime_error("DASH resource was truncated.");cancel->check();const auto digest=fileHash(temporary,[&]{cancel->check();});plain(destination);
    if(!MoveFileExW(temporary.c_str(),destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot preserve DASH resource file.");
    const Json saved={{"binding",key},{"bytes",written},{"sha256",digest}};plain(receipt);plain(receiptTemp);atomicText(receipt,Json{{"schema",1},{"protected",protect(saved.dump())}}.dump(),false);
    return {destination,written,digest,false};
   }
   throw std::runtime_error("DASH resource redirect limit reached.");
  }catch(...){failure=std::current_exception();std::error_code ignored;plain(temporary);fs::remove(temporary,ignored);cancel->check();}
 }
 if(failure)std::rethrow_exception(failure);throw std::runtime_error("No DASH resource location succeeded.");
}

Json captureDashWindow(const fs::path& directory,const DashLiveUpdate& update,const DashResourceHeaders& headers,const Json& prefs,const std::shared_ptr<Cancel>& cancel,const std::function<void(size_t,size_t,const Cancel&)>& onBytes,std::optional<DashCaptureInterval> interval,size_t concurrency){
 if(!cancel)throw std::runtime_error("DASH capture requires cancellation.");cancel->check();
 if(update.state.tracks.empty()||update.state.tracks.size()>3||update.windows.size()!=update.state.tracks.size())throw std::runtime_error("Invalid DASH capture tracks.");
 const auto folder=fs::absolute(directory).lexically_normal(),path=folder/L"recording.json";
 auto plain=[](const fs::path& value){auto flags=GetFileAttributesW(value.c_str());if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("DASH recording index must not be redirected.");};
 for(auto parent=folder;!parent.empty();){plain(parent);auto next=parent.parent_path();if(next==parent)break;parent=std::move(next);}fs::create_directories(folder);plain(path);auto temporary=path;temporary+=L".tmp";plain(temporary);
 Json identities=Json::array();std::set<std::string> distinct;for(const auto& track:update.state.tracks){auto identity=dashSegmentBinding(track,{0,0,1});identity.erase("number");identity.erase("time");identity.erase("duration");if(!distinct.insert(adaptiveSelectionDigest(identity)).second)throw std::runtime_error("Duplicate DASH recording track identity.");identities.push_back(std::move(identity));}
 const auto selection=adaptiveSelectionDigest(identities);Json index;
 if(fs::exists(path)){
  auto envelope=Json::parse(readText(path,16*1024*1024));if(num(envelope,"schema")!=1)throw std::runtime_error("Unsupported DASH recording index.");index=Json::parse(reveal(str(envelope,"protected")));
  if(!index.is_object()||str(index,"selection")!=selection||!index.contains("tracks")||!index["tracks"].is_array()||index["tracks"].size()!=identities.size())throw std::runtime_error("DASH recording selection changed; existing files were retained.");
  if(yes(index,"sealed"))throw std::runtime_error("DASH recording is sealed and cannot accept more segments.");
 }else{index={{"schema",1},{"selection",selection},{"tracks",Json::array()}};for(const auto& identity:identities)index["tracks"].push_back({{"identity",identity},{"segments",Json::object()}});}
 if(interval){
  if(!std::isfinite(interval->start)||interval->start<0||interval->start>7*86400||(interval->end&&(!std::isfinite(*interval->end)||*interval->end<=interval->start||*interval->end>7*86400)))throw std::runtime_error("Invalid DASH capture interval.");
  if(index.contains("captureStart")&&(!index["captureStart"].is_number()||std::abs(index["captureStart"].get<double>()-interval->start)>0.000001))throw std::runtime_error("DASH capture start cannot change during recording.");
  if(index.contains("captureEnd")&&!index["captureEnd"].is_null()&&(!interval->end||!index["captureEnd"].is_number()||std::abs(index["captureEnd"].get<double>()-*interval->end)>0.000001))throw std::runtime_error("DASH capture stop boundary cannot change after it is set.");
  index["captureStart"]=interval->start;index["captureEnd"]=interval->end?Json(*interval->end):Json();
 }
 size_t total=0;
 for(size_t t=0;t<identities.size();++t){
  auto& track=index["tracks"][t];if(!track.is_object()||!track.contains("identity")||track["identity"]!=identities[t]||!track.contains("segments")||!track["segments"].is_object())throw std::runtime_error("Invalid DASH recording track.");
  std::map<i64,DashSegmentReference> ordered;
  for(auto it=track["segments"].begin();it!=track["segments"].end();++it){cancel->check();const auto& record=it.value();if(!record.is_object()||!record.contains("number")||!record["number"].is_number_integer()||!record.contains("time")||!record["time"].is_number_integer()||!record.contains("duration")||!record["duration"].is_number_integer())throw std::runtime_error("Invalid DASH recording segment.");
   DashSegmentReference ref{num(record,"number"),num(record,"time"),num(record,"duration")};if(it.key()!=std::to_string(ref.number)||ref.number<0||ref.time<0||ref.duration<=0||str(record,"binding")!=adaptiveSelectionDigest(dashSegmentBinding(update.state.tracks[t],ref)))throw std::runtime_error("DASH saved segment identity changed.");ordered.emplace(ref.number,ref);
  }
  for(const auto& segment:update.windows[t].segments){cancel->check();const auto& ref=segment.reference;const auto binding=adaptiveSelectionDigest(dashSegmentBinding(update.state.tracks[t],ref));const auto key=std::to_string(ref.number);auto found=ordered.find(ref.number);
   if(found!=ordered.end()){if(found->second.time!=ref.time||found->second.duration!=ref.duration)throw std::runtime_error("DASH refresh replaced an indexed segment.");}
   else{ordered.emplace(ref.number,ref);track["segments"][key]={{"number",ref.number},{"time",ref.time},{"duration",ref.duration},{"binding",binding},{"committed",false}};}
   track["segments"][key]["urls"]=segment.urls;
  }
  track["periodDuration"]=update.state.tracks[t].addressing.period.duration?Json(*update.state.tracks[t].addressing.period.duration):Json();
  if(!update.windows[t].segments.empty())track["pendingInitialization"]=update.windows[t].initialization;
  std::optional<DashSegmentReference> last;for(const auto& entry:ordered){const auto& ref=entry.second;if(last&&(last->number==std::numeric_limits<i64>::max()||ref.number!=last->number+1||last->time>std::numeric_limits<i64>::max()-last->duration||ref.time!=last->time+last->duration))throw std::runtime_error("DASH recording has a segment gap or overlap.");last=ref;}
  total+=ordered.size();if(total>10000)throw std::runtime_error("DASH recording exceeds its segment limit.");
 }
 auto save=[&]{cancel->check();const auto text=index.dump();if(text.size()>8*1024*1024)throw std::runtime_error("DASH recording index exceeds its storage limit.");plain(path);plain(temporary);atomicText(path,Json{{"schema",1},{"protected",protect(text)}}.dump(),false);};
 // Persist identities and pending URLs before any new resource is requested.
 save();
 auto stored=[](const DashStoredResource& resource){return Json{{"file",utf8(resource.path.filename().wstring())},{"bytes",resource.bytes},{"sha256",resource.sha256}};};
 auto group=std::make_shared<Cancel>();group->parent=cancel;std::mutex checkpointMutex;
 std::vector<Json> drafts;for(const auto& track:index["tracks"])drafts.push_back(track);
 auto downloadTrack=[&](size_t t){
  group->check();const auto& window=update.windows[t];if(window.segments.empty())return;auto& track=drafts[t];
  auto checkpoint=[&]{std::lock_guard<std::mutex> lock(checkpointMutex);group->check();index["tracks"][t]=track;save();};
  auto account=[&](size_t bytes){if(onBytes)onBytes(t,bytes,*group);};
  if(!window.initialization.empty()){
   const Json binding={{"schema",1},{"kind","initialization"},{"track",identities[t]},{"urls",window.initialization}};
   auto initialization=storeDashResource(folder,binding,window.initialization,headers,prefs,group,account);
   if(track.contains("initialization")&&lower(str(track["initialization"],"sha256"))!=lower(initialization.sha256))throw std::runtime_error("DASH initialization changed within the selected Period.");
   track["initialization"]=stored(initialization);track["initialization"]["urls"]=window.initialization;track["initialization"]["binding"]=adaptiveSelectionDigest(binding);track["initializationVerified"]=true;track.erase("pendingInitialization");checkpoint();
  }else if(track.contains("initialization"))throw std::runtime_error("DASH initialization disappeared during capture.");else track.erase("pendingInitialization");
  for(const auto& segment:window.segments){
   auto resource=storeDashResource(folder,dashSegmentBinding(update.state.tracks[t],segment.reference),segment.urls,headers,prefs,group,account);
   auto& record=track["segments"][std::to_string(segment.reference.number)];record["resource"]=stored(resource);record["committed"]=true;checkpoint();
  }
  checkpoint();
 };
 std::vector<size_t> active;for(size_t t=0;t<identities.size();++t)if(!update.windows[t].segments.empty())active.push_back(t);
 const size_t workers=std::min(active.size(),std::clamp<size_t>(concurrency,1,3));
 if(workers<=1){for(auto t:active)downloadTrack(t);}
 else{
  std::atomic_size_t next{0};std::vector<std::future<void>> tasks;
  try{for(size_t worker=0;worker<workers;++worker)tasks.push_back(std::async(std::launch::async,[&]{try{for(;;){group->check();const auto slot=next++;if(slot>=active.size())break;downloadTrack(active[slot]);}}catch(...){group->stop=true;throw;}}));}
  catch(...){group->stop=true;for(auto& task:tasks)task.wait();throw;}
  std::exception_ptr failure;for(auto& task:tasks)try{task.get();}catch(const Cancelled&){if(!failure)failure=std::current_exception();}catch(...){failure=std::current_exception();}
  cancel->check();if(failure)std::rethrow_exception(failure);
 }
 index["staticPresentation"]=update.staticPresentation;save();return index;
}

Json reconcileDashRecording(const fs::path& directory,const Cancel& cancel){
 cancel.check();const auto folder=fs::absolute(directory).lexically_normal(),path=folder/L"recording.json";
 auto plain=[](const fs::path& value){auto flags=GetFileAttributesW(value.c_str());if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("DASH recovery index must not be redirected.");};
 for(auto parent=folder;!parent.empty();){plain(parent);auto next=parent.parent_path();if(next==parent)break;parent=std::move(next);}plain(path);auto temporary=path;temporary+=L".tmp";plain(temporary);
 auto envelope=Json::parse(readText(path,16*1024*1024));if(num(envelope,"schema")!=1)throw std::runtime_error("Unsupported DASH recovery index.");auto index=Json::parse(reveal(str(envelope,"protected")));
 if(!index.is_object()||num(index,"schema")!=1||!index.contains("tracks")||!index["tracks"].is_array()||index["tracks"].empty()||index["tracks"].size()>3)throw std::runtime_error("Invalid DASH recovery tracks.");
 Json identities=Json::array();for(const auto& track:index["tracks"]){if(!track.is_object()||!track.contains("identity")||!track["identity"].is_object()||!track.contains("segments")||!track["segments"].is_object())throw std::runtime_error("Invalid DASH recovery track.");identities.push_back(track["identity"]);}
 if(adaptiveSelectionDigest(identities)!=str(index,"selection"))throw std::runtime_error("DASH recovery selection identity changed.");
 auto stored=[](const DashStoredResource& resource){return Json{{"file",utf8(resource.path.filename().wstring())},{"bytes",resource.bytes},{"sha256",resource.sha256}};};
 size_t count=0;for(auto& track:index["tracks"]){
  cancel.check();std::map<i64,DashSegmentReference> ordered;
  for(auto it=track["segments"].begin();it!=track["segments"].end();++it){
   cancel.check();if(++count>10000)throw std::runtime_error("DASH recovery exceeds its segment limit.");auto& record=it.value();
   if(!record.is_object()||!record.contains("number")||!record["number"].is_number_integer()||!record.contains("time")||!record["time"].is_number_integer()||!record.contains("duration")||!record["duration"].is_number_integer())throw std::runtime_error("Invalid DASH recovery segment.");
   DashSegmentReference ref{num(record,"number"),num(record,"time"),num(record,"duration")};if(ref.number<0||ref.time<0||ref.duration<=0||ref.time>std::numeric_limits<i64>::max()-ref.duration||it.key()!=std::to_string(ref.number))throw std::runtime_error("Invalid DASH recovery timeline.");
   Json binding=track["identity"];binding["number"]=ref.number;binding["time"]=ref.time;binding["duration"]=ref.duration;
   if(adaptiveSelectionDigest(binding)!=str(record,"binding"))throw std::runtime_error("DASH recovery segment binding changed.");ordered.emplace(ref.number,ref);
   auto resource=inspectDashResource(folder,binding,cancel);record["committed"]=resource.has_value();if(resource)record["resource"]=stored(*resource);else record.erase("resource");
  }
  std::optional<DashSegmentReference> last;for(const auto& entry:ordered){const auto& ref=entry.second;if(last&&(last->number==std::numeric_limits<i64>::max()||ref.number!=last->number+1||ref.time!=last->time+last->duration))throw std::runtime_error("DASH recovery timeline has a gap or overlap.");last=ref;}
  track["initializationVerified"]=false;
  auto verifyInitialization=[&](const Json& urls)->std::optional<DashStoredResource>{if(!urls.is_array()||urls.empty()||urls.size()>32)throw std::runtime_error("Invalid DASH recovery initialization URLs.");for(const auto& url:urls)if(!url.is_string()||url.get_ref<const std::string&>().size()>32768)throw std::runtime_error("Invalid DASH recovery initialization URL.");return inspectDashResource(folder,Json{{"schema",1},{"kind","initialization"},{"track",track["identity"]},{"urls",urls}},cancel);};
  if(track.contains("initialization")&&track["initialization"].is_object()&&track["initialization"].contains("urls")){
   const auto urls=track["initialization"]["urls"];auto resource=verifyInitialization(urls);if(resource){auto value=stored(*resource);value["urls"]=urls;value["binding"]=adaptiveSelectionDigest(Json{{"schema",1},{"kind","initialization"},{"track",track["identity"]},{"urls",urls}});track["initialization"]=std::move(value);track["initializationVerified"]=true;}
  }
  if(track.contains("pendingInitialization")&&(!track["pendingInitialization"].is_array()||track["pendingInitialization"].size()>32))throw std::runtime_error("Invalid DASH pending initialization.");
  if(track.contains("pendingInitialization")&&track["pendingInitialization"].empty()){if(track.contains("initialization"))throw std::runtime_error("DASH initialization disappeared from pending capture.");track.erase("pendingInitialization");}
  if(track.contains("pendingInitialization")){
   const auto urls=track["pendingInitialization"];auto resource=verifyInitialization(urls);if(resource){if(track.contains("initialization")&&lower(str(track["initialization"],"sha256"))!=lower(resource->sha256))throw std::runtime_error("DASH recovered initialization changed within the Period.");auto value=stored(*resource);value["urls"]=urls;value["binding"]=adaptiveSelectionDigest(Json{{"schema",1},{"kind","initialization"},{"track",track["identity"]},{"urls",urls}});track["initialization"]=std::move(value);track["initializationVerified"]=true;track.erase("pendingInitialization");}
  }
 }
 cancel.check();const auto text=index.dump();if(text.size()>8*1024*1024)throw std::runtime_error("DASH recovery index exceeds its storage limit.");plain(path);plain(temporary);atomicText(path,Json{{"schema",1},{"protected",protect(text)}}.dump(),false);return index;
}

Json sealDashRecording(const fs::path& directory,const Cancel& cancel){
 auto index=reconcileDashRecording(directory,cancel);if(yes(index,"sealed"))return index;
 double begin=0,end=std::numeric_limits<double>::infinity();
 struct Coverage {double begin=0,end=0;};std::vector<Coverage> coverage;
 constexpr i64 exact=9007199254740991LL;
 auto seconds=[&](const Json& record,const Json& identity,bool after){
  const i64 scale=num(identity,"timescale"),offset=num(identity,"presentationTimeOffset"),time=num(record,"time"),duration=num(record,"duration");
  if(scale<=0||scale>exact||offset<0||offset>exact||time<0||duration<=0||time>std::numeric_limits<i64>::max()-duration)throw std::runtime_error("Invalid DASH stop clock.");
  const i64 ticks=(after?time+duration:time)-offset;if(ticks< -exact||ticks>exact)throw std::runtime_error("DASH stop clock exceeds supported precision.");return static_cast<double>(ticks)/scale;
 };
 for(const auto& track:index["tracks"]){
  cancel.check();if(track.contains("pendingInitialization")||(track.contains("initialization")&&!yes(track,"initializationVerified")))throw std::runtime_error("DASH initialization must finish before saving the recording.");
  std::map<i64,Json> ordered;for(const auto& record:track["segments"])ordered.emplace(num(record,"number"),record);
  Coverage current;bool first=true;for(const auto& entry:ordered){const auto& record=entry.second;if(!yes(record,"committed")||!record.contains("resource"))break;
   if(first){current.begin=seconds(record,track["identity"],false);first=false;}current.end=seconds(record,track["identity"],true);
  }
  if(first)throw std::runtime_error("DASH recording has no complete segment prefix to save.");
  if(track.contains("periodDuration")&&!track["periodDuration"].is_null()){if(!track["periodDuration"].is_number())throw std::runtime_error("Invalid DASH stop Period duration.");const double duration=track["periodDuration"].get<double>();if(!std::isfinite(duration)||duration<=0)throw std::runtime_error("Invalid DASH stop Period duration.");current.end=std::min(current.end,duration);}
  begin=std::max(begin,current.begin);end=std::min(end,current.end);coverage.push_back(current);
 }
 if(index.contains("captureStart")){if(!index["captureStart"].is_number())throw std::runtime_error("Invalid DASH capture start at stop.");begin=index["captureStart"].get<double>();}
 if(index.contains("captureEnd")&&!index["captureEnd"].is_null()){if(!index["captureEnd"].is_number())throw std::runtime_error("Invalid DASH capture end at stop.");end=index["captureEnd"].get<double>();}
 if(!std::isfinite(begin)||!std::isfinite(end)||begin<0||end<=begin||end>7*86400)throw std::runtime_error("DASH recording has no complete common interval to save.");
 for(const auto& track:coverage)if(track.begin>begin+0.000001||track.end+0.000001<end)throw std::runtime_error("DASH recording has not captured the requested stop interval.");
 for(auto& track:index["tracks"]){
  Json excluded=Json::object();for(auto it=track["segments"].begin();it!=track["segments"].end();){cancel.check();const auto& record=it.value();
   const bool overlaps=seconds(record,track["identity"],false)<end&&seconds(record,track["identity"],true)>begin;
   if(overlaps){if(!yes(record,"committed"))throw std::runtime_error("DASH stop interval contains unfinished media.");++it;}
   else{excluded[it.key()]=record;it=track["segments"].erase(it);}
  }
  if(track["segments"].empty())throw std::runtime_error("DASH stop interval has an empty track.");if(!excluded.empty())track["excludedSegments"]=std::move(excluded);
 }
 index["captureStart"]=begin;index["captureEnd"]=end;index["sealed"]=true;
 const auto path=fs::absolute(directory).lexically_normal()/L"recording.json";auto temporary=path;temporary+=L".tmp";
 for(const auto& file:{path,temporary}){const auto flags=GetFileAttributesW(file.c_str());if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("DASH sealed index must not be redirected.");}
 cancel.check();const auto text=index.dump();if(text.size()>8*1024*1024)throw std::runtime_error("DASH sealed index exceeds its storage limit.");atomicText(path,Json{{"schema",1},{"protected",protect(text)}}.dump(),false);return index;
}

namespace {
std::string dashAssemblyHash(HANDLE file,const Cancel& cancel){
 LARGE_INTEGER beginning{};if(!SetFilePointerEx(file,beginning,nullptr,FILE_BEGIN))throw std::runtime_error("Cannot seek DASH assembled output.");
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Cannot initialize DASH output verification.");
 try{
  if(BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)<0)throw std::runtime_error("Cannot initialize DASH output verification.");
  BYTE buffer[65536];for(;;){cancel.check();DWORD count=0;if(!ReadFile(file,buffer,sizeof(buffer),&count,nullptr))throw std::runtime_error("Cannot read DASH assembled output.");if(!count)break;if(BCryptHashData(hash,buffer,count,0)<0)throw std::runtime_error("DASH output verification failed.");}
  BYTE digest[32];if(BCryptFinishHash(hash,digest,sizeof(digest),0)<0)throw std::runtime_error("DASH output verification failed.");
  BCryptDestroyHash(hash);hash=nullptr;BCryptCloseAlgorithmProvider(algorithm,0);algorithm=nullptr;std::ostringstream text;for(auto byte:digest)text<<std::hex<<std::setfill('0')<<std::setw(2)<<(int)byte;return text.str();
 }catch(...){if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);throw;}
}
void dashAssemblyReceiptPaths(const fs::path& folder,bool session=false){
 for(const auto* name:session?std::initializer_list<const wchar_t*>{L"session-assembled.json",L"session-assembled.json.tmp"}:std::initializer_list<const wchar_t*>{L"assembled.json",L"assembled.json.tmp"}){const auto flags=GetFileAttributesW((folder/name).c_str());if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("DASH assembly receipt must not be redirected.");}
}
std::optional<DashRecordingOutput> restoreDashAssembly(const fs::path& folder,const Json& index,const Cancel& cancel,bool session=false){
 cancel.check();if(!yes(index,"sealed"))return {};dashAssemblyReceiptPaths(folder,session);const auto path=folder/(session?L"session-assembled.json":L"assembled.json");if(!fs::exists(path))return {};
 try{
  const auto envelope=Json::parse(readText(path,65536));if(num(envelope,"schema")!=1)return {};const auto text=reveal(str(envelope,"protected"));if(text.size()>32768)return {};const auto saved=Json::parse(text);
  if(num(saved,"schema")!=1||str(saved,"recording")!=adaptiveSelectionDigest(index))return {};
  const auto name=str(saved,"file");if(!std::regex_match(name,std::regex("assembled-[0-9a-f]{32}\\.(mp4|m4a)")))return {};
  DashRecordingOutput result;result.staging=folder/wide(name);result.bytes=num(saved,"bytes");result.sha256=lower(str(saved,"sha256"));result.start=real(saved,"start");result.duration=real(saved,"duration");
  if(result.bytes<=0||!std::regex_match(result.sha256,std::regex("[0-9a-f]{64}"))||!std::isfinite(result.start)||!std::isfinite(result.duration)||result.start<0||result.duration<=0||result.start+result.duration>7*86400||!saved.contains("inputs")||!saved["inputs"].is_array()||saved["inputs"].size()>3)return {};
  if(std::abs(result.start-real(index,"captureStart"))>0.000001||std::abs(result.start+result.duration-real(index,"captureEnd"))>0.000001)return {};
  if(session&&saved["inputs"].size()!=1)return {};
  for(const auto& input:saved["inputs"]){if(!input.is_string())return {};const auto file=input.get<std::string>();if(!std::regex_match(file,std::regex(session?"session-[0-9a-f]{32}\\.ffconcat":"assembly-[0-9a-f]{32}-[0-2]\\.bin")))return {};result.temporaryInputs.push_back(folder/wide(file));}
  Handle source(CreateFileW(result.staging.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,nullptr));if(!source)return {};
  BY_HANDLE_FILE_INFORMATION info{};LARGE_INTEGER size{};if(!GetFileInformationByHandle(source.h,&info)||(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||!GetFileSizeEx(source.h,&size)||size.QuadPart!=result.bytes||dashAssemblyHash(source.h,cancel)!=result.sha256)return {};
  cancel.check();result.reused=true;return result;
 }catch(const Cancelled&){throw;}catch(const std::exception&){cancel.check();return {};}
}
void checkpointDashAssembly(const fs::path& folder,const Json& index,DashRecordingOutput& output,const Cancel& cancel,bool session=false){
 cancel.check();dashAssemblyReceiptPaths(folder,session);
 if(output.staging.parent_path()!=folder||!std::regex_match(utf8(output.staging.filename().wstring()),std::regex("assembled-[0-9a-f]{32}\\.(mp4|m4a)")))throw std::runtime_error("Invalid DASH assembly checkpoint path.");
 // Flush the completed media before committing a receipt that claims it exists.
 Handle source(CreateFileW(output.staging.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));if(!source)throw std::runtime_error("Cannot lock the DASH assembly checkpoint.");
 BY_HANDLE_FILE_INFORMATION info{};LARGE_INTEGER size{};if(!GetFileInformationByHandle(source.h,&info)||(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||!GetFileSizeEx(source.h,&size)||size.QuadPart!=output.bytes||!FlushFileBuffers(source.h))throw std::runtime_error("DASH assembled output changed before checkpoint.");
 output.sha256=dashAssemblyHash(source.h,cancel);if(!yes(index,"sealed"))return;
 Json inputs=Json::array();for(const auto& input:output.temporaryInputs){if(input.parent_path()!=folder)throw std::runtime_error("Invalid DASH assembly input checkpoint.");inputs.push_back(utf8(input.filename().wstring()));}
 const Json saved={{"schema",1},{"recording",adaptiveSelectionDigest(index)},{"file",utf8(output.staging.filename().wstring())},{"bytes",output.bytes},{"sha256",output.sha256},{"start",output.start},{"duration",output.duration},{"inputs",inputs}};
 const auto envelope=Json{{"schema",1},{"protected",protect(saved.dump())}}.dump();if(envelope.size()>65536)throw std::runtime_error("DASH assembly receipt exceeds its storage limit.");cancel.check();dashAssemblyReceiptPaths(folder,session);atomicText(folder/(session?L"session-assembled.json":L"assembled.json"),envelope,false);
}
}

static DashRecordingOutput assembleDashRecordingIndex(const fs::path& folder,const Json& index,const Cancel& cancel){
 cancel.check();
 if(auto saved=restoreDashAssembly(folder,index,cancel))return *saved;
 struct Input {Json identity;std::vector<Json> resources;double start=0,end=0,offset=0;std::string kind;};
 std::vector<Input> inputs;double begin=0,end=std::numeric_limits<double>::infinity();size_t videos=0,audios=0;Json selection={{"audioOnly",true},{"audioExpected",false}};std::optional<std::string> period;
 for(const auto& track:index["tracks"]){
  cancel.check();if(track.contains("pendingInitialization"))throw std::runtime_error("DASH initialization is still pending.");Input input;input.identity=track["identity"];
  const auto currentPeriod=str(input.identity,"period");if(!period)period=currentPeriod;else if(*period!=currentPeriod)throw std::runtime_error("DASH recording assembly cannot mix Periods.");
  const auto& format=input.identity["format"];input.kind=str(format,"contentType");if(input.kind.empty()){const auto mime=str(format,"mimeType");if(mime.find("video/")==0)input.kind="video";else if(mime.find("audio/")==0)input.kind="audio";}
  if(input.kind=="video"){if(++videos>1)throw std::runtime_error("DASH assembly contains multiple video tracks.");selection["audioOnly"]=false;auto height=str(format,"height");if(!height.empty()){size_t used=0;auto value=std::stoll(height,&used);if(used!=height.size()||value<=0||value>32768)throw std::runtime_error("Invalid DASH recording height.");selection["height"]=value;}}
  else if(input.kind=="audio"){if(++audios>1)throw std::runtime_error("DASH assembly contains multiple audio tracks.");selection["audioExpected"]=true;selection["audioLanguage"]=str(format,"lang");}
  else throw std::runtime_error("DASH live assembly currently requires audio/video tracks.");
  const i64 scale=num(input.identity,"timescale"),offset=num(input.identity,"presentationTimeOffset");if(scale<=0||offset<0)throw std::runtime_error("Invalid DASH recording clock.");input.offset=static_cast<double>(offset)/scale;
  if(track.contains("initialization")){if(!yes(track,"initializationVerified"))throw std::runtime_error("DASH initialization has not been verified.");input.resources.push_back(track["initialization"]);}
  std::map<i64,Json> ordered;for(const auto& record:track["segments"]){if(!yes(record,"committed")||!record.contains("resource"))throw std::runtime_error("DASH recording contains unfinished media.");ordered.emplace(num(record,"number"),record);}
  if(ordered.empty())throw std::runtime_error("DASH recording track is empty.");
  const auto& first=ordered.begin()->second;const auto& last=ordered.rbegin()->second;
  const i64 firstTicks=num(first,"time")-offset,lastTicks=num(last,"time")+num(last,"duration")-offset;
  constexpr i64 exact=9007199254740991LL;if(firstTicks>exact||firstTicks< -exact||lastTicks>exact||lastTicks< -exact||scale>exact||offset>exact)throw std::runtime_error("DASH assembly clock exceeds supported precision.");
  input.start=static_cast<double>(firstTicks)/scale;input.end=static_cast<double>(lastTicks)/scale;
  if(track.contains("periodDuration")&&!track["periodDuration"].is_null()){if(!track["periodDuration"].is_number())throw std::runtime_error("Invalid DASH saved Period duration.");const double duration=track["periodDuration"].get<double>();if(!std::isfinite(duration)||duration<=0)throw std::runtime_error("Invalid DASH saved Period duration.");input.end=std::min(input.end,duration);}
  for(const auto& entry:ordered)input.resources.push_back(entry.second["resource"]);
  begin=std::max(begin,input.start);end=std::min(end,input.end);inputs.push_back(std::move(input));
 }
 const bool explicitInterval=index.contains("captureStart");
 if(explicitInterval){
  if(!index["captureStart"].is_number())throw std::runtime_error("Invalid DASH saved capture start.");const double requestedStart=index["captureStart"].get<double>();
  if(!std::isfinite(requestedStart)||requestedStart<0||requestedStart>7*86400)throw std::runtime_error("Invalid DASH saved capture start.");
  std::optional<double> requestedEnd;if(index.contains("captureEnd")&&!index["captureEnd"].is_null()){if(!index["captureEnd"].is_number())throw std::runtime_error("Invalid DASH saved capture end.");requestedEnd=index["captureEnd"].get<double>();if(!std::isfinite(*requestedEnd)||*requestedEnd<=requestedStart||*requestedEnd>7*86400)throw std::runtime_error("Invalid DASH saved capture end.");}
  for(const auto& input:inputs)if(input.start>requestedStart+0.000001||(requestedEnd&&input.end+0.000001<*requestedEnd))throw std::runtime_error("DASH recording has not captured the full requested interval.");
  begin=requestedStart;if(requestedEnd)end=*requestedEnd;
 }
 if(inputs.empty()||!std::isfinite(begin)||!std::isfinite(end)||end<=begin||end-begin>7*86400)throw std::runtime_error("DASH recorded tracks have no usable common interval.");
 if(!explicitInterval)for(const auto& input:inputs)if(begin-std::max(0.0,input.start)>0.5||input.end-end>0.5)throw std::runtime_error("DASH track coverage is not aligned; retain the recording and finish pending capture before assembly.");
 DashRecordingOutput result;result.start=begin;result.duration=end-begin;const auto token=wide(guid());result.staging=folder/(L"assembled-"+token+(videos?L".mp4":L".m4a"));
 std::vector<std::wstring> args={L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-n",L"-copyts"};
 for(size_t i=0;i<inputs.size();++i){
  cancel.check();const auto inputPath=folder/(L"assembly-"+token+L"-"+std::to_wstring(i)+L".bin");result.temporaryInputs.push_back(inputPath);
  Handle output(CreateFileW(inputPath.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));if(!output)throw std::runtime_error("Cannot create DASH assembly input.");
  for(const auto& resource:inputs[i].resources){
   cancel.check();const auto name=str(resource,"file");if(!std::regex_match(name,std::regex("[0-9a-f]{64}\\.part")))throw std::runtime_error("Invalid DASH assembly part name.");const auto sourcePath=folder/wide(name);
   Handle source(CreateFileW(sourcePath.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,nullptr));if(!source)throw std::runtime_error("Cannot lock DASH assembly source.");
   BY_HANDLE_FILE_INFORMATION info{};LARGE_INTEGER size{};if(!GetFileInformationByHandle(source.h,&info)||(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||!GetFileSizeEx(source.h,&size)||size.QuadPart!=num(resource,"bytes")||lower(fileHash(sourcePath,[&]{cancel.check();}))!=lower(str(resource,"sha256")))throw std::runtime_error("DASH assembly source changed after recovery.");
   char buffer[65536];i64 copied=0;for(;;){cancel.check();DWORD read=0;if(!ReadFile(source.h,buffer,sizeof(buffer),&read,nullptr))throw std::runtime_error("Cannot read DASH assembly source.");if(!read)break;DWORD written=0;if(!WriteFile(output.h,buffer,read,&written,nullptr)||written!=read)throw std::runtime_error("Cannot write DASH assembly input.");copied+=read;}
   if(copied!=size.QuadPart)throw std::runtime_error("DASH assembly source was truncated.");
  }
  if(!FlushFileBuffers(output.h))throw std::runtime_error("Cannot flush DASH assembly input.");CloseHandle(output.h);output.h=INVALID_HANDLE_VALUE;
  Json expected={{"audioOnly",inputs[i].kind=="audio"},{"audioExpected",inputs[i].kind=="audio"}};if(inputs[i].kind=="video"&&selection.contains("height"))expected["height"]=selection["height"];
  dashAssemblyStreams(dashAssemblyProbe(inputPath,cancel),expected,false);
  args.insert(args.end(),{L"-protocol_whitelist",L"file,pipe",L"-itsoffset",dashSeconds(-inputs[i].offset-begin),L"-i",inputPath.wstring()});
 }
 for(size_t i=0;i<inputs.size();++i)args.insert(args.end(),{L"-map",std::to_wstring(i)+(inputs[i].kind=="video"?L":v:0":L":a:0")});
 args.insert(args.end(),{L"-ss",L"0",L"-t",dashSeconds(result.duration),L"-c",L"copy",L"-avoid_negative_ts",L"disabled",L"-movflags",L"+faststart"});appendAudioMetadata(args,selection);args.push_back(result.staging.wstring());
 execute(appDir()/L"tools"/L"ffmpeg.exe",args,300,cancel);const auto probe=dashAssemblyProbe(result.staging,cancel);dashAssemblyStreams(probe,selection,false);dashAssemblyDuration(probe,result.duration);
 cancel.check();const auto size=fs::file_size(result.staging);if(!size||size>static_cast<uintmax_t>(std::numeric_limits<i64>::max()))throw std::runtime_error("Invalid DASH assembled file size.");result.bytes=static_cast<i64>(size);checkpointDashAssembly(folder,index,result,cancel);return result;
}

DashRecordingOutput assembleDashRecording(const fs::path& directory,const Cancel& cancel){
 cancel.check();const auto folder=fs::absolute(directory).lexically_normal();const auto index=reconcileDashRecording(folder,cancel);return assembleDashRecordingIndex(folder,index,cancel);
}

DashRecordingOutput assembleDashRecordingSession(const fs::path& directory,const std::vector<std::string>& periods,const Json& selection,const Cancel& cancel){
 cancel.check();if(periods.empty()||periods.size()>128)throw std::runtime_error("Invalid DASH recording session Period count.");
 const auto folder=fs::absolute(directory).lexically_normal();const bool audioOnly=yes(selection,"audioOnly");const auto container=str(selection,"container");
 if(container!=(audioOnly?"m4a":"mp4")||(audioOnly&&!yes(selection,"audioExpected")))throw std::runtime_error("Invalid DASH recording session output selection.");
 std::set<std::string> folders,ids;std::string presentation;Json formats,reference;std::vector<DashRecordingOutput> outputs;std::vector<Handle> locks;
 Json sessionBinding={{"schema",1},{"sealed",true},{"selection",selection},{"periods",Json::array()}};
 double firstStart=0,lastEnd=0,lastPeriodStart=0;std::optional<double> lastPeriodEnd;std::ostringstream list;list.imbue(std::locale::classic());list<<"ffconcat version 1.0\n";
 for(size_t p=0;p<periods.size();++p){
  cancel.check();const auto& name=periods[p];if((name!="."&&!std::regex_match(name,std::regex("period-[0-9]{4}")))||(name=="."&&p!=0)||!folders.insert(name).second)throw std::runtime_error("Invalid DASH recording session folder.");
  const auto partFolder=(name=="."?folder:folder/wide(name));const auto index=reconcileDashRecording(partFolder,cancel);
  sessionBinding["periods"].push_back(Json{{"folder",name},{"recording",adaptiveSelectionDigest(index)}});
  if(!yes(index,"sealed")||!index.contains("captureStart")||!index["captureStart"].is_number()||!index.contains("captureEnd")||!index["captureEnd"].is_number())throw std::runtime_error("DASH session contains an unfinished Period.");
  const double begin=real(index,"captureStart"),end=real(index,"captureEnd");if(!std::isfinite(begin)||!std::isfinite(end)||begin<0||end<=begin||end>7*86400)throw std::runtime_error("Invalid DASH session capture interval.");
  const auto& first=index["tracks"].front()["identity"];const auto periodId=str(first,"period"),currentPresentation=str(first,"presentation");const double periodStart=real(first,"periodStart");
  if(periodId.empty()||currentPresentation.empty()||!first.contains("periodStart")||!first["periodStart"].is_number()||!std::isfinite(periodStart)||periodStart<0||periodStart+end>7*86400||!ids.insert(periodId).second)throw std::runtime_error("Invalid or duplicate DASH session Period identity.");
  Json currentFormats=Json::array();std::optional<double> duration;
  for(const auto& track:index["tracks"]){const auto& identity=track["identity"];if(str(identity,"period")!=periodId||str(identity,"presentation")!=currentPresentation||!identity.contains("periodStart")||!identity["periodStart"].is_number()||!std::isfinite(real(identity,"periodStart"))||std::abs(real(identity,"periodStart")-periodStart)>0.000001)throw std::runtime_error("DASH session tracks disagree on their Period.");currentFormats.push_back(identity["format"]);
   if(track.contains("periodDuration")&&!track["periodDuration"].is_null()){const double value=real(track,"periodDuration");if(!track["periodDuration"].is_number()||!std::isfinite(value)||value<=0||value>7*86400||(duration&&std::abs(*duration-value)>0.000001))throw std::runtime_error("DASH session tracks disagree on their Period end.");duration=value;}
  }
  if(duration&&end>*duration+0.000001)throw std::runtime_error("DASH session capture exceeds its Period end.");
  const double absoluteStart=periodStart+begin,absoluteEnd=periodStart+end;
  if(p==0){presentation=currentPresentation;formats=currentFormats;firstStart=absoluteStart;}
  else if(currentPresentation!=presentation||currentFormats!=formats||periodStart<=lastPeriodStart||!lastPeriodEnd||std::abs(*lastPeriodEnd-periodStart)>0.000001||std::abs(lastEnd-absoluteStart)>0.000001||std::abs(begin)>0.000001)throw std::runtime_error("DASH session Periods changed selection or contain a gap or overlap.");
  auto output=assembleDashRecordingIndex(partFolder,index,cancel);if(std::abs(output.start-begin)>0.000001||std::abs(output.duration-(end-begin))>0.000001)throw std::runtime_error("DASH session assembly interval changed.");
  Handle locked(CreateFileW(output.staging.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,nullptr));if(!locked)throw std::runtime_error("Cannot lock a DASH session Period output.");
  BY_HANDLE_FILE_INFORMATION info{};LARGE_INTEGER size{};if(!GetFileInformationByHandle(locked.h,&info)||(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||!GetFileSizeEx(locked.h,&size)||size.QuadPart!=output.bytes||dashAssemblyHash(locked.h,cancel)!=lower(output.sha256))throw std::runtime_error("DASH session Period output changed.");
  const auto probe=dashAssemblyProbe(output.staging,cancel);const auto signature=dashAssemblyStreams(probe,selection,false);dashAssemblyDuration(probe,output.duration);if(p==0)reference=signature;else if(reference!=signature)throw std::runtime_error("DASH session codec configuration or time base changed; stream-copy joining is unavailable.");
  const auto relative=(name=="."?std::string{}:name+"/")+utf8(output.staging.filename().wstring());list<<"file '"<<relative<<"'\nduration "<<std::fixed<<std::setprecision(9)<<output.duration<<"\n";
  locks.push_back(std::move(locked));outputs.push_back(std::move(output));lastEnd=absoluteEnd;lastPeriodStart=periodStart;lastPeriodEnd=duration?std::optional<double>{periodStart+*duration}:std::optional<double>{};
 }
 if(outputs.size()==1){auto result=std::move(outputs.front());result.start=firstStart;return result;}
 DashRecordingOutput result;result.start=firstStart;result.duration=lastEnd-firstStart;if(!std::isfinite(result.duration)||result.duration<=0||result.duration>7*86400)throw std::runtime_error("Invalid DASH session duration.");
 sessionBinding["captureStart"]=firstStart;sessionBinding["captureEnd"]=lastEnd;sessionBinding["streams"]=reference;
 if(auto saved=restoreDashAssembly(folder,sessionBinding,cancel,true))return *saved;
 const auto token=wide(guid());const auto manifest=folder/(L"session-"+token+L".ffconcat");result.temporaryInputs.push_back(manifest);result.staging=folder/(L"assembled-"+token+L"."+wide(container));
 const auto text=list.str();{Handle file(CreateFileW(manifest.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));DWORD written=0;if(!file||text.size()>65536||!WriteFile(file.h,text.data(),static_cast<DWORD>(text.size()),&written,nullptr)||written!=text.size()||!FlushFileBuffers(file.h))throw std::runtime_error("Cannot save the DASH session assembly list.");}
 Handle listLock(CreateFileW(manifest.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));if(!listLock)throw std::runtime_error("Cannot lock the DASH session assembly list.");
 std::vector<std::wstring> args={L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-n",L"-protocol_whitelist",L"file,pipe",L"-f",L"concat",L"-safe",L"1",L"-i",manifest.wstring(),L"-map",L"0",L"-c",L"copy",L"-movflags",L"+faststart"};appendAudioMetadata(args,selection);args.push_back(result.staging.wstring());execute(appDir()/L"tools"/L"ffmpeg.exe",args,300,cancel);
 const auto probe=dashAssemblyProbe(result.staging,cancel);if(dashAssemblyStreams(probe,selection,false)!=reference)throw std::runtime_error("DASH joined output changed its selected streams.");dashAssemblyDuration(probe,result.duration);
 cancel.check();const auto bytes=fs::file_size(result.staging);if(!bytes||bytes>static_cast<uintmax_t>(std::numeric_limits<i64>::max()))throw std::runtime_error("Invalid DASH joined output size.");result.bytes=static_cast<i64>(bytes);checkpointDashAssembly(folder,sessionBinding,result,cancel,true);return result;
}

DashManifest parseDashManifest(const std::string& document,const Cancel& cancel){
 cancel.check();if(document.empty()||document.size()>2*1024*1024)throw std::runtime_error("DASH manifest exceeds its XML size limit.");
 std::unique_ptr<IStream,DashComRelease> stream(SHCreateMemStream((const BYTE*)document.data(),(UINT)document.size()));if(!stream)throw std::runtime_error("Cannot allocate the DASH XML input.");
 IXmlReader* raw=nullptr;dashXmlCheck(CreateXmlReader(__uuidof(IXmlReader),(void**)&raw,nullptr));std::unique_ptr<IXmlReader,DashComRelease> reader(raw);
 dashXmlCheck(reader->SetProperty(XmlReaderProperty_DtdProcessing,DtdProcessing_Prohibit));
 dashXmlCheck(reader->SetProperty(XmlReaderProperty_XmlResolver,0));
 dashXmlCheck(reader->SetProperty(XmlReaderProperty_ConformanceLevel,XmlConformanceLevel_Document));
 dashXmlCheck(reader->SetProperty(XmlReaderProperty_MaxElementDepth,32));
 dashXmlCheck(reader->SetInput(stream.get()));
 DashManifest result;std::vector<size_t> stack;size_t decoded=0;XmlNodeType type=XmlNodeType_None;HRESULT status=S_OK;
 while((status=reader->Read(&type))==S_OK){
  cancel.check();
  if(type==XmlNodeType_Element){
   if(result.nodes.size()>=12000||stack.size()>=32||(!result.nodes.empty()&&stack.empty()))throw std::runtime_error("Invalid or oversized DASH XML tree.");
   DashXmlNode node;node.name=dashXmlString(reader.get(),&IXmlReader::GetLocalName);node.namespaceUri=dashXmlString(reader.get(),&IXmlReader::GetNamespaceUri);decoded+=node.name.size()+node.namespaceUri.size();
   if(node.name=="ContentProtection")throw std::runtime_error("Encrypted or DRM-protected streams are not supported.");
   const bool empty=reader->IsEmptyElement()!=FALSE;HRESULT attribute=reader->MoveToFirstAttribute();
   while(attribute==S_OK){
    auto name=dashXmlString(reader.get(),&IXmlReader::GetLocalName),ns=dashXmlString(reader.get(),&IXmlReader::GetNamespaceUri),value=dashXmlString(reader.get(),&IXmlReader::GetValue);
    auto key=ns.empty()?name:"{"+ns+"}"+name;decoded+=key.size()+value.size();
    if(node.attributes.size()>=64||!node.attributes.emplace(key,value).second)throw std::runtime_error("Duplicate or excessive DASH attributes.");
    attribute=reader->MoveToNextAttribute();
   }
   dashXmlCheck(attribute);dashXmlCheck(reader->MoveToElement());const size_t index=result.nodes.size();result.nodes.push_back(std::move(node));
   if(!stack.empty())result.nodes[stack.back()].children.push_back(index);if(!empty)stack.push_back(index);
  }else if(type==XmlNodeType_EndElement){if(stack.empty())throw std::runtime_error("Invalid DASH XML nesting.");stack.pop_back();}
  else if(type==XmlNodeType_Text||type==XmlNodeType_CDATA||type==XmlNodeType_Whitespace){auto text=dashXmlString(reader.get(),&IXmlReader::GetValue);decoded+=text.size();if(!stack.empty())result.nodes[stack.back()].text+=text;else if(!trim(text).empty())throw std::runtime_error("Unexpected text outside the DASH root.");}
  else if(type==XmlNodeType_DocumentType)throw std::runtime_error("DASH document types are not supported.");
  if(decoded>4*1024*1024)throw std::runtime_error("Decoded DASH XML exceeds its memory limit.");
 }
 dashXmlCheck(status);if(status!=S_FALSE||!stack.empty()||result.nodes.empty())throw std::runtime_error("Incomplete DASH XML document.");
 const auto& root=result.nodes[0];if(root.name!="MPD"||(!root.namespaceUri.empty()&&root.namespaceUri!="urn:mpeg:dash:schema:mpd:2011"))throw std::runtime_error("Expected a DASH MPD document.");
 auto found=root.attributes.find("type");auto mode=found==root.attributes.end()?"static":found->second;
 if(mode!="static"&&mode!="OnDemand"&&mode!="dynamic"&&mode!="Live")throw std::runtime_error("Unsupported DASH presentation type.");result.dynamic=mode=="dynamic"||mode=="Live";
 result.duration=dashXmlDuration(root,"mediaPresentationDuration");result.minimumUpdatePeriod=dashXmlDuration(root,"minimumUpdatePeriod");result.timeShiftBufferDepth=dashXmlDuration(root,"timeShiftBufferDepth");result.suggestedDelay=dashXmlDuration(root,"suggestedPresentationDelay");
 result.availabilityStartUs=dashXmlTime(root,"availabilityStartTime");result.availabilityEndUs=dashXmlTime(root,"availabilityEndTime");result.publishTimeUs=dashXmlTime(root,"publishTime");
 if(result.dynamic&&!result.availabilityStartUs)throw std::runtime_error("Live DASH requires an availability start time.");
 if(result.availabilityStartUs&&result.availabilityEndUs&&*result.availabilityEndUs<*result.availabilityStartUs)throw std::runtime_error("Invalid DASH availability interval.");
 return result;
}
}
