#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <set>
#include <string>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!ro||!ro->Fetch()[0].GetBool())return 4;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    DB2Storage<SummonPropertiesEntry> properties("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
    DB2Storage<VehicleEntry> vehicles("Vehicle.db2",VehicleLoadInfo::Instance());
    DB2Storage<VehicleSeatEntry> seats("VehicleSeat.db2",VehicleSeatLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!properties.Load(argv[1],0)||!vehicles.Load(argv[1],0)||!seats.Load(argv[1],0))return 5;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();properties.LoadFromDB();vehicles.LoadFromDB();seats.LoadFromDB();
    std::cout.precision(9);std::set<uint32> selected{217618,217819,217820,46598},seen,props;
    for(auto spell:spells)
    {
        std::string name=spell->Name->Str[0];std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(spell->ID>=190000&&spell->ID<240000&&(name.find("yu'lon")!=std::string::npos||name.find("yigthregor")!=std::string::npos||name.find("wandering isle")!=std::string::npos))selected.insert(spell->ID);
    }
    for(auto effect:effects)if(effect->Effect==28&&effect->EffectMiscValue[0]==109441)selected.insert(effect->SpellID);
    while(seen.size()!=selected.size())
    {
        if(selected.size()>100)return 6;uint32 id=0;for(auto value:selected)if(!seen.count(value)){id=value;break;}seen.insert(id);
        if(auto spell=spells.LookupEntry(id))std::cout<<"SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';else return 7;
        for(auto e:effects)if(e->SpellID==id&&!e->DifficultyID)
        {
            std::cout<<"EFFECT\t"<<id<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectBasePoints<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';
            if(e->EffectTriggerSpell)selected.insert(e->EffectTriggerSpell);if(e->Effect==28)props.insert(e->EffectMiscValue[1]);
        }
        for(auto m:misc)if(m->SpellID==id){auto d=durations.LookupEntry(m->DurationIndex);std::cout<<"MISC\t"<<id<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<(d?d->Duration:0)<<'\n';}
    }
    for(auto id:props)if(auto p=properties.LookupEntry(id))std::cout<<"PROPERTIES\t"<<id<<'\t'<<p->Control<<'\t'<<p->Title<<'\t'<<p->Slot<<'\t'<<p->Flags<<'\n';
    auto v=vehicles.LookupEntry(4577);if(!v)return 8;
    std::cout<<"VEHICLE\t"<<v->ID<<'\t'<<v->Flags;
    for(auto seat:v->SeatID)std::cout<<'\t'<<seat;std::cout<<'\n';
    for(auto id:v->SeatID)if(id)if(auto seat=seats.LookupEntry(id))std::cout<<"SEAT\t"<<id<<'\t'<<seat->Flags<<'\t'<<seat->FlagsB<<'\t'<<seat->AttachmentOffset.X<<'\t'<<seat->AttachmentOffset.Y<<'\t'<<seat->AttachmentOffset.Z<<'\n';
    HotfixDatabase.Close();std::cout<<"EOF\tFuZanReturnCatalog\n";
}
