#ifndef LEGION_VREKT_SCENE_H
#define LEGION_VREKT_SCENE_H

namespace VrektScene
{
constexpr uint64 BossSpawn = 146706022;
constexpr uint64 VillageDrakeSpawn = 293175;
constexpr uint32 CapturedDrake = 115885;
constexpr uint32 CapturedPose = 230506;
constexpr uint32 Chain = 211409;
constexpr int32 Release = 1;
constexpr uint32 Takeoff = 1;
constexpr uint32 Escape = 2;

inline bool Boss(Creature const* creature)
{
    return Vrekt::Scoped(creature) && creature->GetEntry() == 115846 &&
        creature->GetDBTableGUIDLow() == BossSpawn;
}

inline bool VillageDrake(Creature const* creature)
{
    auto data = creature->GetCreatureData();
    return data && data->areaId == 8362 && creature->GetMapId() == 1220 &&
        creature->GetEntry() == 107830 && creature->GetDBTableGUIDLow() == VillageDrakeSpawn;
}
}

struct npc_vrekt_captured_drake : PassiveAI
{
    explicit npc_vrekt_captured_drake(Creature* creature) : PassiveAI(creature) { }

    bool released = false;
    uint32 retry = 0;
    uint32 departure = 0;
    uint32 timeout = 0;
    uint32 movement = 0;

    Creature* Boss() const
    {
        auto summon = me->ToTempSummon();
        auto owner = summon ? summon->GetSummoner() : nullptr;
        auto creature = owner ? owner->ToCreature() : nullptr;
        return creature && VrektScene::Boss(creature) ? creature : nullptr;
    }

    bool CanBeVisibleFor(WorldObject const*) const override { return Vrekt::Active(); }
    void DamageTakenAfterScaling(Unit*, uint32& damage, DamageEffectType) override { damage = 0; }

    void InitializeAI() override
    {
        PassiveAI::InitializeAI();
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        // Keep the small grid around the escape spline updating after the killer leaves.
        me->setActive(true);
    }

    void UpdateLifecycle(uint32 diff) override
    {
        if (!Vrekt::Active())
        {
            me->DespawnOrUnsummon();
            return;
        }
        if (me->isCharmed())
        {
            me->RemoveCharmAuras();
            if (me->isCharmed())
                me->RemoveCharmedBy(nullptr);
            me->LastCharmerGUID.Clear();
        }
        if (released)
        {
            if (timeout <= diff)
            {
                me->DespawnOrUnsummon();
                return;
            }
            timeout -= diff;
            if (departure)
            {
                if (departure > diff)
                    departure -= diff;
                else
                {
                    departure = 0;
                    me->SetCanFly(true);
                    me->SetDisableGravity(true);
                    movement = VrektScene::Takeoff;
                    me->GetMotionMaster()->MoveTakeoff(VrektScene::Takeoff,
                        me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 8.0f);
                }
            }
            return;
        }
        auto boss = Boss();
        if (!boss)
        {
            me->DespawnOrUnsummon();
            return;
        }
        // The original boss AI can be suspended by native charm when it dies.
        // Death, not combat start or a failed cast, releases this exact summon.
        if (!boss->IsAlive())
        {
            DoAction(VrektScene::Release);
            return;
        }
        if (retry > diff)
        {
            retry -= diff;
            return;
        }
        retry = 1000;
        if (!me->HasAura(VrektScene::CapturedPose))
            me->CastSpell(me, VrektScene::CapturedPose, true);
        if (!boss->HasAura(VrektScene::Chain, me->GetGUID()))
            me->CastSpell(boss, VrektScene::Chain, true);
    }

    void DoAction(int32 const action) override
    {
        auto boss = Boss();
        if (action != VrektScene::Release || released || !Vrekt::Active() || !boss || boss->IsAlive())
            return;
        released = true;
        boss->RemoveAurasDueToSpell(VrektScene::Chain, me->GetGUID());
        me->RemoveAurasDueToSpell(VrektScene::CapturedPose);
        departure = 1000;
        timeout = 20000;
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (!Vrekt::Active())
        {
            me->DespawnOrUnsummon();
            return;
        }
        if (!released || departure || type != EFFECT_MOTION_TYPE || id != movement)
            return;
        if (id == VrektScene::Takeoff)
        {
            movement = VrektScene::Escape;
            // Reconstructed outward/upward escape, not a recovered retail spline.
            // The negative-Y route intersects static collision just after lift.
            auto const& home = me->GetHomePosition();
            Position destination(home.GetPositionX() - 45.0f, home.GetPositionY() + 60.0f,
                home.GetPositionZ() + 60.0f, home.GetOrientation());
            me->GetMotionMaster()->MoveSmoothFlyPath(VrektScene::Escape, destination, 9.0f);
        }
        else if (id == VrektScene::Escape)
        {
            movement = 0;
            me->DespawnOrUnsummon();
        }
    }
};

struct npc_vrekt_vaettfang : npc_vrekt_rotation
{
    explicit npc_vrekt_vaettfang(Creature* creature) : npc_vrekt_rotation(creature) { }
    ObjectGuid drake;
    uint32 retry = 0;

    void Cleanup()
    {
        if (drake)
            me->RemoveAurasDueToSpell(VrektScene::Chain, drake);
        if (auto creature = ObjectAccessor::GetCreature(*me, drake))
            creature->DespawnOrUnsummon();
        drake.Clear();
    }

    void UpdateLifecycle(uint32 diff) override
    {
        npc_vrekt_rotation::UpdateLifecycle(diff);
        if (!Vrekt::Active())
        {
            Cleanup();
            return;
        }
        if (!me->IsAlive())
            return; // The exact summon also observes its dead owner without a killer/player dependency.
        if (retry > diff)
        {
            retry -= diff;
            return;
        }
        retry = 1000;
        if (auto creature = ObjectAccessor::GetCreature(*me, drake))
            if (creature->IsAlive())
                return;
        if (drake)
            me->RemoveAurasDueToSpell(VrektScene::Chain, drake);
        drake.Clear();
        // Reuse the recorded village drake's home, without changing its DB spawn.
        // Its use for the WQ replacement is a reconstruction, recorded in the audit.
        auto anchor = sObjectMgr->GetCreatureData(VrektScene::VillageDrakeSpawn);
        if (!anchor || anchor->id != 107830 || anchor->mapid != 1220 || anchor->areaId != 8362)
            return;
        Position position(anchor->posX, anchor->posY, anchor->posZ, anchor->orientation);
        if (auto creature = me->SummonCreature(VrektScene::CapturedDrake, position, TEMPSUMMON_DEAD_DESPAWN))
        {
            if (dynamic_cast<npc_vrekt_captured_drake*>(creature->AI()))
            {
                drake = creature->GetGUID();
                creature->AI()->UpdateLifecycle(0);
            }
            else
                creature->DespawnOrUnsummon();
        }
    }

    void JustRespawned() override
    {
        Cleanup();
        retry = 0;
        npc_vrekt_rotation::JustRespawned();
    }

    void JustDied(Unit* killer) override
    {
        npc_vrekt_rotation::JustDied(killer);
        if (auto creature = ObjectAccessor::GetCreature(*me, drake))
            creature->AI()->DoAction(VrektScene::Release);
    }
};

// Only the original village spawn is replaced while the WQ is active.
// Keep SmartAI and all unrelated village spawns/scripts on their native path.
struct npc_vrekt_village_drake : SmartAI
{
    explicit npc_vrekt_village_drake(Creature* creature) : SmartAI(creature),
        flags(creature->GetUInt32Value(UNIT_FIELD_FLAGS) & GateFlags), react(creature->GetReactState()) { }
    static constexpr uint32 GateFlags = UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE;
    uint32 flags;
    ReactStates react;
    bool hidden = false;

    void InitializeAI() override
    {
        SmartAI::InitializeAI();
        hidden = !Vrekt::Active();
        UpdateLifecycle(0);
    }
    void JustRespawned() override
    {
        SmartAI::JustRespawned();
        hidden = !Vrekt::Active();
        UpdateLifecycle(0);
    }

    bool CanBeVisibleFor(WorldObject const*) const override { return !Vrekt::Active(); }
    void DamageTakenAfterScaling(Unit*, uint32& damage, DamageEffectType) override
    {
        if (Vrekt::Active())
            damage = 0;
    }
    void UpdateLifecycle(uint32) override
    {
        bool hide = Vrekt::Active();
        if (hide && me->isCharmed())
        {
            me->RemoveCharmAuras();
            if (me->isCharmed())
                me->RemoveCharmedBy(nullptr);
            me->LastCharmerGUID.Clear();
        }
        if (hide == hidden)
            return;
        hidden = hide;
        if (hide)
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
            me->RemoveFlag(UNIT_FIELD_FLAGS, GateFlags & ~flags);
            me->SetFlag(UNIT_FIELD_FLAGS, flags);
            me->SetReactState(react);
        }
        me->UpdateObjectVisibility();
    }
    void UpdateAI(uint32 diff) override { if (!Vrekt::Active()) SmartAI::UpdateAI(diff); }
    void MoveInLineOfSight(Unit* who) override { if (!Vrekt::Active()) SmartAI::MoveInLineOfSight(who); }
    void AttackStart(Unit* who) override { if (!Vrekt::Active()) SmartAI::AttackStart(who); }
};

class npc_vrekt_scene : public CreatureScript
{
public:
    npc_vrekt_scene() : CreatureScript("npc_vrekt_scene") { }
    CreatureAI* GetAI(Creature* creature) const override
    {
        if (VrektScene::Boss(creature))
            return new npc_vrekt_vaettfang(creature);
        if (VrektScene::VillageDrake(creature))
            return new npc_vrekt_village_drake(creature);
        if (creature->GetEntry() == VrektScene::CapturedDrake)
            if (auto summon = creature->ToTempSummon())
                if (auto owner = summon->GetSummoner())
                    if (auto boss = owner->ToCreature())
                        if (VrektScene::Boss(boss))
                            return new npc_vrekt_captured_drake(creature);
        return nullptr;
    }
};
#endif
