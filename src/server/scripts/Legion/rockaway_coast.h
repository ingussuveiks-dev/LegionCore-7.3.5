#ifndef LEGION_ROCKAWAY_COAST_H
#define LEGION_ROCKAWAY_COAST_H

namespace Rockaway
{
constexpr uint32 QuestId = 44746;
constexpr uint32 Credit = 115168;
constexpr uint32 Fragment = 142487;
constexpr uint32 CreditLoot = 229113;
// Reconstruction, not a recovered retail cast: native Kvaldir channel visual.
constexpr uint32 Channel = 231433;
constexpr float ChannelRange = 100.0f;
constexpr float Radius = 150.0f; // Native229113 effect radius41, both effects.

// Reconstruction: three published map pins, fourth shore identified in the
// 2016-11-13 JNasty video. Z comes from the deployed terrain/VMAP, not the pins.
// Exact retail spawn coordinates remain unverified (see the quest audit).
Position const Ritualists[] = {
    {6217.925f, 5016.806f, 0.888313f, 4.866f},
    {6255.625f, 4974.381f, -0.976295f, 3.866f},
    {6265.050f, 4917.815f, 0.697197f, 2.393f},
    {6220.000f, 4900.000f, -0.416786f, 1.392f}
};

inline bool Active()
{
    auto quest = sQuestDataStore->GetQuestTemplate(QuestId);
    auto rotation = sQuestDataStore->GetWorldQuest(quest);
    return rotation && rotation->ResetTime > GameTime::GetGameTime();
}

inline bool Eligible(Creature* source, Player* player)
{
    auto quest = sQuestDataStore->GetQuestTemplate(QuestId);
    return quest && player && player->IsAlive() && source->InSamePhase(player) &&
        source->IsWithinDistInMap(player, Radius, true, true) &&
        player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE &&
        (!player->GetGroup() || !player->GetGroup()->isRaidGroup() ||
            quest->IsAllowedInRaid(player->GetMap()->GetDifficultyID()));
}
}

// One persistent anchor owns one run; no instance/global or transient SAI phase.
struct npc_rockaway_vision : ScriptedAI
{
    explicit npc_rockaway_vision(Creature* creature) : ScriptedAI(creature), summons(me) { }
    SummonList summons;
    ObjectGuid slots[4];
    bool dead[4] = {};
    GuidSet participants;
    EventMap events;
    bool enabled = false, ready = false, started = false, outro = false, resetting = false;
    uint32 poll = 0, emptyTime = 0, finishTime = 0, cooldown = 0;
    ObjectGuid attackTarget;

    void AttackStart(Unit*) override { }
    void MoveInLineOfSight(Unit*) override { }
    uint32 GetData(uint32 id) const override { return id == 1 && enabled && started && !outro; }

    void ClearRun()
    {
        resetting = true;
        started = ready = outro = false;
        events.Reset();
        me->CastStop();
        me->RemoveAurasDueToSpell(231328, me->GetGUID());
        for (auto const& guid : slots)
        {
            if (guid.IsEmpty()) continue;
            if (auto actor = ObjectAccessor::GetCreature(*me, guid)) actor->CastStop();
            me->RemoveAurasDueToSpell(Rockaway::Channel, guid);
        }
        // Removal during reset must not create new essence pools.
        std::list<Player*> players;
        me->GetPlayerListInGrid(players, 200.0f);
        for (auto player : players)
            player->RemoveAurasDueToSpell(231329, me->GetGUID());
        for (auto const& guid : participants)
            if (auto player = ObjectAccessor::GetPlayer(*me, guid))
                player->RemoveAurasDueToSpell(231329, me->GetGUID());
        summons.DespawnAll();
        for (uint32 i = 0; i < 4; ++i) { slots[i].Clear(); dead[i] = false; }
        participants.clear();
        attackTarget.Clear();
        poll = emptyTime = finishTime = 0;
        me->CombatStop(true);
        me->SetFullHealth();
        resetting = false;
    }

    void Reset() override
    {
        enabled = false;
        ClearRun();
        me->setActive(true);
        me->SetReactState(REACT_PASSIVE);
        me->SetDisableGravity(true);
        me->SetVisible(false);
        poll = cooldown = 0;
    }

    void EnsureRitualists()
    {
        ready = true;
        for (uint32 i = 0; i < 4; ++i)
        {
            if (dead[i]) continue;
            if (auto actor = ObjectAccessor::GetCreature(*me, slots[i]))
            {
                if (!actor->IsAlive()) ready = false;
                continue;
            }
            // A failed summon is retried; three actors cannot complete a run.
            if (auto actor = me->SummonCreature(115132, Rockaway::Ritualists[i], TEMPSUMMON_MANUAL_DESPAWN))
                slots[i] = actor->GetGUID();
            else
                ready = false;
        }
    }

    bool Contributor(Player* player) const
    {
        if (participants.count(player->GetGUID())) return true;
        if (auto group = player->GetGroup())
            for (auto const& guid : participants)
                if (auto member = ObjectAccessor::GetPlayer(*me, guid))
                    if (member->GetGroup() == group) return true;
        return false;
    }

    void SetGUID(ObjectGuid const& guid, int32) override
    {
        if (!enabled || !ready || outro || cooldown || !Rockaway::Active()) return;
        auto player = ObjectAccessor::GetPlayer(*me, guid);
        if (!Rockaway::Eligible(me, player)) return;
        participants.insert(guid);
        if (!started)
        {
            started = true;
            Talk(1);
            events.ScheduleEvent(1, 5000);
            events.ScheduleEvent(2, 10000);
            events.ScheduleEvent(3, 15000);
        }
    }

    void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType) override
    {
        // The image is attackable in retail, but killing it must not bypass
        // the four summoners. Clamp after target-relative health scaling.
        if (attacker)
            if (auto player = attacker->GetCharmerOrOwnerPlayerOrPlayerItself()) SetGUID(player->GetGUID(), 0);
        if (!enabled || !ready || outro || cooldown) damage = 0;
        else if (damage >= me->GetHealth(attacker)) damage = me->GetHealth(attacker) > 1 ? me->GetHealth(attacker) - 1 : 0;
    }

    void JustSummoned(Creature* actor) override
    {
        if ((actor->GetEntry() == 116187 || actor->GetEntry() == 116195) &&
            (!enabled || !started || outro || cooldown || !Rockaway::Active()))
        {
            actor->DespawnOrUnsummon();
            return;
        }
        summons.Summon(actor);
        if (actor->GetEntry() == 116187 || actor->GetEntry() == 116195)
            if (auto player = ObjectAccessor::GetPlayer(*me, attackTarget))
            {
                actor->SetFacingToObject(player);
                if (actor->GetEntry() == 116195) actor->AI()->AttackStart(player);
            }
    }

    void SummonedCreatureDies(Creature* actor, Unit* killer) override
    {
        // Also clean up when the dying ritualist's AI is unavailable.
        if (actor->GetEntry() == 115132)
            for (auto const& guid : slots)
                if (guid == actor->GetGUID())
                {
                    actor->CastStop();
                    me->RemoveAurasDueToSpell(Rockaway::Channel, guid);
                    break;
                }
        if (!enabled || !ready || outro || !Rockaway::Active() || actor->GetEntry() != 115132) return;
        if (killer)
            if (auto player = killer->GetCharmerOrOwnerPlayerOrPlayerItself()) SetGUID(player->GetGUID(), 0);
        for (uint32 i = 0; i < 4; ++i)
            if (slots[i] == actor->GetGUID() && !dead[i])
            {
                dead[i] = true;
                if (dead[0] && dead[1] && dead[2] && dead[3])
                {
                    outro = true;
                    events.Reset();
                    me->CastStop();
                    Talk(4);
                    finishTime = 6000; // 2016 video: final yell ~82s, credit ~88s.
                }
                return;
            }
    }

    void SummonedCreatureDespawn(Creature* actor) override
    {
        summons.Despawn(actor);
        if (resetting) return;
        for (uint32 i = 0; i < 4; ++i)
            if (slots[i] == actor->GetGUID() && !dead[i]) { ClearRun(); cooldown = 10000; me->SetVisible(false); return; }
    }

    void Finish()
    {
        if (!enabled || !outro || !finishTime || cooldown || !Rockaway::Active()) return;
        // Consume the transition before callbacks into quest/reward code.
        finishTime = 0;
        cooldown = 30000;
        std::list<Player*> players;
        me->GetPlayerListInGrid(players, Rockaway::Radius);
        for (auto player : players)
            if (Rockaway::Eligible(me, player) && Contributor(player))
            {
                // These are the native effect90 and non-boss effect189 routes
                // of229113. Per-player dispatch avoids an unfiltered enemy-area
                // cast granting a second reward to observers or completed WQs.
                if (player->HasQuestForItem(Rockaway::Fragment))
                    player->AutoStoreLoot(Rockaway::CreditLoot, LootTemplates_Spell);
                player->KilledMonsterCredit(Rockaway::Credit);
            }
        // Normal corpse loot must survive the short outro. Old corpses cannot
        // participate in the next run because its four slot GUIDs are new.
        for (uint32 i = 0; i < 4; ++i)
            if (auto corpse = ObjectAccessor::GetCreature(*me, slots[i]))
                if (dead[i] && !corpse->IsAlive())
                {
                    summons.Despawn(corpse);
                    corpse->DespawnOrUnsummon(60000);
                }
        ClearRun();
        me->SetVisible(false);
    }

    Player* Target()
    {
        std::list<Player*> players;
        me->GetPlayerListInGrid(players, Rockaway::Radius);
        for (auto player : players)
            if (Rockaway::Eligible(me, player) && Contributor(player)) return player;
        return nullptr;
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMapId() != 1220 || !Rockaway::Active())
        {
            if (enabled) { enabled = false; ClearRun(); me->SetVisible(false); }
            return;
        }
        if (cooldown)
        {
            if (cooldown > diff) { cooldown -= diff; return; }
            cooldown = 0;
            enabled = false;
        }
        if (!enabled)
        {
            enabled = true;
            ClearRun();
            me->SetVisible(true);
            // The unsupported Mage Tower self aura231328 is not part of the
            // reconstructed channel. Only the four summoners cast231433;
            // the image must remain free to execute its offensive events.
        }
        if (outro)
        {
            if (finishTime <= diff) Finish(); else finishTime -= diff;
            return;
        }
        if (poll <= diff) { poll = 1000; EnsureRitualists(); } else poll -= diff;
        if (!started || !ready) return;
        auto player = Target();
        if (!player)
        {
            emptyTime += diff;
            if (emptyTime >= 10000) { ClearRun(); cooldown = 10000; me->SetVisible(false); }
            return;
        }
        emptyTime = 0;
        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING)) return;
        attackTarget = player->GetGUID();
        if (auto event = events.ExecuteEvent())
        {
            if (event == 1)
            {
                me->CastSpell(player, 231296, true);
                events.ScheduleEvent(1, 12000);
            }
            else if (event == 2)
            {
                me->SetFacingToObject(player);
                me->CastSpell(player, 231351, true);
                for (uint32 i = 0; i < 5; ++i)
                {
                    auto pos = player->GetNearPosition(3.0f, float(i) * 1.256637f);
                    // Use real missile -> damage/summon effects; the parent
                    // dummy does not dispatch these children in the core.
                    me->CastSpell(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), 231354, true);
                }
                events.ScheduleEvent(2, 20000);
            }
            else
            {
                me->CastSpell(player, 231329, true);
                events.ScheduleEvent(3, 16000);
            }
        }
    }
};

namespace Rockaway
{
inline Creature* ChannelVision(Unit* caster)
{
    if (!caster || !caster->IsCreature() || caster->GetEntry() != 115132 ||
        caster->GetMapId() != 1220 || !caster->IsAlive() || !Active()) return nullptr;
    auto summon = caster->ToCreature()->ToTempSummon();
    auto vision = summon ? ObjectAccessor::GetCreature(*caster, summon->GetSummonerGUID()) : nullptr;
    if (!vision || vision->GetEntry() != 115133 || !vision->IsAlive() ||
        !caster->InSamePhase(vision) || !caster->IsWithinDistInMap(vision, ChannelRange, true, true)) return nullptr;
    auto ai = static_cast<npc_rockaway_vision*>(vision->AI());
    if (!ai || !ai->enabled || !ai->ready || ai->outro || ai->cooldown || ai->resetting) return nullptr;
    for (uint32 i = 0; i < 4; ++i)
        if (ai->slots[i] == caster->GetGUID() && !ai->dead[i]) return vision;
    return nullptr;
}
}

struct npc_rockaway_ritualist : ScriptedAI
{
    explicit npc_rockaway_ritualist(Creature* creature) : ScriptedAI(creature) { }
    ObjectGuid owner;
    uint32 poll = 0;
    bool spoken = false;
    void AttackStart(Unit*) override { }
    void MoveInLineOfSight(Unit*) override { }
    void IsSummonedBy(Unit* summoner) override
    {
        if (summoner->GetEntry() != 115133) return;
        owner = summoner->GetGUID();
        me->SetReactState(REACT_PASSIVE);
        me->SetFacingToObject(summoner);
        // Slot assignment and readiness follow IsSummonedBy. Start only once
        // the controller has admitted all four actors, on the next AI update.
    }
    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType) override
    {
        auto vision = ObjectAccessor::GetCreature(*me, owner);
        auto ai = vision ? static_cast<npc_rockaway_vision*>(vision->AI()) : nullptr;
        if (!ai || !ai->ready || !Rockaway::Active()) { damage = 0; return; }
        if (attacker)
            if (auto player = attacker->GetCharmerOrOwnerPlayerOrPlayerItself())
            {
                ai->SetGUID(player->GetGUID(), 0);
                if (!spoken) { spoken = true; Talk(1); }
            }
    }
    void JustDied(Unit*) override
    {
        StopChannel();
        Talk(2);
    }
    void StopChannel()
    {
        me->CastStop();
        me->RemoveAurasDueToSpell(231333, me->GetGUID());
        if (auto vision = ObjectAccessor::GetCreature(*me, owner))
            vision->RemoveAurasDueToSpell(Rockaway::Channel, me->GetGUID());
    }
    void UpdateAI(uint32 diff) override
    {
        if (poll > diff) { poll -= diff; return; }
        poll = 1000;
        auto vision = Rockaway::ChannelVision(me);
        if (!vision) { StopChannel(); return; }
        bool connected = false;
        if (me->GetChannelSpellId() == Rockaway::Channel && vision->HasAura(Rockaway::Channel, me->GetGUID()))
            for (auto const& guid : me->GetChannelObjects())
                if (guid == vision->GetGUID()) connected = true;
        if (!connected)
        {
            StopChannel();
            me->SetFacingToObject(vision);
            me->CastSpell(vision, Rockaway::Channel, true);
        }
    }
};

struct npc_rockaway_tentacle : ScriptedAI
{
    explicit npc_rockaway_tentacle(Creature* creature) : ScriptedAI(creature) { }
    bool active = false;
    uint32 timer = 3000;
    void AttackStart(Unit*) override { }
    void MoveInLineOfSight(Unit*) override { }
    void IsSummonedBy(Unit* summoner) override
    {
        active = summoner->GetEntry() == 115133;
        if (active) { me->SetReactState(REACT_PASSIVE); me->CastSpell(me, 231386, true); me->DespawnOrUnsummon(6000); }
    }
    void DamageTakenAfterScaling(Unit*, uint32& damage, DamageEffectType) override { if (active) damage = 0; }
    void UpdateAI(uint32 diff) override
    {
        if (!active || !timer) return;
        if (timer > diff) { timer -= diff; return; }
        timer = 0;
        auto pos = me->GetNearPosition(8.0f, 0.0f);
        me->CastSpell(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), 231298, true);
    }
};

struct npc_rockaway_slime : ScriptedAI
{
    explicit npc_rockaway_slime(Creature* creature) : ScriptedAI(creature) { }
    void IsSummonedBy(Unit* summoner) override
    {
        if (summoner->GetEntry() == 115133) me->DespawnOrUnsummon(45000);
    }
    void UpdateAI(uint32) override { if (UpdateVictim()) DoMeleeAttackIfReady(); }
};

class spell_rockaway_channel : public SpellScript
{
    PrepareSpellScript(spell_rockaway_channel);
    bool Load() override
    {
        return GetCaster()->GetEntry() == 115132 && GetCaster()->GetMapId() == 1220;
    }
    SpellCastResult Check()
    {
        return Rockaway::ChannelVision(GetCaster()) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Select(WorldObject*& target)
    {
        // Native target38 searches nearby even when CastSpell supplied an
        // explicit unit. Replace its result with this exact admitted owner.
        target = Rockaway::ChannelVision(GetCaster());
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_rockaway_channel::Check);
        OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_rockaway_channel::Select, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
    }
};

class spell_rockaway_taint : public AuraScript
{
    PrepareAuraScript(spell_rockaway_taint);
    void OnRemove(AuraEffect const*, AuraEffectHandleModes)
    {
        auto caster = GetCaster();
        auto target = GetTarget();
        auto mode = GetTargetApplication()->GetRemoveMode();
        if (caster && target && caster->GetEntry() == 115133 && caster->IsCreature() &&
            caster->ToCreature()->AI()->GetData(1) &&
            (mode == AURA_REMOVE_BY_EXPIRE || mode == AURA_REMOVE_BY_ENEMY_SPELL))
            caster->CastSpell(target, 232681, true);
    }
    void Register() override
    {
        OnEffectRemove += AuraEffectRemoveFn(spell_rockaway_taint::OnRemove, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
    }
};
#endif
