#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>

// Research only: enumerate matching client/hotfix users before considering a
// shared implicit-target implementation. This does not establish retail semantics.
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if(!ro||!ro->Fetch()[0].GetBool())return 4;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0))return 5;
    spells.LoadFromDB();effects.LoadFromDB();
    size_t total=0,selected=0;std::set<uint32> children;
    for(auto effect:effects)
    {
        ++total;
        if(effect->ImplicitTarget[0]!=105&&effect->ImplicitTarget[1]!=105)continue;
        ++selected;
        if(effect->EffectTriggerSpell)children.insert(effect->EffectTriggerSpell);
        auto spell=spells.LookupEntry(effect->SpellID);
        std::cout<<"TARGET105\t"<<effect->SpellID<<'\t'<<effect->DifficultyID<<'\t'<<effect->EffectIndex<<'\t'
            <<effect->Effect<<'\t'<<effect->EffectTriggerSpell<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]
            <<'\t'<<(spell?spell->Name->Str[0]:"<missing>")<<'\n';
    }
    for(auto effect:effects)if(children.count(effect->SpellID))
        std::cout<<"CHILD\t"<<effect->SpellID<<'\t'<<effect->DifficultyID<<'\t'<<effect->EffectIndex<<'\t'
            <<effect->Effect<<'\t'<<effect->EffectTriggerSpell<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
    HotfixDatabase.Close();
    if(!selected||!children.count(217820)||!children.count(202481))return 6;
    std::cout<<"EOF\tArtifactOpenTarget105Catalog\t"<<total<<'\t'<<selected<<'\t'<<children.size()<<'\n';
    return 0;
}
