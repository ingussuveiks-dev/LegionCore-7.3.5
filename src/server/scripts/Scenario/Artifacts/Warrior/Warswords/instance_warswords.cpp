/*
    Warrior: Warswords
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Scenario.h"
#include "ScenarioMgr.h"

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
        bool myscenario;

        void Initialize() override
        {
            waves.Reset();
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

        void SetData(uint32 type,uint32 value) override
        {
            if (type==WarswordsArrivalRecovery::Ready && value==1)
            {
                myscenario=true;
                if (getScenarionStep()==1) waves.Start();
            }
        }


        void onScenarionNextStep(uint32 newStep) override
        {
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
                        if (!player || !player->CanContact())
                            continue;

                        player->AddDelayedEvent(3000, [player]() -> void
                        {
                            player->CastSpell(player, 189740); // scene
                        });
                        break;
                    }
                }
            }
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
