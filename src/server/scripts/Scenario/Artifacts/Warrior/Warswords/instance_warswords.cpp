/*
    Warrior: Warswords
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "MapManager.h"
#include <array>
#include <memory>

namespace WarswordsArrival
{
    bool IsParticipant(Player* player, uint32 instanceId, InstanceScript const* script)
    {
        if (!player || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() ||
            player->IsBeingTeleported() || player->getClass() != CLASS_WARRIOR || !player->GetMap() ||
            player->GetMapId() != 1511 || player->GetInstanceId() != instanceId ||
            player->GetInstanceScript() != script || player->GetQuestRewardStatus(40043) ||
            (player->GetQuestStatus(40043) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40043) != QUEST_STATUS_COMPLETE))
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(instanceId);
        return scenario && scenario->GetScenarioId() == 944;
    }

    bool CanLand(Player* player, uint32 instanceId, InstanceScript const* script)
    {
        if (!IsParticipant(player, instanceId, script))
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(instanceId);
        return scenario && scenario->GetCurrentStep() == 0;
    }
}

#include "warswords_waves.h"
#include "warswords_arrival.h"

class instance_warswords : public InstanceMapScript
{
public:
    instance_warswords() : InstanceMapScript("instance_warswords", 1511) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_warswords_InstanceMapScript(map);
    }

    struct instance_warswords_InstanceMapScript : public InstanceScript
    {
        instance_warswords_InstanceMapScript(InstanceMap* map) : InstanceScript(map), waves(map) {}

        std::map<uint32, ObjectGuid> objects;
        WarswordsWaves::State waves;
        std::array<bool, 4> destroyedRunestones{};
        bool myscenario;
        WorldLocation graveyard;
        std::shared_ptr<uint8> sceneLifetime = std::make_shared<uint8>();

        WorldLocation* GetClosestGraveYard(float /*x*/, float /*y*/, float /*z*/) override
        {
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario)
                return nullptr;

            uint32 safeId = 0;
            switch (scenario->GetScenarioId())
            {
                case 944:
                    // 193335 enters in mid-air; corpses use grounded Fury GY1.
                    safeId = 5228;
                    break;
                case 973:
                    // Native step6 harpoon credit47608 acknowledges crossing
                    // the wards. Do not strand a later death behind that wall
                    // or send an earlier death across it.
                    safeId = scenario->GetCurrentStep() >= 7 ? 5296 : 5295;
                    break;
                default:
                    return nullptr;
            }
            WorldSafeLocsEntry const* safe = sWorldSafeLocsStore.LookupEntry(safeId);
            if (!safe || safe->MapID != 1511 || !(safe->Loc.X || safe->Loc.Y || safe->Loc.Z) ||
                !MapManager::IsValidMapCoord(safe->MapID, safe->Loc.X, safe->Loc.Y, safe->Loc.Z, safe->Loc.O * float(M_PI) / 180.0f))
            {
                if (safeId == 5228)
                    TC_LOG_ERROR("scripts", "Fury graveyard: invalid native WorldSafeLocs 5228 for instance %u", instance->GetInstanceId());
                else
                    TC_LOG_ERROR("scripts", "Survival graveyard: invalid native WorldSafeLocs %u for instance %u", safeId, instance->GetInstanceId());
                return nullptr;
            }
            graveyard.WorldRelocate(safe->MapID, safe->Loc.X, safe->Loc.Y, safe->Loc.Z, safe->Loc.O * float(M_PI) / 180.0f);
            return &graveyard;
        }

        void Initialize() override
        {
            sceneLifetime = std::make_shared<uint8>();
            waves.Reset();
            destroyedRunestones.fill(false);
            myscenario = false;
        }

        void OnPlayerEnter(Player* player) override
        {
            WarswordsArrivalRecovery::Enter(player);
        }

        void OnPlayerLeave(Player* player) override
        {
            WarswordsArrivalRecovery::Forget(player);
        }

        uint32 GetData(uint32 type) const override
        {
            if (type >= 134130 && type <= 134133)
                return destroyedRunestones[type - 134130];
            return 0;
        }

        void SetData(uint32 type,uint32 value) override
        {
            if (type >= 134130 && type <= 134133)
            {
                Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
                if (value <= 1 && scenario && scenario->GetScenarioId() == 944 && scenario->GetCurrentStep() == 3)
                    destroyedRunestones[type - 134130] = value != 0;
                return;
            }
            if (type==WarswordsArrivalRecovery::Ready && value==1)
            {
                myscenario=true;
                if (getScenarionStep()==1) waves.Start();
            }
        }


        void onScenarionNextStep(uint32 newStep) override
        {
            if (newStep == 0)
                if (Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId()))
                    if (scenario->GetScenarioId() == 944)
                        destroyedRunestones.fill(false); // A new native attempt, not a GO reload.
            for (uint32 entry : {98007u, 98602u})
            {
                auto actor = objects.find(entry);
                if (actor != objects.end())
                    if (Creature* creature = instance->GetCreature(actor->second))
                        creature->UpdateObjectVisibility();
            }
            if (!myscenario)
                return;

            switch (newStep)
            {
                case 1:
                    waves.Start();
                    break;
                case 4:
                {
                    Map::PlayerList const &PlList = instance->GetPlayers();
                    if (PlList.isEmpty())
                        return;

                    for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    {
                        Player* player = i->getSource();
                        if (!WarswordsArrival::IsParticipant(player, instance->GetInstanceId(), this))
                            continue;

                        uint32 instanceId = instance->GetInstanceId();
                        auto attempt = player->GetQuestCancellationReceipt(40043);
                        std::weak_ptr<uint8> lifetime = sceneLifetime;
                        player->AddDelayedEvent(3000, [player, instanceId, script = this, attempt, lifetime]() -> void
                        {
                            // The Player owns the queue, but can leave this story
                            // before it runs. Never dereference the old instance.
                            if (lifetime.expired() || !attempt || attempt->load() ||
                                !WarswordsArrival::IsParticipant(player, instanceId, script) ||
                                attempt != player->GetQuestCancellationReceipt(40043))
                                return;
                            Scenario* scenario = sScenarioMgr->GetScenario(instanceId);
                            if (!scenario || scenario->GetCurrentStep() != 4)
                                return;
                            player->CastSpell(player, 189740); // scene
                        });
                        break;
                    }
                }
            }
        }

        void OnGameObjectCreate(GameObject* object) override
        {
            uint64 guid = object->GetDBTableGUIDLow();
            if (object->GetEntry() == 244940 && guid >= 134130 && guid <= 134133 && destroyedRunestones[guid - 134130])
                object->SetGoState(GO_STATE_ACTIVE);
        }

        void OnCreatureCreate(Creature* creature) override
        {
            if (creature->GetEntry() == 98007 || creature->GetEntry() == 98602)
                objects[creature->GetEntry()] = creature->GetGUID();

        }

        void OnCreatureRemove(Creature* creature) override
        {
            InstanceScript::OnCreatureRemove(creature);
            auto actor = objects.find(creature->GetEntry());
            if ((creature->GetEntry() == 98007 || creature->GetEntry() == 98602) && actor != objects.end() && actor->second == creature->GetGUID())
                objects.erase(actor);
        }

        void OnUnitDeath(Unit* creature) override
        {
            if (!myscenario)
                return;

            switch (creature->GetEntry())
            {
                case 98069: // dogs
                case 97994: // rune
                case 97992: // harpoon
                case 97649: // tracked leader, after the three actual waves
                    waves.Death(creature);
                    break;
                case 98892:
                    if (getScenarionStep() != 2)
                        return;
                    updateCriteriaForPlayers(47087);
                    break;

            }
        }

        void updateCriteriaForPlayers(uint32 objective)
        {
            Map::PlayerList const &PlList = instance->GetPlayers();

            if (PlList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                if (Player* player = i->getSource())
                {
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, objective);
                    break;
                }
        }

        void Update(uint32 diff) override
        {
            waves.Update(diff);
        }
    };
};

void AddSC_instance_warswords()
{
    new instance_warswords();
    new player_warswords_arrival_recovery();
}
