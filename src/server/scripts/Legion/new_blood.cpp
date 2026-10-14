#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
#include "Log.h"
#include <array>
#include <cmath>

// The DB SmartAI lists also drive the later archive copies of these actors.
// Only the three persistent prisoners in scenario 962 get this lifecycle.
struct npc_new_blood_prisoner : SmartAI
{
    explicit npc_new_blood_prisoner(Creature* creature) : SmartAI(creature) { }

    enum class State { Bound, Falling, Freed };
    State state = State::Bound;
    bool initialized = false;
    bool landed = false;
    uint32 poll = 0;
    uint32 fallTimeout = 0;
    ObjectGuid rescuer;
    std::array<ObjectGuid, 4> anchors{};

    void JustRespawned() override
    {
        SmartAI::JustRespawned();
        ClearChains();
        initialized = false;
        state = State::Bound;
    }

    Scenario* Progress() const
    {
        if (me->GetMapId() != 1522 || !me->GetDBTableGUIDLow())
            return nullptr;
        Scenario* progress = sScenarioMgr->GetScenario(me->GetInstanceId());
        return progress && progress->GetScenarioId() == 962 ? progress : nullptr;
    }

    bool Completed(Scenario* progress) const
    {
        uint32 treeId = me->GetEntry() == 98775 ? 46021 : me->GetEntry() == 98793 ? 46022 : 46023;
        CriteriaTree const* tree = sAchievementMgr->GetCriteriaTree(treeId);
        return tree && progress->GetAchievementMgr().IsCompletedCriteriaTree(tree);
    }

    bool Ready() const
    {
        Scenario* progress = Progress();
        return progress && progress->GetCurrentStep() == 2 && !Completed(progress) &&
            !me->FindNearestCreature(98878, 12.0f, true) &&
            (me->GetEntry() != 98775 || !me->FindNearestCreature(98876, 12.0f, true));
    }

    bool Eligible(Player* player) const
    {
        return player && player->IsAlive() && player->GetQuestStatus(40729) == QUEST_STATUS_INCOMPLETE &&
            player->GetInstanceId() == me->GetInstanceId() && player->IsWithinDistInMap(me, 10.0f) && Ready();
    }

    void ClearChains()
    {
        for (ObjectGuid& guid : anchors)
        {
            if (Creature* anchor = ObjectAccessor::GetCreature(*me, guid))
                anchor->DespawnOrUnsummon();
            guid.Clear();
        }
        for (uint32 spell : {192195u, 192196u, 192198u, 192200u, 192214u})
            me->RemoveAurasDueToSpell(spell);
    }

    void Bind()
    {
        state = State::Bound;
        landed = false;
        rescuer.Clear();
        me->SetReactState(REACT_PASSIVE);
        me->SetControlled(true, UNIT_STATE_ROOT);
        me->SetDisableGravity(true);
        me->CastSpell(me, 192214, true); // Native suspended pose, AnimKit 8751.
        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    }

    void EnsureChains()
    {
        // Native Mardum chain bunnies and limb spells. Place anchors behind
        // the prisoner's facing, on the four corners of the existing pillar.
        // The DB prisoner positions are already about 3 yards above ground.
        static uint32 const entries[] = {97357, 97575, 97578, 97577};
        static uint32 const spells[] = {192195, 192196, 192198, 192200};
        float angle = me->GetOrientation();
        for (size_t i = 0; i < anchors.size(); ++i)
        {
            Creature* anchor = ObjectAccessor::GetCreature(*me, anchors[i]);
            if (!anchor)
            {
                float side = i % 2 ? -1.4f : 1.4f;
                float x = me->GetPositionX() - 0.6f * std::cos(angle) - side * std::sin(angle);
                float y = me->GetPositionY() - 0.6f * std::sin(angle) + side * std::cos(angle);
                float z = me->GetPositionZ() + (i < 2 ? 2.8f : -1.2f);
                anchor = me->SummonCreature(entries[i], x, y, z, angle, TEMPSUMMON_TIMED_DESPAWN, 5000);
                if (!anchor)
                    continue; // Retry a failed summon without duplicating the other anchors.
                anchors[i] = anchor->GetGUID();
                anchor->SetDisableGravity(true);
            }
            // These are ambient beam auras. The captive is deliberately immune
            // to NPC assistance; a normal cast from a neutral bunny fails the
            // explicit-friendly-target check. Keep the bunny as aura caster.
            if (!me->HasAura(spells[i], anchor->GetGUID()))
                anchor->AddAura(spells[i], me);
            // Orphan helpers expire even if their owner/grid disappears.
            if (TempSummon* summon = anchor->ToTempSummon())
                if (summon->GetTimer() < 4000)
                    summon->AddDuration(5000 - summon->GetTimer());
        }
    }

    void SpellHit(Unit* caster, SpellInfo const* spell) override
    {
        if (spell->Id != 195039 || !Progress())
        {
            SmartAI::SpellHit(caster, spell);
            return;
        }
        Player* player = caster->ToPlayer();
        if (state != State::Bound || !Eligible(player))
            return;
        state = State::Falling;
        rescuer = player->GetGUID();
        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        me->CastSpell(me, 192495, true); // Native Break Chains, not a replacement visual.
        ClearChains();
        me->SetControlled(false, UNIT_STATE_ROOT);
        me->SetDisableGravity(false);
        me->AddUnitMovementFlag(MOVEMENTFLAG_FALLING);
        landed = false;
        fallTimeout = 5000;
        me->GetMotionMaster()->MoveFall(195039);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == EFFECT_MOTION_TYPE && id == 195039 && state == State::Falling)
            landed = true; // Complete outside the movement generator's callback.
        SmartAI::MovementInform(type, id);
    }

    void UpdateAI(uint32 diff) override
    {
        Scenario* progress = Progress();
        if (!progress)
        {
            SmartAI::UpdateAI(diff);
            return;
        }
        if (!initialized)
        {
            initialized = true;
            Bind();
        }
        if (state == State::Freed)
        {
            SmartAI::UpdateAI(diff); // Existing delayed despawn/dialogue.
            return;
        }
        if (Completed(progress) || progress->GetCurrentStep() > 2)
        {
            ClearChains();
            me->DespawnOrUnsummon(); // Do not resurrect an already credited prisoner on grid reload.
            return;
        }
        if (state == State::Falling)
        {
            Player* player = ObjectAccessor::GetPlayer(*me, rescuer);
            float ground = me->GetHeight(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), true, 10.0f);
            if (landed || (ground > INVALID_HEIGHT && std::abs(me->GetPositionZ() - ground) < 0.15f))
            {
                if (Eligible(player))
                {
                    state = State::Freed;
                    me->RemoveUnitMovementFlag(MOVEMENTFLAG_FALLING);
                    // Dialogue, individual credit and native follower aura run once, after landing.
                    SmartAI::SpellHit(player, sSpellMgr->GetSpellInfo(195039));
                    return;
                }
                fallTimeout = 0;
            }
            if (fallTimeout > diff)
                fallTimeout -= diff;
            else
            {
                // Logout, abandonment or missing collision: keep the rescue retryable.
                Position const& home = me->GetHomePosition();
                me->GetMotionMaster()->Clear();
                me->RemoveUnitMovementFlag(MOVEMENTFLAG_FALLING);
                me->NearTeleportTo(home.GetPositionX(), home.GetPositionY(), home.GetPositionZ(), home.GetOrientation());
                Bind();
            }
            return;
        }
        if (poll > diff) { poll -= diff; return; }
        poll = 250;
        EnsureChains();
        // Source-18 conditions mask this field per observer. Merely killing
        // a guard does not dirty it, so update the actual flag as readiness changes.
        if (Ready())
            me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        else
            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    }
};

// Own the archive handoff in the scenario, rather than depending on a single
// JUST_SUMMONED condition check. The ritual's Jagganoth keeps its DB AI.
struct npc_new_blood_jagganoth : SmartAI
{
    explicit npc_new_blood_jagganoth(Creature* creature) : SmartAI(creature) { }
    bool started = false;
    uint32 timer = 1000;
    uint8 action = 0;
    ObjectGuid guide;

    void IsSummonedBy(Unit* summoner) override
    {
        if (me->GetMapId() == 1522 && summoner && summoner->GetEntry() == 99178)
            guide = summoner->GetGUID();
        SmartAI::IsSummonedBy(summoner);
    }

    void UpdateAI(uint32 diff) override
    {
        Scenario* progress = me->GetMapId() == 1522 ? sScenarioMgr->GetScenario(me->GetInstanceId()) : nullptr;
        if (!guide.IsEmpty() && progress && progress->GetScenarioId() == 962 && progress->GetCurrentStep() == 5 && action < 8)
        {
            if (!started)
            {
                started = true;
                me->setActive(true);
                TC_LOG_INFO("scripts.ai", "New Blood: archive confrontation started (instance %u)", me->GetInstanceId());
            }
            if (timer > diff)
                timer -= diff;
            else
            {
                static uint32 const delays[] = {2500, 6000, 9000, 1000, 1000, 3000, 1000, 0};
                static uint32 const dialogue[] = {199070, 199071, 199072, 199073};
                if (action < 4 || action == 6)
                {
                    uint32 spell = action < 4 ? dialogue[action] : 199074;
                    // Conversation spells have native self range. Each player
                    // is their own caster, with actors resolved by the conversation.
                    me->GetMap()->ApplyOnEveryPlayer([spell](Player* player)
                    {
                        if (player->GetQuestStatus(40729) == QUEST_STATUS_INCOMPLETE)
                        {
                            SpellCastResult result = player->CastSpell(player, spell, true);
                            TC_LOG_INFO("scripts.ai", "New Blood: archive conversation %u result %u (instance %u)", spell, uint32(result), player->GetInstanceId());
                        }
                    });
                }
                else if (action == 4)
                {
                    if (Creature* shinfel = me->FindNearestCreature(98783, 80.0f, true))
                        shinfel->AI()->SetData(2, 2);
                }
                else if (action == 5)
                {
                    SpellCastResult result = me->CastSpell(3095.98f, 963.538f, 257.38f, 199150, true);
                    TC_LOG_INFO("scripts.ai", "New Blood: archive smash cast result %u (instance %u)", uint32(result), me->GetInstanceId());
                    if (result != SPELL_CAST_OK)
                    {
                        timer = 1000;
                        return;
                    }
                }
                else if (action == 7)
                {
                    Creature* calydus = ObjectAccessor::GetCreature(*me, guide);
                    if (!calydus)
                    {
                        timer = 1000; // Never silently consume the handoff.
                        return;
                    }
                    calydus->setActive(true);
                    // SmartAI::StartPath rejects an actor still in combat.
                    // The guide flees the smash; he must not acquire Jagganoth.
                    calydus->CombatStop(true);
                    calydus->SetReactState(REACT_PASSIVE);
                    calydus->AI()->SetData(3, 3);
                    TC_LOG_INFO("scripts.ai", "New Blood: Calydus escape signalled (instance %u)", me->GetInstanceId());
                }
                timer = delays[action++];
            }
        }
        SmartAI::UpdateAI(diff);
    }
};

// The archive's persistent Calydus owns the transition from actual tome credit.
// Do not gate it on a transient SmartAI phase set by the Nelach dialogue chain.
struct npc_new_blood_archive_guide : SmartAI
{
    explicit npc_new_blood_archive_guide(Creature* creature) : SmartAI(creature) { }
    uint32 timer = 1000;
    uint8 action = 0;
    bool started = false;

    void UpdateAI(uint32 diff) override
    {
        Scenario* progress = me->GetMapId() == 1522 && me->GetDBTableGUIDLow() == 369458
            ? sScenarioMgr->GetScenario(me->GetInstanceId()) : nullptr;
        if (progress && progress->GetScenarioId() == 962 && progress->GetCurrentStep() == 5 && action < 4)
        {
            bool eligible = false;
            me->GetMap()->ApplyOnEveryPlayer([&eligible](Player* player)
            {
                if (player->IsAlive() && player->GetQuestStatus(40729) == QUEST_STATUS_INCOMPLETE &&
                    player->GetDistance(3093.55f, 970.747f, 257.738f) < 60.0f)
                    eligible = true;
            });
            if (eligible)
            {
                if (!started)
                {
                    started = true;
                    me->setActive(true);
                    TC_LOG_INFO("scripts.ai", "New Blood: tome credited; archive guide starting confrontation (instance %u)", me->GetInstanceId());
                }
                if (timer > diff)
                    timer -= diff;
                else if (action < 3)
                {
                    static uint32 const spells[] = {199066, 199068, 199069};
                    static uint32 const delays[] = {7000, 4000, 2000};
                    uint32 spell = spells[action];
                    me->GetMap()->ApplyOnEveryPlayer([spell](Player* player)
                    {
                        if (player->IsAlive() && player->GetQuestStatus(40729) == QUEST_STATUS_INCOMPLETE)
                        {
                            SpellCastResult result = player->CastSpell(player, spell, true);
                            TC_LOG_INFO("scripts.ai", "New Blood: tome conversation %u result %u (instance %u)", spell, uint32(result), player->GetInstanceId());
                        }
                    });
                    timer = delays[action++];
                }
                else
                {
                    // Keep the real guide as summoner for Jagganoth's exact-GUID handoff.
                    if (me->SummonCreature(98831, 3091.58f, 946.882f, 257.464f, 1.4768f, TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        ++action;
                        TC_LOG_INFO("scripts.ai", "New Blood: archive Jagganoth summoned (instance %u)", me->GetInstanceId());
                    }
                    else
                    {
                        timer = 1000;
                        TC_LOG_ERROR("scripts.ai", "New Blood: archive Jagganoth summon failed; retrying (instance %u)", me->GetInstanceId());
                    }
                }
            }
        }
        SmartAI::UpdateAI(diff);
    }
};

void AddSC_new_blood()
{
    RegisterCreatureAI(npc_new_blood_prisoner);
    RegisterCreatureAI(npc_new_blood_jagganoth);
    RegisterCreatureAI(npc_new_blood_archive_guide);
}
