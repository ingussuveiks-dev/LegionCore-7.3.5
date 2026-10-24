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
        instance_warswords_InstanceMapScript(InstanceMap* map) : InstanceScript(map) {}

        uint32 outrotimer;
        bool jump;
        std::map<uint32, ObjectGuid> objects;
        uint32 count;
        uint32 count_wave;
        bool myscenario;

        void Initialize() override
        {
            outrotimer = 5000;
            jump = true;
            count = 0;
            count_wave = 0;
            myscenario = false;
        }

        void OnPlayerEnter(Player* player) override
        {
            if (!player)
                return;

            uint32 instanceId = instance->GetInstanceId();
            player->AddDelayedEvent(3000, [player, this, instanceId]() -> void
            {
                if (!WarswordsArrival::IsParticipant(player, instanceId, this))
                    return;
                myscenario = true;
                if (!WarswordsArrival::CanLand(player, instanceId, this))
                    return;

                player->CastSpell(player, 205534);
                player->CastSpell(player, 189695);
                player->GetMotionMaster()->MoveJump(3386.46f, 1406.94f, 68.94f, player->GetOrientation(), 3.0f, 3.0f);
                outrotimer = 5000;
                jump = false;
            });
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
                    DoSummonAdds(count_wave++);
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

        void DoSummonAdds(uint32 wave)
        {
            Map::PlayerList const &PlList = instance->GetPlayers();

            if (PlList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                if (Player* player = i->getSource()) // needed player for summon and etc
                {
                    if (wave <= 2)
                    {
                        uint32 main_add = 0;
                        switch (wave)
                        {
                            case 0:
                            case 1:
                                main_add = 97992;
                                break;
                            case 2:
                                main_add = 97994;
                                break;
                        }
                        if (main_add)
                        {
                            if (Creature* creat = player->SummonCreature(main_add, 3427.55f + irand(-5, 5), 1486.10f + irand(-5, 5), 57.40f))
                                creat->AI()->AttackStart(player);
                        }
                        for (uint8 i = 0; i < 2; ++i)
                            if (Creature* creat = player->SummonCreature(98069, 3427.55f + irand(-7, 7), 1486.10f + irand(-7, 7), 57.40f))
                                creat->AI()->AttackStart(player);
                    }
                    else if (wave == 3)
                    {
                        player->SummonCreature(97649, 3363.17f, 1469.68f, 57.40f);
                    }
                }
        }

        void OnCreatureCreate(Creature* creature) override
        {
            if (creature->GetEntry() == 98007 || creature->GetEntry() == 98602)
                objects[creature->GetEntry()] = creature->GetGUID();
            if (!myscenario)
                return;

            if (getScenarionStep() != 1)
                return;

            if (Player* pl = creature->FindNearestPlayer(100.0f, true))
                creature->AI()->AttackStart(pl);
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
                case 97992: // harpun
                    if (getScenarionStep() != 1)
                        return;
                    count++;
                    if (count >= 3)
                    {
                        updateCriteriaForPlayers(47289);
                        count = 0;
                        DoSummonAdds(count_wave++);
                    }
                    break;
                case 97649: // pret
                    updateCriteriaForPlayers(46804);
                    if (Player* pl = creature->FindNearestPlayer(100.0f))
                        pl->CreateConversation(837);
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
            if (!myscenario)
                return;

            if (!jump)
                if (outrotimer <= diff)
                {
                    Map::PlayerList const &PlList = instance->GetPlayers();

                    if (PlList.isEmpty())
                        return;

                    for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                        if (Player* player = i->getSource())
                        {
                            if (!WarswordsArrival::CanLand(player, instance->GetInstanceId(), this))
                                continue;
                            player->CastSpell(3386.46f, 1406.94f, 68.94f, 205744);
                            player->CreateConversation(838);
                        }
                    jump = true;
                }
                else
                    outrotimer -= diff;
        }
    };
};

void AddSC_instance_warswords()
{
    new instance_warswords();
}
