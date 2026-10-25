#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
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
    DB2Storage<MapEntry> maps("Map.db2", MapLoadInfo::Instance());
    DB2Storage<LFGDungeonsEntry> dungeons("LFGDungeons.db2", LfgDungeonsLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    if (!maps.Load(argv[1], 0) || !dungeons.Load(argv[1], 0) || !spells.Load(argv[1], 0) || !effects.Load(argv[1], 0))
        return 4;
    maps.LoadFromDB();
    dungeons.LoadFromDB();
    spells.LoadFromDB();
    effects.LoadFromDB();
    auto map = maps.LookupEntry(1495);
    if (!map)
        return 5;
    std::cout << "MAP\t" << map->ID << '\t' << unsigned(map->InstanceType) << '\t' << unsigned(map->MaxPlayers) << '\n';
    for (auto dungeon : dungeons)
        if (dungeon->ScenarioID == 909)
            std::cout << "DUNGEON\t" << dungeon->ID << '\t' << dungeon->MapID << '\t'
                << dungeon->GetMinGroupSize() << '\t' << dungeon->GetMaxGroupSize() << '\n';
    for (uint32 identity : {57755u, 191162u, 191187u, 191149u})
    {
        auto spell = spells.LookupEntry(identity);
        if (!spell)
            return 6;
        std::cout << "SPELL\t" << identity << '\t' << spell->Name->Str[0] << '\n';
        for (auto effect : effects)
            if (effect->SpellID == identity && !effect->DifficultyID)
                std::cout << "EFFECT\t" << identity << '\t' << unsigned(effect->EffectIndex) << '\t'
                    << effect->Effect << '\t' << effect->EffectAura << '\t' << effect->EffectTriggerSpell << '\t'
                    << unsigned(effect->ImplicitTarget[0]) << '\t' << unsigned(effect->ImplicitTarget[1]) << '\n';
    }
    HotfixDatabase.Close();
    MySQL::Library_End();
}
