#pragma once
#include "ProtocolFallback.hpp"
static void protocolFallbackChecks(const udm::fs::path& root,Fixture& fixture){
 using namespace udm;
 const InternetFailure transport("HTTP read",ERROR_WINHTTP_CONNECTION_ERROR);const CurlBodyDisconnect body("HTTP response body was interrupted.");const CurlResponseDisconnect headersLost("HTTP connection closed before response headers.");
 const std::runtime_error other("Partial file exceeds its assigned range.");const HttpRejected rejected(503);
 check(shouldDisableHttp2("HTTP/2",false,transport)&&shouldDisableHttp2("HTTP/2",false,body)&&shouldDisableHttp2("HTTP/2",false,headersLost),"Transport failures during an HTTP/2 exchange fall back to HTTP/1.1");
 check(!shouldDisableHttp2("HTTP/2",false,other)&&!shouldDisableHttp2("HTTP/2",false,rejected),"Local and server-reply errors do not change the protocol");
 check(!shouldDisableHttp2("HTTP/1.1",false,transport)&&!shouldDisableHttp2("",false,transport),"Failures without a negotiated HTTP/2 exchange keep the protocol");
 check(!shouldDisableHttp2("HTTP/2",true,transport),"The fallback is applied once per download");
 check(shouldDisableHttp3("HTTP/3",false,transport)&&shouldDisableHttp3("HTTP/3",false,body),"Transport failures during an HTTP/3 exchange fall back to HTTP/2");
 check(!shouldDisableHttp3("HTTP/2",false,transport)&&!shouldDisableHttp3("HTTP/3",true,transport)&&!shouldDisableHttp3("HTTP/3",false,rejected),"HTTP/3 fallback applies once, only to HTTP/3 exchanges and only for transport failures");
 check(!shouldDisableHttp2("HTTP/3",false,transport),"An HTTP/3 failure steps down one protocol level rather than straight to HTTP/1.1");
 Manager m(root/L"protocol-fallback");auto settings=m.state["Settings"];settings["DownloadFolder"]=utf8((root/L"protocol-fallback-files").wstring());settings["CategoryFolders"]=false;settings["PrefetchFileInfo"]=false;settings["ProxyMode"]="Connect directly";settings["Connections"]=3;m.setSettings(settings);
 auto job=m.add(fixture.url("/fallback"),"","fallback.bin","Main queue",true);job->data["DisableHttp2"]=true;m.resume(job);
 for(int i=0;i<1500;++i){m.tick();if(!m.isActive(job)&&str(job->data,"Status")=="Complete")break;Sleep(10);}
 check(str(job->data,"Status")=="Complete"&&fileHash(job->target())==fileHash([&]{auto p=root/L"fallback-expected.bin";writeBytes(p,Bytes(fixture.payload.begin(),fixture.payload.end()));return p;}()),"A download pinned to HTTP/1.1 completes with the full payload");
 auto noHttp3=m.add(fixture.url("/fallback"),"","fallback-h2.bin","Main queue",true);noHttp3->data["DisableHttp3"]=true;m.resume(noHttp3);
 for(int i=0;i<1500;++i){m.tick();if(!m.isActive(noHttp3)&&str(noHttp3->data,"Status")=="Complete")break;Sleep(10);}
 check(str(noHttp3->data,"Status")=="Complete"&&fileHash(noHttp3->target())==fileHash(job->target()),"A download pinned below HTTP/3 completes with the full payload");
 settings["UseHttp3"]=false;m.setSettings(settings);auto off=m.add(fixture.url("/fallback"),"","fallback-off.bin","Main queue",true);m.resume(off);
 for(int i=0;i<1500;++i){m.tick();if(!m.isActive(off)&&str(off->data,"Status")=="Complete")break;Sleep(10);}
 check(str(off->data,"Status")=="Complete"&&fileHash(off->target())==fileHash(job->target()),"Sessions created with HTTP/3 turned off still download");
}
