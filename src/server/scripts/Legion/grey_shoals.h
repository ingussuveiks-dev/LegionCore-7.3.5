#ifndef LEGION_GREY_SHOALS_H
#define LEGION_GREY_SHOALS_H

namespace GreyShoals
{
inline bool Active()
{
    auto quest = sQuestDataStore->GetQuestTemplate(44737);
    auto rotation = quest ? sQuestDataStore->GetWorldQuest(quest) : nullptr;
    return rotation && rotation->ResetTime > GameTime::GetGameTime();
}

inline bool Scoped(Creature const* creature)
{
    auto data = creature->GetCreatureData();
    if (!data || creature->GetMapId() != 1220 || data->areaId != 8297)
        return false;
    switch (creature->GetEntry())
    {
        case 115025: case 115026: case 115031: case 115751: return true;
        default: return false;
    }
}

inline bool Scoped(GameObject const* object)
{
    auto data = object->GetGOData();
    return data && object->GetMapId() == 1220 && data->areaId == 8297 && object->GetEntry() == 266170;
}
}

// Retain all database combat events. Only the outdoor DB spawns participate;
// summons and actors using the same entries elsewhere retain ordinary SmartAI.
struct npc_grey_shoals_rotation : SmartAI
{
    explicit npc_grey_shoals_rotation(Creature* creature) : SmartAI(creature),
        savedFlags(creature->GetUInt32Value(UNIT_FIELD_FLAGS) & GateFlags),
        savedReact(creature->GetReactState()) { }

    static constexpr uint32 GateFlags = UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE;
    uint32 savedFlags;
    ReactStates savedReact;
    bool initialized = false;
    bool active = false;

    bool CanBeVisibleFor(WorldObject const*) const override { return GreyShoals::Active(); }

    void Sync(bool force = false)
    {
        bool enabled = GreyShoals::Active();
        if (!enabled && me->isCharmed())
        {
            // SmartAI retains this script during ordinary charm/possession.
            // End control before hiding, using the native aura/control cleanup
            // so the charmer's bar, mover and faction are restored as usual.
            me->RemoveCharmAuras();
            if (me->isCharmed())
                me->RemoveCharmedBy(nullptr); // Direct/scripted charm, no aura.
            me->LastCharmerGUID.Clear();
            force = true; // CharmInfo cleanup may restore an aggressive state.
        }
        if (!force && initialized && active == enabled)
            return;
        initialized = true;
        active = enabled;
        if (!active)
        {
            me->CastStop();
            if (me->IsAlive() && me->isInCombat())
                SmartAI::EnterEvadeMode();
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

    void InitializeAI() override { SmartAI::InitializeAI(); Sync(true); }
    void JustRespawned() override { SmartAI::JustRespawned(); Sync(true); }
    void UpdateLifecycle(uint32) override { Sync(); }
    void UpdateAI(uint32 diff) override
    {
        if (GreyShoals::Active())
            SmartAI::UpdateAI(diff);
    }
    void MoveInLineOfSight(Unit* who) override
    {
        if (GreyShoals::Active())
            SmartAI::MoveInLineOfSight(who);
    }
    void DamageTakenAfterScaling(Unit*, uint32& damage, DamageEffectType) override
    {
        // Enforce the exact expiry boundary, even before the next update tick.
        if (!GreyShoals::Active())
            damage = 0;
    }
};

class npc_grey_shoals : public CreatureScript
{
public:
    npc_grey_shoals() : CreatureScript("npc_grey_shoals") { }
    CreatureAI* GetAI(Creature* creature) const override
    {
        return GreyShoals::Scoped(creature) ? new npc_grey_shoals_rotation(creature) : nullptr;
    }
};

struct go_grey_shoals_rotation : GameObjectAI
{
    explicit go_grey_shoals_rotation(GameObject* object) : GameObjectAI(object) { }
    bool initialized = false;
    bool active = false;
    bool CanBeVisibleFor(WorldObject const*) const override { return GreyShoals::Active(); }
    void InitializeAI() override { UpdateAI(0); }
    void UpdateAI(uint32) override
    {
        bool enabled = GreyShoals::Active();
        if (!initialized || active != enabled)
        {
            initialized = true;
            active = enabled;
            go->UpdateObjectVisibility();
        }
    }
};
#endif
