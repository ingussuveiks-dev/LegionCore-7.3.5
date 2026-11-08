#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <iomanip>
int main(int argc,char**argv){
 if(argc!=2)return 1;
 DB2Storage<WorldSafeLocsEntry> safe("WorldSafeLocs.db2",WorldSafeLocsLoadInfo::Instance());
 DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
 DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
 DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
 if(!safe.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 2;
 DB2Storage<LFGDungeonsEntry> lfg("LFGDungeons.db2",LfgDungeonsLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 if(!lfg.Load(argv[1],0)||!effects.Load(argv[1],0))return 5;
 for(auto e:lfg)if(e->MapID==1489&&e->ScenarioID==1146)std::cout<<"LFG\t"<<e->ID<<'\t'<<e->MapID<<'\t'<<e->ScenarioID<<'\t'<<unsigned(e->DifficultyID)<<'\t'<<e->Flags<<'\n';
 for(auto e:effects)if(e->SpellID==217812)std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<unsigned(e->EffectIndex)<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\n';
 std::cout<<std::setprecision(9);
 for(unsigned id:{5558u,5654u,5655u}){auto s=safe.LookupEntry(id);if(!s)return 3;std::cout<<"SAFE\t"<<id<<'\t'<<s->MapID<<'\t'<<s->Loc.X<<'\t'<<s->Loc.Y<<'\t'<<s->Loc.Z<<'\t'<<s->Loc.O<<'\t'<<s->Name->Str[0]<<'\n';}
 unsigned count=0;
 for(auto step:steps)if(step->ScenarioID==1146){++count;std::cout<<"STEP\t"<<unsigned(step->OrderIndex)<<'\t'<<step->ID<<'\t'<<step->Criteriatreeid<<'\t'<<step->Title->Str[0]<<'\t'<<step->Description->Str[0]<<'\n';
  for(auto leaf:trees)if(auto c=criteria.LookupEntry(leaf->CriteriaID)){
   auto current=leaf;for(unsigned depth=0;current&&depth<64;++depth){if(current->ID==step->Criteriatreeid){std::cout<<"CRITERION\t"<<unsigned(step->OrderIndex)<<'\t'<<leaf->ID<<'\t'<<c->ID<<'\t'<<unsigned(c->Type)<<'\t'<<c->Asset<<'\n';break;}current=trees.LookupEntry(current->Parent);}
  }
 }
 std::cout<<"COMPLETE\t"<<count<<'\n';return count>0?0:4;
}
