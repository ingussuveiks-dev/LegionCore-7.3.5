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
    DB2Storage<ItemEffectEntry> items("ItemEffect.db2",ItemEffectLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellTargetRestrictionsEntry> targets("SpellTargetRestrictions.db2",SpellTargetRestrictionsLoadInfo::Instance());
    DB2Storage<QuestObjectiveEntry> objectives("QuestObjective.db2",QuestObjectiveLoadInfo::Instance());
    if(!items.Load(argv[1],0)||!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!targets.Load(argv[1],0)||!objectives.Load(argv[1],0))return 5;
    items.LoadFromDB();spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();targets.LoadFromDB();objectives.LoadFromDB();
    DB2Storage<SummonPropertiesEntry> summons("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    if(!summons.Load(argv[1],0)||!durations.Load(argv[1],0))return 6;
    summons.LoadFromDB();durations.LoadFromDB();
    std::set<uint32> selected{178752,178753,189764};
    for(auto item:items)if(item->ItemID==122100){selected.insert(item->SpellID);std::cout<<"ITEM\t"<<item->ItemID<<'\t'<<item->SpellID<<'\t'<<unsigned(item->TriggerType)<<'\n';}
    for(int depth=0;depth<3;++depth)for(auto e:effects)if(selected.count(e->SpellID)&&e->EffectTriggerSpell)selected.insert(e->EffectTriggerSpell);
    for(auto id:selected){if(auto spell=spells.LookupEntry(id))std::cout<<"SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\t'<<spell->Description->Str[0]<<'\n';}
    for(auto e:effects)if(!e->DifficultyID&&(selected.count(e->SpellID)||(e->Effect==16&&e->EffectMiscValue[0]==38834)))
        std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';
    for(auto e:misc)if(!e->DifficultyID&&selected.count(e->SpellID)){std::cout<<"MISC\t"<<e->SpellID<<'\t'<<e->DurationIndex<<'\t'<<e->RangeIndex;for(auto attr:e->Attributes)std::cout<<'\t'<<attr;std::cout<<'\n';}
    for(auto e:targets)if(!e->DifficultyID&&selected.count(e->SpellID))std::cout<<"TARGET\t"<<e->SpellID<<'\t'<<e->Targets<<'\t'<<e->TargetCreatureType<<'\t'<<unsigned(e->MaxTargets)<<'\n';
    for(auto e:objectives)if(e->QuestID==37653||e->QuestID==38834)std::cout<<"OBJECTIVE\t"<<e->QuestID<<'\t'<<e->ID<<'\t'<<unsigned(e->Type)<<'\t'<<e->ObjectID<<'\t'<<e->Amount<<'\n';
    if(auto e=summons.LookupEntry(2990))std::cout<<"SUMMON\t"<<e->ID<<'\t'<<e->Control<<'\t'<<e->Title<<'\t'<<e->Flags<<'\n';
    if(auto e=durations.LookupEntry(28))std::cout<<"DURATION\t"<<e->ID<<'\t'<<e->Duration<<'\t'<<e->DurationPerLevel<<'\t'<<e->MaxDuration<<'\n';
}
