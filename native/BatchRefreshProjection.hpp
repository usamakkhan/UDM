#pragma once
#include "BatchRowProjection.hpp"
namespace udm {
// Copy only editable request inputs on the window thread. Metadata, destinations,
// descriptions and selection state stay with the live rows and are not borrowed.
struct BatchRefreshInput {
 struct Row {size_t index;std::string address,name,protectedOverrides;};
 size_t count=0;std::optional<int> changed;Json preferences;std::vector<Row> rows;
};
inline std::shared_ptr<const BatchRefreshInput> captureBatchRefresh(const std::vector<BatchImportRow>& rows,const std::set<int>& added,std::optional<int> changed,const Json& preferences){
 auto result=std::make_shared<BatchRefreshInput>();result->count=rows.size();result->changed=changed;if(changed)result->preferences=preferences;
 if(changed&&(*changed<0||*changed>=(int)rows.size()))throw std::runtime_error("The changed batch row no longer exists.");
 const size_t begin=changed?*changed:0,end=changed?*changed+1:rows.size();result->rows.reserve(end-begin);
 for(size_t i=begin;i<end;++i)if(!added.count((int)i))result->rows.push_back({i,rows[i].address,rows[i].name,rows[i].protectedRequestOverrides});
 return result;
}
struct BatchRefreshProjection {
 struct Value {size_t index;BatchImportRow row;};
 size_t sourceCount=0;std::optional<int> changed;Headers sourceHeaders;
 std::vector<Value> values;std::vector<BatchPreviewRequest> requests;
};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline std::function<void(std::vector<BatchImportRow>&)> batchRefreshApplyTestObserver;
#endif
inline bool batchRefreshMatches(const BatchRefreshProjection& projection,const std::vector<BatchImportRow>& rows,const Headers& headers,const std::set<int>& added){
 if(projection.sourceCount!=rows.size()||projection.sourceHeaders!=headers)return false;
 if(projection.changed&&(*projection.changed<0||*projection.changed>=(int)rows.size()||projection.values.size()!=1))return false;
 if(!projection.changed){size_t excluded=0;for(int index:added)if(index>=0&&(size_t)index<rows.size())++excluded;if(projection.values.size()!=rows.size()-excluded)return false;}
 std::set<size_t> seen;
 for(const auto& value:projection.values){
  if(value.index>=rows.size()||added.count((int)value.index)||!seen.insert(value.index).second||(projection.changed&&value.index!=(size_t)*projection.changed))return false;
  const auto& current=rows[value.index];const auto& prepared=value.row;
  if(current.address!=prepared.address||current.name!=prepared.name||current.protectedRequestOverrides!=prepared.protectedRequestOverrides)return false;
 }
 return true;
}
inline bool applyBatchRefresh(BatchRefreshProjection& projection,std::vector<BatchImportRow>& rows,const Headers& headers,const std::set<int>& added){
 // Validate the entire cohort before consuming any result or changing any row.
 if(!batchRefreshMatches(projection,rows,headers,added))return false;
 for(auto& value:projection.values){auto& row=rows[value.index];auto& prepared=value.row;
  row.metadata=std::move(prepared.metadata);row.browserName=std::move(prepared.browserName);row.browserExtension=std::move(prepared.browserExtension);
  row.fallbackAddress=std::move(prepared.fallbackAddress);row.fallbackName=std::move(prepared.fallbackName);row.host=std::move(prepared.host);row.origin=std::move(prepared.origin);row.projectedDestination.clear();
 }
 return true;
}
inline BatchRefreshProjection projectBatchRefresh(const BatchRefreshInput& input,const std::vector<BrowserBatchRow>* browser,const Headers& headers,const Json& settings,const std::function<void()>& checkpoint={}){
 if(input.changed&&(*input.changed<0||(size_t)*input.changed>=input.count||input.rows.size()!=1))throw std::runtime_error("The changed batch row no longer matches its source.");
 if(browser&&browser->size()!=input.count)throw std::runtime_error("The browser refresh does not match its links.");
 BatchRefreshProjection result;result.sourceCount=input.count;result.changed=input.changed;result.sourceHeaders=headers;result.values.reserve(input.rows.size());result.requests.resize(input.changed?1:input.count);std::set<std::string> savedOrigins;std::set<size_t> seen;
 for(const auto& login:settings.value("SiteLogins",Json::array()))savedOrigins.insert(str(login,"Origin"));
 for(const auto& entry:input.rows){
  if(checkpoint)checkpoint();if(entry.index>=input.count||!seen.insert(entry.index).second||(input.changed&&entry.index!=(size_t)*input.changed))throw std::runtime_error("Invalid batch refresh identity.");
  BatchImportRow row;row.address=entry.address;row.name=entry.name;row.protectedRequestOverrides=entry.protectedOverrides;batchRowFallback(row);
  const BrowserBatchRow* source=browser&&browser->at(entry.index).address==row.address?&browser->at(entry.index):nullptr;
  auto& request=result.requests[input.changed?0:entry.index];
  if(source&&!source->media.empty())row.metadata={{"Status","Ready"},{"Size",-1}};
  else if(source&&source->observedSize>0&&headers.empty()&&row.protectedRequestOverrides.empty()&&(source->proxy.empty()||yes(settings,"UseBrowserProxy",true))&&!savedOrigins.count(row.origin))
   row.metadata={{"Status","Ready"},{"Size",source->observedSize},{"ContentType",source->observedType},{"SuggestedFileName",previewResponseFilename(row.address,source->observedName,source->observedType,yes(settings,"CorrectFileExtensions",true))}};
  else{request.address=row.address;request.headers=headers;request.protectedOverrides=row.protectedRequestOverrides;if(source){for(const auto& [name,value]:source->headers)setHeader(request.headers,name,value);request.proxy=source->proxy;request.session=source->session;}}
  if(browser)refreshBatchBrowserName(row,browser->at(entry.index));
  result.values.push_back({entry.index,std::move(row)});
 }
 if(checkpoint)checkpoint();return result;
}
}
