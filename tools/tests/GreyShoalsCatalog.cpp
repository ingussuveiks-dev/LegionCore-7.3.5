#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <stdexcept>
static void Require(bool ok,char const* reason){if(!ok)throw std::runtime_error(reason);}
int Run(int argc,char** argv){
 if(argc!=2)return 1;
#define STORE(T,N,F,I) DB2Storage<T> N(F ".db2",I::Instance()); if(!N.Load(argv[1],0))return 2
 STORE(QuestObjectiveEntry,objectives,"QuestObjective",QuestObjectiveLoadInfo);
 STORE(QuestV2CliTaskEntry,tasks,"QuestV2CliTask",QuestV2CliTaskLoadInfo);
 STORE(PlayerConditionEntry,conditions,"PlayerCondition",PlayerConditionLoadInfo);
 STORE(SpellEntry,spells,"Spell",SpellLoadInfo);
 STORE(SpellEffectEntry,effects,"SpellEffect",SpellEffectLoadInfo);
 STORE(CreatureDisplayInfoEntry,displays,"CreatureDisplayInfo",CreatureDisplayInfoLoadInfo);
 STORE(CreatureModelDataEntry,models,"CreatureModelData",CreatureModelDataLoadInfo);
 STORE(GameObjectDisplayInfoEntry,objects,"GameObjectDisplayInfo",GameobjectDisplayInfoLoadInfo);
 STORE(FactionTemplateEntry,factions,"FactionTemplate",FactionTemplateLoadInfo);
#undef STORE
 uint32 ids[]={287220,287221,287222,287223,287226,287238};
 int32 entries[]={0,115025,115031,115026,115739,115751},amounts[]={1,17,20,50,20,4};
 // The progress-bar pseudo-objective has native slot0; server SQL uses -1
 // because it is computed from the five real counters, not stored itself.
 int slots[]={0,1,2,3,0,4};
 for(int i=0;i<6;++i){auto o=objectives.LookupEntry(ids[i]);
  Require(o&&o->QuestID==44737&&o->ObjectID==entries[i]&&o->Amount==amounts[i]&&
   o->StorageIndex==slots[i]&&o->Type==(i?0:15)&&o->Flags==(i?92:0),"Native objective mapping changed");
  std::cout<<"objective "<<o->ID<<" entry "<<o->ObjectID<<" amount "<<o->Amount<<" flags "<<int(o->Flags)<<'\n';}
 auto task=tasks.LookupEntry(44737);auto c=task?conditions.LookupEntry(task->ConditionID):nullptr;
 if(task&&c)std::cerr<<"task condition "<<task->ConditionID<<" quests "<<c->PrevQuestID[0]<<','<<c->PrevQuestID[1]<<','<<c->PrevQuestID[2]<<','<<c->PrevQuestID[3]<<" logic "<<c->PrevQuestLogic<<'\n';
 Require(task&&task->ConditionID==47903&&c&&c->PrevQuestID[0]==41694&&c->PrevQuestID[1]==44721&&c->PrevQuestLogic==1,"Native unlock changed");
 for(uint32 id:{29504u,69422u,64467u,28072u,64464u}){auto d=displays.LookupEntry(id);auto m=d?models.LookupEntry(d->ModelID):nullptr;
  Require(m&&m->FileDataID,"Missing encounter model");std::cout<<"display "<<id<<" file "<<m->FileDataID<<'\n';}
 for(uint32 id:{2817u,16u})Require(factions.LookupEntry(id)&&factions.LookupEntry(id)->IsHostileToPlayers(),"Nonhostile combat template");
 for(uint32 id:{186480u,186476u,213738u,215377u,186302u,222484u,186327u,186338u,229049u,184397u,193190u,229972u}){
  Require(spells.LookupEntry(id),"Missing configured spell");
  for(auto e:effects)if(e->SpellID==id&&!e->DifficultyID)
   std::cout<<"spell "<<id<<" effect "<<e->EffectIndex<<" type "<<e->Effect<<" aura "<<e->EffectAura<<" misc "<<e->EffectMiscValue[0]<<" target "<<e->ImplicitTarget[0]<<','<<e->ImplicitTarget[1]<<'\n';}
 for(auto e:effects)if(e->EffectMiscValue[0]==115739||e->EffectMiscValue[1]==115739)
  {Require(e->SpellID==229972&&e->Effect==134&&e->ImplicitTarget[0]==1,"Reinspect supply native credit routing");
   std::cout<<"supply "<<e->SpellID<<" effect "<<e->EffectIndex<<" type "<<e->Effect<<'\n';}
 auto rack=objects.LookupEntry(35575);Require(rack&&rack->FileDataID,"Missing native Kvaldir rack model");
 std::cout<<"rack display35575 file "<<rack->FileDataID<<'\n';
 std::cout<<"PASS: typed native objective/unlock/model/faction/spell catalog; TaskStep weights are server SQL, not QuestObjective.db2 fields\n";
 return 0;
}
int main(int argc,char** argv){try{return Run(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
