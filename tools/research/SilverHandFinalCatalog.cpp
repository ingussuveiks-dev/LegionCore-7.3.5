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
    std::string error;
    auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
    if (argc != 2 || !config || !sConfigMgr->LoadInitial(config, {}, error))
        return 1;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements())
        return 2;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!check || !check->Fetch()[0].GetBool())
        return 3;
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> miscellaneous("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellXSpellVisualEntry> links("SpellXSpellVisual.db2", SpellXSpellVisualLoadInfo::Instance());
    DB2Storage<SpellVisualEntry> visuals("SpellVisual.db2", SpellVisualLoadInfo::Instance());
    if (!spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !miscellaneous.Load(argv[1], 0) ||
        !links.Load(argv[1], 0) || !visuals.Load(argv[1], 0))
        return 4;
    spells.LoadFromDB();
    effects.LoadFromDB();
    miscellaneous.LoadFromDB();
    links.LoadFromDB();
    visuals.LoadFromDB();
    for (auto spell : spells)
        if (spell->ID == 215258 || spell->ID == 215260)
            std::cout << "SPELL\t" << spell->ID << '\t' << spell->Name->Str[0] << '\n';
    for (auto effect : effects)
        if (effect->SpellID == 215258 || effect->SpellID == 215260)
            std::cout << "EFFECT\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t' << effect->Effect << '\t'
                << effect->EffectAura << '\t' << effect->EffectTriggerSpell << '\t' << effect->ImplicitTarget[0] << '\t'
                << effect->ImplicitTarget[1] << '\n';
    for (auto entry : miscellaneous)
        if (entry->SpellID == 215258 || entry->SpellID == 215260)
            std::cout << "MISC\t" << entry->SpellID << '\t' << entry->DurationIndex << '\t' << entry->CastingTimeIndex << '\n';
    for (auto link : links)
        if (link->SpellID == 215258 || link->SpellID == 215260)
        {
            auto visual = visuals.LookupEntry(link->SpellVisualID);
            std::cout << "VISUAL\t" << link->SpellID << '\t' << link->SpellVisualID << '\t' << bool(visual) << '\t'
                << link->CasterPlayerConditionID << '\t' << link->ViewerPlayerConditionID << '\n';
        }
    HotfixDatabase.Close();
    MySQL::Library_End();
}
