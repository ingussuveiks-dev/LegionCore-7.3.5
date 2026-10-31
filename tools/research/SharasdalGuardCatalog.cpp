#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>

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
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2", ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    if (!steps.Load(argv[1], 0) || !trees.Load(argv[1], 0) || !criteria.Load(argv[1], 0) || !effects.Load(argv[1], 0)) return 5;
    steps.LoadFromDB(); trees.LoadFromDB(); criteria.LoadFromDB(); effects.LoadFromDB();
    for (auto step : steps)
        if (step->ScenarioID == 1066 && step->OrderIndex <= 2)
            std::cout << "STEP\t" << unsigned(step->OrderIndex) << '\t' << step->ID << '\t' << step->Criteriatreeid << '\n';
    for (uint32 id : {47122u, 47123u, 47197u})
    {
        auto tree = trees.LookupEntry(id);
        if (!tree) return 6;
        std::cout << "TREE\t" << tree->ID << '\t' << tree->Parent << '\t' << unsigned(tree->Operator) << '\t' << tree->Amount << '\t' << tree->CriteriaID << '\n';
        if (auto criterion = criteria.LookupEntry(tree->CriteriaID))
            std::cout << "CRITERION\t" << criterion->ID << '\t' << unsigned(criterion->Type) << '\t' << criterion->Asset << '\n';
    }
    for (auto effect : effects)
        if (effect->Effect == 61 && effect->EffectMiscValue[0] == 49957)
            std::cout << "PRODUCER\t" << effect->SpellID << '\t' << unsigned(effect->EffectIndex) << '\t' << unsigned(effect->DifficultyID) << '\n';
    HotfixDatabase.Close(); MySQL::Library_End();
}
