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
    DB2Storage<MapEntry> maps("Map.db2",MapLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());
    DB2Storage<SummonPropertiesEntry> properties("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
    DB2Storage<CreatureDifficultyEntry> creatures("CreatureDifficulty.db2",CreatureDifficultyLoadInfo::Instance());
    DB2Storage<PlayerConditionEntry> pc("PlayerCondition.db2",PlayerConditionLoadInfo::Instance());
    DB2Storage<WorldSafeLocsEntry> locs("WorldSafeLocs.db2",WorldSafeLocsLoadInfo::Instance());
    DB2Storage<AreaTableEntry> areas("AreaTable.db2",AreaTableLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!times.Load(argv[1],0))return 4;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();times.LoadFromDB();
    if(!maps.Load(argv[1],0)||!texts.Load(argv[1],0)||!locks.Load(argv[1],0)||!properties.Load(argv[1],0)||!creatures.Load(argv[1],0)||!areas.Load(argv[1],0))return 6;
    maps.LoadFromDB();texts.LoadFromDB();locks.LoadFromDB();properties.LoadFromDB();creatures.LoadFromDB();areas.LoadFromDB();
    if(!pc.Load(argv[1],0)||!locs.Load(argv[1],0))return 7;pc.LoadFromDB();locs.LoadFromDB();
    for(auto id:{38059,38060,38061,38062})if(auto p=pc.LookupEntry(id)){std::cout<<"PLAYER_CONDITION\t"<<id<<" spec="<<int(p->ChrSpecializationIndex)<<" artifact="<<p->ItemLogic<<" auralogic="<<p->AuraSpellLogic;for(auto i:p->ItemID)std::cout<<" item="<<i;for(auto i:p->AuraSpellID)std::cout<<" aura="<<i;for(auto i:p->SpellID)std::cout<<" spell="<<i;std::cout<<'\n';}
    for(auto l:locs)if(l->MapID==1512||l->ID==1354||l->ID==1371)std::cout<<"WORLD_SAFE\t"<<l->ID<<'\t'<<l->Name->Str[0]<<'\t'<<l->MapID<<'\t'<<l->Loc.X<<'\t'<<l->Loc.Y<<'\t'<<l->Loc.Z<<'\t'<<l->Loc.O<<'\n';
    for(auto t:texts)if((t->ID>=104670&&t->ID<=104690)||std::string(t->Text->Str[0]).find("At last we found a way")!=std::string::npos)std::cout<<"VELEN_TEXT\t"<<t->ID<<'\t'<<t->Text->Str[0]<<'\t'<<t->SoundEntriesID[0]<<'\n';
    std::set<uint32> pending{192293,192295,202324,202457,202835,202747,202749,202744,202759,216409,216418,202491,202945},seen;
    
    for(auto e:effects)if(e->EffectMiscValue[0]==102466||e->EffectMiscValue[0]==102465||e->EffectMiscValue[0]==102652||e->EffectMiscValue[0]==102653||e->EffectMiscValue[0]==102654){pending.insert(e->SpellID);std::cout<<"ALL_MARKER_SOURCE\t"<<e->SpellID<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\n';}
    while(!pending.empty()){
        uint32 id=*pending.begin();pending.erase(pending.begin());if(!seen.insert(id).second)continue;if(seen.size()>160)return 5;
        auto s=spells.LookupEntry(id);if(!s){std::cout<<"MISSING\t"<<id<<'\n';continue;}
        std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';
        for(auto e:effects)if(e->SpellID==id){std::cout<<"EFFECT\t"<<id<<'\t'<<e->DifficultyID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\t'<<e->EffectAuraPeriod<<'\n';if(e->EffectTriggerSpell)pending.insert(e->EffectTriggerSpell);}
        for(auto m:misc)if(m->SpellID==id){auto d=durations.LookupEntry(m->DurationIndex);auto t=times.LookupEntry(m->CastingTimeIndex);std::cout<<"MISC\t"<<id<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<(d?d->Duration:0)<<'\t'<<(t?t->Base:0)<<'\n';std::cout<<"CAST_POLICY\t"<<id<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<m->Speed<<'\t'<<unsigned(m->SchoolMask)<<'\n';std::cout<<"ATTR\t"<<id;for(auto a:m->Attributes)std::cout<<'\t'<<uint32(a);std::cout<<'\n';}
    }
    HotfixDatabase.Close();
}
