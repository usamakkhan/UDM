#pragma once
#include "Core.hpp"
#include <condition_variable>
#include <list>
namespace udm {
// Own displayed receipt tokens until their acknowledgement is saved. This queue
// never owns a window or borrows a modal dialog's controls/stack.
class BrowserPresentationAcks {
 enum class Phase {Pending,Running,Failed,Dismissed};
 struct Entry {uint64_t key,attempt=1;Json offer;Phase phase=Phase::Pending;std::string error;};
 mutable std::mutex mutex;std::condition_variable wake;std::thread worker;
 std::list<std::shared_ptr<Entry>> entries;std::set<std::string> claimed;
 std::function<void(const Json&)> acknowledge;std::function<void()> notify;
 uint64_t next=1;bool stopping=false;
 void release(const Entry& entry){for(const auto& token:entry.offer.at("tokens"))claimed.erase(token.get_ref<const std::string&>());}
 void run()noexcept{
  for(;;){
   std::shared_ptr<Entry> entry;
   {std::unique_lock<std::mutex> lock(mutex);wake.wait(lock,[&]{return stopping||std::any_of(entries.begin(),entries.end(),[](const auto& e){return e->phase==Phase::Pending;});});
    for(const auto& candidate:entries)if(candidate->phase==Phase::Pending){entry=candidate;break;}
    if(!entry){if(stopping)return;continue;}entry->phase=Phase::Running;
   }
   bool succeeded=false;std::string error;
   try{
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    if(beforeAcknowledgeForTest)beforeAcknowledgeForTest();
#endif
    acknowledge(entry->offer);succeeded=true;}
   catch(const std::exception& e){try{error.assign(e.what(),std::min<size_t>(std::char_traits<char>::length(e.what()),4096));}catch(...) {}}
   catch(...) {}
   {std::lock_guard<std::mutex> lock(mutex);
    if(succeeded){release(*entry);entries.remove(entry);}
    else{entry->error=std::move(error);entry->phase=Phase::Failed;}
   }
   if(notify)try{notify();}catch(...) {}
  }
 }
public:
#ifdef UDM_TOOLBAR_COMPONENT_TEST
 std::function<void()> beforeAcknowledgeForTest;
#endif
 struct Failure {uint64_t key=0,attempt=0;std::string error;explicit operator bool()const noexcept{return key!=0;}};
 explicit BrowserPresentationAcks(std::function<void(const Json&)> finish,std::function<void()> ready={}):acknowledge(std::move(finish)),notify(std::move(ready)){if(!acknowledge)throw std::invalid_argument("Missing browser acknowledgement handler.");}
 BrowserPresentationAcks(const BrowserPresentationAcks&)=delete;BrowserPresentationAcks& operator=(const BrowserPresentationAcks&)=delete;
 ~BrowserPresentationAcks(){stop();}
 bool enqueue(const Json& offer){
  const auto found=offer.find("tokens");if(!offer.is_object()||found==offer.end()||!found->is_array()||found->empty()||found->size()>2048)throw std::invalid_argument("Invalid browser acknowledgement tokens.");
  std::set<std::string> tokens;for(const auto& token:*found){if(!token.is_string()||token.get_ref<const std::string&>().empty()||token.get_ref<const std::string&>().size()>256)throw std::invalid_argument("Invalid browser acknowledgement token.");tokens.insert(token.get<std::string>());}
  std::unique_lock<std::mutex> lock(mutex);if(stopping)return false;
  for(auto it=tokens.begin();it!=tokens.end();)if(claimed.count(*it))it=tokens.erase(it);else ++it;
  if(tokens.empty())return true;
  if(entries.size()>=64||next==std::numeric_limits<uint64_t>::max())return false;
  auto entry=std::make_shared<Entry>();entry->key=next;entry->offer={{"tokens",Json::array()}};for(const auto& token:tokens)entry->offer["tokens"].push_back(token);
  if(!worker.joinable())worker=std::thread([this]{run();});
  entries.push_back(entry);
  try{claimed.insert(tokens.begin(),tokens.end());}catch(...){for(const auto& token:tokens)claimed.erase(token);entries.pop_back();throw;}
  ++next;lock.unlock();wake.notify_one();return true;
 }
 bool owns(const Json& offer)const{
  const auto found=offer.find("tokens");if(found==offer.end()||!found->is_array()||found->empty())return false;
  std::lock_guard<std::mutex> lock(mutex);for(const auto& token:*found)if(!token.is_string()||!claimed.count(token.get_ref<const std::string&>()))return false;return true;
 }
 Failure failure()const{
  std::lock_guard<std::mutex> lock(mutex);for(const auto& entry:entries)if(entry->phase==Phase::Failed)return {entry->key,entry->attempt,entry->error.empty()?"UDM could not save the browser presentation acknowledgement.":entry->error};return {};
 }
 bool retry(const Failure& failure){
  {std::lock_guard<std::mutex> lock(mutex);if(stopping)return false;for(const auto& entry:entries)if(entry->key==failure.key&&entry->attempt==failure.attempt&&entry->phase==Phase::Failed&&entry->attempt<std::numeric_limits<uint64_t>::max()){++entry->attempt;entry->error.clear();entry->phase=Phase::Pending;wake.notify_one();return true;}}return false;
 }
 bool dismiss(const Failure& failure){
  std::lock_guard<std::mutex> lock(mutex);for(const auto& entry:entries)if(entry->key==failure.key&&entry->attempt==failure.attempt&&entry->phase==Phase::Failed){entry->phase=Phase::Dismissed;return true;}return false;
 }
 bool pending()const{std::lock_guard<std::mutex> lock(mutex);return std::any_of(entries.begin(),entries.end(),[](const auto& e){return e->phase==Phase::Pending||e->phase==Phase::Running;});}
 // Accepted acknowledgements drain before Manager may be destroyed. A failed or
 // dismissed acknowledgement remains persisted as an offer for the next launch.
 void stop(){ {std::lock_guard<std::mutex> lock(mutex);stopping=true;}wake.notify_one();if(worker.joinable())worker.join();}
 void restart(){std::lock_guard<std::mutex> lock(mutex);if(worker.joinable())throw std::logic_error("Join browser acknowledgements before restart.");stopping=false;}
};
}
