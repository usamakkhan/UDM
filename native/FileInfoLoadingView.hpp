#pragma once
#include "FileInfoInitialView.hpp"
#include "FileInfoNameTask.hpp"
#include "MediaSize.hpp"
namespace udm {
// Advisory, immutable display values only. They never authorize a transfer or
// replace the initial reader's membership/request/current-metadata checks.
struct FileInfoLoadingView {
 bool available=false;std::string address,category,destination,description;
 std::string amount=bytes(-1),kind,detail,serverName;
};
inline FileInfoLoadingView fileInfoLoadingView(const std::shared_ptr<const FileInfoInitialView>& prepared,const std::string& jobId){
 FileInfoLoadingView result;if(!prepared||prepared->jobId!=jobId)return result;
 result.available=true;const auto& data=prepared->data;
 result.address=str(data,"SourceUrl",str(data,"Url"));result.category=str(data,"Category");result.destination=utf8(prepared->target.wstring());result.description=str(data,"Description");
 i64 length=-1;
 if(prepared->media&&str(data,"Status")!="Complete"){
  i64 total=0;bool known=false,unknown=false;
  for(const auto* key:{"Video","Audio"})if(auto found=data.find(key);found!=data.end()&&found->is_object()){
   const auto size=num(*found,"Size",-1);if(size<0||size>std::numeric_limits<i64>::max()-total){unknown=true;break;}known=true;total+=size;
  }
  length=known&&!unknown&&total>0?total:reportedMediaSize(data);
  if(length>0){result.kind="Selected track size";result.detail="Selected tracks: "+std::to_string(length)+" bytes";}
  else if((length=mediaSizeHint(data))>0){result.amount="~ "+bytes(length);result.kind="Estimated size";result.detail="Estimated size: "+std::to_string(length)+" bytes";return result;}else length=-1;
 }else if(!prepared->media){
  length=httpOutputSize(data);if(length>=0){result.kind="Recorded size";result.detail="Recorded size: "+std::to_string(length)+" bytes";}
  else if((length=reportedDirectResponseSize(data))>0){result.kind="Reported size";result.detail="Browser-reported size: "+std::to_string(length)+" bytes";}else length=-1;
  const auto name=fileInfoServerSuggestion(data,false,false,{},Json::object());if(!name.empty()&&safeName(name)==name)result.serverName=name;
 }
 if(length>=0)result.amount=bytes(length);
 return result;
}
}
