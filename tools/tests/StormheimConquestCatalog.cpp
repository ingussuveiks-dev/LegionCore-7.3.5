#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char**argv){if(argc!=2)return 1;
#define S(T,N,F,I) DB2Storage<T> N(F ".db2",I::Instance());if(!N.Load(argv[1],0))return 2
S(SpellEffectEntry,effects,"SpellEffect",SpellEffectLoadInfo);S(SpellMiscEntry,misc,"SpellMisc",SpellMiscLoadInfo);
S(SpellDurationEntry,durations,"SpellDuration",SpellDurationLoadInfo);S(ConversationLineEntry,lines,"ConversationLine",ConversationLineLoadInfo);
S(CreatureDisplayInfoEntry,displays,"CreatureDisplayInfo",CreatureDisplayInfoLoadInfo);S(CreatureModelDataEntry,models,"CreatureModelData",CreatureModelDataLoadInfo);
S(AreaTableEntry,areas,"AreaTable",AreaTableLoadInfo);S(ItemSparseEntry,items,"ItemSparse",ItemSparseLoadInfo);
S(PlayerConditionEntry,conditions,"PlayerCondition",PlayerConditionLoadInfo);S(QuestV2CliTaskEntry,tasks,"QuestV2CliTask",QuestV2CliTaskLoadInfo);
#undef S
bool arrival=false,finale=false,transform=false,label=false;
for(auto e:effects){if(e->SpellID==228873&&e->Effect==90&&e->EffectMiscValue[0]==114964&&e->ImplicitTarget[0]==1)arrival=true;
 if(e->SpellID==228897&&e->Effect==90&&e->EffectMiscValue[0]==114972&&e->ImplicitTarget[0]==25)finale=true;
 if(e->SpellID==230410&&e->EffectAura==56&&e->EffectMiscValue[0]==114982)transform=true;
 if(e->SpellID==230408){if(e->ImplicitTarget[0]||e->ImplicitTarget[1]||e->EffectTriggerSpell||(e->Effect!=36&&e->Effect!=3))return 10;if(e->Effect==36)label=true;}}
if(!arrival||!finale||!transform||!label)return 3;
for(auto pair:{std::pair<uint32,int>{229225,1500},{228343,2000},{230410,6000}}){bool found=false;
 for(auto m:misc)if(m->SpellID==pair.first){auto d=durations.LookupEntry(m->DurationIndex);if(d&&d->Duration==pair.second)found=true;}
 if(!found)return 4;std::cout<<"duration "<<pair.first<<" "<<pair.second<<'\n';}
for(uint32 id:{73303u,70110u,66121u,64200u,64975u,64593u}){auto d=displays.LookupEntry(id);auto m=d?models.LookupEntry(d->ModelID):nullptr;if(!m||!m->FileDataID)return 5;std::cout<<"display "<<id<<" model "<<m->ID<<" file "<<m->FileDataID<<'\n';}
auto area=areas.LookupEntry(7847);if(!area||area->ContinentID!=1220)return 6;
auto line=lines.LookupEntry(8817);if(!line)return 7;std::cout<<"arrival-line "<<line->ID<<" "<<line->BroadcastTextID<<'\n';
auto item=items.LookupEntry(142535);if(!item)return 8;
for(auto pair:{std::pair<uint32,uint32>{44737,47903},{44746,47902},{44923,45693}}){
 auto task=tasks.LookupEntry(pair.first);auto condition=conditions.LookupEntry(pair.second);
 if(!task||task->ConditionID!=pair.second||!condition||condition->PrevQuestID[0]!=41694||condition->PrevQuestID[1]!=44721||condition->PrevQuestLogic!=1)return 9;
 std::cout<<"unlock "<<pair.first<<" condition "<<pair.second<<" rewarded "<<condition->PrevQuestID[0]<<" AND44721\n";
}
std::cout<<"PASS: native personal arrival/finale credits, transform/dissolve durations, all four actor models, harbor area, arrival conversation and reward item.\n";
}
