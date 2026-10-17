#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
#include <fstream>
#include <sstream>
#include <vector>
int main(int argc,char** argv)
{
 if(argc!=3)return 1;
 DB2Storage<AreaGroupMemberEntry> groups("AreaGroupMember.db2",AreaGroupMemberLoadInfo::Instance());
 DB2Storage<AreaTableEntry> areas("AreaTable.db2",AreaTableLoadInfo::Instance());
 DB2Storage<QuestV2CliTaskEntry> tasks("QuestV2CliTask.db2",QuestV2CliTaskLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
 DB2Storage<ItemEffectEntry> itemEffects("ItemEffect.db2",ItemEffectLoadInfo::Instance());
 DB2Storage<PlayerConditionEntry> conditions("PlayerCondition.db2",PlayerConditionLoadInfo::Instance());
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 DB2Storage<FactionTemplateEntry> factions("FactionTemplate.db2",FactionTemplateLoadInfo::Instance());
 if(!groups.Load(argv[1],0)||!areas.Load(argv[1],0)||!tasks.Load(argv[1],0)||!effects.Load(argv[1],0)||!items.Load(argv[1],0)||!itemEffects.Load(argv[1],0)||!conditions.Load(argv[1],0)||!spells.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!factions.Load(argv[1],0))return 2;
 unsigned n=0;for(auto g:groups)if(g->AreaGroupID==4410){auto a=areas.LookupEntry(g->AreaID);if(!a)return 3;++n;std::cout<<"area "<<g->AreaID<<" "<<a->AreaName->Str[0]<<'\n';}if(!n)return 3;
 auto t=tasks.LookupEntry(38374);if(!t||n!=1||t->ConditionID!=33048||t->FiltMinLevel!=98||t->FiltMaxLevel!=109||t->FiltMinSkillID||t->FiltMinSkillValue||t->FiltRaces!=-1||t->FiltClasses!=65535||t->FiltCompletedQuest[0]||t->FiltCompletedQuest[1]||t->FiltCompletedQuest[2]||t->WorldStateExpressionID||t->StartItem||t->FiltActiveQuest||t->FiltNonActiveQuest)return 4;
 std::cout<<"task "<<t->ID<<" condition "<<t->ConditionID<<" levels "<<int(t->FiltMinLevel)<<" "<<int(t->FiltMaxLevel)<<" skill "<<t->FiltMinSkillID<<" "<<unsigned(t->FiltMinSkillValue)<<" races "<<t->FiltRaces<<" classes "<<t->FiltClasses<<" completed "<<t->FiltCompletedQuest[0]<<" "<<t->FiltCompletedQuest[1]<<" "<<t->FiltCompletedQuest[2]<<" logic "<<unsigned(t->FiltCompletedQuestLogic)<<" worldstate "<<t->WorldStateExpressionID<<" startitem "<<t->StartItem<<" active "<<unsigned(t->FiltActiveQuest)<<" nonactive "<<unsigned(t->FiltNonActiveQuest)<<'\n';
 std::set<int32> aliases{92238,92239,92240,92241,96992,100474};
 std::set<std::pair<uint32,int32>> credits;
 for(auto e:effects)if((e->Effect==90||e->Effect==134)&&aliases.count(e->EffectMiscValue[0])){if(e->Effect!=134||e->ImplicitTarget[0]!=1||e->ImplicitTarget[1])return 7;credits.emplace(e->SpellID,e->EffectMiscValue[0]);std::cout<<"spell-credit "<<e->SpellID<<" "<<e->EffectIndex<<" "<<e->Effect<<" "<<e->EffectMiscValue[0]<<" "<<e->ImplicitTarget[0]<<" "<<e->ImplicitTarget[1]<<'\n';}
 if(credits!=std::set<std::pair<uint32,int32>>{{181858,92240},{183167,92238},{197885,92241}})return 7;
 auto item=items.LookupEntry(122612);if(!item)return 5;std::cout<<"drop-item "<<item->ID<<" "<<item->Display->Str[0]<<'\n';
 for(auto e:itemEffects)if(e->ItemID==122612)std::cout<<"item-effect "<<e->SpellID<<" "<<unsigned(e->TriggerType)<<'\n';
 auto c=conditions.LookupEntry(t->ConditionID);if(c){
#define PRINT(field) std::cout<<"condition "<<#field<<" "<<int64(c->field)<<'\n';
 PRINT(RaceMask) PRINT(Flags) PRINT(MinLevel) PRINT(MaxLevel) PRINT(ClassMask) PRINT(Gender) PRINT(NativeGender) PRINT(PrevQuestLogic) PRINT(CurrQuestLogic) PRINT(CurrentCompletedQuestLogic) PRINT(WorldStateExpressionID) PRINT(AreaLogic) PRINT(PhaseID) PRINT(PhaseGroupID) PRINT(ModifierTreeID)
#undef PRINT
 for(unsigned i=0;i<4;++i)std::cout<<"condition-quest "<<i<<" "<<c->PrevQuestID[i]<<" "<<c->CurrQuestID[i]<<" "<<c->CurrentCompletedQuestID[i]<<'\n';
 return 8;}else std::cout<<"condition absent\n";
 for(uint32 id:{181858u,181859u,183149u,183167u,183180u,183181u,197885u}){auto s=spells.LookupEntry(id);if(!s)return 6;std::cout<<"spell "<<id<<" "<<s->Name->Str[0]<<'\n';for(auto e:effects)if(e->SpellID==id)std::cout<<"effect "<<id<<" "<<e->EffectIndex<<" "<<e->Effect<<" "<<e->EffectMiscValue[0]<<" "<<e->ImplicitTarget[0]<<" "<<e->ImplicitTarget[1]<<" trigger "<<e->EffectTriggerSpell<<" aura "<<e->EffectAura<<" period "<<e->EffectAuraPeriod<<'\n';}
 for(auto e:effects)if(e->EffectTriggerSpell==181858||e->EffectTriggerSpell==183167)std::cout<<"parent "<<e->SpellID<<" "<<e->EffectTriggerSpell<<'\n';
 for(auto pair:{std::pair<uint32,uint32>{181859,181858},{183181,183167}}){unsigned found=0;for(auto e:effects)if(e->SpellID==pair.first){if(e->EffectIndex||e->Effect!=160||e->EffectTriggerSpell!=pair.second||e->ImplicitTarget[0]!=1||e->ImplicitTarget[1])return 13;++found;}if(found!=1)return 13;}
 for(auto e:itemEffects)if(e->SpellID==181858||e->SpellID==183167)std::cout<<"credit-item "<<e->ItemID<<" "<<e->SpellID<<'\n';
 std::ifstream input(argv[2]);if(!input)return 9;std::string line;std::set<uint32> seen;unsigned actors=0;
 while(std::getline(input,line)){std::istringstream row(line);std::vector<std::string> fields;std::string cell;while(std::getline(row,cell,'\t'))fields.push_back(cell);if(fields.empty()||fields[0]!="source"||fields.size()<14||!std::stoul(fields[13]))continue;++actors;auto f=factions.LookupEntry(std::stoul(fields[4]));auto a=factions.LookupEntry(1),h=factions.LookupEntry(2);if(!f||!a||!h||!f->IsHostileTo(*a)||!f->IsHostileTo(*h))return 10;unsigned count=0;for(unsigned i=8;i<12;++i){auto id=std::stoul(fields[i]);if(!id)continue;auto d=displays.LookupEntry(id);auto m=d?models.LookupEntry(d->ModelID):nullptr;if(!m||!m->FileDataID)return 11;++count;if(seen.insert(id).second)std::cout<<"display "<<id<<" model "<<m->ID<<" file "<<m->FileDataID<<'\n';}if(!count)return 11;}
 if(actors!=25)return 12;
 std::cout<<"PASS: native task/area/filter and three credit spells, brew parent and25 spawned-source model/faction catalogs. Full cast/actor initialization and client pending.\n";
}
