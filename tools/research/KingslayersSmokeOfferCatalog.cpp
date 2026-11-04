// Matching client/hotfix evidence; this probe never applies a spell to an actor.
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
    auto check = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!check || !check->Fetch()[0].GetBool())
        return 4;
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2", SpellDurationLoadInfo::Instance());
    if (!spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) ||
        !misc.Load(argv[1], 0) || !durations.Load(argv[1], 0))
        return 5;
    spells.LoadFromDB(); effects.LoadFromDB(); misc.LoadFromDB(); durations.LoadFromDB();
    std::set<uint32> selected{227061};
    uint32 parents = 0;
    for (auto effect : effects)
        if (effect->EffectTriggerSpell == 227061)
        {
            ++parents;
            selected.insert(effect->SpellID);
            std::cout << "PARENT\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t' << effect->Effect << '\n';
        }
    std::cout << "REVERSE_TRIGGER_COUNT\t227061\t" << parents << '\n';
    for (uint32 id : selected)
    {
        auto spell = spells.LookupEntry(id);
        if (!spell)
            return 6;
        std::cout << "SPELL\t" << id << '\t' << spell->Name->Str[0] << '\t' << spell->Description->Str[0]
            << '\t' << spell->AuraDescription->Str[0] << '\n';
        for (auto row : misc)
            if (row->SpellID == id)
            {
                auto duration = durations.LookupEntry(row->DurationIndex);
                std::cout << "MISC\t" << id << '\t' << unsigned(row->DifficultyID) << '\t' << row->DurationIndex
                    << '\t' << (duration ? duration->Duration : 0) << '\t' << unsigned(row->SchoolMask);
                for (auto attributes : row->Attributes)
                    std::cout << '\t' << attributes;
                std::cout << '\n';
            }
        for (auto effect : effects)
            if (effect->SpellID == id)
                std::cout << "EFFECT\t" << id << '\t' << effect->DifficultyID << '\t' << effect->EffectIndex
                    << '\t' << effect->Effect << '\t' << effect->EffectAura << '\t' << effect->EffectBasePoints
                    << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectMiscValue[1]
                    << '\t' << effect->EffectTriggerSpell << '\t' << effect->ImplicitTarget[0]
                    << '\t' << effect->ImplicitTarget[1] << '\n';
    }
    std::cout << "EOF\tKingslayersSmokeOfferCatalog\n";
    HotfixDatabase.Close();
    MySQL::Library_End();
}
