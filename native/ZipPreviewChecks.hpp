#pragma once
static void remoteZipModelChecks(const udm::fs::path& root){
 using namespace udm;Json data={{"Url","https://example.test/archive.zip"},{"FileName","archive.zip"}};
 check(canPreviewZip(data),"A ZIP filename offers Preview before the transfer starts");data["Status"]="Downloading";data["Received"]=200;check(canPreviewZip(data),"ZIP preview remains eligible during File Info prefetch");
 data["Url"]="ftp://example.test/download";data["FileName"]="download";check(canPreviewZip(data,"Application/ZIP; charset=utf-8"),"FTP and extensionless ZIP metadata enable Preview");check(!canPreviewZip(data),"An unrelated filename without ZIP metadata does not show Preview");
 data["FileName"]="archive.zip";for(auto field:{"ProtectedRequest","SourceUrl","ProtectedAdaptive","DuplicateOf"}){auto bad=data;bad[field]="captured";check(!canPreviewZip(bad),"ZIP preview cannot replay a form or captured media operation");}
 for(auto field:{"RequiresRequestCapture","RequiresMediaCapture","RequiresBrowserSessionCapture"}){auto bad=data;bad[field]=true;check(!canPreviewZip(bad),"ZIP preview respects browser recapture requirements");}
 Bytes empty(22);empty[0]='P';empty[1]='K';empty[2]=5;empty[3]=6;ZipSource source;source.length=empty.size();source.read=[&](uint64_t start,size_t count){return Bytes(empty.begin()+(size_t)start,empty.begin()+(size_t)start+count);};
 bool verified=false;source.verify=[&]{verified=true;};check(zipContents(source,Cancel()).empty()&&verified,"Empty remote ZIP parses and verifies its source");
 empty[8]=1;rejects([&]{zipContents(source,Cancel());},"Split or contradictory ZIP entry counts are rejected");empty[8]=0;empty[10]=1;empty[8]=1;rejects([&]{zipContents(source,Cancel());},"ZIP entry count cannot exceed central directory bounds");empty[8]=empty[10]=0;
 empty[12]=23;rejects([&]{zipContents(source,Cancel());},"Directory cannot overlap its own footer");empty[12]=0;empty[16]=255;rejects([&]{zipContents(source,Cancel());},"Directory offsets beyond the archive are rejected");empty[16]=0;
 Cancel stopped;stopped.stop=true;rejects([&]{zipContents(source,stopped);},"ZIP parsing obeys cancellation before any reads");
 source.read=[](uint64_t,size_t count){return Bytes(count?count-1:0);};rejects([&]{zipContents(source,Cancel());},"Short remote reads are never treated as a directory");
 check(zipNameCrc("123456789")==0xcbf43926u,"Unicode ZIP filename metadata uses the standard CRC32 polynomial");
 (void)root;
}
