#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;
    auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 4;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 5;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check=HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if(!check||!check->Fetch()[0].GetBool())return 6;
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<PlayerConditionEntry> conditions("PlayerCondition.db2",PlayerConditionLoadInfo::Instance());
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
    DB2Storage<MapEntry> maps("Map.db2",MapLoadInfo::Instance());
    if(!effects.Load(argv[1],0)||!conditions.Load(argv[1],0)||!items.Load(argv[1],0)||!maps.Load(argv[1],0))return 2;
    effects.LoadFromDB();conditions.LoadFromDB();items.LoadFromDB();maps.LoadFromDB();
    for(uint32 spellId:{180850u,181257u,181511u,199827u,186987u,192145u,190777u,225875u})
        for(auto effect:effects)if(effect->SpellID==spellId)
            std::cout<<"EFFECT\t"<<spellId<<'\t'<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'<<effect->EffectAura<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectTriggerSpell<<'\t'<<effect->EffectItemType<<'\n';
    auto condition=conditions.LookupEntry(32543);auto item=items.LookupEntry(120978);
    if(condition)std::cout<<"CONDITION\t32543\t"<<condition->ClassMask<<'\t'<<condition->MinLevel<<'\t'<<condition->MaxLevel<<'\t'<<condition->CurrQuestID[0]<<'\t'<<condition->PrevQuestID[0]<<'\n';
    else std::cout<<"CONDITION_MISSING\t32543\n";
    if(item)std::cout<<"ITEM\t120978\t"<<unsigned(item->ArtifactID)<<'\t'<<unsigned(item->InventoryType)<<'\t'<<item->Display->Str[0]<<'\n';
    else std::cout<<"ITEM_MISSING\t120978\n";
    auto map=maps.LookupEntry(1500);if(!map)return 7;
    std::cout<<"MAP\t1500\t"<<map->Instanceable()<<'\t'<<map->IsDungeon()<<'\n';
}
