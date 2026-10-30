#include "DB2FileLoader.h"
#include "DB2LoadInfo.h"
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>
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


int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    SceneFile file(std::string(argv[1])+"SceneScriptText.db2");DB2FileLoader texts;
    if(!texts.Load(&file,SceneScriptTextLoadInfo::Instance()))return 2;
    std::vector<std::string> terms{"spiritwatch","blessing of the eagle","100967","5762","5763","5955","5765","110821","110901","110685"};
    uint32 records=0,matches=0;
    for(uint32 i=0;i<texts.GetRecordCount();++i){auto record=texts.GetRecord(i);if(!record)continue;++records;
        std::string name=record.GetString("Name"),script=record.GetString("Script"),lower=name+'\n'+script;
        std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return char(std::tolower(c));});
        std::vector<std::string> found;for(auto const& term:terms)if(lower.find(term)!=std::string::npos)found.push_back(term);
        if(found.empty())continue;++matches;
        std::cout<<"MATCH\t"<<record.GetId()<<'\t'<<name;for(auto const& term:found)std::cout<<'\t'<<term;std::cout<<'\n';
        std::cout<<"SCRIPT_BEGIN\t"<<record.GetId()<<'\t'<<script.size()<<'\n'<<script<<"\nSCRIPT_END\t"<<record.GetId()<<'\n';
    }
    std::cout<<"COMPLETE\t"<<texts.GetRecordCount()<<'\t'<<records<<'\t'<<matches<<'\n';return 0;
}
