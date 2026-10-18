#pragma once
#include "ContentSniff.hpp"
#include "QueueRetry.hpp"
// Loopback server with fixed bodies per path; no range support, so transfers
// use the sequential path exactly as servers that ignore Range do.
class SniffFixture {
 SOCKET listener=INVALID_SOCKET;std::thread server;std::vector<std::thread> clients;std::atomic_bool stop{false};
 std::map<std::string,std::pair<std::string,std::string>> bodies;
 void serve(SOCKET s){
  try{DWORD timeout=3000;setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));std::string request;char buffer[4096];
   while(request.find("\r\n\r\n")==std::string::npos&&request.size()<32768){int n=recv(s,buffer,sizeof(buffer),0);if(n<=0)break;request.append(buffer,n);}
   auto first=request.find(' '),last=request.find(' ',first+1);std::string path=first==std::string::npos?"":request.substr(first+1,last-first-1);
   auto found=bodies.find(path);std::string response;
   if(found==bodies.end())response="HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
   else response="HTTP/1.1 200 OK\r\nContent-Type: "+found->second.first+"\r\nContent-Length: "+std::to_string(found->second.second.size())+"\r\nConnection: close\r\n\r\n"+found->second.second;
   size_t at=0;while(at<response.size()){int n=::send(s,response.data()+at,(int)std::min<size_t>(65536,response.size()-at),0);if(n<=0)break;at+=n;}
  }catch(...){}
  shutdown(s,SD_BOTH);closesocket(s);
 }
public:
 unsigned short port=0;
 explicit SniffFixture(std::map<std::string,std::pair<std::string,std::string>> content):bodies(std::move(content)){
  listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  if(bind(listener,(sockaddr*)&address,sizeof(address))||listen(listener,16))throw std::runtime_error("Content sniff fixture failed.");int size=sizeof(address);getsockname(listener,(sockaddr*)&address,&size);port=ntohs(address.sin_port);
  server=std::thread([this]{while(!stop){fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval wait{0,100000};if(select(0,&set,nullptr,nullptr,&wait)<=0)continue;auto client=accept(listener,nullptr,nullptr);if(client!=INVALID_SOCKET)clients.emplace_back([this,client]{serve(client);});}});
 }
 ~SniffFixture(){stop=true;if(server.joinable())server.join();closesocket(listener);for(auto& t:clients)if(t.joinable())t.join();}
 std::string url(const std::string& path)const{return "http://127.0.0.1:"+std::to_string(port)+path;}
};
static void contentSniffChecks(const udm::fs::path& root){
 using namespace udm;
 auto sniffText=[](const std::string& s,bool complete=true){return sniffContent(reinterpret_cast<const uint8_t*>(s.data()),s.size(),complete);};
 auto padded=[](std::string head,size_t size){head.resize(std::max(head.size(),size),'\x01');return head;};
 // Signatures at offset zero.
 struct Case{std::string body,ext,mime;bool specific;const char* name;};
 std::string pe(0x200,'\0');pe[0]='M';pe[1]='Z';pe[0x3c]='\x80';pe.replace(0x80,4,std::string("PE\0\0",4));
 std::string dll=pe;dll[0x80+23]=(char)0x20;
 std::string iso(0x8806,'\0');iso.replace(0x8001,5,"CD001");
 std::string tar(1024,'\0');tar.replace(0,8,"file.txt");tar.replace(257,5,"ustar");
 std::string ts(400,'\x11');ts[0]=ts[188]=ts[376]=0x47;
 const std::vector<Case> cases={
  {padded("PK\x03\x04",200),"zip","application/zip",true,"ZIP archive"},
  {padded(std::string("PK\x03\x04",4)+std::string(26,'\0')+"[Content_Types].xmlword/document.xml",300),"docx","application/vnd.openxmlformats-officedocument.wordprocessingml.document",true,"Office Open XML document inside ZIP"},
  {padded(std::string("PK\x03\x04",4)+std::string(26,'\0')+"mimetypeapplication/epub+zip",300),"epub","application/epub+zip",true,"EPUB inside ZIP"},
  {padded(std::string("PK\x03\x04",4)+std::string(26,'\0')+"AndroidManifest.xml",300),"apk","application/vnd.android.package-archive",true,"Android package inside ZIP"},
  {padded("Rar!\x1a\x07\x01",100),"rar","application/vnd.rar",true,"RAR archive"},
  {padded("7z\xbc\xaf\x27\x1c",100),"7z","application/x-7z-compressed",true,"7-Zip archive"},
  {padded("\x1f\x8b\x08",100),"gz","application/gzip",true,"gzip"},
  {padded("%PDF-1.7\n",100),"pdf","application/pdf",true,"PDF"},
  {pe,"exe","application/vnd.microsoft.portable-executable",true,"Windows executable"},
  {dll,"dll","application/vnd.microsoft.portable-executable",true,"Windows library"},
  {iso,"iso","application/x-iso9660-image",true,"ISO 9660 image at its fixed descriptor offset"},
  {tar,"tar","application/x-tar",true,"tar archive"},
  {padded(std::string("\0\0\0\x20" "ftypisom",12),100),"mp4","video/mp4",true,"MP4"},
  {padded(std::string("\0\0\0\x14" "ftypqt  ",12),100),"mov","video/quicktime",true,"QuickTime"},
  {padded(std::string("\0\0\0\x20" "ftypM4A ",12),100),"m4a","audio/mp4",true,"MPEG-4 audio"},
  {padded("\x1a\x45\xdf\xa3\x9f\x42\x86\x81\x01\x42\x82\x84webm",100),"webm","video/webm",true,"WebM"},
  {padded("\x1a\x45\xdf\xa3\x9f\x42\x86\x81\x01\x42\x82\x88matroska",100),"mkv","video/x-matroska",true,"Matroska"},
  {padded(std::string("RIFF\x10\0\0\0AVI LIST",16),100),"avi","video/x-msvideo",true,"AVI"},
  {padded(std::string("FLV\x01\x05\0\0\0\x09",9),100),"flv","video/x-flv",true,"FLV with audio and video"},
  {padded(std::string("OggS\0\x02",6),100),"ogg","audio/ogg",true,"Ogg audio"},
  {padded("\x89PNG\r\n\x1a\n",100),"png","image/png",true,"PNG"},
  {padded("\xff\xd8\xff\xe0",100),"jpg","image/jpeg",true,"JPEG"},
  {ts,"ts","video/mp2t",true,"MPEG transport stream sync bytes"},
  {padded(std::string("ID3\x04\0\0\0\0\0\0",10),100),"mp3","audio/mpeg",false,"ID3-tagged audio is recognized but not treated as specific"},
 };
 for(const auto& c:cases){auto r=sniffText(c.body);check(r.verdict==SniffVerdict::Match&&r.extension==c.ext&&r.mime==c.mime&&r.specific==c.specific,std::string("Content signature: ")+c.name);}
 auto ole=sniffText(padded("\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1",600));
 check(ole.verdict==SniffVerdict::Match&&!ole.specific&&ole.extension.empty()&&std::find(ole.compatible.begin(),ole.compatible.end(),"msi")!=ole.compatible.end(),"Compound documents stay ambiguous between MSI and Office formats");
 auto flvReserved=sniffText(padded(std::string("FLV\x01\x80\0\0\0\x09",9),100));check(flvReserved.verdict!=SniffVerdict::Match||flvReserved.extension!="flv","FLV with reserved flag bits is not accepted");
 // Compound-document producers: the root entry class identifier, then well-known stream names.
 auto compound=[&](const std::string& clsid,const std::string& stream){std::string h(1024,'\0');h.replace(0,8,std::string("\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1",8));h[30]=9;if(!clsid.empty())h.replace(512+80,16,clsid);std::string w;for(char c:stream){w.push_back(c);w.push_back('\0');}h.replace(640,w.size(),w);return h;};
 const std::string msiId("\x84\x10\x0c\0\0\0\0\0\xc0\0\0\0\0\0\0\x46",16),wordId("\x06\x09\x02\0\0\0\0\0\xc0\0\0\0\0\0\0\x46",16),pptId("\x10\x8d\x81\x64\x9b\x4f\xcf\x11\x86\xea\0\xaa\0\xb9\x29\xe8",16);
 auto msi=sniffText(compound(msiId,""));check(msi.verdict==SniffVerdict::Match&&msi.specific&&msi.extension=="msi"&&msi.mime=="application/x-msi","An installer package is identified by its root class identifier");
 check(sniffText(compound(wordId,"")).extension=="doc"&&sniffText(compound(pptId,"")).extension=="ppt","Word and PowerPoint 97-2003 files are identified by their class identifiers");
 check(sniffText(compound("","Workbook")).extension=="xls"&&sniffText(compound("","__properties_version1.0")).extension=="msg","Stream names identify a workbook and an Outlook message without a class identifier");
 check(correctedFileName("setup.php",msi)=="setup.msi"&&correctedFileName("setup.exe",msi).empty(),"A recognized installer gains .msi behind a script address but never replaces an explicit extension");
 // Long-tail signatures.
 auto atOffset=[&](size_t offset,const std::string& magic,size_t size){std::string s(std::max(size,offset+magic.size()),'\x01');s.replace(offset,magic.size(),magic);return s;};
 const std::vector<Case> tail={
  {atOffset(7,"**ACE**",100),"ace","application/x-ace-compressed",true,"ACE archive"},
  {padded("xar!",100),"xar","application/x-xar",true,"XAR archive"},
  {padded("\x1f\x9d\x90",100),"z","application/x-compress",true,"compress data"},
  {padded("\x04\x22\x4d\x18",100),"lz4","application/x-lz4",true,"LZ4 frame"},
  {padded("070701",200),"cpio","application/x-cpio",true,"cpio archive"},
  {padded("hsqs",100),"squashfs","application/vnd.squashfs",true,"SquashFS image"},
  {padded("ITSF\x03\0\0\0",100),"chm","application/vnd.ms-htmlhelp",true,"Compiled HTML help"},
  {padded("KDMV",100),"vmdk","application/x-vmdk",true,"VMware disk"},
  {padded("QFI\xfb",100),"qcow2","application/x-qemu-disk",true,"QEMU disk"},
  {padded("vhdxfile",100),"vhdx","application/x-vhdx",true,"Hyper-V disk"},
  {padded("conectix",100),"vhd","application/x-vhd",true,"Virtual PC disk"},
  {atOffset(0x8001,"BEA01",0x8806),"iso","application/x-iso9660-image",true,"UDF disc image at the volume recognition sequence"},
  {atOffset(0x8801,"CD001",0x8806),"iso","application/x-iso9660-image",true,"Raw 2352-byte sector ISO image"},
  {padded("Cr24\x03\0\0\0",100),"crx","application/x-chrome-extension",true,"Chrome extension"},
  {padded(std::string("\xca\xfe\xba\xbe\0\0\0\x34",8),100),"class","application/java-vm",true,"Java class file"},
  {padded(std::string("\0asm\x01\0\0\0",8),100),"wasm","application/wasm",true,"WebAssembly module"},
  {padded("dex\n035\0",100),"dex","application/vnd.android.dex",true,"Dalvik executable"},
  {padded(std::string("L\0\0\0\x01\x14\x02\0\0\0\0\0\xc0\0\0\0\0\0\0\x46",20),100),"lnk","application/x-ms-shortcut",true,"Windows shortcut"},
  {padded(std::string("\0\x01\0\0\0\x0c\0\x80\0\x03\0\x40",12),100),"ttf","font/ttf",true,"TrueType font"},
  {padded("OTTO",100),"otf","font/otf",true,"OpenType font"},
  {padded("wOFF",100),"woff","font/woff",true,"WOFF font"},
  {padded("wOF2",100),"woff2","font/woff2",true,"WOFF2 font"},
  {padded(std::string("AT&TFORM\0\0\x10\0DJVU",16),100),"djvu","image/vnd.djvu",true,"DjVu document"},
  {atOffset(60,"BOOKMOBI",100),"mobi","application/x-mobipocket-ebook",true,"Mobipocket e-book"},
  {padded(std::string("\0\0\x01\0\x02\0",6),100),"ico","image/x-icon",true,"Windows icon"},
  {padded(std::string("\0\0\0\x0cjP  \r\n\x87\n",12),100),"jp2","image/jp2",true,"JPEG 2000 image"},
  {padded("\xff\x0a",100),"jxl","image/jxl",true,"JPEG XL codestream"},
  {padded("gimp xcf v011",100),"xcf","image/x-xcf",true,"GIMP image"},
  {padded("DDS |\0\0\0",100),"dds","image/vnd-ms.dds",true,"DirectDraw surface"},
  {padded("\x76\x2f\x31\x01",100),"exr","image/x-exr",true,"OpenEXR image"},
  {padded("#?RADIANCE\n",100),"hdr","image/vnd.radiance",true,"Radiance HDR image"},
  {padded("qoif",100),"qoi","image/qoi",true,"QOI image"},
  {padded("MAC \x96\x0f",100),"ape","audio/ape",true,"Monkey's Audio"},
  {padded("wvpk",100),"wv","audio/wavpack",true,"WavPack audio"},
  {padded("MThd\0\0\0\x06",100),"mid","audio/midi",true,"MIDI sequence"},
  {padded("caff\0\x01\0\0",100),"caf","audio/x-caf",true,"Core Audio file"},
  {padded("#!AMR\n",100),"amr","audio/amr",true,"AMR audio"},
  {padded("\x06\x0e\x2b\x34\x02\x05\x01\x01\x0d\x01\x02\x01\x01\x02",100),"mxf","application/mxf",true,"MXF media"},
  {padded("DKIF\0\0\x20\0",100),"ivf","video/x-ivf",true,"IVF video"},
  {padded("YUV4MPEG2 W320",100),"y4m","video/x-yuv4mpeg",true,"YUV4MPEG stream"},
  {padded("DVDVIDEO-VTS",100),"ifo","application/octet-stream",true,"DVD navigation data"},
  {padded("\xd4\xc3\xb2\xa1\x02\0\x04\0",100),"pcap","application/vnd.tcpdump.pcap",true,"Packet capture"},
  {padded(std::string("\x0a\x0d\x0d\x0a\x1c\0\0\0\x4d\x3c\x2b\x1a",12),100),"pcapng","application/x-pcapng",true,"pcapng capture"},
  {padded("PAR1",100),"parquet","application/vnd.apache.parquet",true,"Parquet table"},
  {padded("\x89HDF\r\n\x1a\n",100),"h5","application/x-hdf5",true,"HDF5 data"},
  {padded("\x93NUMPY\x01\0",100),"npy","application/x-numpy-data",true,"NumPy array"},
  {padded("GGUF\x03\0\0\0",100),"gguf","application/x-gguf",true,"GGUF model"},
  {atOffset(4,"Standard Jet DB",100),"mdb","application/x-msaccess",true,"Access database"},
  {padded("REGEDIT4\r\n",100),"reg","text/x-ms-regedit",true,"Registry script"},
  {padded("glTF\x02\0\0\0",100),"glb","model/gltf-binary",true,"glTF binary model"},
  {padded("BLENDER-v300",100),"blend","application/x-blender",true,"Blender scene"},
  {padded("MDMP\x93\xa7",100),"dmp","application/x-dmp",true,"Windows minidump"},
  {padded(std::string("PK\x03\x04",4)+std::string(26,'\0')+"AppxManifest.xml",300),"appx","application/vnd.ms-appx",true,"Windows app package inside ZIP"},
  {padded(std::string("PK\x03\x04",4)+std::string(26,'\0')+"extension.vsixmanifest",300),"vsix","application/vsix",true,"Visual Studio extension inside ZIP"},
  {padded(std::string("PK\x03\x04",4)+std::string(26,'\0')+"Payload/App.app/Info.plist",300),"ipa","application/octet-stream",true,"iOS application archive inside ZIP"},
  {padded(std::string("PK\x03\x04",4)+std::string(26,'\0')+"doc.kml",300),"kmz","application/vnd.google-earth.kmz",true,"Google Earth archive inside ZIP"},
  {padded(std::string("PK\x03\x04",4)+std::string(26,'\0')+"mimetypeapplication/vnd.oasis.opendocument.graphics",300),"odg","application/vnd.oasis.opendocument.graphics",true,"OpenDocument drawing inside ZIP"},
  {padded(std::string("\xca\xfe\xba\xbe\0\0\0\x02",8),100),"","application/x-mach-binary",false,"Universal Mach-O binary stays non-specific"},
  {padded("\xcf\xfa\xed\xfe",100),"","application/x-mach-binary",false,"Mach-O executable stays non-specific"},
  {padded("P6\n320 240\n255\n",100),"ppm","image/x-portable-pixmap",false,"Portable pixmap is recognized but its two-byte magic never renames"},
  {padded(std::string("\0\0\x01\xb3",4),100),"mpg","video/mpeg",false,"MPEG video elementary stream stays non-specific"},
  {padded("regf",100),"","application/x-ms-registry-hive",false,"Registry hive stays non-specific"},
 };
 for(const auto& c:tail){auto r=sniffText(c.body);check(r.verdict==SniffVerdict::Match&&r.extension==c.ext&&r.mime==c.mime&&r.specific==c.specific,std::string("Long-tail signature: ")+c.name);}
 check(sniffText(padded(std::string("\0\x01\0\0\0\0\0\0",8),100)).verdict!=SniffVerdict::Match&&sniffText(padded(std::string("\0\0\x01\0\0\0",6),100)).verdict!=SniffVerdict::Match,"Zero-table fonts and zero-image icons are not recognized");
 check(webPageInsteadOfFile(sniffText("Access denied\n"),"png")&&webPageInsteadOfFile(sniffText("Access denied\n"),"ttf")&&webPageInsteadOfFile(sniffText("Access denied\n"),"vmdk"),"Text under image, font and disk-image names is stopped");
 check(sniffText(std::string("<html><body>Not found</body></html>\n")+padded("wOFF",300)).verdict==SniffVerdict::MatchAfterPrefix,"A long-tail format behind an inserted notice is found at its offset");
 // Text, HTML, empty and pending content.
 auto html=sniffText(std::string("\xef\xbb\xbf  <!DOCTYPE html><html><head><title>Sign in</title></head><body>Please sign in</body></html>"));
 check(html.verdict==SniffVerdict::Html&&html.mime=="text/html"&&html.extension=="html","A login page with a BOM is HTML");
 check(sniffText("<html lang=\"en\"><body>x</body></html>").verdict==SniffVerdict::Html,"Lower-case html element is HTML");
 check(sniffText("<?xml version=\"1.0\"?><feed>entries</feed>").verdict==SniffVerdict::Text,"XML without HTML is text, not a web page");
 check(sniffText("{\"error\":\"link expired\"}").verdict==SniffVerdict::Text,"JSON error bodies are text");
 check(sniffText("Download limit reached. Try again later.\r\n").verdict==SniffVerdict::Text,"Plain notices are text");
 check(sniffText(u8"Résumé — naïve café\n").verdict==SniffVerdict::Text,"UTF-8 text is text");
 check(sniffText(std::string("\xff\xfeH\0e\0l\0l\0o\0\r\0\n\0",14)).verdict==SniffVerdict::Text,"UTF-16 text with a byte-order mark is text");
 check(sniffText("").verdict==SniffVerdict::Empty&&sniffText("").mime=="application/x-empty","Empty complete content is reported as empty");
 check(sniffText("",false).verdict==SniffVerdict::NeedMore&&sniffText("short text",false).verdict==SniffVerdict::NeedMore,"Short unfinished content waits for more bytes");
 check(sniffText(padded("plain",6000),false).verdict==SniffVerdict::None,"Binary content without a signature stays unknown");
 std::string random(4096,'\0');for(size_t i=0;i<random.size();++i)random[i]=(char)((i*131+17)%251);
 auto unknown=sniffText(random);check(unknown.verdict==SniffVerdict::None&&unknown.mime=="application/octet-stream","Unrecognized binary is octet-stream");
 // Binary after a text prefix.
 auto prefix=std::string("<html><body>Your download will start shortly.</body></html>\n");auto prefixed=sniffText(prefix+padded("PK\x03\x04",300));
 check(prefixed.verdict==SniffVerdict::MatchAfterPrefix&&prefixed.extension=="zip"&&prefixed.prefixLength==prefix.size(),"An archive after an HTML notice is found at its offset");
 check(sniffText(std::string(3000,'a')+padded("PK\x03\x04",300)).verdict!=SniffVerdict::MatchAfterPrefix,"A signature beyond the prefix limit is not trusted");
 check(sniffText("ab"+padded("%PDF-1.4",100)).verdict!=SniffVerdict::MatchAfterPrefix,"Very short prefixes are not treated as inserted notices");
 // Web page decision.
 check(webPageInsteadOfFile(html,"zip")&&webPageInsteadOfFile(html,"exe")&&webPageInsteadOfFile(html,"MP4"),"HTML under a file-format name is a web page");
 check(!webPageInsteadOfFile(html,"html")&&!webPageInsteadOfFile(html,"htm")&&!webPageInsteadOfFile(html,"php")&&!webPageInsteadOfFile(html,"aspx"),"HTML under web page names is accepted");
 check(!webPageInsteadOfFile(html,"")&&!webPageInsteadOfFile(html,"txt")&&!webPageInsteadOfFile(html,"xml"),"HTML without an extension or as text/XML is not stopped");
 auto text=sniffText("Access denied\n");
 check(webPageInsteadOfFile(text,"iso")&&webPageInsteadOfFile(text,"rar")&&webPageInsteadOfFile(text,"r01")&&webPageInsteadOfFile(text,"z02"),"Text under binary-only names, including split volumes, is stopped");
 check(!webPageInsteadOfFile(text,"txt")&&!webPageInsteadOfFile(text,"csv")&&!webPageInsteadOfFile(text,"")&&!webPageInsteadOfFile(text,"log"),"Text under text or unknown names is accepted");
 check(!webPageInsteadOfFile(text,"bin")&&!webPageInsteadOfFile(text,"dat"),"Generic binary names may hold text");
 check(!webPageInsteadOfFile(sniffText(cases[0].body),"zip")&&!webPageInsteadOfFile(unknown,"zip"),"Binary content is never a web page");
 // Extension correction.
 auto zip=sniffText(cases[0].body),docx=sniffText(cases[1].body),mp3=sniffText(cases.back().body);
 check(correctedFileName("download",zip)=="download.zip"&&correctedFileName("download.php",zip)=="download.zip"&&correctedFileName("file.bin",zip)=="file.zip"&&correctedFileName("get.aspx",docx)=="get.docx","Missing or endpoint extensions take the content extension");
 check(correctedFileName("setup-1.0.3",sniffText(pe))=="setup-1.0.3.exe","Version-number suffixes are not mistaken for extensions");
 check(correctedFileName("report.docx",zip).empty()&&correctedFileName("bundle.jar",zip).empty()&&correctedFileName("song.mp3",zip).empty(),"Explicit extensions are never changed");
 check(correctedFileName("archive.zip",zip).empty()&&correctedFileName("track",mp3).empty()&&correctedFileName("installer",ole).empty(),"Matching or ambiguous content leaves the name");
 check(correctedFileName("index",html)=="index.html"&&correctedFileName("view.php",html).empty(),"HTML gains .html only without a usable extension");
 check(correctedFileName("notes",text).empty()&&correctedFileName("data",unknown).empty()&&correctedFileName("bundle",prefixed).empty(),"Text, unknown and prefixed content keep the name");
 check(correctedFileName("",zip).empty()&&correctedFileName("archive. ",sniffText(cases[4].body))=="archive.rar","Empty names stay empty and trailing dots are not doubled");
 // End-to-end through the transfer and publication path.
 std::string archive=padded("PK\x03\x04",4096);const std::string noticeText="<html><body>Your download will start shortly.</body></html>\n";
 SniffFixture server({{"/archive.zip",{"text/html","<!DOCTYPE html><html><body>Session expired. Sign in again.</body></html>"}},
  {"/download.php",{"application/octet-stream",archive}},{"/notice.iso",{"text/plain","The file is temporarily unavailable.\n"}},
  {"/readme",{"text/plain","Read me first.\n"}},{"/allowed.zip",{"text/html","<html><body>kept</body></html>"}},{"/taken.php",{"application/octet-stream",archive}},{"/plain.php",{"application/octet-stream",archive}},{"/music.php",{"application/octet-stream",archive}},{"/large.zip",{"text/html","<!DOCTYPE html><html><body>"+std::string(8*1024*1024,'x')+"</body></html>"}},{"/prefixed.zip",{"application/octet-stream",noticeText+archive}},{"/prefixed-kept.zip",{"application/octet-stream",noticeText+archive}}});
 auto base=root/L"content-sniff";fs::create_directories(base/L"files");
 Manager m(base/L"state");auto settings=m.state["Settings"];settings["DownloadFolder"]=utf8((base/L"files").wstring());settings["CategoryFolders"]=false;settings["PrefetchFileInfo"]=false;settings["Retries"]=0;settings["ProxyMode"]="Connect directly";m.setSettings(settings);
 auto finish=[&](JobPtr job){for(int i=0;i<1000;++i){m.tick();if(!m.isActive(job)&&(str(job->data,"Status")=="Complete"||str(job->data,"Status")=="Failed"))return;Sleep(10);}throw std::runtime_error("Content sniff download timed out.");};
 check(yes(m.state["Settings"],"StopOnWebPage")&&yes(m.state["Settings"],"CorrectFileExtensions"),"Content checks are enabled by default");
 auto page=m.add(server.url("/archive.zip"),"","","Main queue",false);finish(page);
 check(str(page->data,"Status")=="Failed"&&yes(page->data,"WebPageReceived")&&str(page->data,"ContentCheck")=="Html"&&str(page->data,"Error").find("web page")!=std::string::npos,"A login page served for a .zip stops with a web page error");
 check(!fs::exists(page->target())&&fs::exists(m.root/L"parts"/wide(page->id())),"The web page is not published and its received data is kept");
 check(!queueFailureCanRetry(Json{{"Enabled",true},{"FileRetries",5}},Json{{"Status","Failed"},{"QueueOrigin",true},{"QueueMember",true},{"WebPageReceived",true}},true),"Queues do not automatically retry a web page response");
 auto large=m.add(server.url("/large.zip"),"","","Main queue",false);finish(large);
 check(str(large->data,"Status")=="Failed"&&yes(large->data,"WebPageReceived")&&num(large->data,"Received")<1048576,"A large web page served for a .zip is stopped after its first bytes");
 auto hashOf=[&](const std::string& bytes){auto p=base/wide("expected-"+std::to_string(bytes.size())+".bin");writeBytes(p,Bytes(bytes.begin(),bytes.end()));return fileHash(p);};
 auto prefixedJob=m.add(server.url("/prefixed.zip"),"","","Main queue",false);finish(prefixedJob);
 check(str(prefixedJob->data,"Status")=="Failed"&&yes(prefixedJob->data,"ContentPrefixPending")&&num(prefixedJob->data,"ContentPrefixBytes")==(i64)noticeText.size()&&str(prefixedJob->data,"ContentCheck")=="MatchAfterPrefix"&&fs::exists(m.root/L"parts"/wide(prefixedJob->id())),"An archive behind inserted web page text waits for a choice with its parts kept");
 prefixedJob->data["StripContentPrefix"]=true;m.resume(prefixedJob);finish(prefixedJob);
 check(str(prefixedJob->data,"Status")=="Complete"&&num(prefixedJob->data,"StrippedPrefixBytes")==(i64)noticeText.size()&&fileHash(prefixedJob->target())==hashOf(archive)&&num(prefixedJob->data,"Size")==(i64)archive.size(),"Removing the inserted text publishes exactly the archive bytes");
 auto keptPrefix=m.add(server.url("/prefixed-kept.zip"),"","","Main queue",false);finish(keptPrefix);keptPrefix->data["AllowContentPrefix"]=true;m.resume(keptPrefix);finish(keptPrefix);
 check(str(keptPrefix->data,"Status")=="Complete"&&fileHash(keptPrefix->target())==hashOf(noticeText+archive)&&!keptPrefix->data.contains("StrippedPrefixBytes"),"Keeping the data as received publishes the whole body");
 check(!queueFailureCanRetry(Json{{"Enabled",true},{"FileRetries",5}},Json{{"Status","Failed"},{"QueueOrigin",true},{"QueueMember",true},{"ContentPrefixPending",true}},true),"Queues wait for the prefix choice instead of retrying");
 auto notice=m.add(server.url("/notice.iso"),"","","Main queue",false);finish(notice);
 check(str(notice->data,"Status")=="Failed"&&yes(notice->data,"WebPageReceived")&&str(notice->data,"ContentCheck")=="Text","A text notice served for an .iso is stopped");
 auto renamed=m.add(server.url("/download.php"),"","","Main queue",false);finish(renamed);
 check(str(renamed->data,"Status")=="Complete"&&str(renamed->data,"FileName")=="download.zip"&&str(renamed->data,"OriginalFileName")=="download.php"&&fs::exists(base/L"files"/L"download.zip")&&!fs::exists(base/L"files"/L"download.php"),"An archive behind a script address is published with its content extension");
 check(str(renamed->data,"SniffedType")=="application/zip"&&str(renamed->data,"ContentCheck")=="Match","The recognized content type is recorded");
 {const auto host=Url(server.url("/download.php")).host,before=downloadCategory("download.php",host,m.state["Settings"]),after=downloadCategory("download.zip",host,m.state["Settings"]);
  check(before!=after&&str(renamed->data,"Category")==after,"An automatically assigned category follows the corrected extension");}
 auto chosen=m.add(server.url("/music.php"),"","","Main queue",true);chosen->data["Category"]="Music";m.resume(chosen);finish(chosen);
 check(str(chosen->data,"FileName")=="music.zip"&&str(chosen->data,"Category")=="Music","A user-chosen category is kept when the extension is corrected");
 auto readme=m.add(server.url("/readme"),"","","Main queue",false);finish(readme);
 check(str(readme->data,"Status")=="Complete"&&str(readme->data,"FileName")=="readme","Text without an extension is published unchanged");
 writeBytes(base/L"files"/L"taken.zip",Bytes{'x'});
 auto taken=m.add(server.url("/taken.php"),"","","Main queue",false);finish(taken);
 check(str(taken->data,"Status")=="Complete"&&str(taken->data,"FileName")=="taken.php"&&fs::file_size(base/L"files"/L"taken.zip")==1,"An occupied corrected name keeps the original name and never touches the other file");
 auto off=m.state["Settings"];off["StopOnWebPage"]=false;off["CorrectFileExtensions"]=false;m.setSettings(off);
 auto allowed=m.add(server.url("/allowed.zip"),"","","Main queue",false);finish(allowed);
 check(str(allowed->data,"Status")=="Complete"&&fs::exists(allowed->target()),"Turning the check off publishes the server's response");
 auto plain=m.add(server.url("/plain.php"),"","","Main queue",false);finish(plain);
 check(str(plain->data,"Status")=="Complete"&&str(plain->data,"FileName")=="plain.php","Turning extension correction off keeps the name");
 off["StopOnWebPage"]=true;m.setSettings(off);page->data["AllowWebPageContent"]=true;m.resume(page);finish(page);
 check(str(page->data,"Status")=="Complete"&&!yes(page->data,"WebPageReceived")&&fs::exists(page->target()),"A download allowed to keep web page content resumes and publishes");
}
