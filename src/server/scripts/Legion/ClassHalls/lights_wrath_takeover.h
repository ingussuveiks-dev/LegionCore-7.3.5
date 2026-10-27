#pragma once

namespace LightsWrathTakeover
{
    enum class Phase { Pending, Channel, Ready, Casting, Controlled, Cancelled, Done };

    bool Scoped(Creature* actor)
    {
        return actor && actor->IsInWorld() && actor->GetMap() && !actor->IsPreDelete() && actor->IsAlive() &&
            actor->GetEntry() == 104985 && actor->GetDBTableGUIDLow() == 370524 && actor->GetMapId() == 1583;
    }

    Scenario* Progress(Creature* actor)
    {
        Scenario* scenario = actor ? sScenarioMgr->GetScenario(actor->GetInstanceId()) : nullptr;
        return scenario && scenario->GetScenarioId() == 1065 ? scenario : nullptr;
    }

    bool Earned(Creature* actor, uint32 tree)
    {
        Scenario* scenario = Progress(actor);
        if (!scenario) return false;
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(tree);
        return found != progress->end() && found->second.Counter;
    }

    Player* FindPlayer(Map* map, ObjectGuid const& guid)
    {
        for (auto const& reference : map->GetPlayers())
            if (Player* player = reference.getSource())
                if (player->GetGUID() == guid) return player;
        return nullptr;
    }

    bool Eligible(Player* player, Creature* actor)
    {
        if (!Scoped(actor) || !player || !player->IsInWorld() || !player->CanContact() || player->isGameMaster() ||
            !player->IsAlive() || player->HasPendingTeleport() || player->getClass() != CLASS_PRIEST ||
            player->GetMap() != actor->GetMap() || !player->InSamePhase(actor) ||
            !player->IsWithinDistInMap(actor, 100.0f) || player->GetQuestRewardStatus(41631) ||
            (player->GetQuestStatus(41631) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(41631) != QUEST_STATUS_COMPLETE)) return false;
        Scenario* scenario = Progress(actor);
        return scenario && scenario->GetCurrentStep() == 5 && Earned(actor, 47117);
    }

    Creature* FindActor(Player* player)
    {
        if (!player || !player->IsInWorld() || !player->GetMap()) return nullptr;
        std::list<Creature*> actors;
        player->GetCreatureListWithEntryInGrid(actors, 104985, 100.0f);
        Creature* result = nullptr;
        for (Creature* actor : actors)
            if (Scoped(actor) && actor->GetMap() == player->GetMap() && actor->InSamePhase(player))
            {
                if (result) return nullptr;
                result = actor;
            }
        return result;
    }

    Creature* Resolve(Player* player, ObjectGuid const& guid)
    {
        return player && player->GetMap() ? player->GetMap()->GetCreature(guid) : nullptr;
    }

    struct State;
    bool OtherAttempt(Creature* actor, State const* old);
    // Unit::CastSpell prepares and checks synchronously; retain only a weak
    // receipt in the spell script for its later native event/effect delivery.
    thread_local std::weak_ptr<State> controlRequest;
    struct ControlScope
    {
        std::weak_ptr<State> previous;
        explicit ControlScope(std::shared_ptr<State> const& current) : previous(std::move(controlRequest)) { controlRequest = current; }
        ~ControlScope() { controlRequest = std::move(previous); }
    };

    struct State : std::enable_shared_from_this<State>
    {
        Phase phase = Phase::Pending;
        ObjectGuid actorGuid, ownerGuid;
        Map* originMap = nullptr; // identity only; never keep a borrowed actor/player
        uint32 instanceId = 0;
        std::weak_ptr<uint8> attempt;
        uint32 timer = 5000, furyRetry = 0;
        bool furyObserved = false, furyCleaned = false, furyRequestAccepted = false;
        bool furyWarning = false, controlWarning = false, spawnWarning = false;

        bool Original(Player* player, Creature* actor) const
        {
            QuestStatusData* quest = player ? player->getQuestStatus(41631) : nullptr;
            return !attempt.expired() && quest && quest->ScriptLifetime == attempt.lock() &&
                player->GetGUID() == ownerGuid && Scoped(actor) && actor->GetGUID() == actorGuid &&
                actor->GetMap() == originMap && actor->GetInstanceId() == instanceId;
        }

        bool Current(Player* player, Creature* actor) const
        {
            return Original(player, actor) && Eligible(player, actor) && actor->GetCharmerGUID().IsEmpty();
        }

        void Cleanup(Creature* actor)
        {
            if (!Scoped(actor) || actor->GetGUID() != actorGuid || actor->GetMap() != originMap ||
                actor->GetInstanceId() != instanceId || OtherAttempt(actor, this)) return;
            Map* map = actor->GetMap();
            auto fresh = [&]() -> Creature*
            {
                Creature* current = map->GetCreature(actorGuid);
                return Scoped(current) && !OtherAttempt(current, this) ? current : nullptr;
            };
            if (actor->GetCharmerGUID().IsEmpty())
                if (Scenario* scenario = Progress(actor))
                    if (scenario->GetCurrentStep() == 5) actor->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
            if (Creature* current = fresh())
                if (current->GetCharmerGUID().IsEmpty()) current->RemoveAurasDueToSpell(207956, actorGuid);
            if (!ownerGuid.IsEmpty() && fresh())
                if (Player* player = FindPlayer(map, ownerGuid))
                    if (player->GetMap() == map) player->RemoveAurasDueToSpell(207949, ownerGuid);
            if (!ownerGuid.IsEmpty())
                if (Creature* current = fresh()) current->RemoveAurasDueToSpell(207949, ownerGuid);
        }

        void Spawn(Creature* actor, uint32 diff)
        {
            if (!actor->GetCharmerGUID().IsEmpty()) return;
            Scenario* scenario = Progress(actor);
            if (!scenario || !Earned(actor, 47119) || scenario->GetCurrentStep() < 6 || scenario->GetCurrentStep() > 7)
            {
                phase = Phase::Cancelled;
                return;
            }
            // Once accepted, this is an earned instance world fact. Restore it
            // even after the original owner disconnects, without another credit.
            if (scenario->GetCurrentStep() > 6)
            {
                phase = Phase::Done;
                actor->DespawnOrUnsummon();
                return;
            }
            if (timer > diff) { timer -= diff; return; }
            std::list<GameObject*> objects;
            actor->GetGameObjectListWithEntryInGrid(objects, 248572, 5.0f);
            for (GameObject* object : objects)
                if (object && object->IsInWorld() && !object->IsPreDelete() && object->isSpawned() &&
                    object->GetMap() == actor->GetMap() && object->InSamePhase(actor) && object->GetOwnerGUID().IsEmpty() &&
                    object->GetSummonerGUID().IsCreatureOrVehicle() && object->GetSummonerGUID().GetEntry() == 104985 &&
                    object->IsWithinDist3d(1117.78f, 1048.03f, 310.663f, 2.0f, false))
                {
                    phase = Phase::Done;
                    actor->DespawnOrUnsummon();
                    return;
                }
            Map* map = actor->GetMap();
            GameObject* object = actor->SummonGameObject(248572, 1117.78f, 1048.03f, 310.663f, 2.09636f,
                0, 0, 0, 0, 0, ObjectGuid::Empty, nullptr, false);
            if (!object || !object->IsInWorld() || object->IsPreDelete())
            {
                if (!spawnWarning) TC_LOG_WARN("scripts", "Light's Wrath: accepted47119 retained after failed native248572 creation; retry without retiring source");
                spawnWarning = true;
                timer = 2000;
                return;
            }
            phase = Phase::Done;
            if (Creature* current = map->GetCreature(actorGuid))
                if (Scoped(current)) current->DespawnOrUnsummon();
        }

        void Update(Creature* actor, uint32 diff)
        {
            if (!Scoped(actor) || actor->GetGUID() != actorGuid || actor->GetMap() != originMap ||
                actor->GetInstanceId() != instanceId) { phase = Phase::Cancelled; return; }
            if (phase == Phase::Done || phase == Phase::Cancelled) return;
            if (phase == Phase::Controlled) { Spawn(actor, diff); return; }
            Player* player = FindPlayer(actor->GetMap(), ownerGuid);
            if (!Current(player, actor)) { phase = Phase::Cancelled; return; }
            if (phase == Phase::Pending)
            {
                if (timer > diff) timer -= diff;
                else phase = Phase::Cancelled;
                return;
            }
            if (phase == Phase::Channel)
            {
                if (!player->HasAura(207949, ownerGuid)) { phase = Phase::Cancelled; return; }
                if (furyRequestAccepted && actor->HasAura(207956, actorGuid)) furyObserved = true;
                if (furyObserved) return;
                if (furyRetry > diff) { furyRetry -= diff; return; }
                furyRetry = 5000;
                SpellCastResult result = actor->CastSpell(actor, 207956, false);
                furyRequestAccepted = result == SPELL_CAST_OK;
                if (result != SPELL_CAST_OK && !furyWarning)
                {
                    furyWarning = true;
                    TC_LOG_WARN("scripts", "Light's Wrath: native207956 response failed; retain original channel without granting control");
                }
                return;
            }
            if (timer > diff) { timer -= diff; return; }
            timer = 0;
            if (phase == Phase::Casting)
            {
                if (!controlWarning) TC_LOG_WARN("scripts", "Light's Wrath: original207996 did not produce accepted47119; retry original recipient");
                controlWarning = true;
                phase = Phase::Ready;
            }
            if (phase != Phase::Ready) return;
            if (!furyCleaned)
            {
                furyCleaned = true;
                actor->RemoveAurasDueToSpell(207956, actorGuid);
                return;
            }
            phase = Phase::Casting;
            timer = 5000;
            ControlScope request(shared_from_this());
            SpellCastResult result = player->CastSpell(player, 207996, false);
            if (result != SPELL_CAST_OK && phase == Phase::Casting)
            {
                phase = Phase::Ready;
                if (!controlWarning) TC_LOG_WARN("scripts", "Light's Wrath: native207996 cast rejected; retain original earned channel and5s retry");
                controlWarning = true;
            }
        }
    };
}

struct npc_lights_wrath_takeover : SmartAI
{
    explicit npc_lights_wrath_takeover(Creature* actor) : SmartAI(actor) { Rearm(); }
    ~npc_lights_wrath_takeover() override
    {
        if (state && state->phase != LightsWrathTakeover::Phase::Done) state->phase = LightsWrathTakeover::Phase::Cancelled;
    }
    std::shared_ptr<LightsWrathTakeover::State> state;
    bool resetting = false;

    void Rearm()
    {
        if (LightsWrathTakeover::Scoped(me) && me->GetCharmerGUID().IsEmpty())
            if (Scenario* scenario = LightsWrathTakeover::Progress(me))
                if (scenario->GetCurrentStep() == 5) me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    }

    void Reset() override
    {
        resetting = true;
        auto old = state;
        if (old) { old->phase = LightsWrathTakeover::Phase::Cancelled; old->Cleanup(me); }
        state.reset();
        SmartAI::Reset();
        Rearm();
        resetting = false;
    }

    void UpdateLifecycle(uint32 diff) override
    {
        if (resetting) { resetting = false; return; }
        if (state && state->phase == LightsWrathTakeover::Phase::Cancelled)
        {
            resetting = true; // reject reentrant requests until the next locked update
            auto old = std::move(state);
            old->Cleanup(me);
            return;
        }
        if (!state && LightsWrathTakeover::Scoped(me))
            if (Scenario* scenario = LightsWrathTakeover::Progress(me))
                if (scenario->GetCurrentStep() >= 6 && scenario->GetCurrentStep() <= 7 && LightsWrathTakeover::Earned(me, 47119))
                {
                    state = std::make_shared<LightsWrathTakeover::State>();
                    state->actorGuid = me->GetGUID();
                    state->originMap = me->GetMap(); state->instanceId = me->GetInstanceId();
                    state->phase = LightsWrathTakeover::Phase::Controlled;
                    state->timer = 0;
                }
        auto current = state;
        if (current) current->Update(me, diff);
    }
};

namespace LightsWrathTakeover
{
    std::shared_ptr<State> Get(Creature* actor)
    {
        auto* ai = actor ? dynamic_cast<npc_lights_wrath_takeover*>(actor->AI()) : nullptr;
        return ai && !ai->resetting ? ai->state : nullptr;
    }
    bool OtherAttempt(Creature* actor, State const* old)
    {
        auto current = Get(actor);
        return current && current.get() != old;
    }
}

class spell_lights_wrath_control_request : public SpellScript
{
    PrepareSpellScript(spell_lights_wrath_control_request);
    std::weak_ptr<LightsWrathTakeover::State> pending;
    bool bound = false;

    bool Validate(SpellInfo const* info) override
    {
        return info->Id == 207949 && info->Effects[EFFECT_0] && info->Effects[EFFECT_1] &&
            info->Effects[EFFECT_0]->ApplyAuraName == SPELL_AURA_MOD_ROOT &&
            info->Effects[EFFECT_1]->ApplyAuraName == SPELL_AURA_PERIODIC_TRIGGER_SPELL &&
            info->Effects[EFFECT_1]->TriggerSpell == 207960 && info->GetDuration() == 30000;
    }

    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        Creature* actor = GetExplTargetUnit() ? GetExplTargetUnit()->ToCreature() : nullptr;
        if (!LightsWrathTakeover::Eligible(player, actor) || !player->IsWithinDistInMap(actor, 5.0f) ||
            !player->canSeeOrDetect(actor) || !player->IsWithinLOSInMap(actor) || !actor->GetCharmerGUID().IsEmpty()) return SPELL_FAILED_BAD_TARGETS;
        auto* ai = dynamic_cast<npc_lights_wrath_takeover*>(actor->AI());
        if (!ai || ai->resetting) return SPELL_FAILED_BAD_TARGETS;
        if (bound)
        {
            auto previous = pending.lock();
            return previous && previous == ai->state &&
                (previous->phase == LightsWrathTakeover::Phase::Pending || previous->phase == LightsWrathTakeover::Phase::Channel) &&
                previous->Current(player, actor) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
        }
        if (ai->state) return SPELL_FAILED_SPELL_IN_PROGRESS;
        player->AddQuestDelayedEvent(41631, 0, []() { });
        QuestStatusData* quest = player->getQuestStatus(41631);
        if (!quest || !quest->ScriptLifetime) return SPELL_FAILED_BAD_TARGETS;
        auto next = std::make_shared<LightsWrathTakeover::State>();
        next->actorGuid = actor->GetGUID(); next->ownerGuid = player->GetGUID(); next->attempt = quest->ScriptLifetime;
        next->originMap = actor->GetMap(); next->instanceId = actor->GetInstanceId();
        ai->state = next;
        pending = next;
        bound = true;
        return SPELL_CAST_OK;
    }

    void Hit(SpellEffIndex effect)
    {
        auto current = pending.lock();
        Player* player = GetCaster()->ToPlayer();
        Creature* actor = current ? LightsWrathTakeover::Resolve(player, current->actorGuid) : nullptr;
        Unit* target = GetHitUnit();
        if (!current || !current->Current(player, actor) ||
            (current->phase != LightsWrathTakeover::Phase::Pending && current->phase != LightsWrathTakeover::Phase::Channel) ||
            !target || (target->GetGUID() != current->ownerGuid && target->GetGUID() != current->actorGuid))
            PreventHitDefaultEffect(effect);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_lights_wrath_control_request::Check);
        OnEffectHitTarget += SpellEffectFn(spell_lights_wrath_control_request::Hit, EFFECT_ALL, SPELL_EFFECT_APPLY_AURA);
    }
public:
    ~spell_lights_wrath_control_request() override
    {
        if (auto current = pending.lock())
            if (current->phase == LightsWrathTakeover::Phase::Pending) current->phase = LightsWrathTakeover::Phase::Cancelled;
    }
};

class spell_lights_wrath_control_channel : public AuraScript
{
    PrepareAuraScript(spell_lights_wrath_control_channel);
    std::weak_ptr<LightsWrathTakeover::State> original;
    bool Load() override { return GetUnitOwner() && GetUnitOwner()->ToPlayer(); }

    void Apply(AuraEffect const* effect, AuraEffectHandleModes)
    {
        Player* player = GetUnitOwner()->ToPlayer();
        Creature* actor = LightsWrathTakeover::FindActor(player);
        auto current = LightsWrathTakeover::Get(actor);
        if (!current || !current->Current(player, actor) || GetCasterGUID() != player->GetGUID() ||
            (current->phase != LightsWrathTakeover::Phase::Pending && current->phase != LightsWrathTakeover::Phase::Channel))
        {
            Remove();
            return;
        }
        original = current;
        if (effect->GetEffIndex() == EFFECT_1)
        {
            current->phase = LightsWrathTakeover::Phase::Channel;
            actor->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }
    }

    void Tick(uint32, AuraEffect*)
    {
        auto current = original.lock();
        Player* player = GetUnitOwner()->ToPlayer();
        Creature* actor = current ? LightsWrathTakeover::Resolve(player, current->actorGuid) : nullptr;
        if (!current || !current->Current(player, actor) ||
            (current->phase != LightsWrathTakeover::Phase::Pending && current->phase != LightsWrathTakeover::Phase::Channel))
        {
            if (current) current->phase = LightsWrathTakeover::Phase::Cancelled;
            Remove();
        }
    }

    void Removed(AuraEffect const*, AuraEffectHandleModes)
    {
        auto current = original.lock();
        if (!current || current->phase != LightsWrathTakeover::Phase::Channel) return;
        Player* player = GetUnitOwner()->ToPlayer();
        Creature* actor = LightsWrathTakeover::Resolve(player, current->actorGuid);
        bool nativeExpiry = GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE &&
            GetDuration() == 0 && GetMaxDuration() == 30000 && GetAura()->GetAllDuration() >= 30000;
        if (nativeExpiry && current->Current(player, actor) && current->furyObserved)
        {
            current->phase = LightsWrathTakeover::Phase::Ready;
            current->timer = 200; // native action85's original deferred invoker cast
            current->furyCleaned = true;
            actor->RemoveAurasDueToSpell(207956, current->actorGuid);
        }
        else
        {
            if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE && !nativeExpiry)
                TC_LOG_WARN("scripts", "Light's Wrath: unexpected207949 expiry duration; retain retry without granting control");
            current->phase = LightsWrathTakeover::Phase::Cancelled;
        }
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_lights_wrath_control_channel::Apply, EFFECT_0, SPELL_AURA_MOD_ROOT, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        AfterEffectApply += AuraEffectApplyFn(spell_lights_wrath_control_channel::Apply, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        OnEffectUpdate += AuraEffectUpdateFn(spell_lights_wrath_control_channel::Tick, EFFECT_0, SPELL_AURA_MOD_ROOT);
        AfterEffectRemove += AuraEffectRemoveFn(spell_lights_wrath_control_channel::Removed, EFFECT_0, SPELL_AURA_MOD_ROOT, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_lights_wrath_control_result : public SpellScript
{
    PrepareSpellScript(spell_lights_wrath_control_result);
    std::weak_ptr<LightsWrathTakeover::State> original;
    bool bound = false;

    bool Validate(SpellInfo const* info) override
    {
        return info->Id == 207996 && info->Effects[EFFECT_1] &&
            info->Effects[EFFECT_1]->Effect == SPELL_EFFECT_SEND_EVENT && info->Effects[EFFECT_1]->MiscValue == 49811;
    }

    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        auto current = bound ? original.lock() : LightsWrathTakeover::controlRequest.lock();
        Creature* actor = current ? LightsWrathTakeover::Resolve(player, current->actorGuid) : nullptr;
        if (!current || current->phase != LightsWrathTakeover::Phase::Casting || !current->Current(player, actor)) return SPELL_FAILED_BAD_TARGETS;
        original = current;
        bound = true;
        return SPELL_CAST_OK;
    }

    void Hit(SpellEffIndex effect)
    {
        auto current = original.lock();
        Player* player = GetCaster()->ToPlayer();
        Creature* actor = current ? LightsWrathTakeover::Resolve(player, current->actorGuid) : nullptr;
        if (!current || current->phase != LightsWrathTakeover::Phase::Casting || !current->Current(player, actor) || GetHitUnit() != player)
            PreventHitDefaultEffect(effect);
    }

    void After()
    {
        auto current = original.lock();
        Player* player = GetCaster()->ToPlayer();
        Creature* actor = current ? LightsWrathTakeover::Resolve(player, current->actorGuid) : nullptr;
        if (current && current->phase == LightsWrathTakeover::Phase::Casting && current->Original(player, actor) &&
            LightsWrathTakeover::Earned(actor, 47119))
        {
            current->phase = LightsWrathTakeover::Phase::Controlled;
            current->timer = 0;
        }
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_lights_wrath_control_result::Check);
        OnEffectHitTarget += SpellEffectFn(spell_lights_wrath_control_result::Hit, EFFECT_ALL, SPELL_EFFECT_ANY);
        AfterHit += SpellHitFn(spell_lights_wrath_control_result::After);
    }
};
