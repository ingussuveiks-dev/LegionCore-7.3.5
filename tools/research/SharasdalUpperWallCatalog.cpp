#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "MovementPackets.h"
#include "SpellAuraDefines.h"
#include "SpellInfo.h"
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
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
    DB2Storage<SpellRangeEntry> ranges("SpellRange.db2",SpellRangeLoadInfo::Instance());
    if (!spells.Load(argv[1],0) || !effects.Load(argv[1],0) || !misc.Load(argv[1],0) || !times.Load(argv[1],0) || !ranges.Load(argv[1],0)) return 5;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();times.LoadFromDB();ranges.LoadFromDB();
    std::set<uint32> selected{209904,209895};
    for(auto effect:effects) if(effect->Effect==61 && effect->EffectMiscValue[0]==49819) selected.insert(effect->SpellID);
    for(bool changed=true;changed;)
    {
        changed=false;
        for(auto effect:effects)
            if(selected.count(effect->SpellID) && effect->EffectTriggerSpell)
                changed=selected.insert(effect->EffectTriggerSpell).second || changed;
    }
    for (uint32 id : selected)
    {
        auto spell=spells.LookupEntry(id);
        if (!spell) return 6;
        std::cout << "SPELL\t" << id << '\t' << spell->Name->Str[0] << '\n';
        for(auto effect : effects) if(effect->SpellID==id)
        {
            std::cout << "EFFECT\t" << id << '\t' << unsigned(effect->DifficultyID) << '\t' << unsigned(effect->EffectIndex) << '\t' << effect->Effect << '\t' << effect->EffectAura << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t' << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
            SpellImplicitTargetInfo target(effect->ImplicitTarget[0]);
            std::cout<<"TARGET\t"<<id<<'\t'<<unsigned(effect->EffectIndex)<<'\t'<<target.GetCheckType()<<'\t'<<target.GetReferenceType()<<'\n';
        }
        for(auto row:misc) if(row->SpellID==id)
        {
            auto time=times.LookupEntry(row->CastingTimeIndex);auto range=ranges.LookupEntry(row->RangeIndex);
            std::cout<<"MISC\t"<<id<<'\t'<<unsigned(row->DifficultyID)<<'\t'<<row->CastingTimeIndex<<'\t'<<(time?time->Base:-1)<<'\t'<<row->RangeIndex<<'\t'<<(range?range->RangeMax[0]:-1)<<'\t'<<(range?range->RangeMax[1]:-1)<<'\n';
        }
    }
    HotfixDatabase.Close(); MySQL::Library_End();
}
