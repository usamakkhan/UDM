#pragma once
#include "WebVtt.hpp"
template<class Check> void webVttClockChecks(Check check){
 auto verify=[&](const std::string& header,const std::string& times,double media,double timeline,bool hls,udm::i64 start,udm::i64 end,const char* name){std::vector<udm::SubtitleCue> cues;udm::appendWebVtt(cues,"WEBVTT\n"+header+"\n"+times+"\nCaption\n\n",media,timeline,hls);check(cues.size()==1&&cues[0].start==start&&cues[0].end==end,name);};
 verify("X-TIMESTAMP-MAP=LOCAL:00:00:00.000,MPEGTS:0\n","26:23:21.000 --> 26:23:22.000",95000,0,true,1000,2000,"Distant zero anchor does not add a spurious MPEGTS epoch");
 verify("X-TIMESTAMP-MAP=LOCAL:00:00:00.000,MPEGTS:0\n","52:46:41.000 --> 52:46:42.000",190000,0,true,1000,2000,"Unwrapped cues preserve synchronization across multiple MPEGTS epochs");
 verify("X-TIMESTAMP-MAP=LOCAL:26:23:20.000,MPEGTS:0\n","52:46:41.000 --> 52:46:42.000",95000,0,true,1000,2000,"Distant nonzero LOCAL anchor is included before epoch selection");
 verify("X-TIMESTAMP-MAP=MPEGTS:45000,LOCAL:00:00:00.000\n","00:00:00.000 --> 00:00:01.000",((1LL<<33)-90000)/90000.0,2,true,1500,2500,"Wrapped PES with local cue clock retains existing behavior");
 verify("","26:30:44.718 --> 26:30:45.718",0,0,true,1000,2000,"Implicit zero timestamp map handles wrapped PES and unwrapped cues");
 verify("","26:30:44.718 --> 26:30:45.718",0,0,false,95444718,95445718,"Non-HLS subtitles retain their absolute cue clock");
 verify("X-TIMESTAMP-MAP=LOCAL:00:00:00.000,MPEGTS:900000\n","00:00:01.000 --> 00:00:03.000",10,0,true,1000,3000,"Ordinary timestamp maps remain aligned");
 verify("X-TIMESTAMP-MAP=LOCAL:00:00:00.000,MPEGTS:0\n","26:23:19.500 --> 26:23:21.000",95000,0,true,0,1000,"Boundary-spanning cue preserves duration and clips negative start");
 verify("X-TIMESTAMP-MAP=LOCAL:00:00:00.000,MPEGTS:0\n","00:00:00.000 --> 24:00:00.000",0,72000,true,0,86400000,"Long cue spanning the segment is not moved into another epoch");
}