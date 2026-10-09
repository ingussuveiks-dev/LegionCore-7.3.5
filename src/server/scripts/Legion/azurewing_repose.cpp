#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "TemporarySummon.h"
#include "DatabaseEnv.h"
#include "ScriptedGossip.h"
#include "QuestData.h"
#include <map>
#include <mutex>

namespace Azurewing
{
bool Active(Player* player, uint32 quest)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 &&
        player->GetZoneId() == 7334 && player->GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE;
}
bool Done(Player* player, uint32 quest, uint32 objective)
{
    return player->GetQuestObjectiveData(quest, objective) != 0;
}
bool CanRide(Player* player)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 && player->GetZoneId() == 7334 &&
        (player->GetQuestStatus(37862) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(37862) == QUEST_STATUS_COMPLETE) &&
        !Done(player, 37862, 107995);
}
void FinishRide(Player* player)
{
    // The native objective is optional. The quest may already be complete;
    // KilledMonsterCredit deliberately skips completed quests in this core.
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(37862))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.ID == 284778 && objective.ObjectID == 107995)
                player->SetQuestObjectiveData(quest, &objective, 1);
}
Creature* Owned(Player* player, uint32 entry)
{
    for (ObjectGuid const& guid : *player->GetSummonList(entry))
        if (Creature* creature = ObjectAccessor::GetCreature(*player, guid))
            if (creature->IsAlive() && creature->ToTempSummon() && creature->ToTempSummon()->GetSummonerGUID() == player->GetGUID())
                return creature;
    return nullptr;
}
void Clear(Player* player, uint32 entry)
{
    GuidList copies = *player->GetSummonList(entry);
    for (ObjectGuid const& guid : copies)
        if (Creature* creature = ObjectAccessor::GetCreature(*player, guid))
            creature->DespawnOrUnsummon();
}
Creature* Personal(Player* player, uint32 entry, Position const& position)
{
    if (Creature* existing = Owned(player, entry))
        return existing;
    return player->SummonCreature(entry, position, TEMPSUMMON_TIMED_DESPAWN, 300000, 0, player->GetGUID());
}
Player* Owner(Creature* creature)
{
    if (TempSummon* summon = creature->ToTempSummon())
        if (Unit* owner = summon->GetSummoner())
            return owner->ToPlayer();
    return nullptr;
}

// Only distinct whelp interactions need a ledger. Quest progress remains owned
// by the core; reconcile a rolled-back save or abandoned run before reuse.
std::mutex interactionMutex;
bool CanRevive(Player* player, Creature* target, uint32 spell)
{
    uint32 quest = spell == 180713 ? 42271 : 37859;
    uint32 entry = spell == 180713 ? 90880 : 90167;
    if (!Active(player, quest) || !target || target->GetEntry() != entry || !target->IsAlive() ||
        !player->IsWithinDistInMap(target, 6.0f) || !player->IsWithinLOSInMap(target) ||
        !player->HasItemCount(spell == 180713 ? 122292 : 122188, 1) ||
        (spell == 180713 && !player->HasItemCount(122306, 4)))
        return false;
    return target->GetDBTableGUIDLow() != 0;
}
bool Revive(Player* player, Creature* target, uint32 spell)
{
    if (!CanRevive(player, target, spell)) return false;
    uint32 quest = spell == 180713 ? 42271 : 37859;
    uint32 entry = spell == 180713 ? 90880 : 90167;
    uint64 spawn = target->GetDBTableGUIDLow();
    std::lock_guard<std::mutex> lock(interactionMutex);
    uint32 progress = player->GetQuestObjectiveData(quest, entry);
    if (progress >= (spell == 180713 ? 4u : 10u))
        return false;
    CharacterDatabase.DirectPExecute("DELETE FROM character_azurewing_interactions WHERE guid=%u AND quest=%u AND ordinal>%u",
        player->GetGUIDLow(), quest, progress);
    if (CharacterDatabase.PQuery("SELECT spawn FROM character_azurewing_interactions WHERE guid=%u AND quest=%u AND spawn=%llu",
        player->GetGUIDLow(), quest, static_cast<unsigned long long>(spawn)))
        return false;
    CharacterDatabase.DirectPExecute("INSERT INTO character_azurewing_interactions (guid,quest,spawn,ordinal) VALUES (%u,%u,%llu,%u)",
        player->GetGUIDLow(), quest, static_cast<unsigned long long>(spawn), progress + 1);
    player->KilledMonsterCredit(entry, target->GetGUID());
    return true;
}

Position const EscortHome = {1076.59f, 6590.37f, 139.738f, 0.793652f};
Position const Orbyth = {610.09f, 6657.11f, 60.6189f, 0.721406f};
Position const GuidePath[] = {
    {1114.41f,6581.29f,139.86f,0}, {1125.06f,6560.87f,145.63f,0},
    {1128.60f,6500.70f,143.40f,0}, {1124.18f,6464.61f,137.50f,0},
    {1093.01f,6429.57f,133.48f,0}, {1103.63f,6412.50f,133.47f,0},
    {1097.70f,6399.60f,133.48f,0}, {1072.30f,6394.16f,133.33f,0},
    {1065.05f,6365.14f,128.62f,0}, {1067.91f,6287.22f,117.33f,0}
};
} // namespace Azurewing

class spell_azurewing_pool : public SpellScript
{
    PrepareSpellScript(spell_azurewing_pool);
    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        if (!Azurewing::Active(player, 37853) || Azurewing::Done(player, 37853, 90315) || !player->HasItemCount(122095, 6))
            return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
        Creature* senegos = player->FindNearestCreature(89975, 35.0f);
        return senegos && player->IsWithinLOSInMap(senegos) ? SPELL_CAST_OK : SPELL_FAILED_OUT_OF_RANGE;
    }
    void Throw(SpellEffIndex index)
    {
        PreventHitDefaultEffect(index);
        if (Check() != SPELL_CAST_OK)
            return;
        Player* player = GetCaster()->ToPlayer();
        Creature* senegos = player->FindNearestCreature(89975, 35.0f);
        player->DestroyItemCount(122095, 6, true);
        // Target 46 uses the explicit unit as its destination fallback.
        player->CastSpell(senegos, 179913, true);
        player->KilledMonsterCredit(90315);
    }
    void SuppressMissingTrigger(SpellEffIndex index) { PreventHitDefaultEffect(index); }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_azurewing_pool::Check);
        OnEffectHitTarget += SpellEffectFn(spell_azurewing_pool::Throw, EFFECT_0, SPELL_EFFECT_REMOVE_AURA);
        OnEffectHitTarget += SpellEffectFn(spell_azurewing_pool::SuppressMissingTrigger, EFFECT_1, SPELL_EFFECT_APPLY_AURA);
    }
};

class spell_azurewing_revive : public SpellScript
{
    PrepareSpellScript(spell_azurewing_revive);
    SpellCastResult Check()
    {
        Unit* target = GetExplTargetUnit();
        return Azurewing::CanRevive(GetCaster()->ToPlayer(), target ? target->ToCreature() : nullptr, GetSpellInfo()->Id)
            ? SPELL_CAST_OK : SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
    }
    void Credit(SpellEffIndex index)
    {
        // Both native spells put their credit on the caster, independently of
        // the clicked whelp. Validate the actual explicit target first.
        PreventHitDefaultEffect(index);
        Unit* target = GetExplTargetUnit();
        Azurewing::Revive(GetCaster()->ToPlayer(), target ? target->ToCreature() : nullptr, GetSpellInfo()->Id);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_azurewing_revive::Check);
        OnEffectHitTarget += SpellEffectFn(spell_azurewing_revive::Credit, EFFECT_1, SPELL_EFFECT_ANY);
    }
};

class spell_azurewing_pylon : public SpellScript
{
    PrepareSpellScript(spell_azurewing_pylon);
    void Hit(SpellEffIndex)
    {
        Player* player = GetCaster()->ToPlayer();
        Creature* target = GetHitCreature();
        if (!Azurewing::Active(player, 37860) || !target)
            return;
        switch (target->GetEntry())
        {
            case 90263: case 100383: case 100384: case 100385:
                player->KilledMonsterCredit(100386);
                break;
            default: break;
        }
    }
    void MissingTrigger(SpellEffIndex index) { PreventHitDefaultEffect(index); }
    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_azurewing_pylon::Hit, EFFECT_0, SPELL_EFFECT_DUMMY);
        OnEffectLaunch += SpellEffectFn(spell_azurewing_pylon::MissingTrigger, EFFECT_1, SPELL_EFFECT_TRIGGER_SPELL);
        OnEffectLaunchTarget += SpellEffectFn(spell_azurewing_pylon::MissingTrigger, EFFECT_1, SPELL_EFFECT_TRIGGER_SPELL);
    }
};

class spell_azurewing_whelp_pickup : public SpellScript
{
    PrepareSpellScript(spell_azurewing_whelp_pickup);
    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        Unit* target = GetExplTargetUnit();
        Creature* whelp = target ? target->ToCreature() : nullptr;
        return Azurewing::Active(player, 42271) && whelp && whelp->GetEntry() == 91037 &&
            Azurewing::Owner(whelp) == player && !player->HasItemCount(122292, 1) &&
            player->IsWithinDistInMap(whelp, 6.0f) && player->IsWithinLOSInMap(whelp)
            ? SPELL_CAST_OK : SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
    }
    void MissingTrigger(SpellEffIndex index) { PreventHitDefaultEffect(index); }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_azurewing_whelp_pickup::Check);
        OnEffectLaunch += SpellEffectFn(spell_azurewing_whelp_pickup::MissingTrigger, EFFECT_1, SPELL_EFFECT_TRIGGER_SPELL);
        OnEffectLaunchTarget += SpellEffectFn(spell_azurewing_whelp_pickup::MissingTrigger, EFFECT_1, SPELL_EFFECT_TRIGGER_SPELL);
    }
};

// Replace the exact-health polling surrender. DamageTaken precedes the core's
// scaling division, so compare normalized damage with target-relative health.
struct npc_azurewing_runas_duel : public ScriptedAI
{
    npc_azurewing_runas_duel(Creature* creature) : ScriptedAI(creature) { }
    bool surrendered = false;
    void Reset() override
    {
        surrendered = false;
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        me->SetReactState(REACT_AGGRESSIVE);
    }
    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType) override
    {
        if (surrendered) { damage = 0; return; }
        Player* player = attacker ? attacker->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (!Azurewing::Active(player, 37957))
            return;
        double multiplier = me->GetHealthMultiplierForTarget(attacker);
        double finalDamage = multiplier > 0 ? damage / multiplier : damage;
        uint64 health = me->GetHealth(attacker), threshold = me->GetMaxHealth(attacker) / 2;
        if (health > threshold && finalDamage < health - threshold)
            return;
        surrendered = true;
        damage = 0;
        me->CombatStop(true);
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        Talk(8);
        player->KilledMonsterCredit(90372);
        me->DespawnOrUnsummon(3000);
    }
    void UpdateAI(uint32) override { if (!surrendered && UpdateVictim()) DoMeleeAttackIfReady(); }
};

struct npc_azurewing_runas_follower : public ScriptedAI
{
    npc_azurewing_runas_follower(Creature* creature) : ScriptedAI(creature) { me->SetReactState(REACT_PASSIVE); }
};

struct npc_azurewing_runas_guide : public ScriptedAI
{
    npc_azurewing_runas_guide(Creature* creature) : ScriptedAI(creature) { me->SetReactState(REACT_PASSIVE); }
    uint32 point = 0;
    bool moving = false;
    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == POINT_MOTION_TYPE && moving && id == point + 1)
        {
            moving = false;
            ++point;
        }
    }
    void UpdateAI(uint32) override
    {
        using namespace Azurewing;
        Player* player = Owner(me);
        if (!Active(player, 37857) || Done(player, 37857, 90406))
        {
            me->DespawnOrUnsummon();
            return;
        }
        if (!player->IsWithinDistInMap(me, 35.0f))
            return;
        if (point == 10)
        {
            if (player->IsWithinLOSInMap(me))
            {
                player->KilledMonsterCredit(90406);
                me->DespawnOrUnsummon(3000);
            }
        }
        else if (!moving)
        {
            moving = true;
            me->GetMotionMaster()->MovePoint(point + 1, GuidePath[point]);
        }
    }
};

class npc_azurewing_runas_start : public CreatureScript
{
public:
    npc_azurewing_runas_start() : CreatureScript("npc_azurewing_runas_start") { }
    bool OnGossipSelect(Player* player, Creature* creature, uint32, uint32) override
    {
        using namespace Azurewing;
        if (player->PlayerTalkClass->GetGossipMenu().GetMenuId() != 18200 || !Active(player, 37857) ||
            Done(player, 37857, 90406) || !player->IsWithinDistInMap(creature, 6.0f) || !player->IsWithinLOSInMap(creature))
            return true;
        player->PlayerTalkClass->SendCloseGossip();
        Personal(player, 90406, creature->GetPosition());
        return true;
    }
};

struct npc_azurewing_final_enemy : public ScriptedAI
{
    npc_azurewing_final_enemy(Creature* creature) : ScriptedAI(creature) { }
    void UpdateAI(uint32) override
    {
        if (!Azurewing::Active(Azurewing::Owner(me), 42756))
            me->DespawnOrUnsummon();
        else if (UpdateVictim())
            DoMeleeAttackIfReady();
    }
};

// Native waypoints 107995 plus the old script's ground disembark position.
Position const AzurewingReturnPath[] = {
    {1183.65f,6091.44f,134.01f,0}, {1151.88f,6135.79f,140.90f,0},
    {1022.56f,6270.63f,155.98f,0}, {906.82f,6145.79f,182.46f,0},
    {809.45f,6186.56f,160.20f,0}, {658.08f,6184.17f,79.24f,0},
    {611.54f,6334.34f,79.11f,0}, {632.43f,6395.20f,83.65f,0},
    {610.84f,6490.68f,86.90f,0}, {643.79f,6545.20f,83.36f,0},
    {637.61f,6539.50f,75.00f,0.6f}
};
class npc_azurewing_stellagosa_return : public CreatureScript
{
public:
    npc_azurewing_stellagosa_return() : CreatureScript("npc_azurewing_stellagosa_return") { }
    bool OnGossipSelect(Player* player, Creature* creature, uint32, uint32) override
    {
        using namespace Azurewing;
        if (player->PlayerTalkClass->GetGossipMenu().GetMenuId() != 18196 || !CanRide(player) ||
            Done(player, 37862, 107995) || player->GetVehicle() || Owned(player, 107995) ||
            !player->IsWithinDistInMap(creature, 6.0f) || !player->IsWithinLOSInMap(creature))
            return true;
        player->PlayerTalkClass->SendCloseGossip();
        if (Creature* ride = Personal(player, 107995, creature->GetPosition()))
            player->CastSpell(ride, 77901, true);
        return true;
    }
    struct AI : public ScriptedAI
    {
        AI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetCanFly(true);
            me->SetDisableGravity(true);
        }
        uint32 point = 0, boardingTime = 0;
        bool boarded = false, moving = false, finished = false;
        void PassengerBoarded(Unit* passenger, int8, bool apply) override
        {
            if (passenger != Azurewing::Owner(me)) return;
            boarded = apply;
            if (!apply && !finished) me->DespawnOrUnsummon(1000);
        }
        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE && moving && id == point + 1)
            {
                moving = false;
                ++point;
            }
        }
        void UpdateAI(uint32 diff) override
        {
            if (!me->ToTempSummon() || finished) return;
            Player* player = Azurewing::Owner(me);
            if (!Azurewing::CanRide(player))
            {
                me->DespawnOrUnsummon();
                return;
            }
            if (!boarded || player->GetVehicleBase() != me)
            {
                boardingTime += diff;
                if (boardingTime >= 5000) me->DespawnOrUnsummon();
                return;
            }
            if (point == 11)
            {
                finished = true;
                player->ExitVehicle(&AzurewingReturnPath[10]);
                Azurewing::FinishRide(player);
                me->DespawnOrUnsummon(3000);
            }
            else if (!moving)
            {
                moving = true;
                Position const& pos = AzurewingReturnPath[point];
                me->GetMotionMaster()->MovePoint(point + 1, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), false);
            }
        }
    };
    CreatureAI* GetAI(Creature* creature) const override { return new AI(creature); }
};

class player_azurewing_recovery : public PlayerScript
{
    std::mutex timerMutex;
    std::map<ObjectGuid, uint32> timers;
public:
    player_azurewing_recovery() : PlayerScript("player_azurewing_recovery") { }
    void OnLogout(Player* player) override
    {
        for (uint32 entry : {90476u, 90406u, 91155u, 108721u, 107995u}) Azurewing::Clear(player, entry);
        std::lock_guard<std::mutex> lock(timerMutex);
        timers.erase(player->GetGUID());
    }
    void OnMapChanged(Player* player) override { if (player->GetMapId() != 1220) OnLogout(player); }
    void OnUpdate(Player* player, uint32 diff) override
    {
        using namespace Azurewing;
        // Player's area update interval is not guaranteed. Keep the polling
        // local to this player and limit world/database work to once a second.
        {
            std::lock_guard<std::mutex> lock(timerMutex);
            bool relevant = Active(player, 37957) || Active(player, 37857) || Active(player, 42756);
            if (!relevant && !timers.count(player->GetGUID())) return;
            uint32& timer = timers[player->GetGUID()];
            timer += diff;
            if (timer < 1000) return;
            timer = 0;
            if (!relevant) timers.erase(player->GetGUID());
        }
        bool escort = Active(player, 37957) && Done(player, 37957, 90372) && !Done(player, 37957, 90479);
        if (escort)
        {
            if (Creature* runas = Personal(player, 90476, player->GetPosition()))
            {
                if (runas->GetMotionMaster()->GetCurrentMovementGeneratorType() != FOLLOW_MOTION_TYPE)
                    runas->GetMotionMaster()->MoveFollow(player, 3.0f, 3.14f);
                if (player->GetDistance(EscortHome) < 25.0f && player->IsWithinDistInMap(runas, 15.0f))
                    if (Creature* senegos = player->FindNearestCreature(89978, 25.0f))
                        if (player->IsWithinLOSInMap(senegos))
                        {
                            player->KilledMonsterCredit(90479);
                            Clear(player, 90476);
                        }
            }
        }
        else Clear(player, 90476);
        if (!Active(player, 37857)) Clear(player, 90406);
        if (Active(player, 42756) && player->GetDistance(Orbyth) < 80.0f)
        {
            if (!Done(player, 42756, 91155))
            {
                Clear(player, 108721);
                Personal(player, 91155, Orbyth);
            }
            else if (!Done(player, 42756, 108721))
                Personal(player, 108721, Orbyth);
        }
        else
        {
            Clear(player, 91155);
            Clear(player, 108721);
        }
    }
};

void AddSC_azurewing_repose()
{
    RegisterSpellScript(spell_azurewing_pool);
    RegisterSpellScript(spell_azurewing_revive);
    RegisterSpellScript(spell_azurewing_pylon);
    RegisterSpellScript(spell_azurewing_whelp_pickup);
    RegisterCreatureAI(npc_azurewing_runas_duel);
    RegisterCreatureAI(npc_azurewing_runas_follower);
    RegisterCreatureAI(npc_azurewing_runas_guide);
    RegisterCreatureAI(npc_azurewing_final_enemy);
    new npc_azurewing_runas_start();
    new npc_azurewing_stellagosa_return();
    new player_azurewing_recovery();
}
