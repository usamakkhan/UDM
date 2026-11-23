from pathlib import Path
root=Path('<repo>');p=root/'native/MediaPublication.hpp';s=p.read_text(encoding='utf-8-sig');s=s.replace('#include "PublicationRecovery.hpp"','#include "PublicationRecovery.hpp"\n#include "ScopedPublicationSource.hpp"',1);a=s.index(' cancel->check();requirePublicationRecovered(manager,job);',s.index('inline void publishMediaOutput'));b=s.index('\ntemplate<class Binding>',a);new=''' cancel->check();requirePublicationRecovered(manager,job);
 auto sourcePin=PublicationSourcePin::openRead(source);if(!sourcePin)throw std::runtime_error("Cannot keep the verified file stable for publication. Its source was retained.");
 CreatedStagingFile sourceProof;sourcePin->captureCleanup(sourceProof);
 const auto folder=job->target().parent_path();
 if(folder.lexically_normal()==source.parent_path().lexically_normal()){
  ScopedPublicationSource handoff(manager,job,source,std::move(sourcePin));manager.publishFile(job,source,digest,cancel.get(),live);return;
 }
 fs::create_directories(folder);
 const auto staging=folder/(L".udm-"+wide(job->id())+L"-"+wide(guid())+L".publishing");
 bool ownsStaging=false;CreatedStagingFile stageProof;
 try{
  ownsStaging=CreateHardLinkW(staging.c_str(),source.c_str(),nullptr)!=FALSE;
  std::unique_ptr<PublicationSourcePin> publicationPin;
  if(ownsStaging){stageProof=sourceProof;publicationPin=std::move(sourcePin);}
  else{
   auto input=sourcePin->duplicate();if(!input)throw std::runtime_error("Cannot read the verified media file.");
   {
    Handle output(CreateFileW(staging.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_FLAG_SEQUENTIAL_SCAN,nullptr));
    if(!output)throw std::runtime_error("Cannot prepare media in the selected folder.");
    ownsStaging=true;stageProof.capture(output.h);
    std::vector<char> bytes(1024*1024);DWORD count=0;
    for(;;){cancel->check();if(!ReadFile(input.h,bytes.data(),(DWORD)bytes.size(),&count,nullptr))throw std::runtime_error("Cannot read the verified media file.");if(!count)break;
     DWORD written=0;if(!WriteFile(output.h,bytes.data(),count,&written,nullptr)||written!=count)throw std::runtime_error("Cannot copy media to the selected folder.");}
    cancel->check();if(!FlushFileBuffers(output.h))throw std::runtime_error("Cannot flush media in the selected folder.");
   }
   // Move the existing one-pass copy verification onto a strict read pin after
   // the writer closes. Its identity must still match the newly created file.
   publicationPin=PublicationSourcePin::openRead(staging);
   if(!publicationPin||!publicationPin->matches(stageProof))throw std::runtime_error("The copied media source changed. Its source was retained.");
   if(publicationPin->hash([&]{cancel->check();})!=digest)throw std::runtime_error("The copied media failed verification. Source tracks were retained.");
  }
  cancel->check();ScopedPublicationSource handoff(manager,job,staging,std::move(publicationPin));manager.publishFile(job,staging,digest,cancel.get(),live);
 }catch(...){if(ownsStaging&&!publicationNeedsRecovery(manager,job))stageProof.removeOwned(staging);throw;}
 sourcePin.reset();sourceProof.removeOwned(source);
}
''';s=s[:a]+new+s[b:];p.write_text(s,encoding='utf-8')
p=root/'native/Duplicates.cpp';s=p.read_text(encoding='utf-8-sig');s='#include "ScopedPublicationSource.hpp"\n'+s;old=' const bool overlap=finalCommit&&finalCommit->pendingWriteback()&&finalCommit->hasWritebackPin()&&originalId.empty()&&!knownHash.empty();\n std::unique_ptr<PublicationSourcePin> bodyPin;if(overlap)bodyPin=finalCommit->takeWritebackPin();';new=' auto bodyPin=ScopedPublicationSource::take(*this,job,staging);const bool suppliedSource=bool(bodyPin);\n const bool overlap=!suppliedSource&&finalCommit&&finalCommit->pendingWriteback()&&finalCommit->hasWritebackPin()&&originalId.empty()&&!knownHash.empty();\n if(overlap)bodyPin=finalCommit->takeWritebackPin();';assert old in s;s=s.replace(old,new);s=s.replace('Json stagedIdentity;if(overlap)stagedIdentity=prepared.sourceIdentity();','Json stagedIdentity;if(overlap||suppliedSource)stagedIdentity=prepared.sourceIdentity();');p.write_text(s,encoding='utf-8')
p=root/'native/Version.hpp';s=p.read_text(encoding='utf-8-sig').replace('9,69,0,0','9,70,0,0').replace('9.69','9.70');p.write_text(s,encoding='utf-8')
for name in ['browser/chromium/manifest.json','browser/firefox/manifest.json']:
 p=root/name;s=p.read_text(encoding='utf-8-sig').replace('"version": "0.73.33"','"version": "0.73.34"');p.write_text(s,encoding='utf-8')
p=root/'native/backend-tests.json';s=p.read_text(encoding='utf-8-sig');p.write_text(s.replace('[\n','[\n  {\n    "source": "RelocationOwnershipTests",\n    "timeoutSeconds": 90\n  },\n',1),encoding='utf-8')
base=Path.home()/'AppData/Local/UDM-Development';s=(base/'build-gzip-stage-ownership-9.69-private.py').read_text(encoding='utf-8-sig').replace('gzip-stage-ownership','relocation-ownership').replace('9.69','9.70').replace('0.73.33','0.73.34').replace("root/'.media-cache/owned-transfer-stage-cleanup-build-9.68/build-result.json'","root/'.media-cache/owned-gzip-stage-ownership-build-9.69/build-result.json'");s=s.replace("'native/GzipStageOwnershipTests.cpp'","'native/RelocationOwnershipTests.cpp','native/ScopedPublicationSource.hpp','native/GzipStageOwnershipTests.cpp'").replace("tests=['GzipStageOwnershipTests'","tests=['RelocationOwnershipTests','GzipStageOwnershipTests'").replace("baseline='9.68 captured source inventory'","baseline='9.69 captured source inventory'");(base/'build-relocation-ownership-9.70-private.py').write_text(s,encoding='utf-8')
