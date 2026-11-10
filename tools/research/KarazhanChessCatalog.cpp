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
    if (!spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !misc.Load(argv[1], 0) || !durations.Load(argv[1], 0) || !times.Load(argv[1], 0)) return 5;
    spells.LoadFromDB(); effects.LoadFromDB(); misc.LoadFromDB(); durations.LoadFromDB(); times.LoadFromDB();
    std::set<uint32> selected{229468, 229493, 229567, 229570};
    for (auto spell : spells)
        if (std::string(spell->Name->Str[0]).find("Rook") != std::string::npos ||
            (spell->ID >= 229550 && spell->ID <= 229580))
            selected.insert(spell->ID);
    for (bool changed = true; changed;)
    {
        changed = false;
        for (auto effect : effects)
            if (selected.count(effect->SpellID) && effect->EffectTriggerSpell)
                changed = selected.insert(effect->EffectTriggerSpell).second || changed;
    }
    for (auto id : selected)
    {
        auto spell = spells.LookupEntry(id);
        if (!spell) { std::cout << "MISSING\t" << id << '\n'; continue; }
        std::cout << "SPELL\t" << id << '\t' << spell->Name->Str[0] << '\t' << spell->Description->Str[0] << '\n';
        for (auto effect : effects) if (effect->SpellID == id)
            std::cout << "EFFECT\t" << id << '\t' << unsigned(effect->DifficultyID) << '\t' << unsigned(effect->EffectIndex) << '\t' << effect->Effect << '\t' << effect->EffectAura << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t' << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
        for (auto row : misc) if (row->SpellID == id)
        {
            auto duration = durations.LookupEntry(row->DurationIndex);
            auto time = times.LookupEntry(row->CastingTimeIndex);
            std::cout << "TIMING\t" << id << '\t' << unsigned(row->DifficultyID) << '\t' << (duration ? duration->Duration : 0) << '\t' << (time ? time->Base : 0) << '\n';
        }
    }
    std::cout << "EOF\tkarazhan-chess\n";
    HotfixDatabase.Close(); MySQL::Library_End();
}
