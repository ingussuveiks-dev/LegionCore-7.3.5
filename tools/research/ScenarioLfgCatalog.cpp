#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    DB2Storage<LFGDungeonsEntry> lfg("LFGDungeons.db2", LfgDungeonsLoadInfo::Instance());
    DB2Storage<ScenarioEntry> scenarios("Scenario.db2", ScenarioLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    if (!lfg.Load(argv[1], 0) || !scenarios.Load(argv[1], 0) || !effects.Load(argv[1], 0)) return 2;
    std::set<int> maps{1554, 1557, 1460, 1522, 1498, 1600, 1533};
    for (auto row : lfg)
        if (maps.count(row->MapID))
        {
            auto scenario = scenarios.LookupEntry(row->ScenarioID);
            std::cout << "LFG\t" << row->ID << '\t' << row->MapID << '\t' << row->ScenarioID
                << '\t' << (scenario ? int(scenario->Type) : -1) << '\t' << int(row->DifficultyID)
                << '\t' << row->IsScenario() << '\t' << row->Name->Str[0] << '\n';
        }
    for (auto row : scenarios)
        if (row->Type == 5)
            std::cout << "BOOST\t" << row->ID << '\t' << int(row->Type) << '\t' << row->Name->Str[0] << '\n';
    for (auto row : effects)
        if (row->SpellID == 219912 || row->SpellID == 208514 || row->SpellID == 208517)
            std::cout << "EFFECT\t" << row->SpellID << '\t' << int(row->EffectIndex)
                << '\t' << row->Effect << '\t' << row->EffectTriggerSpell << '\t'
                << row->ImplicitTarget[0] << '\t' << row->EffectMiscValue[0] << '\n';
}
