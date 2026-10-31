#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <algorithm>
#include <cctype>

int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    std::string error;
    auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
    if (!config || !sConfigMgr->LoadInitial(config, {}, error)) return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements()) return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!check || !check->Fetch()[0].GetBool()) return 4;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0))return 5;
    spells.LoadFromDB();effects.LoadFromDB();
    std::set<uint32> selected{209919,209707,209934,209983,209132,209743,209684,209685};
    for(auto spell:spells){std::string name=spell->Name->Str[0];std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return std::tolower(c);});
        bool found=name.find("grash")!=std::string::npos || name.find("zithreen")!=std::string::npos || name.find("sharas")!=std::string::npos;
        if(spell->ID>=208000 && spell->ID<211000)
            for(auto term:{"smash","crush","escape","bubble","defeat","surrender","execute"})found=found||name.find(term)!=std::string::npos;
        if(found)selected.insert(spell->ID);
    }
    for(auto effect:effects)if(effect->EffectMiscValue[0]==104856 ||
        (effect->Effect==219 && (effect->EffectMiscValue[0]==1906 || effect->EffectMiscValue[0]==1923)))selected.insert(effect->SpellID);
    for(bool changed=true;changed;){changed=false;for(auto effect:effects)if(selected.count(effect->SpellID)&&effect->EffectTriggerSpell)changed=selected.insert(effect->EffectTriggerSpell).second||changed;}
    for(auto id:selected){auto spell=spells.LookupEntry(id);if(!spell){std::cout<<"MISSING\t"<<id<<'\n';continue;}
        std::cout<<"SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';
        for(auto effect:effects)if(effect->SpellID==id)
            std::cout<<"EFFECT\t"<<id<<'\t'<<unsigned(effect->DifficultyID)<<'\t'<<unsigned(effect->EffectIndex)<<'\t'<<effect->Effect<<'\t'<<effect->EffectAura<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectMiscValue[1]<<'\t'<<effect->EffectTriggerSpell<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
    }
    HotfixDatabase.Close();MySQL::Library_End();
}
