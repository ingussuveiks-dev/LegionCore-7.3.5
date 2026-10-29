#pragma once

#include <array>

// Reuse the native transform, destination teleport, missile, summon and decay
// spells. Local collision-relative placement is a reconstruction; no original
// server destination coordinates were present in the matching data.
struct npc_stromkar_illusionist : SmartAI
{
    explicit npc_stromkar_illusionist(Creature* actor) : SmartAI(actor) { }
    struct Image
    {
        Position position;
        ObjectGuid guid;
        uint32 retry = 0, casts = 0, channels = 0;
        bool requested = false, delivered = false, channelAcknowledged = false;
    };
    std::array<Image, 2> images;
    Position destination;
    ObjectGuid witnessGuid;
    std::weak_ptr<uint8> lifetime;
    uint64 generation = 0;
    uint32 age = 0, teleportRetry = 0, teleports = 0, channelRetry = 0, channels = 0;
    bool active = false, teleported = false, channelAcknowledged = false, warned = false, deathHandled = false;

    static bool Identity(Creature* actor)
    {
        return actor && actor->GetEntry() == 103339 && actor->GetDBTableGUIDLow() == 370996 && actor->GetMapId() == 1539;
    }

    bool Stage() const
    {
        Scenario* scenario = me->IsInWorld() ? sScenarioMgr->GetScenario(me->GetInstanceId()) : nullptr;
        return Identity(me) && scenario && scenario->GetScenarioId() == 1037 && scenario->GetCurrentStep() == 6;
    }

    bool Eligible(Player* player, bool combat = true) const
    {
        return player && player->IsInWorld() && player->CanContact() && (!combat || player->IsAlive()) &&
            !player->HasPendingTeleport() && player->getClass() == CLASS_WARRIOR &&
            player->GetQuestStatus(41105) == QUEST_STATUS_INCOMPLETE && !player->GetQuestRewardStatus(41105) &&
            player->GetMap() == me->GetMap() && player->InSamePhase(me) && player->IsWithinDistInMap(me, 50.0f);
    }

    void Bind(Unit* unit)
    {
        Player* player = unit ? unit->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (!Eligible(player)) return;
        QuestStatusData* quest = player->getQuestStatus(41105);
        if (!quest) return;
        if (!quest->ScriptLifetime) quest->ScriptLifetime = std::make_shared<uint8>();
        witnessGuid = player->GetGUID(); lifetime = quest->ScriptLifetime;
    }

    Player* Witness(bool combat = true) const
    {
        Player* player = ObjectAccessor::GetPlayer(*me, witnessGuid);
        QuestStatusData* quest = player ? player->getQuestStatus(41105) : nullptr;
        return Eligible(player, combat) && quest && !lifetime.expired() && quest->ScriptLifetime == lifetime.lock() ? player : nullptr;
    }

    bool Current() const { return Stage() && me->IsAlive() && me->isInCombat() && Witness(); }

    void StopChannel(Creature* actor, uint32 id)
    {
        if (Spell* spell = actor->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
            if (spell->GetSpellInfo()->Id == id) actor->InterruptSpell(CURRENT_CHANNELED_SPELL);
        actor->RemoveAurasDueToSpell(id, actor->GetGUID());
    }

    void Clear()
    {
        ++generation; // pending native spell hooks from the old attempt reject
        active = false;
        for (auto& image : images)
        {
            Creature* actor = ObjectAccessor::GetCreature(*me, image.guid);
            TempSummon* summon = actor ? actor->ToTempSummon() : nullptr;
            if (summon && summon->GetSummonerGUID() == me->GetGUID() && actor->GetEntry() == 104737 &&
                actor->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == 207299)
            {
                StopChannel(actor, 207275);
                actor->DespawnOrUnsummon();
            }
            image = Image{};
        }
        if (Identity(me))
        {
            StopChannel(me, 207258);
            me->RemoveAurasDueToSpell(207244, me->GetGUID());
        }
        age = teleportRetry = teleports = channelRetry = channels = 0;
        teleported = channelAcknowledged = warned = false;
    }

    void Warn()
    {
        if (!warned)
        {
            TC_LOG_WARN("scripts.ai", "Stromkar split: native teleport/summon/channel not acknowledged; bounded retry without progression");
            warned = true;
        }
    }

    void Reset() override
    {
        Clear(); witnessGuid.Clear(); lifetime.reset(); deathHandled = false;
        SmartAI::Reset();
    }

    void EnterCombat(Unit* unit) override
    {
        if (Stage()) Bind(unit);
        // Keep native conversation204595, including a real pet/guardian owner.
        Player* player = Stage() ? Witness() : nullptr;
        SmartAI::EnterCombat(player ? player : unit);
    }

    void JustDied(Unit* killer) override
    {
        if (!Identity(me)) { SmartAI::JustDied(killer); return; }
        if (deathHandled) return;
        deathHandled = true;
        Player* owner = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        Player* witness = Witness(false);
        bool currentLease = witnessGuid.IsEmpty() || witness;
        Player* player = currentLease && Eligible(owner, false) &&
            (!witness || owner == witness || witness->IsInSameGroupWith(owner)) ? owner : nullptr;
        // An unowned ally can finish the fight actually engaged by this lease;
        // never choose an unrelated nearest player as a progression anchor.
        if (!player && killer && Stage())
            if (witness)
                if ((owner && witness->IsInSameGroupWith(owner)) || (!owner && killer->IsFriendlyTo(witness))) player = witness;
        bool scoped = Stage();
        Clear();
        SmartAI::JustDied(scoped ? static_cast<Unit*>(player) : nullptr);
    }

    bool Allow(uint32 spell, Position const* position, Unit* caster) const
    {
        if (!Current()) return false;
        if (spell == 207275)
        {
            for (auto const& image : images)
                if (active && image.delivered && caster && image.guid == caster->GetGUID()) return true;
            return false;
        }
        if (caster != me) return false;
        if (spell == 207244) return !active;
        if (spell == 207258) return active && teleported;
        if (!active || !position) return false;
        if (spell == 207249)
            return images[0].delivered && images[1].delivered && !teleported && destination.GetExactDist(position) < 0.25f;
        for (auto const& image : images)
            if (image.requested && !image.delivered && image.position.GetExactDist(position) < 0.25f)
                return spell == 207297 || spell == 207299;
        return false;
    }

    void SpellHit(Unit* caster, SpellInfo const* info) override
    {
        if (info && info->Id == 207244 && caster == me && Current() && !active && me->HasAura(207244, me->GetGUID()))
        {
            std::array<Position, 3> positions;
            bool supported = true;
            for (uint32 i = 0; i < positions.size(); ++i)
            {
                positions[i] = me->GetPosition();
                me->MovePositionToFirstCollision(positions[i], 6.0f, float(i) * 2.0943951f);
                supported = supported && positions[i].GetExactDist2d(me) >= 2.0f &&
                    std::fabs(positions[i].GetPositionZ() - me->GetPositionZ()) <= 4.0f;
                for (uint32 j = 0; j < i; ++j)
                    supported = supported && positions[i].GetExactDist2d(&positions[j]) >= 2.0f;
            }
            if (supported)
            {
                ++generation; active = true;
                images[0].position = positions[0]; images[1].position = positions[1]; destination = positions[2];
            }
            else Warn(); // no unsupported teleport, stacked copies or credit
        }
        SmartAI::SpellHit(caster, info);
    }

    void JustSummoned(Creature* actor) override
    {
        SmartAI::JustSummoned(actor);
        TempSummon* summon = actor ? actor->ToTempSummon() : nullptr;
        if (!Identity(me) || !summon || summon->GetSummonerGUID() != me->GetGUID() ||
            actor->GetEntry() != 104737 || actor->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) != 207299) return;
        if (active && Current() && actor->IsInWorld() && actor->IsAlive() && actor->GetMap() == me->GetMap() && actor->InSamePhase(me))
            for (auto& image : images)
                if (image.requested && !image.delivered && image.position.GetExactDist(actor) < 1.0f)
                {
                    image.delivered = true; image.guid = actor->GetGUID(); image.retry = 0;
                    actor->SetReactState(REACT_PASSIVE); actor->CombatStop(true);
                    return;
                }
        actor->DespawnOrUnsummon(); // late, duplicate or foreign-position own copy
    }

    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        if (!active) return;
        if (!Current()) { Clear(); return; }
        age += std::min(diff, uint32(20000));
        if (age >= 20000) { Clear(); return; } // native copies live12s; finite owner cleanup
        bool complete = true;
        for (auto& image : images)
        {
            if (image.retry > diff) { image.retry -= diff; complete = complete && image.delivered; continue; }
            image.retry = 1500;
            if (!image.delivered)
            {
                complete = false;
                if (image.casts >= 3) { Warn(); Clear(); return; }
                image.requested = true; ++image.casts;
                if (me->CastSpell(image.position, 207297, true) != SPELL_CAST_OK) Warn();
            }
            else if (Creature* actor = ObjectAccessor::GetCreature(*me, image.guid))
            {
                if (actor->HasAura(207275, actor->GetGUID())) image.channelAcknowledged = true;
                if (actor->IsInWorld() && actor->IsAlive() && !image.channelAcknowledged)
                {
                    if (image.channels >= 3) { Warn(); Clear(); return; }
                    ++image.channels;
                    if (actor->CastSpell(actor, 207275, true) != SPELL_CAST_OK) Warn();
                }
            }
        }
        if (!complete) return;
        // Preserve the actual one-second transformation before relocation.
        if (me->HasAura(207244, me->GetGUID())) return;
        if (!teleported)
        {
            if (teleports && destination.GetExactDist(me) < 0.25f) teleported = true;
            else
            {
                if (teleportRetry > diff) { teleportRetry -= diff; return; }
                teleportRetry = 1500;
                if (teleports >= 3) { Warn(); Clear(); return; }
                ++teleports;
                if (me->CastSpell(destination, 207249, true) != SPELL_CAST_OK) Warn();
                return;
            }
        }
        if (me->HasAura(207258, me->GetGUID())) channelAcknowledged = true;
        if (channelAcknowledged) return;
        if (channelRetry > diff) { channelRetry -= diff; return; }
        channelRetry = 1500;
        if (channels >= 3) { Warn(); Clear(); return; }
        ++channels;
        if (me->CastSpell(me, 207258, true) != SPELL_CAST_OK) Warn();
    }
};

// Bind each pending native cast to its actual attempt. Reset, death, quest
// reacceptance or changed stage cannot deliver a stale missile/teleport later.
class spell_stromkar_split_transport : public SpellScript
{
    PrepareSpellScript(spell_stromkar_split_transport);
    uint64 generation = 0;
    bool bound = false;

    npc_stromkar_illusionist* Controller()
    {
        Creature* actor = GetCaster() ? GetCaster()->ToCreature() : nullptr;
        if (actor && actor->GetEntry() == 104737 && actor->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == 207299)
            if (TempSummon* summon = actor->ToTempSummon())
                actor = summon->GetSummoner() ? summon->GetSummoner()->ToCreature() : nullptr;
        return npc_stromkar_illusionist::Identity(actor) && actor->IsInWorld() && actor->IsAIEnabled ? dynamic_cast<npc_stromkar_illusionist*>(actor->AI()) : nullptr;
    }

    bool Load() override { return Controller() != nullptr; }
    bool Validate(SpellInfo const* info) override
    {
        if (!ValidateSpellInfo({207244,207249,207258,207275,207297,207299})) return false;
        return info && info->Effects[EFFECT_0] &&
            ((info->Id == 207244 && info->Effects[EFFECT_0]->ApplyAuraName == SPELL_AURA_TRANSFORM) ||
             (info->Id == 207249 && info->Effects[EFFECT_0]->Effect == SPELL_EFFECT_TELEPORT_L) ||
             (info->Id == 207297 && info->Effects[EFFECT_0]->Effect == SPELL_EFFECT_TRIGGER_MISSILE && info->Effects[EFFECT_0]->TriggerSpell == 207299) ||
             (info->Id == 207299 && info->Effects[EFFECT_0]->Effect == SPELL_EFFECT_SUMMON && info->Effects[EFFECT_0]->MiscValue == 104737 && info->Effects[EFFECT_0]->MiscValueB == 64) ||
             ((info->Id == 207258 || info->Id == 207275) && info->Effects[EFFECT_1] &&
                 info->Effects[EFFECT_1]->ApplyAuraName == SPELL_AURA_PERIODIC_TRIGGER_SPELL && info->Effects[EFFECT_1]->TriggerSpell == 207274));
    }
    bool Current()
    {
        npc_stromkar_illusionist* ai = Controller();
        return ai && (!bound || ai->generation == generation) && ai->Allow(GetSpellInfo()->Id, GetExplTargetDest(), GetCaster());
    }
    SpellCastResult Check()
    {
        if (!Current()) return SPELL_FAILED_BAD_TARGETS;
        generation = Controller()->generation; bound = true;
        return SPELL_CAST_OK;
    }
    void Guard(SpellEffIndex effect) { if (!Current()) PreventHitDefaultEffect(effect); }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_stromkar_split_transport::Check);
        OnEffectHit += SpellEffectFn(spell_stromkar_split_transport::Guard, EFFECT_ALL, SPELL_EFFECT_ANY);
        OnEffectHitTarget += SpellEffectFn(spell_stromkar_split_transport::Guard, EFFECT_ALL, SPELL_EFFECT_ANY);
    }
};
