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
    DB2Storage<QuestObjectiveEntry> objectives("QuestObjective.db2", QuestObjectiveLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2", ScenarioStepLoadInfo::Instance());
    if (!objectives.Load(argv[1], 0) || !steps.Load(argv[1], 0)) return 5;
    objectives.LoadFromDB(); steps.LoadFromDB();
    std::cout << "SIZE\tQuestObjective\t" << sizeof(QuestObjectiveEntry) << '\t' << objectives.GetNumRows() << '\n';
    uint32 count = 0, selected = 0;
    for (auto objective : objectives)
    {
        ++count;
        if (objective->QuestID == 40341 || objective->ObjectID == 105805 || objective->ID == 283383 || objective->ID == 283384 || objective->ID == 283385 || objective->ID == 285665)
        {
            ++selected;
            std::cout << "OBJECTIVE\t" << objective->ID << '\t' << objective->QuestID << '\t'
                << unsigned(objective->Type) << '\t' << unsigned(objective->StorageIndex) << '\t'
                << objective->ObjectID << '\t' << objective->Amount << '\t' << unsigned(objective->Flags) << '\n';
        }
    }
    std::cout << "CENSUS\tQuestObjective\t" << count << '\t' << selected << '\n';
    for (auto step : steps)
        if (step->ScenarioID == 1066)
            std::cout << "STEP\t" << unsigned(step->OrderIndex) << '\t' << step->ID << '\t' << step->Criteriatreeid << '\n';
    std::cout << "EOF\terunak-native\n";
    HotfixDatabase.Close(); MySQL::Library_End();
}
