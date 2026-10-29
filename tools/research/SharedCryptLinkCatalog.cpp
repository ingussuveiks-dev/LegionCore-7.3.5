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
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2",ConversationLineLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!times.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0)||!lines.Load(argv[1],0)||!texts.Load(argv[1],0))return 4;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();times.LoadFromDB();steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();lines.LoadFromDB();texts.LoadFromDB();
    std::set<uint32> pending{200165,201004,202014,63048,202925,37816},seen;
    for(auto e:effects)if(e->Effect==28&&(e->EffectMiscValue[0]==103151||e->EffectMiscValue[0]==101430)){pending.insert(e->SpellID);std::cout<<"SUMMON\t"<<e->SpellID<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\n';}
    while(!pending.empty()){
        uint32 id=*pending.begin();pending.erase(pending.begin());if(!seen.insert(id).second)continue;if(seen.size()>80)return 5;
        auto s=spells.LookupEntry(id);if(!s){std::cout<<"MISSING\t"<<id<<'\n';continue;}
        std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';
        for(auto e:effects)if(e->SpellID==id){std::cout<<"EFFECT\t"<<id<<'\t'<<e->DifficultyID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\t'<<e->EffectAuraPeriod<<'\n';if(e->EffectTriggerSpell)pending.insert(e->EffectTriggerSpell);}
        for(auto m:misc)if(m->SpellID==id){auto d=durations.LookupEntry(m->DurationIndex);auto t=times.LookupEntry(m->CastingTimeIndex);std::cout<<"MISC\t"<<id<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<(d?d->Duration:0)<<'\t'<<(t?t->Base:0)<<'\n';}
    }
    for(auto c:criteria)if(c->Type==92&&(c->Asset==48121||c->Asset==48160||c->Asset==49041))
        for(auto t:trees)if(t->CriteriaID==c->ID)
            for(auto s:steps){std::set<uint32> visited;uint32 id=t->ID;while(id&&visited.insert(id).second){if(id==s->Criteriatreeid)std::cout<<"SCENARIO_EVENT\t"<<c->Asset<<'\t'<<s->ScenarioID<<'\t'<<unsigned(s->OrderIndex)<<'\t'<<s->Criteriatreeid<<'\t'<<t->ID<<'\n';auto p=trees.LookupEntry(id);if(!p)break;id=p->Parent;}}
    for(uint32 id:{2522u,2523u,2524u})if(auto l=lines.LookupEntry(id)){auto t=texts.LookupEntry(l->BroadcastTextID);std::cout<<"LINE\t"<<id<<'\t'<<l->BroadcastTextID<<'\t'<<l->NextConversationLineID<<'\t'<<l->AdditionalDuration<<'\t'<<(t?t->Text->Str[0]:"")<<'\n';}
    HotfixDatabase.Close();
}
