#pragma once
#include "Core.hpp"

namespace udm {
struct DashXmlNode {
 std::string name,namespaceUri,text;
 std::map<std::string,std::string> attributes;
 std::vector<size_t> children;
};
struct DashManifest {
 std::vector<DashXmlNode> nodes;
 bool dynamic=false;
 std::optional<double> duration,minimumUpdatePeriod,timeShiftBufferDepth,suggestedDelay;
 std::optional<i64> availabilityStartUs,availabilityEndUs,publishTimeUs;
};
// Bounded native parsing for future MPD refreshes. It does not fetch resources,
// expand segment templates, or start a recording.
DashManifest parseDashManifest(const std::string&,const Cancel&);
struct DashPeriodTiming {
 size_t node=0,index=0;
 std::optional<std::string> id;
 double start=0;
 std::optional<double> duration;
};
std::vector<DashPeriodTiming> resolveDashPeriods(const DashManifest&,const Cancel&);
struct DashTimelineEntry {
 std::optional<i64> time;
 i64 duration=0,repeat=0;
};
struct DashSegmentReference {
 i64 number=0,time=0,duration=0;
 std::optional<std::pair<i64,i64>> byteRange; // start, length
 Json resourceVersion; // null for legacy resources; exact URL, total and strong ETag
};
struct DashIndexReference {
 bool index=false;
 i64 start=0,length=0,time=0,duration=0;
 uint32_t sap=0; // Original starts-with-SAP/type/delta field; not a clock guess.
};
struct DashIndexBox {
 i64 start=0,length=0,earliest=0,firstOffset=0,duration=0;
 uint32_t referenceId=0,timescale=0;
 std::vector<DashIndexReference> references;
};
// Decode bounded ISO BMFF SIDX boxes. Embedded offsets use the index resource;
// external indexes keep separate index/media address spaces. No network I/O.
std::vector<DashIndexBox> parseDashIndexes(const Bytes&,i64 indexStart,i64 resourceSize,
 const Cancel&,std::optional<i64> externalMediaSize={});
struct DashIndexedMedia {DashIndexReference reference;uint32_t referenceId=0,timescale=0;};
// The reader must return the exact range from one version-bound index resource.
// Traversal owns byte/request/depth budgets and exact parent/child clock checks.
std::vector<DashIndexedMedia> resolveDashIndexes(i64 start,i64 length,i64 resourceSize,
 const std::function<Bytes(i64,i64,const Cancel&)>& reader,const Cancel&,
 std::optional<i64> externalMediaSize={});
std::vector<DashTimelineEntry> readDashTimeline(const DashManifest&,size_t,const Cancel&);
// Times are integer media ticks, before presentationTimeOffset adjustment.
// Selects overlap with [windowStart,windowEnd); availability is a caller policy.
std::vector<DashSegmentReference> expandDashTimeline(
 const std::vector<DashTimelineEntry>&,i64 startNumber,i64 windowStart,i64 windowEnd,const Cancel&,
 std::optional<i64> endNumber={});
struct DashTemplateValues {
 std::optional<std::string> representationId;
 std::optional<i64> bandwidth,number,time;
};
// Substitutes a URI reference; base URL resolution and request policy are separate.
std::string expandDashTemplate(const std::string&,const DashTemplateValues&);
struct DashTemplateAddressing {
 struct ListedResource {std::string media;std::optional<std::pair<i64,i64>> byteRange;};
 struct BaseTiming {double offset=0;bool complete=true;};
 std::vector<std::string> bases;
 std::vector<BaseTiming> baseTiming;
 double availabilityOffset=0;
 bool availabilityComplete=true;
 std::string media,initialization;
 DashTemplateValues values;
 i64 timescale=1,presentationTimeOffset=0,startNumber=1;
 i64 earliestPresentationDelta=0; // Nominal duration-template coverage; excluded from $Time$ URI values.
 std::optional<i64> endNumber;
 DashPeriodTiming period;
 std::vector<DashTimelineEntry> timeline;
 bool listed=false,hasInitialization=false;
 bool indexed=false;
 std::string index;
 std::optional<std::pair<i64,i64>> indexRange;
 std::optional<uint32_t> indexReferenceId;
 Json indexVersion;
 Json indexSource;
 std::vector<ListedResource> resources;
 std::vector<DashSegmentReference> listedReferences;
 std::optional<std::pair<i64,i64>> initializationRange;
};
struct DashPlannedSegment {
 DashSegmentReference reference;
 std::vector<std::string> urls;
};
struct DashResourceWindow {
 i64 timescale=1,presentationTimeOffset=0;
 DashPeriodTiming period;
 std::vector<std::string> initialization;
 std::vector<DashPlannedSegment> segments;
 std::optional<std::pair<i64,i64>> initializationRange;
 Json initializationVersion;
};
DashTemplateAddressing resolveDashTemplate(const DashManifest&,size_t representation,
 const std::string& manifestUrl,const Cancel&);
DashResourceWindow planDashTemplateWindow(const DashTemplateAddressing&,i64 windowStart,
 i64 windowEnd,const Cancel&);
// nowUs must come from a synchronized server clock. The requested window remains
// media ticks; this filters availability, not playback delay or refresh validity.
DashResourceWindow planDashAvailableWindow(const DashManifest&,const DashTemplateAddressing&,
 i64 windowStart,i64 windowEnd,i64 nowUs,const Cancel&);
struct DashClockSample {
 i64 serverUs=0,uncertaintyUs=0;
 std::chrono::steady_clock::time_point anchor;
 std::string scheme;
};
using DashMetadataAccounting=std::function<void(size_t,const Cancel&)>;
// manifestReceived must identify receipt of this fresh MPD, for direct timing.
DashClockSample synchronizeDashClock(const DashManifest&,const std::string& manifestUrl,
 const Json& prefs,std::chrono::steady_clock::time_point manifestReceived,const std::shared_ptr<Cancel>&,
 bool allowDirect=true,const DashMetadataAccounting& onBytes={});
i64 dashClockNow(const DashClockSample&,std::chrono::steady_clock::time_point now);
struct DashManifestSnapshot {
 DashManifest manifest;
 std::string requestUrl,url,document,etag,modified;
 std::chrono::steady_clock::time_point bodyReceived,checked;
 std::optional<std::chrono::steady_clock::time_point> validUntil;
 i64 responseAgeSeconds=0;
};
// Returns a new snapshot only after a complete successful fetch/revalidation.
// Callers retain their previous snapshot on failure.
DashManifestSnapshot refreshDashManifest(const std::string&,const Headers&,const Json&,
 const std::shared_ptr<Cancel>&,const DashManifestSnapshot* previous=nullptr,
 const DashMetadataAccounting& onBytes={});
// Browser catalog selection uses a freshly fetched MPD and synchronized server
// time. No local wall-clock guess or implicit browser/site credentials.
Json selectDashLivePeriod(const std::string&,const Headers&,const Json&,const std::shared_ptr<Cancel>&);
struct DashSelectedTrack {
 std::string presentation;
 std::string periodId,adaptationId,representationId;
 Json format;
 DashTemplateAddressing addressing;
};
DashSelectedTrack selectDashTrack(const DashManifestSnapshot&,size_t representation,const Cancel&);
// Rematches within the same Period by IDs, never XML ordinal or nearest quality.
DashSelectedTrack rematchDashTrack(const DashManifestSnapshot&,const DashSelectedTrack&,const Cancel&);
struct DashLiveState {
 DashManifestSnapshot snapshot;
 std::vector<DashSelectedTrack> tracks;
 std::optional<DashClockSample> clock;
};
struct DashLiveUpdate {
 DashLiveState state;
 std::vector<DashResourceWindow> windows;
 bool staticPresentation=false;
};
struct DashPeriodTransition {DashLiveState state;double previousEnd=0;};
// Plans only an adjacent nonempty Period with compatible selected track metadata.
// The caller must finish/seal the previous Period and own durable session joining.
std::optional<DashPeriodTransition> planDashPeriodTransition(const DashLiveState&,
 const DashManifestSnapshot& refreshed,const Cancel&);
DashLiveState createDashLiveState(const DashManifestSnapshot&,const std::vector<size_t>&,const Cancel&);
// Windows are per-track media ticks. The caller commits returned state only on
// success and owns polling, segment receipts, Period transitions and finalization.
DashLiveUpdate updateDashLivePlan(const DashLiveState&,const std::vector<std::pair<i64,i64>>&,
 const Headers&,const Json&,const std::shared_ptr<Cancel>&,const DashMetadataAccounting& onBytes={});
struct DashStoredResource {fs::path path;i64 bytes=0;std::string sha256;bool reused=false;};
// Explicit credentials per canonical HTTP(S) origin. Missing origins are anonymous;
// manifest headers must never implicitly become CDN credentials.
using DashResourceHeaders=std::map<std::string,Headers>;
Json dashSegmentBinding(const DashSelectedTrack&,const DashSegmentReference&);
// Range coordinates are part of the protected identity, never only a request
// header. Whole-resource bindings keep their existing receipt keys.
Json dashRangeBinding(const Json& identity,i64 start,i64 length);
// A strong validator is scoped to its exact resource URL, not interchangeable
// CDN alternatives. The version becomes part of the protected receipt identity.
Json dashVersionBinding(const Json& identity,const std::string& resourceUrl,i64 total,const std::string& etag);
// Bounded metadata discovery; no media body is downloaded. Returns url/total/etag.
Json discoverDashResourceVersion(const std::vector<std::string>&,const DashResourceHeaders&,const Json&,const std::shared_ptr<Cancel>&);
std::optional<DashStoredResource> inspectDashResource(const fs::path&,const Json&,const Cancel&);
// Offline reconciliation: promotes verified receipts and clears stale completion
// claims without deleting media. Returns the protected index's decoded content.
Json reconcileDashRecording(const fs::path&,const Cancel&);
// Freeze the complete common interval after Stop. Excluded tail records and all
// resource files are retained; an explicit requested end must be fully covered.
Json sealDashRecording(const fs::path&,const Cancel&);
// The caller owns rate/quota accounting through onBytes. Receipts bind the
// caller's media identity and verified file bytes, not expiring URL tokens.
// A dashRangeBinding requires an exact single-range HTTP 206 response.
DashStoredResource storeDashResource(const fs::path& directory,const Json& binding,
 const std::vector<std::string>& urls,const DashResourceHeaders&,const Json& prefs,
 const std::shared_ptr<Cancel>&,const std::function<void(size_t)>& onBytes={});
// Retrieves a bounded index tree through the version-bound resource store.
std::vector<DashIndexedMedia> loadDashIndexes(const fs::path& directory,const Json& identity,
 const Json& version,const std::vector<std::string>& urls,i64 start,i64 length,
 const DashResourceHeaders&,const Json& prefs,const std::shared_ptr<Cancel>&,
 std::optional<i64> externalMediaSize={},const DashMetadataAccounting& onBytes={});
// Expand indexed addressing into version-bound list references. The caller owns
// integrating the resulting clock/identity into its capture lifecycle.
DashTemplateAddressing materializeDashIndex(const fs::path&,const Json& identity,
 const DashTemplateAddressing&,const DashResourceHeaders&,const Json& prefs,
 const std::shared_ptr<Cancel>&,const DashMetadataAccounting& onBytes={});
void checkpointDashIndex(const fs::path&,const Json& identity,const DashTemplateAddressing&,const Cancel&);
std::string dashIndexSourceDigest(const DashTemplateAddressing&);
std::optional<DashTemplateAddressing> restoreDashIndex(const fs::path&,const Json& identity,
 const DashTemplateAddressing& source,const Cancel&);
// Pending work is checkpointed before requests. Track workers have private
// drafts and serialize index writes. onBytes(track,bytes,cancel) can be concurrent;
// use its cancellation token for rate waits. Resources remain ordered per track.
struct DashCaptureInterval {double start=0;std::optional<double> end;};
Json captureDashWindow(const fs::path&,const DashLiveUpdate&,const DashResourceHeaders&,const Json&,
 const std::shared_ptr<Cancel>&,const std::function<void(size_t,size_t,const Cancel&)>& onBytes={},
 std::optional<DashCaptureInterval> interval={},size_t concurrency=1);
struct DashRecordingOutput {
 fs::path staging;
 std::vector<fs::path> temporaryInputs;
 i64 bytes=0;std::string sha256;double start=0,duration=0;bool reused=false;
};
// Produces a verified local staging file; sealed recordings can reuse an atomic
// output receipt after binding/size/hash checks. The job owns publication/cleanup.
DashRecordingOutput assembleDashRecording(const fs::path&,const Cancel&);
// Joins ordered, sealed Period folders ("." for a legacy first Period, otherwise
// period-NNNN). Requires adjacent clocks and compatible streams. Returned start
// is presentation-relative. Session capture/advancement and publication are caller-owned.
DashRecordingOutput assembleDashRecordingSession(const fs::path&,const std::vector<std::string>&,
 const Json& selection,const Cancel&);
}
