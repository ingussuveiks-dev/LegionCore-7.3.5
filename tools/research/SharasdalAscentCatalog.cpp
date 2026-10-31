#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "MovementPackets.h"
#include "SpellAuraDefines.h"
#include "SpellInfo.h"
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
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check=HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if(!check||!check->Fetch()[0].GetBool())return 4;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
    DB2Storage<SpellRangeEntry> ranges("SpellRange.db2",SpellRangeLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!times.Load(argv[1],0)||!ranges.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 5;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();times.LoadFromDB();ranges.LoadFromDB();steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();
    for(uint32 id:{83810u,208580u,209992u})
    {
        auto spell=spells.LookupEntry(id);if(!spell)return 6;
        std::cout<<"SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';
        for(auto effect:effects)if(effect->SpellID==id)
        {
            std::cout<<"EFFECT\t"<<id<<'\t'<<unsigned(effect->DifficultyID)<<'\t'<<unsigned(effect->EffectIndex)<<'\t'<<effect->Effect<<'\t'<<effect->EffectAura<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectTriggerSpell<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
            SpellImplicitTargetInfo target(effect->ImplicitTarget[0]);
            std::cout<<"TARGET\t"<<id<<'\t'<<unsigned(effect->EffectIndex)<<'\t'<<target.GetCheckType()<<'\t'<<target.GetReferenceType()<<'\n';
        }
        for(auto row:misc)if(row->SpellID==id)
        {
            auto duration=durations.LookupEntry(row->DurationIndex);auto time=times.LookupEntry(row->CastingTimeIndex);auto range=ranges.LookupEntry(row->RangeIndex);
            std::cout<<"MISC\t"<<id<<'\t'<<unsigned(row->DifficultyID)<<'\t'<<row->DurationIndex<<'\t'<<(duration?duration->Duration:-2)<<'\t'<<(time?time->Base:-1)<<'\t'<<(range?range->RangeMax[0]:-1)<<'\t'<<(range?range->RangeMax[1]:-1)<<'\n';
        }
    }
    for(auto step:steps)if(step->ScenarioID==1066&&step->OrderIndex<=3)
    {
        std::cout<<"STEP\t"<<unsigned(step->OrderIndex)<<'\t'<<step->ID<<'\t'<<step->Criteriatreeid<<'\n';
        std::set<uint32> pending{uint32(step->Criteriatreeid)},seen;
        while(!pending.empty())
        {
            uint32 id=*pending.begin();pending.erase(pending.begin());if(!seen.insert(id).second)continue;
            auto tree=trees.LookupEntry(id);if(!tree)return 7;
            std::cout<<"TREE\t"<<unsigned(step->OrderIndex)<<'\t'<<id<<'\t'<<tree->Parent<<'\t'<<unsigned(tree->Operator)<<'\t'<<tree->Amount<<'\t'<<tree->CriteriaID<<'\n';
            if(tree->CriteriaID){auto c=criteria.LookupEntry(tree->CriteriaID);if(!c)return 8;std::cout<<"CRITERIA\t"<<c->ID<<'\t'<<unsigned(c->Type)<<'\t'<<c->Asset<<'\n';}
            for(auto child:trees)if(child->Parent==id)pending.insert(child->ID);
        }
    }
    std::cout<<"EOF\tascent-native\n";
    HotfixDatabase.Close();MySQL::Library_End();
}
