#include "AudioMetadata.hpp"
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("Fresh test directory required");
 auto root=std::filesystem::path(argv[1]);if(std::filesystem::exists(root))throw std::runtime_error("Fixture exists");std::filesystem::create_directories(root);
 auto path=root/"audio.mp3";int checks=0;auto check=[&](bool value){if(!value)throw std::runtime_error("Metadata check "+std::to_string(checks));++checks;};
 auto write=[&](const std::string& data){std::ofstream out(path,std::ios::binary);out.write(data.data(),data.size());};
 check(udm::readId3v1(path).title.empty());write("TAGshort");check(udm::readId3v1(path).title.empty());
 std::string tag(128,'\0');tag.replace(0,3,"TAG");tag.replace(3,9,"Song name");tag.replace(33,6,"Artist");tag.replace(63,5,"Album");write(std::string(1000,'x')+tag);
 auto metadata=udm::readId3v1(path);check(metadata.title=="Song name");check(metadata.artist=="Artist");check(metadata.album=="Album");
 tag.replace(3,30,std::string(30,'a'));write(tag);check(udm::readId3v1(path).title==std::string(30,'a'));
 tag.replace(3,30,std::string(30,' '));tag[3]='A';write(tag);check(udm::readId3v1(path).title=="A");
 tag[3]=char(0xe9);tag[4]='\n';write(tag);check(udm::readId3v1(path).title=="\xc3\xa9");
 tag[3]='\0';tag[4]='x';write(tag);check(udm::readId3v1(path).title.empty());
 tag[0]='X';write(tag);check(udm::readId3v1(path).artist.empty());
 tag[0]='T';write(tag+"trailing");check(udm::readId3v1(path).album.empty());
 std::cout<<checks<<" passed, 0 failed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
