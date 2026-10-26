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
    std::string error; auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
    if (!config || !sConfigMgr->LoadInitial(config, {}, error)) return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements()) return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!readOnly || !readOnly->Fetch()[0].GetBool()) return 4;
    DB2Storage<QuestObjectiveEntry> objectives("QuestObjective.db2", QuestObjectiveLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    if (!objectives.Load(argv[1], 0) || !effects.Load(argv[1], 0)) return 5;
    for (int overlay = 0; overlay < 2; ++overlay)
    {
        if (overlay) { objectives.LoadFromDB(); effects.LoadFromDB(); }
        uint32 count = 0;
        for (auto objective : objectives) if (objective->QuestID == 40419)
        {
            ++count;
            std::cout << "OBJECTIVE\t" << overlay << '\t' << objective->ID << '\t' << unsigned(objective->OrderIndex)
                << '\t' << unsigned(objective->StorageIndex) << '\t' << objective->ObjectID << '\t' << unsigned(objective->Flags) << '\n';
        }
        std::cout << "OBJECTIVE_COUNT\t" << overlay << '\t' << count << '\n';
        for (auto effect : effects) if (!effect->DifficultyID &&
            (effect->SpellID == 205040 || ((effect->Effect == 90 || effect->Effect == 134) &&
                (effect->EffectMiscValue[0] == 102548 || effect->EffectMiscValue[0] == 103481 || effect->EffectMiscValue[0] == 103490))))
            std::cout << "EFFECT\t" << overlay << '\t' << effect->SpellID << '\t' << effect->EffectIndex << '\t'
                << effect->Effect << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t'
                << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
    }
}
