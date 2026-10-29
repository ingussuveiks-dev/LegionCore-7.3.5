#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>

// Matching client data and read-only hotfixes; no character/world writes.
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
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    DB2Storage<ItemChildEquipmentEntry> children("ItemChildEquipment.db2", ItemChildEquipmentLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2", ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    if (!spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !items.Load(argv[1], 0) || !children.Load(argv[1], 0) ||
        !steps.Load(argv[1], 0) || !trees.Load(argv[1], 0) || !criteria.Load(argv[1], 0)) return 4;
    spells.LoadFromDB(); effects.LoadFromDB(); items.LoadFromDB(); children.LoadFromDB();
    steps.LoadFromDB(); trees.LoadFromDB(); criteria.LoadFromDB();
    std::set<uint32> selected{199963,199960,208779,207560,207562,207582,214714,214715};
    for (auto id : selected)
    {
        auto spell = spells.LookupEntry(id);
        if (!spell) return 5;
        std::cout << "SPELL\t" << id << '\t' << spell->Name->Str[0] << '\n';
    }
    for (auto effect : effects)
        if (selected.count(effect->SpellID))
        {
            std::cout << "EFFECT\t" << effect->SpellID << '\t' << effect->DifficultyID << '\t' << effect->EffectIndex << '\t'
                << effect->Effect << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t'
                << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
            if (effect->Effect == 6)
                std::cout << "AURA\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t'
                    << effect->EffectAura << '\t' << effect->EffectAuraPeriod << '\n';
        }
    for (uint32 id : {128821u,128306u,128860u})
    {
        auto item = items.LookupEntry(id);
        if (!item) return 6;
        std::cout << "ITEM\t" << id << '\t' << item->MaxCount << '\t' << item->Stackable << '\t' << item->AllowableClass << '\n';
    }
    for (auto child : children)
        if (child->ItemID == 128821 || child->ItemID == 128306 || child->ItemID == 128860)
            std::cout << "CHILD\t" << child->ItemID << '\t' << child->ChildItemID << '\t' << unsigned(child->ChildItemEquipSlot) << '\n';
    for (auto step : steps)
        if (step->ScenarioID == 990 || step->ScenarioID == 1014 || step->ScenarioID == 1061 || step->ScenarioID == 1108)
        {
            std::cout << "STEP\t" << step->ScenarioID << '\t' << unsigned(step->OrderIndex) << '\t' << step->Title->Str[0] << '\n';
            std::set<uint32> pending{uint32(step->Criteriatreeid)}, seen;
            while (!pending.empty())
            {
                uint32 id = *pending.begin(); pending.erase(pending.begin());
                if (!seen.insert(id).second) continue;
                auto tree = trees.LookupEntry(id);
                if (!tree) return 7;
                if (auto criterion = criteria.LookupEntry(tree->CriteriaID))
                    std::cout << "CRITERION\t" << step->ScenarioID << '\t' << unsigned(step->OrderIndex) << '\t'
                        << unsigned(criterion->Type) << '\t' << criterion->Asset << '\t' << tree->Amount << '\n';
                for (auto child : trees) if (uint32(child->Parent) == id) pending.insert(child->ID);
            }
        }
    HotfixDatabase.Close();
    MySQL::Library_End();
}
