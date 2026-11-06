#ifndef SLAGHAMMER_HANDOFF_H
#define SLAGHAMMER_HANDOFF_H

#include "SmartAI.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "ObjectAccessor.h"
#include "TemporarySummon.h"
#include "CreatureTextMgr.h"
#include "GameObject.h"

namespace SlaghammerHandoff
{
inline Scenario* World(Creature* actor)
{
    if (!actor || !actor->IsInWorld() || actor->IsPreDelete() || !actor->IsAlive() ||
        actor->GetEntry() != 101430 || actor->GetMapId() != 1539)
        return nullptr;
    Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 991 ? scenario : nullptr;
}

inline bool Active(Player* player)
{
    return player && player->IsInWorld() && player->CanContact() && !player->isGameMaster() &&
        player->getClass() == CLASS_PRIEST && player->GetQuestStatus(40710) == QUEST_STATUS_INCOMPLETE &&
        !player->GetQuestRewardStatus(40710);
}

inline bool Near(Player* player, Creature* actor, float range = 50.0f)
{
    return Active(player) && player->IsAlive() && !player->HasPendingTeleport() &&
        player->GetMap() == actor->GetMap() && player->InSamePhase(actor) &&
        player->IsWithinDistInMap(actor, range);
}

inline bool HasEvent(Scenario* scenario, uint32 asset)
{
    if (!scenario) return false;
    for (auto const& row : *scenario->GetAchievementMgr().GetCriteriaProgressMap())
    {
        auto tree = sCriteriaTreeStore.LookupEntry(row.first);
        auto criterion = tree ? sCriteriaStore.LookupEntry(tree->CriteriaID) : nullptr;
        if (criterion && criterion->Type == CRITERIA_TYPE_SCRIPT_EVENT_2 && criterion->Asset == asset && row.second.Counter)
            return true;
    }
    return false;
}

// This receipt belongs to the live instance, not the replaceable SmartAI or
// Player. It never owns the static NPC or retains a raw player pointer.
struct State
{
    enum Phase { Unclaimed, Waiting, FirstText, SecondText, Summon, Finished };
    Phase phase = Unclaimed;
    ObjectGuid source, owner;
    std::shared_ptr<QuestCancellationReceipt const> attempt;
    bool pending = false;
    uint32 timer = 0;

    bool Cancelled() const { return !attempt || attempt->load(std::memory_order_acquire); }

    void Rejoin(Player* player)
    {
        if (!Cancelled() && Active(player) && player->GetGUID() == owner)
            player->AdoptQuestCancellationReceipt(40710, attempt);
    }

    void Claim(Creature* actor, Player* player, std::shared_ptr<QuestCancellationReceipt const> const& receipt)
    {
        if (phase != Unclaimed || !receipt || receipt->load(std::memory_order_acquire)) return;
        source = actor->GetGUID();
        owner = player->GetGUID();
        attempt = receipt;
        phase = Waiting;
    }

    Player* Owner(Creature* actor) const
    {
        Player* player = ObjectAccessor::GetPlayer(*actor, owner);
        return Near(player, actor) && !Cancelled() &&
            player->GetQuestCancellationReceipt(40710) == attempt ? player : nullptr;
    }

    bool Companion(Player* player) const
    {
        for (auto const& guid : *player->GetSummonList(101899))
            if (Creature* creature = ObjectAccessor::GetCreature(*player, guid))
                if (TempSummon* summon = creature->ToTempSummon())
                    if (creature->IsInWorld() && !creature->IsPreDelete() && creature->IsAlive() &&
                        summon->GetSummonerGUID() == owner && creature->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == 201004 &&
                        !creature->IsQuestObjectExpired())
                    {
                        auto scope = creature->GetQuestObjectScope().lock();
                        if (scope && scope->CancellationReceipt() == attempt) return true;
                    }
        return false;
    }

    void Update(Creature* actor, SmartAI* ai, uint32 diff)
    {
        Scenario* scenario = World(actor);
        if (!pending || Cancelled() || phase == Finished || actor->GetGUID() != source || !scenario)
            return;
        Player* player = Owner(actor);
        if (!player) return;
        if (phase == Waiting)
        {
            if (scenario->GetCurrentStep() != 2 || !ai->StartPath(true, 101430, false, player)) return;
            // Preserve native48160 before the new companion's48306. Consume
            // this checkpoint before criteria delivery can re-enter scripts.
            phase = FirstText;
            timer = 8000;
            sCreatureTextMgr->SendChat(actor, 0, owner);
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 48160);
            return;
        }
        // JUST_SUMMONED48306 can synchronously advance to step4 before this
        // actor's next update. Acknowledge that real endpoint before the
        // step3-only cast retry gate.
        if (phase == Summon && HasEvent(scenario, 48160) && HasEvent(scenario, 48306) && Companion(player))
        {
            phase = Finished;
            return;
        }
        if (scenario->GetCurrentStep() != 3 || !HasEvent(scenario, 48160)) return;
        if (timer > diff) { timer -= diff; return; }
        timer = 0;
        if (phase == FirstText)
        {
            phase = SecondText;
            timer = 10000;
            sCreatureTextMgr->SendChat(actor, 1, owner);
            return;
        }
        if (phase == SecondText)
        {
            // Shared ward and door are independent of the optional cast result.
            if (Creature* ward = actor->FindNearestCreature(101461, 50.0f, true))
                ward->AI()->SetData(1, 1);
            std::list<GameObject*> doors;
            GetGameObjectListWithEntryInGrid(doors, actor, 251349, 30.0f);
            for (GameObject* door : doors)
            {
                door->SetLootState(GO_READY);
                door->UseDoorOrButton(0, false, nullptr);
            }
            phase = Summon;
        }
        if (phase == Summon)
        {
            if (!Companion(player) && !player->FindCurrentSpellBySpellId(201004))
            {
                SpellCastResult result = player->CastSpell(player, 201004, false);
                // Native non-triggered effects can finish on a later unit tick.
                // Retain the actor and receipt until an owned summon exists.
                timer = 5000;
                if (result != SPELL_CAST_OK)
                    TC_LOG_WARN("scripts", "Slaghammer handoff: native201004 failed (%u); retain original owner for retry", uint32(result));
                return;
            }
            // A still-casting spell is an outstanding request, not a reason
            // to issue another one or to retire the source actor.
        }
    }
};

struct Instance
{
    virtual ~Instance() = default;
    State slaghammer;
};

class AI : public SmartAI
{
public:
    explicit AI(Creature* creature) : SmartAI(creature) { }
    void Reset() override
    {
        SmartAI::Reset();
        _retirementIssued = false;
    }
    State* StateForActor() const
    {
        auto instance = dynamic_cast<Instance*>(me->GetInstanceScript());
        return instance ? &instance->slaghammer : nullptr;
    }
    void MoveInLineOfSight(Unit* who) override
    {
        State* state = StateForActor();
        Scenario* scenario = World(me);
        Player* player = who ? who->ToPlayer() : nullptr;
        if (!state || !scenario) { SmartAI::MoveInLineOfSight(who); return; }
        if (scenario->GetCurrentStep() != 1 || state->phase != State::Unclaimed || !Near(player, me, 70.0f)) return;
        auto receipt = player->GetQuestCancellationReceipt(40710);
        bool before = HasEvent(scenario, 48121);
        SmartAI::MoveInLineOfSight(who); // Original root0/200165/unknownLINK2 unchanged.
        if (!before && HasEvent(scenario, 48121) && scenario->GetCurrentStep() == 2)
            state->Claim(me, player, receipt);
    }
    void SetData(uint32 id, uint32 value) override
    {
        if (id != 1 || value != 1) { SmartAI::SetData(id, value); return; }
        if (State* state = StateForActor())
            if (World(me) && state->source == me->GetGUID() && !state->Cancelled())
                state->pending = true;
        // No nearest-player or cached-invoker fallback for an unclaimed source.
    }
    void UpdateLifecycle(uint32 diff) override
    {
        if (State* state = StateForActor())
        {
            state->Update(me, this, diff);
            // Replacing SmartAI must not lose an already acknowledged source
            // retirement. Arm once per AI, without restarting its5s removal.
            if (!_retirementIssued && state->phase == State::Finished && state->source == me->GetGUID() &&
                me->IsInWorld() && !me->IsPreDelete())
            {
                _retirementIssued = true;
                SetDespawnTime(1);
                StartDespawn();
            }
        }
    }
private:
    bool _retirementIssued = false;
};
}
#endif
