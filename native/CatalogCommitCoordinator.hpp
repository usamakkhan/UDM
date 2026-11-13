#pragma once
#include <mutex>
#include <stdexcept>
#include <type_traits>
namespace udm {
enum class CatalogCommitStage { Acquired, Published, Released };
enum class CatalogCommitDisposition { Published, AlreadyPublished, Superseded };
struct CatalogCommitResult {
 CatalogCommitDisposition disposition;
 unsigned long long requestedSequence,durableSequence;
 bool published()const noexcept{return disposition==CatalogCommitDisposition::Published;}
};
// Non-owning coordinator over the Manager's existing writer state. It never
// takes a catalog/manager lock. Callers publish cached state after commit returns.
// Snapshot preparation, reservation merging and asynchronous admission are
// separate responsibilities; a sequence alone does not prove receipt contents.
class CatalogCommitCoordinator {
 std::mutex& mutex;
 unsigned long long& written;
public:
 CatalogCommitCoordinator(std::mutex& gate,unsigned long long& durable)noexcept:mutex(gate),written(durable){}
 template<class Write,class Observe>
 CatalogCommitResult commit(unsigned long long sequence,Write&& write,Observe&& observe){
  static_assert(std::is_nothrow_invocable_v<Observe&,CatalogCommitStage>,"Commit observation must not turn a successful write into a failure");
  if(!sequence)throw std::invalid_argument("Catalog commit sequence must be positive.");
  std::unique_lock<std::mutex> lock(mutex);observe(CatalogCommitStage::Acquired);
  if(sequence<=written){
   const CatalogCommitResult result{sequence==written?CatalogCommitDisposition::AlreadyPublished:CatalogCommitDisposition::Superseded,sequence,written};
   lock.unlock();observe(CatalogCommitStage::Released);return result;
  }
  try{write();}catch(...){lock.unlock();observe(CatalogCommitStage::Released);throw;}
  written=sequence;observe(CatalogCommitStage::Published);
  const CatalogCommitResult result{CatalogCommitDisposition::Published,sequence,written};
  lock.unlock();observe(CatalogCommitStage::Released);return result;
 }
 template<class Write>CatalogCommitResult commit(unsigned long long sequence,Write&& write){
  return commit(sequence,static_cast<Write&&>(write),[](CatalogCommitStage)noexcept{});
 }
};
}
