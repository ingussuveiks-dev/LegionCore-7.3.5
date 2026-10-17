#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
int main(int argc,char** argv)
{
 if(argc!=2)return 1;
 DB2Storage<AreaGroupMemberEntry> groups("AreaGroupMember.db2",AreaGroupMemberLoadInfo::Instance());
 DB2Storage<AreaTableEntry> areas("AreaTable.db2",AreaTableLoadInfo::Instance());
 DB2Storage<QuestV2CliTaskEntry> tasks("QuestV2CliTask.db2",QuestV2CliTaskLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<PlayerConditionEntry> conditions("PlayerCondition.db2",PlayerConditionLoadInfo::Instance());
 if(!groups.Load(argv[1],0)||!areas.Load(argv[1],0)||!tasks.Load(argv[1],0)||!effects.Load(argv[1],0)||!conditions.Load(argv[1],0))return 2;
 unsigned n=0;for(auto g:groups)if(g->AreaGroupID==4306){auto a=areas.LookupEntry(g->AreaID);if(!a)return 3;++n;std::cout<<"area "<<g->AreaID<<" "<<a->AreaName->Str[0]<<'\n';}if(!n)return 3;
 auto t=tasks.LookupEntry(39998);if(!t||n!=1||t->ConditionID||t->FiltMinLevel!=98||t->FiltMaxLevel!=109||t->FiltMinSkillID||t->FiltMinSkillValue||t->FiltRaces!=-1||t->FiltClasses!=65535||t->FiltCompletedQuest[0]!=40002||t->FiltCompletedQuest[1]||t->FiltCompletedQuest[2]||t->FiltCompletedQuestLogic||t->WorldStateExpressionID||t->StartItem||t->FiltActiveQuest||t->FiltNonActiveQuest)return 4;
 std::cout<<"task "<<t->ID<<" condition "<<t->ConditionID<<" levels "<<int(t->FiltMinLevel)<<" "<<int(t->FiltMaxLevel)<<" skill "<<t->FiltMinSkillID<<" "<<unsigned(t->FiltMinSkillValue)<<" races "<<t->FiltRaces<<" classes "<<t->FiltClasses<<" completed "<<t->FiltCompletedQuest[0]<<" "<<t->FiltCompletedQuest[1]<<" "<<t->FiltCompletedQuest[2]<<" logic "<<unsigned(t->FiltCompletedQuestLogic)<<" worldstate "<<t->WorldStateExpressionID<<" startitem "<<t->StartItem<<" active "<<unsigned(t->FiltActiveQuest)<<" nonactive "<<unsigned(t->FiltNonActiveQuest)<<'\n';
 std::set<std::pair<uint32,uint32>> credits;
 for(auto e:effects){if((e->Effect==90||e->Effect==134)&&e->EffectMiscValue[0]>=97945&&e->EffectMiscValue[0]<=97949){credits.emplace(e->SpellID,e->EffectMiscValue[0]);std::cout<<"spell-credit "<<e->SpellID<<" "<<e->EffectMiscValue[0]<<'\n';}if(e->SpellID==193190||e->SpellID==193191){if(e->EffectIndex||e->Effect!=134||e->ImplicitTarget[0]!=1||e->ImplicitTarget[1])return 6;std::cout<<"effect "<<e->SpellID<<" "<<e->EffectIndex<<" "<<e->Effect<<" "<<e->EffectMiscValue[0]<<" "<<e->ImplicitTarget[0]<<" "<<e->ImplicitTarget[1]<<'\n';}}
 if(credits!=std::set<std::pair<uint32,uint32>>{{193190,97947},{193191,97949}})return 5;
 auto c=conditions.LookupEntry(36239);
 std::cout<<"condition36239 "<<(c?"present":"absent")<<'\n';
 std::cout<<"PASS: native signed task filters, area-group membership and banner/circle credit effects. Client pending.\n";
}
