#ifndef LEGION_BROKEN_SHORE_CLASS_ROUTE_H
#define LEGION_BROKEN_SHORE_CLASS_ROUTE_H

// Map1500 is shared by three independent class scenarios. Its native Warrior
// LFG1176 intentionally has ScenarioID0; choosing the first map row selects a
// different class. Resolve both the scenario and its dungeon as one route.
namespace BrokenShoreClassRoute
{
    inline bool Applies(uint32 map, uint32 playerClass)
    {
        return map == 1500 && (playerClass == 1 || playerClass == 2 || playerClass == 12);
    }

    inline uint32 ScenarioId(uint32 playerClass)
    {
        return playerClass == 1 ? 933 : playerClass == 2 ? 775 : playerClass == 12 ? 961 : 0;
    }

    inline lfg::LFGDungeonData const* Dungeon(Player const* player, Difficulty difficulty, uint32 expectedScenario = 0)
    {
        if (!player || !Applies(1500, player->getClass())) return nullptr;
        uint32 scenario = ScenarioId(player->getClass());
        uint32 dungeon = player->getClass() == 1 ? 1176 : player->getClass() == 2 ? 906 : 1166;
        if (expectedScenario && expectedScenario != scenario) return nullptr;
        auto route = sObjectMgr->GetScenarioOnMap(1500, difficulty, player->GetTeam(), player->getClass(), 0);
        if (!route || route->ScenarioID != scenario || route->MapID != 1500 ||
            route->Class != player->getClass() || route->LfgDungeonID != dungeon ||
            (route->Team && route->Team != player->GetTeam()) ||
            (route->DifficultyID && route->DifficultyID != difficulty)) return nullptr;
        auto data = sLFGMgr->GetLFGDungeon(dungeon, player->GetTeam());
        if (!data || !data->dbc || data->map != 1500 || data->difficulty != difficulty ||
            (data->dbc->ScenarioID && data->dbc->ScenarioID != scenario)) return nullptr;
        return data;
    }
}
#endif
