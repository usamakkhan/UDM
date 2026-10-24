#pragma once
// Opt-in: runs content recognition over the public libmagic test corpus
// (https://github.com/file/file/tree/master/tests, fetched by tools/fetch-libmagic-corpus.py)
// and compares each file with the verdict UDM's own table should give it.
// Udm.NativeTests.exe --sniff-corpus <folder with the .testfile files>
#include "ContentSniff.hpp"
#include <fstream>
static void contentSniffCorpusChecks(const udm::fs::path& corpus){
 using namespace udm;
 struct Expectation{const char* stem;SniffVerdict verdict;const char* extension;const char* note;};
 // "" extension with Match means the family is recognized without one specific extension (ELF, generic compound document).
 const std::vector<Expectation> expectations={
  {"Animated_PNG_example_bouncing_beach_ball",SniffVerdict::Match,"png","Animated PNG"},{"apng-ffmpeg",SniffVerdict::Match,"png","Animated PNG"},
  {"arj",SniffVerdict::Match,"arj",""},{"cl8m8ocofedso",SniffVerdict::Match,"mp3",""},{"JW07022A.mp3",SniffVerdict::Match,"mp3",""},
  {"dsd64-dsf",SniffVerdict::Match,"dsf",""},{"dsd64-dff",SniffVerdict::Match,"dff",""},{"escapevel",SniffVerdict::Match,"zip","ZIP with a custom MIME"},
  {"gpkg-1-zst",SniffVerdict::Match,"tar","Gentoo binary package is a tar"},{"HWP2016.hwp",SniffVerdict::Match,"hwp",""},{"HWP2016.hwpx.zip",SniffVerdict::Match,"hwpx",""},{"HWP97.hwp",SniffVerdict::Match,"hwp",""},
  {"issue311docx",SniffVerdict::Match,"docx",""},{"issue359xlsx",SniffVerdict::Match,"xlsx",""},{"keyman-0",SniffVerdict::Match,"kmx",""},{"keyman-1",SniffVerdict::Match,"kmx",""},{"keyman-2",SniffVerdict::Match,"zip","Keyman package is a ZIP"},
  {"matilde.arm",SniffVerdict::Match,"amr",""},{"pnm1",SniffVerdict::Match,"pgm",""},{"pnm2",SniffVerdict::Match,"pgm",""},{"pnm3",SniffVerdict::Match,"ppm",""},
  {"rpm-v3.0-bin-aarch64",SniffVerdict::Match,"rpm",""},{"rpm-v3.0-bin-powerpc64",SniffVerdict::Match,"rpm",""},{"rpm-v3.0-bin-s390x",SniffVerdict::Match,"rpm",""},{"rpm-v3.0-bin-x86_64",SniffVerdict::Match,"rpm",""},{"rpm-v3.0-src",SniffVerdict::Match,"rpm",""},
  {"sh-other-be",SniffVerdict::Match,"","ELF"},{"sh-other-le",SniffVerdict::Match,"","ELF"},{"sh4a-extra-be",SniffVerdict::Match,"","ELF"},{"sh4a-extra-le",SniffVerdict::Match,"","ELF"},{"sh4a-nofpu-be",SniffVerdict::Match,"","ELF"},{"sh4a-nofpu-le",SniffVerdict::Match,"","ELF"},
  {"zstd-v0.2-FF",SniffVerdict::Match,"zst","legacy frame"},{"zstd-v0.3-FF",SniffVerdict::Match,"zst","legacy frame"},{"zstd-v0.4-FF",SniffVerdict::Match,"zst","legacy frame"},{"zstd-v0.5-FF",SniffVerdict::Match,"zst","legacy frame"},{"zstd-v0.6-FF",SniffVerdict::Match,"zst","legacy frame"},{"zstd-v0.7-21",SniffVerdict::Match,"zst","legacy frame"},{"zstd-v0.7-22",SniffVerdict::Match,"zst","legacy frame"},
  {"zstd-v0.8-01",SniffVerdict::Match,"zst",""},{"zstd-v0.8-02",SniffVerdict::Match,"zst",""},{"zstd-v0.8-03",SniffVerdict::Match,"zst",""},{"zstd-v0.8-16",SniffVerdict::Match,"zst",""},{"zstd-v0.8-20",SniffVerdict::Match,"zst",""},{"zstd-v0.8-21",SniffVerdict::Match,"zst",""},{"zstd-v0.8-22",SniffVerdict::Match,"zst",""},{"zstd-v0.8-23",SniffVerdict::Match,"zst",""},{"zstd-v0.8-F4",SniffVerdict::Match,"zst",""},{"zstd-v0.8-FF",SniffVerdict::Match,"zst",""},
  {"uf2",SniffVerdict::Match,"uf2",""},{"bgcode",SniffVerdict::Match,"bgcode",""},{"registry-pol",SniffVerdict::Match,"pol",""},{"fit-map-data",SniffVerdict::Match,"fit",""},{"gedcom",SniffVerdict::Match,"ged",""},
  {"android-vdex-1",SniffVerdict::Match,"vdex",""},{"android-vdex-2",SniffVerdict::Match,"vdex",""},{"xclbin",SniffVerdict::Match,"xclbin",""},{"xilinx",SniffVerdict::Match,"bit",""},
  {"tarantool-xlog",SniffVerdict::Match,"xlog",""},{"tarantool-snap",SniffVerdict::Match,"snap",""},{"ext4",SniffVerdict::Match,"img","superblock magic at 0x438"},{"pdf-utf8-bom",SniffVerdict::Match,"pdf","BOM before %PDF"},{"utf16xmlsvg",SniffVerdict::Match,"svg","UTF-16"},
  {"pgp-binary-key-v2-phil",SniffVerdict::Match,"pgp",""},{"pgp-binary-key-v3-lutz",SniffVerdict::Match,"pgp",""},{"pgp-binary-key-v4-dsa",SniffVerdict::Match,"pgp",""},{"pgp-binary-key-v4-ecc-no-userid-secret",SniffVerdict::Match,"pgp",""},
  {"pgp-binary-key-v4-ecc-secret-key",SniffVerdict::Match,"pgp",""},{"pgp-binary-key-v4-rsa-key",SniffVerdict::Match,"pgp",""},{"pgp-binary-key-v4-rsa-no-userid-secret",SniffVerdict::Match,"pgp",""},{"pgp-binary-key-v4-rsa-secret-key",SniffVerdict::Match,"pgp",""},
  // Text that stays text: scripts, JSON, XML without a signature of its own, a Python snippet that merely mentions %PDF.
  {"cmd1",SniffVerdict::Text,"txt",""},{"cmd2",SniffVerdict::Text,"txt",""},{"json1",SniffVerdict::Text,"txt",""},{"json2",SniffVerdict::Text,"txt",""},{"json3",SniffVerdict::Text,"txt",""},{"json4",SniffVerdict::Text,"txt",""},
  {"json5",SniffVerdict::Text,"txt",""},{"json6",SniffVerdict::Text,"txt",""},{"json7",SniffVerdict::Text,"txt",""},{"json8",SniffVerdict::Text,"txt",""},{"jsonlines1",SniffVerdict::Text,"txt",""},{"multiple",SniffVerdict::Text,"txt",""},
  {"osm",SniffVerdict::Text,"txt",""},{"regex-eol",SniffVerdict::Text,"txt",""},{"searchbug",SniffVerdict::Text,"txt",""},{"jpeg-text",SniffVerdict::Text,"txt",""},{"tarantool-not",SniffVerdict::Text,"txt",""},{"pdf-no-version",SniffVerdict::Text,"txt","a script, not a PDF"},
  // Deliberately unrecognized: no public signature UDM needs for downloads. These guard against accidental false matches.
  {"bcachefs",SniffVerdict::None,"",""},{"bcachefs2",SniffVerdict::None,"",""},{"cmd3",SniffVerdict::None,"",""},{"cmd4",SniffVerdict::None,"",""},{"CVE-2014-1943",SniffVerdict::None,"",""},{"efi-signature-list-sha256",SniffVerdict::None,"",""},
  {"hddrawcopytool",SniffVerdict::None,"",""},{"hello-racket_rkt",SniffVerdict::None,"",""},{"pcjr",SniffVerdict::None,"",""},{"rtfd",SniffVerdict::None,"",""},{"tarantool-run",SniffVerdict::None,"",""},{"tarantool-index",SniffVerdict::None,"",""},{"tarantool-vylog",SniffVerdict::None,"",""},{"ascii-escape-so-si",SniffVerdict::None,"",""},
 };
 size_t seen=0;
 for(const auto& e:expectations){
  const auto path=corpus/(std::string(e.stem)+".testfile");std::ifstream in(path,std::ios::binary);
  if(!in){check(false,std::string("libmagic corpus file present: ")+e.stem);continue;}
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),{});const size_t size=std::min(data.size(),contentSniffReadLimit);
  auto r=sniffContent(data.data(),size,data.size()==size);++seen;
  const bool ok=r.verdict==e.verdict&&(e.verdict==SniffVerdict::None||r.extension==e.extension);
  check(ok,std::string("libmagic corpus: ")+e.stem+" -> "+(e.verdict==SniffVerdict::None?"unrecognized":e.verdict==SniffVerdict::Text?"text":std::string("'")+e.extension+"'")+(ok?"":std::string(" (got ")+sniffVerdictName(r.verdict)+" '"+r.extension+"' "+r.description+")"));
 }
 std::cout<<"libmagic corpus: "<<seen<<" files classified"<<std::endl;
}
