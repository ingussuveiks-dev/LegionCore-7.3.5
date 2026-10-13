#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
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

void AddSC_new_blood()
{
    RegisterCreatureAI(npc_new_blood_prisoner);
}
