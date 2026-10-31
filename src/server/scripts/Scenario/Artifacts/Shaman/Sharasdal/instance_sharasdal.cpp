/*
    Shaman: Sharasdal
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "Conversation.h"
#include "ObjectMgr.h"
#include <array>

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

namespace SharasdalBrutes
{
    uint64 Count(Scenario* scenario)
    {
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(47161);
        return found == progress->end() ? 0 : found->second.Counter;
    }
}

namespace SharasdalKraliss
{
    bool Receipt(Scenario* scenario)
    {
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(47330);
        return found != progress->end() && found->second.Counter;
    }

    Player* Recipient(Creature* defeated, Unit* killer)
    {
        // Unit::Kill has already resolved the native reward/looter assignment.
        // Retain that real recipient when eligible, including an environmental
        // finishing blow; proximity alone must never manufacture a claimant.
        if (Player* tagged = defeated->GetLootRecipient())
            if (SharasdalGuard::Eligible(tagged, defeated->GetMap()) && tagged->InSamePhase(defeated) &&
                tagged->IsWithinDistInMap(defeated, 100.0f)) return tagged;
        if (Player* player = SharasdalGuard::Recipient(defeated, killer)) return player;
        if (!killer || killer->GetMap() != defeated->GetMap() || !killer->InSamePhase(defeated) ||
            killer->GetCharmerOrOwnerGUID()) return nullptr;
        Creature* ally = killer->ToCreature();
        if (!ally || ally->GetEntry() != 104853 || ally->GetDBTableGUIDLow() != 362078) return nullptr;
        Player* recipient = nullptr;
        for (auto const& reference : defeated->GetMap()->GetPlayers())
            if (Player* player = reference.getSource())
                if (SharasdalGuard::Eligible(player, defeated->GetMap()) && player->InSamePhase(defeated) &&
                    player->IsWithinDistInMap(defeated, 100.0f))
                {
                    if (recipient && !recipient->IsInSameGroupWith(player)) return nullptr;
                    recipient = player;
                }
        return recipient;
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

        struct BruteCredit
        {
            ObjectGuid recipient;
            std::weak_ptr<uint8> attempt;
            bool pending = false, exhausted = false;
            uint32 retry = 0, tries = 0;
        };
        std::array<BruteCredit, 2> bruteCredits;
        uint8 bruteSeen = 0;
        bool bruteBusy = false, brutesStarted = false, bruteVoicePending = false, bruteVoiceExhausted = false;
        bool bruteRoleDone = false, bruteRoleExhausted = false;
        ObjectGuid bruteVoiceRecipient, lowerGrash, lowerErunak;
        std::weak_ptr<uint8> bruteVoiceAttempt;
        uint32 bruteVoiceRetry = 0, bruteVoiceTries = 0, bruteRoleWait = 0, bruteRoleRetry = 0, bruteRoleTries = 0;

        ObjectGuid kralissRecipient;
        std::weak_ptr<uint8> kralissAttempt;
        bool kralissSeen = false, kralissPending = false, kralissBusy = false, kralissExhausted = false;
        uint32 kralissRetry = 0, kralissTries = 0;

        void RecordKralissDeath(Creature* creature, Unit* killer)
        {
            if (kralissSeen || !creature || creature->GetEntry() != 102839 || creature->GetDBTableGUIDLow() != 362080 ||
                creature->GetMap() != instance || !creature->IsInWorld() || creature->IsPreDelete() || creature->IsAlive()) return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1066 || scenario->GetCurrentStep() > 2 || SharasdalKraliss::Receipt(scenario)) return;
            kralissSeen = true;
            Player* player = SharasdalKraliss::Recipient(creature, killer);
            QuestStatusData* status = player ? player->getQuestStatus(40341) : nullptr;
            if (!status)
            {
                TC_LOG_WARN("scripts", "Sharasdal Kraliss death has no eligible original player/owner or known companion recipient");
                return;
            }
            player->AddQuestDelayedEvent(40341, 0, []() {});
            kralissRecipient = player->GetGUID();
            kralissAttempt = status->ScriptLifetime;
            kralissPending = true;
            kralissExhausted = false;
            kralissRetry = kralissTries = 0;
            UpdateKraliss(0);
        }

        void UpdateKraliss(uint32 diff)
        {
            if (!kralissPending || kralissBusy || kralissExhausted) return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1066) return;
            if (SharasdalKraliss::Receipt(scenario)) { kralissPending = false; return; }
            Player* player = nullptr;
            for (auto const& reference : instance->GetPlayers())
                if (reference.getSource() && reference.getSource()->GetGUID() == kralissRecipient) player = reference.getSource();
            QuestStatusData* status = player ? player->getQuestStatus(40341) : nullptr;
            if (kralissAttempt.expired() || (player && (!status || status->ScriptLifetime != kralissAttempt.lock() ||
                player->GetQuestRewardStatus(40341) ||
                (player->GetQuestStatus(40341) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40341) != QUEST_STATUS_COMPLETE))))
            {
                kralissPending = false;
                TC_LOG_WARN("scripts", "Sharasdal Kraliss victory cancelled after its original quest attempt ended");
                return;
            }
            if (scenario->GetCurrentStep() < 2) return;
            if (scenario->GetCurrentStep() != 2)
            {
                kralissPending = false;
                TC_LOG_WARN("scripts", "Sharasdal Kraliss victory has no accepted criterion after its stage ended");
                return;
            }
            if (kralissRetry > diff) { kralissRetry -= diff; return; }
            struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{kralissBusy};
            kralissBusy = true;
            if (SharasdalGuard::Eligible(player, instance)) player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50240);
            if (SharasdalKraliss::Receipt(scenario)) kralissPending = false;
            else
            {
                kralissRetry = 1000;
                if (++kralissTries >= 30)
                {
                    kralissExhausted = true;
                    TC_LOG_WARN("scripts", "Sharasdal Kraliss victory could not reach its original recipient/native criterion; retry on re-entry");
                }
            }
        }

        void BeginBrutes(ObjectGuid recipient = ObjectGuid::Empty, std::weak_ptr<uint8> attempt = std::weak_ptr<uint8>())
        {
            if (brutesStarted) return;
            brutesStarted = true;
            bruteVoiceRecipient = recipient;
            bruteVoiceAttempt = attempt;
            bruteVoicePending = !recipient.IsEmpty();
            bruteRoleWait = 5000;
        }

        void RecordBruteDeath(Creature* creature, Unit* killer)
        {
            if (creature->GetMap() != instance || !creature->IsInWorld() || creature->IsPreDelete() || creature->IsAlive()) return;
            uint32 index;
            if (creature->GetDBTableGUIDLow() == 362064) index = 0;
            else if (creature->GetDBTableGUIDLow() == 362067) index = 1;
            else return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1066 || scenario->GetCurrentStep() != 0 ||
                SharasdalBrutes::Count(scenario) >= 2 || (bruteSeen & (1u << index))) return;
            Player* player = SharasdalGuard::Recipient(creature, killer);
            if (!player) return;
            player->AddQuestDelayedEvent(40341, 0, []() { });
            QuestStatusData* status = player->getQuestStatus(40341);
            if (!status || !status->ScriptLifetime) return;
            BruteCredit& credit = bruteCredits[index];
            credit.recipient = player->GetGUID();
            credit.attempt = status->ScriptLifetime;
            credit.pending = true;
            bruteSeen |= uint8(1u << index);
            UpdateBrutes(0);
        }

        Player* BrutePlayer(ObjectGuid const& guid)
        {
            for (auto const& reference : instance->GetPlayers())
                if (reference.getSource() && reference.getSource()->GetGUID() == guid)
                    return reference.getSource();
            return nullptr;
        }

        void UpdateBrutes(uint32 diff)
        {
            if (bruteBusy) return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1066) return;
            struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{bruteBusy};
            bruteBusy = true;
            for (BruteCredit& credit : bruteCredits)
            {
                if (!credit.pending || credit.exhausted) continue;
                Player* player = BrutePlayer(credit.recipient);
                QuestStatusData* status = player ? player->getQuestStatus(40341) : nullptr;
                if (credit.attempt.expired() || (player && (!status || status->ScriptLifetime != credit.attempt.lock() ||
                    player->GetQuestRewardStatus(40341) ||
                    (player->GetQuestStatus(40341) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40341) != QUEST_STATUS_COMPLETE))))
                {
                    credit.pending = false;
                    TC_LOG_WARN("scripts", "Sharasdal Brute victory cancelled after its original quest attempt ended");
                    continue;
                }
                if (SharasdalBrutes::Count(scenario) >= 2)
                {
                    credit.pending = false;
                    continue;
                }
                if (scenario->GetCurrentStep() != 0)
                {
                    credit.pending = false;
                    TC_LOG_WARN("scripts", "Sharasdal Brute victory has no accepted two-Brute criterion after stage transition");
                    continue;
                }
                if (credit.retry > diff) { credit.retry -= diff; continue; }
                uint64 before = SharasdalBrutes::Count(scenario);
                if (SharasdalGuard::Eligible(player, instance))
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49861);
                if (SharasdalBrutes::Count(scenario) > before)
                {
                    credit.pending = false;
                    if (SharasdalBrutes::Count(scenario) >= 2) BeginBrutes(credit.recipient, credit.attempt);
                    continue;
                }
                credit.retry = 1000;
                if (++credit.tries >= 30)
                {
                    credit.exhausted = true;
                    TC_LOG_WARN("scripts", "Sharasdal Brute victory could not reach its original recipient/native criterion; retry on re-entry");
                }
            }
            // Presentation may recover already accepted history, without
            // claiming that legacy history proves either current actor died.
            if (SharasdalBrutes::Count(scenario) >= 2) BeginBrutes();
        }

        Creature* BruteActor(ObjectGuid const& guid, uint32 entry, uint64 spawn)
        {
            Creature* actor = instance->GetCreature(guid);
            return actor && actor->IsInWorld() && !actor->IsPreDelete() && actor->IsAlive() &&
                actor->GetMap() == instance && actor->GetEntry() == entry && actor->GetDBTableGUIDLow() == spawn ? actor : nullptr;
        }

        void UpdateBrutePresentation(uint32 diff)
        {
            if (!brutesStarted || bruteBusy) return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1066 || SharasdalBrutes::Count(scenario) < 2) return;
            Creature* grash = BruteActor(lowerGrash, 105298, 362066);
            if (scenario->GetCurrentStep() > 1 || (grash && ((grash->AI() && grash->AI()->GetData(19509)) ||
                (grash->GetDisabledAI() && grash->GetDisabledAI()->GetData(19509)))))
            {
                bruteVoicePending = false;
                bruteRoleDone = true;
                return;
            }
            if (bruteRoleDone)
            {
                bruteVoicePending = false; // do not start optional dialogue after recruitment was made available
                return;
            }
            struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{bruteBusy};
            bruteBusy = true;
            bool freshVoice = false;
            if (bruteVoicePending && !bruteVoiceExhausted)
            {
                Player* player = BrutePlayer(bruteVoiceRecipient);
                QuestStatusData* status = player ? player->getQuestStatus(40341) : nullptr;
                if (bruteVoiceAttempt.expired() || (player && (!status || status->ScriptLifetime != bruteVoiceAttempt.lock() ||
                    player->GetQuestRewardStatus(40341) ||
                    (player->GetQuestStatus(40341) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40341) != QUEST_STATUS_COMPLETE))))
                    bruteVoicePending = false;
                else if (bruteVoiceRetry > diff) bruteVoiceRetry -= diff;
                else if (!instance->GetPlayers().isEmpty())
                {
                    Creature* erunak = BruteActor(lowerErunak, 102826, 362063);
                    if (SharasdalGuard::Eligible(player, instance) && grash && erunak && player->InSamePhase(grash) &&
                        player->InSamePhase(erunak) && player->IsWithinDistInMap(grash, 100.0f) && player->IsWithinDistInMap(erunak, 100.0f))
                    {
                        std::unique_ptr<Conversation> conversation(new Conversation);
                        if (conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 1693, player, nullptr, *player))
                        {
                            bruteRoleWait = std::max(bruteRoleWait, uint32(std::max(0, conversation->GetDuration())));
                            conversation.release(); // native map ownership after successful creation
                            bruteVoicePending = false;
                            freshVoice = true;
                        }
                    }
                    if (bruteVoicePending)
                    {
                        bruteVoiceRetry = 1000;
                        if (++bruteVoiceTries >= 30)
                        {
                            bruteVoiceExhausted = true;
                            TC_LOG_WARN("scripts", "Sharasdal Brute victory dialogue1693 unavailable after thirty attempts; recruitment remains independent");
                        }
                    }
                }
            }
            // A newly created native conversation owns its full duration;
            // an old update's diff must not shorten its freshly armed wait.
            if (freshVoice || bruteRoleDone) return;
            if (bruteRoleWait > diff) { bruteRoleWait -= diff; return; }
            bruteRoleWait = 0;
            if (bruteRoleExhausted || instance->GetPlayers().isEmpty() || (grash && grash->HasUnitState(UNIT_STATE_STUNNED))) return;
            if (bruteRoleRetry > diff) { bruteRoleRetry -= diff; return; }
            if (grash && (grash->AI() || grash->GetDisabledAI()))
            {
                bruteRoleDone = true;
                grash->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                return;
            }
            bruteRoleRetry = 1000;
            if (++bruteRoleTries >= 30)
            {
                bruteRoleExhausted = true;
                TC_LOG_WARN("scripts", "Sharasdal initial Grash recruitment actor unavailable after thirty attempts; retry on re-entry/create");
            }
        }

        void CreatureDies(Creature* creature, Unit* killer) override
        {
            if (creature && creature->GetEntry() == 102839)
            {
                RecordKralissDeath(creature, killer);
                return;
            }
            if (creature && creature->GetEntry() == 102792)
            {
                RecordBruteDeath(creature, killer);
                return;
            }
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

        void RearmGrashWall(Player* player)
        {
            if (!player) return;
            if (Creature* grash = instance->GetCreature(lowerGrash))
            {
                auto current = grash->AI();
                auto suspended = grash->GetDisabledAI();
                if (current) current->SetGUID(player->GetGUID(), 40341);
                if (suspended && suspended != current) suspended->SetGUID(player->GetGUID(), 40341);
            }
        }

        void OnPlayerEnter(Player* player) override
        {
            if (kralissPending && player && player->GetGUID() == kralissRecipient)
            {
                kralissExhausted = false;
                kralissRetry = kralissTries = 0;
            }
            if (player)
            {
                for (BruteCredit& credit : bruteCredits)
                    if (credit.pending && credit.recipient == player->GetGUID())
                    {
                        credit.exhausted = false;
                        credit.retry = credit.tries = 0;
                    }
                if (bruteVoiceRecipient == player->GetGUID())
                {
                    bruteVoiceExhausted = false;
                    bruteVoiceRetry = bruteVoiceTries = 0;
                }
                bruteRoleExhausted = false;
                bruteRoleRetry = bruteRoleTries = 0;
            }
            if (guardPending && player && player->GetGUID() == guardRecipient)
            {
                guardExhausted = false;
                guardRetry = guardTries = 0;
            }
            RearmGrashWall(player);
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

        void OnUnitDeath(Unit* /*creature*/) override
        {
            // A state-only death has no killer. Native Kill owns real victories.
        }
        
        void OnCreatureCreate(Creature* creature) override
        {
            if (creature->GetEntry() == 105298 && creature->GetDBTableGUIDLow() == 362066) lowerGrash = creature->GetGUID();
            if (creature->GetEntry() == 102826 && creature->GetDBTableGUIDLow() == 362063) lowerErunak = creature->GetGUID();
            if (creature->GetGUID() == lowerGrash || creature->GetGUID() == lowerErunak)
            {
                bruteVoiceExhausted = bruteRoleExhausted = false;
                bruteVoiceRetry = bruteVoiceTries = bruteRoleRetry = bruteRoleTries = 0;
            }
            if (creature->GetEntry() == 38821)
            {
                creature->SetFlag(UNIT_FIELD_FLAGS, 0);
                creature->SetVisible(true);
            }
        }
         
        void Update(uint32 diff) override
        {
            UpdateKraliss(diff);
            UpdateBrutePresentation(diff);
            UpdateBrutes(diff);
            UpdateGuard(diff);
        }
    };
};

void AddSC_instance_sharasdal()
{
    new instance_sharasdal();
}
