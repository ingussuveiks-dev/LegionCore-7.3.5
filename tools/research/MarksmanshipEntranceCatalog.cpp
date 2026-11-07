#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char**argv){
 if(argc!=2)return 1;
 DB2Storage<LFGDungeonsEntry> lfg("LFGDungeons.db2",LfgDungeonsLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 if(!lfg.Load(argv[1],0)||!effects.Load(argv[1],0))return 2;
 unsigned routes=0;
 for(auto row:lfg)if(row->MapID==1489){std::cout<<"LFG\t"<<row->ID<<'\t'<<row->MapID<<'\t'<<row->ScenarioID<<'\t'<<unsigned(row->DifficultyID)<<'\t'<<row->Flags<<'\n';++routes;}
 for(auto row:effects)if(row->SpellID==201674||row->SpellID==201678||row->SpellID==205040)std::cout<<"EFFECT\t"<<row->SpellID<<'\t'<<unsigned(row->EffectIndex)<<'\t'<<row->Effect<<'\t'<<row->EffectMiscValue[0]<<'\t'<<row->EffectTriggerSpell<<'\t'<<unsigned(row->ImplicitTarget[0])<<'\t'<<unsigned(row->ImplicitTarget[1])<<'\n';
 std::cout<<"COMPLETE\t"<<routes<<'\n';return routes?0:3;
}
