#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
int main(int argc,char**argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> castTimes("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
    DB2Storage<SpellXSpellVisualEntry> visuals("SpellXSpellVisual.db2",SpellXSpellVisualLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!castTimes.Load(argv[1],0)||!visuals.Load(argv[1],0))return 4;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();castTimes.LoadFromDB();visuals.LoadFromDB();
    std::set<uint32> selected{204771,204641,130966,206390,206467,207414,201917};
    std::map<uint32,uint32> nameOrigins;
    for(auto spell:spells){std::string n=spell->Name->Str[0];std::transform(n.begin(),n.end(),n.begin(),[](unsigned char c){return char(std::tolower(c));});if(spell->ID>=200000&&spell->ID<=210000&&(n.find("awak")!=std::string::npos||n.find("zakaj")!=std::string::npos||n.find("taking")!=std::string::npos||n.find("reviv")!=std::string::npos||n.find("resur")!=std::string::npos||n.find("wake")!=std::string::npos||n.find("shockwave")!=std::string::npos)){nameOrigins.emplace(spell->ID,spell->ID);std::cout<<"CANDIDATE_SPELL\t"<<spell->ID<<'\t'<<spell->Name->Str[0]<<'\n';}}
    for(unsigned pass=0;pass<5;++pass)for(auto e:effects)if(!e->DifficultyID&&nameOrigins.count(e->SpellID)&&e->EffectTriggerSpell)nameOrigins.emplace(e->EffectTriggerSpell,nameOrigins[e->SpellID]);
    for(unsigned pass=0;pass<5;++pass)for(auto e:effects)if(!e->DifficultyID&&selected.count(e->SpellID)&&e->EffectTriggerSpell)selected.insert(e->EffectTriggerSpell);
    // Retain the unrelated stale reference exposed by the initial broad name
    // search; do not mistake narrowing this reader for repairing that record.
    for(auto e:effects)if(e->EffectTriggerSpell==81040&&nameOrigins.count(e->SpellID)){auto s=spells.LookupEntry(e->SpellID);std::cout<<"OTHER_UNRESOLVED_TRIGGER\t"<<nameOrigins[e->SpellID]<<'\t'<<e->SpellID<<'\t'<<(s?s->Name->Str[0]:"")<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<"\t81040\n";}
    for(uint32 id:selected){auto s=spells.LookupEntry(id);if(!s){std::cerr<<"Unresolved selected spell "<<id<<'\n';return 5;}std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';}
    for(auto e:effects)if(!e->DifficultyID&&selected.count(e->SpellID))std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectBasePoints<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\t'<<e->EffectRadiusIndex[0]<<'\t'<<e->EffectRadiusIndex[1]<<'\t'<<e->EffectAuraPeriod<<'\n';
    for(auto m:misc)if(!m->DifficultyID&&selected.count(m->SpellID)){auto d=durations.LookupEntry(m->DurationIndex);auto c=castTimes.LookupEntry(m->CastingTimeIndex);std::cout<<"MISC\t"<<m->SpellID<<'\t'<<(d?d->Duration:0)<<'\t'<<(c?c->Base:0);for(auto a:m->Attributes)std::cout<<'\t'<<uint32(a);std::cout<<'\n';}
    for(auto v:visuals)if(selected.count(v->SpellID))std::cout<<"VISUAL\t"<<v->SpellID<<'\t'<<v->SpellVisualID<<'\n';
    DB2Storage<SpellRadiusEntry> radii("SpellRadius.db2",SpellRadiusLoadInfo::Instance());
    if(!radii.Load(argv[1],0))return 6;radii.LoadFromDB();
    for(uint32 id:{10u,48u}){auto r=radii.LookupEntry(id);if(!r)return 7;std::cout<<"RADIUS\t"<<id<<'\t'<<r->Radius<<'\t'<<r->RadiusMin<<'\t'<<r->RadiusMax<<'\n';}
    DB2Storage<WorldMapAreaEntry> maps("WorldMapArea.db2",WorldMapAreaLoadInfo::Instance());
    if(!maps.Load(argv[1],0))return 8;maps.LoadFromDB();
    for(auto m:maps)if(m->MapID==1539)std::cout<<"MAP\t"<<m->ID<<'\t'<<m->AreaName->Str[0]<<'\t'<<m->LocLeft<<'\t'<<m->LocRight<<'\t'<<m->LocTop<<'\t'<<m->LocBottom<<'\n';
    DB2Storage<DungeonMapEntry> floors("DungeonMap.db2",DungeonMapLoadInfo::Instance());
    if(!floors.Load(argv[1],0))return 9;floors.LoadFromDB();
    for(auto f:floors)if(f->MapID==1539)std::cout<<"FLOOR\t"<<f->ID<<'\t'<<uint32(f->FloorIndex)<<'\t'<<f->Min[0]<<'\t'<<f->Min[1]<<'\t'<<f->Max[0]<<'\t'<<f->Max[1]<<'\n';
    HotfixDatabase.Close();MySQL::Library_End();return 0;
}
