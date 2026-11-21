#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
namespace udm {
// Storage and its process-wide budget share one lifetime. The final reference
// destroys bytes before releasing the token, on whichever thread owns it then.
class BodyBufferStorage {
public:
 using Byte=std::uint8_t;
 using Bytes=std::vector<Byte>;
 using Ptr=std::shared_ptr<BodyBufferStorage>;
 static constexpr std::size_t maxBytes=64*1048576;
private:
 struct Lease {
  inline static std::atomic_bool occupied{false};
  bool owned=false;
  Lease()noexcept{bool free=false;owned=occupied.compare_exchange_strong(free,true,std::memory_order_acquire,std::memory_order_relaxed);}
  Lease(Lease&& other)noexcept:owned(std::exchange(other.owned,false)){}
  Lease(const Lease&)=delete;Lease& operator=(const Lease&)=delete;Lease& operator=(Lease&&)=delete;
  ~Lease(){if(owned)occupied.store(false,std::memory_order_release);}
 };
 Lease lease;
 Bytes body;
 BodyBufferStorage(Lease&& token,std::size_t size):lease(std::move(token)),body(size){}
public:
 BodyBufferStorage(const BodyBufferStorage&)=delete;BodyBufferStorage& operator=(const BodyBufferStorage&)=delete;
 BodyBufferStorage(BodyBufferStorage&&)=delete;BodyBufferStorage& operator=(BodyBufferStorage&&)=delete;
 ~BodyBufferStorage()=default;
 static Ptr reserve(std::size_t size)noexcept{
  if(!size||size>maxBytes)return {};
  Lease token;if(!token.owned)return {};
  try{return Ptr(new BodyBufferStorage(std::move(token),size));}catch(...){return {};}
 }
 std::size_t size()const noexcept{return body.size();}
 Byte* data()noexcept{return body.data();}
 const Bytes& bytes()const noexcept{return body;}
};
}
