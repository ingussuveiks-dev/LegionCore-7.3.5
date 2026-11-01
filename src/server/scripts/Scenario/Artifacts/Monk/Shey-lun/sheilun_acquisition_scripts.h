#ifndef SHEILUN_ACQUISITION_SCRIPTS_H
#define SHEILUN_ACQUISITION_SCRIPTS_H

namespace SheilunAcquisition
{
class StaffScript : public CreatureScript
{
public:
    StaffScript() : CreatureScript("npc_sheilun_owned_staff") { }
    struct AI : ScriptedAI
    {
        explicit AI(Creature* creature) : ScriptedAI(creature)
        {
            // Constructor runs before the first create packet. Select the
            // actual template's invisible alternative, retaining spellclick.
            if (Instance* controller = Controller(me))
                if (controller->StaffSpawn(me))
                    if (uint32 model = me->GetCreatureTemplate()->GetFirstInvisibleModel())
                    {
                        me->SetDisplayId(model); me->SetNativeDisplayId(model);
                    }
        }
    };
    CreatureAI* GetAI(Creature* creature) const override { return new AI(creature); }
};

class GuideScript : public CreatureScript
{
public:
    static bool Driver(Creature* actor)
    {
        Unit* vehicle = actor ? actor->GetVehicleBase() : nullptr;
        // Accessory join is asynchronous. It is already this kite's scoped
        // child while the native seat1 event is still pending.
        if (!vehicle && actor && actor->HasQuestObjectScope())
            if (TempSummon* summon = actor->ToTempSummon())
                vehicle = CreatureAt(actor->GetMap(), summon->GetSummonerGUID());
        return vehicle && vehicle->GetEntry() == KiteEntry && vehicle->HasQuestObjectScope();
    }
    GuideScript() : CreatureScript("npc_sheilun_return_guide") { }
    struct AI : SmartAI
    {
        explicit AI(Creature* creature) : SmartAI(creature) { }
        void sGossipSelect(Player* player, uint32 menu, uint32 option) override
        {
            if (Driver(me)) { SheilunPlan::Close(player); return; }
            Instance* controller = Controller(me);
            if (controller && controller->State().guide == me->GetGUID())
                controller->Request(player, me, menu, option);
            else SmartAI::sGossipSelect(player, menu, option);
        }
    };
    CreatureAI* GetAI(Creature* creature) const override { return new AI(creature); }
    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (Driver(creature)) { SheilunPlan::Close(player); return true; }
        Instance* controller = Controller(creature);
        if (!controller || controller->State().guide != creature->GetGUID()) return false;
        player->PlayerTalkClass->ClearMenus();
        if (controller->Guide(player, creature) && controller->State().travel == Travel::None)
        {
            player->PrepareGossipMenu(creature, 19257, true);
            player->SEND_GOSSIP_MENU(28290, creature->GetGUID());
        }
        else player->CLOSE_GOSSIP_MENU();
        return true;
    }
    bool OnGossipSelect(Player*, Creature* creature, uint32, uint32) override
    {
        if (Driver(creature)) return true;
        Instance* controller = Controller(creature);
        return controller && controller->State().guide == creature->GetGUID();
    }
};

class KiteScript : public CreatureScript
{
public:
    KiteScript() : CreatureScript("npc_sheilun_return_kite") { }
    struct AI : ScriptedAI
    {
        explicit AI(Creature* creature) : ScriptedAI(creature) { }
        void IsSummonedBy(Unit* summoner) override { RegisterKite(me, summoner ? summoner->ToPlayer() : nullptr); }
        void PassengerBoarded(Unit* passenger, int8 seat, bool apply) override
        {
            Player* player = passenger ? passenger->ToPlayer() : nullptr;
            if (!player) return;
            if (Instance* controller = Controller(me)) controller->Boarded(me, player, seat, apply);
            else TransitBoarded(me, player, seat, apply);
        }
        void MovementInform(uint32 type, uint32 point) override
        {
            if (type != EFFECT_MOTION_TYPE || point != FlightPoint) return;
            if (Instance* controller = Controller(me)) controller->FlightEnded(me);
            else TransitEnded(me);
        }
    };
    CreatureAI* GetAI(Creature* creature) const override { return new AI(creature); }
};

class spell_sheilun_manual_claim : public SpellScript
{
    PrepareSpellScript(spell_sheilun_manual_claim);
    uint64 epoch = 0, claim = 0;
    ObjectGuid source;
    uint8 effects = 0;
    bool Load() override { return GetCaster() && GetCaster()->GetMapId() == MapId; }
    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        source = GetOriginalTargetGUID();
        Creature* target = player ? CreatureAt(player->GetMap(), source) : nullptr;
        Instance* controller = Controller(player);
        return controller && controller->BeginClaim(player, target, epoch, claim) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Hit(SpellEffIndex effect)
    {
        Player* player = GetCaster()->ToPlayer();
        if (effect != EFFECT_2 && GetHitPlayer() != player) { PreventHitDefaultEffect(effect); return; }
        Creature* target = player ? CreatureAt(player->GetMap(), source) : nullptr;
        Instance* controller = Controller(player);
        if (!controller || !controller->ClaimEffect(player, target, epoch, claim, uint8(effect)))
            PreventHitDefaultEffect(effect);
        else effects |= 1 << effect;
    }
    void Finished()
    {
        Player* player = GetCaster()->ToPlayer();
        if (Instance* controller = Controller(player)) controller->EndClaim(player, epoch, claim, effects);
    }
    void LaunchScene(SpellEffIndex effect)
    {
        SpellInfo const* child = sSpellMgr->GetSpellInfo(202437);
        if (!child || child->NeedsToBeTriggeredByCaster(GetSpellInfo(), GetCaster()->GetMap()->GetDifficultyID()))
            PreventHitDefaultEffect(effect);
        else Hit(effect);
    }
    void LaunchSceneTarget(SpellEffIndex effect)
    {
        SpellInfo const* child = sSpellMgr->GetSpellInfo(202437);
        if (!child || !child->NeedsToBeTriggeredByCaster(GetSpellInfo(), GetCaster()->GetMap()->GetDifficultyID()))
            PreventHitDefaultEffect(effect);
        else Hit(effect);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_sheilun_manual_claim::Check);
        OnEffectHitTarget += SpellEffectFn(spell_sheilun_manual_claim::Hit, EFFECT_0, SPELL_EFFECT_KILL_CREDIT);
        OnEffectHitTarget += SpellEffectFn(spell_sheilun_manual_claim::Hit, EFFECT_1, SPELL_EFFECT_SEND_EVENT);
        OnEffectLaunch += SpellEffectFn(spell_sheilun_manual_claim::LaunchScene, EFFECT_2, SPELL_EFFECT_TRIGGER_SPELL_4);
        OnEffectLaunchTarget += SpellEffectFn(spell_sheilun_manual_claim::LaunchSceneTarget, EFFECT_2, SPELL_EFFECT_TRIGGER_SPELL_4);
        AfterCast += SpellCastFn(spell_sheilun_manual_claim::Finished);
    }
};

class spell_sheilun_claim_scene : public SpellScript
{
    PrepareSpellScript(spell_sheilun_claim_scene);
    bool allowed = false;
    // The native10ms child may execute after a map transfer. This unique
    // Sheilun scene must retain its ownership/attempt guard on every map.
    bool Load() override { return true; }
    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        Instance* controller = Controller(player);
        return controller && controller->SceneCast(player) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Hit(SpellEffIndex effect)
    {
        Player* player = GetCaster()->ToPlayer();
        Instance* controller = Controller(player);
        allowed = controller && controller->SceneCast(player);
        if (!allowed) PreventHitDefaultEffect(effect);
    }
    void Finished()
    {
        Player* player = GetCaster()->ToPlayer();
        if (allowed) if (Instance* controller = Controller(player)) controller->SceneDone(player);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_sheilun_claim_scene::Check);
        OnEffectHitTarget += SpellEffectFn(spell_sheilun_claim_scene::Hit, EFFECT_0, SPELL_EFFECT_ACTIVATE_SCENE3);
        AfterCast += SpellCastFn(spell_sheilun_claim_scene::Finished);
    }
};

class spell_sheilun_return_boarding : public SpellScript
{
    PrepareSpellScript(spell_sheilun_return_boarding);
    uint64 epoch = 0, request = 0;
    uint8 effects = 0;
    Transit transit;
    bool destination = false;
    bool Load() override
    {
        return GetCaster() && (GetCaster()->GetMapId() == MapId || GetCaster()->GetMapId() == SheilunReturnRoute::ReturnMap);
    }
    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        if (Instance* controller = Controller(player))
            return controller->ReturnCast(player, epoch, request) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
        destination = player && ReadTransit(player->GetGUID(), transit) && TransitLease(player, transit) &&
            transit.leg == Leg::Summoning && transit.kite.IsEmpty();
        return destination ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Hit(SpellEffIndex effect)
    {
        Player* player = GetCaster()->ToPlayer();
        if (effect != EFFECT_0 && GetHitPlayer() != player) { PreventHitDefaultEffect(effect); return; }
        bool allowed = false;
        if (Instance* controller = Controller(player)) allowed = controller->ReturnEffect(player, epoch, request, uint8(effect));
        else if (destination && effect == EFFECT_0 && TransitLease(player, transit))
            EditTransit(transit, [&](Transit& current)
            {
                if (current.leg == Leg::Summoning && current.kite.IsEmpty() && !current.summon)
                { current.summon = true; allowed = true; }
            });
        if (!allowed) PreventHitDefaultEffect(effect);
        else effects |= 1 << effect;
    }
    void Finished()
    {
        Player* player = GetCaster()->ToPlayer();
        if (Instance* controller = Controller(player)) controller->EndReturnCast(player, epoch, request, effects);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_sheilun_return_boarding::Check);
        OnEffectHit += SpellEffectFn(spell_sheilun_return_boarding::Hit, EFFECT_0, SPELL_EFFECT_SUMMON);
        OnEffectHitTarget += SpellEffectFn(spell_sheilun_return_boarding::Hit, EFFECT_1, SPELL_EFFECT_KILL_CREDIT);
        OnEffectHitTarget += SpellEffectFn(spell_sheilun_return_boarding::Hit, EFFECT_2, SPELL_EFFECT_SEND_EVENT);
        AfterCast += SpellCastFn(spell_sheilun_return_boarding::Finished);
    }
};

class spell_sheilun_return_transfer : public SpellScript
{
    PrepareSpellScript(spell_sheilun_return_transfer);
    bool Load() override { return GetCaster() && GetCaster()->GetMapId() == MapId; }
    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        Instance* controller = Controller(player);
        return controller && controller->Transfer(player) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Hit(SpellEffIndex effect)
    {
        Player* player = GetCaster()->ToPlayer();
        Instance* controller = Controller(player);
        if (!controller || !controller->Transfer(player)) PreventHitDefaultEffect(effect);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_sheilun_return_transfer::Check);
        OnEffectHitTarget += SpellEffectFn(spell_sheilun_return_transfer::Hit, EFFECT_0, SPELL_EFFECT_TELEPORT_L);
    }
};

class PlayerEvents : public PlayerScript
{
public:
    PlayerEvents() : PlayerScript("player_sheilun_acquisition") { }
    void OnLootItem(Player* player, Item* item, uint32) override
    {
        if (Instance* controller = Controller(player)) controller->Stored(player, item);
    }
    void OnUpdate(Player* player, uint32 diff) override { UpdateTransit(player, diff); }
    void OnBeforeLogout(Player* player) override
    {
        if (!player || !player->GetSession() || !player->GetSession()->PlayerLogout() || !player->IsAlive()) return;
        Transit transit;
        // The private transit is proof of the actual earned transfer. Its
        // spatial cleanup also applies to abandonment immediately before
        // logout, before the next map update can consume the expired lease.
        bool destination = ReadTransit(player->GetGUID(), transit) && TransitItem(player, transit);
        Instance* source = Controller(player);
        bool departure = source && source->LogoutDeparture(player);
        if (!destination && !departure) return;
        ObjectGuid kite = destination ? transit.kite : source->State().kite;
        if (player->GetTransport() || player->isInFlight() ||
            (player->GetVehicleBase() && player->GetVehicleBase()->GetGUID() != kite)) return;
        if (player->HasPendingTeleport())
        {
            // Retain an unrelated pending teleport (including death recovery).
            // Only this flight's exact native air transfer needs a safe save.
            SpellTargetPosition const* native = sSpellMgr->GetSpellTargetPosition(202831);
            WorldLocation const& pending = player->GetTeleportDest();
            if (!destination || transit.leg != Leg::Transfer || !native ||
                pending.GetMapId() != native->target_mapId ||
                pending.GetExactDist(native->target_X,native->target_Y,native->target_Z) > .1f) return;
        }
        else if (!player->IsInWorld() ||
            (player->GetMapId() != SheilunReturnRoute::ReturnMap && !departure)) return;
        if (destination) CancelTransit(player, transit);
        else
        {
            RestoreControl(player, source->State().kite);
            if (player->GetVehicleBase() && player->GetVehicleBase()->GetGUID() == source->State().kite) player->ExitVehicle();
        }
        // TeleportTo deliberately ignores calls during logout. Supply the
        // native save destination instead; SaveToDB already serializes it.
        Position landing = Landing();
        player->GetTeleportDest().WorldRelocate(SheilunReturnRoute::ReturnMap, landing.GetPositionX(),
            landing.GetPositionY(), landing.GetPositionZ(), landing.GetOrientation());
        player->SetSemaphoreTeleportFar(true);
    }
    void OnLogout(Player* player) override
    {
        Transit transit;
        if (ReadTransit(player->GetGUID(), transit)) CancelTransit(player, transit);
    }
    void OnMapChanged(Player* player) override
    {
        Transit transit;
        if (ReadTransit(player->GetGUID(), transit) && player->GetMapId() != SheilunReturnRoute::ReturnMap)
            CancelTransit(player, transit);
    }
};

inline void Started(Player* player) { if (Instance* instance = Controller(player)) instance->Started(player); }
inline void BattleAlly(Creature* boss, Creature* ally) { if (Instance* instance = Controller(boss)) instance->BattleAlly(boss, ally); }
inline void Defeated(Creature* boss, Unit* killer) { if (Instance* instance = Controller(boss)) instance->Defeated(boss, killer); }
inline void Register()
{
    new StaffScript(); new GuideScript(); new KiteScript(); new PlayerEvents();
    RegisterSpellScript(spell_sheilun_manual_claim);
    RegisterSpellScript(spell_sheilun_claim_scene);
    RegisterSpellScript(spell_sheilun_return_boarding);
    RegisterSpellScript(spell_sheilun_return_transfer);
}
}

// Preserve the exact DB-bound registration name when moving the formerly empty
// instance factory out of the shared Rogue translation unit.
struct instance_sheylun : SheilunAcquisition::Instance
{
    explicit instance_sheylun(InstanceMap* map) : Instance(map) { }
};
#endif
