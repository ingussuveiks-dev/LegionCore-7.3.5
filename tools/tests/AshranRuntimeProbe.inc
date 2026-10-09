// Optional integration probe, included after instance_ashran in a validation
// build only. Never register this script in a production runtime build.
class world_ashran_runtime_probe : public WorldScript
{
public:
    world_ashran_runtime_probe() : WorldScript("world_ashran_runtime_probe")
    {
    }
    void OnStartup() override
    {
        TC_LOG_INFO("server", "ASHRAN_PROBE: begin actual instance lifecycle");
        Map* parent = sMapMgr->CreateBaseMap(AshranMapID);
        auto map =
            new InstanceMap(AshranMapID, 300000, sMapMgr->GenerateInstanceId(), DIFFICULTY_PVEVP_SCENARIO, parent);
        map->CreateInstanceData(nullptr);
        auto script = static_cast<instance_ashran::instance_ashran_InstanceScript*>(map->GetInstanceScript());
        ASSERT(script);
        script->InitializeBattle();
        script->_battle.InitializeControlPoints();
        for (uint32 type : {uint32(AllianceFactionBoss), uint32(HordeFactionBoss)})
        {
            Creature* boss = map->GetCreature(script->_battle.GetCreature(type));
            ASSERT(boss && boss->GetMap() == map && boss->GetOutdoorPvP() == &script->_battle);
            std::list<Creature*> nearby;
            boss->GetCreatureListWithEntryInGrid(nearby, boss->GetEntry(), 100.0f);
            ASSERT(nearby.size() == 1);
        }
        script->_battle.StartEvent(EventStadiumRacing);
        ObjectGuid first = script->_battle.GetCreature(SpeedyHordeRacerSpawn);
        ASSERT(map->GetCreature(first));
        script->_battle.EndEvent(EventStadiumRacing, false);
        map->RemoveAllObjectsInRemoveList();
        ASSERT(!map->GetCreature(first));
        script->_battle.StartEvent(EventStadiumRacing);
        ObjectGuid second = script->_battle.GetCreature(SpeedyHordeRacerSpawn);
        ASSERT(second != first && map->GetCreature(second));
        script->_battle.EndEvent(EventStadiumRacing, false);
        map->RemoveAllObjectsInRemoveList();
        TC_LOG_INFO("server", "ASHRAN_PROBE: bosses unique; controller ownership and racing restart passed");
        map->UnloadAll();
        TC_LOG_INFO("server", "ASHRAN_PROBE: grids unloaded; destroying controller");
        delete map;
        TC_LOG_INFO("server", "ASHRAN_PROBE: unload passed");
    }
};
