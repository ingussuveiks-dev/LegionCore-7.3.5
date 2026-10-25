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
    if (argc != 2)
        return 1;
    std::string error;
    auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
    if (!config || !sConfigMgr->LoadInitial(config, {}, error))
        return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements())
        return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!check || !check->Fetch()[0].GetBool())
        return 4;
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2", ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    if (!steps.Load(argv[1], 0) || !trees.Load(argv[1], 0) || !criteria.Load(argv[1], 0))
        return 5;
    steps.LoadFromDB();
    trees.LoadFromDB();
    criteria.LoadFromDB();
    for (auto step : steps)
        if (step->ScenarioID == 909 || step->ScenarioID == 1068 || step->ScenarioID == 1082)
            std::cout << "SHARED_STEP\t" << step->ScenarioID << '\t' << unsigned(step->OrderIndex) << '\t'
                << step->ID << '\t' << step->Criteriatreeid << '\n';
    for (auto step : steps)
        if (step->ScenarioID == 1082)
        {
            std::cout << "STEP\t" << unsigned(step->OrderIndex) << '\t' << step->ID << '\t'
                << step->Criteriatreeid << '\t' << step->IsBonusObjective() << '\t' << step->Title->Str[0] << '\n';
            std::set<uint32> pending{uint32(step->Criteriatreeid)}, seen;
            while (!pending.empty())
            {
                uint32 identity = *pending.begin();
                pending.erase(identity);
                if (!seen.insert(identity).second)
                    continue;
                auto tree = trees.LookupEntry(identity);
                if (!tree)
                    return 6;
                if (auto criterion = criteria.LookupEntry(tree->CriteriaID))
                    std::cout << "CRITERION\t" << unsigned(step->OrderIndex) << '\t' << criterion->ID << '\t'
                        << unsigned(criterion->Type) << '\t' << criterion->Asset << '\t' << tree->Amount << '\n';
                for (auto child : trees)
                    if (uint32(child->Parent) == identity)
                        pending.insert(child->ID);
            }
        }

    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    DB2Storage<ItemChildEquipmentEntry> children("ItemChildEquipment.db2", ItemChildEquipmentLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> castTimes("SpellCastTimes.db2", SpellCastTimesLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2", SpellDurationLoadInfo::Instance());
    DB2Storage<SummonPropertiesEntry> summons("SummonProperties.db2", SummonPropertiesLoadInfo::Instance());
    DB2Storage<LockEntry> locks("Lock.db2", LockLoadInfo::Instance());
    DB2Storage<MapEntry> maps("Map.db2", MapLoadInfo::Instance());
    DB2Storage<VehicleEntry> vehicles("Vehicle.db2", VehicleLoadInfo::Instance());
    DB2Storage<VehicleSeatEntry> seats("VehicleSeat.db2", VehicleSeatLoadInfo::Instance());
    if (!items.Load(argv[1], 0) || !children.Load(argv[1], 0) || !effects.Load(argv[1], 0) ||
        !misc.Load(argv[1], 0) || !castTimes.Load(argv[1], 0) || !locks.Load(argv[1], 0) || !maps.Load(argv[1], 0) ||
        !vehicles.Load(argv[1], 0) || !seats.Load(argv[1], 0) || !durations.Load(argv[1], 0) || !summons.Load(argv[1], 0))
        return 7;
    items.LoadFromDB();
    children.LoadFromDB();
    effects.LoadFromDB();
    misc.LoadFromDB();
    castTimes.LoadFromDB();
    durations.LoadFromDB();
    summons.LoadFromDB();
    locks.LoadFromDB();
    maps.LoadFromDB();
    vehicles.LoadFromDB();
    seats.LoadFromDB();
    auto vehicle = vehicles.LookupEntry(4726);
    if (!vehicle)
        return 10;
    std::cout << "VEHICLE\t" << vehicle->ID << '\t' << vehicle->Flags << '\n';
    for (uint32 index = 0; index < 8; ++index)
        if (vehicle->SeatID[index])
        {
            auto seat = seats.LookupEntry(vehicle->SeatID[index]);
            if (!seat)
                return 11;
            std::cout << "SEAT\t4726\t" << index << '\t' << seat->ID << '\t' << seat->Flags << '\t' << seat->FlagsB
                << '\t' << seat->FlagsC << '\t' << seat->EnterUISoundID << '\t' << seat->ExitUISoundID << '\n';
        }
    std::set<uint32> equipment{128866};
    for (auto child : children)
        if (child->ItemID == 128866)
        {
            equipment.insert(child->ChildItemID);
            std::cout << "CHILD\t" << child->ItemID << '\t' << child->ChildItemID << '\t' << unsigned(child->ChildItemEquipSlot) << '\n';
        }
    for (uint32 identity : equipment)
    {
        auto item = items.LookupEntry(identity);
        if (!item)
            return 8;
        std::cout << "ITEM\t" << identity << '\t' << unsigned(item->ArtifactID) << '\t' << unsigned(item->InventoryType)
            << '\t' << item->Display->Str[0] << '\n';
    }
    for (uint32 identity : {210132, 210133, 225876, 3365, 210036, 210043, 209884, 209908, 210358, 210360, 52391})
    {
        if (identity == 209884 || identity == 210358)
        {
            for (auto effect : effects)
                if (effect->SpellID == identity && !effect->DifficultyID && effect->Effect == 28)
                {
                    auto properties = summons.LookupEntry(effect->EffectMiscValue[1]);
                    if (!properties)
                        return 10;
                    std::cout << "TRAVEL_SUMMON\t" << identity << '\t' << properties->ID << '\t' << properties->Control
                        << '\t' << properties->Title << '\t' << properties->Flags << '\t' << properties->Slot << '\n';
                }
            for (auto data : misc)
                if (data->SpellID == identity && !data->DifficultyID)
                {
                    auto duration = durations.LookupEntry(data->DurationIndex);
                    std::cout << "TRAVEL_DURATION\t" << identity << '\t' << data->DurationIndex << '\t'
                        << (duration ? duration->Duration : 0) << '\t' << (duration ? duration->MaxDuration : 0) << '\n';
                }
        }
        for (auto effect : effects)
            if (effect->SpellID == identity && !effect->DifficultyID)
                std::cout << "EFFECT\t" << identity << '\t' << effect->EffectIndex << '\t' << effect->Effect << '\t'
                    << effect->EffectAura << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t'
                    << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
        for (auto data : misc)
            if (data->SpellID == identity && !data->DifficultyID)
            {
                auto cast = castTimes.LookupEntry(data->CastingTimeIndex);
                std::cout << "CAST\t" << identity << '\t' << data->CastingTimeIndex << '\t'
                    << (cast ? cast->Base : 0) << '\t' << data->LaunchDelay << '\n';
            }
    }
    auto lock = locks.LookupEntry(1691);
    auto map = maps.LookupEntry(1495);
    if (!lock || !map)
        return 9;
    for (uint32 index = 0; index < MAX_LOCK_CASE; ++index)
        std::cout << "LOCK\t1691\t" << index << '\t' << unsigned(lock->Type[index]) << '\t' << lock->Index[index] << '\t' << lock->Skill[index] << '\n';
    std::cout << "MAP\t1495\t" << unsigned(map->InstanceType) << '\t' << unsigned(map->MaxPlayers) << '\n';

    HotfixDatabase.Close();
    MySQL::Library_End();
}
