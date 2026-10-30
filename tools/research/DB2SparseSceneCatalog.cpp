#include "DB2FileLoader.h"
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
struct SceneFile : DB2FileSource
{
    std::string name;
    FILE* file;
    explicit SceneFile(std::string path) : name(std::move(path)), file(std::fopen(name.c_str(), "rb")) { }
    ~SceneFile() override { if (file) std::fclose(file); }
    bool IsOpen() const override { return file != nullptr; }
    bool Read(void* buffer, std::size_t size) override { return std::fread(buffer, size, 1, file) == 1; }
    std::size_t GetPosition() const override { return std::ftell(file); }
    std::size_t GetFileSize() const override { return std::filesystem::file_size(name); }
    char const* GetFileName() const override { return name.c_str(); }
};



#pragma pack(push,1)
struct SceneCatalogEntry { uint32 Offset; uint16 Size; };
#pragma pack(pop)
template<class Entry> bool CheckPlainTable(char const* directory,char const* name,DB2LoadInfo const* info)
{
    SceneFile source(std::string(directory)+name);DB2Header header{};
    if(!source.Read(&header,sizeof(header))||std::fseek(source.file,0,SEEK_SET))return false;
    DB2FileLoader raw;DB2Storage<Entry> typed(name,info);
    if(!raw.Load(&source,info)||!typed.Load(directory,0))return false;
    uint32 count=0;
    for(uint32 i=0;i<raw.GetRecordCount();++i){auto r=raw.GetRecord(i);if(!r)continue;
        auto t=typed.LookupEntry(r.GetId());if(!t||std::strcmp(t->Name,r.GetString("Name")))return false;++count;
    }
    std::cout<<"NATIVE_PLAIN\t"<<name<<'\t'<<header.Flags<<'\t'<<count<<'\n';return count!=0;
}
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string path=std::string(argv[1])+"SceneScriptText.db2";SceneFile source(path);
    DB2Header header{};if(!source.Read(&header,sizeof(header)))return 2;
    if(header.Signature!=0x31434457||!(header.Flags&1))return 3;
    std::vector<SceneCatalogEntry> catalog(header.MaxId-header.MinId+1);
    if(std::fseek(source.file,header.CatalogDataOffset,SEEK_SET)||!source.Read(catalog.data(),catalog.size()*sizeof(SceneCatalogEntry)))return 4;
    std::fseek(source.file,0,SEEK_SET);DB2FileLoader raw;if(!raw.Load(&source,SceneScriptTextLoadInfo::Instance()))return 5;
    uint64 stored=0,actual=0;uint32 records=0,paddedRecords=0;std::set<uint32> offsets;
    for(uint32 i=0;i<raw.GetRecordCount();++i){auto r=raw.GetRecord(i);if(!r)continue;++records;
        auto bytes=std::strlen(r.GetString("Name"))+std::strlen(r.GetString("Script"))+2;
        stored+=catalog[i].Size;actual+=bytes;offsets.insert(catalog[i].Offset);
        if(bytes!=catalog[i].Size)++paddedRecords;
    }
    std::cout<<"MEASURE\t"<<records<<'\t'<<stored<<'\t'<<actual<<'\t'<<paddedRecords<<'\n';
    std::cout<<"ALIASES\t"<<records-offsets.size()<<'\t'<<header.CatalogDataOffset-sizeof(DB2Header)-header.FieldCount*4<<'\n'<<std::flush;
    if(!std::getenv("LEGION_SCENE_BULK_TEST"))return 0;
    DB2Storage<SceneScriptTextEntry> typed("SceneScriptText.db2",SceneScriptTextLoadInfo::Instance());
    std::cout<<"BULK_BEGIN\n"<<std::flush;if(!typed.Load(argv[1],0))return 6;
    uint32 matched=0;
    for(unsigned pass=0;pass<2;++pass){
        if(pass&&!typed.LoadStringsFrom(argv[1],0))return 9;
        matched=0;
        for(uint32 i=0;i<raw.GetRecordCount();++i){auto r=raw.GetRecord(i);if(!r)continue;
            auto t=typed.LookupEntry(r.GetId());if(!t||t->ID!=r.GetId()||std::strcmp(t->Name->Str[0],r.GetString("Name"))||std::strcmp(t->Script->Str[0],r.GetString("Script")))return 7;++matched;
        }
        uint32 iterated=0;for(auto row:typed){if(!row||!typed.LookupEntry(row->ID))return 10;++iterated;}
        if(iterated!=matched)return 11;
    }
    if(!CheckPlainTable<NameGenEntry>(argv[1],"NameGen.db2",NameGenLoadInfo::Instance())||
       !CheckPlainTable<NamesReservedEntry>(argv[1],"NamesReserved.db2",NamesReservedLoadInfo::Instance()))return 12;
    std::cout<<"COMPLETE\t"<<matched<<'\t'<<actual<<'\n';return matched==records?0:8;
}
