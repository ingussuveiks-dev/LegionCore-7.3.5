#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>

int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    std::string error;
    auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
    if (!config || !sConfigMgr->LoadInitial(config, {}, error)) return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements()) return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!check || !check->Fetch()[0].GetBool()) return 4;
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2", SpellDurationLoadInfo::Instance());
    DB2Storage<VehicleEntry> vehicles("Vehicle.db2", VehicleLoadInfo::Instance());
    DB2Storage<VehicleSeatEntry> seats("VehicleSeat.db2", VehicleSeatLoadInfo::Instance());
    if (!spells.Load(argv[1],0) || !effects.Load(argv[1],0) || !misc.Load(argv[1],0) ||
        !durations.Load(argv[1],0) || !vehicles.Load(argv[1],0) || !seats.Load(argv[1],0)) return 5;
    spells.LoadFromDB(); effects.LoadFromDB(); misc.LoadFromDB();
    durations.LoadFromDB(); vehicles.LoadFromDB(); seats.LoadFromDB();
    std::set<uint32> selected{46598,209132,209184,210247};
    for (bool changed=true; changed;)
    {
        changed=false;
        for (auto effect:effects)
            if (selected.count(effect->SpellID) && effect->EffectTriggerSpell)
                changed=selected.insert(effect->EffectTriggerSpell).second || changed;
    }
    for (auto id:selected)
    {
        auto spell=spells.LookupEntry(id); if (!spell) return 6;
        std::cout<<"SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';
        for (auto effect:effects) if (effect->SpellID==id)
            std::cout<<"EFFECT\t"<<id<<'\t'<<effect->DifficultyID<<'\t'<<effect->EffectIndex<<'\t'
                <<effect->Effect<<'\t'<<effect->EffectAura<<'\t'<<effect->EffectBasePoints<<'\t'
                <<effect->EffectAuraPeriod<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectMiscValue[1]<<'\t'
                <<effect->EffectTriggerSpell<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
        for (auto row:misc) if (row->SpellID==id)
        {
            auto duration=durations.LookupEntry(row->DurationIndex);
            std::cout<<"MISC\t"<<id<<'\t'<<unsigned(row->DifficultyID)<<'\t'<<row->DurationIndex<<'\t'
                <<(duration?duration->Duration:-2)<<'\t'<<row->RangeIndex;
            for (auto attribute:row->Attributes) std::cout<<'\t'<<uint32(attribute);
            std::cout<<'\n';
        }
    }
    auto vehicle=vehicles.LookupEntry(4717); if (!vehicle) return 7;
    std::cout<<"VEHICLE\t"<<vehicle->ID<<'\t'<<vehicle->Flags<<'\t'<<unsigned(vehicle->FlagsB)<<'\n';
    for (int index=0; index<8; ++index) if (vehicle->SeatID[index])
    {
        auto seat=seats.LookupEntry(vehicle->SeatID[index]); if (!seat) return 8;
        std::cout<<"SEAT\t"<<index<<'\t'<<seat->ID<<'\t'<<seat->Flags<<'\t'<<seat->FlagsB<<'\t'<<seat->FlagsC
            <<'\t'<<unsigned(seat->AttachmentID)<<'\t'<<seat->AttachmentOffset.X<<'\t'<<seat->AttachmentOffset.Y<<'\t'
            <<seat->AttachmentOffset.Z<<'\t'<<seat->EnterAnimStart<<'\t'<<seat->RideAnimLoop<<'\t'<<seat->ExitAnimStart<<'\n';
    }
    std::cout<<"EOF\twave-native\n";
    HotfixDatabase.Close(); MySQL::Library_End();
}
