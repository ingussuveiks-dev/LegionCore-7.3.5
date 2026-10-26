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
 DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());
 if(!locks.Load(argv[1],0))return 6;locks.LoadFromDB();
 for(uint32 id:{43u,1691u}){auto lock=locks.LookupEntry(id);if(!lock)return 7;for(unsigned i=0;i<8;++i)if(lock->Type[i])std::cout<<"LOCK\t"<<id<<'\t'<<i<<'\t'<<unsigned(lock->Type[i])<<'\t'<<lock->Index[i]<<'\t'<<lock->Skill[i]<<'\t'<<unsigned(lock->Action[i])<<'\n';}
 DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
 DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
 DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
 if(!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 8;
 steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();
 for(auto step:steps)if(step->ScenarioID==1085){
  std::cout<<"STEP\t"<<unsigned(step->OrderIndex)<<'\t'<<step->ID<<'\t'<<step->Criteriatreeid<<'\t'<<step->Title->Str[0]<<'\t'<<step->Description->Str[0]<<'\n';
  std::cout<<"STEP_META\t"<<unsigned(step->OrderIndex)<<'\t'<<unsigned(step->Flags)<<'\t'<<step->RewardQuestID<<'\n';
  std::set<uint32> pending{uint32(step->Criteriatreeid)},seen;
  while(!pending.empty()){auto id=*pending.begin();pending.erase(id);if(!seen.insert(id).second)continue;
   if(auto tree=trees.LookupEntry(id)){std::cout<<"TREE\t"<<unsigned(step->OrderIndex)<<'\t'<<tree->ID<<'\t'<<tree->Parent<<'\t'<<tree->Amount<<'\t'<<unsigned(tree->Operator)<<'\t'<<tree->Flags<<'\t'<<tree->CriteriaID<<'\n';if(auto c=criteria.LookupEntry(tree->CriteriaID))std::cout<<"CRITERION\t"<<unsigned(step->OrderIndex)<<'\t'<<tree->ID<<'\t'<<tree->Parent<<'\t'<<tree->Amount<<'\t'<<c->ID<<'\t'<<unsigned(c->Type)<<'\t'<<c->Asset<<'\n';
    for(auto child:trees)if(child->Parent==id)pending.insert(child->ID);}
  }
 }
 DB2Storage<LFGDungeonsEntry> lfg("LFGDungeons.db2",LfgDungeonsLoadInfo::Instance());
 if(!lfg.Load(argv[1],0))return 9;lfg.LoadFromDB();
 for(auto dungeon:lfg)if(dungeon->ScenarioID==1085)std::cout<<"LFG\t"<<dungeon->ID<<'\t'<<dungeon->MapID<<'\t'<<unsigned(dungeon->DifficultyID)<<'\t'<<dungeon->GetMinGroupSize()<<'\t'<<dungeon->GetMaxGroupSize()<<'\t'<<unsigned(dungeon->CountTank)<<'\t'<<unsigned(dungeon->CountHealer)<<'\t'<<unsigned(dungeon->CountDamage)<<'\n';
 std::set<uint32> selected{210733,210734,210818,210715,210703,210807,204237,211185,211380};
 for(int depth=0;depth<3;++depth)for(auto effect:effects)if(selected.count(effect->SpellID)&&effect->EffectTriggerSpell)selected.insert(effect->EffectTriggerSpell);
 for(auto id:selected)if(auto spell=spells.LookupEntry(id))std::cout<<"SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';else std::cout<<"MISSING_SPELL\t"<<id<<'\n';
 for(auto effect:effects)if(!effect->DifficultyID&&(selected.count(effect->SpellID)||(effect->Effect==157||effect->Effect==249)&&effect->EffectMiscValue[0]==128825))std::cout<<"EFFECT\t"<<effect->SpellID<<'\t'<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'<<effect->EffectAura<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectTriggerSpell<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
 for(auto entry:misc)if(!entry->DifficultyID&&selected.count(entry->SpellID))std::cout<<"MISC\t"<<entry->SpellID<<'\t'<<entry->DurationIndex<<'\t'<<entry->CastingTimeIndex<<'\t'<<entry->Speed<<'\t'<<entry->LaunchDelay<<'\n';
 for(auto item:items)if(item->ID==128825)std::cout<<"ITEM\t"<<item->ID<<'\t'<<unsigned(item->InventoryType)<<'\t'<<unsigned(item->ArtifactID)<<'\n';
 for(auto entry:children)if(entry->ItemID==128825)std::cout<<"CHILD\t"<<entry->ItemID<<'\t'<<entry->ChildItemID<<'\t'<<unsigned(entry->ChildItemEquipSlot)<<'\n';
 for(auto entry:misc)if(!entry->DifficultyID&&selected.count(entry->SpellID))if(auto duration=durations.LookupEntry(entry->DurationIndex))std::cout<<"DURATION\t"<<entry->SpellID<<'\t'<<duration->Duration<<'\t'<<duration->MaxDuration<<'\n';
 for(auto effect:effects)if(!effect->DifficultyID&&selected.count(effect->SpellID))std::cout<<"PERIOD\t"<<effect->SpellID<<'\t'<<effect->EffectIndex<<'\t'<<effect->EffectAuraPeriod<<'\n';
}
