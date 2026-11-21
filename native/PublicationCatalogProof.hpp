#pragma once
#include "Core.hpp"
#include "WindowsPath.hpp"
#include <limits>
namespace udm {
// In-process evidence tied to the exact serialized catalog snapshot. Neither
// an ID nor an unrelated newer sequence can acknowledge a publication.
class PublicationCatalogProof {
public:
 struct RecoveryRow {Json before,after;bool inCatalog=true;};
private:
 struct RecoveryBinding {Json values=Json::object();std::set<std::string> absent;std::string id;bool inCatalog=true;};
 static RecoveryBinding recoveryBinding(const RecoveryRow& row){
  RecoveryBinding result;result.id=str(row.after,"Id");result.inCatalog=row.inCatalog;
  if(result.id!=str(row.before,"Id")||!std::regex_match(result.id,std::regex("[a-fA-F0-9]{32}"))||!fs::path(wide(str(row.after,"Folder"))).is_absolute()||str(row.after,"FileName").empty()||safeName(str(row.after,"FileName"))!=str(row.after,"FileName"))throw std::runtime_error("Invalid recovery catalog binding.");
  for(auto it=row.after.begin();it!=row.after.end();++it)if(!row.before.contains(it.key())||row.before.at(it.key())!=it.value())result.values[it.key()]=it.value();
  for(auto it=row.before.begin();it!=row.before.end();++it)if(!row.after.contains(it.key()))result.absent.insert(it.key());
  for(const char* key:{"Id","Folder","FileName","Status","Size","Received","Sha256","Finished","PublicationCommitId"}){if(row.after.contains(key))result.values[key]=row.after.at(key);else result.absent.insert(key);}
  return result;
 }
 static bool recoveryRowMatches(const RecoveryBinding& expected,const Json& row){for(auto it=expected.values.begin();it!=expected.values.end();++it)if(!row.contains(it.key())||row.at(it.key())!=it.value())return false;for(const auto& key:expected.absent)if(row.contains(key))return false;return true;}
 static bool containsRecoveryRow(const Json& catalog,const RecoveryBinding& expected){
  auto downloads=catalog.find("Downloads");if(downloads==catalog.end()||!downloads->is_array())return false;std::vector<const Json*> pending;for(const auto& row:*downloads)pending.push_back(&row);size_t count=0;bool matches=false;
  while(!pending.empty()){const auto& row=*pending.back();pending.pop_back();if(!row.is_object())continue;if(str(row,"Id")==expected.id){++count;matches=recoveryRowMatches(expected,row);}for(const char* key:{"Video","Audio"})if(row.contains(key)&&row.at(key).is_object())pending.push_back(&row.at(key));}
  return expected.inCatalog?count==1&&matches:count==0;
 }
 struct Entry {std::vector<RecoveryBinding> recovery;Json recoveryReceipt;Json retiredReceipts;Json expected,related;std::string id,job,relatedJob;std::set<std::string> retirementJobs;std::atomic<unsigned long long> durable{0};size_t captures=0;bool owner=true,resolving=false,receipt=false;explicit Entry(Json row,bool privateReceipt):expected(std::move(row)),id(str(expected,"PublicationCommitId")),job(str(expected,"Id")),receipt(privateReceipt){};};
 static bool references(const Entry& entry,const std::string& job){return !job.empty()&&(entry.job==job||entry.relatedJob==job||entry.retirementJobs.count(job));}
 static bool overlaps(const Entry& left,const Entry& right){if(references(left,right.job)||references(left,right.relatedJob))return true;for(const auto& job:right.retirementJobs)if(references(left,job))return true;return false;}
 struct State {std::mutex mutex;std::condition_variable changed;std::map<const Manager*,std::map<std::string,std::shared_ptr<Entry>>> entries;};
 static State& state(){static auto* value=new State;return *value;}
 static void retire(State& state,const Manager* manager,const std::shared_ptr<Entry>& entry){if(entry->owner||entry->captures)return;auto found=state.entries.find(manager);if(found==state.entries.end())return;auto item=found->second.find(entry->id);if(item!=found->second.end()&&item->second==entry)found->second.erase(item);if(found->second.empty())state.entries.erase(found);}
 static bool containsRow(const Json& catalog,const Json& expected){
  const auto downloads=catalog.find("Downloads");if(downloads==catalog.end()||!downloads->is_array())return false;std::vector<const Json*> pending;for(const auto& row:*downloads)pending.push_back(&row);unsigned matches=0;
  while(!pending.empty()){const auto& row=*pending.back();pending.pop_back();if(!row.is_object())continue;if(str(row,"Id")==str(expected,"Id")){++matches;for(auto it=expected.begin();it!=expected.end();++it)if(!row.contains(it.key())||row.at(it.key())!=it.value())return false;}for(const char* child:{"Video","Audio"})if(row.contains(child)&&row.at(child).is_object())pending.push_back(&row.at(child));}return matches==1;
 }
 static bool contains(const Json& catalog,const Entry& entry){
  if(!entry.retiredReceipts.is_null()){
   const auto receipts=catalog.find("PublicationReceipts");if(receipts==catalog.end())return true;if(!receipts->is_object())return false;
   for(auto it=entry.retiredReceipts.begin();it!=entry.retiredReceipts.end();++it)if(receipts->contains(it.key()))return false;
   return true;
  }
  if(!entry.recovery.empty()){for(const auto& row:entry.recovery)if(!containsRecoveryRow(catalog,row))return false;if(!entry.recoveryReceipt.is_null()){auto receipts=catalog.find("PublicationReceipts");if(receipts==catalog.end()||!receipts->is_object())return false;auto found=receipts->find(str(entry.recoveryReceipt,"PublicationCommitId"));if(found==receipts->end()||*found!=entry.recoveryReceipt)return false;}return true;}
  if(!entry.related.is_null()&&!containsRow(catalog,entry.related))return false;
  if(!entry.receipt)return containsRow(catalog,entry.expected);
  auto receipts=catalog.find("PublicationReceipts");if(receipts==catalog.end()||!receipts->is_object())return false;auto found=receipts->find(entry.id);return found!=receipts->end()&&*found==entry.expected;
 }
public:
 class Lease {
  friend class PublicationCatalogProof;const Manager* manager=nullptr;std::shared_ptr<Entry> entry;
  Lease(const Manager& owner,std::shared_ptr<Entry> value):manager(&owner),entry(std::move(value)){}
 public:
  Lease()=default;Lease(const Lease&)=delete;Lease& operator=(const Lease&)=delete;Lease(Lease&&)=default;
  Lease& operator=(Lease&& other){if(this!=&other){release();manager=other.manager;entry=std::move(other.entry);}return *this;}
  ~Lease(){release();}
  // No Manager lock: already captured writers may need it to retire.
  bool resolve(){if(!entry)return false;auto& shared=state();std::unique_lock<std::mutex> lock(shared.mutex);entry->resolving=true;shared.changed.wait(lock,[&]{return entry->durable.load(std::memory_order_acquire)||entry->captures==0;});return entry->durable.load(std::memory_order_acquire)!=0;}
  void release(){if(!entry)return;auto& shared=state();{std::lock_guard<std::mutex> lock(shared.mutex);entry->owner=false;retire(shared,manager,entry);}entry.reset();}
 };
 class Publication {
  friend class PublicationCatalogProof;const Manager* manager=nullptr;std::vector<std::shared_ptr<Entry>> entries;
  Publication(const Manager& owner,std::vector<std::shared_ptr<Entry>> values):manager(&owner),entries(std::move(values)){}
  void release(){if(entries.empty())return;auto& shared=state();{std::lock_guard<std::mutex> lock(shared.mutex);for(const auto& entry:entries){--entry->captures;retire(shared,manager,entry);}shared.changed.notify_all();}entries.clear();}
 public:
  Publication()=default;Publication(const Publication&)=delete;Publication& operator=(const Publication&)=delete;Publication(Publication&&)=default;
  Publication& operator=(Publication&& other){if(this!=&other){release();manager=other.manager;entries=std::move(other.entries);}return *this;}
  ~Publication(){release();}explicit operator bool()const noexcept{return !entries.empty();}
  // Only after these exact snapshot bytes were successfully published.
  void published(unsigned long long sequence)noexcept{if(!sequence||entries.empty())return;auto& shared=state();std::lock_guard<std::mutex> lock(shared.mutex);for(const auto& entry:entries){auto previous=entry->durable.load(std::memory_order_relaxed);while(previous<sequence&&!entry->durable.compare_exchange_weak(previous,sequence,std::memory_order_release,std::memory_order_relaxed)){};}shared.changed.notify_all();}
 };
 // Caller holds Manager::mutex while installing the complete row.
 // Complete byte counters are exact bounded integers, never unknown or
 // fractional values. Paused rows retain their separate partial-byte schema.
 static bool completeByteCounts(const Json& row){
  i64 counts[2]{};size_t index=0;
  for(const char* key:{"Size","Received"}){
   auto value=row.find(key);if(value==row.end()||!value->is_number_integer())return false;
   if(value->is_number_unsigned()){const auto count=value->get<unsigned long long>();if(count>static_cast<unsigned long long>(std::numeric_limits<i64>::max()))return false;counts[index++]=static_cast<i64>(count);}
   else{const auto count=value->get<i64>();if(count<0)return false;counts[index++]=count;}
  }
  return counts[0]==counts[1];
 }
 static Json binding(const Json& row){Json expected=Json::object();for(const char* key:{"Id","Folder","FileName","Status","Size","Received","Sha256","Finished","PublicationCommitId"}){if(!row.contains(key))throw std::runtime_error("Missing publication catalog binding.");expected[key]=row.at(key);}if(str(row,"Status")!="Complete"||!completeByteCounts(row)||!std::regex_match(str(row,"PublicationCommitId"),std::regex("[a-f0-9]{32}"))||!std::regex_match(str(row,"Id"),std::regex("[a-fA-F0-9]{32}"))||!fs::path(wide(str(row,"Folder"))).is_absolute()||str(row,"FileName").empty()||safeName(str(row,"FileName"))!=str(row,"FileName"))throw std::runtime_error("Invalid publication catalog binding.");return expected;}
 static bool installReceipt(Manager& manager,const Json& descriptor){auto found=manager.state.find("PublicationReceipts");Json receipts=found==manager.state.end()?Json::object():*found;if(!receipts.is_object())throw std::runtime_error("Invalid private publication receipt store.");const auto id=str(descriptor,"PublicationCommitId");auto existing=receipts.find(id);if(existing!=receipts.end()){if(*existing!=descriptor)throw std::runtime_error("Conflicting private publication receipt.");return false;}receipts[id]=descriptor;if(found==manager.state.end())manager.state.emplace("PublicationReceipts",std::move(receipts));else found->swap(receipts);return true;}
 static bool eraseReceipt(Manager& manager,const Json& descriptor){auto found=manager.state.find("PublicationReceipts");if(found==manager.state.end()||!found->is_object())return false;auto item=found->find(str(descriptor,"PublicationCommitId"));if(item==found->end()||*item!=descriptor)return false;found->erase(item);if(found->empty())manager.state.erase(found);return true;}
 static Lease prepare(const Manager& manager,const Json& row,bool privateReceipt=false,const Json* related=nullptr){
  auto entry=std::make_shared<Entry>(binding(row),privateReceipt);if(related){entry->related=binding(*related);entry->relatedJob=str(entry->related,"Id");if(entry->relatedJob==entry->job||str(entry->related,"PublicationCommitId")!=entry->id)throw std::runtime_error("Invalid paired publication binding.");}auto& shared=state();std::lock_guard<std::mutex> lock(shared.mutex);auto found=shared.entries.find(&manager);
  if(found!=shared.entries.end())for(const auto& item:found->second)if(item.first==entry->id||(item.second->owner&&overlaps(*item.second,*entry)))throw std::runtime_error("Another catalog publication owns this completion.");
  if(found==shared.entries.end()){std::map<std::string,std::shared_ptr<Entry>> entries;entries.emplace(entry->id,entry);shared.entries.emplace(&manager,std::move(entries));}else found->second.emplace(entry->id,entry);return Lease(manager,std::move(entry));
 }
 // Recovery binds the owned row delta (including removed fields), identity
 // and membership to actual serialized snapshots. No persisted ID or sequence
 // alone can acknowledge a legacy/Paused/private recovery outcome.
 static Lease prepareRecovery(const Manager& manager,const std::vector<RecoveryRow>& rows,const Json& receipt=Json()){
  if(rows.empty()||rows.size()>2)throw std::runtime_error("Invalid recovery catalog rows.");
  auto entry=std::make_shared<Entry>(rows.front().after,false);entry->id=guid();
  for(const auto& row:rows){if(str(row.after,"Status")!="Complete"&&str(row.after,"Status")!="Paused")throw std::runtime_error("Invalid recovery outcome binding.");if(str(row.after,"Status")=="Complete"&&!completeByteCounts(row.after))throw std::runtime_error("Invalid recovered completion byte counts.");if(!row.inCatalog&&str(row.after,"Status")=="Complete"&&(receipt.is_null()||str(receipt,"Id")!=str(row.after,"Id")))throw std::runtime_error("A private recovered completion requires its exact catalog receipt.");entry->recovery.push_back(recoveryBinding(row));}
  if(rows.size()==2){entry->relatedJob=str(rows.back().after,"Id");if(entry->relatedJob==entry->job)throw std::runtime_error("Duplicate recovery catalog row.");}
  if(!receipt.is_null()){if(binding(receipt)!=receipt)throw std::runtime_error("Invalid private recovery receipt.");entry->recoveryReceipt=receipt;}
  auto& shared=state();std::lock_guard<std::mutex> lock(shared.mutex);auto found=shared.entries.find(&manager);
  if(found!=shared.entries.end())for(const auto& item:found->second)if(item.first==entry->id||(item.second->owner&&overlaps(*item.second,*entry)))throw std::runtime_error("Another catalog publication owns this recovery.");
  if(found==shared.entries.end()){std::map<std::string,std::shared_ptr<Entry>> entries;entries.emplace(entry->id,entry);shared.entries.emplace(&manager,std::move(entries));}else found->second.emplace(entry->id,entry);return Lease(manager,std::move(entry));
 }
 // Receipt absence is a distinct durable outcome, not a completed row binding.
 static Lease prepareReceiptRetirement(const Manager& manager,const Json& receipts){
  if(!receipts.is_object()||receipts.empty())throw std::runtime_error("Missing receipt retirement bindings.");
  auto entry=std::make_shared<Entry>(Json{{"Id",""},{"PublicationCommitId",guid()}},false);entry->retiredReceipts=receipts;
  for(auto it=receipts.begin();it!=receipts.end();++it){if(binding(it.value())!=it.value()||it.key()!=str(it.value(),"PublicationCommitId"))throw std::runtime_error("Invalid receipt retirement binding.");entry->retirementJobs.insert(str(it.value(),"Id"));}
  auto& shared=state();std::lock_guard<std::mutex> lock(shared.mutex);auto found=shared.entries.find(&manager);
  if(found!=shared.entries.end())for(const auto& item:found->second)if(item.first==entry->id||(item.second->owner&&overlaps(*item.second,*entry)))throw std::runtime_error("Another catalog publication owns this receipt retirement.");
  if(found==shared.entries.end()){std::map<std::string,std::shared_ptr<Entry>> entries;entries.emplace(entry->id,entry);shared.entries.emplace(&manager,std::move(entries));}else found->second.emplace(entry->id,entry);return Lease(manager,std::move(entry));
 }
 static bool recoveryMatches(const Json& before,const Json& applied,const Json& current){return recoveryRowMatches(recoveryBinding(RecoveryRow{before,applied,true}),current);}
 // Caller holds Manager::mutex. Validate all bindings before capturing any.
 static Publication capture(const Manager& manager,const Json& catalog){auto& shared=state();std::lock_guard<std::mutex> lock(shared.mutex);auto found=shared.entries.find(&manager);if(found==shared.entries.end())return {};std::vector<std::shared_ptr<Entry>> included;
  for(const auto& item:found->second){const auto& entry=item.second;if(!entry->owner){if(entry->captures&&!entry->durable.load(std::memory_order_acquire))throw std::runtime_error("An abandoned publication outcome is still pending.");continue;}if(entry->resolving&&!entry->durable.load(std::memory_order_acquire))throw std::runtime_error("A publication catalog outcome is being resolved. Retry shortly.");if(!contains(catalog,*entry))throw std::runtime_error("A catalog snapshot cannot omit or change its pending publication.");included.push_back(entry);}
  for(const auto& entry:included)++entry->captures;return Publication(manager,std::move(included));
 }
};
}
