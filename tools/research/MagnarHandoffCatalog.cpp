#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

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
    auto readOnly = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!readOnly || !readOnly->Fetch()[0].GetBool())
        return 4;
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2", SpellDurationLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> casts("SpellCastTimes.db2", SpellCastTimesLoadInfo::Instance());
    DB2Storage<SpellXSpellVisualEntry> visuals("SpellXSpellVisual.db2", SpellXSpellVisualLoadInfo::Instance());
    if (!spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !misc.Load(argv[1], 0) ||
        !durations.Load(argv[1], 0) || !casts.Load(argv[1], 0) || !visuals.Load(argv[1], 0))
        return 5;
    spells.LoadFromDB(); effects.LoadFromDB(); misc.LoadFromDB();
    durations.LoadFromDB(); casts.LoadFromDB(); visuals.LoadFromDB();
    std::set<uint32> wanted{188067, 188068, 188074, 189718, 189721, 190588, 192065};
    for (uint32 identity : wanted)
    {
        auto spell = spells.LookupEntry(identity);
        if (!spell)
        {
            std::cerr << "Missing native handoff candidate Spell " << identity << '\n';
            for (auto effect : effects)
                if (effect->SpellID == identity || (wanted.count(effect->SpellID) && effect->EffectTriggerSpell == identity))
                    std::cerr << "Candidate edge " << effect->SpellID << ' ' << effect->Effect << ' '
                        << effect->EffectMiscValue[0] << ' ' << effect->EffectTriggerSpell << '\n';
            return 6;
        }
        std::cout << "SPELL\t" << identity << '\t' << spell->Name->Str[0] << '\n';
        for (auto entry : misc)
            if (entry->SpellID == identity && !entry->DifficultyID)
            {
                auto duration = durations.LookupEntry(entry->DurationIndex);
                auto cast = casts.LookupEntry(entry->CastingTimeIndex);
                std::cout << "TIME\t" << identity << '\t' << (cast ? cast->Base : 0) << '\t'
                    << (duration ? duration->Duration : 0) << '\t' << (duration ? duration->MaxDuration : 0) << '\n';
            }
        for (auto effect : effects)
            if (effect->SpellID == identity && !effect->DifficultyID)
            {
                std::cout << "EFFECT\t" << identity << '\t' << effect->EffectIndex << '\t' << effect->Effect << '\t'
                    << effect->EffectAura << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectMiscValue[1]
                    << '\t' << effect->EffectBasePoints << '\t' << effect->EffectTriggerSpell << '\t'
                    << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
                if (effect->EffectTriggerSpell)
                    std::cout << "TRIGGER_REFERENCE\t" << identity << '\t' << effect->EffectTriggerSpell << '\t'
                        << bool(spells.LookupEntry(effect->EffectTriggerSpell)) << '\n';
            }
        for (auto visual : visuals)
            if (visual->SpellID == identity)
                std::cout << "VISUAL\t" << identity << '\t' << visual->SpellVisualID << '\t'
                    << unsigned(visual->DifficultyID) << '\n';
    }
    HotfixDatabase.Close();
    MySQL::Library_End();
}
