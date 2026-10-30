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
#include <numeric>

namespace udm {
static Json dashIndexSource(const DashTemplateAddressing&);
std::vector<DashIndexBox> parseDashIndexes(const Bytes& bytes,i64 indexStart,i64 resourceSize,const Cancel& cancel,std::optional<i64> externalMediaSize){
 constexpr i64 exact=9007199254740991LL,indexLimit=512*1024,mediaLimit=256LL*1024*1024;
 cancel.check();
 if(bytes.empty()||bytes.size()>indexLimit||indexStart<0||resourceSize<=0||resourceSize>exact||indexStart>resourceSize||static_cast<i64>(bytes.size())>resourceSize-indexStart||(externalMediaSize&&(*externalMediaSize<=0||*externalMediaSize>exact)))throw std::runtime_error("Invalid DASH index resource bounds.");
 auto add=[&](i64 left,i64 right){if(left<0||right<0||left>exact-right)throw std::runtime_error("DASH index exceeds exact integer bounds.");return left+right;};
 auto u32=[&](size_t position){if(position>bytes.size()||bytes.size()-position<4)throw std::runtime_error("Truncated DASH index integer.");return (uint32_t(bytes[position])<<24)|(uint32_t(bytes[position+1])<<16)|(uint32_t(bytes[position+2])<<8)|uint32_t(bytes[position+3]);};
 auto u64=[&](size_t position){const uint64_t value=(uint64_t(u32(position))<<32)|u32(position+4);if(value>static_cast<uint64_t>(exact))throw std::runtime_error("DASH index integer exceeds supported precision.");return static_cast<i64>(value);};
 std::vector<DashIndexBox> result;size_t at=0,totalReferences=0;
 while(at<bytes.size()){
  cancel.check();if(bytes.size()-at<8)throw std::runtime_error("Truncated DASH index box header.");
  i64 length=u32(at);size_t header=8;
  if(length==1){if(bytes.size()-at<16)throw std::runtime_error("Truncated large DASH index box header.");length=u64(at+8);header=16;}
  if(length<static_cast<i64>(header)||length>static_cast<i64>(bytes.size()-at))throw std::runtime_error("Invalid DASH index box length.");
  const size_t end=at+static_cast<size_t>(length);
  if(u32(at+4)==0x73696478u){
   if(result.size()>=64)throw std::runtime_error("Too many DASH index roots.");
   size_t cursor=at+header;auto require=[&](size_t count){if(cursor>end||count>end-cursor)throw std::runtime_error("Truncated DASH segment index.");};
   require(12);const auto full=u32(cursor),version=full>>24;if(version>1||(full&0xffffffu))throw std::runtime_error("Unsupported DASH index version or flags.");cursor+=4;
   DashIndexBox box;box.start=add(indexStart,static_cast<i64>(at));box.length=length;box.referenceId=u32(cursor);box.timescale=u32(cursor+4);cursor+=8;
   if(!box.timescale)throw std::runtime_error("DASH index timescale must be positive.");
   require(version?20:12);box.earliest=version?u64(cursor):u32(cursor);cursor+=version?8:4;box.firstOffset=version?u64(cursor):u32(cursor);cursor+=version?8:4;
   const auto counts=u32(cursor);cursor+=4;const size_t count=counts&0xffffu;
   if((counts>>16)||!count||count>=10000||totalReferences>10000-count||count*12!=end-cursor)throw std::runtime_error("Invalid or oversized DASH reference table.");totalReferences+=count;
   i64 indexOffset=add(box.start,box.length),mediaOffset=externalMediaSize?box.firstOffset:add(indexOffset,box.firstOffset),time=box.earliest;
   if(externalMediaSize&&box.firstOffset>=*externalMediaSize)throw std::runtime_error("External DASH index starts beyond its media resource.");
   for(size_t n=0;n<count;++n,cursor+=12){
    cancel.check();const auto reference=u32(cursor);DashIndexReference part;part.index=(reference&0x80000000u)!=0;part.length=reference&0x7fffffffu;part.duration=u32(cursor+4);part.sap=u32(cursor+8);part.time=time;
    part.start=externalMediaSize&&part.index?indexOffset:mediaOffset;
    if(part.length<=0||part.length>(part.index?indexLimit:mediaLimit)||part.duration<=0)throw std::runtime_error("Invalid DASH index reference.");
    const i64 next=add(part.start,part.length),limit=externalMediaSize&&!part.index?*externalMediaSize:resourceSize;
    if(next>limit)throw std::runtime_error("DASH index reference exceeds its resource.");
    if(!externalMediaSize&&!part.index&&part.start<indexStart+static_cast<i64>(bytes.size())&&next>indexStart)throw std::runtime_error("DASH media reference overlaps the supplied index range.");
    if(externalMediaSize&&part.index)indexOffset=next;else mediaOffset=next;
    time=add(time,part.duration);box.references.push_back(part);
   }
   box.duration=time-box.earliest;result.push_back(std::move(box));
  }
  at=end;
 }
 if(result.empty())throw std::runtime_error("DASH resource contains no segment index.");
 // A child locator may contain several complete roots, but cannot cut through
 // a parsed box or point back into the parent. Tree resolution owns cycles.
 for(const auto& box:result)for(const auto& part:box.references){if(!part.index)continue;
  for(const auto& target:result){cancel.check();if(part.start<target.start+target.length&&target.start<part.start+part.length&&(part.start>target.start||part.start+part.length<target.start+target.length||target.start==box.start))throw std::runtime_error("DASH child index overlaps an index box.");}
 }
 return result;
}
std::vector<DashIndexedMedia> resolveDashIndexes(i64 start,i64 length,i64 resourceSize,const std::function<Bytes(i64,i64,const Cancel&)>& reader,const Cancel& cancel,std::optional<i64> externalMediaSize){
 constexpr i64 exact=9007199254740991LL;
 if(!reader||resourceSize<=0||resourceSize>exact||(externalMediaSize&&(*externalMediaSize<=0||*externalMediaSize>exact)))throw std::runtime_error("Invalid DASH index tree resource.");
 struct Span {i64 start,length;};struct Chunk {Span span;Bytes bytes;};
 struct Clock {uint32_t id,scale;i64 time,duration;};
 std::vector<Chunk> chunks;std::vector<DashIndexedMedia> media;
 std::set<std::pair<i64,i64>> encountered,visited;
 size_t requests=0,bytesRead=0,references=0;
 auto overlaps=[](Span a,Span b){return a.start<b.start+b.length&&b.start<a.start+a.length;};
 auto equalTime=[](i64 a,uint32_t scaleA,i64 b,uint32_t scaleB){
  // Remainders fit uint32, so their cross products fit uint64. Avoid both
  // floating-point tolerance and overflowing a full timestamp cross product.
  return a/scaleA==b/scaleB&&uint64_t(a%scaleA)*scaleB==uint64_t(b%scaleB)*scaleA;
 };
 auto read=[&](Span span){
  cancel.check();if(span.start<0||span.length<8||span.length>512*1024||span.start>resourceSize||span.length>resourceSize-span.start)throw std::runtime_error("Invalid DASH index tree range.");
  for(const auto& chunk:chunks){if(span.start>=chunk.span.start&&span.start+span.length<=chunk.span.start+chunk.span.length){const auto offset=static_cast<size_t>(span.start-chunk.span.start);return Bytes(chunk.bytes.begin()+offset,chunk.bytes.begin()+offset+static_cast<size_t>(span.length));}if(overlaps(span,chunk.span))throw std::runtime_error("DASH index reads partially overlap.");}
  if(!externalMediaSize)for(const auto& part:media)if(overlaps(span,{part.reference.start,part.reference.length}))throw std::runtime_error("DASH index overlaps an earlier media range.");
  if(++requests>64||span.length>2*1024*1024-static_cast<i64>(bytesRead))throw std::runtime_error("DASH index tree exceeds its fetch budget.");bytesRead+=static_cast<size_t>(span.length);
  auto bytes=reader(span.start,span.length,cancel);cancel.check();if(bytes.size()!=static_cast<size_t>(span.length))throw std::runtime_error("DASH index reader returned an incomplete range.");
  chunks.push_back({span,bytes});return bytes;
 };
 std::function<i64(Span,size_t,std::optional<Clock>,std::optional<i64>)> visit;
 visit=[&](Span span,size_t depth,std::optional<Clock> parent,std::optional<i64> expectedStart)->i64{
  cancel.check();if(depth>8)throw std::runtime_error("DASH index nesting exceeds eight levels.");
  auto data=read(span);auto boxes=parseDashIndexes(data,span.start,resourceSize,cancel,externalMediaSize);
  for(const auto& box:boxes)encountered.emplace(box.start,box.length);
  const auto visitedOnEntry=visited;
  std::optional<Clock> previous;std::optional<i64> cursor=expectedStart;
  for(const auto& box:boxes){
   cancel.check();const auto key=std::make_pair(box.start,box.length);
   // A fetched range may also contain descendants just visited through a root.
   // A box already visited before entering this range is a repeat or cycle.
   if(visited.count(key)){if(!visitedOnEntry.count(key))continue;throw std::runtime_error("DASH index tree repeats or cycles through a box.");}
   for(const auto& old:visited)if(overlaps({box.start,box.length},{old.first,old.second}))throw std::runtime_error("DASH index boxes overlap.");
   visited.insert(key);
   if(previous&&(box.referenceId!=previous->id||!equalTime(box.earliest,box.timescale,previous->time+previous->duration,previous->scale)))throw std::runtime_error("DASH index roots have discontinuous identity or timing.");
   if(!previous&&parent&&(box.referenceId!=parent->id||!equalTime(box.earliest,box.timescale,parent->time,parent->scale)))throw std::runtime_error("DASH child index does not match its parent.");
   if(box.references.size()>20000-references)throw std::runtime_error("DASH index tree has too many references.");references+=box.references.size();
   if(externalMediaSize){if(cursor&&*cursor!=box.firstOffset)throw std::runtime_error("External DASH index media offsets are discontinuous.");cursor=box.firstOffset;}
   for(const auto& source:box.references){
    cancel.check();if(source.index){const auto next=visit({source.start,source.length},depth+1,Clock{box.referenceId,box.timescale,source.time,source.duration},externalMediaSize?cursor:std::optional<i64>{});if(externalMediaSize)cursor=next;continue;}
    auto part=source;
    if(externalMediaSize){part.start=*cursor;if(part.start<0||part.start>=*externalMediaSize||part.length>*externalMediaSize-part.start)throw std::runtime_error("External DASH media range exceeds its resource.");cursor=part.start+part.length;}
    else for(const auto& chunk:chunks)if(overlaps({part.start,part.length},chunk.span))throw std::runtime_error("DASH media overlaps fetched index bytes.");
    if(!media.empty()){const auto& last=media.back();if(part.start<last.reference.start+last.reference.length||(externalMediaSize&&part.start!=last.reference.start+last.reference.length)||box.referenceId!=last.referenceId||!equalTime(part.time,box.timescale,last.reference.time+last.reference.duration,last.timescale))throw std::runtime_error("DASH indexed media is out of order or discontinuous.");}
    if(media.size()>=10000)throw std::runtime_error("DASH index tree has too many media segments.");media.push_back({part,box.referenceId,box.timescale});
   }
   previous=Clock{box.referenceId,box.timescale,box.earliest,box.duration};
  }
  if(!previous)throw std::runtime_error("DASH index range contains no unvisited roots.");
  if(parent&&!equalTime(previous->time+previous->duration,previous->scale,parent->time+parent->duration,parent->scale))throw std::runtime_error("DASH child duration does not match its parent.");
  return cursor.value_or(0);
 };
 visit({start,length},0,{},{});
 if(media.empty()||encountered!=visited)throw std::runtime_error("DASH index tree contains unaccounted indexes or no media.");
 return media;
}
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
 std::map<std::string,std::string> attributes;std::optional<size_t> timeline,initializationNode,indexNode;std::vector<size_t> listNodes;bool templateSeen=false,listSeen=false,baseSeen=false;
 for(auto index:lineage){
  cancel.check();std::vector<size_t> bases,templates,lists,segmentBases;
  for(auto child:nodes[index].children){const auto& node=nodes[child];if(node.namespaceUri!=nodes[0].namespaceUri)continue;
   if(node.name=="BaseURL")bases.push_back(child);else if(node.name=="SegmentTemplate")templates.push_back(child);
   else if(node.name=="SegmentList")lists.push_back(child);else if(node.name=="SegmentBase")segmentBases.push_back(child);
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
  if(templates.size()>1||lists.size()>1||segmentBases.size()>1||(int(!templates.empty())+int(!lists.empty())+int(!segmentBases.empty())>1))throw std::runtime_error("Ambiguous DASH segment addressing.");
  templateSeen=templateSeen||!templates.empty();listSeen=listSeen||!lists.empty();baseSeen=baseSeen||!segmentBases.empty();if(int(templateSeen)+int(listSeen)+int(baseSeen)>1)throw std::runtime_error("Mixed inherited DASH addressing types are unsupported.");
  if(!segmentBases.empty()){
   result.indexed=true;const auto& node=nodes[segmentBases.front()];if(node.attributes.count("{http://www.w3.org/1999/xlink}href"))throw std::runtime_error("External DASH addressing elements are unsupported.");for(const auto& attribute:node.attributes)attributes[attribute.first]=attribute.second;
   std::optional<size_t> localInitialization,localIndex;
   for(auto child:node.children){const auto& element=nodes[child];if(element.namespaceUri!=nodes[0].namespaceUri)continue;
    if(element.name=="Initialization"){if(localInitialization)throw std::runtime_error("Ambiguous DASH base initialization.");localInitialization=child;}
    else if(element.name=="RepresentationIndex"){if(localIndex)throw std::runtime_error("Ambiguous DASH representation index.");localIndex=child;}
    else throw std::runtime_error("Unsupported DASH SegmentBase child.");
   }
   if(localInitialization)initializationNode=localInitialization;if(localIndex)indexNode=localIndex;
  }
  if(!lists.empty()){
   result.listed=true;const auto& node=nodes[lists.front()];if(node.attributes.count("{http://www.w3.org/1999/xlink}href"))throw std::runtime_error("External DASH segment lists are unsupported.");for(const auto& attribute:node.attributes)attributes[attribute.first]=attribute.second;
   std::optional<size_t> localTimeline,localInitialization;std::vector<size_t> localResources;
   for(auto child:node.children){const auto& element=nodes[child];if(element.namespaceUri!=nodes[0].namespaceUri)continue;
    if(element.name=="SegmentTimeline"){if(localTimeline)throw std::runtime_error("Ambiguous DASH list timeline.");localTimeline=child;}
    else if(element.name=="Initialization"){if(localInitialization)throw std::runtime_error("Ambiguous DASH list initialization.");localInitialization=child;}
    else if(element.name=="SegmentURL"){if(localResources.size()>=10000)throw std::runtime_error("DASH segment list exceeds its resource limit.");localResources.push_back(child);}
    else throw std::runtime_error("Unsupported DASH segment-list child.");
   }
   if(localTimeline)timeline=localTimeline;if(localInitialization)initializationNode=localInitialization;if(!localResources.empty())listNodes=std::move(localResources);
  }
  if(!templates.empty()){
   const auto& node=nodes[templates.front()];if(node.attributes.count("{http://www.w3.org/1999/xlink}href"))throw std::runtime_error("External DASH templates are unsupported.");for(const auto& attribute:node.attributes)attributes[attribute.first]=attribute.second;
   std::optional<size_t> localTimeline,localInitialization,localIndex;
   for(auto child:node.children){const auto& element=nodes[child];if(element.namespaceUri!=nodes[0].namespaceUri)continue;
    if(element.name=="SegmentTimeline"){if(localTimeline)throw std::runtime_error("Ambiguous DASH segment timeline.");localTimeline=child;}
    else if(element.name=="Initialization"){if(localInitialization)throw std::runtime_error("Ambiguous DASH template initialization.");localInitialization=child;}
    else if(element.name=="RepresentationIndex"){if(localIndex)throw std::runtime_error("Ambiguous DASH template representation index.");localIndex=child;}
   }
   if(localTimeline)timeline=localTimeline;if(localInitialization)initializationNode=localInitialization;if(localIndex)indexNode=localIndex;
  }
 }
 const bool templateIndex=templateSeen&&(attributes.count("index")||indexNode);
 if(templateIndex){if(timeline)throw std::runtime_error("Conflicting DASH index template and explicit timeline.");result.indexed=true;}
 auto integer=[](const std::string& text){if(text.empty()||text.size()>19)throw std::runtime_error("Invalid DASH addressing integer.");i64 value=0;for(char c:text){if(c<'0'||c>'9'||value>(std::numeric_limits<i64>::max()-(c-'0'))/10)throw std::runtime_error("Invalid DASH addressing integer.");value=value*10+c-'0';}return value;};
 auto number=[&](const char* key,i64 fallback){auto found=attributes.find(key);return found==attributes.end()?fallback:integer(found->second);};
 if(attributes.count("pdDelta"))throw std::runtime_error("Unsupported DASH duration boundary adjustment.");
 if(attributes.count("eptDelta")){
  if(result.listed||result.indexed||timeline)throw std::runtime_error("DASH earliest-presentation offsets require duration-based templates.");
  const auto& value=attributes.at("eptDelta");const bool negative=!value.empty()&&value.front()=='-';const i64 magnitude=integer(negative?value.substr(1):value);
  if(magnitude>9007199254740991LL||(!negative&&magnitude))throw std::runtime_error("Unsupported DASH earliest-presentation offset or initial gap.");
  result.earliestPresentationDelta=-magnitude;
 }
 result.availabilityOffset=dashAvailabilityOffset(attributes);result.availabilityComplete=dashAvailabilityComplete(attributes);
 auto media=attributes.find("media");if(!result.listed&&!result.indexed){if(media==attributes.end()||media->second.empty())throw std::runtime_error("DASH media template is missing.");result.media=media->second;}
 auto initialization=attributes.find("initialization");if(initialization!=attributes.end())result.initialization=initialization->second;
 result.timescale=number("timescale",1);result.presentationTimeOffset=number("presentationTimeOffset",0);result.startNumber=number("startNumber",1);
 if(attributes.count("endNumber")){result.endNumber=number("endNumber",0);if(*result.endNumber<result.startNumber)throw std::runtime_error("DASH endNumber precedes startNumber.");}
 if(result.timescale<=0)throw std::runtime_error("DASH timescale must be positive.");
 if(result.earliestPresentationDelta < -result.presentationTimeOffset)throw std::runtime_error("DASH first segment has a negative media timestamp.");
 const auto& rep=nodes[representation];auto id=rep.attributes.find("id");if(id!=rep.attributes.end())result.values.representationId=id->second;
 auto bandwidth=rep.attributes.find("bandwidth");if(bandwidth!=rep.attributes.end())result.values.bandwidth=integer(bandwidth->second);
 if(result.indexed){
  for(const char* key:{"media","duration","startNumber","endNumber"})if(attributes.count(key))throw std::runtime_error("Invalid DASH indexed addressing attribute.");
  const bool templateInit=templateIndex&&attributes.count("initialization"),templateLocator=templateIndex&&attributes.count("index");
  if((!templateIndex&&attributes.count("initialization"))||(!initializationNode&&!templateInit)||(initializationNode&&templateInit)||(!indexNode&&!templateLocator&&!attributes.count("indexRange"))||(indexNode&&templateLocator)||(!templateIndex&&indexNode&&attributes.count("indexRange")))throw std::runtime_error("DASH indexed media requires unambiguous initialization and index locators.");
  if(attributes.count("indexRangeExact")){const auto& value=attributes.at("indexRangeExact");if(value!="true"&&value!="false"&&value!="1"&&value!="0")throw std::runtime_error("Invalid DASH exact index-range flag.");}
  auto range=[&](const std::string& text,i64 limit){const auto split=text.find('-');if(split==std::string::npos)throw std::runtime_error("Invalid DASH index/initialization range.");const i64 start=integer(text.substr(0,split)),end=integer(text.substr(split+1));if(end<start||end-start>=limit||end>9007199254740991LL)throw std::runtime_error("DASH indexed range exceeds supported bounds.");return std::pair<i64,i64>{start,end-start+1};};
  auto source=[&](size_t index){const auto& node=nodes[index];if(!node.children.empty()||node.attributes.count("{http://www.w3.org/1999/xlink}href"))throw std::runtime_error("Invalid DASH indexed resource locator.");auto found=node.attributes.find("sourceURL");return found==node.attributes.end()?std::string{}:found->second;};
  auto expandIndexTemplate=[&](const char* key){const auto& pattern=attributes.at(key);if(pattern.empty())throw std::runtime_error("Empty DASH indexed resource template.");return expandDashTemplate(pattern,result.values);};
  result.hasInitialization=true;
  if(templateInit)result.initialization=expandIndexTemplate("initialization");
  else{result.initialization=source(*initializationNode);const auto& init=nodes[*initializationNode].attributes;if(init.count("range"))result.initializationRange=range(init.at("range"),256LL*1024*1024);}
  if(templateLocator)result.index=expandIndexTemplate("index");
  else if(indexNode){result.index=source(*indexNode);const auto& locator=nodes[*indexNode].attributes;if(locator.count("range"))result.indexRange=range(locator.at("range"),512*1024);}
  if(attributes.count("indexRange")){if(result.indexRange)throw std::runtime_error("Conflicting DASH index ranges.");result.indexRange=range(attributes.at("indexRange"),512*1024);}
  std::optional<bool> external;
  for(const auto& base:result.bases){cancel.check();if(base==address(manifestUrl))throw std::runtime_error("DASH indexed media requires a media file BaseURL.");const auto indexUrl=address(combineUrl(base,result.index)),initUrl=address(combineUrl(base,result.initialization));const bool separate=indexUrl!=base;
   if(external&&*external!=separate)throw std::runtime_error("DASH index alternatives mix embedded and external resources.");external=separate;
   if((!separate&&!result.indexRange)||(initUrl==base&&!result.initializationRange))throw std::runtime_error("DASH in-file index and initialization require ranges.");
  }
  return result;
 }
 if(!result.listed&&initializationNode&&!attributes.count("initialization")){
  const auto& node=nodes[*initializationNode];if(!node.children.empty()||node.attributes.count("{http://www.w3.org/1999/xlink}href"))throw std::runtime_error("Invalid DASH template initialization locator.");
  result.hasInitialization=true;auto source=node.attributes.find("sourceURL");result.initialization=source==node.attributes.end()?std::string{}:source->second;
  auto range=node.attributes.find("range");if(range!=node.attributes.end()){
   const auto split=range->second.find('-');if(split==std::string::npos)throw std::runtime_error("Invalid DASH template initialization range.");
   const i64 start=integer(range->second.substr(0,split)),end=integer(range->second.substr(split+1));if(end<start||end-start>=256LL*1024*1024||end>9007199254740991LL)throw std::runtime_error("DASH template initialization range exceeds supported bounds.");result.initializationRange=std::pair<i64,i64>{start,end-start+1};
  }
  for(const auto& base:result.bases)if(address(combineUrl(base,result.initialization))==address(manifestUrl))throw std::runtime_error("DASH initialization locator resolves to the manifest.");
 }
 if(result.listed&&manifest.dynamic&&(!timeline||!nodes[*timeline].children.size()))throw std::runtime_error("Live DASH segment lists require an explicit timeline.");
 if(timeline)result.timeline=readDashTimeline(manifest,*timeline,cancel);
 else{const i64 duration=number("duration",0);if(duration<=0)throw std::runtime_error("DASH template requires a timeline or duration.");result.timeline.push_back({result.listed?i64{0}:result.presentationTimeOffset+result.earliestPresentationDelta,duration,-1});}
 if(result.listed){
  if(attributes.count("media")||attributes.count("initialization"))throw std::runtime_error("Template attributes cannot address a DASH segment list.");
  if(listNodes.empty()||result.startNumber>std::numeric_limits<i64>::max()-static_cast<i64>(listNodes.size()-1))throw std::runtime_error("Invalid DASH segment-list numbering.");
  const i64 lastNumber=result.startNumber+static_cast<i64>(listNodes.size()-1);if(result.endNumber&&*result.endNumber!=lastNumber)throw std::runtime_error("DASH list endNumber disagrees with its resource count.");result.endNumber=lastNumber;
  auto byteRange=[&](const DashXmlNode& node,const char* key)->std::optional<std::pair<i64,i64>>{const auto found=node.attributes.find(key);if(found==node.attributes.end())return {};const auto split=found->second.find('-');if(split==std::string::npos)throw std::runtime_error("Invalid DASH list byte range.");const i64 start=integer(found->second.substr(0,split)),end=integer(found->second.substr(split+1));if(end<start||end-start>=256LL*1024*1024||end>=9007199254740991LL)throw std::runtime_error("DASH list byte range exceeds supported bounds.");return std::pair<i64,i64>{start,end-start+1};};
  if(initializationNode){const auto& node=nodes[*initializationNode];if(!node.children.empty())throw std::runtime_error("Invalid DASH list initialization.");result.hasInitialization=true;const auto source=node.attributes.find("sourceURL");result.initialization=source==node.attributes.end()?std::string{}:source->second;result.initializationRange=byteRange(node,"range");}
  for(auto index:listNodes){
   cancel.check();const auto& node=nodes[index];if(!node.children.empty())throw std::runtime_error("Invalid DASH segment-list resource.");
   const auto mediaRange=byteRange(node,"mediaRange"),indexRange=byteRange(node,"indexRange");
   const auto locator=node.attributes.find("index");
   if(locator!=node.attributes.end()){
    if(locator->second.empty()||locator->second.size()>32768||std::any_of(locator->second.begin(),locator->second.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("Invalid DASH segment index locator.");
    for(const auto& base:result.bases){Url parsed(combineUrl(base,locator->second));if(parsed.scheme!="http"&&parsed.scheme!="https")throw std::runtime_error("DASH segment indexes require HTTP or HTTPS.");}
   }else if(indexRange&&mediaRange&&(indexRange->first<mediaRange->first||indexRange->first+indexRange->second>mediaRange->first+mediaRange->second))throw std::runtime_error("DASH embedded index lies outside its media range.");
   // Full-segment recording consumes the declared media range, not subsegments
   // from the optional index. Index locators are validated but never fetched.
   const auto source=node.attributes.find("media");result.resources.push_back({source==node.attributes.end()?std::string{}:source->second,mediaRange});
  }
  if(manifest.dynamic&&!result.timeline.front().time)throw std::runtime_error("Live DASH lists require an explicit first timestamp for sliding-window identity.");
  i64 cursor=0;size_t position=0;
  for(size_t i=0;i<result.timeline.size();++i){cancel.check();const auto& entry=result.timeline[i];const i64 time=entry.time.value_or(cursor);if(time<cursor)throw std::runtime_error("DASH list timeline overlaps.");if(entry.repeat>=static_cast<i64>(listNodes.size()))throw std::runtime_error("DASH list repeat exceeds its resource count.");i64 count=entry.repeat+1;
   if(entry.repeat==-1){if(i+1<result.timeline.size()){if(!result.timeline[i+1].time||*result.timeline[i+1].time<=time||(*result.timeline[i+1].time-time)%entry.duration)throw std::runtime_error("DASH list repeat has an ambiguous boundary.");count=(*result.timeline[i+1].time-time)/entry.duration;}else count=static_cast<i64>(listNodes.size()-position);}
   if(count<=0||count>static_cast<i64>(listNodes.size()-position)||entry.duration>(std::numeric_limits<i64>::max()-time)/count)throw std::runtime_error("DASH list timeline and resource count disagree.");
   for(i64 n=0;n<count;++n){result.listedReferences.push_back({result.startNumber+static_cast<i64>(position),time+n*entry.duration,entry.duration,result.resources[position].byteRange});++position;}cursor=time+count*entry.duration;
  }
  if(position!=listNodes.size())throw std::runtime_error("DASH list has resources without timeline entries.");
 }
 return result;
}

static std::string dashSegmentLocation(const DashTemplateAddressing& addressing,const DashSegmentReference& reference){
 if(addressing.listed){if(reference.number<addressing.startNumber||static_cast<uint64_t>(reference.number-addressing.startNumber)>=addressing.resources.size())throw std::runtime_error("DASH list resource position is invalid.");return addressing.resources[static_cast<size_t>(reference.number-addressing.startNumber)].media;}
 // Nominal coverage includes eptDelta; template $Time$ deliberately does not.
 if(addressing.earliestPresentationDelta>0||addressing.earliestPresentationDelta==std::numeric_limits<i64>::min()||reference.time<0||reference.time>std::numeric_limits<i64>::max()+addressing.earliestPresentationDelta)throw std::runtime_error("DASH template URL clock overflows.");
 auto values=addressing.values;values.number=reference.number;values.time=reference.time-addressing.earliestPresentationDelta;return expandDashTemplate(addressing.media,values);
}

DashResourceWindow planDashTemplateWindow(const DashTemplateAddressing& addressing,i64 windowStart,i64 windowEnd,const Cancel& cancel){
 if(addressing.indexed&&!addressing.listed)throw std::runtime_error("DASH indexed addressing must be materialized before planning.");
 cancel.check();if(addressing.bases.empty()||addressing.bases.size()>32||addressing.timescale<=0||addressing.presentationTimeOffset<0)throw std::runtime_error("Invalid DASH template addressing.");
 std::vector<DashSegmentReference> references;
 if(addressing.listed){
  if(windowStart<0||windowEnd<=windowStart||addressing.resources.empty()||addressing.resources.size()>10000||addressing.listedReferences.size()!=addressing.resources.size())throw std::runtime_error("Invalid DASH listed-resource window.");
  for(const auto& ref:addressing.listedReferences){cancel.check();if(ref.time<0||ref.duration<=0||ref.time>std::numeric_limits<i64>::max()-ref.duration)throw std::runtime_error("Invalid DASH listed-resource clock.");if(ref.time<windowEnd&&ref.time+ref.duration>windowStart)references.push_back(ref);}
 }else references=expandDashTimeline(addressing.timeline,addressing.startNumber,windowStart,windowEnd,cancel,addressing.endNumber);
 DashResourceWindow result;result.timescale=addressing.timescale;result.presentationTimeOffset=addressing.presentationTimeOffset;result.period=addressing.period;
 size_t bytes=0;
 auto resources=[&](const std::string& relative){
  if(relative.size()>32768||std::any_of(relative.begin(),relative.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("Invalid DASH resource URI reference.");
  std::vector<std::string> urls;
  for(const auto& base:addressing.bases){cancel.check();Url parsed(combineUrl(base,relative));if(parsed.scheme!="http"&&parsed.scheme!="https")throw std::runtime_error("DASH resources require HTTP or HTTPS.");
   if(std::find(urls.begin(),urls.end(),parsed.full)==urls.end()){if(parsed.full.size()>4*1024*1024-bytes)throw std::runtime_error("DASH resource window exceeds its URL memory limit.");bytes+=parsed.full.size();urls.push_back(std::move(parsed.full));}
  }return urls;
 };
 if(!addressing.initialization.empty()||addressing.hasInitialization){result.initialization=resources((addressing.listed||addressing.hasInitialization)?addressing.initialization:expandDashTemplate(addressing.initialization,addressing.values));result.initializationRange=addressing.initializationRange;}
 for(const auto& reference:references){cancel.check();result.segments.push_back({reference,resources(dashSegmentLocation(addressing,reference))});}
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
  const auto relative=dashSegmentLocation(addressing,ref);std::set<std::string> ready;
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
 if(previous.addressing.indexed){
  if(!current.addressing.indexed||!previous.addressing.listed||previous.addressing.indexSource!=dashIndexSource(current.addressing))throw std::runtime_error("DASH indexed source changed within the selected Period.");
  const auto refreshedPeriod=current.addressing.period;current.addressing=previous.addressing;current.addressing.period=refreshedPeriod;
 }else if(current.addressing.indexed)throw std::runtime_error("DASH addressing changed to an index within the selected Period.");
 if(current.format!=previous.format||current.addressing.listed!=previous.addressing.listed||current.addressing.timescale!=previous.addressing.timescale||current.addressing.presentationTimeOffset!=previous.addressing.presentationTimeOffset||current.addressing.earliestPresentationDelta!=previous.addressing.earliestPresentationDelta||std::abs(current.addressing.period.start-previous.addressing.period.start)>0.000001)throw std::runtime_error("Selected DASH track format or timeline changed during refresh.");
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

static Json dashBindResourceVersion(Json identity,const Json& version);
Json dashSegmentBinding(const DashSelectedTrack& track,const DashSegmentReference& segment){
 if(segment.number<0||segment.time<0||segment.duration<=0||segment.time>std::numeric_limits<i64>::max()-segment.duration)throw std::runtime_error("Invalid DASH segment identity.");
 if(track.presentation.empty())throw std::runtime_error("DASH segment has no presentation binding.");
 Json result={{"schema",1},{"presentation",track.presentation},{"period",track.periodId},{"adaptation",track.adaptationId},{"representation",track.representationId},{"format",track.format},{"periodStart",track.addressing.period.start},{"timescale",track.addressing.timescale},{"presentationTimeOffset",track.addressing.presentationTimeOffset},{"number",segment.number},{"time",segment.time},{"duration",segment.duration}};
 if(track.addressing.earliestPresentationDelta)result["eptDelta"]=track.addressing.earliestPresentationDelta;
 if(track.addressing.indexed&&track.addressing.listed){if(!track.addressing.indexReferenceId||track.addressing.indexVersion.is_null())throw std::runtime_error("DASH indexed track has no resolved index identity.");result["index"]={{"referenceId",*track.addressing.indexReferenceId},{"version",track.addressing.indexVersion},{"source",adaptiveSelectionDigest(track.addressing.indexSource)}};}
 if(segment.byteRange)result=dashRangeBinding(result,segment.byteRange->first,segment.byteRange->second);
 return dashBindResourceVersion(std::move(result),segment.resourceVersion);
}

namespace {
std::optional<std::pair<i64,i64>> dashResourceRange(const Json& binding){
 if(!binding.contains("byteRange"))return {};
 const auto& range=binding["byteRange"];if(!range.is_object()||range.size()!=2||!range.contains("start")||!range.contains("length"))throw std::runtime_error("Invalid DASH byte-range binding.");
 auto integer=[](const Json& value){if(!value.is_number_integer()||(value.is_number_unsigned()&&value.get<uint64_t>()>static_cast<uint64_t>(std::numeric_limits<i64>::max())))throw std::runtime_error("Invalid DASH byte-range integer.");return value.get<i64>();};
 const i64 start=integer(range["start"]),length=integer(range["length"]);
 if(start<0||length<=0||length>256LL*1024*1024||start>std::numeric_limits<i64>::max()-(length-1))throw std::runtime_error("DASH byte range exceeds supported bounds.");
 return std::pair<i64,i64>{start,length};
}
struct DashResourceVersion {std::string url,etag;i64 total;};
std::optional<DashResourceVersion> dashResourceVersion(const Json& binding){
 if(!binding.contains("resourceVersion"))return {};
 const auto& value=binding["resourceVersion"];
 if(!value.is_object()||value.size()!=3||!value.contains("url")||!value["url"].is_string()||!value.contains("etag")||!value["etag"].is_string()||!value.contains("total")||!value["total"].is_number_integer()||(value["total"].is_number_unsigned()&&value["total"].get<uint64_t>()>9007199254740991ULL))throw std::runtime_error("Invalid DASH resource version binding.");
 DashResourceVersion result{str(value,"url"),str(value,"etag"),num(value,"total")};
 if(result.url.empty()||result.url.size()>32768||result.url.find('#')!=std::string::npos||std::any_of(result.url.begin(),result.url.end(),[](unsigned char c){return c<=0x20||c==0x7f;}))throw std::runtime_error("Invalid DASH version resource URL.");
 const Url address(result.url);if((address.scheme!="http"&&address.scheme!="https")||address.full!=result.url)throw std::runtime_error("DASH version requires a canonical HTTP resource URL.");
 if(result.total<=0||result.total>9007199254740991LL||result.etag.size()<2||result.etag.size()>1024||result.etag.front()!='"'||result.etag.back()!='"'||std::any_of(result.etag.begin()+1,result.etag.end()-1,[](unsigned char c){return c<0x21||c==0x22||c==0x7f;}))throw std::runtime_error("DASH resource version requires a size and strong entity tag.");
 const auto range=dashResourceRange(binding);if(range?(range->first>=result.total||range->second>result.total-range->first):result.total>256LL*1024*1024)throw std::runtime_error("DASH version size does not contain the requested resource.");
 return result;
}
}
Json dashRangeBinding(const Json& identity,i64 start,i64 length){
 if(!identity.is_object()||identity.empty()||identity.contains("byteRange"))throw std::runtime_error("Invalid DASH ranged resource identity.");
 auto result=identity;result["byteRange"]={{"start",start},{"length",length}};dashResourceRange(result);if(result.dump().size()>65536)throw std::runtime_error("DASH range binding exceeds its storage limit.");return result;
}
Json dashVersionBinding(const Json& identity,const std::string& resourceUrl,i64 total,const std::string& etag){
 if(!identity.is_object()||identity.empty()||identity.contains("resourceVersion"))throw std::runtime_error("Invalid DASH versioned resource identity.");
 auto result=identity;result["resourceVersion"]={{"url",resourceUrl},{"total",total},{"etag",etag}};dashResourceVersion(result);if(result.dump().size()>65536)throw std::runtime_error("DASH version binding exceeds its storage limit.");return result;
}
static Json dashBindResourceVersion(Json identity,const Json& version){
 if(version.is_null())return identity;
 identity["resourceVersion"]=version;dashResourceVersion(identity);if(identity.dump().size()>65536)throw std::runtime_error("DASH version binding exceeds its storage limit.");return identity;
}
static Json dashInitializationBinding(const Json& identity,const Json& urls,const std::optional<std::pair<i64,i64>>& range,const Json& version=Json()){
 Json result={{"schema",1},{"kind","initialization"},{"track",identity},{"urls",urls}};
 if(range)result=dashRangeBinding(result,range->first,range->second);
 return dashBindResourceVersion(std::move(result),version);
}

Json discoverDashResourceVersion(const std::vector<std::string>& urls,const DashResourceHeaders& headers,const Json& prefs,const std::shared_ptr<Cancel>& cancel){
 if(!cancel||urls.empty()||urls.size()>32||headers.size()>64)throw std::runtime_error("Invalid DASH resource discovery request.");cancel->check();
 size_t headerBytes=0;for(const auto& scope:headers){const Url origin(scope.first);if(origin.origin!=scope.first||(origin.scheme!="http"&&origin.scheme!="https"))throw std::runtime_error("DASH credentials require canonical HTTP origins.");validateHeaders(scope.second);for(const auto& entry:scope.second){headerBytes+=entry.first.size()+entry.second.size();if(headerBytes>256*1024)throw std::runtime_error("DASH resource headers exceed their size limit.");}}
 auto budget=std::make_shared<Cancel>();budget->parent=cancel;budget->deadline=GetTickCount64()+15000;
 auto integer=[](const std::string& text){i64 value=0;if(text.empty())throw std::runtime_error("Missing DASH resource length.");for(char c:text){if(c<'0'||c>'9'||value>(9007199254740991LL-(c-'0'))/10)throw std::runtime_error("Invalid DASH resource length.");value=value*10+c-'0';}if(!value)throw std::runtime_error("Empty DASH resource.");return value;};
 Json settings=prefs;settings.erase("ActiveBrowserSession");settings["SiteLogins"]=Json::array();settings["WindowsLoginIntranet"]=false;
 const Url original(urls.front());std::exception_ptr failure;size_t requests=0;
 for(const auto& source:urls){
  try{
   // HEAD is cheap when supported; a one-byte range request supplies headers
   // for origins which omit HEAD metadata. Neither path consumes the media body.
   for(bool head:{true,false}){
    std::string address=source;
    for(int hop=0;hop<6;++hop){
     budget->check();if(++requests>12)throw std::runtime_error("DASH resource discovery request limit reached.");const Url current(address);
     if((current.scheme!="http"&&current.scheme!="https")||(original.scheme=="https"&&current.scheme!="https"))throw std::runtime_error("Invalid DASH resource discovery destination.");
     const auto scope=headers.find(current.origin);Headers requestHeaders=scope==headers.end()?Headers{}:scope->second;
     for(auto it=requestHeaders.begin();it!=requestHeaders.end();){const auto name=lower(it->first);if(name=="range"||name=="if-range"||name=="if-match"||name=="if-unmodified-since"||name=="if-none-match"||name=="if-modified-since"||name=="accept-encoding"||name=="cache-control")it=requestHeaders.erase(it);else ++it;}
     requestHeaders["Accept-Encoding"]="identity";requestHeaders["Cache-Control"]="no-cache";if(!head)requestHeaders["Range"]="bytes=0-0";
     Http response(address,requestHeaders,settings,*budget,{},{},"",nullptr,false,{},head);
     if(response.status>=300&&response.status<400){const auto location=response.header(L"Location");if(location.empty())throw std::runtime_error("DASH discovery redirect has no destination.");auto next=combineUrl(address,location);if(current.scheme=="https"&&Url(next).scheme!="https")throw std::runtime_error("DASH discovery HTTPS downgrade rejected.");address=std::move(next);continue;}
     if(head&&(response.status==405||response.status==501))break;
     if(response.status!=200u&&(head||response.status!=206u))throw HttpRejected(response.status,response.header(L"Retry-After"));
     try{
      const auto encoding=lower(trim(response.header(L"Content-Encoding"))),type=lower(response.header(L"Content-Type"));
      if((!encoding.empty()&&encoding!="identity")||type.find("text/html")!=std::string::npos||type.find("application/xhtml")!=std::string::npos||type.find("multipart/byteranges")!=std::string::npos)throw std::runtime_error("DASH discovery did not return a plain media resource.");
      i64 total=0;if(response.status==206u){std::smatch match;const auto range=trim(response.header(L"Content-Range"));if(!std::regex_match(range,match,std::regex("bytes 0-0/([0-9]{1,16})")))throw std::runtime_error("DASH discovery range was not honored.");total=integer(match[1]);const auto length=trim(response.header(L"Content-Length"));if(!length.empty()&&integer(length)!=1)throw std::runtime_error("Invalid DASH discovery range length.");}
      else total=integer(trim(response.header(L"Content-Length")));
      // Validate the total independently of the per-part storage bound.
      Json descriptor={{"byteRange",{{"start",0},{"length",1}}},{"resourceVersion",{{"url",address},{"total",total},{"etag",response.header(L"ETag")}}}};
      dashResourceVersion(descriptor);budget->check();return descriptor["resourceVersion"];
     }catch(const Cancelled&){throw;}catch(...){if(head)break;throw;}
    }
   }
   throw std::runtime_error("DASH resource discovery exhausted redirects or metadata methods.");
  }catch(const Cancelled&){cancel->check();throw std::runtime_error("DASH resource discovery timed out.");}catch(...){failure=std::current_exception();cancel->check();if(requests>=12)break;}
 }
 if(failure)std::rethrow_exception(failure);throw std::runtime_error("No DASH resource version was discovered.");
}

std::optional<DashStoredResource> inspectDashResource(const fs::path& directory,const Json& binding,const Cancel& cancel){
 cancel.check();if(!binding.is_object()||binding.empty()||binding.dump().size()>65536)throw std::runtime_error("Invalid DASH resource binding.");
 const auto range=dashResourceRange(binding);const auto version=dashResourceVersion(binding);
 auto plain=[](const fs::path& path){auto flags=GetFileAttributesW(path.c_str());if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("DASH recovery files must not be redirected.");};
 const auto folder=fs::absolute(directory).lexically_normal();for(auto parent=folder;!parent.empty();){plain(parent);auto next=parent.parent_path();if(next==parent)break;parent=std::move(next);}
 const auto key=adaptiveSelectionDigest(binding);const auto name=wide(key);const auto destination=folder/(name+L".part"),receipt=folder/(name+L".receipt.json");plain(destination);plain(receipt);
 if(!fs::is_regular_file(receipt)||!fs::is_regular_file(destination))return {};
 try{auto envelope=Json::parse(readText(receipt,256*1024));if(num(envelope,"schema")!=1)return {};auto saved=Json::parse(reveal(str(envelope,"protected")));const i64 length=num(saved,"bytes");const auto digest=str(saved,"sha256");
  if(str(saved,"binding")!=key||length<=0||length>256LL*1024*1024||(range&&length!=range->second)||!std::regex_match(digest,std::regex("[0-9a-fA-F]{64}"))||fs::file_size(destination)!=static_cast<uintmax_t>(length)||lower(fileHash(destination,[&]{cancel.check();}))!=lower(digest))return {};
  if(version&&!range&&length!=version->total)return {};
  return DashStoredResource{destination,length,digest,true};
 }catch(const Cancelled&){throw;}catch(const std::exception&){cancel.check();return {};}
}

DashStoredResource storeDashResource(const fs::path& directory,const Json& binding,const std::vector<std::string>& urls,const DashResourceHeaders& headers,const Json& prefs,const std::shared_ptr<Cancel>& cancel,const std::function<void(size_t)>& onBytes){
 if(!cancel)throw std::runtime_error("DASH resource storage requires cancellation.");cancel->check();
 if(headers.size()>64)throw std::runtime_error("Too many DASH credential origins.");
 size_t headerBytes=0;for(const auto& scope:headers){const Url origin(scope.first);if((origin.scheme!="http"&&origin.scheme!="https")||origin.origin!=scope.first)throw std::runtime_error("DASH credentials require canonical HTTP origins.");validateHeaders(scope.second);for(const auto& entry:scope.second){headerBytes+=entry.first.size()+entry.second.size();if(headerBytes>256*1024)throw std::runtime_error("DASH resource headers exceed their size limit.");}}
 if(!binding.is_object()||binding.empty()||binding.dump().size()>65536||urls.empty()||urls.size()>32)throw std::runtime_error("Invalid DASH resource storage request.");
 const auto range=dashResourceRange(binding);const auto version=dashResourceVersion(binding);
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
    if(range){for(auto it=requestHeaders.begin();it!=requestHeaders.end();)if(lower(it->first)=="accept-encoding")it=requestHeaders.erase(it);else ++it;requestHeaders["Accept-Encoding"]="identity";requestHeaders["Range"]="bytes="+std::to_string(range->first)+"-"+std::to_string(range->first+range->second-1);}
    if(version){
     for(auto it=requestHeaders.begin();it!=requestHeaders.end();){const auto name=lower(it->first);if(name=="if-match"||name=="if-unmodified-since"||name=="cache-control"||name=="accept-encoding")it=requestHeaders.erase(it);else ++it;}
     requestHeaders["Accept-Encoding"]="identity";requestHeaders["Cache-Control"]="no-cache";
     if(current.full==version->url)requestHeaders["If-Match"]=version->etag;
    }
    // Every alternative and redirect resolves its own credential scope. Disable
    // implicit browser/site credentials so anonymous origins stay anonymous.
    Json settings=prefs;settings.erase("ActiveBrowserSession");settings["SiteLogins"]=Json::array();settings["WindowsLoginIntranet"]=false;
    Http response(address,requestHeaders,settings,*cancel,{},{},"",nullptr,false);
    if(response.status>=300&&response.status<400){auto location=response.header(L"Location");if(location.empty())throw std::runtime_error("DASH resource redirect has no destination.");auto next=combineUrl(address,location);if(current.scheme=="https"&&Url(next).scheme!="https")throw std::runtime_error("DASH resource HTTPS downgrade rejected.");address=std::move(next);continue;}
    if(response.status!=(range?206u:200u))throw HttpRejected(response.status,response.header(L"Retry-After"));
    if(version&&(current.full!=version->url||response.header(L"ETag")!=version->etag))throw std::runtime_error("DASH resource version changed; existing media was retained.");
    auto encoding=lower(trim(response.header(L"Content-Encoding")));if(!encoding.empty()&&encoding!="identity")throw std::runtime_error("Unexpected encoded DASH resource.");
    const auto type=lower(response.header(L"Content-Type"));if(type.find("text/html")!=std::string::npos||type.find("application/xhtml")!=std::string::npos)throw std::runtime_error("DASH resource returned a web page.");
    i64 expected=-1;const auto length=trim(response.header(L"Content-Length"));if(!length.empty()){expected=0;for(char c:length){if(c<'0'||c>'9'||expected>(limit-(c-'0'))/10)throw std::runtime_error("Invalid DASH resource length.");expected=expected*10+c-'0';}}
    if(range){
     std::smatch match;const auto contentRange=trim(response.header(L"Content-Range"));const i64 end=range->first+range->second-1;
     if(type.find("multipart/byteranges")!=std::string::npos||!std::regex_match(contentRange,match,std::regex("bytes ([0-9]{1,19})-([0-9]{1,19})/([0-9]{1,19})"))||std::stoll(match[1])!=range->first||std::stoll(match[2])!=end||std::stoll(match[3])<=end||(expected>=0&&expected!=range->second))throw std::runtime_error("DASH byte range was not honored exactly.");
     expected=range->second;
     if(version&&std::stoll(match[3])!=version->total)throw std::runtime_error("DASH shared resource size changed.");
    }
    if(version&&!range){if(expected>=0&&expected!=version->total)throw std::runtime_error("DASH resource version length changed.");expected=version->total;}
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

std::vector<DashIndexedMedia> loadDashIndexes(const fs::path& directory,const Json& identity,const Json& version,const std::vector<std::string>& urls,i64 start,i64 length,const DashResourceHeaders& headers,const Json& prefs,const std::shared_ptr<Cancel>& cancel,std::optional<i64> externalMediaSize,const DashMetadataAccounting& onBytes){
 if(!cancel||!identity.is_object()||identity.empty()||version.is_null())throw std::runtime_error("DASH index loading requires a version-bound identity.");cancel->check();
 const Json base={{"schema",1},{"kind","dash-index"},{"identity",identity}};
 auto initial=dashBindResourceVersion(dashRangeBinding(base,start,length),version);const auto resource=dashResourceVersion(initial);
 auto read=[&](i64 offset,i64 count,const Cancel& checkpoint){
  checkpoint.check();auto budget=std::make_shared<Cancel>();budget->parent=cancel;budget->deadline=GetTickCount64()+15000;
  const auto binding=dashBindResourceVersion(dashRangeBinding(base,offset,count),version);
  DashStoredResource stored;try{stored=storeDashResource(directory,binding,urls,headers,prefs,budget,[&](size_t bytes){accountDashMetadata(bytes,onBytes,cancel,budget);});}catch(const Cancelled&){cancel->check();throw std::runtime_error("DASH index fetch timed out.");}
  cancel->check();std::string text(static_cast<size_t>(count),'\0');{
   std::ifstream file(stored.path,std::ios::binary);if(!file)throw std::runtime_error("Cannot read saved DASH index bytes.");file.read(text.data(),static_cast<std::streamsize>(count));
   if(!file||file.gcount()!=count||file.peek()!=std::char_traits<char>::eof()||file.bad())throw std::runtime_error("Saved DASH index range changed length.");
  }
  // Verify the bytes actually handed to the decoder, including cache reuse.
  BCRYPT_ALG_HANDLE algorithm=nullptr;if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Cannot initialize DASH index verification.");
  unsigned char digest[32]{};const auto status=BCryptHash(algorithm,nullptr,0,(PUCHAR)text.data(),static_cast<ULONG>(text.size()),digest,sizeof(digest));BCryptCloseAlgorithmProvider(algorithm,0);
  if(status<0)throw std::runtime_error("Cannot hash DASH index bytes.");std::string hash;const char* hex="0123456789abcdef";for(auto byte:digest){hash+=hex[byte>>4];hash+=hex[byte&15];}
  if(hash!=lower(stored.sha256))throw std::runtime_error("Saved DASH index bytes changed after verification.");cancel->check();return Bytes(text.begin(),text.end());
 };
 return resolveDashIndexes(start,length,resource->total,read,*cancel,externalMediaSize);
}
static Json dashIndexSource(const DashTemplateAddressing& source){
 if(!source.indexed||source.listed)throw std::runtime_error("Expected unresolved DASH index source.");
 auto range=[](const std::optional<std::pair<i64,i64>>& value){return value?Json{{"start",value->first},{"length",value->second}}:Json();};
 auto offset=[](double value){if(std::isinf(value)&&value>0)return Json("INF");if(!std::isfinite(value))throw std::runtime_error("Invalid DASH availability offset.");return Json(value);};
 Json timings=Json::array();for(const auto& timing:source.baseTiming)timings.push_back({{"offset",offset(timing.offset)},{"complete",timing.complete}});
 return {{"bases",source.bases},{"baseTiming",timings},{"offset",offset(source.availabilityOffset)},{"complete",source.availabilityComplete},{"periodStart",source.period.start},{"timescale",source.timescale},{"presentationTimeOffset",source.presentationTimeOffset},{"index",source.index},{"indexRange",range(source.indexRange)},{"initialization",source.initialization},{"initializationRange",range(source.initializationRange)},{"hasInitialization",source.hasInitialization}};
}
static fs::path dashIndexCheckpointPath(const fs::path& directory,const Json& identity){
 if(!identity.is_object()||identity.empty()||identity.dump().size()>65536)throw std::runtime_error("Invalid DASH index checkpoint identity.");
 const auto folder=fs::absolute(directory).lexically_normal();auto plain=[](const fs::path& path){const auto flags=GetFileAttributesW(path.c_str());if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("DASH index checkpoint must not be redirected.");};
 for(auto parent=folder;!parent.empty();){plain(parent);auto next=parent.parent_path();if(next==parent)break;parent=std::move(next);}const auto path=folder/(L"index-state-"+wide(adaptiveSelectionDigest(identity))+L".json");plain(path);auto temporary=path;temporary+=L".tmp";plain(temporary);return path;
}
std::string dashIndexSourceDigest(const DashTemplateAddressing& source){return adaptiveSelectionDigest(dashIndexSource(source));}
std::optional<DashTemplateAddressing> restoreDashIndex(const fs::path& directory,const Json& identity,const DashTemplateAddressing& source,const Cancel& cancel){
 cancel.check();const auto path=dashIndexCheckpointPath(directory,identity);if(!fs::exists(path))return {};
 const auto envelope=Json::parse(readText(path,16*1024*1024));if(num(envelope,"schema")!=1)throw std::runtime_error("Unsupported DASH index checkpoint.");const auto text=reveal(str(envelope,"protected"));if(text.size()>8*1024*1024)throw std::runtime_error("DASH index checkpoint exceeds its limit.");const auto saved=Json::parse(text);
 if(!saved.is_object()||saved.value("identity",Json())!=identity||saved.value("source",Json())!=dashIndexSource(source)||!saved.contains("segments")||!saved["segments"].is_array()||saved["segments"].empty()||saved["segments"].size()>10000)throw std::runtime_error("DASH index checkpoint source changed.");
 constexpr i64 exact=9007199254740991LL;auto integer=[&](const Json& object,const char* key){const auto& value=object.at(key);if(!value.is_number_integer()||(value.is_number_unsigned()&&value.get<uint64_t>()>static_cast<uint64_t>(exact)))throw std::runtime_error("Invalid DASH index checkpoint integer.");const auto n=value.get<i64>();if(n<0||n>exact)throw std::runtime_error("Invalid DASH index checkpoint integer.");return n;};
 auto result=source;result.timescale=integer(saved,"timescale");result.presentationTimeOffset=integer(saved,"presentationTimeOffset");const auto id=integer(saved,"referenceId");
 if(source.timescale<=0||source.presentationTimeOffset<0||!result.timescale||result.timescale%source.timescale||source.presentationTimeOffset>exact/(result.timescale/source.timescale)||result.presentationTimeOffset!=source.presentationTimeOffset*(result.timescale/source.timescale)||id>UINT32_MAX)throw std::runtime_error("DASH index checkpoint clock changed.");
 result.indexVersion=saved.at("indexVersion");const auto mediaVersion=saved.at("mediaVersion");
 dashResourceVersion(Json{{"byteRange",{{"start",0},{"length",1}}},{"resourceVersion",result.indexVersion}});
 result.indexReferenceId=static_cast<uint32_t>(id);result.indexSource=saved["source"];result.listed=true;result.startNumber=1;result.timeline.clear();result.resources.clear();result.listedReferences.clear();
 i64 number=1,lastEnd=-1,lastByteEnd=-1;for(const auto& segment:saved["segments"]){cancel.check();const i64 time=integer(segment,"time"),duration=integer(segment,"duration"),start=integer(segment,"start"),length=integer(segment,"length");
  if(integer(segment,"number")!=number||duration<=0||time>exact-duration||(lastEnd>=0&&time!=lastEnd))throw std::runtime_error("DASH index checkpoint timeline is discontinuous.");
  dashResourceVersion(Json{{"byteRange",{{"start",start},{"length",length}}},{"resourceVersion",mediaVersion}});
  if(lastByteEnd>=0&&start<lastByteEnd)throw std::runtime_error("DASH index checkpoint byte ranges overlap.");lastByteEnd=start+length;
  result.resources.push_back({"",std::pair<i64,i64>{start,length}});result.listedReferences.push_back({number++,time,duration,std::pair<i64,i64>{start,length},mediaVersion});lastEnd=time+duration;
 }
 result.endNumber=number-1;return result;
}
void checkpointDashIndex(const fs::path& directory,const Json& identity,const DashTemplateAddressing& state,const Cancel& cancel){
 cancel.check();if(!state.indexed||!state.listed||!state.indexReferenceId||state.indexSource.is_null()||state.indexVersion.is_null()||state.listedReferences.empty())throw std::runtime_error("Cannot checkpoint an unresolved DASH index.");
 Json segments=Json::array();const auto version=state.listedReferences.front().resourceVersion;for(const auto& ref:state.listedReferences){cancel.check();if(!ref.byteRange||ref.resourceVersion!=version)throw std::runtime_error("DASH index checkpoint mixes media versions.");segments.push_back({{"number",ref.number},{"time",ref.time},{"duration",ref.duration},{"start",ref.byteRange->first},{"length",ref.byteRange->second}});}
 const Json saved={{"identity",identity},{"source",state.indexSource},{"timescale",state.timescale},{"presentationTimeOffset",state.presentationTimeOffset},{"referenceId",*state.indexReferenceId},{"indexVersion",state.indexVersion},{"mediaVersion",version},{"segments",segments}};
 const auto text=saved.dump();if(text.size()>8*1024*1024)throw std::runtime_error("DASH index checkpoint exceeds its size limit.");const auto path=dashIndexCheckpointPath(directory,identity);fs::create_directories(path.parent_path());dashIndexCheckpointPath(directory,identity);atomicText(path,Json{{"schema",1},{"protected",protect(text)}}.dump(),false);
}
DashTemplateAddressing materializeDashIndex(const fs::path& directory,const Json& identity,const DashTemplateAddressing& addressing,const DashResourceHeaders& headers,const Json& prefs,const std::shared_ptr<Cancel>& cancel,const DashMetadataAccounting& onBytes){
 constexpr i64 exact=9007199254740991LL;
 if(!cancel||!addressing.indexed||addressing.listed||addressing.bases.empty()||addressing.bases.size()>32||addressing.bases.size()!=addressing.baseTiming.size()||addressing.timescale<=0||addressing.timescale>exact||addressing.presentationTimeOffset<0||addressing.presentationTimeOffset>exact||!addressing.hasInitialization)throw std::runtime_error("Invalid DASH indexed addressing state.");cancel->check();
 const auto mediaVersion=discoverDashResourceVersion(addressing.bases,headers,prefs,cancel);const i64 mediaSize=num(mediaVersion,"total");
 std::vector<std::string> indexUrls;std::optional<bool> external;
 for(const auto& base:addressing.bases){cancel->check();const auto address=combineUrl(base,addressing.index);const bool separate=address!=base;if(external&&*external!=separate)throw std::runtime_error("DASH index alternatives mix address spaces.");external=separate;if(std::find(indexUrls.begin(),indexUrls.end(),address)==indexUrls.end())indexUrls.push_back(address);}
 const auto indexVersion=*external?discoverDashResourceVersion(indexUrls,headers,prefs,cancel):mediaVersion;
 const auto range=addressing.indexRange.value_or(std::pair<i64,i64>{0,num(indexVersion,"total")});
 if(range.first<0||range.second<8||range.second>512*1024)throw std::runtime_error("DASH index locator exceeds the bounded reader.");
 const auto parts=loadDashIndexes(directory,identity,indexVersion,indexUrls,range.first,range.second,headers,prefs,cancel,*external?std::optional<i64>{mediaSize}:std::optional<i64>{},onBytes);
 i64 timescale=addressing.timescale;
 for(const auto& part:parts){cancel->check();const i64 scale=part.timescale;if(scale<=0)throw std::runtime_error("Invalid DASH indexed media clock.");const i64 factor=scale/std::gcd(timescale,scale);if(timescale>exact/factor)throw std::runtime_error("DASH indexed clocks have no supported exact common timescale.");timescale*=factor;}
 auto rescale=[&](i64 value,i64 source){const i64 factor=timescale/source;if(value<0||value>exact/factor)throw std::runtime_error("DASH indexed timestamp exceeds supported precision.");return value*factor;};
 auto result=addressing;result.listed=true;result.startNumber=1;result.timescale=timescale;result.presentationTimeOffset=rescale(addressing.presentationTimeOffset,addressing.timescale);result.resources.clear();result.listedReferences.clear();result.timeline.clear();
 for(const auto& part:parts){cancel->check();const auto& ref=part.reference;const i64 time=rescale(ref.time,part.timescale),duration=rescale(ref.duration,part.timescale);if(time>exact-duration)throw std::runtime_error("DASH indexed media end exceeds supported precision.");
  result.resources.push_back({"",std::pair<i64,i64>{ref.start,ref.length}});
  result.listedReferences.push_back({static_cast<i64>(result.listedReferences.size())+1,time,duration,std::pair<i64,i64>{ref.start,ref.length},mediaVersion});
 }
 if(result.listedReferences.empty())throw std::runtime_error("DASH index contains no media references.");result.indexReferenceId=parts.front().referenceId;result.indexVersion=indexVersion;result.indexSource=dashIndexSource(addressing);result.endNumber=static_cast<i64>(result.listedReferences.size());return result;
}
static void validateDashRecordingVersions(const Json& index,const Cancel& cancel){
 std::map<std::string,Json> versions;
 auto accept=[&](const Json& descriptor){cancel.check();const auto version=dashResourceVersion(descriptor);if(!version)return;const auto& value=descriptor["resourceVersion"];auto inserted=versions.emplace(version->url,value);if(!inserted.second&&inserted.first->second!=value)throw std::runtime_error("DASH recording combines different versions of a shared resource.");};
 for(const auto& track:index.at("tracks")){
  for(const auto& record:track.at("segments"))accept(record);
  if(track.contains("initialization"))accept(track["initialization"]);
  if(track.contains("pendingInitializationVersion")){
   Json descriptor={{"resourceVersion",track["pendingInitializationVersion"]}};
   if(track.contains("pendingInitializationByteRange"))descriptor["byteRange"]=track["pendingInitializationByteRange"];
   accept(descriptor);
  }
 }
}
static void bindDashListVersions(std::vector<DashResourceWindow>& windows,const std::vector<DashSelectedTrack>& tracks,const Json& index,const DashResourceHeaders& headers,const Json& prefs,const std::shared_ptr<Cancel>& cancel){
 std::map<std::string,Json> known;
 auto remember=[&](const Json& urls,const Json& version){
  if(version.is_null())return;
  if(!urls.is_array()||urls.empty()||urls.size()>32)throw std::runtime_error("Invalid DASH version discovery URLs.");
  for(const auto& url:urls)if(!url.is_string())throw std::runtime_error("Invalid DASH version discovery URL.");
  const auto key=adaptiveSelectionDigest(urls);auto added=known.emplace(key,version);if(!added.second&&added.first->second!=version)throw std::runtime_error("DASH source locations have conflicting saved versions.");
 };
 for(const auto& track:index.at("tracks")){
  if(!track.is_object()||!track.contains("segments")||!track["segments"].is_object())throw std::runtime_error("Invalid DASH recording track.");
  for(const auto& record:track["segments"]){if(!record.is_object())throw std::runtime_error("Invalid DASH recording segment.");if(record.contains("resourceVersion"))remember(record.at("urls"),record["resourceVersion"]);}
  if(track.contains("initialization")&&track["initialization"].contains("resourceVersion"))remember(track["initialization"].at("urls"),track["initialization"]["resourceVersion"]);
  if(track.contains("pendingInitializationVersion"))remember(track.at("pendingInitialization"),track["pendingInitializationVersion"]);
 }
 auto bind=[&](const std::vector<std::string>& urls,Json& version,const std::optional<std::pair<i64,i64>>& range){
  cancel->check();const Json sources=urls;const auto key=adaptiveSelectionDigest(sources);auto found=known.find(key);
  if(found!=known.end()){if(!version.is_null()&&version!=found->second)throw std::runtime_error("DASH planned resource version changed.");version=found->second;}
  else{const auto discovered=discoverDashResourceVersion(urls,headers,prefs,cancel);if(!version.is_null()&&version!=discovered)throw std::runtime_error("DASH supplied version does not match the resource.");version=discovered;remember(sources,version);}
  Json descriptor={{"resourceVersion",version}};if(range)descriptor["byteRange"]={{"start",range->first},{"length",range->second}};dashResourceVersion(descriptor);
 };
 for(size_t t=0;t<tracks.size();++t){if(windows[t].segments.empty())continue;auto& window=windows[t];const auto& addressing=tracks[t].addressing;
  if(!window.initialization.empty()&&(addressing.listed||window.initializationRange))bind(window.initialization,window.initializationVersion,window.initializationRange);
  if(addressing.listed)for(auto& segment:window.segments)bind(segment.urls,segment.reference.resourceVersion,segment.reference.byteRange);
 }
}
Json captureDashWindow(const fs::path& directory,const DashLiveUpdate& update,const DashResourceHeaders& headers,const Json& prefs,const std::shared_ptr<Cancel>& cancel,const std::function<void(size_t,size_t,const Cancel&)>& onBytes,std::optional<DashCaptureInterval> interval,size_t concurrency){
 if(!cancel)throw std::runtime_error("DASH capture requires cancellation.");cancel->check();
 if(update.state.tracks.empty()||update.state.tracks.size()>3||update.windows.size()!=update.state.tracks.size())throw std::runtime_error("Invalid DASH capture tracks.");
 for(const auto& track:update.state.tracks)if(track.addressing.indexed&&!track.addressing.listed)throw std::runtime_error("Cannot capture an unresolved DASH index.");
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
 auto windows=update.windows;
 bindDashListVersions(windows,update.state.tracks,index,headers,prefs,cancel);
 size_t total=0;
 for(size_t t=0;t<identities.size();++t){
  auto& track=index["tracks"][t];if(!track.is_object()||!track.contains("identity")||track["identity"]!=identities[t]||!track.contains("segments")||!track["segments"].is_object())throw std::runtime_error("Invalid DASH recording track.");
  std::map<i64,DashSegmentReference> ordered;
  for(auto it=track["segments"].begin();it!=track["segments"].end();++it){cancel->check();const auto& record=it.value();if(!record.is_object()||!record.contains("number")||!record["number"].is_number_integer()||!record.contains("time")||!record["time"].is_number_integer()||!record.contains("duration")||!record["duration"].is_number_integer())throw std::runtime_error("Invalid DASH recording segment.");
   DashSegmentReference ref{num(record,"number"),num(record,"time"),num(record,"duration"),dashResourceRange(record),record.value("resourceVersion",Json())};if(it.key()!=std::to_string(ref.number)||ref.number<0||ref.time<0||ref.duration<=0||str(record,"binding")!=adaptiveSelectionDigest(dashSegmentBinding(update.state.tracks[t],ref)))throw std::runtime_error("DASH saved segment identity changed.");ordered.emplace(ref.number,ref);
  }
  for(const auto& segment:windows[t].segments){cancel->check();const auto& ref=segment.reference;const auto binding=adaptiveSelectionDigest(dashSegmentBinding(update.state.tracks[t],ref));const auto key=std::to_string(ref.number);auto found=ordered.find(ref.number);
   if(found!=ordered.end()){if(found->second.time!=ref.time||found->second.duration!=ref.duration||found->second.byteRange!=ref.byteRange||found->second.resourceVersion!=ref.resourceVersion)throw std::runtime_error("DASH refresh replaced an indexed segment.");}
   else{ordered.emplace(ref.number,ref);track["segments"][key]={{"number",ref.number},{"time",ref.time},{"duration",ref.duration},{"binding",binding},{"committed",false}};}
   track["segments"][key]["urls"]=segment.urls;
   if(ref.byteRange)track["segments"][key]["byteRange"]={{"start",ref.byteRange->first},{"length",ref.byteRange->second}};
   if(!ref.resourceVersion.is_null())track["segments"][key]["resourceVersion"]=ref.resourceVersion;
  }
  track["periodDuration"]=update.state.tracks[t].addressing.period.duration?Json(*update.state.tracks[t].addressing.period.duration):Json();
  if(!windows[t].segments.empty()){
   const auto& window=windows[t];if((window.initializationRange||!window.initializationVersion.is_null())&&window.initialization.empty())throw std::runtime_error("DASH initialization range has no resource.");
   const auto initialization=dashInitializationBinding(identities[t],window.initialization,window.initializationRange,window.initializationVersion);
   if(track.contains("initialization")&&dashResourceRange(track["initialization"])!=window.initializationRange)throw std::runtime_error("DASH initialization range changed within the Period.");
   if(track.contains("pendingInitialization")){Json pending=Json::object();if(track.contains("pendingInitializationByteRange"))pending["byteRange"]=track["pendingInitializationByteRange"];if(dashResourceRange(pending)!=window.initializationRange)throw std::runtime_error("DASH pending initialization range changed.");}
   if(track.contains("initialization")&&track["initialization"].value("resourceVersion",Json())!=window.initializationVersion)throw std::runtime_error("DASH initialization version changed within the Period.");
   if(track.contains("pendingInitialization")&&track.value("pendingInitializationVersion",Json())!=window.initializationVersion)throw std::runtime_error("DASH pending initialization version changed.");
   track["pendingInitialization"]=window.initialization;
   if(!window.initializationVersion.is_null())track["pendingInitializationVersion"]=window.initializationVersion;else track.erase("pendingInitializationVersion");
   if(window.initializationRange)track["pendingInitializationByteRange"]=initialization["byteRange"];else track.erase("pendingInitializationByteRange");
  }
  std::optional<DashSegmentReference> last;for(const auto& entry:ordered){const auto& ref=entry.second;if(last&&(last->number==std::numeric_limits<i64>::max()||ref.number!=last->number+1||last->time>std::numeric_limits<i64>::max()-last->duration||ref.time!=last->time+last->duration))throw std::runtime_error("DASH recording has a segment gap or overlap.");last=ref;}
  total+=ordered.size();if(total>10000)throw std::runtime_error("DASH recording exceeds its segment limit.");
 }
 auto save=[&]{cancel->check();const auto text=index.dump();if(text.size()>8*1024*1024)throw std::runtime_error("DASH recording index exceeds its storage limit.");plain(path);plain(temporary);atomicText(path,Json{{"schema",1},{"protected",protect(text)}}.dump(),false);};
 // Validate all planned versions before workers can write receipts. Workers only
 // promote these fixed descriptors, so this does not need repeating per segment.
 validateDashRecordingVersions(index,*cancel);
 // Persist identities and pending URLs before any new resource is requested.
 save();
 auto stored=[](const DashStoredResource& resource){return Json{{"file",utf8(resource.path.filename().wstring())},{"bytes",resource.bytes},{"sha256",resource.sha256}};};
 auto group=std::make_shared<Cancel>();group->parent=cancel;std::mutex checkpointMutex;
 std::vector<Json> drafts;for(const auto& track:index["tracks"])drafts.push_back(track);
 auto downloadTrack=[&](size_t t){
  group->check();const auto& window=windows[t];if(window.segments.empty())return;auto& track=drafts[t];
  auto checkpoint=[&]{std::lock_guard<std::mutex> lock(checkpointMutex);group->check();index["tracks"][t]=track;save();};
  auto account=[&](size_t bytes){if(onBytes)onBytes(t,bytes,*group);};
  if(!window.initialization.empty()){
   const Json binding=dashInitializationBinding(identities[t],window.initialization,window.initializationRange,window.initializationVersion);
   auto initialization=storeDashResource(folder,binding,window.initialization,headers,prefs,group,account);
   if(track.contains("initialization")&&lower(str(track["initialization"],"sha256"))!=lower(initialization.sha256))throw std::runtime_error("DASH initialization changed within the selected Period.");
   track["initialization"]=stored(initialization);track["initialization"]["urls"]=window.initialization;track["initialization"]["binding"]=adaptiveSelectionDigest(binding);if(window.initializationRange)track["initialization"]["byteRange"]=binding["byteRange"];if(!window.initializationVersion.is_null())track["initialization"]["resourceVersion"]=window.initializationVersion;track["initializationVerified"]=true;track.erase("pendingInitialization");track.erase("pendingInitializationByteRange");track.erase("pendingInitializationVersion");checkpoint();
  }else if(track.contains("initialization"))throw std::runtime_error("DASH initialization disappeared during capture.");else{track.erase("pendingInitialization");track.erase("pendingInitializationByteRange");track.erase("pendingInitializationVersion");}
  for(const auto& segment:window.segments){
   auto resource=storeDashResource(folder,dashSegmentBinding(update.state.tracks[t],segment.reference),segment.urls,headers,prefs,group,account);
   auto& record=track["segments"][std::to_string(segment.reference.number)];record["resource"]=stored(resource);record["committed"]=true;checkpoint();
  }
  checkpoint();
 };
 std::vector<size_t> active;for(size_t t=0;t<identities.size();++t)if(!windows[t].segments.empty())active.push_back(t);
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
   DashSegmentReference ref{num(record,"number"),num(record,"time"),num(record,"duration"),dashResourceRange(record),record.value("resourceVersion",Json())};if(ref.number<0||ref.time<0||ref.duration<=0||ref.time>std::numeric_limits<i64>::max()-ref.duration||it.key()!=std::to_string(ref.number))throw std::runtime_error("Invalid DASH recovery timeline.");
   Json binding=track["identity"];binding["number"]=ref.number;binding["time"]=ref.time;binding["duration"]=ref.duration;
   if(ref.byteRange)binding=dashRangeBinding(binding,ref.byteRange->first,ref.byteRange->second);
   binding=dashBindResourceVersion(std::move(binding),ref.resourceVersion);
   if(adaptiveSelectionDigest(binding)!=str(record,"binding"))throw std::runtime_error("DASH recovery segment binding changed.");ordered.emplace(ref.number,ref);
   auto resource=inspectDashResource(folder,binding,cancel);record["committed"]=resource.has_value();if(resource)record["resource"]=stored(*resource);else record.erase("resource");
  }
  std::optional<DashSegmentReference> last;for(const auto& entry:ordered){const auto& ref=entry.second;if(last&&(last->number==std::numeric_limits<i64>::max()||ref.number!=last->number+1||ref.time!=last->time+last->duration))throw std::runtime_error("DASH recovery timeline has a gap or overlap.");last=ref;}
  track["initializationVerified"]=false;
  auto verifyInitialization=[&](const Json& binding)->std::optional<DashStoredResource>{const auto& urls=binding.at("urls");if(!urls.is_array()||urls.empty()||urls.size()>32)throw std::runtime_error("Invalid DASH recovery initialization URLs.");for(const auto& url:urls)if(!url.is_string()||url.get_ref<const std::string&>().size()>32768)throw std::runtime_error("Invalid DASH recovery initialization URL.");return inspectDashResource(folder,binding,cancel);};
  auto saveInitialization=[&](const DashStoredResource& resource,const Json& binding){auto value=stored(resource);value["urls"]=binding["urls"];value["binding"]=adaptiveSelectionDigest(binding);if(binding.contains("byteRange"))value["byteRange"]=binding["byteRange"];if(binding.contains("resourceVersion"))value["resourceVersion"]=binding["resourceVersion"];track["initialization"]=std::move(value);track["initializationVerified"]=true;};
  if(track.contains("initialization")&&track["initialization"].is_object()&&track["initialization"].contains("urls")){
   const auto binding=dashInitializationBinding(track["identity"],track["initialization"]["urls"],dashResourceRange(track["initialization"]),track["initialization"].value("resourceVersion",Json()));auto resource=verifyInitialization(binding);if(resource)saveInitialization(*resource,binding);
  }
  if((track.contains("pendingInitializationByteRange")||track.contains("pendingInitializationVersion"))&&!track.contains("pendingInitialization"))throw std::runtime_error("DASH pending initialization range has no resource.");
  if(track.contains("pendingInitialization")&&(!track["pendingInitialization"].is_array()||track["pendingInitialization"].size()>32))throw std::runtime_error("Invalid DASH pending initialization.");
  if(track.contains("pendingInitialization")&&track["pendingInitialization"].empty()){if(track.contains("initialization")||track.contains("pendingInitializationByteRange")||track.contains("pendingInitializationVersion"))throw std::runtime_error("DASH initialization disappeared from pending capture.");track.erase("pendingInitialization");}
  if(track.contains("pendingInitialization")){
   Json descriptor=Json::object();if(track.contains("pendingInitializationByteRange"))descriptor["byteRange"]=track["pendingInitializationByteRange"];const auto range=dashResourceRange(descriptor);
   if(track.contains("initialization")&&dashResourceRange(track["initialization"])!=range)throw std::runtime_error("DASH recovered initialization range changed within the Period.");
   const auto version=track.value("pendingInitializationVersion",Json());if(track.contains("initialization")&&track["initialization"].value("resourceVersion",Json())!=version)throw std::runtime_error("DASH recovered initialization version changed within the Period.");
   const auto binding=dashInitializationBinding(track["identity"],track["pendingInitialization"],range,version);auto resource=verifyInitialization(binding);if(resource){if(track.contains("initialization")&&lower(str(track["initialization"],"sha256"))!=lower(resource->sha256))throw std::runtime_error("DASH recovered initialization changed within the Period.");saveInitialization(*resource,binding);track.erase("pendingInitialization");track.erase("pendingInitializationByteRange");track.erase("pendingInitializationVersion");}
  }
 }
 validateDashRecordingVersions(index,cancel);
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
