#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Conversation.h"
#include <array>

// Keep the original SmartAI dialogue, animations, spells, console and later
// waypoint chain. Replace the UPDATE/nearby-living test with actual participants
// in this workshop: the tank starts over 40 yards from XR9-47.
struct npc_titanstrike_xr947 : SmartAI
{
    explicit npc_titanstrike_xr947(Creature* creature) : SmartAI(creature) { }
    std::array<ObjectGuid, 3> attackers{};
    std::array<bool, 3> defeated{};
    ObjectGuid imageGuid;
    uint32 timer = 0, elapsed = 0;
    bool booted = false, functional = false, console = false, dialogue = false;

    bool Active() const
    {
        auto progress = me->GetMapId() == 1495 ? sScenarioMgr->GetScenario(me->GetInstanceId()) : nullptr;
        return progress && progress->GetScenarioId() == 1068 && progress->GetCurrentStep() == 3;
    }

    void SetData(uint32 id, uint32 value) override
    {
        if (Active())
        {
            if (id == 1 && value == 1)
            {
                if (booted)
                    return;
                booted = true;
            }
            if (id == 3 && value == 3)
            {
                console = true;
                return;
            }
            // Only the tracked deaths, never a stale/foreign signal, can
            // authorize the native functional/path transition.
            if (id == 2 && value == 2)
                return;
        }
        SmartAI::SetData(id, value);
    }

    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        if (!Active())
            return;
        if (functional)
            elapsed += diff;
        if (timer > diff) { timer -= diff; return; }
        timer = 500;
        if (!booted)
            return;

        bool allDead = true, retryActivation = false;
        uint32 const spawns[3] = {382302, 382300, 382303};
        for (unsigned i = 0; i < attackers.size(); ++i)
        {
            if (defeated[i])
                continue;
            auto actor = ObjectAccessor::GetCreature(*me, attackers[i]);
            if (!actor)
            {
                std::list<Creature*> nearby;
                me->GetCreatureListWithEntryInGrid(nearby, i ? 106354 : 106352, 80.0f);
                for (auto candidate : nearby)
                    if (candidate->GetDBTableGUIDLow() == spawns[i] && me->InSamePhaseId(candidate))
                    { actor = candidate; attackers[i] = actor->GetGUID(); break; }
            }
            if (actor && !actor->IsAlive())
                defeated[i] = true;
            else
            {
                allDead = false; // a missing actor is not a kill
                if (actor && actor->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                    retryActivation = true;
            }
        }
        if (!functional)
        {
            if (retryActivation)
                me->CastSpell(me, 210845, true); // native SpellHit unlock; failed casts can retry
            if (!allDead)
                return;
            functional = true;
            elapsed = 0;
            me->RemoveAurasDueToSpell(210836);
            me->CombatStop(true);
            SmartAI::SetData(2, 2); // original path, functional voice and console activation
        }
        if (!console)
            return;
        auto image = ObjectAccessor::GetCreature(*me, imageGuid);
        if (!image)
        {
            image = me->SummonCreature(106376, 4970.32f, 297.495f, -36.7847f, 2.60656f, TEMPSUMMON_MANUAL_DESPAWN);
            if (!image)
                return; // never consume a failed hologram summon
            imageGuid = image->GetGUID();
        }
        // The original conversation's Mimiron lines started 14.15s after the
        // functional line. Start their separate object only after the actual
        // hologram exists, retaining native line/sound IDs and relative gaps.
        if (!dialogue && elapsed >= 14150)
        {
            auto conversation = new Conversation;
            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 9002055, image, nullptr, *image))
                delete conversation;
            else
                dialogue = true;
        }
    }
};

void AddSC_titanstrike_workshop()
{
    RegisterCreatureAI(npc_titanstrike_xr947);
}
