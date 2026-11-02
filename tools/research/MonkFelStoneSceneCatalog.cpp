#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <set>
#include <string>
#include <vector>

int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<SceneScriptEntry> scripts("SceneScript.db2",SceneScriptLoadInfo::Instance());
    DB2Storage<SceneScriptPackageMemberEntry> members("SceneScriptPackageMember.db2",SceneScriptPackageMemberLoadInfo::Instance());
    DB2Storage<SceneScriptTextEntry> texts("SceneScriptText.db2",SceneScriptTextLoadInfo::Instance());
    if(!scripts.Load(argv[1],0)||!members.Load(argv[1],0)||!texts.Load(argv[1],0))return 2;
    std::set<uint32> pendingPackages{1492},seenPackages,pendingScripts,seenScripts,emitted;
    while(!pendingPackages.empty())
    {
        auto id=*pendingPackages.begin();pendingPackages.erase(id);if(!seenPackages.insert(id).second)continue;
        for(auto member:members)if(member->SceneScriptPackageID==id)
        {
            std::cout<<"MEMBER\t"<<member->ID<<'\t'<<id<<'\t'<<member->ChildSceneScriptPackageID<<'\t'<<member->SceneScriptID<<'\n';
            if(member->ChildSceneScriptPackageID)pendingPackages.insert(member->ChildSceneScriptPackageID);
            if(member->SceneScriptID)pendingScripts.insert(member->SceneScriptID);
        }
    }
    auto emit=[&](uint32 id,char const* reason)
    {
        auto text=texts.LookupEntry(id);if(!text)return false;
        if(!emitted.insert(id).second)return true;
        std::cout<<"TEXT\t"<<id<<'\t'<<reason<<'\t'<<text->Name->Str[0]<<"\nSCRIPT_BEGIN\t"<<id<<'\n'
                 <<text->Script->Str[0]<<"\nSCRIPT_END\t"<<id<<'\n';
        return true;
    };
    while(!pendingScripts.empty())
    {
        auto id=*pendingScripts.begin();pendingScripts.erase(id);if(!seenScripts.insert(id).second)continue;
        auto script=scripts.LookupEntry(id);if(!script||!emit(id,"PACKAGE1492"))return 3;
        if(script->NextSceneScriptID)pendingScripts.insert(script->NextSceneScriptID);
    }
    std::vector<std::string>const terms{"fel stone","jorvinax","193890","194141","200617","98353"};
    uint32 count=0;
    for(auto text:texts)
    {
        ++count;std::string value=std::string(text->Name->Str[0])+"\n"+text->Script->Str[0];
        std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(std::any_of(terms.begin(),terms.end(),[&](std::string const&term){return value.find(term)!=std::string::npos;}))
            if(!emit(text->ID,"RELATED_TEXT_SEARCH"))return 4;
    }
    std::cout<<"EOF\tmonk-fel-stone-scene\t"<<count<<'\t'<<seenPackages.size()<<'\t'<<seenScripts.size()<<'\t'<<emitted.size()<<'\n';
    return !seenScripts.empty()&&count==5969?0:5;
}
