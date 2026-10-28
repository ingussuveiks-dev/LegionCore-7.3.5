#pragma once

// The first attempt to draw Strom'kar is a trap, not artifact acquisition.
// Keep the native 204771 -> 206357 -> 206360 -> 206361 chain and acknowledge
// its real arrival/stun before admitting the combat stage.
namespace StromkarAwakening
{
inline Scenario* Stage(Creature* actor)
{
    if (!actor || !actor->IsInWorld() || !actor->IsAlive() || actor->GetMapId() != 1539 ||
        actor->GetEntry() != 103398 || actor->GetDBTableGUIDLow() != 370991)
        return nullptr;
    Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 1037 ? scenario : nullptr;
}

inline bool Eligible(Creature* actor, Player* player)
{
    return player && player->IsInWorld() && player->IsAlive() && player->getClass() == CLASS_WARRIOR &&
        player->GetMap() == actor->GetMap() && player->InSamePhase(actor) &&
        player->GetQuestStatus(41105) == QUEST_STATUS_INCOMPLETE && player->IsWithinDistInMap(actor, 100.0f);
}
}

struct npc_stromkar_awakening : SmartAI
{
    explicit npc_stromkar_awakening(Creature* actor) : SmartAI(actor) { }
    struct Participant { ObjectGuid guid; bool stunned = false, hit = false, recoilRetried = false; };
    std::vector<Participant> participants;
    ObjectGuid bossGuid;
    uint32 spawnDelay = 0, retry = 0, creditRetry = 0, attemptTime = 0, sourceVisualRetry = 0;
    bool burst = false, hidden = false, warned = false;
    bool sourcePoseRemoved = false, sourceWarned = false;
    bool sourceChannelPending = false, bossChannelPending = false;
    uint32 addedFlags = 0;
    ReactStates previousReact = REACT_AGGRESSIVE;

    void ClearSourceChannel()
    {
        if (Spell* channel = me->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
            if (channel->GetSpellInfo()->Id == 206390)
                me->InterruptSpell(CURRENT_CHANNELED_SPELL);
        me->RemoveAurasDueToSpell(206390, me->GetGUID());
    }

    void RestoreSourcePose()
    {
        if (sourcePoseRemoved && me->IsAlive() && me->GetEntry() == 103398 && me->GetMapId() == 1539 &&
            me->GetDBTableGUIDLow() == 370991 && (me->HasAura(130966, me->GetGUID()) || me->AddAura(130966, me)))
            sourcePoseRemoved = false;
    }

    Creature* Boss() const
    {
        Creature* boss = ObjectAccessor::GetCreature(*me, bossGuid);
        return boss && boss->IsInWorld() && boss->GetEntry() == 104276 && boss->GetMap() == me->GetMap() &&
            boss->ToTempSummon() && boss->ToTempSummon()->GetSummonerGUID() == me->GetGUID() ? boss : nullptr;
    }

    bool HasParticipant(Player* player) const
    {
        for (auto const& p : participants)
            if (player && p.guid == player->GetGUID())
                return true;
        return false;
    }

    void RestoreBoss(Creature* boss)
    {
        if (!boss)
            return;
        if (Spell* channel = boss->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
            if (channel->GetSpellInfo()->Id == 206390)
                boss->InterruptSpell(CURRENT_CHANNELED_SPELL);
        boss->RemoveAurasDueToSpell(206390, boss->GetGUID());
        boss->RemoveFlag(UNIT_FIELD_FLAGS, addedFlags);
        boss->SetReactState(previousReact);
        addedFlags = 0;
    }

    void Abort()
    {
        ClearSourceChannel();
        RestoreSourcePose();
        if (Creature* boss = Boss())
        {
            RestoreBoss(boss);
            boss->DespawnOrUnsummon();
        }
        // This native stun is a self cast by each participant on arrival.
        for (auto const& p : participants)
            if (Player* player = ObjectAccessor::GetPlayer(*me, p.guid))
                if (player->GetMap() == me->GetMap())
                    player->RemoveAurasDueToSpell(206361, player->GetGUID());
        bossGuid.Clear();
        participants.clear();
        burst = false;
        spawnDelay = retry = creditRetry = attemptTime = sourceVisualRetry = 0;
        warned = false;
        sourceWarned = false;
        sourceChannelPending = bossChannelPending = false;
        if (hidden)
            me->SetVisible(true);
        hidden = false;
        if (me->GetEntry() == 103398 && me->GetMapId() == 1539 && me->GetDBTableGUIDLow() == 370991)
            me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    }

    void Reset() override
    {
        Abort();
        SmartAI::Reset();
    }

    void SpellHit(Unit* caster, SpellInfo const* info) override
    {
        Scenario* scenario = StromkarAwakening::Stage(me);
        if (scenario && scenario->GetCurrentStep() == 7 && info)
        {
            if (info->Id == 204771 && !burst)
            {
                Player* player = caster ? caster->ToPlayer() : nullptr;
                if (StromkarAwakening::Eligible(me, player) && !HasParticipant(player))
                    participants.push_back({ player->GetGUID(), false, false, false });
            }
            // Effect2 targets the real caster; this is the actual periodic
            // blast, not merely an accepted click or the start of a channel.
            if (info->Id == 206357 && caster == me && !participants.empty() && !burst)
            {
                burst = true;
                spawnDelay = 5000; // Existing native Smart timed-list delay.
                me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
            }
        }
        SmartAI::SpellHit(caster, info);
    }

    void SpellHitTarget(Unit* target, SpellInfo const* info) override
    {
        if (info && info->Id == 206357)
            for (auto& p : participants)
                if (target && target->GetGUID() == p.guid)
                    p.hit = true;
        SmartAI::SpellHitTarget(target, info);
    }

    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        if (!burst)
        {
            RestoreSourcePose();
            return;
        }
        Scenario* scenario = StromkarAwakening::Stage(me);
        if (!scenario || (scenario->GetCurrentStep() != 7 && scenario->GetCurrentStep() != 8))
        {
            Abort();
            return;
        }
        Creature* boss = Boss();
        if (scenario->GetCurrentStep() == 8)
        {
            // Native49101 has actually advanced the shared scenario.
            RestoreBoss(boss);
            ClearSourceChannel();
            if (boss && boss->IsAlive() && boss->IsAIEnabled && boss->AI())
                for (auto const& p : participants)
                    if (Player* player = ObjectAccessor::GetPlayer(*me, p.guid))
                        if (p.hit && p.stunned && StromkarAwakening::Eligible(me, player))
                        {
                            boss->AI()->AttackStart(player);
                            break;
                        }
            participants.clear();
            bossGuid.Clear();
            burst = false;
            me->DespawnOrUnsummon();
            return;
        }
        attemptTime = std::min(uint32(60000), attemptTime + std::min(diff, uint32(60000)));
        Player* witness = nullptr;
        bool ready = true;
        creditRetry = creditRetry > diff ? creditRetry - diff : 0;
        for (auto& p : participants)
        {
            if (!p.hit)
                continue;
            Player* player = ObjectAccessor::GetPlayer(*me, p.guid);
            if (!StromkarAwakening::Eligible(me, player))
                continue;
            witness = player;
            if (player->HasAura(206361, player->GetGUID()))
                p.stunned = true;
            if (!p.stunned || player->HasAura(206361, player->GetGUID()) || !player->movespline->Finalized())
                ready = false;
        }
        if (!witness || attemptTime >= 60000)
        {
            if (witness)
                TC_LOG_WARN("scripts.ai", "Stromkar awakening: native arrival/stun acknowledgement timed out; resetting this attempt without progression");
            Abort();
            return;
        }
        if (!hidden && !me->HasAura(206390, me->GetGUID()))
        {
            if (sourceVisualRetry > diff)
                sourceVisualRetry -= diff;
            else
            {
                sourceVisualRetry = 2000;
                if (sourceChannelPending && !sourceWarned)
                {
                    TC_LOG_WARN("scripts.ai", "Stromkar awakening: initial rebirth cast accepted but aura not acknowledged after2s; retrying without progression");
                    sourceWarned = true;
                }
                if (me->HasAura(130966, me->GetGUID()))
                {
                    sourcePoseRemoved = true;
                    me->RemoveAurasDueToSpell(130966, me->GetGUID());
                }
                // Start the actual awakening on the original body immediately;
                // the accepted native combat replacement follows after5s.
                SpellCastResult result = me->CastSpell(me, 206390, false);
                sourceChannelPending = result == SPELL_CAST_OK && !me->HasAura(206390, me->GetGUID());
                // Ordinary casts are scheduled by the native spell event.
                // SPELL_CAST_OK is not synchronous aura application. Keep the
                // real aura acknowledgement gate and diagnose only a failure
                // or an accepted cast still missing its aura on the retry.
                if (result != SPELL_CAST_OK && !sourceWarned)
                {
                    TC_LOG_WARN("scripts.ai", "Stromkar awakening: initial rebirth cast failed (%u); retrying without progression", uint32(result));
                    sourceWarned = true;
                }
            }
            if (!me->HasAura(206390, me->GetGUID()))
                return;
        }
        sourceChannelPending = false;
        if (spawnDelay > diff)
        {
            spawnDelay -= diff;
            return;
        }
        spawnDelay = 0;
        if (retry > diff)
        {
            retry -= diff;
            return;
        }
        retry = 2000;
        if (!boss)
        {
            if (!bossGuid.IsEmpty())
            {
                Abort(); // A lost/dead accepted boss must not create duplicates.
                return;
            }
            boss = me->SummonCreature(104276, 1882.18f, 2163.35f, 35.9247f, 2.00205f, TEMPSUMMON_DEAD_DESPAWN, 0);
            if (!boss)
                return;
            bossGuid = boss->GetGUID();
            previousReact = boss->GetReactState();
            uint32 flags = UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC;
            addedFlags = flags & ~boss->GetUInt32Value(UNIT_FIELD_FLAGS);
            boss->SetFlag(UNIT_FIELD_FLAGS, flags);
            boss->SetReactState(REACT_PASSIVE);
            boss->CombatStop(true);
            ClearSourceChannel();
            me->SetVisible(false);
            hidden = true;
        }
        if (!boss->IsAlive() || !boss->IsAIEnabled || !boss->AI() || !dynamic_cast<npc_stromkar_zakajz*>(boss->AI()))
            return;
        for (auto& p : participants)
            if (p.hit && !p.stunned && !p.recoilRetried)
                if (Player* player = ObjectAccessor::GetPlayer(*me, p.guid))
                    if (StromkarAwakening::Eligible(me, player) && player->movespline->Finalized())
                    {
                        // One bounded retry of a failed native launch. Never
                        // manufacture the arrival stun or repeatedly push out.
                        p.recoilRetried = true;
                        player->CastSpell(me, 206360, true);
                    }
        if (!boss->HasAura(206390, boss->GetGUID()))
        {
            if (bossChannelPending && !warned)
            {
                TC_LOG_WARN("scripts.ai", "Stromkar awakening: combat rebirth cast accepted but aura not acknowledged after2s; retrying without starting combat");
                warned = true;
            }
            SpellCastResult result = boss->CastSpell(boss, 206390, false);
            bossChannelPending = result == SPELL_CAST_OK && !boss->HasAura(206390, boss->GetGUID());
            if (result != SPELL_CAST_OK || !boss->HasAura(206390, boss->GetGUID()))
            {
                if (result != SPELL_CAST_OK && !warned)
                {
                    TC_LOG_WARN("scripts.ai", "Stromkar awakening: combat rebirth cast failed (%u); retrying without starting combat", uint32(result));
                    warned = true;
                }
                return;
            }
        }
        bossChannelPending = false;
        if (!ready)
            return;
        // No pet or arbitrary nearby player receives progression. These are
        // real eligible click/channel participants in this exact map attempt.
        if (!creditRetry)
        {
            creditRetry = 2000;
            // Match native Smart action205, including its group-criteria
            // override; otherwise Player may return before scenario dispatch.
            witness->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49101, 0, 0, witness, true);
        }
    }
};

class spell_stromkar_opening_recoil : public SpellScript
{
    PrepareSpellScript(spell_stromkar_opening_recoil);

    bool Validate(SpellInfo const* info) override
    {
        return info->Id == 206360 && info->Effects[EFFECT_0] &&
            info->Effects[EFFECT_0]->Effect == SPELL_EFFECT_JUMP_DEST && info->Effects[EFFECT_0]->TriggerSpell == 206361;
    }

    npc_stromkar_awakening* Controller()
    {
        // InitExplicitTargets removes the redundant unit for destination106;
        // the force-cast's original source is retained by the native Spell.
        Creature* actor = GetOriginalTarget() ? GetOriginalTarget()->ToCreature() : nullptr;
        Scenario* scenario = StromkarAwakening::Stage(actor);
        return scenario && scenario->GetCurrentStep() == 7 && actor->IsAIEnabled &&
            StromkarAwakening::Eligible(actor, GetCaster()->ToPlayer()) ? dynamic_cast<npc_stromkar_awakening*>(actor->AI()) : nullptr;
    }

    SpellCastResult Check()
    {
        auto* owner = Controller();
        return owner && owner->HasParticipant(GetCaster()->ToPlayer()) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }

    void Launch(SpellEffIndex effect)
    {
        auto* owner = Controller();
        if (!owner || !owner->HasParticipant(GetCaster()->ToPlayer()) || !GetHitDest())
        {
            PreventHitDefaultEffect(effect);
            return;
        }
        Player* player = GetCaster()->ToPlayer();
        Unit* source = GetOriginalTarget();
        WorldLocation landing = *GetHitDest();
        SpellTargetPosition const* authored = sSpellMgr->GetSpellTargetPosition(206360);
        if (authored && authored->target_mapId != player->GetMapId())
        {
            PreventHitDefaultEffect(effect);
            return;
        }
        if (!authored)
        {
            // Target106 expects server DB coordinates, absent in this dataset.
            // A scoped30yd outward fallback is a local reconstruction, not a
            // claimed sniffed Blizzard landing. Use native collision/floor.
            float angle = source->GetExactDist2d(player) > 0.1f ? source->GetAngle(player) : source->GetOrientation();
            Position position = player->GetFirstCollisionPosition(30.0f, angle - player->GetOrientation());
            landing.Relocate(position);
        }
        float distance = player->GetExactDist2d(&landing);
        if (player->isInFlight() || !std::isfinite(landing.GetPositionX()) || !std::isfinite(landing.GetPositionY()) ||
            !std::isfinite(landing.GetPositionZ()) || !std::isfinite(distance) || distance < 0.5f || distance > 60.0f ||
            std::fabs(player->GetHeight(landing.GetPositionX(), landing.GetPositionY(), landing.GetPositionZ(), true) - landing.GetPositionZ()) > 1.0f ||
            !player->IsWithinLOS(landing.GetPositionX(), landing.GetPositionY(), landing.GetPositionZ() + 1.5f))
        {
            PreventHitDefaultEffect(effect);
            return;
        }
        GetHitDest()->Relocate(landing);
        SetExplTargetDest(landing);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_stromkar_opening_recoil::Check);
        OnEffectLaunch += SpellEffectFn(spell_stromkar_opening_recoil::Launch, EFFECT_0, SPELL_EFFECT_JUMP_DEST);
    }
};
