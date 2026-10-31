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
    if (!config || !sConfigMgr->LoadInitial(config, {}, error)) return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements()) return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!check || !check->Fetch()[0].GetBool()) return 4;
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2", SpellDurationLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2", SpellCastTimesLoadInfo::Instance());
    DB2Storage<SpellInterruptsEntry> interrupts("SpellInterrupts.db2", SpellInterruptsLoadInfo::Instance());
    DB2Storage<SpellRangeEntry> ranges("SpellRange.db2", SpellRangeLoadInfo::Instance());
    if (!spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !misc.Load(argv[1], 0) ||
        !durations.Load(argv[1], 0) || !times.Load(argv[1], 0) || !interrupts.Load(argv[1], 0) || !ranges.Load(argv[1], 0)) return 5;
    spells.LoadFromDB(); effects.LoadFromDB(); misc.LoadFromDB();
    durations.LoadFromDB(); times.LoadFromDB(); interrupts.LoadFromDB();
    ranges.LoadFromDB();
    for (auto effect : effects)
    {
        if (effect->Effect == 61 && effect->EffectMiscValue[0] == 49816)
            std::cout << "PRODUCER\t" << effect->SpellID << '\t' << effect->DifficultyID << '\t' << effect->EffectIndex << '\n';
        if (effect->EffectTriggerSpell == 208171)
            std::cout << "TRIGGER_IN\t" << effect->SpellID << '\t' << effect->DifficultyID << '\t' << effect->EffectIndex << '\n';
    }
    std::set<uint32> pending{208171}, seen;
    while (!pending.empty())
    {
        uint32 id = *pending.begin(); pending.erase(pending.begin());
        if (!seen.insert(id).second) continue;
        if (seen.size() > 80) return 6;
        auto spell = spells.LookupEntry(id);
        if (!spell) { std::cout << "MISSING\t" << id << '\n'; continue; }
        std::cout << "SPELL\t" << id << '\t' << spell->Name->Str[0] << '\n';
        for (auto effect : effects)
            if (effect->SpellID == id)
            {
                std::cout << "EFFECT\t" << id << '\t' << effect->DifficultyID << '\t' << effect->EffectIndex << '\t'
                    << effect->Effect << '\t' << effect->EffectAura << '\t' << effect->EffectBasePoints << '\t'
                    << effect->EffectMiscValue[0] << '\t' << effect->EffectMiscValue[1] << '\t' << effect->EffectTriggerSpell
                    << '\t' << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\t' << effect->EffectAuraPeriod << '\n';
                if (effect->EffectTriggerSpell) pending.insert(effect->EffectTriggerSpell);
            }
        for (auto entry : misc)
            if (entry->SpellID == id)
            {
                auto duration = durations.LookupEntry(entry->DurationIndex);
                auto cast = times.LookupEntry(entry->CastingTimeIndex);
                std::cout << "MISC\t" << id << '\t' << unsigned(entry->DifficultyID) << '\t' << (duration ? duration->Duration : 0)
                    << '\t' << (cast ? cast->Base : 0) << '\t' << entry->RangeIndex;
                for (auto attribute : entry->Attributes) std::cout << '\t' << uint32(attribute);
                std::cout << '\n';
                if (auto range = ranges.LookupEntry(entry->RangeIndex))
                    std::cout << "RANGE\t" << id << '\t' << range->ID << '\t' << range->RangeMin[0] << '\t' << range->RangeMin[1]
                        << '\t' << range->RangeMax[0] << '\t' << range->RangeMax[1] << '\t' << unsigned(range->Flags) << '\n';
            }
        for (auto entry : interrupts)
            if (entry->SpellID == id)
                std::cout << "INTERRUPTS\t" << id << '\t' << unsigned(entry->DifficultyID) << '\t' << entry->InterruptFlags << '\t'
                    << entry->AuraInterruptFlags[0] << '\t' << entry->AuraInterruptFlags[1] << '\t'
                    << entry->ChannelInterruptFlags[0] << '\t' << entry->ChannelInterruptFlags[1] << '\n';
    }
    HotfixDatabase.Close();
    MySQL::Library_End();
}
