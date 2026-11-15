#ifndef LEGION_ARCANE_INFUSED_EGG_H
#define LEGION_ARCANE_INFUSED_EGG_H

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "GameObject.h"
#include "Map.h"
#include "SpellInfo.h"
#include "CombatAI.h"

// Functional reconstruction of the observed Legion nest-defence reaction.
// The original non-DB2 spell179109 payload was not recovered. Retain native
// quest/item/loot admission and credit; this helper only provokes a nest guard.
namespace ArcaneEggLoot
{
    constexpr uint32 Egg = 239692;
    constexpr uint32 Heron = 89652;
    constexpr uint32 ReactionSpell = 179109;

    // The matching template carries vehicle3963. Preserve its ordinary
    // vehicle conditions/boarding lifecycle, adding native melee retaliation
    // only when explicitly attacked or provoked by a validated egg opening.
    struct npc_arcane_egg_heron : VehicleAI
    {
        explicit npc_arcane_egg_heron(Creature* creature) : VehicleAI(creature) { }
        void AttackStart(Unit* target) override { CreatureAI::AttackStart(target); }
        void UpdateAI(uint32 diff) override
        {
            VehicleAI::UpdateAI(diff);
            if (UpdateVictim())
                DoMeleeAttackIfReady();
        }
    };

    inline Creature* Guard(Player* player)
    {
        if (!player || !player->IsAlive() || !player->IsInWorld() ||
            player->GetMapId() != 1220 || player->HasPendingTeleport())
            return nullptr;

        auto egg = player->GetMap()->GetGameObject(player->GetLootGUID());
        if (!egg || egg->GetEntry() != Egg || egg->GetGoType() != GAMEOBJECT_TYPE_CHEST ||
            egg->GetGOInfo()->GetLootId() != Egg || !egg->IsInWorld() || !egg->isSpawned() ||
            !egg->InSamePhase(player) || !egg->IsWithinDistInMap(player, egg->GetInteractionDistance()))
            return nullptr;

        std::list<Creature*> birds;
        GetCreatureListWithEntryInGrid(birds, egg, Heron, player->GetVisibilityRange());
        Creature* closest = nullptr;
        float distance = 0.0f;
        for (auto bird : birds)
        {
            if (!bird->IsAlive() || bird->IsControlledByPlayer() || !bird->InSamePhase(player) ||
                !player->canSeeOrDetect(bird) || !bird->IsWithinDistInMap(egg, bird->GetAttackDistance(player)) ||
                !bird->IsWithinLOSInMap(player) || !bird->IsValidAttackTarget(player))
                continue;
            float candidate = bird->GetExactDistSq(egg);
            if (!closest || candidate < distance || (candidate == distance && bird->GetGUID() < closest->GetGUID()))
            {
                closest = bird;
                distance = candidate;
            }
        }
        // Do not taunt an engaged guard away from another player or redirect
        // a repeated loot-open event. Native combat/evade restores neutrality.
        if (!closest || closest->isInCombat() || !closest->IsAIEnabled || !closest->AI())
            return nullptr;
        return closest;
    }

    class spell_arcane_infused_egg : public SpellScript
    {
        PrepareSpellScript(spell_arcane_infused_egg);
        bool Validate(SpellInfo const* info) override
        {
            return info && info->Id == ReactionSpell && info->HasEffect(SPELL_EFFECT_DUMMY);
        }
        void Defend(SpellEffIndex)
        {
            auto player = GetCaster()->ToPlayer();
            if (auto guard = Guard(player))
                guard->AI()->AttackStart(player);
        }
        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_arcane_infused_egg::Defend, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    inline void Register()
    {
        RegisterSpellScript(spell_arcane_infused_egg);
        RegisterCreatureAI(npc_arcane_egg_heron);
    }
}
#endif
