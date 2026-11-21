#include "Core.hpp"
#include <future>
#include <iostream>

namespace udm {
struct CatalogCheckpointTestAccess {
 static void observe(Manager& manager, std::function<void(const char*)> callback) {
  Lock lock(manager.mutex); manager.catalogCheckpoint = std::move(callback);
 }
};
}
using namespace udm;

int wmain(int argc, wchar_t** argv) {
 if (argc != 5) return 2;
 const auto area = fs::absolute(argv[1]);
 const auto mode = utf8(argv[3]), operation = utf8(argv[4]);
 if ((mode != "prepare" && mode != "resume") ||
     (operation != "folder" && operation != "refresh" && operation != "remove")) return 3;
 if (mode == "prepare") {
  if (fs::exists(area)) return 4;
  fs::create_directories(area);
  atomicText(area / L".udm-lifecycle-control", "owned lifecycle qualification", false);
 } else if (!fs::exists(area / L".udm-lifecycle-control") || operation == "remove") return 5;
 Json result, checks = Json::array();
 bool passed = false;
 std::string error;
 auto check = [&](bool value, const char* name) {
  checks.push_back({{"name", name}, {"passed", value}});
  if (!value) throw std::runtime_error(name);
 };
 try {
  struct Gate {
   std::atomic<DWORD> saver{0};
   std::atomic_bool armed{false}, held{false}, expired{false};
   std::mutex mutex; std::condition_variable changed; bool released = false;
   void release() { std::lock_guard<std::mutex> lock(mutex); released = true; changed.notify_all(); }
  } gate;
  Manager manager(area / L"catalog");
  {
   Lock lock(manager.mutex); auto& prefs = manager.state["Settings"];
   prefs["Connections"] = 8; prefs["Parallel"] = 1;
   prefs["SuppressProgressDialog"] = true; prefs["SuppressCompletionDialog"] = true;
   prefs["Sound"] = false; prefs["ClipboardMonitor"] = false;
   prefs["PrefetchFileInfo"] = false; prefs["CategoryFolders"] = false; prefs["UseHttp3"] = false;
  }
  auto job = mode == "prepare" ? manager.add(utf8(argv[2]), utf8((area / L"files").wstring()),
                                            "lifecycle-control.bin", "Main queue", true)
                                : manager.jobs.at(0);
  const auto oldFolder = str(job->data, "Folder"), oldUrl = str(job->data, "Url");
  const auto newFolder = utf8((area / L"changed-files").wstring()), newUrl = oldUrl + "?refreshed=1";
  const auto part = manager.root / L"parts" / wide(job->id()) / (wide(job->id()) + L".data");
  const auto mutate = [&] {
   if (operation == "folder") manager.configure(job, {{"Folder", newFolder}});
   else if (operation == "refresh") manager.refreshAddress(job, newUrl);
   else manager.remove(job);
  };
  const auto unchanged = [&] {
   Lock lock(manager.mutex);
   return std::find(manager.jobs.begin(), manager.jobs.end(), job) != manager.jobs.end() &&
          str(job->data, "Folder") == oldFolder && str(job->data, "Url") == oldUrl;
  };
  auto rejectActive = [&] {
   bool rejected = false;
   try { mutate(); } catch (const std::exception&) { rejected = true; }
   check(rejected && unchanged(), "Active mutation is refused without changing destination, address or membership");
  };
  std::future<void> saving, pausing;
  bool saveStarted = false, pauseStarted = false;
  try {
   if (mode == "prepare") CatalogCheckpointTestAccess::observe(manager, [&](const char* stage) {
    if (std::string_view(stage) != "snapshot-prepared" || GetCurrentThreadId() != gate.saver || !gate.armed.exchange(false)) return;
    gate.held = true; std::unique_lock<std::mutex> lock(gate.mutex);
    if (!gate.changed.wait_for(lock, std::chrono::seconds(15), [&] { return gate.released; })) gate.expired = true;
   });
   manager.resume(job); const auto started = GetTickCount64();
   for (;;) {
    manager.tick(); Json live; bool active;
    { Lock lock(manager.mutex); live = job->snapshot(); active = manager.isActive(job); }
    if (mode == "prepare" && !saveStarted && num(live, "Received") >= 524288) {
     rejectActive(); saveStarted = true;
     saving = std::async(std::launch::async, [&] {
      gate.saver = GetCurrentThreadId(); gate.armed = true; manager.save();
     });
    }
    if (mode == "prepare" && gate.held && !pauseStarted) {
     pauseStarted = true;
     pausing = std::async(std::launch::async, [&] { manager.pause(job); });
    }
    if (mode == "prepare" && pauseStarted && yes(live.value("OrderedBodyWrites", Json()), "Paused")) {
     check(active && saving.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout && !gate.expired,
           "Old checkpoint keeps the settled ordered generation active during retirement");
     rejectActive();
     check(fs::exists(part) && !fs::exists(job->target()) && !manager.completionSaved(job),
           "Pending retirement retains parts without publishing a destination");
     gate.release(); saving.get(); pausing.get(); CatalogCheckpointTestAccess::observe(manager, {});
     break;
    }
    if (mode == "resume" && !active && (str(live, "Status") == "Complete" || str(live, "Status") == "Failed")) {
     manager.stop();
     check(str(job->data, "Status") == "Complete" && manager.completionSaved(job),
           "Fresh-process recovery completes acknowledged publication");
     check(num(job->data, "Received") == num(job->data, "Size"), "Completed recovery has exact byte counters");
     result["target"] = utf8(job->target().wstring()); result["fileBytes"] = fs::file_size(job->target());
     result["fileSha256"] = fileHash(job->target()); break;
    }
    if (GetTickCount64() - started > 90000) throw std::runtime_error("Lifecycle qualification deadline exceeded");
    Sleep(5);
   }
   if (mode == "prepare") {
    const auto started = GetTickCount64();
    for (;;) {
     { Lock lock(manager.mutex); if (!manager.isActive(job)) break; }
     if (GetTickCount64() - started > 15000) throw std::runtime_error("Transfer retirement deadline exceeded");
     Sleep(5);
    }
    check(!gate.expired && str(job->data, "Status") == "Paused", "Pause finishes before inactive mutation");
    result["pausedJob"] = job->snapshot(); result["part"] = utf8(part.wstring());
    const auto partDigest = fileHash(part), previousCatalog = readText(manager.root / L"state.json");
    const auto previousJob = job->snapshot(); std::atomic_bool injected{false};
    CatalogCheckpointTestAccess::observe(manager, [&](const char* stage) {
     if (std::string_view(stage) == "snapshot-prepared" && !injected.exchange(true))
      throw std::runtime_error("Injected lifecycle catalog-save failure");
    });
    bool rejected = false;
    try { mutate(); } catch (const std::exception&) { rejected = true; }
    CatalogCheckpointTestAccess::observe(manager, {});
    check(rejected && injected && unchanged() && job->snapshot() == previousJob,
          "Failed operation save rolls back job properties and membership");
    check(readText(manager.root / L"state.json") == previousCatalog && fileHash(part) == partDigest,
          "Failed operation preserves the previous real catalog and parts bytes");
    mutate();
    if (operation == "remove") {
     check(manager.jobs.empty() && Json::parse(readText(manager.root / L"state.json"))["Downloads"].empty(),
           "Successful removal persists absent membership");
     check(!fs::exists(job->target()) && fileHash(part) == partDigest,
           "List removal does not publish or rewrite retained partial data");
    } else {
     check((operation == "folder" ? str(job->data, "Folder") == newFolder : str(job->data, "Url") == newUrl) &&
           job->data["Segments"] == previousJob["Segments"] && job->data["Received"] == previousJob["Received"],
           "Successful inactive edit preserves exact partial-credit ownership");
     check(fileHash(part) == partDigest && !fs::exists(job->target()),
           "Inactive edit leaves partial bytes intact without premature publication");
    }
    manager.stop();
   }
  } catch (...) {
   gate.release();
   if (saving.valid()) try { saving.get(); } catch (...) {}
   if (pausing.valid()) try { pausing.get(); } catch (...) {}
   CatalogCheckpointTestAccess::observe(manager, {}); try { manager.stop(); } catch (...) {}
   throw;
  }
  result["job"] = job->snapshot(); result["catalog"] = Json::parse(readText(manager.root / L"state.json"));
  result["normalStopJoined"] = true; passed = true;
 } catch (const std::exception& exception) { error = exception.what(); }
 result["passed"] = passed; result["error"] = error; result["checks"] = checks;
 result["operation"] = operation; result["mode"] = mode; result["performanceComparison"] = false;
 atomicText(area / wide(mode + "-result.json"), result.dump(2), false);
 std::cout << checks.size() << " checks; " << (passed ? "passed" : error) << "\n";
 return passed ? 0 : 1;
}
