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
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
    DB2Storage<SpellRangeEntry> ranges("SpellRange.db2",SpellRangeLoadInfo::Instance());
    if (!spells.Load(argv[1],0) || !effects.Load(argv[1],0) || !misc.Load(argv[1],0) || !times.Load(argv[1],0) || !ranges.Load(argv[1],0)) return 5;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();times.LoadFromDB();ranges.LoadFromDB();
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    DB2Storage<VehicleEntry> vehicles("Vehicle.db2",VehicleLoadInfo::Instance());
    DB2Storage<VehicleSeatEntry> seats("VehicleSeat.db2",VehicleSeatLoadInfo::Instance());
    if(!durations.Load(argv[1],0)||!vehicles.Load(argv[1],0)||!seats.Load(argv[1],0))return 7;
    durations.LoadFromDB();vehicles.LoadFromDB();seats.LoadFromDB();
    if(auto vehicle=vehicles.LookupEntry(4720)) for(int index=0;index<8;++index)if(vehicle->SeatID[index]) {
        auto seat=seats.LookupEntry(vehicle->SeatID[index]);if(!seat)return 8;
        std::cout<<"SEAT\t"<<4720<<'\t'<<index<<'\t'<<seat->ID<<'\t'<<seat->Flags<<'\t'<<seat->FlagsB<<'\t'<<seat->FlagsC<<'\t'<<unsigned(seat->AttachmentID)<<'\t'<<seat->AttachmentOffset.X<<'\t'<<seat->AttachmentOffset.Y<<'\t'<<seat->AttachmentOffset.Z<<'\t'<<seat->EnterAnimStart<<'\t'<<seat->RideAnimLoop<<'\t'<<seat->ExitAnimStart<<'\t'<<seat->VehicleEnterAnimKitID<<'\t'<<seat->VehicleRideAnimKitID<<'\t'<<seat->VehicleExitAnimKitID<<'\t'<<seat->VehicleEnterAnim<<'\t'<<seat->VehicleRideAnimLoop<<'\t'<<seat->VehicleExitAnim<<'\t'<<unsigned(seat->VehicleEnterAnimBone)<<'\t'<<unsigned(seat->VehicleExitAnimBone)<<'\t'<<unsigned(seat->VehicleRideAnimLoopBone)<<'\n';
    }
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    if(!displays.Load(argv[1],0)||!models.Load(argv[1],0))return 14;
    displays.LoadFromDB();models.LoadFromDB();
    auto display=displays.LookupEntry(69084);if(!display)return 15;
    auto model=models.LookupEntry(display->ModelID);if(!model)return 16;
    std::cout<<"MODEL\t"<<display->ID<<'\t'<<display->ModelID<<'\t'<<model->FileDataID<<'\t'<<display->CreatureModelScale<<'\t'<<model->ModelScale<<'\n';
    std::set<uint32> selected{209903,209983,209996,210213,210214};
    for(auto spell:spells) if(spell->ID>=209430 && spell->ID<=209580)selected.insert(spell->ID);
    for(bool changed=true;changed;)
    {
        changed=false;
        for(auto effect:effects)
            if(selected.count(effect->SpellID) && effect->EffectTriggerSpell)
                changed=selected.insert(effect->EffectTriggerSpell).second || changed;
    }
    for (uint32 id : selected)
    {
        auto spell=spells.LookupEntry(id);
        if (!spell) return 6;
        std::cout << "SPELL\t" << id << '\t' << spell->Name->Str[0] << '\n';
        for(auto effect : effects) if(effect->SpellID==id)
        {
            std::cout << "EFFECT\t" << id << '\t' << unsigned(effect->DifficultyID) << '\t' << unsigned(effect->EffectIndex) << '\t' << effect->EffectBasePoints << '\t' << effect->Effect << '\t' << effect->EffectAura << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t' << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
            SpellImplicitTargetInfo target(effect->ImplicitTarget[0]);
            std::cout<<"TARGET\t"<<id<<'\t'<<unsigned(effect->EffectIndex)<<'\t'<<target.GetCheckType()<<'\t'<<target.GetReferenceType()<<'\n';
        }
        for(auto row:misc) if(row->SpellID==id)
        {
            auto time=times.LookupEntry(row->CastingTimeIndex);auto range=ranges.LookupEntry(row->RangeIndex);auto duration=durations.LookupEntry(row->DurationIndex);
            std::cout<<"DURATION\t"<<id<<'\t'<<row->DurationIndex<<'\t'<<(duration?duration->Duration:-2)<<'\n';
            std::cout<<"MISC\t"<<id<<'\t'<<unsigned(row->DifficultyID)<<'\t'<<row->CastingTimeIndex<<'\t'<<(time?time->Base:-1)<<'\t'<<row->RangeIndex<<'\t'<<(range?range->RangeMax[0]:-1)<<'\t'<<(range?range->RangeMax[1]:-1)<<'\n';
        }
    }
    std::cout<<"EOF\t"<<"living-lady"<<'\n';
    HotfixDatabase.Close(); MySQL::Library_End();
}
