#pragma once
#include "DownloadPreview.hpp"
#include "PreviewFilename.hpp"
#include "GuiModels.hpp"
#include "OptionsModel.hpp"
namespace udm {
struct BatchImportRow {
 std::string address,name,folder,description;Json metadata=Json::object();bool checked=true,selectionTouched=false,descriptionEdited=false;
 std::string protectedRequestOverrides,browserName,browserExtension,fallbackAddress,fallbackName,host,origin,projectedDestination,projectedDestinationName;
};
inline const std::string& batchRowFallback(BatchImportRow& row){
 if(row.fallbackAddress!=row.address||row.fallbackName.empty()){Url parsed(row.address);row.projectedDestination.clear();row.fallbackName=safeName(unescape(parsed.path));row.host=parsed.host;row.origin=parsed.origin;row.fallbackAddress=row.address;}
 return row.fallbackName;
}
inline void refreshBatchBrowserName(BatchImportRow& row,const BrowserBatchRow& source){
 batchRowFallback(row);auto name=row.name;
 if(name.empty()){const auto suggested=str(row.metadata,"SuggestedFileName");if(!source.media.empty())name=source.suggestedName;else name=!suggested.empty()&&safeName(suggested)==suggested?suggested:row.fallbackName;}
 if(name==row.browserName)return;row.browserName=std::move(name);auto extension=lower(utf8(fs::path(wide(row.browserName)).extension().wstring()));row.browserExtension=extension.size()>=2&&extension.size()<=12?std::move(extension):std::string{};
}
struct BatchRowProjection {std::vector<BatchImportRow> rows;std::vector<int> visible;bool hasRepeated=false;std::vector<BatchPreviewRequest> requests;std::shared_ptr<BatchDownloadPreview> preview;};
inline BatchRowProjection projectBatchRows(const std::vector<std::string>& urls,const std::vector<BrowserBatchRow>* browser,const Headers& headers,const Json& settings,const std::function<void()>& checkpoint={}){
 if(browser&&browser->size()!=urls.size())throw std::runtime_error("The browser batch does not match its links.");BatchRowProjection result;result.rows.reserve(urls.size());result.visible.reserve(urls.size());result.requests.resize(urls.size());std::set<std::string> seen,savedOrigins;
 for(const auto& login:settings.value("SiteLogins",Json::array()))savedOrigins.insert(str(login,"Origin"));
 for(size_t index=0;index<urls.size();++index){
  if(checkpoint)checkpoint();BatchImportRow row;row.address=urls[index];batchRowFallback(row);const BrowserBatchRow* source=nullptr;
  if(browser){const auto& raw=browser->at(index);row.name=raw.suggestedName;row.checked=false;row.selectionTouched=true;if(raw.address==row.address)source=&raw;}
  if(source&&!source->media.empty())row.metadata={{"Status","Ready"},{"Size",-1}};
  else if(source&&source->observedSize>0&&headers.empty()&&(source->proxy.empty()||yes(settings,"UseBrowserProxy",true))&&!savedOrigins.count(row.origin))row.metadata={{"Status","Ready"},{"Size",source->observedSize},{"ContentType",source->observedType},{"SuggestedFileName",previewResponseFilename(row.address,source->observedName,source->observedType,yes(settings,"CorrectFileExtensions",true))}};
  else{auto& request=result.requests[index];request.address=row.address;request.headers=headers;if(source){for(const auto& [name,value]:source->headers)setHeader(request.headers,name,value);request.proxy=source->proxy;request.session=source->session;}}
  if(browser){refreshBatchBrowserName(row,browser->at(index));const auto category=downloadCategory(row.browserName,row.host,settings);row.projectedDestination=utf8((fs::path(wide(categoryFolder(settings,category)))/wide(row.browserName)).wstring());row.projectedDestinationName=row.browserName;}
  const bool repeated=browser&&(!source||source->media.empty())&&!seen.insert(row.address).second;result.hasRepeated|=repeated;if(!repeated&&!(browser&&source&&source->imageOnPage))result.visible.push_back(static_cast<int>(index));result.rows.push_back(std::move(row));
 }
 if(checkpoint)checkpoint();return result;
}
}
