#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!check||!check->Fetch()[0].GetBool())return 4;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SummonPropertiesEntry> properties("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!properties.Load(argv[1],0))return 5;
    spells.LoadFromDB();effects.LoadFromDB();properties.LoadFromDB();
    std::set<uint32> selected={197344,197388,211145,211146,211147,211148};
    for(auto id:selected){auto spell=spells.LookupEntry(id);if(!spell)return 6;std::cout<<"SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';}
    for(auto effect:effects)if(!effect->DifficultyID&&selected.count(effect->SpellID)&&effect->Effect==28)
        std::cout<<"SUMMON\t"<<effect->SpellID<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectMiscValue[1]<<'\n';
    auto summon=properties.LookupEntry(3725);if(!summon)return 7;
    std::cout<<"PROPERTIES\t"<<summon->ID<<'\t'<<summon->Control<<'\t'<<summon->Title<<'\t'<<summon->Slot<<'\t'<<summon->Flags<<'\n';
#ifdef HATI_REACTION_CATALOG
    for(auto effect:effects)if(!effect->DifficultyID&&effect->SpellID==197344)
        std::cout<<"BOND_EFFECT\t"<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'<<effect->EffectAura<<'\t'<<effect->ImplicitTarget[0]<<'\n';
    WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);
    if(WorldDatabase.Open()||!WorldDatabase.PrepareStatements())return 8;
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly=WorldDatabase.Query("SELECT @@session.transaction_read_only");if(!readOnly||!readOnly->Fetch()[0].GetBool())return 9;
    auto binding=WorldDatabase.Query("SELECT spell_id,ScriptName FROM spell_script_names WHERE spell_id=197344 ORDER BY ScriptName");
    if(!binding)return 10;
    do{auto row=binding->Fetch();std::cout<<"BINDING\t"<<row[0].GetUInt32()<<'\t'<<row[1].GetString()<<'\n';}while(binding->NextRow());
    auto actors=WorldDatabase.Query("SELECT entry,AIName,ScriptName FROM creature_template WHERE entry IN(100324,106548,106549,106550,106551) ORDER BY entry");
    if(!actors)return 11;
    do{auto row=actors->Fetch();std::cout<<"ACTOR\t"<<row[0].GetUInt32()<<'\t'<<row[1].GetString()<<'\t'<<row[2].GetString()<<'\n';}while(actors->NextRow());
    std::cout<<"COMPLETE\tHatiReaction\n";
#endif
}
