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
    char const* config=std::getenv("LEGION_DIALOGUE_CONFIG");
    std::string error;
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly=HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if(!readOnly||!readOnly->Fetch()[0].GetBool())return 4;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0))return 5;
    spells.LoadFromDB();effects.LoadFromDB();
    std::set<uint32> selected{198941,198950,46598};
    for(uint32 identity:selected){auto spell=spells.LookupEntry(identity);if(!spell)return 6;std::cout<<"SPELL\t"<<identity<<'\t'<<spell->Name->Str[0]<<'\n';}
    for(auto effect:effects)if(!effect->DifficultyID&&selected.count(effect->SpellID))
        std::cout<<"EFFECT\t"<<effect->SpellID<<'\t'<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'
            <<effect->EffectAura<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectTriggerSpell<<'\t'
            <<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\t'<<effect->EffectMiscValue[1]<<'\n';
    DB2Storage<SummonPropertiesEntry> properties("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
    DB2Storage<VehicleEntry> vehicles("Vehicle.db2",VehicleLoadInfo::Instance());
    DB2Storage<VehicleSeatEntry> seats("VehicleSeat.db2",VehicleSeatLoadInfo::Instance());
    if(!properties.Load(argv[1],0)||!vehicles.Load(argv[1],0)||!seats.Load(argv[1],0))return 7;
    properties.LoadFromDB();vehicles.LoadFromDB();seats.LoadFromDB();
    auto property=properties.LookupEntry(61);auto vehicle=vehicles.LookupEntry(4535);auto seat=seats.LookupEntry(16201);
    if(!property||!vehicle||!seat||vehicle->SeatID[0]!=16201)return 8;
    std::cout<<"SUMMON\t61\t"<<property->Control<<'\t'<<property->Title<<'\t'<<property->Flags<<'\n';
    std::cout<<"SEAT\t4535\t16201\t"<<seat->Flags<<'\t'<<seat->FlagsB<<'\t'<<seat->FlagsC<<'\t'<<seat->EnterMinDuration<<'\t'<<seat->EnterMaxDuration<<'\t'<<seat->ExitMinDuration<<'\t'<<seat->ExitMaxDuration<<'\n';
    std::cout<<"USABILITY\t16201\t"<<seat->CanEnterOrExit()<<'\t'<<seat->IsUsableByOverride()<<'\n';
    HotfixDatabase.Close();
}
