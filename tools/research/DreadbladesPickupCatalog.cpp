#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
int main(int argc,char** argv){
 if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
 if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
 if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 auto readOnly=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!readOnly||!readOnly->Fetch()[0].GetBool())return 4;
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
 DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
 DB2Storage<ArtifactEntry> artifacts("Artifact.db2",ArtifactLoadInfo::Instance());
 DB2Storage<ItemChildEquipmentEntry> children("ItemChildEquipment.db2",ItemChildEquipmentLoadInfo::Instance());
 if(!effects.Load(argv[1],0)||!spells.Load(argv[1],0)||!misc.Load(argv[1],0)||!items.Load(argv[1],0)||!artifacts.Load(argv[1],0)||!durations.Load(argv[1],0)||!children.Load(argv[1],0))return 5;
 effects.LoadFromDB();spells.LoadFromDB();misc.LoadFromDB();items.LoadFromDB();artifacts.LoadFromDB();durations.LoadFromDB();children.LoadFromDB();
 std::set<uint32> selected{202040,202182};
 for(int depth=0;depth<3;++depth)for(auto effect:effects)if(selected.count(effect->SpellID)&&effect->EffectTriggerSpell)selected.insert(effect->EffectTriggerSpell);
 for(auto id:selected)if(auto spell=spells.LookupEntry(id))std::cout<<"SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';
 for(auto effect:effects)if(!effect->DifficultyID&&(selected.count(effect->SpellID)||(effect->Effect==157||effect->Effect==249)&&effect->EffectMiscValue[0]==128872))std::cout<<"EFFECT\t"<<effect->SpellID<<'\t'<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'<<effect->EffectAura<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectTriggerSpell<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
 for(auto entry:misc)if(!entry->DifficultyID&&selected.count(entry->SpellID))std::cout<<"MISC\t"<<entry->SpellID<<'\t'<<entry->DurationIndex<<'\t'<<entry->CastingTimeIndex<<'\t'<<entry->Speed<<'\t'<<entry->LaunchDelay<<'\n';
 for(auto item:items)if(item->ID==128872||item->ID==134552)std::cout<<"ITEM\t"<<item->ID<<'\t'<<unsigned(item->InventoryType)<<'\t'<<unsigned(item->ArtifactID)<<'\n';
 for(auto entry:children)if(entry->ItemID==128872)std::cout<<"CHILD\t"<<entry->ItemID<<'\t'<<entry->ChildItemID<<'\t'<<unsigned(entry->ChildItemEquipSlot)<<'\n';
 for(auto entry:misc)if(!entry->DifficultyID&&selected.count(entry->SpellID))if(auto duration=durations.LookupEntry(entry->DurationIndex))std::cout<<"DURATION\t"<<entry->SpellID<<'\t'<<duration->Duration<<'\t'<<duration->MaxDuration<<'\n';
 for(auto effect:effects)if(!effect->DifficultyID&&selected.count(effect->SpellID))std::cout<<"PERIOD\t"<<effect->SpellID<<'\t'<<effect->EffectIndex<<'\t'<<effect->EffectAuraPeriod<<'\n';
}
