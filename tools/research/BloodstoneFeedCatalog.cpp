#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
    DB2Storage<SpellRangeEntry> ranges("SpellRange.db2",SpellRangeLoadInfo::Instance()); if(!ranges.Load(argv[1],0))return 9;ranges.LoadFromDB(); DB2Storage<OverrideSpellDataEntry> overrides("OverrideSpellData.db2",OverrideSpellDataLoadInfo::Instance());if(!overrides.Load(argv[1],0))return 10;overrides.LoadFromDB(); DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!times.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 4;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();times.LoadFromDB();steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();
    std::set<uint32> pending{207711,207715,207722},seen;for(auto e:effects)if((e->Effect==90||e->Effect==134)&&e->EffectMiscValue[0]==104732)pending.insert(e->SpellID);
    while(!pending.empty()){
        uint32 id=*pending.begin();pending.erase(pending.begin());if(!seen.insert(id).second)continue;if(seen.size()>80)return 5;
        auto s=spells.LookupEntry(id);if(!s){std::cout<<"MISSING\t"<<id<<'\n';continue;}
        std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';
        for(auto e:effects)if(e->SpellID==id){std::cout<<"EFFECT\t"<<id<<'\t'<<e->DifficultyID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\t'<<e->EffectAuraPeriod<<'\t'<<e->EffectBasePoints<<'\t'<<e->EffectDieSides<<'\n';if(e->EffectTriggerSpell)pending.insert(e->EffectTriggerSpell);}
        for(auto m:misc)if(m->SpellID==id){auto d=durations.LookupEntry(m->DurationIndex);auto t=times.LookupEntry(m->CastingTimeIndex);std::cout<<"MISC\t"<<id<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<(d?d->Duration:0)<<'\t'<<(t?t->Base:0)<<'\t'<<m->RangeIndex;auto r=ranges.LookupEntry(m->RangeIndex);std::cout<<'\t'<<(r?r->RangeMax[0]:0)<<'\t'<<(r?r->RangeMax[1]:0)<<'\n';}
    }
    DB2Storage<UnitPowerBarEntry> bars("UnitPowerBar.db2",UnitPowerBarLoadInfo::Instance());if(!bars.Load(argv[1],0))return 12;bars.LoadFromDB();auto bar=bars.LookupEntry(365);if(!bar)return 13;std::cout<<"BLOODSTONE_BAR\t365\t"<<bar->StartPower<<"\t"<<bar->MinPower<<"\t"<<bar->MaxPower<<"\n";HotfixDatabase.Close();
}