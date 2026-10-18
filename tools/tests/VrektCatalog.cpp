#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
#include <stdexcept>
static void Require(bool ok,char const* why){if(!ok)throw std::runtime_error(why);}
int Run(int argc,char**argv){if(argc!=2)return 1;
#define S(T,N,F,I) DB2Storage<T> N(F ".db2",I::Instance());if(!N.Load(argv[1],0))return 2
 S(QuestObjectiveEntry,objectives,"QuestObjective",QuestObjectiveLoadInfo);
 S(QuestV2CliTaskEntry,tasks,"QuestV2CliTask",QuestV2CliTaskLoadInfo);
 S(PlayerConditionEntry,conditions,"PlayerCondition",PlayerConditionLoadInfo);
 S(WorldStateExpressionEntry,expressions,"WorldStateExpression",WorldStateExpressionLoadInfo);
 S(AreaTableEntry,areas,"AreaTable",AreaTableLoadInfo);
 S(MapEntry,maps,"Map",MapLoadInfo);
 S(WorldMapAreaEntry,mapAreas,"WorldMapArea",WorldMapAreaLoadInfo);
 S(SpellEffectEntry,effects,"SpellEffect",SpellEffectLoadInfo);
 S(SpellEntry,spells,"Spell",SpellLoadInfo);
 S(SpellMiscEntry,misc,"SpellMisc",SpellMiscLoadInfo);
 S(CreatureDisplayInfoEntry,displays,"CreatureDisplayInfo",CreatureDisplayInfoLoadInfo);
 S(CreatureModelDataEntry,models,"CreatureModelData",CreatureModelDataLoadInfo);
#undef S
 uint32 ids[]={44737,44746,44923},conditionIDs[]={47903,47902,45693},expressionIDs[]={20860,20875,20988};
 for(unsigned i=0;i<3;++i){auto t=tasks.LookupEntry(ids[i]);auto c=t?conditions.LookupEntry(t->ConditionID):nullptr;
  auto e=t?expressions.LookupEntry(t->WorldStateExpressionID):nullptr;
  Require(t&&t->ConditionID==conditionIDs[i]&&t->WorldStateExpressionID==expressionIDs[i]&&t->FiltMinLevel==110&&
   c&&c->PrevQuestID[0]==41694&&c->PrevQuestID[1]==44721&&c->PrevQuestLogic==1&&e,"Native landing task/unlock changed");
  // Native Eval(nullptr,&states) harvests the required state, as the runtime
  // loader/generator does. Its boolean is not a live-player visibility result.
  std::set<WorldQuestState> states;e->Eval(nullptr,&states);
  Require(states.size()==1&&states.count({12678,i+1}),"Native shared state/variant changed");
  std::cout<<"state "<<ids[i]<<" 12678 "<<i+1<<" expression "<<expressionIDs[i]<<'\n';
 }
 unsigned count=0;
 for(auto o:objectives)if(o->QuestID==44923){
  Require(o->Type==0&&o->Flags==0&&
   ((o->ID==287303&&o->ObjectID==115845&&o->Amount==10&&o->StorageIndex==0)||
    (o->ID==287309&&o->ObjectID==115846&&o->Amount==1&&o->StorageIndex==1)),"Native Vrekt objective changed");
  ++count;std::cout<<"objective "<<o->ID<<" object "<<o->ObjectID<<" amount "<<o->Amount<<" slot "<<int(o->StorageIndex)<<'\n';}
 Require(count==2,"Expected both mandatory objectives");
 Require(maps.LookupEntry(1220)&&!maps.LookupEntry(uint32(-117440508))&&
  !objectives.LookupEntry(uint32(-2063596446)),"Reinspect corrupt POI native identifiers");
 for(uint32 id:{1015u,1017u})Require(mapAreas.LookupEntry(id)&&mapAreas.LookupEntry(id)->MapID==1220,"Valid shared-counter world-map area changed");
 auto a=areas.LookupEntry(8362);Require(a&&a->ContinentID==1220&&a->ParentAreaID==7541,"Native Vrekt map/zone changed");
 for(uint32 id:{66181u,64200u,64208u,28072u,29504u,69422u}){auto d=displays.LookupEntry(id);auto m=d?models.LookupEntry(d->ModelID):nullptr;
  Require(m&&m->FileDataID,"Missing Vrekt actor model");std::cout<<"display "<<id<<" file "<<m->FileDataID<<'\n';}
 bool outro=false;
 for(auto e:effects)if(e->SpellID==230157&&e->Effect==219&&e->EffectMiscValue[0]==3982&&e->ImplicitTarget[0]==18)outro=true;
 Require(outro,"Missing native Vrekt outro conversation");
 for(uint32 id:{186476u,186480u,186553u,186327u,195585u,222484u,213738u,215377u}){
  Require(spells.LookupEntry(id),"Missing Vrekt combat spell");
  for(auto m:misc)if(m->SpellID==id&&!m->DifficultyID)
   for(auto e:effects)if(e->SpellID==id&&!e->DifficultyID&&!e->EffectIndex){
    if(id==213738||id==215377){Require((m->Attributes[0]&0x800000)&&e->Effect==6&&
     e->EffectAura==(id==213738?129:79),"Artifact perk classification changed");
     auto description=std::string(spells.LookupEntry(id)->Description->Str[0]);
     Require(description.find(id==213738?"Fate and Fortune":"Maw of the Damned")!=std::string::npos,"Artifact perk identity changed");}
    std::cout<<"combat "<<id<<" attributes0 "<<m->Attributes[0]<<" aura "<<e->EffectAura<<" target "<<e->ImplicitTarget[0]<<" trigger "<<e->EffectTriggerSpell<<'\n';
   }
 }
 std::cout<<"PASS: typed native shared-state expressions, unlocks, mandatory objectives, Vrekt area, actor models and outro. No native retail duration or client verification claim.\n";
 return 0;
}
int main(int argc,char**argv){try{return Run(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
