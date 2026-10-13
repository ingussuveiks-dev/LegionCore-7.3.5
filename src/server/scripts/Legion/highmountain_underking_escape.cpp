#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "GameObject.h"

namespace UnderkingEscape
{
constexpr uint32 Dargrul = 97442, Escape = 192468;
// Cave ground beside the existing Neltharion's Lair portal, below the mesa.
Position const Start = {3617.0f,3928.0f,907.1f,0.0f};
Position const Exit = {3611.37f,3927.83f,907.282f,3.14f};
bool Source(Unit* boss)
{
    return boss && boss->GetEntry() == 97266 && !boss->IsAlive() &&
        boss->GetMapId() == 1220 && boss->HasPhaseId(9180) && boss->GetExactDist(&Start) < 90.0f;
}
}

void StartHighmountainUnderkingEscape(Creature* boss)
{
    if (UnderkingEscape::Source(boss))
        boss->SummonCreature(UnderkingEscape::Dargrul,UnderkingEscape::Start,TEMPSUMMON_TIMED_DESPAWN,30000);
}

struct npc_highmountain_underking_escape : ScriptedAI
{
    npc_highmountain_underking_escape(Creature* npc) : ScriptedAI(npc) { }
    enum Stage { Waiting, Speaking, Casting, Running, Finished };
    Stage stage = Waiting;
    uint32 timer = 2000;
    bool active = false;
    void IsSummonedBy(Unit* summoner) override
    {
        if (active) return;
        if (!UnderkingEscape::Source(summoner)) { me->DespawnOrUnsummon(); return; }
        active = true;
        me->SetPhaseId(summoner->GetPhases(),true);
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        me->SetWalk(false);
    }
    void SpellFinishCast(SpellInfo const* spell) override
    {
        if (!active || stage != Casting || spell->Id != UnderkingEscape::Escape) return;
        // The native 1.3 s spell supplies the hammer animation and impact kits.
        // Temporary scenery never changes the shared portal or traps players.
        if (GameObject* rubble = me->SummonGameObject(190386,UnderkingEscape::Exit,0,0,0,0,1,30))
            rubble->EnableCollision(false);
        stage = Running; timer = 10000;
        me->GetMotionMaster()->MovePoint(1,UnderkingEscape::Exit);
    }
    void MovementInform(uint32 type, uint32 id) override
    {
        if (!active || stage != Running || type != POINT_MOTION_TYPE || id != 1 ||
            me->GetExactDist(&UnderkingEscape::Exit) > 1.0f) return;
        stage = Finished;
        me->DespawnOrUnsummon();
    }
    void UpdateAI(uint32 diff) override
    {
        if (!active) return;
        if (timer > diff) { timer -= diff; return; }
        switch (stage)
        {
            case Waiting:
                stage = Speaking; timer = 6006;
                Talk(50); // BroadcastText 99475 and its native voice line.
                break;
            case Speaking:
                stage = Casting; timer = 5000;
                DoCast(me,UnderkingEscape::Escape);
                break;
            default:
                stage = Finished; active = false;
                me->DespawnOrUnsummon(); // Missing cast/movement callbacks cannot strand the actor.
                break;
        }
    }
};

class spell_highmountain_underking_escape : public SpellScript
{
    PrepareSpellScript(spell_highmountain_underking_escape);
    void Credit(SpellEffIndex index)
    {
        if (GetCaster()->GetEntry() == UnderkingEscape::Dargrul)
            PreventHitDefaultEffect(index);
    }
    void Register() override
    {
        // Preserve the visual without emitting the native zero-entry kill credit.
        OnEffectHitTarget += SpellEffectFn(spell_highmountain_underking_escape::Credit,EFFECT_0,SPELL_EFFECT_KILL_CREDIT);
    }
};

void AddSC_highmountain_underking_escape()
{
    RegisterCreatureAI(npc_highmountain_underking_escape);
    RegisterSpellScript(spell_highmountain_underking_escape);
}
