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
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2", SpellDurationLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2", SpellCastTimesLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2", ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    DB2Storage<MapEntry> maps("Map.db2", MapLoadInfo::Instance());
    DB2Storage<MapDifficultyEntry> mapDifficulties("MapDifficulty.db2", MapDifficultyLoadInfo::Instance());
    if (!spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !misc.Load(argv[1], 0) ||
        !durations.Load(argv[1], 0) || !times.Load(argv[1], 0) || !steps.Load(argv[1], 0) ||
        !trees.Load(argv[1], 0) || !criteria.Load(argv[1], 0) || !maps.Load(argv[1], 0) || !mapDifficulties.Load(argv[1], 0)) return 4;
    spells.LoadFromDB(); effects.LoadFromDB(); misc.LoadFromDB(); durations.LoadFromDB();
    times.LoadFromDB(); steps.LoadFromDB(); trees.LoadFromDB(); criteria.LoadFromDB();
    maps.LoadFromDB(); mapDifficulties.LoadFromDB();
    if (auto map = maps.LookupEntry(1014))
        std::cout << "MAP\t" << map->ID << '\t' << unsigned(map->InstanceType) << '\t' << unsigned(map->MaxPlayers) << '\t' << map->MapName->Str[0] << '\n';
    for (auto entry : mapDifficulties) if (entry->MapID == 1014)
        std::cout << "MAP_DIFFICULTY\t" << entry->ID << '\t' << unsigned(entry->DifficultyID) << '\t' << unsigned(entry->MaxPlayers) << '\n';
    std::set<uint32> pending{193890}, seen;
    for (auto effect : effects) if (effect->EffectMiscValue[0] == 46951 || effect->EffectMiscValue[0] == 46952)
    {
        std::cout << "FINAL_ASSET_EFFECT\t" << effect->SpellID << '\t' << effect->Effect << '\t' << effect->EffectMiscValue[0] << '\t' << effect->ImplicitTarget[0] << '\n';
        if (effect->Effect == 61) pending.insert(effect->SpellID);
    }
    for (auto effect : effects) if (effect->EffectTriggerSpell == 197120 || effect->EffectTriggerSpell == 194020)
        std::cout << "CLICK_INCOMING\t" << effect->SpellID << '\t' << effect->Effect << '\t' << effect->EffectTriggerSpell << '\n';
    for (auto effect : effects) if ((effect->Effect == 90 || effect->Effect == 134) && effect->EffectMiscValue[0] == 98514)
        std::cout << "ARRIVAL_CREDIT_SPELL\t" << effect->SpellID << '\t' << effect->Effect << '\t' << effect->ImplicitTarget[0] << '\n';
    while (!pending.empty())
    {
        uint32 id = *pending.begin(); pending.erase(pending.begin());
        if (!seen.insert(id).second) continue;
        if (seen.size() > 80) return 5;
        auto spell = spells.LookupEntry(id);
        if (!spell) { std::cout << "MISSING\t" << id << '\n'; return 6; }
        std::cout << "SPELL\t" << id << '\t' << spell->Name->Str[0] << '\n';
        for (auto effect : effects) if (effect->SpellID == id)
        {
            std::cout << "EFFECT\t" << id << '\t' << effect->DifficultyID << '\t'
                << effect->EffectIndex << '\t' << effect->Effect << '\t' << effect->EffectAura << '\t'
                << effect->EffectMiscValue[0] << '\t' << effect->EffectMiscValue[1] << '\t'
                << effect->EffectTriggerSpell << '\t' << effect->ImplicitTarget[0] << '\t'
                << effect->ImplicitTarget[1] << '\t' << effect->EffectAuraPeriod << '\n';
            if (effect->EffectTriggerSpell) pending.insert(effect->EffectTriggerSpell);
        }
        for (auto entry : misc) if (entry->SpellID == id)
        {
            auto duration = durations.LookupEntry(entry->DurationIndex);
            auto time = times.LookupEntry(entry->CastingTimeIndex);
            std::cout << "MISC\t" << id << '\t' << unsigned(entry->DifficultyID) << '\t'
                << (duration ? duration->Duration : 0) << '\t' << (time ? time->Base : 0) << '\t' << entry->RangeIndex;
            for (auto attributes : entry->Attributes) std::cout << '\t' << uint32(attributes);
            std::cout << '\n';
        }
    }
    uint32 count = 0;
    for (auto criterion : criteria) if ((criterion->Type == 73 || criterion->Type == 92) && criterion->Asset == 46736)
    {
        std::cout << "CLICK_EVENT\t" << criterion->ID << '\t' << unsigned(criterion->Type) << '\t' << criterion->Asset << '\n';
        for (auto tree : trees) if (tree->CriteriaID == criterion->ID)
            for (auto step : steps)
            {
                std::set<uint32> visited;
                uint32 id = tree->ID;
                while (id && visited.insert(id).second)
                {
                    if (id == uint32(step->Criteriatreeid))
                        std::cout << "CLICK_EVENT_OWNER\t" << step->ScenarioID << '\t' << unsigned(step->OrderIndex) << '\t' << step->ID << '\n';
                    auto parent = trees.LookupEntry(id); if (!parent) break;
                    id = parent->Parent;
                }
            }
    }
    for (auto step : steps) if (step->ScenarioID == 943)
    {
        ++count;
        std::cout << "STEP\t" << step->ID << '\t' << unsigned(step->OrderIndex) << '\t'
            << step->Criteriatreeid << '\t' << step->Title->Str[0] << '\t' << step->Description->Str[0] << '\n';
        std::set<uint32> nodes{uint32(step->Criteriatreeid)}, visited;
        while (!nodes.empty())
        {
            uint32 id = *nodes.begin(); nodes.erase(nodes.begin());
            if (!visited.insert(id).second) continue;
            auto tree = trees.LookupEntry(id); if (!tree) return 7;
            std::cout << "TREE\t" << step->ID << '\t' << id << '\t' << tree->Parent << '\t'
                << int(tree->Operator) << '\t' << tree->Amount << '\t' << tree->CriteriaID << '\n';
            if (auto criterion = criteria.LookupEntry(tree->CriteriaID))
                std::cout << "CRITERION\t" << step->ID << '\t' << criterion->ID << '\t'
                    << unsigned(criterion->Type) << '\t' << criterion->Asset << '\t'
                    << criterion->ModifierTreeId << '\t' << unsigned(criterion->Flags) << '\n';
            for (auto child : trees) if (uint32(child->Parent) == id) nodes.insert(child->ID);
        }
    }
    std::cout << "COMPLETE\t" << seen.size() << '\t' << count << '\n';
    HotfixDatabase.Close();
    MySQL::Library_End();
    return count ? 0 : 8;
}
