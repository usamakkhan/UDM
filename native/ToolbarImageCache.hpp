#pragma once
#include "ToolbarModel.hpp"
#include <list>
#include <memory>
#include <optional>
#define UDM_BUILTIN_TOOLBAR_CACHE_AVAILABLE
namespace udm {
// UI-thread-owned caches. Borrowers retain immutable pixels through invalidation.
class ToolbarSourceCache {
 std::array<std::shared_ptr<const ToolbarBitmap>,ToolbarCommandCount> images{};
 uint64_t epoch=0;
public:
 template<class Loader>std::shared_ptr<const ToolbarBitmap> get(int index,Loader load){
  if(index<0||index>=ToolbarCommandCount)throw std::runtime_error("Invalid toolbar icon index.");
  auto& image=images[index];if(!image){auto value=load(index);if(value.width<1||value.height<1||value.width>256||value.height>256||value.pixels.size()!=(size_t)value.width*value.height*4)throw std::runtime_error("Invalid decoded toolbar icon.");image=std::make_shared<const ToolbarBitmap>(std::move(value));}return image;
 }
 void clear()noexcept{for(auto& image:images)image.reset();++epoch;}
 uint64_t generation()const noexcept{return epoch;}
 size_t entries()const noexcept{size_t n=0;for(const auto& image:images)n+=image!=nullptr;return n;}
 size_t bytes()const noexcept{size_t n=0;for(const auto& image:images)if(image)n+=image->pixels.size();return n;}
};
struct ToolbarRasterKey {
 int width=0,height=0;bool raised=false;uint64_t generation=0;
 friend bool operator==(const ToolbarRasterKey& a,const ToolbarRasterKey& b)noexcept{return a.width==b.width&&a.height==b.height&&a.raised==b.raised&&a.generation==b.generation;}
};
struct ToolbarRasterSet {std::array<std::array<Bytes,3>,ToolbarCommandCount> pixels;};
class ToolbarRasterCache {
 struct Entry {ToolbarRasterKey key;std::shared_ptr<const ToolbarRasterSet> data;size_t bytes;};
 std::list<Entry> frames;size_t retained=0,entryLimit,byteLimit;
 static std::optional<size_t> sizeFor(const ToolbarRasterKey& key)noexcept{
  if(key.width<1||key.height<1)return {};const uint64_t pixels=(uint64_t)key.width*key.height;
  if(pixels>MaxBytes/(ToolbarCommandCount*3*4))return {};return (size_t)pixels*ToolbarCommandCount*3*4;
 }
public:
 static constexpr size_t MaxEntries=4,MaxBytes=8*1024*1024;
 explicit ToolbarRasterCache(size_t entryCount=MaxEntries,size_t budget=MaxBytes):entryLimit(entryCount),byteLimit(budget){if(!entryCount||entryCount>MaxEntries||!budget||budget>MaxBytes)throw std::runtime_error("Invalid toolbar cache limits.");}
 bool cacheable(const ToolbarRasterKey& key)const noexcept{auto size=sizeFor(key);return size&&*size<=byteLimit;}
 template<class Builder>std::shared_ptr<const ToolbarRasterSet> get(const ToolbarRasterKey& key,Builder build){
  const auto size=sizeFor(key);if(!size||*size>byteLimit)throw std::runtime_error("Toolbar raster is too large to cache.");
  for(auto it=frames.begin();it!=frames.end();++it)if(it->key==key){auto data=it->data;frames.splice(frames.begin(),frames,it);return data;}
  auto value=build();const size_t bytes=(size_t)key.width*key.height*4;for(const auto& icon:value.pixels)for(const auto& state:icon)if(state.size()!=bytes)throw std::runtime_error("Invalid toolbar raster pixels.");
  auto data=std::make_shared<const ToolbarRasterSet>(std::move(value));frames.push_front({key,data,*size});retained+=*size;
  while(frames.size()>entryLimit||retained>byteLimit){retained-=frames.back().bytes;frames.pop_back();}return data;
 }
 void clear()noexcept{frames.clear();retained=0;}
 size_t entries()const noexcept{return frames.size();}size_t bytes()const noexcept{return retained;}
};
}
