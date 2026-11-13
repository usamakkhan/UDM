#pragma once
#include <windows.h>
#include <atomic>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
namespace udm {
// A wake-up only: no command data, pointers or UI-control access crosses the
// message boundary. Disarm before HWND destruction; reject recycled messages.
class UiTaskSignal:public std::enable_shared_from_this<UiTaskSignal> {
 inline static std::atomic<UINT_PTR> next{1};
 std::mutex mutex;HWND window=nullptr;
 const UINT_PTR identity=allocate();
 static UINT_PTR allocate(){auto key=next.load();for(;;){if(key==std::numeric_limits<UINT_PTR>::max())throw std::runtime_error("UI task identities exhausted.");if(next.compare_exchange_weak(key,key+1))return key;}}
public:
 static constexpr UINT message=WM_APP+0x34F;
 void arm(HWND target){std::lock_guard<std::mutex> lock(mutex);window=target;}
 void disarm(){std::lock_guard<std::mutex> lock(mutex);window=nullptr;}
 bool matches(WPARAM key)const noexcept{return key==identity;}
 std::function<void()> notifier(){std::weak_ptr<UiTaskSignal> weak=shared_from_this();return[weak]{if(auto target=weak.lock()){std::lock_guard<std::mutex> lock(target->mutex);if(target->window)PostMessageW(target->window,message,target->identity,0);}};}
};
}
