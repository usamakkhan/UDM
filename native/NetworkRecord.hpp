#pragma once
#include "Core.hpp"
namespace udm {
// Where the desktop records its live network helper so a later desktop process
// can adopt it. Defaults to the per-user data folder; tests point it elsewhere.
fs::path& monitorRecordPath();
}
