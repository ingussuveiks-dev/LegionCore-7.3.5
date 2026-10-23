#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
#include <string>
int main(int argc,char** argv)
{
 if(argc!=2)return 1;
 DB2Storage<ScenarioEntry> scenarios("Scenario.db2",ScenarioLoadInfo::Instance());
 DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
 DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
 DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellEntry> names("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 if(!scenarios.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0)||!effects.Load(argv[1],0)||!names.Load(argv[1],0)||!displays.Load(argv[1],0))return 2;
 for(int id:{14316,66838,68958,11686}){auto d=displays.LookupEntry(id);if(!d)return 6;std::cout<<"DISPLAY\t"<<id<<'\t'<<d->ModelID<<'\n';}
 for(auto id:{1068,1099}){
  auto scenario=scenarios.LookupEntry(id);if(!scenario)return 3;
  std::cout<<"SCENARIO\t"<<id<<'\t'<<scenario->Name->Str[0]<<'\n';
  std::set<int32> selected;
  for(auto s:steps)if(s->ScenarioID==id){selected.insert(s->Criteriatreeid);std::cout<<"STEP\t"<<id<<'\t'<<unsigned(s->OrderIndex)<<'\t'<<s->ID<<'\t'<<s->Criteriatreeid<<'\t'<<s->RewardQuestID<<'\n';}
  for(size_t old=0;old!=selected.size();){old=selected.size();for(auto t:trees)if(selected.count(t->Parent))selected.insert(t->ID);}
  for(auto t:trees)if(selected.count(t->ID))if(auto c=criteria.LookupEntry(t->CriteriaID))std::cout<<"CRITERIA\t"<<id<<'\t'<<t->ID<<'\t'<<t->Parent<<'\t'<<t->Amount<<'\t'<<c->ID<<'\t'<<unsigned(c->Type)<<'\t'<<c->Asset<<'\n';
 }
 std::set<int32> spells={6660,194584,210492,210494,210497,210498,212060,210720,210724,210755,210758,210763,210768,210772,210777,210789,210790,210791};
 for(auto s:names){std::string n=s->Name->Str[0];if(n.find("Grif")!=std::string::npos||n.find("Prustaga")!=std::string::npos){spells.insert(s->ID);std::cout<<"NAME\t"<<s->ID<<'\t'<<n<<'\n';}}
 for(auto e:effects)if(e->SpellID>=210700&&e->SpellID<=210850&&e->Effect==28){spells.insert(e->SpellID);std::cout<<"SUMMON\t"<<e->SpellID<<'\t'<<e->EffectMiscValue[0]<<'\n';}
 for(size_t old=0;old!=spells.size();){old=spells.size();for(auto e:effects)if(!e->DifficultyID&&spells.count(e->SpellID)&&e->EffectTriggerSpell)spells.insert(e->EffectTriggerSpell);}
 for(auto id:spells)if(auto s=names.LookupEntry(id))std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\t'<<s->Description->Str[0]<<'\n';
 for(auto e:effects)if(!e->DifficultyID&&spells.count(e->SpellID))
  std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\n';
}
