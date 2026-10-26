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
    if (!config || !sConfigMgr->LoadInitial(config, {}, error)) return 4;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements()) return 5;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!readOnly || !readOnly->Fetch()[0].GetBool()) return 6;
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    if (!effects.Load(argv[1], 0)) return 2;
    effects.LoadFromDB();
    bool jump = false;
    for (auto effect : effects)
        if (effect && (effect->SpellID == 6544 || effect->SpellID == 94954))
        {
            std::cout << effect->SpellID << ' ' << effect->EffectIndex << ' ' << effect->DifficultyID
                << " effect=" << effect->Effect << " misc=" << effect->EffectMiscValue[0] << ',' << effect->EffectMiscValue[1]
                << " trigger=" << effect->EffectTriggerSpell << " targets=" << effect->ImplicitTarget[0] << ',' << effect->ImplicitTarget[1] << '\n';
            if (effect->SpellID == 94954 && effect->EffectIndex == 1 && effect->Effect == 42 && effect->EffectMiscValue[0] == 25) jump = true;
        }
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2", ScenarioStepLoadInfo::Instance());
    if (!criteria.Load(argv[1], 0) || !trees.Load(argv[1], 0) || !steps.Load(argv[1], 0)) return 7;
    criteria.LoadFromDB(); trees.LoadFromDB(); steps.LoadFromDB();
    std::set<int32> lessonTrees;
    for (auto node : trees)
        if (auto criterion = criteria.LookupEntry(node->CriteriaID))
            if (criterion->Type == 29 && criterion->Asset == 6544)
            {
                auto parent = node;
                for (unsigned depth = 0; parent && depth < 32; ++depth)
                {
                    lessonTrees.insert(parent->ID);
                    parent = trees.LookupEntry(parent->Parent);
                }
            }
    bool lesson = false;
    for (auto step : steps)
        if (lessonTrees.count(step->Criteriatreeid))
        {
            std::cout << "Leap lesson scenario=" << step->ScenarioID << " step=" << unsigned(step->OrderIndex)
                << " tree=" << step->Criteriatreeid << " title=" << step->Title->Str[0] << '\n';
            lesson = true;
        }
    return jump && lesson ? 0 : 3;
}
