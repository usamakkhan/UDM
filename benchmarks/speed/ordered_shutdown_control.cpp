#include "Core.hpp"
#include <future>
#include <iostream>

namespace udm {
struct CatalogCheckpointTestAccess {
 static void observe(Manager& manager, std::function<void(const char*)> callback) {
  Lock lock(manager.mutex);
  manager.catalogCheckpoint = std::move(callback);
 }
};
}
using namespace udm;

struct CheckpointGate {
 std::atomic<DWORD> saver{0};
 std::atomic_bool armed{false}, held{false}, expired{false};
 std::mutex mutex;
 std::condition_variable changed;
 bool released = false;
 void release() {
  std::lock_guard<std::mutex> lock(mutex);
  released = true;
  changed.notify_all();
 }
 void capture(const char* stage) {
  if (std::string_view(stage) != "snapshot-prepared" ||
      GetCurrentThreadId() != saver || !armed.exchange(false)) return;
  held = true;
  std::unique_lock<std::mutex> lock(mutex);
  if (!changed.wait_for(lock, std::chrono::seconds(15), [&] { return released; })) expired = true;
 }
};

int wmain(int argc, wchar_t** argv) {
 if (argc != 4) return 2;
 const auto area = fs::absolute(argv[1]);
 const auto mode = utf8(argv[3]);
 if (mode != "stop" && mode != "resume") return 3;
 if (mode == "stop") {
  if (fs::exists(area)) return 4;
  fs::create_directories(area);
  atomicText(area / L".udm-shutdown-control", "owned shutdown qualification", false);
 } else if (!fs::exists(area / L".udm-shutdown-control")) return 5;
 Json result, checks = Json::array();
 bool passed = false;
 std::string error;
 auto check = [&](bool value, const char* name) {
  checks.push_back({{"name", name}, {"passed", value}});
  if (!value) throw std::runtime_error(name);
 };
 try {
  CheckpointGate gate;
  Manager manager(area / L"catalog");
  {
   Lock lock(manager.mutex);
   auto& prefs = manager.state["Settings"];
   prefs["Connections"] = 8; prefs["Parallel"] = 1;
   prefs["SuppressProgressDialog"] = true; prefs["SuppressCompletionDialog"] = true;
   prefs["Sound"] = false; prefs["ClipboardMonitor"] = false;
   prefs["PrefetchFileInfo"] = false; prefs["CategoryFolders"] = false; prefs["UseHttp3"] = false;
  }
  auto job = mode == "stop" ? manager.add(utf8(argv[2]), utf8((area / L"files").wstring()),
                                         "shutdown-control.bin", "Main queue", true)
                             : manager.jobs.at(0);
  if (mode == "resume") {
   check(str(job->data, "Status") == "Paused", "Reload recognizes an interrupted transfer as paused");
   check(num(job->data, "Received") > 0 && num(job->data, "Received") < num(job->data, "Size"),
         "Reload retains partial written credit");
  }
  std::future<void> saving, stopping;
  bool saveStarted = false, stopStarted = false;
  std::atomic_bool stopEntered{false};
  try {
   if (mode == "stop") CatalogCheckpointTestAccess::observe(manager, [&](const char* stage) { gate.capture(stage); });
   manager.resume(job);
   const auto started = GetTickCount64();
   for (;;) {
    if (!stopStarted) manager.tick();
    Json live; bool active;
    { Lock lock(manager.mutex); live = job->snapshot(); active = manager.isActive(job); }
    if (mode == "stop" && !saveStarted && num(live, "Received") >= 524288) {
     saveStarted = true;
     saving = std::async(std::launch::async, [&] {
      gate.saver = GetCurrentThreadId(); gate.armed = true; manager.save();
     });
    }
    if (mode == "stop" && gate.held && !stopStarted) {
     stopping = std::async(std::launch::async, [&] { stopEntered = true; manager.stop(); });
     stopStarted = true;
    }
    if (stopStarted && stopEntered && GetTickCount64() - started > 150) {
     // The hold is outside the Manager lock. Model reads remain possible while
     // shutdown must wait for the detached generation's catalog lease.
     Sleep(100);
     check(stopping.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout,
           "Shutdown cannot return while the detached old checkpoint is held");
     check(!gate.expired && !fs::exists(job->target()) && !manager.completionSaved(job),
           "A held shutdown cannot publish an incomplete destination");
     gate.release(); saving.get(); stopping.get();
     CatalogCheckpointTestAccess::observe(manager, {});
     check(yes(job->data.value("OrderedBodyWrites", Json()), "Used"),
           "Joined shutdown reports the actual ordered generation");
     check(!gate.expired && !manager.isActive(job), "Released shutdown joins all transfer workers");
     break;
    }
    if (mode == "resume" && !active && (str(live, "Status") == "Complete" || str(live, "Status") == "Failed")) {
     manager.stop();
     check(str(job->data, "Status") == "Complete" && manager.completionSaved(job),
           "Resumed transfer reaches acknowledged publication");
     check(num(job->data, "Received") == num(job->data, "Size"), "Resumed output has exact complete byte counters");
     result["target"] = utf8(job->target().wstring());
     result["fileBytes"] = fs::file_size(job->target());
     result["fileSha256"] = fileHash(job->target());
     break;
    }
    if (GetTickCount64() - started > 90000) throw std::runtime_error("Shutdown qualification deadline exceeded");
    Sleep(5);
   }
  } catch (...) {
   gate.release();
   if (saving.valid()) try { saving.get(); } catch (...) {}
   if (stopping.valid()) try { stopping.get(); } catch (...) {}
   CatalogCheckpointTestAccess::observe(manager, {});
   try { manager.stop(); } catch (...) {}
   throw;
  }
  result["job"] = job->snapshot();
  result["catalog"] = Json::parse(readText(manager.root / L"state.json"));
  result["normalStopJoined"] = true;
  if (mode == "stop") {
   check(num(job->data, "Received") > 0 && num(job->data, "Received") < num(job->data, "Size"),
         "Joined shutdown retains partial rather than completed credit");
   check(result["catalog"]["Downloads"][0]["Received"] == job->data["Received"],
         "Final shutdown catalog matches reconciled written credit");
   result["part"] = utf8((manager.root / L"parts" / wide(job->id()) / (wide(job->id()) + L".data")).wstring());
  }
  passed = true;
 } catch (const std::exception& exception) { error = exception.what(); }
 result["passed"] = passed; result["error"] = error; result["checks"] = checks; result["mode"] = mode;
 result["qualificationFeatureDefine"] = "UDM_ORDERED_BODY_QUALIFICATION";
 result["performanceComparison"] = false;
 atomicText(area / wide(mode + "-result.json"), result.dump(2), false);
 std::cout << checks.size() << " checks; " << (passed ? "passed" : error) << "\n";
 return passed ? 0 : 1;
}
