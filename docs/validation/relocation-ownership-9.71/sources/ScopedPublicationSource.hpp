#pragma once
#include "PublicationSourcePin.hpp"
namespace udm {
// Same-thread, single-use handoff of an actual file object. This neither accepts
// caller-supplied identity metadata nor changes durable catalog acceptance.
class ScopedPublicationSource {
 inline static thread_local ScopedPublicationSource* current=nullptr;
 ScopedPublicationSource* previous;Manager* manager;const Job* job;fs::path stage;
 std::unique_ptr<PublicationSourcePin> pin;bool consumed=false;
public:
 ScopedPublicationSource(Manager& m,const JobPtr& j,const fs::path& path,std::unique_ptr<PublicationSourcePin> source):previous(current),manager(&m),job(j.get()),stage(path.lexically_normal()),pin(std::move(source)){
  if(!j||!stage.is_absolute()||!pin)throw std::runtime_error("Invalid pinned publication handoff.");current=this;
 }
 ScopedPublicationSource(const ScopedPublicationSource&)=delete;
 ~ScopedPublicationSource(){current=previous;}
 static std::unique_ptr<PublicationSourcePin> take(Manager& m,const JobPtr& j,const fs::path& path){
  auto owner=current;if(!owner||owner->manager!=&m||owner->job!=j.get())return {};
  if(owner->consumed||!owner->pin||owner->stage!=path.lexically_normal())throw std::runtime_error("The pinned publication source does not match this handoff.");
  owner->consumed=true;return std::move(owner->pin);
 }
};
}
