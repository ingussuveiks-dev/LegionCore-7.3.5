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
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<LFGDungeonsEntry> dungeons("LFGDungeons.db2", LfgDungeonsLoadInfo::Instance());
    if (!steps.Load(argv[1], 0) || !trees.Load(argv[1], 0) || !criteria.Load(argv[1], 0) || !spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !dungeons.Load(argv[1], 0))
        return 5;
    steps.LoadFromDB();
    trees.LoadFromDB();
    criteria.LoadFromDB();
    spells.LoadFromDB();
    effects.LoadFromDB();
    dungeons.LoadFromDB();
    bool foundDungeon = false;
    for (auto dungeon : dungeons)
        if (dungeon->ScenarioID == 944)
        {
            foundDungeon = true;
            std::cout << "DUNGEON\t" << dungeon->ID << '\t' << dungeon->MapID << '\t' << unsigned(dungeon->DifficultyID)
                << '\t' << int(dungeon->Faction) << '\t' << dungeon->GetMinGroupSize() << '\t' << dungeon->GetMaxGroupSize() << '\n';
        }
    if (!foundDungeon)
        std::cout << "NO_NATIVE_DUNGEON\t944\n";
    for (uint32 identity : {128264, 195996})
    {
        auto spell = spells.LookupEntry(identity);
        if (!spell)
            return 7;
        std::cout << "SPELL\t" << identity << '\t' << spell->Name->Str[0] << '\n';
        for (auto effect : effects)
            if (!effect->DifficultyID && effect->SpellID == identity)
                std::cout << "EFFECT\t" << identity << '\t' << effect->EffectIndex << '\t' << effect->Effect << '\t'
                    << effect->EffectAura << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t'
                    << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
    }
    for (auto step : steps)
        if (step->ScenarioID == 944)
        {
            std::cout << "STEP\t" << unsigned(step->OrderIndex) << '\t' << step->ID << '\t'
                << step->Title->Str[0] << '\t' << step->Description->Str[0] << '\n';
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
                for (auto descendant : trees)
                    if (uint32(descendant->Parent) == identity)
                        pending.insert(descendant->ID);
            }
        }
    HotfixDatabase.Close();
    MySQL::Library_End();
}
