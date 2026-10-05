#include "ServerConnections.hpp"
#include "BrowserProxy.hpp"
#include "QueueWake.hpp"
#include "Scanner.hpp"
#include "Core.hpp"
#include "QueueStop.hpp"
#include "QueueRetry.hpp"
#include "DefaultQueues.hpp"
#include "QueueMembership.hpp"
#include "CapturePresentation.hpp"
#include "DirectMediaRefresh.hpp"
#include "BrowserRequest.hpp"
#include "ProxyPolicy.hpp"
#include "SocksProxy.hpp"
#include "SiteLogins.hpp"
#include "DialUp.hpp"
#include "OfflineSite.hpp"
#include "BrowserSettings.hpp"
#include "OptionsModel.hpp"
#include "ToolbarModel.hpp"
#include "GrabberDestinations.hpp"
#include "GrabberProject.hpp"
#include "PacProxy.hpp"
#include "GuiModels.hpp"
#include <wincrypt.h>
#include <bcrypt.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <regex>
#include <algorithm>
#include <cwctype>
#include <limits>
namespace udm {
std::wstring wide(const std::string& s){if(s.empty())return {};int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),(int)s.size(),nullptr,0);if(!n)throw std::runtime_error("Invalid UTF-8 text.");std::wstring r(n,0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),(int)s.size(),r.data(),n);return r;}
std::string utf8(const std::wstring& s){if(s.empty())return {};int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s.data(),(int)s.size(),nullptr,0,nullptr,nullptr);if(!n)throw std::runtime_error("Invalid Unicode text.");std::string r(n,0);WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s.data(),(int)s.size(),r.data(),n,nullptr,nullptr);return r;}
std::string lower(std::string s){for(char& c:s)c=(char)tolower((unsigned char)c);return s;}
std::string trim(std::string s){size_t a=s.find_first_not_of(" \t\r\n"),b=s.find_last_not_of(" \t\r\n");return a==std::string::npos?"":s.substr(a,b-a+1);}
std::string str(const Json& j,const char* k,std::string d){return j.contains(k)&&j[k].is_string()?j[k].get<std::string>():d;}
i64 num(const Json& j,const char* k,i64 d){return j.contains(k)&&j[k].is_number_integer()?j[k].get<i64>():d;}
double real(const Json& j,const char* k,double d){return j.contains(k)&&j[k].is_number()?j[k].get<double>():d;}
bool yes(const Json& j,const char* k,bool d){return j.contains(k)&&j[k].is_boolean()?j[k].get<bool>():d;}
std::string guid(){GUID g{};if(FAILED(CoCreateGuid(&g)))throw std::runtime_error("Cannot create a download identifier.");wchar_t b[40];StringFromGUID2(g,b,40);std::string s=lower(utf8(b));s.erase(std::remove_if(s.begin(),s.end(),[](char c){return c=='{'||c=='}'||c=='-';}),s.end());return s;}
std::string safeName(std::string s){auto w=wide(s);auto slash=w.find_last_of(L"/\\");if(slash!=std::wstring::npos)w=w.substr(slash+1);for(auto& c:w)if(c<32||wcschr(L"<>:\"/\\|?*",c))c=L'_';while(!w.empty()&&(w.back()==L'.'||iswspace(w.back())))w.pop_back();while(!w.empty()&&iswspace(w.front()))w.erase(w.begin());if(w.empty())w=L"download.bin";if(std::regex_search(lower(utf8(w)),std::regex("^(con|prn|aux|nul|com[1-9]|lpt[1-9])([.]|$)")))w=L"_"+w;if(w.size()>160){auto ext=fs::path(w).extension().wstring();w=w.substr(0,140)+ext.substr(0,19);if(!w.empty()&&w.back()>=0xD800&&w.back()<=0xDBFF)w.pop_back();}return utf8(w);}
std::vector<std::string> words(const std::string& v){std::vector<std::string> out;std::regex re("[^\\s,;]+");for(auto i=std::sregex_iterator(v.begin(),v.end(),re);i!=std::sregex_iterator();++i){auto s=lower(i->str());if(std::find(out.begin(),out.end(),s)==out.end())out.push_back(s);}return out;}
std::string category(const std::string& f){auto ext=lower(utf8(fs::path(wide(f)).extension().wstring()));const std::pair<const char*,const char*> rules[]={{"Archives",".zip .7z .rar .gz .tar .iso"},{"Video",".mp4 .mkv .webm .mov .avi .ts"},{"Music",".mp3 .flac .wav .ogg .m4a .aac"},{"Programs",".exe .msi .msix .apk"},{"Images",".jpg .jpeg .png .svg .webp .gif"},{"Documents",".pdf .doc .docx .xlsx .txt .epub .pptx .csv"}};for(auto r:rules)for(auto x:words(r.second))if(x==ext)return r.first;return "Other";}
std::string bytes(double n){if(n<0)return "Unknown";const char* u[]={"B","KB","MB","GB","TB"};int i=0;while(n>=1024&&i<4){n/=1024;++i;}std::ostringstream s;s<<std::fixed<<std::setprecision(i?1:0)<<n<<' '<<u[i];return s.str();}
std::string b64(const Bytes& b){if(b.empty())return {};DWORD n=0;CryptBinaryToStringA(b.data(),(DWORD)b.size(),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,nullptr,&n);std::string s(n,0);if(!CryptBinaryToStringA(b.data(),(DWORD)b.size(),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,s.data(),&n))throw std::runtime_error("Base64 encoding failed.");s.resize(n);return s;}
Bytes unb64(const std::string& s){if(s.empty())return {};DWORD n=0;if(!CryptStringToBinaryA(s.data(),(DWORD)s.size(),CRYPT_STRING_BASE64,nullptr,&n,nullptr,nullptr))throw std::runtime_error("Invalid base64 data.");Bytes b(n);if(!CryptStringToBinaryA(s.data(),(DWORD)s.size(),CRYPT_STRING_BASE64,b.data(),&n,nullptr,nullptr))throw std::runtime_error("Invalid base64 data.");b.resize(n);return b;}
std::string protect(const std::string& s){if(s.empty())return {};DATA_BLOB in{(DWORD)s.size(),(BYTE*)s.data()},out{};if(!CryptProtectData(&in,L"UDM",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out))throw std::runtime_error("Windows could not encrypt the setting.");Bytes b(out.pbData,out.pbData+out.cbData);SecureZeroMemory(out.pbData,out.cbData);LocalFree(out.pbData);return b64(b);}
std::string reveal(const std::string& s){if(s.empty())return {};Bytes b=unb64(s);DATA_BLOB in{(DWORD)b.size(),b.data()},out{};if(!CryptUnprotectData(&in,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out))throw std::runtime_error("This encrypted setting cannot be opened by the current Windows account.");std::string r((char*)out.pbData,out.cbData);SecureZeroMemory(out.pbData,out.cbData);LocalFree(out.pbData);return r;}
Json dictionary(const Json& j){if(j.is_object())return j;Json d=Json::object();if(j.is_array())for(const auto& v:j)if(v.contains("Key"))d[str(v,"Key")]=v.value("Value",Json());return d;}
Json legacyDictionary(const Json& j){Json a=Json::array();for(auto it=j.begin();it!=j.end();++it)a.push_back({{"Key",it.key()},{"Value",it.value()}});return a;}
Headers readHeaders(const Json& j){auto s=reveal(str(j,"ProtectedHeaders"));Headers h;if(!s.empty()){auto d=dictionary(Json::parse(s));for(auto it=d.begin();it!=d.end();++it)if(it.value().is_string())h[it.key()]=it.value().get<std::string>();}validateHeaders(h);return h;}
void validateHeaders(const Headers& h){for(const auto& [k,v]:h){auto key=lower(k);if((key!="authorization"&&key!="cookie"&&key!="referer"&&key!="user-agent"&&key!="accept"&&key!="accept-language"&&key!="origin")||v.size()>16384||std::any_of(v.begin(),v.end(),[](unsigned char c){return (c<32&&c!=9)||c==127;}))throw std::runtime_error("Invalid request header.");}}
Json validatePostRequest(const Json& input,const std::string& address){
 if(input.is_object()&&input.empty())return Json::object();
 if(!input.is_object()||str(input,"method")!="POST"||!input.contains("body")||!input["body"].is_string())throw std::runtime_error("Unsupported browser download request.");
 Url url(address);if(url.scheme!="http"&&url.scheme!="https")throw std::runtime_error("Form downloads require HTTP or HTTPS.");
 auto encoded=str(input,"body"),type=str(input,"contentType");
 if(encoded.size()>((MaxBrowserPostBytes+2)/3)*4||type.empty()||type.size()>256||std::any_of(type.begin(),type.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("Invalid form download content type or body size.");
 auto body=encoded.empty()?Bytes{}:unb64(encoded);if(body.size()>MaxBrowserPostBytes||b64(body)!=encoded)throw std::runtime_error("Invalid form download body.");
 if(input.contains("url")&&str(input,"url")!=address)throw std::runtime_error("This form request belongs to a different address. Capture it again in the browser.");
 return {{"method","POST"},{"url",address},{"contentType",type},{"body",encoded}};
}
Json readPostRequest(const Json& data){auto encoded=str(data,"ProtectedRequest");if(encoded.empty())return Json::object();return validatePostRequest(Json::parse(reveal(encoded)),str(data,"Url"));}

i64 epoch(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
std::string date(i64 ms){return "/Date("+std::to_string(ms?ms:epoch())+")/";}
i64 parseDate(const Json& j){
 if(j.is_null())return 0;
 if(!j.is_string())throw std::runtime_error("Invalid saved date.");
 auto s=j.get<std::string>();
 if(s.rfind("/Date(",0)==0){try{return std::stoll(s.substr(6));}catch(...){throw std::runtime_error("Invalid saved date.");}}
 std::smatch m;
 if(!std::regex_match(s,m,std::regex("([0-9]{4})-([0-9]{2})-([0-9]{2})T([0-9]{2}):([0-9]{2}):([0-9]{2})(?:[.]([0-9]{1,7}))?(Z|[+-][0-9]{2}:[0-9]{2})")))throw std::runtime_error("Unsupported saved date.");
 SYSTEMTIME t{};t.wYear=(WORD)std::stoi(m[1]);t.wMonth=(WORD)std::stoi(m[2]);t.wDay=(WORD)std::stoi(m[3]);t.wHour=(WORD)std::stoi(m[4]);t.wMinute=(WORD)std::stoi(m[5]);t.wSecond=(WORD)std::stoi(m[6]);
 auto fraction=m[7].str()+"000";t.wMilliseconds=(WORD)std::stoi(fraction.substr(0,3));FILETIME f{};
 if(!SystemTimeToFileTime(&t,&f))throw std::runtime_error("Invalid saved date.");
 ULARGE_INTEGER value{};value.LowPart=f.dwLowDateTime;value.HighPart=f.dwHighDateTime;i64 result=(i64)(value.QuadPart/10000)-11644473600000LL;
 auto zone=m[8].str();if(zone!="Z"){int hours=std::stoi(zone.substr(1,2)),minutes=std::stoi(zone.substr(4,2));if(hours>23||minutes>59)throw std::runtime_error("Invalid date time zone.");result+=(zone[0]=='+'?-1:1)*(hours*60LL+minutes)*60000;}
 return result;
}
std::string readText(const fs::path& p,size_t limit){if(fs::file_size(p)>limit)throw std::runtime_error("File exceeds the supported size.");std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Cannot read file.");std::string s((std::istreambuf_iterator<char>(f)),{});if(s.rfind("\xEF\xBB\xBF",0)==0)s.erase(0,3);return s;}
void atomicText(const fs::path& p,const std::string& s,bool backup,const std::function<void(const char*)>& checkpoint){fs::create_directories(p.parent_path());auto temp=p;temp+=L".tmp";{Handle f(CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));if(!f)throw std::runtime_error("Cannot write state temporary file.");DWORD n=0;if(!WriteFile(f.h,s.data(),(DWORD)s.size(),&n,nullptr)||n!=s.size()||!FlushFileBuffers(f.h))throw std::runtime_error("Could not flush the state file.");}if(checkpoint)checkpoint("temporary-flushed");if(fs::exists(p)){auto bak=p;bak+=L".bak";for(unsigned attempt=0;;++attempt){
 if(ReplaceFileW(p.c_str(),temp.c_str(),backup?bak.c_str():nullptr,0,nullptr,nullptr))break;
 const DWORD code=GetLastError();
 // A reader may briefly omit FILE_SHARE_DELETE. Retry only these unchanged-file failures.
 if(attempt>=6||(code!=ERROR_SHARING_VIOLATION&&code!=ERROR_LOCK_VIOLATION))throw std::runtime_error("Could not replace the saved state (Windows error "+std::to_string(code)+"). Retry after closing programs that hold the state file.");
 Sleep(25*(attempt+1));
}}else if(!MoveFileExW(temp.c_str(),p.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not publish the saved state.");}
void writeBytes(const fs::path& p,const Bytes& b){std::ofstream f(p,std::ios::binary|std::ios::trunc);f.write((const char*)b.data(),(std::streamsize)b.size());f.flush();if(!f)throw std::runtime_error("Could not write file.");}
fs::path appDir(){wchar_t p[32768];DWORD n=GetModuleFileNameW(nullptr,p,32768);return fs::path(std::wstring(p,n)).parent_path();}
fs::path configuredData(const fs::path& installation,const fs::path& fallback){
 auto config=installation/L"udm-data.json";if(!fs::exists(config))return fallback;
 auto settings=Json::parse(readText(config,4096));auto value=str(settings,"dataDirectory");
 if(!settings.is_object()||value.empty()||value.size()>32700||value.find_first_of("\r\n")!=std::string::npos||value.find('\0')!=std::string::npos)throw std::runtime_error("Invalid UDM data location. Check udm-data.json beside the app.");
 auto target=fs::path(wide(value));if(target.is_relative())target=installation/target;
 target=fs::absolute(target).lexically_normal();if(target==target.root_path())throw std::runtime_error("UDM data must use its own folder, not a drive root.");
 return target;
}
fs::path defaultData(){PWSTR p=nullptr;if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&p)))throw std::runtime_error("Cannot locate the Windows data folder.");fs::path r=fs::path(p)/L"UDM";CoTaskMemFree(p);return configuredData(appDir(),r);}
std::wstring quote(const std::wstring& s){std::wstring r=L"\"";size_t n=0;for(auto c:s){if(c==L'\\'){++n;continue;}if(c==L'\"')r.append(n*2+1,L'\\');else r.append(n,L'\\');r+=c;n=0;}r.append(n*2,L'\\');return r+L'\"';}
void Cancel::wait(int ms)const{while(ms>0){check();int n=std::min(ms,50);Sleep(n);ms-=n;}check();}
void Rate::wait(size_t n,i64 kbps,const Cancel& c){if(kbps<=0){std::lock_guard<std::mutex> l(mutex);next={};return;}std::chrono::steady_clock::time_point until;{std::lock_guard<std::mutex> l(mutex);auto now=std::chrono::steady_clock::now();if(next<now)next=now;next+=std::chrono::microseconds((i64)(n*1000000.0/(kbps*1024.0)));until=next;}while(std::chrono::steady_clock::now()<until)c.wait((int)std::min<i64>(50,std::chrono::duration_cast<std::chrono::milliseconds>(until-std::chrono::steady_clock::now()).count()+1));}
std::string unescape(const std::string& s){std::string r;for(size_t i=0;i<s.size();++i){if(s[i]=='%'&&i+2<s.size()&&isxdigit((unsigned char)s[i+1])&&isxdigit((unsigned char)s[i+2])){r+=(char)std::stoi(s.substr(i+1,2),nullptr,16);i+=2;}else r+=s[i];}return r;}
Url::Url(const std::string& value):full(value){if(value.size()>32768||value.find_first_of("\r\n")!=std::string::npos)throw std::runtime_error("Invalid URL.");auto w=wide(value);URL_COMPONENTS u{};u.dwStructSize=sizeof(u);u.dwSchemeLength=u.dwHostNameLength=u.dwUrlPathLength=u.dwExtraInfoLength=u.dwUserNameLength=u.dwPasswordLength=(DWORD)-1;if(lower(value.substr(0,6))=="ftp://"||!WinHttpCrackUrl(w.c_str(),(DWORD)w.size(),0,&u)){
 // FTP must use its own grammar even when WinHttpCrackUrl accepts the scheme.
 std::smatch m;if(!std::regex_match(value,m,std::regex("^(ftp)://([^/@:#?]+)(?::([0-9]{1,5}))?(/[^#]*)?$",std::regex::icase)))throw std::runtime_error("Use a complete HTTP, HTTPS or FTP URL without credentials.");scheme="ftp";host=lower(m[2]);auto ftpPort=m[3].matched?std::stoi(m[3]):21;if(ftpPort<1||ftpPort>65535)throw std::runtime_error("FTP port must be 1-65535.");port=(INTERNET_PORT)ftpPort;path=m[4].matched?m[4].str():"/";origin="ftp://"+host+(port==21?"":":"+std::to_string(port));return;}
 scheme=lower(utf8(std::wstring(u.lpszScheme,u.dwSchemeLength)));host=lower(utf8(std::wstring(u.lpszHostName,u.dwHostNameLength)));port=u.nPort;if(host.empty()||(scheme!="http"&&scheme!="https")||u.dwUserNameLength||u.dwPasswordLength)throw std::runtime_error("Use a complete HTTP or HTTPS URL without credentials.");path=utf8(std::wstring(u.lpszUrlPath,u.dwUrlPathLength));if(path.empty())path="/";if(u.dwExtraInfoLength)query=utf8(std::wstring(u.lpszExtraInfo,u.dwExtraInfoLength));auto fragment=query.find('#');if(fragment!=std::string::npos)query.resize(fragment);origin=scheme+"://"+host+(((scheme=="http"&&port==80)||(scheme=="https"&&port==443))?"":":"+std::to_string(port));}
std::string combineUrl(const std::string& base,const std::string& relative){auto a=wide(base),b=wide(relative);DWORD n=32768;std::wstring r(n,0);if(FAILED(UrlCombineW(a.c_str(),b.c_str(),r.data(),&n,0)))throw std::runtime_error("Invalid redirect URL.");r.resize(n);auto s=utf8(r);auto hash=s.find('#');if(hash!=std::string::npos)s.resize(hash);return s;}
bool hostIs(const std::string& h,const std::string& d){return h==d||(h.size()>d.size()&&h.compare(h.size()-d.size()-1,d.size()+1,"."+d)==0);}
std::map<std::string,std::string> query(const std::string& s){std::map<std::string,std::string> r;auto q=Url(s).query;if(!q.empty()&&q[0]=='?')q.erase(0,1);std::stringstream in(q);std::string p;while(std::getline(in,p,'&')){auto e=p.find('=');if(e!=std::string::npos)r[unescape(p.substr(0,e))]=unescape(p.substr(e+1));}return r;}
std::vector<std::string> expand(const std::string& text){std::vector<std::string> r;std::istringstream lines(text);std::string line;std::regex re("\\[([0-9]{1,6}|[A-Za-z])-([0-9]{1,6}|[A-Za-z])\\]");while(std::getline(lines,line)){line=trim(line);if(line.empty()||line[0]=='#')continue;std::smatch m;std::vector<std::string> next;if(std::regex_search(line,m,re)){std::string a=m[1],b=m[2];bool letter=isalpha((unsigned char)a[0])!=0;if(letter&&(a.size()!=1||b.size()!=1||(!isupper(a[0])!=!isupper(b[0]))))throw std::runtime_error("Use letters of the same case.");int first=letter?a[0]:std::stoi(a),last=letter?b[0]:std::stoi(b);if(last<first||last-first>999)throw std::runtime_error("A batch can contain at most 1,000 URLs.");for(int i=first;i<=last;++i){std::ostringstream n;if(letter)n<<(char)i;else n<<std::setfill('0')<<std::setw(a[0]=='0'?(int)a.size():1)<<i;next.push_back(m.prefix().str()+n.str()+m.suffix().str());}}else next.push_back(line);for(const auto& s:next){Url u(s);if(std::find(r.begin(),r.end(),s)==r.end())r.push_back(s);if(r.size()>1000)throw std::runtime_error("Add at most 1,000 URLs at a time.");}}return r;}
std::string fileHash(const fs::path& p){BCRYPT_ALG_HANDLE a=nullptr;BCRYPT_HASH_HANDLE h=nullptr;if(BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Cannot initialize SHA-256.");try{if(BCryptCreateHash(a,&h,nullptr,0,nullptr,0,0)<0)throw std::runtime_error("Cannot initialize SHA-256.");std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Cannot open file for verification.");char b[65536];while(f){f.read(b,sizeof(b));if(f.gcount()&&BCryptHashData(h,(PUCHAR)b,(ULONG)f.gcount(),0)<0)throw std::runtime_error("SHA-256 failed.");}if(!f.eof())throw std::runtime_error("Cannot read file for verification.");BYTE digest[32];if(BCryptFinishHash(h,digest,32,0)<0)throw std::runtime_error("SHA-256 failed.");BCryptDestroyHash(h);h=nullptr;BCryptCloseAlgorithmProvider(a,0);a=nullptr;std::ostringstream s;for(auto c:digest)s<<std::hex<<std::setfill('0')<<std::setw(2)<<(int)c;return s.str();}catch(...){if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);throw;}}
void markZone(const fs::path& p){Handle f(CreateFileW((p.wstring()+L":Zone.Identifier").c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,0,nullptr));if(f){const char s[]="[ZoneTransfer]\r\nZoneId=3\r\n";DWORD n;WriteFile(f.h,s,sizeof(s)-1,&n,nullptr);}}
Json defaultSettings(){PWSTR p=nullptr;std::string folder;if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads,0,nullptr,&p))){folder=utf8((fs::path(p)/L"UDM").wstring());CoTaskMemFree(p);}return {{"UseTls13",true},{"IgnoreLastModified",false},{"ProxyAutoConfigUrl",""},{"BrowserDownloadLater",false},{"QueuePromptLater",true},{"QueuePromptBatch",true},{"CategoryRememberLast",Json::object()},{"ClipboardMode","Show a suggestion"},{"ClipboardOnlyFileTypes",true},{"GlobalLimitMode","Total across downloads"},{"UserAgent",""},{"FtpPassive",true},{"DialEnabled",false},{"DialEntry",""},{"DialPhonebook",""},{"DialAttempts",3},{"DialRetrySeconds",10},{"DialTimeoutSeconds",120},{"DownloadFolder",folder},{"Parallel",3},{"Connections",8},{"Retries",3},{"LimitKbps",0},{"QuotaMb",0},{"QuotaHours",1},{"ServerConnections",Json::array()},{"TemporaryFolder",""},{"UseServerDate",false},{"DuplicatePolicy","Ask"},{"PrefetchFileInfo",true},{"CompletedDoubleClick","Properties"},{"DarkMode",false},{"HideCategories",false},{"TrayIcon","Color"},{"FontName","Tahoma"},{"FontHeight",11},{"FontWeight",400},{"QueueProgressMinimized",true},{"CategoryFolders",true},{"ClipboardMonitor",false},{"CloseToTray",false},{"Sound",true},{"SkipBrowserFileInfo",false},{"SuppressProgressDialog",false},{"SuppressCompletionDialog",false},{"CategoryPaths",Json::array()},{"CustomCategories",Json::array()},{"CategoryRules",Json::array()},{"SiteLogins",Json::array()},{"CaptureExtensions","zip 7z rar iso exe msi pdf mp4 mkv mp3 flac"},{"CaptureExcludedHosts",""},{"Proxy",""},{"ProxyUser",""},{"ProxySecret",""},{"ScanProgram",""},{"ScanArguments","\"{file}\""},{"ScanTimeoutSeconds",300}};}
Json defaultQueue(std::string name){return {{"Name",name},{"Enabled",true},{"Parallel",2},{"Scheduled",false},{"StartMinute",0},{"StopMinute",1440},{"Days",127},{"RunOnce",false},{"OnceStarted",false},{"WakeComputer",false},{"StartOnStartup",false},{"StartOnceUtc",nullptr},{"StopOnceUtc",nullptr},{"Retries",nullptr},{"FileRetries",0},{"RetryDelaySeconds",30},{"RepeatMinutes",0},{"Synchronize",false},{"FinishAction","None"},{"FinishDelaySeconds",30}};}
bool inWindow(const Json& q,i64 now,bool manual){if(!yes(q,"Enabled",true))return false;if(!now)now=epoch();if(independentStopExpired(q,now))return false;if(yes(q,"RunOnce")){i64 a=parseDate(q.value("StartOnceUtc",Json())),b=parseDate(q.value("StopOnceUtc",Json()));return (manual||(a&&now>=a))&&(!b||now<b);}if(dailyRunsToCompletion(q)&&yes(q,"DailyRunUntilComplete"))return true;if(manual)return !dailyManualStopExpired(q,now);if(!yes(q,"Scheduled"))return true;time_t sec=now/1000;tm t{};localtime_s(&t,&sec);int m=t.tm_hour*60+t.tm_min,d=t.tm_wday,a=(int)num(q,"StartMinute"),b=yes(q,"DailyStopEnabled",true)?(int)num(q,"StopMinute",1440):1440;if(a>b&&m<b)d=(d+6)%7;if(!(num(q,"Days",127)&(1LL<<d)))return false;return a==b||(a<b?m>=a&&m<b:m>=a||m<b);}
Job::Job(Json j):data(std::move(j)){if(data.contains("Video")&&data["Video"].is_object())video=std::make_shared<Job>(data["Video"]);if(data.contains("Audio")&&data["Audio"].is_object())audio=std::make_shared<Job>(data["Audio"]);if(!std::regex_match(str(data,"Id"),std::regex("[a-fA-F0-9]{32}")))throw std::runtime_error("Invalid saved download identifier.");data["FileName"]=safeName(str(data,"FileName"));if(!data.contains("Segments")||!data["Segments"].is_array())data["Segments"]=Json::array();std::set<i64> seen;for(const auto& s:data["Segments"])if(num(s,"Index",-1)<0||num(s,"Index")>100000||!seen.insert(num(s,"Index")).second||num(s,"Start")<0||num(s,"Done")<0)throw std::runtime_error("Invalid saved download segment.");}
Json Job::snapshot()const{auto j=data;j["Video"]=video?video->snapshot():Json();j["Audio"]=audio?audio->snapshot():Json();return j;}
fs::path Job::target()const{return fs::path(wide(str(data,"Folder")))/wide(str(data,"FileName"));}
Manager::Manager(fs::path p):root(fs::absolute(p)){fs::create_directories(root);auto file=root/L"state.json";if(fs::exists(file)){state=Json::parse(readText(file));if(num(state,"Schema")!=1||!state["Settings"].is_object()||!state["Downloads"].is_array()||!state["Queues"].is_array())throw std::runtime_error("Unsupported or invalid UDM state. Original preserved.");auto backup=root/L"state.before-native.json";if(!fs::exists(backup))fs::copy_file(file,backup);}else state={{"Schema",1},{"Settings",defaultSettings()},{"Downloads",Json::array()},{"Queues",Json::array({defaultQueue()})},{"Projects",Json::array()},{"QuotaStart",date()},{"QuotaBytes",0}};auto defaults=defaultSettings();for(auto it=defaults.begin();it!=defaults.end();++it)if(!state["Settings"].contains(it.key())||state["Settings"][it.key()].is_null())state["Settings"][it.key()]=it.value();if(!state.contains("Projects")||state["Projects"].is_null())state["Projects"]=Json::array();if(state["Queues"].empty())state["Queues"].push_back(defaultQueue());initializeDefaultQueues(state);for(const auto& j:state["Downloads"]){auto job=std::make_shared<Job>(j);recoverScanner(job->data);auto status=str(job->data,"Status");if(status=="Downloading"||status=="Pausing"||status=="Verifying"||status=="Merging"||status=="Resolving"||status=="Awaiting confirmation")job->data["Status"]="Paused";job->data["ConfirmationPending"]=false;if(!job->data.contains("QueueMember"))job->data["QueueMember"]=status!="Complete";jobs.push_back(job);}recoverFileOperation();recoverReplacements();recoverRestarts();recoverLinkConversions();}
Manager::~Manager(){try{stop();}catch(...) {}}
Json Manager::snapshot()const{Lock l(mutex);auto s=state;s["Downloads"]=Json::array();for(auto j:jobs)s["Downloads"].push_back(j->snapshot());return s;}
void Manager::save(){Lock l(mutex);if(catalogTransaction){if(catalogCheckpoint)catalogCheckpoint("deferred-save");return;}try{auto serialized=snapshot().dump();if(serialized.size()>32*1024*1024)throw std::runtime_error("Download history has reached its 32 MB storage limit. Export and remove older records before adding more downloads.");atomicText(root/L"state.json",serialized,true,catalogCheckpoint);checkpointSnapshot=std::move(serialized);storageError.clear();}catch(const std::exception& e){storageError=e.what();throw;}}
std::vector<std::string> Manager::categories()const{Lock l(mutex);return optionCategories(state["Settings"]);}
JobPtr Manager::add(std::string address,std::string folder,std::string name,std::string queue,bool paused,Headers h,std::string expected,const Json& request,const Json& browserProxy,const Json& browserSession){auto session=browserDownloadSession(browserSession,address,h);auto proxy=validateBrowserProxy(browserProxy,address);auto post=validatePostRequest(request,address);Url u(address);validateHeaders(h);if(!expected.empty()&&!std::regex_match(expected,std::regex("[a-fA-F0-9]{64}")))throw std::runtime_error("SHA-256 must contain 64 hexadecimal characters.");Lock l(mutex);if(stopping)throw std::runtime_error("UDM is closing.");auto& p=state["Settings"];auto filename=safeName(name.empty()?unescape(u.path):name);auto cat=downloadCategory(filename,u.host,p);if(folder.empty())folder=categoryFolder(p,cat);folder=utf8(fs::absolute(fs::path(wide(folder))).wstring());bool found=false;for(auto q:state["Queues"])found|=str(q,"Name")==queue;if(!found)queue=str(state["Queues"][0],"Name");auto original=fs::path(wide(filename));for(int i=1;;++i){auto dest=fs::path(wide(folder))/wide(filename);bool exists=fs::exists(dest);for(auto j:jobs){exists|=lower(utf8(j->target().wstring()))==lower(utf8(dest.wstring()));exists|=lower(str(j->data,"PreviousPath"))==lower(utf8(dest.wstring()));}if(!exists)break;filename=utf8(original.stem().wstring())+" ("+std::to_string(i)+")"+utf8(original.extension().wstring());}auto j=std::make_shared<Job>(Json{{"Id",guid()},{"Url",address},{"FileName",filename},{"Folder",folder},{"Category",cat},{"Queue",queue},{"Status",paused?"Paused":"Queued"},{"Error",""},{"Size",-1},{"Received",0},{"Connections",std::clamp<i64>(num(p,"Connections",8),1,32)},{"QueueMember",true},{"LimitKbps",0},{"ExpectedSha256",expected},{"Sha256",""},{"ProtectedHeaders",h.empty()?"":protect(legacyDictionary(Json(h)).dump())},{"ProtectedRequest",post.empty()?"":protect(post.dump())},{"ProtectedBrowserProxy",proxy.empty()?"":protect(proxy.dump())},{"Added",date()},{"Segments",Json::array()},{"Description",""}});j->data["Connections"]=connectionsForUrl(p,u);if(!session.empty())j->data["ProtectedBrowserSession"]=protect(session.dump());jobs.push_back(j);try{save();}catch(...){jobs.pop_back();throw;}return j;}
bool Manager::isActive(JobPtr j)const{Lock l(mutex);return active.count(j->id())!=0;}
void Manager::resume(JobPtr j){
 Lock l(mutex);if(isActive(j))return;
 if(str(j->data,"Status")=="Complete"){
  if(!yes(j->data,"SyncRetryFailed")||!canRequestLogin(j->data))return;
  validateQueueMembership(*this,j,true,str(j->data,"Queue"));
  auto before=j->data;
  j->data["IndividualStart"]=true;j->data["SyncPending"]=true;j->data["NotBefore"]=nullptr;j->data["AuthenticationPromptPending"]=false;
  try{save();}catch(...){j->data=before;throw;}return;
 }
 readBrowserSession(j->data);
 if(yes(j->data,"RequiresRequestCapture"))throw std::runtime_error("This form download needs its original request. Submit the download form again in your browser.");
 if(yes(j->data,"RequiresMediaCapture"))throw std::runtime_error("This imported video needs fresh browser links. Open its page and choose a quality from the UDM video panel.");
 if(!str(j->data,"DuplicateOf").empty())throw std::runtime_error("Choose how to handle this duplicate first.");
 auto before=j->data;auto oldLimit=j->sessionLimit;auto oldPaused=schedulePaused;auto captures=state.value("BrowserCaptures",Json());
 j->data["PostAttempted"]=false;j->data["IndividualStart"]=true;j->sessionLimit.reset();j->data["QueueMember"]=true;j->data["QueueOrigin"]=false;j->data["QueueAttempts"]=0;j->data["Status"]="Queued";j->data["Error"]="";schedulePaused.erase(j->id());eraseCapturePresentations(state,j->id());
 try{save();}catch(...){j->data=before;j->sessionLimit=oldLimit;schedulePaused=oldPaused;restoreCaptureRows(state,captures);throw;}
}
void Manager::pause(JobPtr j){
 Lock l(mutex);if(str(j->data,"Status")=="Complete"){auto before=j->data;j->data["SyncPending"]=false;j->data.erase("IndividualStart");try{save();}catch(...){j->data=before;throw;}auto it=active.find(j->id());if(it!=active.end())it->second->stop=true;return;}
 auto captures=state.value("BrowserCaptures",Json());j->data["GrabberImmediate"]=false;j->data.erase("IndividualStart");cycleFailed[str(j->data,"Queue")]=true;j->data["SyncPending"]=false;schedulePaused.erase(j->id());
 if(isActive(j)){if(str(j->data,"Status")!="Complete")j->data["Status"]="Pausing";active[j->id()]->stop=true;}else if(str(j->data,"Status")!="Complete")j->data["Status"]="Paused";
 eraseCapturePresentations(state,j->id());try{save();}catch(...){restoreCaptureRows(state,captures);throw;}existingOffers.erase(j->id());
}
void Manager::remove(JobPtr j){
 Lock l(mutex);for(auto other:jobs)if(str(other->data,"ReplacementOf")==j->id())throw std::runtime_error("A pending replacement depends on this record. Remove that replacement first.");
 if(isActive(j))throw std::runtime_error("Pause the download and wait until it stops before removing it.");
 auto old=jobs;auto captures=state.value("BrowserCaptures",Json());jobs.erase(std::remove(jobs.begin(),jobs.end(),j),jobs.end());eraseCapturePresentations(state,j->id());
 try{save();existingOffers.erase(j->id());}catch(...){jobs=old;restoreCaptureRows(state,captures);throw;}
}
int Manager::retries(const std::string& queue)const{Lock l(mutex);for(auto q:state["Queues"])if(str(q,"Name")==queue)return (int)std::clamp<i64>(num(q,"Retries",num(state["Settings"],"Retries",3)),0,10);return 3;}
static i64 quotaPeriodStart(const Json& state){
 try{return parseDate(state.value("QuotaStart",Json()));}catch(const std::exception&){return 0;}
}
// Quota reservations share the manager lock across all jobs and transport workers.
void Manager::charge(size_t n,const Cancel& c,Rate& local,JobPtr j){
 i64 global=0,per=0;bool perDownload=false;
 {Lock l(mutex);perDownload=str(state["Settings"],"GlobalLimitMode","Total across downloads")=="Each download";global=num(state["Settings"],"LimitKbps");per=j->sessionLimit.value_or(num(j->data,"LimitKbps"));}
 if(perDownload){if(global>0)per=per>0?std::min(global,per):global;global=0;}
 globalRate.wait(n,global,c);local.wait(n,per,c);
 bool waiting=false;
 auto clear=[&]{Lock l(mutex);if(waiting){auto it=quotaWaiters.find(j);if(it!=quotaWaiters.end()&&--it->second==0)quotaWaiters.erase(it);waiting=false;}};
 try{size_t remaining=n;do{
  c.check();
  {Lock l(mutex);auto now=epoch(),start=quotaPeriodStart(state);auto period=std::clamp<i64>(num(state["Settings"],"QuotaHours",1),1,168)*3600000;
   if(start<=0||start>now||now-start>=period){state["QuotaStart"]=date(now);state["QuotaBytes"]=0;quotaNoticeToken.clear();}
   auto maximum=std::clamp<i64>(num(state["Settings"],"QuotaMb"),0,100000000)*1024*1024;
   auto used=std::max<i64>(0,num(state,"QuotaBytes"));
   auto available=maximum>0?std::max<i64>(0,maximum-used):std::numeric_limits<i64>::max()-used;
   auto reserved=std::min<size_t>(remaining,(size_t)available);
   state["QuotaBytes"]=used+(i64)reserved;remaining-=reserved;
   if(!remaining){clear();return;}
   if(!waiting){++quotaWaiters[j];waiting=true;}
  }
  c.wait(100);
 }while(true);}catch(...){clear();throw;}
}
Json Manager::quotaStatus(JobPtr job)const{
 Lock lock(mutex);const auto& prefs=state["Settings"];auto limit=std::clamp<i64>(num(prefs,"QuotaMb"),0,100000000)*1024*1024;
 auto hours=std::clamp<i64>(num(prefs,"QuotaHours",1),1,168),start=quotaPeriodStart(state),until=start+hours*3600000,now=epoch();
 bool waiting=false;for(auto& item:quotaWaiters)if(!job||item.first==job||item.first==job->video||item.first==job->audio){waiting=true;break;}
 waiting=waiting&&limit>0&&num(state,"QuotaBytes")>=limit&&start>0&&start<=now&&now<until;
 return {{"Waiting",waiting},{"LimitBytes",limit},{"Bytes",num(state,"QuotaBytes")},{"Hours",hours},{"PeriodStart",start},{"ResumeAt",until},{"Seconds",std::max<i64>(0,(until-now+999)/1000)}};
}
Json Manager::takeQuotaWarning(){
 Lock lock(mutex);auto value=quotaStatus();if(!yes(value,"Waiting")||!yes(state["Settings"],"WarnQuota",true))return Json::object();
 auto token=std::to_string(num(value,"PeriodStart"))+":"+std::to_string(num(value,"Hours"))+":"+std::to_string(num(value,"LimitBytes"));
 if(token==quotaNoticeToken)return Json::object();quotaNoticeToken=token;return value;
}
std::string quotaWaitText(const Json& value){
 if(!yes(value,"Waiting"))return "";auto seconds=num(value,"Seconds");
 return "Download limit reached. Resuming automatically in "+std::to_string(seconds/3600)+" hr "+std::to_string((seconds/60)%60)+" min "+std::to_string(seconds%60)+" sec.";
}

void Manager::progress(JobPtr j,size_t n,size_t segment,Worker* worker){Lock l(mutex);auto& s=j->data["Segments"][segment];s["Done"]=num(s,"Done")+n;j->data["Received"]=num(j->data,"Received")+n;j->data["TransferredBytes"]=num(j->data,"TransferredBytes")+n;if(worker){worker->received+=n;worker->position=num(s,"Start")+num(s,"Done");worker->state="Receiving data";}}
void Manager::ensureConnection(JobPtr j,const Cancel& c){Json prefs;{Lock l(mutex);prefs=state["Settings"];}ensureDialConnection(prefs,c,[this,j](const std::string& message){Lock l(mutex);if(message.empty())j->data.erase("ConnectionStatus");else j->data["ConnectionStatus"]=message;});}
void requestLiveHlsFinish(Manager& manager,JobPtr job){
 Lock lock(manager.mutex);if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()||!yes(job->data,"LiveRecording")||str(job->data,"Status")=="Complete"||str(job->data,"Status")=="Awaiting confirmation")throw std::runtime_error("There is no active or retained live recording to save.");
 auto before=job->data;job->data["LiveStopRequested"]=true;try{manager.save();}catch(...){job->data=before;throw;}
 if(job->liveCapture)job->liveCapture->stop=true;else if(!manager.isActive(job))manager.resume(job);
}
void Manager::start(JobPtr j){if(stopping||isActive(j)||str(j->data,"Status")=="Complete"||!str(j->data,"DuplicateOf").empty())return;state["CompletionWorkEpoch"]=guid();auto c=std::make_shared<Cancel>();active[j->id()]=c;j->data["Status"]="Downloading";j->data["LastAttempt"]=date();j->data["Error"]="";j->data["LastHttpStatus"]=0;j->data["LastFtpStatus"]=0;clearServerRetry(j->data);j->data["AuthenticationPromptPending"]=false;j->data.erase("AuthenticationOrigin");j->data.erase("AuthenticationScheme");j->speedMeter.reset(num(j->data,"TransferredBytes"));threads.emplace_back([this,j,c]{bool complete=false;try{if(yes(j->data,"RequiresMediaCapture"))throw std::runtime_error("Fresh browser-captured media is required.");if(event&&!yes(j->data,"ConfirmationPending"))event(j,false);ensureConnection(j,*c);if(j->data.contains("OfflineProject"))offlineTransfer(*this,j,c);else if(!str(j->data,"ProtectedAdaptive").empty())adaptiveTransfer(*this,j,c);else if(str(j->data,"SourceUrl").empty())transfer(*this,j,c);else mediaTransfer(*this,j,c);complete=true;{Lock l(mutex);j->data["QueueMember"]=retainCompletedMembership(*this,j->data);}}catch(const std::exception& e){Lock l(mutex);if(c->cancelled()){bool retryScheduled=schedulePaused.erase(j->id())&&!stopping;if(retryScheduled)for(const auto& q:state["Queues"])if(str(q,"Name")==str(j->data,"Queue")){auto stop=parseDate(q.value("StopOnceUtc",Json()));if(independentStopExpired(q,epoch())||(yes(q,"RunOnce")&&stop&&epoch()>=stop))retryScheduled=false;}j->data["Status"]=retryScheduled?"Queued":"Paused";}else j->data["Status"]="Failed";j->data["Error"]=c->cancelled()?"":e.what();if(auto http=dynamic_cast<const HttpRejected*>(&e)){j->data["LastHttpStatus"]=http->status;recordServerRetry(j->data,*http);}if(auto auth=dynamic_cast<const AuthenticationRequired*>(&e)){j->data["AuthenticationOrigin"]=auth->origin;j->data["AuthenticationScheme"]=auth->scheme;j->data["AuthenticationPromptPending"]=canRequestLogin(j->data);}if(auto auth=dynamic_cast<const FtpAuthenticationRequired*>(&e)){j->data["LastFtpStatus"]=530;j->data["AuthenticationOrigin"]=auth->origin;j->data["AuthenticationScheme"]="FTP";j->data["AuthenticationPromptPending"]=canRequestLogin(j->data);}}if(complete)scanCompleted(j,*c);{Lock l(mutex);active.erase(j->id());j->data.erase("IndividualStart");schedulePaused.erase(j->id());j->data.erase("ConnectionStatus");j->speed=0;try{save();}catch(...) {}}if(complete&&event)event(j,true);});}
void Manager::tick(){Lock l(mutex);if(stopping)return;auto now=std::chrono::steady_clock::now();double seconds=std::max(.000001,std::chrono::duration<double>(now-lastTick).count());lastTick=now;for(auto j:jobs){if(j->video||j->audio){i64 received=0,total=0,network=0;bool known=true;for(auto ch:{j->video,j->audio})if(ch){received+=num(ch->data,"Received");network+=num(ch->data,"TransferredBytes");known&=num(ch->data,"Size",-1)>=0;total+=num(ch->data,"Size");}if(str(j->data,"Status")=="Downloading"){j->data["Received"]=received;j->data["TransferredBytes"]=network;j->data["Size"]=known?total:-1;}}j->speed=j->speedMeter.update(num(j->data,"TransferredBytes"),seconds,isActive(j)&&str(j->data,"Status")=="Downloading");j->data["PeakSpeed"]=std::max(real(j->data,"PeakSpeed"),j->speedMeter.instant);}queueTick(epoch());projectLinksTick();updateWakeTimer(epoch());if(++ticks%3==0&&(!storageError.empty()||snapshot().dump()!=checkpointSnapshot))save();}
void Manager::stop(){{Lock l(mutex);stopping=true;if(wakeTimer)wakeTimer->stop();for(auto& [id,c]:active)c->stop=true;}for(auto& t:threads)if(t.joinable())t.join();threads.clear();try{save();}catch(...){Lock l(mutex);stopping=false;throw;}}
int retryAfterDelay(const std::string& header,i64 now){
 auto first=header.find_first_not_of(" \t"),last=header.find_last_not_of(" \t");if(first==std::string::npos)return -1;
 auto value=header.substr(first,last-first+1);
 if(value.find_first_not_of("0123456789")==std::string::npos){
  unsigned seconds=0;for(char c:value){seconds=seconds*10+(c-'0');if(seconds>300)return -2;}return (int)seconds*1000;
 }
 // WinHTTP's date parser accepts incomplete strings; require a full HTTP-date first.
 static const std::regex httpDate(R"(^(?:(?:Mon|Tue|Wed|Thu|Fri|Sat|Sun), [0-9]{2} (?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) [0-9]{4} [0-2][0-9]:[0-5][0-9]:[0-5][0-9] GMT|(?:Monday|Tuesday|Wednesday|Thursday|Friday|Saturday|Sunday), [0-9]{2}-(?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)-[0-9]{2} [0-2][0-9]:[0-5][0-9]:[0-5][0-9] GMT|(?:Mon|Tue|Wed|Thu|Fri|Sat|Sun) (?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) (?: [0-9]|[0-9]{2}) [0-2][0-9]:[0-5][0-9]:[0-5][0-9] [0-9]{4})$)");
 if(!std::regex_match(value,httpDate))return -1;
 SYSTEMTIME system{};FILETIME file{};if(!WinHttpTimeToSystemTime(wide(value).c_str(),&system)||!SystemTimeToFileTime(&system,&file))return -1;
 ULARGE_INTEGER stamp{};stamp.LowPart=file.dwLowDateTime;stamp.HighPart=file.dwHighDateTime;
 i64 delay=(i64)(stamp.QuadPart/10000)-11644473600000LL-now;if(delay<=0)return 0;if(delay>300000)return -2;return (int)delay;
}
HttpRejected::HttpRejected(DWORD code,const std::string& retryAfter):runtime_error("The server rejected the download URL (HTTP "+std::to_string(code)+")."+(code==401?" This server requires authentication. Enter its login and password, or sign in through the browser.":code==407?" The proxy requires authentication. Check Options > Proxy.":(code==403||code==410)?" The link or session may have expired. Use Refresh download address to obtain a fresh link; saved parts are retained.":"")+(retryAfterDelay(retryAfter,epoch())==-2?" The server requested a wait longer than five minutes. Try again later; saved parts are retained.":"")),status(code),retryAfterMs(retryAfterDelay(retryAfter,epoch())){}
bool HttpRejected::retryable()const{return retryAfterMs!=-2&&(status==408||status==425||status==429||status==500||status==502||status==503||status==504);}
int HttpRejected::delay(int attempt)const{return std::max(retryAfterMs,std::min(10000,500*(1<<std::clamp(attempt,0,4))));}

std::string recoveryPage(const Json& data){
 auto source=str(data,"DownloadPage");if(source.empty()){auto h=readHeaders(data);for(const auto& [key,value]:h)if(lower(key)=="referer")source=value;}
 if(!source.empty())try{Url page(source);if(page.scheme=="http"||page.scheme=="https")return source;}catch(...){}
 try{Url u(str(data,"Url"));if(hostIs(u.host,"microsoft.com")&&u.path.find("Win11_")!=std::string::npos)return "https://www.microsoft.com/en-us/software-download/windows11";}catch(...){}
 return "";
}
bool Manager::canRefreshAddress(JobPtr job)const{
 Lock lock(mutex);if(!job||job->data.contains("OfflineProject")||!str(job->data,"ProtectedRequest").empty()||yes(job->data,"RequiresRequestCapture")||isActive(job)||str(job->data,"Status")=="Complete"||(!str(job->data,"SourceUrl").empty()&&str(job->data,"ProtectedSabr").empty()&&!directMediaJob(job))||yes(job->data,"LiveRecording"))return false;
 try{if(!str(job->data,"ProtectedAdaptive").empty()&&yes(Json::parse(reveal(str(job->data,"ProtectedAdaptive"))),"live"))return false;Url u(str(job->data,"Url"));return u.scheme=="http"||u.scheme=="https";}catch(...){return false;}
}
void Manager::beginAddressRefresh(JobPtr job,bool preserveCaptured){
 Lock lock(mutex);if(!canRefreshAddress(job))throw std::runtime_error("Pause an unfinished HTTP download before refreshing its address. For video captures, use the browser's video panel.");
 if(!refreshId.empty()&&refreshId!=job->id()&&refreshUntil>epoch())throw std::runtime_error("Another download is already waiting for a replacement address.");
 auto previous=job->data;job->data["Status"]="Paused";if((!str(job->data,"SourceUrl").empty()||!str(job->data,"ProtectedAdaptive").empty())&&(!preserveCaptured||addressRefreshCandidate(job).is_null()))job->data.erase("ProtectedRefreshOffer");try{save();}catch(...){job->data=previous;throw;}refreshId=job->id();refreshUntil=epoch()+10*60*1000;
}
void Manager::cancelAddressRefresh(JobPtr job){Lock lock(mutex);if(job&&refreshId==job->id()){refreshId.clear();refreshUntil=0;}}
Json Manager::addressRefreshCandidate(JobPtr job)const{
 Lock lock(mutex);auto value=str(job->data,"ProtectedRefreshOffer");if(value.empty())return Json();auto offer=Json::parse(reveal(value));if(num(offer,"received")+10*60*1000<epoch())return Json();
 if(str(offer,"kind")=="sabr"){const auto& session=offer.at("message").at("sabr");auto captured=real(session,"capturedAt");if(!std::isfinite(captured)||captured<epoch()-300000||captured>epoch()+30000)return Json();}
 return offer;
}
JobPtr Manager::captureAddressRefresh(const std::string& address,const Headers& headers,const std::string& name,const std::string& page){
 Lock lock(mutex);if(refreshId.empty()||refreshUntil<epoch())return {};
 JobPtr job;for(auto j:jobs)if(j->id()==refreshId)job=j;if(!canRefreshAddress(job)||!str(job->data,"SourceUrl").empty()||!str(job->data,"ProtectedAdaptive").empty())return {};
 Url old(str(job->data,"Url")),next(address);if(next.scheme!="http"&&next.scheme!="https")return {};
 auto filename=lower(safeName(name.empty()?unescape(next.path):name));
 if(filename!=lower(str(job->data,"FileName"))&&filename!=lower(safeName(unescape(old.path))))return {};
 validateHeaders(headers);Json offer={{"url",address},{"headers",Json(headers)},{"page",page},{"received",epoch()}};
 auto before=job->data;job->data["ProtectedRefreshOffer"]=protect(offer.dump());try{save();}catch(...){job->data=before;throw;}return job;
}
void Manager::refreshAddress(JobPtr job,const std::string& address,std::optional<Headers> supplied,const std::string& sourcePage){
 Lock lock(mutex);if(!canRefreshAddress(job))throw std::runtime_error("This download cannot refresh its address while active or complete.");
 if(std::find(jobs.begin(),jobs.end(),job)==jobs.end())throw std::runtime_error("Unknown download.");
 if(!str(job->data,"SourceUrl").empty()||!str(job->data,"ProtectedAdaptive").empty())throw std::runtime_error("Refresh this media session using the browser panel and review the captured streams.");
 Url old(str(job->data,"Url")),next(trim(address));if(next.scheme!="http"&&next.scheme!="https")throw std::runtime_error("The new address must use HTTP or HTTPS.");
 auto headers=supplied?*supplied:readHeaders(job->data);
 if(!supplied&&old.origin!=next.origin)for(auto it=headers.begin();it!=headers.end();){auto key=lower(it->first);if(key=="cookie"||key=="authorization"||key=="referer"||key=="origin")it=headers.erase(it);else ++it;}
 validateHeaders(headers);if(!sourcePage.empty()){Url page(sourcePage);if(page.scheme!="http"&&page.scheme!="https")throw std::runtime_error("The download page must use HTTP or HTTPS.");}
 auto before=job->data;auto captures=state.value("BrowserCaptures",Json());auto captured=addressRefreshCandidate(job);auto nextSession=supplied&&captured.is_object()&&str(captured,"url")==next.full?browserDownloadSession(captured.value("browserSession",Json::object()),next.full,headers):Json::object();if(captured.is_object()&&str(captured,"url")==next.full&&captured.contains("browserProxy")){auto route=validateBrowserProxy(captured["browserProxy"],next.full);job->data["ProtectedBrowserProxy"]=route.empty()?"":protect(route.dump());job->data.erase("RequiresBrowserProxyCapture");}if(supplied){auto session=nextSession;if(session.empty())job->data.erase("ProtectedBrowserSession");else job->data["ProtectedBrowserSession"]=protect(session.dump());job->data.erase("RequiresBrowserSessionCapture");}job->data["Url"]=next.full;job->data.erase("ProtectedResolvedUrl");job->data.erase("AuthenticationOrigin");job->data.erase("AuthenticationScheme");job->data["AuthenticationPromptPending"]=false;job->data["ProtectedHeaders"]=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());job->data["RefreshPendingValidation"]=true;job->data["Status"]="Paused";job->data["Error"]="";job->data["LastHttpStatus"]=0;clearServerRetry(job->data);job->data["AddressRefreshed"]=date();job->data.erase("ProtectedRefreshOffer");if(!sourcePage.empty())job->data["DownloadPage"]=sourcePage;
 eraseCapturePresentations(state,job->id());try{save();}catch(...){job->data=before;restoreCaptureRows(state,captures);throw;}cancelAddressRefresh(job);
}
void Manager::configure(JobPtr j,const Json& edit){Lock l(mutex);if(isActive(j)||str(j->data,"Status")=="Complete")throw std::runtime_error("Stop the download before changing its properties.");auto next=j->data;for(auto it=edit.begin();it!=edit.end();++it)next[it.key()]=it.value();readPostRequest(next);if(j->data.contains("OfflineProject")&&str(next,"Url")!=str(j->data,"Url"))throw std::runtime_error("Change the starting page in Site Grabber and create a new offline project.");Url u(str(next,"Url"));if((!str(j->data,"SourceUrl").empty()||!str(j->data,"ProtectedAdaptive").empty())&&str(next,"Url")!=str(j->data,"Url"))throw std::runtime_error("Refresh video streams using the browser panel.");auto target=fs::path(wide(str(next,"Folder")))/wide(str(next,"FileName"));if(!target.is_absolute()||safeName(str(next,"FileName"))!=str(next,"FileName"))throw std::runtime_error("Choose a complete destination and valid file name.");if(!str(j->data,"ReplacementOf").empty()&&target.lexically_normal()!=j->target().lexically_normal())throw std::runtime_error("A replacement keeps the original file location. Choose a numbered copy to save elsewhere.");for(auto other:jobs)if(other!=j&&((lower(utf8(other->target().wstring()))==lower(utf8(target.wstring()))&&other->id()!=str(j->data,"ReplacementOf"))||lower(str(other->data,"PreviousPath"))==lower(utf8(target.wstring()))))throw std::runtime_error("That destination belongs to another download.");if(fs::exists(target)&&target!=j->target())throw std::runtime_error("Destination already exists.");auto expected=str(next,"ExpectedSha256");if(!expected.empty()&&!std::regex_match(expected,std::regex("[a-fA-F0-9]{64}")))throw std::runtime_error("Invalid SHA-256.");if(num(next,"Connections",8)<1||num(next,"Connections",8)>32||num(next,"LimitKbps")<0||num(next,"LimitKbps")>1000000)throw std::runtime_error("Invalid connection or speed limit.");if(str(next,"Url")!=str(j->data,"Url")){
 next["RefreshPendingValidation"]=true;next.erase("ProtectedResolvedUrl");next.erase("AuthenticationOrigin");next.erase("AuthenticationScheme");next["AuthenticationPromptPending"]=false;
 if(Url(str(next,"Url")).origin!=Url(str(j->data,"Url")).origin){auto headers=readHeaders(next);for(auto it=headers.begin();it!=headers.end();){auto key=lower(it->first);if(key=="cookie"||key=="authorization"||key=="referer"||key=="origin")it=headers.erase(it);else ++it;}next["ProtectedHeaders"]=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());}
 }
 if(str(next,"Folder")!=str(j->data,"Folder"))next.erase("ProjectDestinationRoot");validateGrabberFolder(next);
 validateFileMetadata(next);if(edit.contains("Queue")){bool found=false;for(auto& q:state["Queues"])found|=str(q,"Name")==str(next,"Queue");if(!found)throw std::runtime_error("Choose an existing queue.");}if(edit.contains("Category")){auto cats=categories();if(std::find(cats.begin(),cats.end(),str(next,"Category"))==cats.end())throw std::runtime_error("Choose an existing category.");}if(!yes(next,"QueueMember",true)&&str(next,"Status")=="Queued")next["Status"]="Paused";auto previous=j->data;j->data=next;try{save();}catch(...){j->data=previous;throw;}}
void Manager::setSettings(const Json& p){
 validateBrowserSettings(p);validateOptionsModel(p);validateToolbarSettings(p);validatePacSettings(p);validateSiteLogins(p);validateDialSettings(p);validateScannerSettings(p);auto agent=str(p,"UserAgent");if(agent.size()>1024||std::any_of(agent.begin(),agent.end(),[](unsigned char ch){return ch<32||ch==127;}))throw std::runtime_error("User-Agent must be one line of at most 1024 bytes without control characters.");
 validateProtocolProxies(p);
 auto proxyMode=str(p,"ProxyMode",str(p,"Proxy").empty()?"Use Windows proxy / PAC settings":"Use a proxy server");if(proxyMode!="Use Windows proxy / PAC settings"&&proxyMode!="Connect directly"&&proxyMode!="Use a proxy server"&&proxyMode!="Use a SOCKS5 proxy"&&proxyMode!="Use a SOCKS4 / 4a proxy"&&!usesPacScript(p))throw std::runtime_error("Choose a valid proxy mode.");if(isSocksProxy(p))validateSocksSettings(p);if(proxyMode=="Use a proxy server"&&trim(str(p,"Proxy")).empty())throw std::runtime_error("Enter a proxy server.");for(const char* field:{"Proxy","ProxyBypass"})if(str(p,field).find_first_of("\r\n")!=std::string::npos)throw std::runtime_error("Proxy settings must be on one line.");
 auto limitMode=str(p,"GlobalLimitMode","Total across downloads"),clipboardMode=str(p,"ClipboardMode","Show a suggestion");if(limitMode!="Total across downloads"&&limitMode!="Each download")throw std::runtime_error("Choose how the global speed limit applies.");if(clipboardMode!="Show a suggestion"&&clipboardMode!="Open Download File Info")throw std::runtime_error("Choose a clipboard capture mode.");
 if(p.contains("WarnQuota")&&!p["WarnQuota"].is_boolean())throw std::runtime_error("Choose whether download-limit warnings are enabled.");
 auto duplicate=str(p,"DuplicatePolicy","Ask");if(duplicate!="Ask"&&duplicate!="Numbered"&&duplicate!="Existing"&&duplicate!="Replace")throw std::runtime_error("Choose a valid duplicate-link policy.");
 if(num(p,"QuotaHours",1)<1||num(p,"QuotaHours",1)>168)throw std::runtime_error("Quota period must be 1 to 168 hours.");
 if(!str(p,"TemporaryFolder").empty()&&!fs::path(wide(str(p,"TemporaryFolder"))).is_absolute())throw std::runtime_error("Choose an absolute temporary-files folder.");
 if(p.contains("ServerConnections")){if(!p["ServerConnections"].is_array()||p["ServerConnections"].size()>100)throw std::runtime_error("Too many server connection rules.");for(const auto& rule:p["ServerConnections"])validateConnectionRule(rule);}
 if(num(p,"FontHeight",11)<8||num(p,"FontHeight",11)>24||str(p,"FontName","Tahoma").size()>31)throw std::runtime_error("Choose a font size from 8 to 24 pixels.");
 if(num(p,"Parallel")<1||num(p,"Parallel")>16||num(p,"Connections")<1||num(p,"Connections")>32||num(p,"Retries")<0||num(p,"Retries")>10||num(p,"LimitKbps")<0||num(p,"LimitKbps")>1000000||num(p,"QuotaMb")<0||num(p,"QuotaMb")>100000000)throw std::runtime_error("Invalid connection, retry or bandwidth setting.");if(!fs::path(wide(str(p,"DownloadFolder"))).is_absolute())throw std::runtime_error("Choose an absolute download folder.");for(auto ext:words(str(p,"CaptureExtensions")))if(!std::regex_match(ext,std::regex("[a-z0-9_-]{1,30}")))throw std::runtime_error("Use extensions separated by spaces.");Lock l(mutex);auto old=state["Settings"];state["Settings"]=p;if(num(p,"LimitKbps")>0)state["Settings"]["GlobalLimitRememberedKbps"]=num(p,"LimitKbps");try{save();}catch(...){state["Settings"]=old;throw;}if(!yes(p,"OfferCaptureExclusions",true)){lastCancelledCaptureHost.clear();captureCancellationCount=0;}}

void Manager::deleteQueue(const std::string& name){
 Lock lock(mutex);if(state["Queues"].size()<2)throw std::runtime_error("Keep at least one queue.");
 auto found=std::find_if(state["Queues"].begin(),state["Queues"].end(),[&](const Json& q){return str(q,"Name")==name;});if(found==state["Queues"].end())throw std::runtime_error("That queue no longer exists.");const bool sync=yes(*found,"Synchronize");
 for(auto job:jobs)if(str(job->data,"Queue")==name&&isActive(job))throw std::runtime_error("Stop this queue first.");
 auto previous=state["Queues"],next=previous;next.erase(std::remove_if(next.begin(),next.end(),[&](const Json& q){return str(q,"Name")==name;}),next.end());
 const Json* destination=nullptr;for(const auto& q:next)if(yes(q,"Synchronize")==sync){if(!destination)destination=&q;if(str(q,"DefaultQueueRole")==std::string(sync?"Synchronization":"Download")||(!sync&&str(q,"Name")=="Main queue")){destination=&q;break;}}
 if(!destination)destination=&next.front();const auto target=str(*destination,"Name");const bool targetSync=yes(*destination,"Synchronize");std::vector<std::pair<JobPtr,Json>> records;
 for(auto job:jobs)if(str(job->data,"Queue")==name)records.emplace_back(job,job->data);
 try{
  state["Queues"]=next;for(auto& entry:records){auto& data=entry.first->data;data["Queue"]=target;data["SyncPending"]=false;if(str(data,"Status")=="Complete"&&!targetSync)data["QueueMember"]=false;if(str(data,"Status")=="Queued")data["Status"]="Paused";}
  save();
 }catch(...){state["Queues"].swap(previous);for(auto& entry:records)entry.first->data.swap(entry.second);throw;}
 manualQueues.erase(name);cyclingQueues.erase(name);openWindows.erase(name);cycleFailed.erase(name);finishedQueues.erase(std::remove_if(finishedQueues.begin(),finishedQueues.end(),[&](const Json& q){return str(q,"Queue")==name;}),finishedQueues.end());for(auto& entry:records)schedulePaused.erase(entry.first->id());updateWakeTimer(epoch());
}
void Manager::saveProject(const Json& incoming){
 Lock l(mutex);auto p=incoming;
 // A browser handoff can finish between the wizard collecting its draft and
 // saving it. Preserve that exact ticket's receipt instead of resurrecting it.
 if(yes(p,"BrowserLogin")&&p.contains("PendingBrowserLogin")){
  auto pending=p["PendingBrowserLogin"];auto ticket=str(pending,"Token");
  for(const auto& saved:state["Projects"])if(str(saved,"Id")==str(p,"Id")&&!ticket.empty()&&!saved.contains("PendingBrowserLogin")&&str(saved,"LastBrowserLoginTicket")==ticket&&yes(saved,"BrowserLogin")&&str(saved,"StartUrl")==str(p,"StartUrl")){
   for(const char* key:{"ProtectedBrowserSession","LastBrowserLoginTicket","BrowserLoginSaved"})if(saved.contains(key))p[key]=saved[key];
   p.erase("PendingBrowserLogin");p.erase("ExploreState");break;
  }
 }
 validateGrabberProject(p,state["Settings"]);validateGrabberActions(p);auto old=state["Projects"];bool found=false;
 for(auto& item:state["Projects"])if(str(item,"Id")==str(p,"Id")){item=p;found=true;break;}if(!found)state["Projects"].push_back(p);
 try{save();}catch(...){state["Projects"].swap(old);throw;}
}
int Manager::addProject(const Json& project,const std::string& queue,bool paused,bool immediate,bool resumeExisting){
 Lock l(mutex);if(catalogTransaction)throw std::runtime_error("A project is already being added.");
 auto p=project;validateGrabberProject(p,state["Settings"]);validateGrabberActions(p);GrabberFilters filters(p);auto oldJobs=jobs;auto oldProjects=state["Projects"],oldScheduled=Json(schedulePaused);int count=0;
 std::vector<Json> oldData;std::vector<std::optional<i64>> oldLimits;std::map<std::string,JobPtr> existingLinks;for(auto j:jobs){oldData.push_back(j->data);oldLimits.push_back(j->sessionLimit);if(str(j->data,"ProjectId")==str(p,"Id"))existingLinks[str(j->data,"Url")]=j;}
 auto startMatched=[&](JobPtr job){if(!immediate||isActive(job)||str(job->data,"Status")=="Complete")return;resume(job);job->data["GrabberImmediate"]=true;job->data["GrabberParallel"]=num(p,"DownloadParallel",2);job->data["SuppressProgressDialog"]=true;job->data["SuppressCompletionDialog"]=true;};
 // No worker can observe the staged jobs while this lock is held. The only
 // persistent write contains both the complete selection and its saved project.
 catalogTransaction=true;
 try{
  if(str(p,"Template")=="Offline website (ZIP)"){
   bool found=false;for(auto j:jobs)found|=str(j->data,"ProjectId")==str(p,"Id")&&j->data.contains("OfflineProject");
   if(!found){addOfflineProject(p,queue,paused);++count;}
  }else for(auto& link:p["Links"])if(yes(link,"Selected",true)){
   auto found=existingLinks.find(str(link,"Url"));if(found!=existingLinks.end()){link["DownloadId"]=found->second->id();if(resumeExisting)startMatched(found->second);continue;}
   if(!filters.legacy&&(!filters.fileLocationAllowed(str(link,"Url"))||!filters.nameAllowed(str(link,"DiscoveredUrl",str(link,"Url")),str(link,"FileName",grabberFileName(str(link,"Url"))))||!filters.sizeAllowed(num(link,"Size",-1))))throw std::runtime_error("A selected file no longer matches the project filters. Explore again or change the selection.");
   auto dest=grabberDestination(p,state["Settings"],str(link,"Url"),grabberCollectedName(p,link));

   auto j=add(str(link,"Url"),dest.folder,dest.name,queue,paused,grabberRequestHeaders(p,str(link,"Referrer"),str(link,"Url")));j->data["ProjectId"]=str(p,"Id");if(yes(p,"BrowserLogin"))j->data["ProtectedBrowserSession"]=protect(grabberBrowserSession(p).dump());j->data["Description"]=yes(p,"UseLinkDescriptions",true)?str(link,"Description"):"";if(yes(p,"ConvertLinks"))j->data["SuppressCompletionDialog"]=true;if(!str(link,"ContentType").empty())j->data["GrabberContentType"]=str(link,"ContentType");if(!str(link,"DiscoveredUrl").empty())j->data["GrabberDiscoveredUrl"]=str(link,"DiscoveredUrl");if(!str(link,"Referrer").empty())j->data["DownloadPage"]=str(link,"Referrer");
   if(!dest.category.empty())j->data["Category"]=dest.category;
   if(!dest.root.empty())j->data["ProjectDestinationRoot"]=dest.root;
   validateGrabberFolder(j->data);startMatched(j);existingLinks[str(link,"Url")]=j;link["DownloadId"]=j->id();++count;
  }
  validateGrabberPlan(jobs,oldJobs.size());saveProject(p);catalogTransaction=false;save();return count;
 }catch(...){catalogTransaction=false;jobs.swap(oldJobs);state["Projects"].swap(oldProjects);schedulePaused=oldScheduled.get<std::set<std::string>>();for(size_t i=0;i<jobs.size();++i){jobs[i]->data=oldData[i];jobs[i]->sessionLimit=oldLimits[i];}throw;}
}
}
