#pragma once

namespace FireLyandra
{
bool Scoped(Creature* actor)
{
    if (!actor || !actor->IsInWorld() || actor->IsQuestObjectExpired() ||
        actor->GetEntry() != 99615 || actor->GetMapId() != 1480)
        return false;
    Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 957 && scenario->GetCurrentStep() == 3;
}

bool Wreathed(Creature* actor)
{
    return Scoped(actor) && actor->IsAlive() && actor->HasAura(194360);
}

bool NativeWingData()
{
    struct Row { uint32 id, event, flags, e1, e2, e3, e4, action, a1, a2, target, t1, t2; };
    Row const expected[] = {
        {0,38,1,1,1,0,0,80,98686,2,1,0,0},
        {1,0,0,0,5000,15000,20000,11,198608,0,2,0,0},
        {2,6,1,0,0,0,0,45,4,4,19,99615,80},
        {3,2,1,0,35,0,0,11,196571,3,1,0,0}
    };
    auto rows = sSmartScriptMgr->GetScript(98686, SMART_SCRIPT_TYPE_CREATURE);
    if (rows.size() != 4 || !sSmartScriptMgr->GetScript(-370121, SMART_SCRIPT_TYPE_CREATURE).empty())
        return false;
    for (Row const& required : expected)
    {
        uint32 found = 0;
        for (auto const& row : rows)
            if (row.event_id == required.id)
            {
                ++found;
                if (row.entryOrGuid != 98686 || row.GetScriptType() != SMART_SCRIPT_TYPE_CREATURE || row.link ||
                    row.GetEventType() != required.event || row.event.event_phase_mask || row.event.event_chance != 100 ||
                    row.event.event_flags != required.flags || row.event.raw.param1 != required.e1 ||
                    row.event.raw.param2 != required.e2 || row.event.raw.param3 != required.e3 ||
                    row.event.raw.param4 != required.e4 || row.event.raw.param5 || row.GetActionType() != required.action ||
                    row.action.raw.param1 != required.a1 || row.action.raw.param2 != required.a2 ||
                    row.action.raw.param3 || row.action.raw.param4 || row.action.raw.param5 || row.action.raw.param6 ||
                    row.GetTargetType() != required.target || row.target.raw.param1 != required.t1 ||
                    row.target.raw.param2 != required.t2 || row.target.raw.param3 || row.target.raw.param4 ||
                    row.target.x || row.target.y || row.target.z || row.target.o ||
                    !sConditionMgr->GetConditionsForSmartEvent(98686, row.event_id, SMART_SCRIPT_TYPE_CREATURE).empty())
                    return false;
            }
        if (found != 1)
            return false;
    }
    Row const timedExpected[] = {
        {0,0,1,8000,8000,0,0,28,195699,0,1,0,0},
        {1,0,1,5000,5000,0,0,19,33555200,0,1,0,0},
        {2,0,1,0,0,0,0,49,0,0,21,50,0},
        {3,0,1,0,0,0,0,1,0,0,21,50,0}
    };
    auto timed = sSmartScriptMgr->GetScript(98686, SMART_SCRIPT_TYPE_TIMED_ACTIONLIST);
    if (timed.size() != 4)
        return false;
    for (Row const& required : timedExpected)
    {
        uint32 found = 0;
        for (auto const& row : timed)
            if (row.event_id == required.id)
            {
                ++found;
                if (row.entryOrGuid != 98686 || row.GetScriptType() != SMART_SCRIPT_TYPE_TIMED_ACTIONLIST || row.link ||
                    row.GetEventType() != required.event || row.event.event_phase_mask || row.event.event_chance != 100 ||
                    row.event.event_flags != required.flags || row.event.raw.param1 != required.e1 ||
                    row.event.raw.param2 != required.e2 || row.event.raw.param3 || row.event.raw.param4 || row.event.raw.param5 ||
                    row.GetActionType() != required.action || row.action.raw.param1 != required.a1 ||
                    row.action.raw.param2 != required.a2 || row.action.raw.param3 || row.action.raw.param4 ||
                    row.action.raw.param5 || row.action.raw.param6 || row.GetTargetType() != required.target ||
                    row.target.raw.param1 != required.t1 || row.target.raw.param2 != required.t2 ||
                    row.target.raw.param3 || row.target.raw.param4 || row.target.x || row.target.y || row.target.z || row.target.o ||
                    !sConditionMgr->GetConditionsForSmartEvent(98686, row.event_id, SMART_SCRIPT_TYPE_TIMED_ACTIONLIST).empty())
                    return false;
            }
        if (found != 1)
            return false;
    }
    return true;
}

bool OriginalWing(Creature* boss, Creature* wing)
{
    if (!boss || !wing || !wing->IsInWorld() || wing->IsQuestObjectExpired() ||
        wing->GetEntry() != 98686 || wing->GetDBTableGUIDLow() != 370121 ||
        wing->GetMap() != boss->GetMap() || !boss->InSamePhase(wing) || !boss->CanTargetQuestObject(wing))
        return false;
    CreatureTemplate const* native = wing->GetCreatureTemplate();
    return native && !native->ScriptID && native->AIName == "SmartAI" &&
        (!wing->AI() || dynamic_cast<SmartAI*>(wing->AI())) &&
        native->unit_flags == 33555200 && NativeWingData();
}

bool WingRequest(SmartScriptHolder& request)
{
    auto rows = sSmartScriptMgr->GetScript(99615, SMART_SCRIPT_TYPE_CREATURE);
    uint32 found = 0;
    for (auto const& row : rows)
        if (row.event_id == 21)
        {
            ++found;
            request = row;
        }
    return found == 1 && request.entryOrGuid == 99615 && request.GetScriptType() == SMART_SCRIPT_TYPE_CREATURE &&
        request.GetEventType() == SMART_EVENT_LINK && !request.link && !request.event.event_phase_mask &&
        request.event.event_chance == 100 && request.event.event_flags == SMART_EVENT_FLAG_NOT_REPEATABLE &&
        !request.event.raw.param1 && !request.event.raw.param2 && !request.event.raw.param3 &&
        !request.event.raw.param4 && !request.event.raw.param5 && request.GetActionType() == SMART_ACTION_SET_DATA &&
        request.action.raw.param1 == 1 && request.action.raw.param2 == 1 && !request.action.raw.param3 &&
        !request.action.raw.param4 && !request.action.raw.param5 && !request.action.raw.param6 &&
        request.GetTargetType() == SMART_TARGET_CLOSEST_CREATURE && request.target.raw.param1 == 98686 &&
        request.target.raw.param2 == 50 && !request.target.raw.param3 && !request.target.raw.param4 &&
        !request.target.x && !request.target.y && !request.target.z && !request.target.o;
}

uint32 Elapsed(uint32 prior, uint32 diff)
{
    return std::min<uint32>(60000, prior + std::min<uint32>(diff, 60000));
}
}

struct npc_fire_artifact_lyandra : SmartAI
{
    explicit npc_fire_artifact_lyandra(Creature* actor) : SmartAI(actor) { }

    void AttackStart(Unit* target) override
    {
        // Unit::Attack removes unattackable auras even for a stunned creature.
        // Keep the native Wreath until Gorewing's actual DATA4 aura cleanup.
        if (!FireLyandra::Wreathed(me))
            SmartAI::AttackStart(target);
    }

    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        UpdateRearm(diff);
        if (!FireLyandra::Wreathed(me))
            return;
        phaseSeen = true;
        BindWing();

        // Wreath's native aura93 calls CombatStop, but retains the threat list.
        // Reuse native threat selection/evade; never choose a nearby stranger.
        Unit* victim = me->SelectVictim();
        if (!victim && FireLyandra::Wreathed(me))
        {
            // An alive taunt caster can make SelectVictim return the cleared
            // attack pointer after CombatStop. Its real native threat reference
            // remains authoritative; do not replace it with a nearest player.
            Unit* threat = me->getThreatManager().getHostilTarget();
            if (threat && me->canCreatureAttack(threat))
                victim = threat;
        }
        if (!victim || !FireLyandra::Wreathed(me))
            return;
        me->SetInCombatWith(victim);
        victim->SetInCombatWith(me);

        elapsed = FireLyandra::Elapsed(elapsed, diff);
        if (!GetScript()->HasCompletedLinkedCast(99615, 19))
        {
            if (elapsed >= 15000)
                FailPhase("Awaken was not acknowledged");
            return;
        }
        Creature* wing = Wing();
        if (!wing)
        {
            missing = FireLyandra::Elapsed(missing, diff);
            if (missing >= 30000)
                FailPhase("the original Gorewing is unavailable");
            return;
        }
        missing = 0;
        if (!wing->IsAlive())
        {
            // A corpse alone is not authority to synthesize native DATA4.
            missingDeath = FireLyandra::Elapsed(missingDeath, diff);
            if (missingDeath >= 1000)
                FailPhase("Gorewing died without the native phase handoff");
            return;
        }
        missingDeath = 0;
        if (elapsed < nextWake || (!wing->HasAura(195699) && !wing->HasFlag(UNIT_FIELD_FLAGS, 33555200)))
            return;
        if (wakeAttempts >= 2)
        {
            FailPhase("Gorewing remained frozen after bounded native awakening retries");
            return;
        }
        // The authored list needs13s. Only a still-frozen original actor may
        // restart that failed activation after15s, with a fresh native AI/list.
        if (wing->AI() && !dynamic_cast<SmartAI*>(wing->AI()))
        {
            FailPhase("the original Gorewing has an unexpected AI");
            return;
        }
        ++wakeAttempts;
        nextWake += 15000;
        if (wing->AIM_Initialize())
            wing->AI()->SetData(1, 1);
    }

    void EnterEvadeMode() override
    {
        bool scoped = FireLyandra::Scoped(me);
        if (scoped && (phaseSeen || FireLyandra::Wreathed(me)))
            TeardownPhase();
        SmartAI::EnterEvadeMode();
        if (scoped && me->IsInEvadeMode())
            CancelPending();
    }

    void JustReachedHome() override
    {
        SmartAI::JustReachedHome();
        if (FireLyandra::Scoped(me))
            Reset();
    }

    void Reset() override
    {
        if (FireLyandra::Scoped(me))
        {
            if (phaseSeen || FireLyandra::Wreathed(me))
                TeardownPhase();
            CancelPending();
        }
        else
        {
            ClearPhase();
            rearmGuid = ObjectGuid::Empty;
        }
        SmartAI::Reset();
    }

    void JustDied(Unit* killer) override
    {
        bool phase = phaseSeen && FireLyandra::Scoped(me);
        SmartAI::JustDied(killer);
        // Native death/loot/owner credit/Aethas return already ran. Only retire
        // this attempt's witnessed living wing; never emit a fabricated death.
        if (phase)
            if (Creature* wing = Wing())
                if (wing->IsAlive())
                    wing->DespawnOrUnsummon();
        rearmGuid = ObjectGuid::Empty;
        rearmDelay = rearmAttempts = 0;
        ClearPhase();
    }

private:
    ObjectGuid wingGuid;
    ObjectGuid rearmGuid;
    uint32 elapsed = 0, nextWake = 15000, missing = 0, missingDeath = 0, wakeAttempts = 0;
    uint32 rearmDelay = 0, rearmAttempts = 0;
    bool phaseSeen = false;

    Creature* Wing()
    {
        Creature* wing = wingGuid.IsEmpty() ? nullptr : ObjectAccessor::GetCreature(*me, wingGuid);
        return FireLyandra::OriginalWing(me, wing) ? wing : nullptr;
    }

    void BindWing()
    {
        if (!wingGuid.IsEmpty())
            return;
        SmartScriptHolder request;
        if (!FireLyandra::WingRequest(request))
            return;
        std::unique_ptr<ObjectList> targets(GetScript()->GetTargets(request));
        if (targets)
            for (WorldObject* target : *targets)
                if (Creature* wing = target->ToCreature())
                    if (FireLyandra::OriginalWing(me, wing))
                    {
                        wingGuid = wing->GetGUID();
                        return;
                    }
    }

    void ClearPhase()
    {
        phaseSeen = false;
        wingGuid = ObjectGuid::Empty;
        elapsed = missing = missingDeath = wakeAttempts = 0;
        nextWake = 15000;
    }

    void CancelPending()
    {
        GetScript()->CancelPendingActions();
    }

    void TeardownPhase()
    {
        BindWing();
        if (Creature* wing = Wing())
        {
            wing->CombatStop(true);
            wing->SetFlag(UNIT_FIELD_FLAGS, wing->GetCreatureTemplate()->unit_flags);
            rearmGuid = wing->GetGUID();
            rearmDelay = rearmAttempts = 0;
        }
        std::list<Creature*> actors;
        me->GetCreatureListWithEntryInGrid(actors, 99763, 100.0f);
        for (Creature* actor : actors)
            if (TempSummon* summon = actor->ToTempSummon())
                if (summon->GetSummonerGUID() == me->GetGUID())
                    actor->DespawnOrUnsummon();
        ClearPhase();
    }

    void UpdateRearm(uint32 diff)
    {
        if (rearmGuid.IsEmpty())
            return;
        if (!FireLyandra::Scoped(me) || !me->IsAlive())
        {
            rearmGuid = ObjectGuid::Empty;
            return;
        }
        if (diff < rearmDelay)
        {
            rearmDelay -= diff;
            return;
        }
        Creature* wing = ObjectAccessor::GetCreature(*me, rearmGuid);
        if (!FireLyandra::OriginalWing(me, wing))
        {
            TC_LOG_WARN("scripts.ai", "Fire Lyandra %u: original Gorewing disappeared before encounter reset.", me->GetGUIDLow());
            rearmGuid = ObjectGuid::Empty;
            return;
        }
        bool alive = wing->IsAlive();
        if ((!wing->AI() || dynamic_cast<SmartAI*>(wing->AI())) && wing->AIM_Initialize())
        {
            // AIM_Initialize skips InitializeAI for a corpse. This pinned wing
            // has no respawn/init actions; initialize its new native script now.
            if (!alive)
                wing->AI()->InitializeAI();
            wing->Respawn(true, 1);
            rearmGuid = ObjectGuid::Empty;
            return;
        }
        if (++rearmAttempts >= 5)
        {
            TC_LOG_WARN("scripts.ai", "Fire Lyandra %u: original Gorewing AI could not be restored after five attempts.", me->GetGUIDLow());
            rearmGuid = ObjectGuid::Empty;
            return;
        }
        rearmDelay = 500;
    }

    void FailPhase(char const* reason)
    {
        TC_LOG_WARN("scripts.ai", "Fire Lyandra %u: %s; resetting the encounter without progression.", me->GetGUIDLow(), reason);
        EnterEvadeMode();
    }
};
