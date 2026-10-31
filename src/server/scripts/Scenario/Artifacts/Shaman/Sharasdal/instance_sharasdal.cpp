/*
    Shaman: Sharasdal
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Scenario.h"
#include "ScenarioMgr.h"

namespace SharasdalGuard
{
    bool Eligible(Player* player, Map* map)
    {
        return player && player->IsInWorld() && player->CanContact() && !player->isGameMaster() &&
            !player->IsBeingTeleported() && player->GetMap() == map && player->getClass() == CLASS_SHAMAN &&
            !player->GetQuestRewardStatus(40341) &&
            (player->GetQuestStatus(40341) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(40341) == QUEST_STATUS_COMPLETE);
    }

    Player* Recipient(Creature* defeated, Unit* killer)
    {
        if (!killer || killer->GetMap() != defeated->GetMap() || !killer->InSamePhase(defeated))
            return nullptr;
        if (Player* owner = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
            return Eligible(owner, defeated->GetMap()) && owner->InSamePhase(defeated) &&
                owner->IsWithinDistInMap(defeated, 100.0f) ? owner : nullptr;
        Creature* ally = killer->ToCreature();
        if (killer->GetCharmerOrOwnerGUID() || !ally ||
            !((ally->GetEntry() == 102826 && ally->GetDBTableGUIDLow() == 362063) ||
              (ally->GetEntry() == 105298 && ally->GetDBTableGUIDLow() == 362066)))
            return nullptr;
        Player* recipient = nullptr;
        for (auto const& reference : defeated->GetMap()->GetPlayers())
            if (Player* player = reference.getSource())
                if (Eligible(player, defeated->GetMap()) && player->InSamePhase(defeated) &&
                    player->IsWithinDistInMap(defeated, 100.0f))
                {
                    // One native update forwards to the shared scenario. Unrelated
                    // players must not choose an owner for a companion's victory.
                    if (recipient && !recipient->IsInSameGroupWith(player))
                        return nullptr;
                    recipient = player;
                }
        return recipient;
    }

    bool Receipt(Scenario* scenario)
    {
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(47197);
        return found != progress->end() && found->second.Counter;
    }
}


class instance_sharasdal : public InstanceMapScript
{
public:
    instance_sharasdal() : InstanceMapScript("instance_sharasdal", 1600) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_sharasdal_InstanceMapScript(map);
    }

    struct instance_sharasdal_InstanceMapScript : public InstanceScript
    {
        instance_sharasdal_InstanceMapScript(InstanceMap* map) : InstanceScript(map) {}
        
        WorldLocation loc_res_pla;  // for respawn
        ObjectGuid guardRecipient;
        std::weak_ptr<uint8> guardAttempt;
        bool guardPending = false, guardBusy = false, guardExhausted = false;
        uint32 guardRetry = 0, guardTries = 0;

        void CreatureDies(Creature* creature, Unit* killer) override
        {
            if (!creature || creature->GetMap() != instance || !creature->IsInWorld() || creature->IsPreDelete() ||
                creature->IsAlive() || creature->GetEntry() != 102831 || creature->GetDBTableGUIDLow() != 362076 || guardPending)
                return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1066 || scenario->GetCurrentStep() > 1 || SharasdalGuard::Receipt(scenario))
                return;
            Player* player = SharasdalGuard::Recipient(creature, killer);
            if (!player)
                return;
            player->AddQuestDelayedEvent(40341, 0, []() { });
            QuestStatusData* status = player->getQuestStatus(40341);
            if (!status || !status->ScriptLifetime)
                return;
            guardRecipient = player->GetGUID();
            guardAttempt = status->ScriptLifetime;
            guardPending = true;
            guardExhausted = false;
            guardRetry = guardTries = 0;
            UpdateGuard(0);
        }

        void OnPlayerEnter(Player* player) override
        {
            if (guardPending && player && player->GetGUID() == guardRecipient)
            {
                guardExhausted = false;
                guardRetry = guardTries = 0;
            }
        }

        void UpdateGuard(uint32 diff)
        {
            if (!guardPending || guardBusy || guardExhausted)
                return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1066)
                return;
            if (SharasdalGuard::Receipt(scenario))
            {
                guardPending = false;
                return;
            }
            Player* player = nullptr;
            for (auto const& reference : instance->GetPlayers())
                if (reference.getSource() && reference.getSource()->GetGUID() == guardRecipient)
                    player = reference.getSource();
            QuestStatusData* status = player ? player->getQuestStatus(40341) : nullptr;
            if (guardAttempt.expired() || (player && (!status || status->ScriptLifetime != guardAttempt.lock() ||
                player->GetQuestRewardStatus(40341) ||
                (player->GetQuestStatus(40341) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40341) != QUEST_STATUS_COMPLETE))))
            {
                guardPending = false;
                TC_LOG_WARN("scripts", "Sharasdal Tidecaller victory cancelled after its original quest attempt ended");
                return;
            }
            // An actual early defeat may wait for Big Help; never manufacture
            // either prerequisite or another death to satisfy its current tree.
            if (scenario->GetCurrentStep() == 0)
                return;
            if (scenario->GetCurrentStep() != 1)
            {
                guardPending = false;
                TC_LOG_WARN("scripts", "Sharasdal Tidecaller victory has no accepted guard criterion after stage transition");
                return;
            }
            if (guardRetry > diff) { guardRetry -= diff; return; }
            struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{guardBusy};
            guardBusy = true;
            if (SharasdalGuard::Eligible(player, instance))
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49957);
            if (SharasdalGuard::Receipt(scenario))
            {
                guardPending = false;
                return;
            }
            guardRetry = 1000;
            if (++guardTries >= 30)
            {
                guardExhausted = true;
                TC_LOG_WARN("scripts", "Sharasdal Tidecaller victory could not reach its original recipient/native criterion; retry on re-entry");
            }
        }

        WorldLocation* GetClosestGraveYard(float x, float y, float z) override
        // OnPlayerDeath
        {            
            // Init data
            loc_res_pla.WorldRelocate(1600, x, y, z);

            return &loc_res_pla;
        }

        void OnUnitDeath(Unit* creature) override
        {            
            switch (creature->GetEntry())
            {
                case 102792:
                    if (Player* targ = creature->FindNearestPlayer(100.0f))
                    {
                        targ->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49861);
                        targ->CreateConversation(1693);
                        targ->AddDelayedEvent(5000, [targ] () -> void
                        {
                            if (Creature* grash = targ->FindNearestCreature(105298, 70.0f, true))
                                grash->SetUInt32Value(UNIT_FIELD_NPC_FLAGS, 1); // госсип
                        });
                    }
                    break;
            }
        }
        
        void OnCreatureCreate(Creature* creature) override
        {
            if (creature->GetEntry() == 38821)
            {
                creature->SetFlag(UNIT_FIELD_FLAGS, 0);
                creature->SetVisible(true);
            }
        }
         
        void Update(uint32 diff) override { UpdateGuard(diff); }
    };
};

void AddSC_instance_sharasdal()
{
    new instance_sharasdal();
}
