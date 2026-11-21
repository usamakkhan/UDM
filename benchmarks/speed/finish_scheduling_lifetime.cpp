// Diagnostic-only profile. No FileIo provider and no file-operation attribution.
// Existing full FileIo capture and its rejection gate remain independently usable.
#define UDM_SCHEDULING_TRACE_ONLY
#include "finish_realtime_lifetime.cpp"
