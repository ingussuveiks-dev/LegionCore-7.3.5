#include "ScriptMgr.h"
#include "SpellScript.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "QuestData.h"

namespace
{
bool HasKarazhanQuest(Player* player, uint32 quest)
{
    return player && player->GetMapId() == 0 && player->IsAlive() &&
           player->GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE;
}

Creature* FindKarazhanSample(Player* player)
{
    if (!HasKarazhanQuest(player, 44684))
        return nullptr;
    Creature* soil = player->FindNearestCreature(114641, 10.0f);
    Creature* water = player->FindNearestCreature(114645, 10.0f);
    if (soil && player->GetQuestObjectiveData(44684, 114641))
        soil = nullptr;
    if (water && player->GetQuestObjectiveData(44684, 114645))
        water = nullptr;
    if (!soil)
        return water;
    if (!water)
        return soil;
    return player->GetDistance(soil) < player->GetDistance(water) ? soil : water;
}
} // namespace

// Item 141878 uses native dummy spell 228271 (nearby-entry radius 10).
class spell_karazhan_collect_sample : public SpellScriptLoader
{
public:
    spell_karazhan_collect_sample() : SpellScriptLoader("spell_karazhan_collect_sample")
    {
    }
    class Script : public SpellScript
    {
        PrepareSpellScript(Script);
        SpellCastResult Check()
        {
            return FindKarazhanSample(GetCaster()->ToPlayer()) ? SPELL_CAST_OK : SPELL_FAILED_NOT_HERE;
        }
        void Select(WorldObject*& target)
        {
            target = FindKarazhanSample(GetCaster()->ToPlayer());
        }
        void Hit(SpellEffIndex)
        {
            Player* player = GetCaster()->ToPlayer();
            Creature* sample = GetHitCreature();
            if (!HasKarazhanQuest(player, 44684) || !sample || !player->IsWithinDistInMap(sample, 10.0f))
                return;
            if (sample->GetEntry() == 114641 || sample->GetEntry() == 114645)
                player->KilledMonsterCredit(sample->GetEntry(), sample->GetGUID());
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

// Credit remains in each native effect 1. The missing dummy interaction must
// validate the clicked actor and consume it before another cast can credit it.
class spell_karazhan_disable_node : public SpellScriptLoader
{
public:
    spell_karazhan_disable_node() : SpellScriptLoader("spell_karazhan_disable_node")
    {
    }
    class Script : public SpellScript
    {
        PrepareSpellScript(Script);
        uint32 Entry()
        {
            switch (GetSpellInfo()->Id)
            {
            case 229466:
                return 115414;
            case 228208:
                return 115027;
            case 231458:
                return 115037;
            default:
                return 0;
            }
        }
        uint32 QuestId()
        {
            return GetSpellInfo()->Id == 229466 ? 44557 : 44683;
        }
        Creature* Target()
        {
            Unit* target = GetExplTargetUnit();
            Creature* node = target ? target->ToCreature() : nullptr;
            Player* player = GetCaster()->ToPlayer();
            if (!HasKarazhanQuest(player, QuestId()) || !node || !node->IsAlive() || node->GetEntry() != Entry() ||
                node->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE) || !player->IsWithinDistInMap(node, 10.0f))
                return nullptr;
            return node;
        }
        SpellCastResult Check()
        {
            return Target() ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
        }
        void Select(WorldObject*& target)
        {
            target = Target();
        }
        void Credit(SpellEffIndex index)
        {
            Creature* node = Target();
            if (!node)
            {
                PreventHitDefaultEffect(index);
                return;
            }
            node->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            node->DespawnOrUnsummon(100, Seconds(60));
        }
        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(Script::Check);
            if (m_scriptSpellId != 229466)
                OnObjectTargetSelect += SpellObjectTargetSelectFn(Script::Select, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
            OnEffectHitTarget += SpellEffectFn(Script::Credit, EFFECT_1, SPELL_EFFECT_KILL_CREDIT2);
        }
    };
    SpellScript* GetSpellScript() const override
    {
        return new Script();
    }
};

class npc_karazhan_quest_node : public CreatureScript
{
public:
    npc_karazhan_quest_node() : CreatureScript("npc_karazhan_quest_node")
    {
    }
    struct AI : ScriptedAI
    {
        explicit AI(Creature* creature) : ScriptedAI(creature)
        {
        }
        void Reset() override
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
        }
    };
    CreatureAI* GetAI(Creature* creature) const override
    {
        return new AI(creature);
    }
};

class player_karazhan_arrival : public PlayerScript
{
public:
    player_karazhan_arrival() : PlayerScript("player_karazhan_arrival")
    {
    }
    void OnMapChanged(Player* player) override
    {
        // Complete the optional activation marker only after the native
        // portal's transfer actually arrives. Old quest logs remain finishable.
        if (player->GetMapId() == 0 && player->GetDistance(-11144.5f, -2108.11f, 49.8f) < 25.0f &&
            (player->GetQuestStatus(44556) == QUEST_STATUS_INCOMPLETE ||
             player->GetQuestStatus(44556) == QUEST_STATUS_COMPLETE) &&
            player->GetQuestObjectiveData(44556, 115871) && !player->GetQuestObjectiveData(44556, 114322))
            player->CastSpell(player, 230205, true);
    }
};

void AddSC_karazhan_campaign()
{
    new npc_karazhan_quest_node();
    new spell_karazhan_collect_sample();
    new spell_karazhan_disable_node();
    new player_karazhan_arrival();
}
