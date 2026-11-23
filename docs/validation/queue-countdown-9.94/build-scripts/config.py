from pathlib import Path
import os,hashlib,json
root=Path('<repo>');native=root/'native';area=Path(__file__).parent;cache=root/'.media-cache/driver-toolchain';vc=Path('Contents/VC/Tools/MSVC/14.44.35207');bin=cache/'Microsoft.VC.14.44.17.14.Tools.HostX64.TargetX64.base'/vc/'bin/Hostx64/x64';compiler=bin/'cl.exe';assert compiler.exists()
sdk=cache/'microsoft.windows.sdk.cpp/c';sdklib=cache/'microsoft.windows.sdk.cpp.x64/c'
includes=[cache/'Microsoft.VC.14.44.17.14.CRT.Headers.base'/vc/'include',cache/'Microsoft.VC.14.44.17.14.ATL.Headers.base'/vc/'atlmfc/include',cache/'Microsoft.VC.14.44.17.14.MFC.Headers.base'/vc/'atlmfc/include']+[sdk/'Include/10.0.26100.0'/name for name in ['ucrt','um','shared']]
libs=[cache/'Microsoft.VC.14.44.17.14.CRT.x64.Desktop.base'/vc/'lib/x64',cache/'Microsoft.VC.14.44.17.14.CRT.x64.Store.base'/vc/'lib/x64',cache/'Microsoft.VC.14.44.17.14.ATL.X64.base'/vc/'atlmfc/lib/x64',cache/'Microsoft.VC.14.44.17.14.MFC.X64.base'/vc/'atlmfc/lib/x64',sdklib/'ucrt/x64',sdklib/'um/x64']
for p in includes+libs:assert p.is_dir(),p
env=os.environ.copy();env.update(INCLUDE=';'.join(map(str,includes)),LIB=';'.join(map(str,libs)),PATH=str(bin)+';'+env['PATH'],VSCMD_SKIP_SENDTELEMETRY='1',TEMP=str(area),TMP=str(area))
core=Path(os.environ['LOCALAPPDATA'])/'UDM-Development/native-work/out-nosymbols';actual=core;work=area;resource=core/'App.res';out=area/'objects';release=area/'release-native';productRelease=core.parent/'release-native'
for p in [out,release,productRelease]:p.mkdir(exist_ok=True)
baselineConfig=json.loads((root/'docs/validation/finishing-8.18.json').read_text());objects=[core/x['name']for x in baselineConfig['coreObjects']];core_names=[p.stem for p in objects if(native/(p.stem+'.cpp')).is_file()];assert len(objects)==42
common=['/nologo','/bigobj','/std:c++17','/EHsc','/MT','/O2','/W4','/utf-8','/permissive-','/DUNICODE','/D_UNICODE','/DNOMINMAX','/D_WIN32_WINNT=0x0A00','/DWINVER=0x0A00','/D_CRT_SECURE_NO_WARNINGS','/Zc:__cplusplus','/DCURL_STATICLIB','/I'+str(native/'vendor/curl/include'),'/I'+str(core)]
system=[str(native/'vendor/curl/lib/libcurl.lib'),str(native/'vendor/nghttp2/lib/nghttp2_static.lib'),'secur32.lib','normaliz.lib','winhttp.lib','wininet.lib','crypt32.lib','bcrypt.lib','shell32.lib','shlwapi.lib','xmllite.lib','ole32.lib','oleaut32.lib','advapi32.lib','user32.lib','gdi32.lib','comctl32.lib','ws2_32.lib','iphlpapi.lib','uuid.lib','winmm.lib','uxtheme.lib','powrprof.lib','rasapi32.lib','rasdlg.lib']
def sha(p):
 with p.open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest().upper()
