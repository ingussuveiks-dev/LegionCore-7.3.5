#ifndef FUZAN_YIGTHREGOR_H
#define FUZAN_YIGTHREGOR_H

namespace FuZanYigthregor
{
inline bool Source(Creature const* actor)
{
    if (!actor || actor->GetEntry()!=109820 || actor->GetDBTableGUIDLow()!=14670030 || actor->GetMapId()!=1625)
        return false;
    Scenario* scenario=sScenarioMgr->GetScenario(actor->GetInstanceId());
    return scenario && scenario->GetScenarioId()==1137;
}
inline void Clear(Creature* actor)
{
    if (!Source(actor)) return;
    actor->CastStop();
    actor->RemoveAreaObject(217787); // Only this actor's Felfire Bomb fields.
}
}

struct npc_fuzan_yigthregor : SmartAI
{
    explicit npc_fuzan_yigthregor(Creature* actor) : SmartAI(actor) { }
    void Reset() override
    {
        FuZanYigthregor::Clear(me);
        SmartAI::Reset();
    }
    void JustDied(Unit* killer) override
    {
        FuZanYigthregor::Clear(me);
        SmartAI::JustDied(killer);
    }
};

// The original missile may resolve after its caster's death. Its child must
// not re-create an infinite field after the native DATA1/1 cleanup.
class spell_fuzan_felfire_bomb : public SpellScript
{
    PrepareSpellScript(spell_fuzan_felfire_bomb);
    bool Load() override { return FuZanYigthregor::Source(GetCaster()->ToCreature()); }
    void Hit(SpellEffIndex effect)
    {
        if (!GetCaster()->IsAlive()) PreventHitDefaultEffect(effect);
    }
    void Register() override
    {
        OnEffectHit+=SpellEffectFn(spell_fuzan_felfire_bomb::Hit,EFFECT_0,SPELL_EFFECT_CREATE_AREATRIGGER);
    }
};
#endif
