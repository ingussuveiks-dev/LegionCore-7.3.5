/*
*/

#include "GameEventMgr.h"
#include "QuestObjectScope.h"

//122138
struct npc_moonkin_hatchling : ScriptedAI
{
    explicit npc_moonkin_hatchling(Creature* creature) : ScriptedAI(creature) {}

    void Reset() override
    {
        me->AddDelayedEvent(1000, [=, aiLifetimeGuard = std::weak_ptr<uint8>(_delayedAILifetimeGuard)]() -> void
        {
            if (aiLifetimeGuard.expired()) return;
            if (me->FindNearestCreature(122241, 40.0f, true))
                me->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, EMOTE_STATE_DANCE);
            else
            {
                DoCast(243958);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }
        });
    }
private:
    // Unit-owned queues may outlive this AI object.
    std::shared_ptr<uint8> _delayedAILifetimeGuard = std::make_shared<uint8>(0);
};

//122139
struct npc_moonkin_hatchling_lunar_strike : ScriptedAI
{
    explicit npc_moonkin_hatchling_lunar_strike(Creature* creature) : ScriptedAI(creature)
    {
        me->AddUnitState(UNIT_STATE_ROOT);
    }

    uint32 SpellVisualCast = 0;

    void Reset() override
    {
        SpellVisualCast = urand(100, 2500);
    }

    void UpdateAI(uint32 diff) override
    {
        if (SpellVisualCast <= diff)
        {
            SpellVisualCast = 4000;

            if (!me->isInCombat())
            {
                if (auto target = me->FindNearestCreature(122252, 30.0f, true))
                    AttackStart(target);
            }
            else
            {
                if (auto victim = me->getVictim())
                    DoCast(victim, 243383, false);
            }
        }
        else
            SpellVisualCast -= diff;
    }
};

namespace
{
constexpr uint32 MoonkinFestivalEvent = 299;
constexpr uint32 MoonkinMonitoringQuest = 47430;

struct MoonkinObservationAI : ScriptedAI
{
    MoonkinObservationAI(Creature* creature, float range, uint32 delay)
        : ScriptedAI(creature), observationRange(range), startDelay(delay) {}

    void Reset() override
    {
        observers.clear();
        observations.Reset();
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (observers.empty() && sGameEventMgr->IsActiveEvent(MoonkinFestivalEvent) && CanObserve(who->ToPlayer()))
            StartObservation();
    }

protected:
    bool CanObserve(Player* player) const
    {
        return player && me->IsAlive() && player->IsAlive() && me->IsWithinDistInMap(player, observationRange) &&
            me->IsWithinLOSInMap(player) && player->GetQuestStatus(MoonkinMonitoringQuest) == QUEST_STATUS_INCOMPLETE &&
            !player->GetQuestObjectiveData(MoonkinMonitoringQuest, me->GetEntry());
    }

    uint32 AdvanceObservation(uint32 diff)
    {
        if (observers.empty())
            return 0;
        if (!sGameEventMgr->IsActiveEvent(MoonkinFestivalEvent))
        {
            MoonkinObservationAI::Reset();
            return 0;
        }

        observers.erase(std::remove_if(observers.begin(), observers.end(), [this](Observer const& observer)
        {
            return observer.Receipt->load(std::memory_order_acquire) || !CanObserve(ObjectAccessor::GetPlayer(*me, observer.Guid));
        }), observers.end());
        if (observers.empty())
        {
            observations.Reset();
            StartObservation();
            return 0;
        }
        observations.Update(diff);
        return observations.ExecuteEvent();
    }

    void TalkToObservers(uint8 line)
    {
        for (auto const& observer : observers)
            Talk(line, observer.Guid);
    }

    void FinishObservation()
    {
        auto finished = std::move(observers);
        MoonkinObservationAI::Reset();
        for (auto const& observer : finished)
            if (Player* player = ObjectAccessor::GetPlayer(*me, observer.Guid))
                player->KilledMonsterCredit(me->GetEntry());
        StartObservation();
    }

    EventMap observations;

private:
    void StartObservation()
    {
        std::list<Player*> nearby;
        GetPlayerListInGrid(nearby, me, observationRange);
        for (Player* player : nearby)
            if (CanObserve(player))
                if (auto receipt = player->GetQuestCancellationReceipt(MoonkinMonitoringQuest))
                    observers.push_back({player->GetGUID(), std::move(receipt)});

        if (!observers.empty())
            observations.ScheduleEvent(EVENT_1, startDelay);
    }

    struct Observer
    {
        ObjectGuid Guid;
        std::shared_ptr<QuestCancellationReceipt const> Receipt;
    };

    std::vector<Observer> observers;
    float const observationRange;
    uint32 const startDelay;
};
}

//122134
struct npc_makkaw : ScriptedAI
{
    explicit npc_makkaw(Creature* creature) : ScriptedAI(creature) {}

    void sQuestAccept(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() == MoonkinMonitoringQuest)
            if (auto astralaire = me->FindNearestCreature(122241, 50.0f, true))
                astralaire->AI()->MoveInLineOfSight_Safe(player);
    }
};

//122241
struct npc_astralaire : MoonkinObservationAI
{
    explicit npc_astralaire(Creature* creature) : MoonkinObservationAI(creature, 20.0f, 3000) {}

    void Reset() override
    {
        MoonkinObservationAI::Reset();
        me->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, EMOTE_STATE_DANCE);
    }

    void UpdateAI(uint32 diff) override
    {
        switch (AdvanceObservation(diff))
        {
            case EVENT_1:
                Talk(0);
                observations.ScheduleEvent(EVENT_2, 4000);
                break;
            case EVENT_2:
                Talk(1);
                observations.ScheduleEvent(EVENT_3, 6000);
                break;
            case EVENT_3:
                Talk(2);
                observations.ScheduleEvent(EVENT_4, 7000);
                break;
            case EVENT_4:
                Talk(3);
                observations.ScheduleEvent(EVENT_5, 7000);
                break;
            case EVENT_5:
                Talk(4);
                observations.ScheduleEvent(EVENT_6, 1000);
                break;
            case EVENT_6:
            {
                std::list<Creature*> hatchlings;
                me->GetCreatureListWithEntryInGrid(hatchlings, 122138, 50.0f);
                for (Creature* creature : hatchlings)
                    if (creature->IsAlive() && creature->AI())
                        creature->AI()->Talk(0);
                observations.ScheduleEvent(EVENT_7, 4000);
                break;
            }
            case EVENT_7:
                FinishObservation();
                break;
        }
    }
};

//122390
struct npc_dramock : MoonkinObservationAI
{
    explicit npc_dramock(Creature* creature) : MoonkinObservationAI(creature, 15.0f, 500) {}

    void Reset() override
    {
        MoonkinObservationAI::Reset();
        DoCast(243958);
    }

    void UpdateAI(uint32 diff) override
    {
        switch (AdvanceObservation(diff))
        {
            case EVENT_1:
                TalkToObservers(0);
                observations.ScheduleEvent(EVENT_2, 4000);
                break;
            case EVENT_2:
                TalkToObservers(1);
                observations.ScheduleEvent(EVENT_3, 6000);
                break;
            case EVENT_3:
                TalkToObservers(2);
                observations.ScheduleEvent(EVENT_4, 7000);
                break;
            case EVENT_4:
                TalkToObservers(3);
                observations.ScheduleEvent(EVENT_5, 7000);
                break;
            case EVENT_5:
                Talk(4);
                observations.ScheduleEvent(EVENT_6, 5000);
                break;
            case EVENT_6:
                FinishObservation();
                break;
        }
    }
};

//122256
struct npc_pewkew : MoonkinObservationAI
{
    explicit npc_pewkew(Creature* creature) : MoonkinObservationAI(creature, 15.0f, 500)
    {
        me->AddUnitState(UNIT_STATE_ROOT);
    }

    void Reset() override
    {
        MoonkinObservationAI::Reset();
        lunarStrikeTimer = 1000;
    }

    void UpdateAI(uint32 diff) override
    {
        switch (AdvanceObservation(diff))
        {
            case EVENT_1:
                Talk(0);
                observations.ScheduleEvent(EVENT_2, 4000);
                break;
            case EVENT_2:
                Talk(1);
                observations.ScheduleEvent(EVENT_3, 6000);
                break;
            case EVENT_3:
                Talk(2);
                observations.ScheduleEvent(EVENT_4, 7000);
                break;
            case EVENT_4:
                Talk(3);
                observations.ScheduleEvent(EVENT_5, 7000);
                break;
            case EVENT_5:
                Talk(4);
                observations.ScheduleEvent(EVENT_6, 5000);
                break;
            case EVENT_6:
                FinishObservation();
                break;
        }

        if (lunarStrikeTimer > diff)
        {
            lunarStrikeTimer -= diff;
            return;
        }
        lunarStrikeTimer = 1700;

        if (!me->isInCombat())
        {
            if (auto target = me->FindNearestCreature(122252, 30.0f, true))
                AttackStart(target);
        }
        else if (auto victim = me->getVictim())
            DoCast(victim, 243829, false);
    }

private:
    uint32 lunarStrikeTimer = 1000;
};

//122139
struct npc_clookle : ScriptedAI
{
    explicit npc_clookle(Creature* creature) : ScriptedAI(creature)
    {
        DoCast(244526);
    }

    void sGossipSelect(Player* player, uint32 /*sender*/, uint32 action) override
    {
        if (action == 0)
        {
            me->RemoveAura(244526);
            me->AddDelayedEvent(3000, [=, aiLifetimeGuard = std::weak_ptr<uint8>(_delayedAILifetimeGuard)]() -> void { if (aiLifetimeGuard.expired()) return;  DoCast(244526); });

            player->DestroyItemCount(150750, 1, true);
            player->CastSpell(player, 244157, false);
            player->CLOSE_GOSSIP_MENU();
        }
    }
private:
    // Unit-owned queues may outlive this AI object.
    std::shared_ptr<uint8> _delayedAILifetimeGuard = std::make_shared<uint8>(0);
};

//122859,121946
struct npc_moonkin_hatchling_pet : ScriptedAI
{
    explicit npc_moonkin_hatchling_pet(Creature* creature) : ScriptedAI(creature) {}

    uint32 checking;
    uint32 SpellVisualCast;

    void Reset() override
    {
        checking = 1000;
        SpellVisualCast = 0;
    }

    void EnterEvadeMode() override
    {
        SpellVisualCast = 0;
        me->SetReactState(REACT_PASSIVE);
        ScriptedAI::EnterEvadeMode();
    }

    void IsSummonedBy(Unit* /*owner*/) override
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void SpellHit(Unit* /*caster*/, SpellInfo const* spell) override
    {
        if (spell->Id == 245342)
            me->SendPlaySpellVisualKit(84318, 0, 0);
    }

    void UpdateAI(uint32 diff) override
    {
        if (checking <= diff)
        {
            checking = 1000;

            if (auto owner = me->GetAnyOwner())
            {
                if (owner->isInCombat())
                {
                    me->SetReactState(REACT_AGGRESSIVE);

                    if (me->isInCombat())
                    {
                        SpellVisualCast = 1000;

                        if (auto target = owner->getVictim())
                            AttackStart(target);
                    }
                }
                else
                {
                    if (me->isInCombat())
                        EnterEvadeMode();
                }
                
                if (owner->IsMounted() || owner->HasAura(783) || owner->HasAura(125883))
                {
                    if (!me->HasUnitMovementFlag(MOVEMENTFLAG_CAN_FLY))
                    {
                        me->SetCanFly(true);
                        me->SetDisableGravity(true);
                        me->SetHover(true);
                        me->SetSpeed(MOVE_FLIGHT, owner->GetSpeed(MOVE_FLIGHT));
                    }
                }
                else
                {
                    if (me->HasUnitMovementFlag(MOVEMENTFLAG_CAN_FLY))
                    {
                        me->SetCanFly(false);
                        me->SetDisableGravity(false);
                        me->SetHover(false);
                    }
                }
            }
        }
        else
            checking -= diff;

        if (SpellVisualCast <= diff)
        {
            SpellVisualCast = 4000;

            if (me->isInCombat())
                if (auto victim = me->getVictim())
                    DoCast(victim, 243383, false);
        }
        else
            SpellVisualCast -= diff;
    }
};

//247428
class spell_moonkin_hatchling_tracker : public AuraScript
{
    PrepareAuraScript(spell_moonkin_hatchling_tracker);

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        if (!caster->HasAura(243333) && !caster->HasAura(244151) && !caster->HasAura(244155) && !caster->HasAura(244157) && !caster->HasAura(244158))
            caster->CastSpellDelay(caster, 243333, false, 5000);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_moonkin_hatchling_tracker::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

//247429
class spell_item_beanie_boomie : public SpellScript
{
    PrepareSpellScript(spell_item_beanie_boomie);

    void HandleOnHit(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        if (caster->HasAura(244157))
            caster->CastSpell(caster, 244158, false);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_item_beanie_boomie::HandleOnHit, EFFECT_0, SPELL_EFFECT_TRIGGER_SPELL);
    }
};

//244495
class spell_moonkissed_antidote : public SpellScript
{
    PrepareSpellScript(spell_moonkissed_antidote);

    void HandleOnHit(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        if (caster->HasAura(244151))
            caster->CastSpell(caster, 244155, false);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_moonkissed_antidote::HandleOnHit, EFFECT_0, SPELL_EFFECT_CREATE_ITEM);
    }
};


//244188
class spell_feed_moonkin_hatchling : public AuraScript
{
    PrepareAuraScript(spell_feed_moonkin_hatchling);

    uint16 timer;

    bool Load()
    {
        timer = 2000;
        return true;
    }

    void OnUpdate(uint32 diff, AuraEffect* /*aurEff*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        if (timer <= diff)
        {
            if (Aura* aura = caster->GetAura(244188))
            {
                if (aura->GetStackAmount() == 5)
                {
                    if (caster->HasAura(243333))
                    {
                        caster->CastSpell(caster, 244151, false);
                        caster->RemoveAura(244188);
                    }
                }
            }
        }
        else
            timer -= diff;
    }

    void Register() override
    {
        OnEffectUpdate += AuraEffectUpdateFn(spell_feed_moonkin_hatchling::OnUpdate, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

void AddSC_MoonkinFestival()
{
    RegisterCreatureAI(npc_moonkin_hatchling);
    RegisterCreatureAI(npc_moonkin_hatchling_lunar_strike);
    RegisterCreatureAI(npc_makkaw);
    RegisterCreatureAI(npc_astralaire);
    RegisterCreatureAI(npc_dramock);
    RegisterCreatureAI(npc_pewkew);
    RegisterCreatureAI(npc_clookle);
    RegisterCreatureAI(npc_moonkin_hatchling_pet);
    RegisterAuraScript(spell_moonkin_hatchling_tracker);
    RegisterAuraScript(spell_feed_moonkin_hatchling);
    RegisterSpellScript(spell_item_beanie_boomie);
    RegisterSpellScript(spell_moonkissed_antidote);
}
