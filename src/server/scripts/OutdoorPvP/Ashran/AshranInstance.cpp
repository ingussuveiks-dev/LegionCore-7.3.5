#include "AshranMgr.hpp"
#include "InstanceScript.h"
#include "QuestData.h"

namespace
{
template <class T> void CollectEntries(std::set<uint32>& entries, T const& spawn)
{
    if (spawn.entry)
        entries.insert(spawn.entry);
}
template <class T, size_t N> void CollectEntries(std::set<uint32>& entries, T const (&spawns)[N])
{
    for (auto const& spawn : spawns)
        CollectEntries(entries, spawn);
}

void OfferAshranQuests(Player* player)
{
    if (player->getLevel() < PlayerMinLevel || player->isGameMaster())
        return;
    uint32 quests[] = {player->GetTeamId() == TEAM_HORDE ? 38923u : 38925u,
                       player->GetTeamId() == TEAM_HORDE ? 39090u : 39096u};
    for (uint32 id : quests)
        if (player->GetQuestStatus(id) == QUEST_STATUS_NONE)
            if (Quest const* quest = sQuestDataStore->GetQuestTemplate(id))
                if (player->CanTakeQuest(quest, false) && player->CanAddQuest(quest, false))
                    player->AddQuest(quest, nullptr);
}
} // namespace

class instance_ashran : public InstanceMapScript
{
public:
    instance_ashran() : InstanceMapScript("instance_ashran", AshranMapID)
    {
    }

    struct instance_ashran_InstanceScript : InstanceScript
    {
        explicit instance_ashran_InstanceScript(InstanceMap* map) : InstanceScript(map)
        {
            _battle.SetMap(map);
            CollectEntries(_managedCreatures, g_RacingCreaturesPos);
            CollectEntries(_managedCreatures, g_StormshieldGladiators);
            CollectEntries(_managedCreatures, g_WarspearGladiators);
            CollectEntries(_managedCreatures, g_AllianceFangraal);
            CollectEntries(_managedCreatures, g_HordeKronus);
            CollectEntries(_managedCreatures, g_AllianceGuardian);
            CollectEntries(_managedCreatures, g_HordeGuardian);
            CollectEntries(_managedCreatures, g_WarlockGatewaysSpawns);
            CollectEntries(_managedCreatures, g_MagePortalsSpawns);
            CollectEntries(_managedCreatures, g_MarketplaceGraveyardSpirits);
            CollectEntries(_managedCreatures, g_EmberfallTowerSpiritHealer);
            CollectEntries(_managedCreatures, g_ArchmageOverwatchSpiritHealer);
            CollectEntries(_managedCreatures, g_BasesSpiritHealers);
            CollectEntries(_managedCreatures, g_FactionGuardians);
            CollectEntries(_managedCreatures, g_FactionBossesSpawn);
            CollectEntries(_managedCreatures, g_FactionBossesGuardians);
            CollectEntries(_managedCreatures, g_FactionTaxisToBase);
            CollectEntries(_managedCreatures, g_EmberfallTowerSpawns);
            CollectEntries(_managedCreatures, g_EmberfallTowerNeutralSpawns);
            CollectEntries(_managedCreatures, g_VolrathsAdvanceSpawns);
            CollectEntries(_managedCreatures, g_VolrathsAdvanceNeutralSpawns);
            CollectEntries(_managedCreatures, g_CrossroadSpawns);
            CollectEntries(_managedCreatures, g_CrossroadsNeutralSpawns);
            CollectEntries(_managedCreatures, g_TrembladesVanguardSpawns);
            CollectEntries(_managedCreatures, g_TrembladesVanguardNeutralSpawns);
            CollectEntries(_managedCreatures, g_ArchmageOverwatchSpawns);
            CollectEntries(_managedCreatures, g_ArchmageOverwatchNeutral);
            CollectEntries(_managedCreatures, g_Korlok);
            CollectEntries(_managedCreatures, g_AllianceChapion);
            CollectEntries(_managedCreatures, g_HordeChampion);
            CollectEntries(_managedObjects, g_RacingFlagsPos);
            CollectEntries(_managedObjects, g_AncientArtifactPos);
            CollectEntries(_managedObjects, g_WarlockGatewaysGob);
            CollectEntries(_managedObjects, g_MagePortalsGob);
            CollectEntries(_managedObjects, g_GraveyardBanner_H);
            CollectEntries(_managedObjects, g_GraveyardBanner_A);
            CollectEntries(_managedObjects, g_GraveyardBanner_N);
            CollectEntries(_managedObjects, g_EmberfallFiresSpawns);
            CollectEntries(_managedObjects, g_VolrathsAdvanceFires);
            CollectEntries(_managedObjects, g_CrossroadsBanners);
            CollectEntries(_managedObjects, g_TrembladesVanguardFires);
            CollectEntries(_managedObjects, g_ArchmageOverwatchFires);
            CollectEntries(_managedObjects, g_CapturePoint);
            _managedCreatures.insert(SLGGenericMoPLargeAoI);
            for (auto const& captain : g_AshranCaptains)
                _managedCreatures.insert(captain.Entry);
        }

        ~instance_ashran_InstanceScript() override
        {
            instance->OutdoorPvPList = nullptr;
        }

        uint32 GetCreatureEntry(uint32, CreatureData const* data) override
        {
            return _managedCreatures.count(data->id) ? 0 : data->id;
        }

        uint32 GetGameObjectEntry(uint32 guid, uint32 entry) override
        {
            // Runtime GUIDs are allocated above the database's maximum. Never
            // suppress the controller's own summons, only duplicate DB spawns.
            auto data = sObjectMgr->GetGOData(guid);
            return data && data->mapid == AshranMapID && _managedObjects.count(entry) ? 0 : entry;
        }

        void OnCreatureCreate(Creature* creature) override
        {
            if (_ready)
                _battle.OnCreatureCreate(creature);
            else
                _pendingCreatures.insert(creature->GetGUID());
        }
        void OnCreatureRemove(Creature* creature) override
        {
            _pendingCreatures.erase(creature->GetGUID());
            if (_ready)
                _battle.OnCreatureRemove(creature);
        }
        void OnGameObjectCreate(GameObject* object) override
        {
            if (_ready)
                _battle.OnGameObjectCreate(object);
            else
                _pendingObjects.insert(object->GetGUID());
        }
        void OnGameObjectRemove(GameObject* object) override
        {
            _pendingObjects.erase(object->GetGUID());
            if (_ready)
                _battle.OnGameObjectRemove(object);
        }

        void OnPlayerLeave(Player* player) override
        {
            if (!_areas.count(player->GetGUID()))
                return;
            _battle.HandlePlayerLeaveArea(player->GetGUID(), _areas[player->GetGUID()]);
            _battle.HandlePlayerLeaveMap(player->GetGUID(), AshranMapID);
            _areas.erase(player->GetGUID());
        }

        void FillInitialWorldStates(WorldPackets::WorldState::InitWorldStates& packet) override
        {
            if (_ready)
                _battle.FillInitialWorldStates(packet);
        }

        void InitializeBattle()
        {
            if (_ready)
                return;
            _controllers.insert(&_battle);
            instance->OutdoorPvPList = &_controllers;
            _battle.Initialize(AshranZoneID);
            _ready = true;
            auto creatures = std::move(_pendingCreatures);
            auto objects = std::move(_pendingObjects);
            for (auto guid : creatures)
                if (auto creature = instance->GetCreature(guid))
                    _battle.OnCreatureCreate(creature);
            for (auto guid : objects)
                if (auto object = instance->GetGameObject(guid))
                    _battle.OnGameObjectCreate(object);
        }

        void Update(uint32 diff) override
        {
            if (_poll > diff)
            {
                _poll -= diff;
                return;
            }
            _poll = 1000;
            if (instance->GetPlayers().isEmpty())
                return;
            InitializeBattle();
            // Map::Update owns the battle tick. Only registration/area/quest
            // transitions run here, after players have actually entered the map.
            for (auto const& reference : instance->GetPlayers())
            {
                Player* player = reference.getSource();
                if (!player || !player->IsInWorld() || player->GetTeamId() >= TEAM_NEUTRAL)
                    continue;
                auto found = _areas.find(player->GetGUID());
                uint32 area = player->GetAreaId();
                if (found == _areas.end())
                {
                    _battle.HandlePlayerEnterMap(player->GetGUID(), AshranZoneID);
                    if (player->getLevel() < PlayerMinLevel)
                        continue;
                    _battle.HandleBFMGREntryInviteResponse(true, player);
                    _areas[player->GetGUID()] = area;
                    _battle.HandlePlayerEnterArea(player->GetGUID(), area);
                    player->SendInitWorldStates(AshranZoneID, area);
                }
                else if (found->second != area)
                {
                    _battle.HandlePlayerLeaveArea(player->GetGUID(), found->second);
                    found->second = area;
                    _battle.HandlePlayerEnterArea(player->GetGUID(), area);
                }
                OfferAshranQuests(player);
            }
        }

        OutdoorPvPAshran _battle;
        std::set<OutdoorPvP*> _controllers;
        std::set<uint32> _managedCreatures, _managedObjects;
        GuidSet _pendingCreatures, _pendingObjects;
        std::map<ObjectGuid, uint32> _areas;
        uint32 _poll = 1000;
        bool _ready = false;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_ashran_InstanceScript(map);
    }
};

class player_ashran_entry : public PlayerScript
{
public:
    player_ashran_entry() : PlayerScript("player_ashran_entry")
    {
    }

    void OnUpdate(Player* player, uint32) override
    {
        if (player->GetMapId() != AshranNeutralMapID || player->GetZoneId() != 6941 ||
            player->getLevel() < PlayerMinLevel || !player->IsAlive() || player->isInCombat() || player->isInFlight() ||
            player->IsBeingTeleported() || player->GetTransport() || player->GetVehicle())
            return;
        uint32 area = player->GetAreaId();
        if (area == AshranPreAreaAlliance || area == AshranPreAreaHorde)
            return;
        player->SafeTeleport(AshranMapID, player);
    }
};

void AddSC_instance_ashran()
{
    new instance_ashran();
    new player_ashran_entry();
}
