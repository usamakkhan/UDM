#pragma once
#include "HlsPlaylist.hpp"
static void hlsPlaylistChecks(){
 const std::string base="https://media.example.test/live/index.m3u8?token=private";
 auto parse=[&](const std::string& body){return parseHlsPlaylist(body,base);};
 auto window=[](i64 first,int count,bool end=false){std::string text="#EXTM3U\n#EXT-X-TARGETDURATION:4\n#EXT-X-MEDIA-SEQUENCE:"+std::to_string(first)+"\n";for(int index=0;index<count;++index)text+="#EXTINF:4,\nsegment-"+std::to_string(first+index)+".ts\n";return text+(end?"#EXT-X-ENDLIST\n":"");};
 auto live=parse(window(20,3));
 check(!live.ended&&live.firstSequence==20&&live.targetDuration==4&&live.segments.size()==3&&live.segments[2].sequence==22,"Live HLS keeps media sequence and target duration");
 check(live.segments[0].media.url=="https://media.example.test/live/segment-20.ts","Live HLS resolves resource addresses against the playlist");
 auto crlf=parse("#EXTM3U\r\n#EXT-X-TARGETDURATION:2\r\n#EXTINF:1.75,title\r\na.ts\r\n#EXT-X-ENDLIST\r\n");
 check(crlf.ended&&crlf.firstSequence==0&&crlf.segments[0].duration==1.75,"Recorded HLS retains decimal duration and accepts CRLF");
 auto rounded=parse("#EXTM3U\n#EXT-X-TARGETDURATION:2\n#EXTINF:2.49,\na.ts\n");
 check(rounded.segments.size()==1,"HLS target-duration validation uses rounded segment durations");
 auto empty=parse("#EXTM3U\n#EXT-X-TARGETDURATION:4\n#EXT-X-MEDIA-SEQUENCE:20\n");
 check(empty.segments.empty()&&!empty.ended,"HLS can wait on an empty starting live window");
 auto range=parse("#EXTM3U\n#EXT-X-TARGETDURATION:4\n#EXT-X-MAP:URI=\"../init.mp4?x=1,2\",BYTERANGE=\"100@0\"\n#EXT-X-BYTERANGE:20@100\n#EXTINF:4,\nmedia.mp4\n#EXTINF:4,\n#EXT-X-BYTERANGE:30\nmedia.mp4\n");
 check(range.segments[0].initialization&&range.segments[0].initialization->url=="https://media.example.test/init.mp4?x=1,2"&&range.segments[0].initialization->range->length==100&&range.segments[1].media.range->start==120&&range.segments[1].media.range->length==30,"Live HLS preserves initialization and implicit same-resource byte ranges");
 auto clocks=parse("#EXTM3U\n#EXT-X-TARGETDURATION:4\n#EXT-X-MEDIA-SEQUENCE:100\n#EXT-X-DISCONTINUITY-SEQUENCE:7\n#EXT-X-PROGRAM-DATE-TIME:2026-09-28T10:00:00.123456Z\n#EXTINF:3.5,\na.ts\n#EXTINF:4,\nb.ts\n#EXT-X-DISCONTINUITY\n#EXTINF:4,\nc.ts\n#EXT-X-PROGRAM-DATE-TIME:2026-09-28T12:00:11.623456+02:00\n#EXTINF:4,\nd.ts\n");
 check(clocks.segments[0].discontinuity==7&&clocks.segments[2].discontinuity==8&&!clocks.segments[2].programTimeUs&&*clocks.segments[1].programTimeUs-*clocks.segments[0].programTimeUs==3500000&&*clocks.segments[3].programTimeUs-*clocks.segments[0].programTimeUs==11500000,"Live HLS tracks discontinuity clocks without inventing a clock across a reset");
 check(hlsProgramTime("2026-09-28T10:00:00.123456789Z")==hlsProgramTime("2026-09-28T12:00:00.123456+02:00"),"HLS program times normalize offsets and submicrosecond precision");
 auto clockBefore=parse("#EXTM3U\n#EXT-X-TARGETDURATION:4\n#EXT-X-PROGRAM-DATE-TIME:2026-09-28T10:00:00Z\n#EXT-X-DISCONTINUITY\n#EXTINF:4,\na.ts\n");
 check(clockBefore.segments[0].programTimeUs.has_value(),"Explicit HLS program time applies regardless of discontinuity-tag order");
 auto ll=parse("#EXTM3U\n#EXT-X-TARGETDURATION:4\n#EXT-X-PART-INF:PART-TARGET=0.5\n#EXT-X-PART:DURATION=0.5,URI=\"part0.mp4\"\n#EXTINF:4,\nfull.mp4\n#EXT-X-PRELOAD-HINT:TYPE=PART,URI=\"next.mp4\"\n");
 check(ll.segments.size()==1&&ll.segments[0].sequence==0&&ll.segments[0].media.url.find("full.mp4")!=std::string::npos,"Low-latency HLS records complete segments without double-counting partial segments");
 auto gap=parse("#EXTM3U\n#EXT-X-TARGETDURATION:4\n#EXT-X-GAP\n#EXTINF:4,\na.ts\n#EXTINF:4,\nb.ts\n");
 check(gap.segments[0].gap&&!gap.segments[1].gap,"HLS gap markers remain explicit and apply to one segment");
 auto maps=parse("#EXTM3U\n#EXT-X-TARGETDURATION:4\n#EXT-X-MAP:URI=\"init1.mp4\"\n#EXTINF:4,\na.m4s\n#EXT-X-DISCONTINUITY\n#EXT-X-MAP:URI=\"init2.mp4\"\n#EXTINF:4,\nb.m4s\n");
 check(maps.segments[0].initialization->url!=maps.segments[1].initialization->url&&maps.segments[1].discontinuity==1,"HLS retains initialization changes at discontinuities");
 auto clear=parse("#EXTM3U\n#EXT-X-TARGETDURATION:4\n#EXT-X-KEY:METHOD=NONE\n#EXTINF:4,\na.ts\n");
 check(clear.segments.size()==1,"HLS accepts explicitly unencrypted segments");
 const std::string header="#EXTM3U\n#EXT-X-TARGETDURATION:4\n";
 for(const auto& test:std::vector<std::pair<std::string,std::string>>{
  {"#EXT-X-KEY:METHOD=AES-128,URI=\"key.bin\"\n#EXTINF:4,\na.ts\n","HLS rejects AES-encrypted media"},
  {"#EXT-X-KEY:METHOD=SAMPLE-AES,URI=\"key.bin\"\n#EXTINF:4,\na.ts\n","HLS rejects sample-encrypted media"},
  {"#EXT-X-SESSION-KEY:METHOD=AES-128,URI=\"key.bin\"\n","HLS rejects encrypted session keys"},
  {"#EXT-X-SKIP:SKIPPED-SEGMENTS=2\n","HLS refuses incomplete delta playlists"},
  {"#EXT-X-DEFINE:NAME=\"x\",VALUE=\"y\"\n","HLS refuses unsupported variable expansion"},
  {"#EXT-X-I-FRAMES-ONLY\n","HLS refuses trick-play playlists"},
  {"#EXT-X-STREAM-INF:BANDWIDTH=1000\nquality.m3u8\n","HLS recording requires a selected media playlist"},
  {"#EXT-X-TARGETDURATION:4\n","HLS rejects duplicate target duration"},
  {"#EXTINF:4,\na.ts\n#EXT-X-MEDIA-SEQUENCE:2\n","HLS rejects late media sequence declarations"},
  {"#EXTINF:4,\na.ts\n#EXT-X-DISCONTINUITY-SEQUENCE:2\n","HLS rejects late discontinuity sequence declarations"},
  {"#EXTINF:4,\n","HLS rejects a truncated segment declaration"},
  {"#EXT-X-ENDLIST\n#EXTINF:4,\na.ts\n","HLS rejects media after ENDLIST"},
  {"#EXT-X-PLAYLIST-TYPE:VOD\n#EXTINF:4,\na.ts\n","HLS rejects VOD without ENDLIST"},
  {"#EXTINF:4,\nfile:///secret.ts\n","HLS refuses local resource addresses"},
  {"#EXTINF:4,\n{$variable}.ts\n","HLS refuses unresolved URI variables"},
  {"#EXT-X-BYTERANGE:20\n#EXTINF:4,\nmedia.mp4\n","HLS rejects an ambiguous initial byte range"},
  {"#EXT-X-BYTERANGE:20@10\n#EXTINF:4,\na.mp4\n#EXT-X-BYTERANGE:20\n#EXTINF:4,\nb.mp4\n","HLS does not borrow byte offsets from another resource"},
  {"#EXT-X-MAP:URI=\"init.mp4\",BYTERANGE=\"20\"\n#EXTINF:4,\na.m4s\n","HLS initialization ranges require explicit offsets"}})
  rejects([&]{parse(header+test.first);},test.second.c_str());
 rejects([&]{parse("#EXTM3U\n#EXT-X-TARGETDURATION:2\n#EXTINF:2.5,\na.ts\n");},"HLS rejects oversized rounded segment duration");
 rejects([&]{parse("#EXTM3U\n#EXTINF:4,\na.ts\n");},"Live HLS requires a target duration");
 rejects([&]{parse(header+char(0)+"\n");},"HLS rejects embedded control bytes");
 rejects([&]{parse(std::string("#EXTM3U\n#comment ")+char(-1)+"\n");},"HLS rejects malformed UTF-8");
 rejects([&]{parse("#EXTM3U\n#"+std::string(2*1024*1024,'a'));},"HLS bounds playlist response size");
 rejects([&]{hlsRange("268435457@0",{},base);},"HLS bounds segment byte-range sizes");
 rejects([&]{hlsRange("10@9007199254740990",{},base);},"HLS rejects overflowing byte ranges");
 rejects([&]{hlsProgramTime("2026-02-30T00:00:00Z");},"HLS rejects invalid calendar dates");
 rejects([&]{hlsProgramTime("2026-09-28T00:00:00+24:00");},"HLS rejects invalid timezone offsets");
 rejects([&]{hlsAttributes("URI=\"a\",URI=\"b\"");},"HLS rejects repeated attributes");
 rejects([&]{hlsAttributes("URI=\"unfinished");},"HLS rejects unterminated quoted attributes");
 HlsTimeline timeline;check(timeline.append(live)==3&&timeline.append(live)==0&&timeline.segments.size()==3,"Live recording refreshes do not duplicate previously seen segments");
 check(timeline.append(parse(window(21,4)))==2&&timeline.segments.front().sequence==20&&timeline.segments.back().sequence==24,"Live recording appends rolling windows and retains the captured prefix");
 check(timeline.append(live)==0&&timeline.segments.size()==5,"Live recording ignores a stale consistent CDN snapshot");
 auto changed=parse(window(22,4));changed.segments[0].media.url="https://media.example.test/other.ts";
 rejects([&]{timeline.append(changed);},"Live recording rejects rewritten segment identity");
 check(timeline.segments.size()==5&&!timeline.ended,"A rejected HLS refresh leaves the recording intact");
 auto discontinuityChanged=parse(window(23,3));discontinuityChanged.segments[0].discontinuity=1;
 rejects([&]{timeline.append(discontinuityChanged);},"Live recording rejects rewritten discontinuity history");
 auto durationChanged=parse(window(23,3));durationChanged.segments[0].duration=3;
 rejects([&]{timeline.append(durationChanged);},"Live recording rejects rewritten segment duration");
 rejects([&]{timeline.append(parse(window(27,3)));},"Live recording detects expired media instead of silently joining a gap");
 auto targetChanged=parse(window(23,4));targetChanged.targetDuration=5;
 rejects([&]{timeline.append(targetChanged);},"Live recording rejects target-duration changes");
 auto identityChanged=parseHlsPlaylist(window(23,4),"https://different.example.test/live.m3u8");
 rejects([&]{timeline.append(identityChanged);},"Live recording cannot switch playlist identity");
 check(timeline.append(parse(window(23,4,true)))==2&&timeline.ended&&timeline.segments.size()==7,"Live recording accepts the final complete media and ENDLIST");
 check(timeline.append(parse(window(23,4,true)))==0,"Completed HLS timeline tolerates an identical final snapshot");
 rejects([&]{timeline.append(parse(window(24,4,true)));},"An ended recording cannot acquire later media");
 HlsTimeline event;auto eventFirst=parse(header+"#EXT-X-PLAYLIST-TYPE:EVENT\n#EXTINF:4,\na.ts\n");event.append(eventFirst);
 auto eventNext=eventFirst;eventNext.firstSequence=1;eventNext.segments[0].sequence=1;eventNext.segments[0].media.url="https://media.example.test/live/b.ts";
 rejects([&]{event.append(eventNext);},"An HLS event recording rejects removal of its advertised history");
 HlsTimeline starting;starting.append(empty);check(starting.append(live)==3,"An empty starting live playlist records later media");
 HlsTimeline fmp4;fmp4.append(maps);auto changedMap=maps;changedMap.segments[0].initialization->url="https://media.example.test/new-init.mp4";
 rejects([&]{fmp4.append(changedMap);},"Live recording rejects initialization changes for already associated media");
}
