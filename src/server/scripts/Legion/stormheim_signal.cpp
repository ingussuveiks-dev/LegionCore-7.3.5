#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "TemporarySummon.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>

namespace StormheimSignal
{
constexpr uint32 QuestId = 38060, Controller = 92475, Boss = 90907;
constexpr int32 OwnerKey = 1, ArrivalKey = 2, SurrenderKey = 3;
constexpr uint32 AbortKey = 4;
std::map<ObjectGuid, ObjectGuid> encounters;

bool Scope(WorldObject* object)
{
    return object && object->IsInWorld() && object->GetMapId() == 1220 && object->GetZoneId() == 7541;
}
bool Active(Player* player)
{
    return Scope(player) && player->IsAlive() && !player->IsBeingTeleported() &&
        player->GetTeam() == ALLIANCE && player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE;
}
bool Objective(Player* player, uint32 entry)
{
    return player->GetQuestObjectiveData(QuestId, entry) > 0;
}
void Credit(Player* player, uint32 entry)
{
    if (!Active(player) || Objective(player, entry)) return;
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.ObjectID == entry)
            {
                player->SetQuestObjectiveData(quest, &objective, 1);
                player->SendQuestUpdateAddCredit(quest, ObjectGuid::Empty, objective, 1);
                if (player->CanCompleteQuest(QuestId)) player->CompleteQuest(QuestId);
                return;
            }
}
// These are the installed 92547 flight waypoints, including the arrival
// checkpoint 16. Arrival comes from movement completion, never a timer.
Position const Flight[] = {
    {3208.01f,3127.68f,529.622f,0}, {3216.55f,3138.85f,531.204f,0},
    {3225.49f,3152.43f,535.045f,0}, {3231.85f,3163.41f,535.002f,0},
    {3241.34f,3171.01f,535.15f,0}, {3272.01f,3171.2f,534.556f,0},
    {3278.25f,3185.21f,534.791f,0}, {3270.34f,3203.01f,536.743f,0},
    {3251.43f,3206.99f,537.899f,0}, {3232.05f,3196.35f,537.163f,0},
    {3235.63f,3178.33f,535.599f,0}, {3246.99f,3166.32f,535.918f,0},
    {3267.35f,3160.98f,537.528f,0}, {3277.58f,3144.76f,537.632f,0},
    {3290.77f,3124.95f,540.134f,0}, {3300.34f,3117.35f,540.789f,0},
    {3322.29f,3104.57f,541.355f,0}
};
Position const Waves[] = {
    {3183.78f,3169.5f,543.87f,1}, {3192.96f,3129.88f,543.95f,1},
    {3214.62f,3119.56f,544.54f,1}, {3277.6f,3122.89f,548.05f,1}
};
}

struct npc_stormheim_signal_controller : SmartAI
{
    explicit npc_stormheim_signal_controller(Creature* creature) : SmartAI(creature), summons(me) { }
    SummonList summons;
    ObjectGuid owner, carrier, boss, rogers, overspark;
    std::weak_ptr<uint8> lifetime;
    uint32 stage = 0, timer = 3000, age = 0;
    bool stopped = false;
    ObjectGuid GetGUID(int32 id) override { return id == StormheimSignal::OwnerKey ? owner : SmartAI::GetGUID(id); }
    Player* PlayerOwner()
    {
        using namespace StormheimSignal;
        Player* player = ObjectAccessor::GetPlayer(*me, owner);
        auto token = lifetime.lock();
        if (!Scope(player) || !token || !player->getQuestStatus(QuestId) || player->getQuestStatus(QuestId)->ScriptLifetime != token ||
            player->GetTeam() != ALLIANCE || !player->IsAlive() || player->IsBeingTeleported() ||
            !player->InSamePhase(me) || player->GetExactDist(me) > 100.0f) return nullptr;
        QuestStatus status = player->GetQuestStatus(QuestId);
        return status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE ? player : nullptr;
    }
    void Stop()
    {
        if (stopped) return;
        stopped = true;
        auto found = StormheimSignal::encounters.find(owner);
        if (found != StormheimSignal::encounters.end() && found->second == me->GetGUID())
            StormheimSignal::encounters.erase(found);
        summons.DespawnAll();
        me->DespawnOrUnsummon();
    }
    void IsSummonedBy(Unit* summoner) override
    {
        using namespace StormheimSignal;
        Player* player = summoner ? summoner->ToPlayer() : nullptr;
        if (!Scope(me) || !Active(player)) { SmartAI::IsSummonedBy(summoner); return; }
        owner = player->GetGUID();
        // AddQuestDelayedEvent initializes the acceptance token, also used to
        // reject abandonment followed by immediate reacceptance between ticks.
        player->AddQuestDelayedEvent(QuestId, 0, [] { });
        lifetime = player->getQuestStatus(QuestId)->ScriptLifetime;
        encounters[owner] = me->GetGUID();
        me->AddPlayerInPersonnalVisibilityList(owner);
        me->SetReactState(REACT_PASSIVE);
        me->setActive(true);
        me->CastSpell(me, 183439, true);
        Credit(player, Controller);
    }
    void JustSummoned(Creature* summon) override
    {
        if (owner.IsEmpty()) { SmartAI::JustSummoned(summon); return; }
        summons.Summon(summon);
        summon->SetPhaseMask(me->GetPhaseMask(), false);
        summon->SetPhaseId(me->GetPhases(), true);
        summon->AddPlayerInPersonnalVisibilityList(owner);
    }
    Creature* Spawn(uint32 entry, Position const& position, uint32 duration = 600000)
    {
        return me->SummonCreature(entry, position, TEMPSUMMON_TIMED_DESPAWN, duration);
    }
    void SetData(uint32 id, uint32 value) override
    {
        if (!owner.IsEmpty() && id == StormheimSignal::AbortKey && value) Stop();
        else SmartAI::SetData(id, value);
    }
    void SetGUID(ObjectGuid const& guid, int32 id) override
    {
        using namespace StormheimSignal;
        if (owner.IsEmpty()) { SmartAI::SetGUID(guid, id); return; }
        Player* player = PlayerOwner();
        if (stopped || !Active(player)) return;
        if (id == ArrivalKey && stage == 5 && guid == carrier)
        {
            Creature* first = Spawn(92557, {3246.89f,3170.68f,526.299f,1.34892f});
            Creature* second = Spawn(92556, {3250.89f,3170.68f,526.299f,1.34892f});
            if (!first || !second) { Stop(); return; }
            rogers = first->GetGUID(); overspark = second->GetGUID();
            first->AI()->Talk(0, owner);
            stage = 6; timer = 4000;
        }
        else if (id == SurrenderKey && stage == 8 && guid == boss && Objective(player, Controller))
        {
            stage = 9;
            Credit(player, Boss);
            // Keep the personal quest giver available for the actual hand-in.
            if (Creature* npc = ObjectAccessor::GetCreature(*me, overspark)) npc->AI()->Talk(1, owner);
        }
    }
    void UpdateAI(uint32 diff) override
    {
        using namespace StormheimSignal;
        if (owner.IsEmpty()) { SmartAI::UpdateAI(diff); return; }
        if (stopped) return;
        age += diff;
        Player* player = PlayerOwner();
        if (!player || age > 600000 || (stage < 9 && !Active(player))) { Stop(); return; }
        if (stage == 5)
        {
            if (timer <= diff || !ObjectAccessor::GetCreature(*me, carrier)) Stop();
            else timer -= diff;
            return;
        }
        if (stage == 8)
        {
            if (!ObjectAccessor::GetCreature(*me, boss)) Stop();
            return;
        }
        if (stage == 9) return;
        if (timer > diff) { timer -= diff; return; }
        if (stage == 0) { Talk(3, owner); stage = 1; timer = 3000; }
        else if (stage <= 4)
        {
            if (!Spawn(90899, Waves[stage-1], 120000)) { Stop(); return; }
            ++stage;
            timer = stage == 5 ? 60000 : 5000;
            // Stage 5 first waits for help, then waits for its real arrival.
            if (stage == 5) stage = 10;
        }
        else if (stage == 10)
        {
            Creature* bird = Spawn(92547, {3193.02f,3117.96f,513.7f,0}, 180000);
            if (!bird) { Stop(); return; }
            carrier = bird->GetGUID(); stage = 5; timer = 180000;
        }
        else if (stage == 6)
        {
            Creature* npc = ObjectAccessor::GetCreature(*me, overspark);
            if (!npc) { Stop(); return; }
            npc->AI()->Talk(0, owner); stage = 7; timer = 4000;
        }
        else if (stage == 7)
        {
            Creature* npc = ObjectAccessor::GetCreature(*me, rogers);
            if (!npc) { Stop(); return; }
            Creature* enemy = Spawn(Boss, {3263.32f,3223.42f,530.064f,4.4f});
            if (!enemy) { Stop(); return; }
            boss = enemy->GetGUID(); npc->AI()->Talk(1, owner); stage = 8;
        }
    }
};

struct npc_stormheim_signal_actor : SmartAI
{
    explicit npc_stormheim_signal_actor(Creature* creature) : SmartAI(creature) { }
    ObjectGuid controller, owner;
    uint32 point = 0, attackTimer = 3000, stormTimer = 10000;
    bool surrendered = false;
    Creature* Controller()
    {
        return ObjectAccessor::GetCreature(*me, controller);
    }
    void IsSummonedBy(Unit* summoner) override
    {
        using namespace StormheimSignal;
        Creature* source = summoner ? summoner->ToCreature() : nullptr;
        if (!Scope(me) || !source || source->GetEntry() != StormheimSignal::Controller ||
            source->AI()->GetGUID(OwnerKey).IsEmpty()) { SmartAI::IsSummonedBy(summoner); return; }
        controller = source->GetGUID(); owner = source->AI()->GetGUID(OwnerKey);
        me->setActive(true);
        me->SetWalk(false);
        if (me->GetEntry() == 92547)
        {
            me->SetReactState(REACT_PASSIVE); me->SetCanFly(true); me->SetDisableGravity(true);
            point = 1; me->GetMotionMaster()->MovePoint(point, Flight[0], false);
        }
        else if (me->GetEntry() == Boss || me->GetEntry() == 90899)
        {
            me->SetReactState(REACT_PASSIVE); me->SetCanFly(true); me->SetDisableGravity(true);
            point = 1;
            me->GetMotionMaster()->MovePoint(point, 3248.89f,3170.68f,526.299f, false);
        }
        else me->SetReactState(REACT_PASSIVE);
    }
    void MovementInform(uint32 type, uint32 id) override
    {
        using namespace StormheimSignal;
        if (controller.IsEmpty()) { SmartAI::MovementInform(type, id); return; }
        if (type != POINT_MOTION_TYPE || id != point) return;
        if (me->GetEntry() == 92547)
        {
            if (point == 16)
                if (Creature* source = Controller()) source->AI()->SetGUID(me->GetGUID(), ArrivalKey);
            if (point < 17) { ++point; me->GetMotionMaster()->MovePoint(point, Flight[point-1], false); }
            else me->DespawnOrUnsummon();
        }
        else if (surrendered) me->DespawnOrUnsummon();
        else if (me->GetEntry() == Boss || me->GetEntry() == 90899)
        {
            me->SetCanFly(false); me->SetDisableGravity(false); me->SetReactState(REACT_AGGRESSIVE);
            if (Player* player = ObjectAccessor::GetPlayer(*me, owner)) AttackStart(player);
        }
    }
    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType type) override
    {
        using namespace StormheimSignal;
        if (controller.IsEmpty()) { SmartAI::DamageTaken(attacker, damage, type); return; }
        Player* player = attacker ? attacker->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (!Active(player) || player->GetGUID() != owner || !Controller()) { damage = 0; return; }
        if (me->GetEntry() != Boss) return;
        if (surrendered) { damage = 0; return; }
        // Creature AI runs BEFORE DealDamage's scaling division. Compare the
        // same final normalized damage with target-relative health, including
        // a pet's strike and an otherwise lethal first hit.
        float multiplier = me->GetHealthMultiplierForTarget(attacker);
        if (!std::isfinite(multiplier) || multiplier <= 0.0f) { damage = 0; return; }
        double finalDamage = std::floor(double(float(damage) / multiplier));
        uint64 health = me->GetHealth(attacker), maximum = me->GetMaxHealth(attacker);
        if (health > maximum / 2 && finalDamage < double(health - maximum / 2)) return;
        damage = 0; surrendered = true;
        me->CombatStop(true); me->SetReactState(REACT_PASSIVE); me->setFaction(35);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        if (Creature* source = Controller()) source->AI()->SetGUID(me->GetGUID(), SurrenderKey);
        me->SetCanFly(true); me->SetDisableGravity(true); point = 2;
        me->GetMotionMaster()->MovePoint(point,3178.4f,3119.54f,542.682f,false);
        me->DespawnOrUnsummon(15000);
    }
    bool CanAIAttack(Unit const* target) const override
    {
        if (controller.IsEmpty()) return SmartAI::CanAIAttack(target);
        Player* player = target ? target->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        return !surrendered && StormheimSignal::Active(player) && player->GetGUID() == owner &&
            (me->GetEntry() == StormheimSignal::Boss || me->GetEntry() == 90899);
    }
    void EnterEvadeMode() override
    {
        if (controller.IsEmpty()) { SmartAI::EnterEvadeMode(); return; }
        if (!surrendered && me->GetEntry() == StormheimSignal::Boss)
            if (Creature* source = Controller()) source->AI()->SetData(StormheimSignal::AbortKey,1);
    }
    void UpdateAI(uint32 diff) override
    {
        if (controller.IsEmpty()) { SmartAI::UpdateAI(diff); return; }
        if (surrendered || !UpdateVictim()) return;
        if (attackTimer <= diff)
        {
            DoCastVictim(me->GetEntry() == StormheimSignal::Boss ? 183454 : 183005);
            attackTimer = 6000;
        }
        else attackTimer -= diff;
        if (me->GetEntry() == StormheimSignal::Boss)
        {
            if (stormTimer <= diff) { DoCastVictim(220129); stormTimer = 15000; }
            else stormTimer -= diff;
        }
        DoMeleeAttackIfReady();
    }
};

class go_stormheim_signal : public GameObjectScript
{
public:
    go_stormheim_signal() : GameObjectScript("go_stormheim_signal") { }
    bool OnGossipHello(Player* player, GameObject* object) override
    {
        using namespace StormheimSignal;
        if (!Scope(object)) return false;
        if (!Active(player) || object->GetEntry() != 240235 || !object->isSpawned() ||
            object->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) ||
            player->GetVehicle() || player->GetTransport() || player->isInCombat() ||
            player->GetExactDist(object) > 6.0f || !player->InSamePhase(object) ||
            !player->canSeeOrDetect(object) || !player->IsWithinLOSInMap(object) ||
            !player->HasAura(180860) || Objective(player,Boss)) return true;
        auto found = encounters.find(player->GetGUID());
        if (found != encounters.end())
            if (Creature* current = ObjectAccessor::GetCreature(*player,found->second))
            {
                auto ai = dynamic_cast<npc_stormheim_signal_controller*>(current->AI());
                if (!ai || ai->PlayerOwner()) return true;
                ai->Stop();
            }
        // The native GO event credits first, even if its summon fails. Our
        // controller awards start credit only after successful initialization.
        if (Creature* summon = player->SummonCreature(Controller,{3248.89f,3170.68f,526.299f,1.34892f},TEMPSUMMON_TIMED_DESPAWN,660000))
            if (summon->AI()->GetGUID(OwnerKey) == player->GetGUID())
                object->SendCustomAnim(object->GetGoAnimProgress());
        return true;
    }
};

class player_stormheim_signal : public PlayerScript
{
public:
    player_stormheim_signal() : PlayerScript("player_stormheim_signal") { }
    void OnLogin(Player* player) override { Sync(player); }
    void OnUpdate(Player* player, uint32) override { Sync(player); }
    void Sync(Player* player)
    {
        using namespace StormheimSignal;
        QuestStatus status = player->GetQuestStatus(QuestId);
        if (Scope(player) && player->GetTeam() == ALLIANCE && player->IsAlive() && !player->IsBeingTeleported() &&
            (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE) &&
            player->GetQuestStatus(38058) == QUEST_STATUS_REWARDED && !player->HasAura(180860))
            player->CastSpell(player,180860,true);
    }
};

void AddSC_stormheim_signal()
{
    RegisterCreatureAI(npc_stormheim_signal_controller);
    RegisterCreatureAI(npc_stormheim_signal_actor);
    new go_stormheim_signal();
    new player_stormheim_signal();
}
