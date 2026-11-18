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
    DB2Storage<AreaTableEntry> areas("AreaTable.db2",AreaTableLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!times.Load(argv[1],0))return 4;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();times.LoadFromDB();
    if(!maps.Load(argv[1],0)||!texts.Load(argv[1],0)||!locks.Load(argv[1],0)||!properties.Load(argv[1],0)||!creatures.Load(argv[1],0)||!areas.Load(argv[1],0))return 6;
    maps.LoadFromDB();texts.LoadFromDB();locks.LoadFromDB();properties.LoadFromDB();creatures.LoadFromDB();areas.LoadFromDB();
    for(auto c:creatures)if(c->CreatureID==110635)std::cout<<"CREATURE_DIFFICULTY\t"<<c->ID<<'\t'<<c->CreatureID<<'\t'<<c->FactionID<<'\t'<<int(c->MinLevel)<<'\t'<<int(c->MaxLevel)<<'\t'<<int(c->ExpansionID)<<'\n';
    for(auto a:areas){std::string s=a->AreaName->Str[0];if(s=="Sorrow Hill"||s=="Uther's Tomb")std::cout<<"AREA\t"<<a->ID<<'\t'<<s<<'\t'<<a->ParentAreaID<<'\t'<<a->ContinentID<<'\n';}
    for(int id:{1469,1572})if(auto m=maps.LookupEntry(id))std::cout<<"MAP\t"<<id<<'\t'<<unsigned(m->InstanceType)<<'\t'<<unsigned(m->MapType)<<'\t'<<m->Flags[0]<<'\t'<<m->Flags[1]<<'\t'<<unsigned(m->MaxPlayers)<<'\n';
    for(int id:{100765,100767,100768,100783,100830})if(auto t=texts.LookupEntry(id))std::cout<<"TEXT\t"<<id<<'\t'<<t->Text->Str[0]<<'\t'<<t->Text1->Str[0]<<'\t'<<t->SoundEntriesID[0]<<'\t'<<t->SoundEntriesID[1]<<'\n';
    if(auto l=locks.LookupEntry(2454))for(int i=0;i<8;++i)std::cout<<"LOCK\t"<<l->ID<<'\t'<<i<<'\t'<<unsigned(l->Type[i])<<'\t'<<l->Index[i]<<'\t'<<l->Skill[i]<<'\t'<<unsigned(l->Action[i])<<'\n';
    for(int id:{2912,714})if(auto s=properties.LookupEntry(id))std::cout<<"SUMMON_PROPERTIES\t"<<s->ID<<'\t'<<s->Flags<<'\t'<<s->Control<<'\t'<<s->Faction<<'\t'<<s->Title<<'\t'<<s->Slot<<'\n';
    for(auto t:texts){std::string a=t->Text->Str[0];if(a.find("Ye won't be takin'")!=std::string::npos||a.find("I could do this all day!")!=std::string::npos||a.find("There's nae hope")!=std::string::npos||a=="Thank... ye...")std::cout<<"DARGAL_TEXT\t"<<t->ID<<'\t'<<a<<'\t'<<t->SoundEntriesID[0]<<'\n';}
    std::set<uint32> dargalSpells;
    for(auto s:spells)if(std::string(s->Name->Str[0])=="Shrapnel Blast" || std::string(s->Name->Str[0])=="Summon Spectral Mortar"){dargalSpells.insert(s->ID);std::cout<<"DARGAL_ABILITY_CANDIDATE\t"<<s->ID<<'\t'<<s->Description->Str[0]<<'\n';}
    for(auto e:effects)if((e->Effect==90&&(e->EffectMiscValue[0]==102466||e->EffectMiscValue[0]==102465||e->EffectMiscValue[0]==110635))||((e->Effect==28||e->Effect==56)&&e->EffectMiscValue[0]==110635)){
        auto s=spells.LookupEntry(e->SpellID);std::cout<<"MANDATORY_SOURCE\t"<<e->SpellID<<'\t'<<(s?s->Name->Str[0]:"")<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\n';}
    std::set<uint32> pending{192303,192344,190324,187818,195070,195132,228199,202324},seen;
    pending.insert(dargalSpells.begin(),dargalSpells.end());
    while(!pending.empty()){
        uint32 id=*pending.begin();pending.erase(pending.begin());if(!seen.insert(id).second)continue;if(seen.size()>80)return 5;
        auto s=spells.LookupEntry(id);if(!s){std::cout<<"MISSING\t"<<id<<'\n';continue;}
        std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';
        for(auto e:effects)if(e->SpellID==id){std::cout<<"EFFECT\t"<<id<<'\t'<<e->DifficultyID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\t'<<e->EffectAuraPeriod<<'\n';if(e->EffectTriggerSpell)pending.insert(e->EffectTriggerSpell);}
        for(auto m:misc)if(m->SpellID==id){auto d=durations.LookupEntry(m->DurationIndex);auto t=times.LookupEntry(m->CastingTimeIndex);std::cout<<"MISC\t"<<id<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<(d?d->Duration:0)<<'\t'<<(t?t->Base:0)<<'\n';std::cout<<"CAST_POLICY\t"<<id<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<m->Speed<<'\t'<<unsigned(m->SchoolMask)<<'\n';std::cout<<"ATTR\t"<<id;for(auto a:m->Attributes)std::cout<<'\t'<<uint32(a);std::cout<<'\n';}
    }
    HotfixDatabase.Close();
}
