# Native DASH SegmentList planning

Build completed on 2026-10-29: all five native production targets and the UDM 1.1.0 / extension 0.63.64 installer compiled successfully. The interrupted October 28 log is retained. Evidence: validation/native-dash-list-planning-build-20261029.json. The candidate is unsigned, staged, uninstalled and unpublished.

The native addressing resolver now reads inherited SegmentList attributes, initialization resources and SegmentURL children. A descendant list can replace its inherited resource entries, initialization or timeline. Mixed inherited template/list types, duplicate addressing elements, external XLink lists and per-segment indexes remain explicitly unsupported. SegmentBase still requires separate native index parsing.

List expansion maps every URL to a numbered timeline reference and requires exact agreement between resource count, repeats and any endNumber. Finite repeats and a final open repeat are bounded by the list length; internal open repeats require an explicit, exactly aligned next timestamp. Overflow, overlap and unmatched resources are rejected. Static duration-only lists are planned from their declared duration. Live lists require an explicit timeline and first timestamp so sliding windows cannot silently reset media time.

MediaRange and initialization range values become the range-bearing descriptors added in the prior recording batch. URI references are resolved literally against inherited BaseURL alternatives, including dollar characters; template substitution applies only to templates. Window filtering, live availability offsets, time-shift limits, complete-resource requirements and URL-memory bounds use the existing native planner. Invalid URI references and oversized byte ranges are rejected.

This is a planner milestone. Both the native selected-track admission and browser catalog still reject live list recording. The native gate also prevents an existing template recording from silently switching to a newly list-addressed representation during refresh or Period advancement. Shared resource-version validation must be completed before this gate is removed; index-addressed files additionally need native index parsing. This change does not claim end-to-end SegmentList or SegmentBase support.

Runtime planner, network, restart and playback tests remain deferred at user request. Extension 0.63.64 is unchanged. The candidate remains staged only, without installation or publication.
