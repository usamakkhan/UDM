from pathlib import Path
base=Path('<development>')
s=(base/'media-handoff-workflows-9.75.cpp').read_text(encoding='utf-8-sig');old='mediaChecks(manager,root);manager.stop();Cancel cancel;';assert s.count(old)==1
new='''mediaChecks(manager,root);manager.stop();Cancel cancel;
{auto source=root/L"generated-video.mp4";auto pin=PublicationSourcePin::openReadWrite(source);check(bool(pin),"DASH write-capable source ownership opens the real video");const auto digest=pin->hash();
execute(appDir()/L"tools/ffprobe.exe",{L"-v",L"error",L"-show_streams",L"-of",L"json",source.wstring()},30,cancel);check(true,"FFprobe can read a video while its owned write-capable pin excludes other writers");
execute(appDir()/L"tools/ffmpeg.exe",{L"-hide_banner",L"-loglevel",L"error",L"-xerror",L"-nostdin",L"-i",source.wstring(),L"-f",L"null",L"-"},30,cancel);check(true,"Write-capable pinned video decodes through EOF without errors");
Manager copied(root/L"owned-rw-copy-catalog");auto prefs=copied.state["Settings"];prefs["DownloadFolder"]=utf8((fs::path(wide(std::getenv("UDM_MEDIA_CROSS_VOLUME_TEST_ROOT")))/L"rw-hashed-copy").wstring());prefs["CategoryFolders"]=false;copied.setSettings(prefs);auto job=copied.add("https://example.invalid/live-dash.mp4","","copied.mp4","Main queue",true);auto cloned=pin->clone();pin.reset();publishMediaOutput(copied,job,source,digest,std::make_shared<Cancel>(),nullptr,std::move(cloned));check(str(job->data,"Status")=="Complete"&&fileHash(job->target())==digest,"Cross-volume real video copy starts at zero after the source hash reached EOF");copied.stop();}
'''
s=s.replace(old,new);(base/'media-handoff-workflows-9.76.cpp').write_text(s,encoding='utf-8')
s=(base/'run-media-handoff-workflows-9.75.py').read_text(encoding='utf-8-sig').replace('9.75','9.76').replace('owned-verified-handoff-build-9.76','owned-dash-output-build-9.76');p=base/'run-media-handoff-workflows-9.76.py';assert not p.exists();p.write_text(s,encoding='utf-8')
