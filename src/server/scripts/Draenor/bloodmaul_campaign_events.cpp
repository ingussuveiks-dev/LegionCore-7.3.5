// Functional recovery of the Bloodmaul campaign's missing quest encounters.
// Coordinates come from quest POIs; ground heights are resolved on the live map.
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "TemporarySummon.h"
#include "GameObject.h"
#include "Map.h"
#include "garrison_campaign_actors.h"
#include <map>
#include <mutex>

namespace BloodmaulEvents
{
enum : uint32
{
    Gate = 34381,
    Totems = 34318,
    Souls = 34469,
    Survivor = 34319,
    GateObjective = 272613,
    GrubnorObjective = 272612,
    PlacedObjective = 272761,
    BorgalObjective = 272540,
    SpiritPhaseSpell = 158051
};

using namespace CampaignRecovery;

} // namespace BloodmaulEvents

class player_bloodmaul_events : public PlayerScript
{
public:
    player_bloodmaul_events() : PlayerScript("player_bloodmaul_events")
    {
    }

    void OnLogout(Player* player) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = encounters.find(player->GetGUID());
        if (it != encounters.end())
        {
            it->second.Begin();
            it->second.End(player);
            encounters.erase(it);
        }
    }

    void OnMapChanged(Player* player) override
    {
        if (player->GetMapId() != 1116)
        {
            player->RemoveAurasDueToSpell(BloodmaulEvents::SpiritPhaseSpell);
            OnLogout(player); // Old-map personal summons also have a finite lifetime.
        }
    }

    void OnUpdate(Player* player, uint32 diff) override
    {
        using namespace BloodmaulEvents;
        if (player->GetMapId() != 1116 || player->GetTeamId() != TEAM_HORDE)
            return;
        std::lock_guard<std::mutex> lock(mutex);
        Encounter& state = encounters[player->GetGUID()];
        state.timer += diff;
        if (state.timer < 2000)
            return;
        state.timer = 0;
        state.Begin();

        bool gateActive = Active(player, Gate);
        bool gateDone = player->GetQuestRewardStatus(Gate);
        bool finished = player->GetQuestRewardStatus(Survivor);
        bool entered = player->GetQuestObjectiveData(Gate, GateObjective) != 0;
        bool inCampaignArea = player->GetPositionX() > 7250 && player->GetPositionX() < 7850 &&
                              player->GetPositionY() > 4880 && player->GetPositionY() < 5300;
        // AddAura restores only the phase. It does not replay scene 580 or
        // credit an objective on login. Remove it when leaving the campaign.
        bool spirit = !finished && inCampaignArea && (gateActive || gateDone);
        if (spirit && !player->HasAura(SpiritPhaseSpell))
            player->AddAura(SpiritPhaseSpell, player);
        else if (!spirit && player->HasAura(SpiritPhaseSpell))
            player->RemoveAurasDueToSpell(SpiritPhaseSpell);

        if ((!gateActive || !inCampaignArea) && state.introRemaining)
        {
            state.introRemaining = 0;
            Position pos = player->GetPosition();
            player->SendSpellScene(580, nullptr, false, &pos);
        }
        if (gateActive)
        {
            if (!state.introRemaining && !player->GetQuestObjectiveData(Gate, 272614) && player->IsAlive() &&
                player->GetDistance2d(7384, 5027) < 25.0f && !player->isInFlight())
            {
                player->CastSpell(player, SpiritPhaseSpell, true);
                state.introRemaining = 60000;
            }
            if (state.introRemaining)
            {
                if (player->HasSceneStatus(580, SCENE_COMPLETE))
                    state.introRemaining = 0;
                else if (state.introRemaining > 2000)
                    state.introRemaining -= 2000;
                else
                {
                    state.introRemaining = 0;
                    Position pos = player->GetPosition();
                    player->SendSpellScene(580, nullptr, false, &pos);
                }
            }
            if (!state.introRemaining && !player->GetQuestObjectiveData(Gate, GrubnorObjective))
                // Final Grubnor position in native scene script 11609.
                state.At(player, 1, 78003, 7321.39f, 5099.22f, false, true);
            if (!entered)
                state.At(player, 2, 229026, 7306, 5083, true);
            else
                state.At(player, 3, 78792, 7313, 5089, false, false, true);
        }
        if (gateDone && !finished)
        {
            state.At(player, 4, 78428, 7319, 5095, false, false, true);
            if (player->GetQuestStatus(Totems) == QUEST_STATUS_INCOMPLETE)
            {
                // Personal loot containers use existing loot 52578 / item 110378.
                // These recovery positions lie inside the native collection POI.
                static float const sites[][2] = {{7450, 5100}, {7485, 5180}, {7560, 5190}, {7630, 5100}, {7700, 5030}};
                for (uint32 i = 0; i < 5; ++i)
                    state.At(player, 10 + i, 229131, sites[i][0], sites[i][1], true);
            }
            if (player->GetQuestStatus(Souls) == QUEST_STATUS_INCOMPLETE)
            {
                static float const sites[][2] = {{7438, 5005}, {7470, 5025}, {7510, 5040}, {7550, 5030}, {7600, 5040},
                                                 {7640, 5065}, {7650, 5115}, {7590, 5180}, {7520, 5190}, {7460, 5140}};
                static uint32 const entries[] = {77958, 77959, 77965, 77966};
                for (uint32 i = 0; i < 10; ++i)
                    state.At(player, 20 + i, entries[i % 4], sites[i][0], sites[i][1], false, true);
            }
            if (Active(player, Survivor))
            {
                uint32 placed = player->GetQuestObjectiveData(Survivor, PlacedObjective);
                if (placed == 0)
                    state.At(player, 40, 78386, 7702, 5169);
                if (placed == 1)
                    state.At(player, 41, 78393, 7718, 5193);
                if (placed >= 2 && !player->GetQuestObjectiveData(Survivor, BorgalObjective))
                    state.At(player, 42, 77997, 7710, 5182, false, true);
                if (player->GetQuestObjectiveData(Survivor, BorgalObjective))
                    state.At(player, 43, 78821, 7699, 5210, false, false, true);
            }
        }
        state.End(player);
    }

private:
    std::mutex mutex;
    std::map<ObjectGuid, BloodmaulEvents::Encounter> encounters;
};

class go_bloodmaul_shadow_gate : public GameObjectScript
{
public:
    go_bloodmaul_shadow_gate() : GameObjectScript("go_bloodmaul_shadow_gate")
    {
    }
    bool OnGossipHello(Player* player, GameObject* gate) override
    {
        using namespace BloodmaulEvents;
        if (gate->GetOwnerGUID() != player->GetGUID() || !player->IsAlive() ||
            player->GetQuestStatus(Gate) != QUEST_STATUS_INCOMPLETE ||
            !player->GetQuestObjectiveData(Gate, GrubnorObjective) ||
            player->GetQuestObjectiveData(Gate, GateObjective))
            return true;
        // Actual gate interaction after Grubnor's death. The introduction has
        // already played; do not replay it here. Core handles GO credit.
        player->CastSpell(player, 158396, true);
        return false;
    }
};

class npc_bloodmaul_ritual_totem : public CreatureScript
{
public:
    npc_bloodmaul_ritual_totem() : CreatureScript("npc_bloodmaul_ritual_totem")
    {
    }
    struct AI : ScriptedAI
    {
        AI(Creature* creature) : ScriptedAI(creature)
        {
        }
        bool activated = false;
        void OnSpellClick(Unit* clicker) override
        {
            using namespace BloodmaulEvents;
            Player* player = clicker->ToPlayer();
            if (!player || activated || !Owned(me, player) || !player->IsAlive() ||
                player->GetQuestStatus(Survivor) != QUEST_STATUS_INCOMPLETE)
                return;
            uint32 placed = player->GetQuestObjectiveData(Survivor, PlacedObjective);
            uint32 expected = me->GetEntry() == 78386 ? 0 : 1;
            if (placed != expected)
                return;
            activated = true;
            // The native npc_spellclick_spells binding supplies the visual.
            player->KilledMonsterCredit(78386);
        }
    };
    CreatureAI* GetAI(Creature* creature) const override
    {
        return new AI(creature);
    }
};

class npc_bloodmaul_campaign_enemy : public CreatureScript
{
public:
    npc_bloodmaul_campaign_enemy() : CreatureScript("npc_bloodmaul_campaign_enemy")
    {
    }
    struct AI : ScriptedAI
    {
        AI(Creature* creature) : ScriptedAI(creature)
        {
        }
        uint32 purified = 0;
        void Reset() override
        {
            purified = 0;
        }
        void SetData(uint32, uint32 value) override
        {
            purified = value;
        }
        uint32 GetData(uint32) const override
        {
            return purified;
        }
        void UpdateAI(uint32) override
        {
            if (UpdateVictim())
                DoMeleeAttackIfReady();
        }
    };
    CreatureAI* GetAI(Creature* creature) const override
    {
        return new AI(creature);
    }
};

class spell_bloodmaul_purify_soul : public SpellScriptLoader
{
public:
    spell_bloodmaul_purify_soul() : SpellScriptLoader("spell_bloodmaul_purify_soul")
    {
    }
    class Script : public SpellScript
    {
        PrepareSpellScript(Script);
        Creature* FindSoul()
        {
            Player* player = GetCaster()->ToPlayer();
            if (!player || player->GetQuestStatus(BloodmaulEvents::Souls) != QUEST_STATUS_INCOMPLETE ||
                !player->HasItemCount(110394, 1))
                return nullptr;
            std::list<Creature*> souls;
            for (uint32 entry : {77958, 77959, 77965, 77966})
                player->GetCreatureListWithEntryInGridAppend(souls, entry, 5.0f);
            Creature* nearest = nullptr;
            for (Creature* soul : souls)
                if (!soul->IsAlive() && BloodmaulEvents::Owned(soul, player) && soul->IsAIEnabled &&
                    !soul->AI()->GetData(1) && player->IsWithinLOSInMap(soul) &&
                    (!nearest || player->GetDistance(soul) < player->GetDistance(nearest)))
                    nearest = soul;
            return nearest;
        }
        SpellCastResult Check()
        {
            return FindSoul() ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
        }
        void Select(WorldObject*& target)
        {
            target = FindSoul();
        }
        void Hit(SpellEffIndex)
        {
            Player* player = GetCaster()->ToPlayer();
            Creature* soul = GetHitCreature();
            if (!player || !soul || soul->IsAlive() || !BloodmaulEvents::Owned(soul, player) || !soul->IsAIEnabled ||
                soul->AI()->GetData(1) || !player->HasItemCount(110394, 1) ||
                player->GetQuestStatus(BloodmaulEvents::Souls) != QUEST_STATUS_INCOMPLETE)
                return;
            soul->AI()->SetData(1, 1);
            // 158282 gives credit 78431 to its explicit player target and
            // summons the original purified spirit. No extra scripted credit.
            soul->CastSpell(player, 158282, true);
            soul->DespawnOrUnsummon(2000);
        }
        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(Script::Check);
            OnObjectTargetSelect += SpellObjectTargetSelectFn(Script::Select, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
            OnEffectHitTarget += SpellEffectFn(Script::Hit, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };
    SpellScript* GetSpellScript() const override
    {
        return new Script();
    }
};

void AddSC_bloodmaul_campaign_events()
{
    new player_bloodmaul_events();
    new go_bloodmaul_shadow_gate();
    new npc_bloodmaul_ritual_totem();
    new npc_bloodmaul_campaign_enemy();
    new spell_bloodmaul_purify_soul();
}
