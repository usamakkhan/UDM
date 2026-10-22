#include "Core.hpp"
#include "Id3v2.hpp"
#include <iostream>
int main(int argc,char** argv){if(argc!=2)return 2;const auto m=udm::readAudioMetadata(udm::wide(argv[1]));std::cout<<udm::Json{{"title",m.title},{"artist",m.artist},{"album",m.album},{"source",m.source}}.dump();}
