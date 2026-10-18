#ifndef LEGION_VREKT_H
#define LEGION_VREKT_H

namespace Vrekt
{
inline bool Active()
{
    auto quest = sQuestDataStore->GetQuestTemplate(44923);
    auto rotation = quest ? sQuestDataStore->GetWorldQuest(quest) : nullptr;
    return rotation && rotation->ResetTime > GameTime::GetGameTime();
}

inline bool Scoped(Creature const* creature)
{
    auto data = creature->GetCreatureData();
    if (!data || creature->GetMapId() != 1220 || data->areaId != 8362)
        return false;
    switch (creature->GetEntry())
    {
        case 115846: case 116180: case 116600: case 116601: return true;
        default: return false;
    }
}
}

// Keep the existing creature spell pool, native automatic scheduling, village
// actions and assistance. Native PetAI/PossessedAI still takes over on charm;
// the suspended original AI retains the content lifecycle/visibility gates.
struct npc_vrekt_rotation : AggressorAI
{
    explicit npc_vrekt_rotation(Creature* creature) : AggressorAI(creature),
        savedFlags(creature->GetUInt32Value(UNIT_FIELD_FLAGS) & GateFlags),
        savedReact(creature->GetReactState()) { }

    static constexpr uint32 GateFlags = UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE;
    uint32 savedFlags;
    ReactStates savedReact;
    bool initialized = false;
    bool active = false;

    bool CanBeVisibleFor(WorldObject const*) const override { return Vrekt::Active(); }

    void Sync(bool force = false)
    {
        bool enabled = Vrekt::Active();
        if (!enabled && me->isCharmed())
        {
            me->RemoveCharmAuras();
            if (me->isCharmed())
                me->RemoveCharmedBy(nullptr);
            me->LastCharmerGUID.Clear();
            force = true;
        }
        if (!force && initialized && active == enabled)
            return;
        initialized = true;
        active = enabled;
        if (!active)
        {
            me->CastStop();
            if (me->IsAlive() && me->isInCombat())
                AggressorAI::EnterEvadeMode();
            me->CombatStop(true);
            me->DeleteThreatList();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, GateFlags);
        }
        else
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, GateFlags & ~savedFlags);
            me->SetFlag(UNIT_FIELD_FLAGS, savedFlags);
            me->SetReactState(savedReact);
        }
        me->UpdateObjectVisibility();
    }

    void InitializeAI() override { AggressorAI::InitializeAI(); Sync(true); }
    void JustRespawned() override { AggressorAI::JustRespawned(); Sync(true); }
    void UpdateLifecycle(uint32) override { Sync(); }
    void UpdateAI(uint32 diff) override
    {
        if (Vrekt::Active())
            AggressorAI::UpdateAI(diff);
    }
    void MoveInLineOfSight(Unit* who) override
    {
        if (Vrekt::Active())
            AggressorAI::MoveInLineOfSight(who);
    }
    void AttackStart(Unit* who) override
    {
        if (Vrekt::Active())
            AggressorAI::AttackStart(who);
    }
    void EnterCombat(Unit* who) override
    {
        if (Vrekt::Active())
            AggressorAI::EnterCombat(who);
    }
    void JustDied(Unit* killer) override
    {
        if (Vrekt::Active())
            AggressorAI::JustDied(killer);
    }
    void DamageTakenAfterScaling(Unit*, uint32& damage, DamageEffectType) override
    {
        if (!Vrekt::Active())
            damage = 0;
    }
};

class npc_vrekt : public CreatureScript
{
public:
    npc_vrekt() : CreatureScript("npc_vrekt") { }
    CreatureAI* GetAI(Creature* creature) const override
    {
        return Vrekt::Scoped(creature) ? new npc_vrekt_rotation(creature) : nullptr;
    }
};
#endif
