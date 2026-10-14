#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "Map.h"
#include <cmath>
#include <map>
#include <memory>

namespace StormheimMight
{
constexpr uint32 QuestId = 39594;
std::map<ObjectGuid, ObjectGuid> consoles;
std::map<ObjectGuid, ObjectGuid> activating;
bool Scope(WorldObject* object)
{
    return object && object->IsInWorld() && object->GetMapId() == 1220 && object->GetZoneId() == 7541;
}
bool Active(Player* player)
{
    return Scope(player) && player->IsAlive() && !player->IsBeingTeleported() &&
        player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE;
}
void Credit(Player* player, uint32 entry)
{
    if (!Active(player) || player->GetQuestObjectiveData(QuestId, entry)) return;
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
}

// Unrelated uses of these entries retain SmartAI. Only a validated archive
// click can arm this private chain; an arbitrary summon cannot grant credit.
struct npc_stormheim_might_actor : SmartAI
{
    explicit npc_stormheim_might_actor(Creature* creature) : SmartAI(creature) { }
    ObjectGuid owner, source, projector, opponent;
    std::shared_ptr<uint8> token;
    std::weak_ptr<QuestObjectScope> scope;
    uint32 stage = 0, timer = 1500, age = 0;
    float floor = 0.0f;
    bool scoped = false, stopped = false;
    ~npc_stormheim_might_actor()
    {
        auto found = StormheimMight::consoles.find(owner);
        if (found != StormheimMight::consoles.end() && found->second == me->GetGUID())
            StormheimMight::consoles.erase(found);
    }
    Player* Owner() const { return ObjectAccessor::GetPlayer(*me, owner); }
    bool Valid() const
    {
        Player* player = Owner();
        return !stopped && StormheimMight::Active(player) && token &&
            player->getQuestStatus(StormheimMight::QuestId)->ScriptLifetime == token &&
            !me->IsQuestObjectExpired() && player->InSamePhase(me) && player->IsWithinDistInMap(me, 80.0f);
    }
    npc_stormheim_might_actor* Console() const
    {
        Creature* console = ObjectAccessor::GetCreature(*me, source);
        auto ai = console ? dynamic_cast<npc_stormheim_might_actor*>(console->AI()) : nullptr;
        if (!ai || !ai->scoped || !ai->Valid() || ai->stage != 3 || ai->opponent != me->GetGUID()) return nullptr;
        Creature* helper = ObjectAccessor::GetCreature(*me, ai->projector);
        return helper && helper->IsAlive() && !helper->IsQuestObjectExpired() ? ai : nullptr;
    }
    void Stop()
    {
        if (stopped) return;
        stopped = true;
        // Cancel the original acceptance even when the owner has left this
        // map. A delayed cleanup must never cancel a newly accepted quest.
        if (auto objects = scope.lock()) objects->Cancel();
        Player* player = Owner();
        if (player && player->getQuestStatus(StormheimMight::QuestId) &&
            player->getQuestStatus(StormheimMight::QuestId)->ScriptLifetime == token)
            player->CancelQuestObjects(StormheimMight::QuestId);
        me->DespawnOrUnsummon();
    }
    void IsSummonedBy(Unit* summoner) override
    {
        using namespace StormheimMight;
        Player* player = summoner ? summoner->ToPlayer() : nullptr;
        auto parent = summoner && summoner->ToCreature() ?
            dynamic_cast<npc_stormheim_might_actor*>(summoner->ToCreature()->AI()) : nullptr;
        if (parent && parent->scoped && parent->Valid()) player = parent->Owner();
        scoped = Scope(me) && Active(player) &&
            ((me->GetEntry() == 96176 && activating.count(player->GetGUID())) ||
                (parent && parent->scoped && parent->me->GetEntry() == 96176 && me->HasQuestObjectScope() &&
                    ((me->GetEntry() == 35845 && parent->stage == 1) ||
                        (me->GetEntry() == 96175 && parent->stage == 2))));
        if (!scoped) { SmartAI::IsSummonedBy(summoner); return; }
        owner = player->GetGUID();
        player->AddQuestDelayedEvent(QuestId, 0, [] { });
        token = player->getQuestStatus(QuestId)->ScriptLifetime;
        player->BindQuestObject(QuestId, me);
        scope = player->getQuestStatus(QuestId)->ScriptObjects;
        if (parent) source = parent->me->GetGUID();
        if (me->GetEntry() == 96175)
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
            Talk(3, owner);
            timer = 8000;
        }
    }
    bool CanAIAttack(Unit const* target) const override
    {
        if (!scoped) return SmartAI::CanAIAttack(target);
        return me->GetEntry() == 96175 && me->IsAlive() && stage == 2 && Valid() && Console() && target &&
            target->GetCharmerOrOwnerPlayerOrPlayerItself() == Owner();
    }
    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType type) override
    {
        if (!scoped) { SmartAI::DamageTaken(attacker, damage, type); return; }
        if (!CanAIAttack(attacker)) { damage = 0; return; }
        // AI runs before DealDamage divides by the target's health multiplier.
        // Compare final damage and health in the same target-relative space.
        float multiplier = me->GetHealthMultiplierForTarget(attacker);
        if (!std::isfinite(multiplier) || multiplier <= 0.0f) { damage = 0; return; }
        double finalDamage = std::floor(double(float(damage) / multiplier));
        uint64 health = me->GetHealth(attacker), quarter = me->GetMaxHealth(attacker) / 4;
        if (health > quarter && finalDamage < double(health - quarter)) return;
        damage = 0;
        me->CombatStop(true);
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        if (me->CastSpell(me, 68442, true) != SPELL_CAST_OK) { Stop(); return; }
        stage = 3;
        Talk(4, owner);
        timer = 5000;
    }
    void JustDied(Unit* killer) override
    {
        if (!scoped) { SmartAI::JustDied(killer); return; }
        Stop(); // A death or despawn is never proof of surrender.
    }
    void EnterEvadeMode() override
    {
        if (!scoped) { SmartAI::EnterEvadeMode(); return; }
        Stop();
    }
    void UpdateAI(uint32 diff) override
    {
        if (!scoped) { SmartAI::UpdateAI(diff); return; }
        if (!Valid()) { Stop(); return; }
        age += diff;
        if (age >= (me->GetEntry() == 96175 ? 180000u : 300000u)) { Stop(); return; }
        if (me->GetEntry() == 35845) return;
        if (me->GetEntry() == 96176 && stage == 3)
        {
            Creature* boss = ObjectAccessor::GetCreature(*me, opponent);
            Creature* helper = ObjectAccessor::GetCreature(*me, projector);
            if (!boss || !boss->IsAlive() || !helper || !helper->IsAlive()) Stop();
            return;
        }
        if (me->GetEntry() == 96175)
        {
            if (!Console()) { Stop(); return; }
            if (stage == 2) { if (UpdateVictim()) DoMeleeAttackIfReady(); return; }
        }
        if (timer > diff) { timer -= diff; return; }
        // One transition per update preserves each dialogue/power-up pause
        // even after a delayed server tick.
        if (me->GetEntry() == 96176)
        {
            if (stage == 0) { Talk(3, owner); stage = 1; timer = 1500; }
            else if (stage == 1)
            {
                // The installed summon anchor is 2.55m above this vault's
                // VMAP floor. Use the map's indoor-aware height calculation;
                // the outdoor terrain is another 54m higher.
                floor = me->GetMap()->GetHeight(3502.97f, 2164.51f, 231.977f);
                if (!std::isfinite(floor) || std::abs(floor - 231.977f) > 5.0f) { Stop(); return; }
                // Anchor the private projection at the installed Yotnar
                // summon position rather than selecting an arbitrary GUID 0.
                Creature* helper = me->SummonCreature(35845, 3502.97f, 2164.51f, floor, 6.23052f,
                    TEMPSUMMON_MANUAL_DESPAWN, 0);
                if (!helper || helper->CastSpell(helper, 190763, true) != SPELL_CAST_OK) { Stop(); return; }
                projector = helper->GetGUID();
                stage = 2; timer = 3000;
            }
            else if (stage == 2)
            {
                Creature* helper = ObjectAccessor::GetCreature(*me, projector);
                if (!helper || !helper->IsAlive() || helper->CastSpell(helper, 190773, true) != SPELL_CAST_OK)
                { Stop(); return; }
                Creature* boss = me->SummonCreature(96175, 3502.97f, 2164.51f, floor, 6.23052f,
                    TEMPSUMMON_MANUAL_DESPAWN, 0);
                if (!boss) { Stop(); return; }
                opponent = boss->GetGUID(); stage = 3;
            }
        }
        else if (me->GetEntry() == 96175)
        {
            if (stage == 0) { Talk(0, owner); stage = 1; timer = 8000; }
            else if (stage == 1)
            {
                Talk(1, owner);
                StormheimMight::Credit(Owner(), 96290);
                stage = 2;
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                me->SetReactState(REACT_AGGRESSIVE);
                AttackStart(Owner());
            }
            else if (stage == 3)
            {
                Talk(2, owner);
                StormheimMight::Credit(Owner(), 96175);
                Stop();
            }
        }
    }
};

class go_stormheim_might_console : public GameObjectScript
{
public:
    go_stormheim_might_console() : GameObjectScript("go_stormheim_might_console") { }
    static bool Use(Player* player, GameObject* object)
    {
        using namespace StormheimMight;
        if (!Scope(object) || object->GetEntry() != 243817) return false;
        if (!Active(player) || !object->isSpawned() || object->IsQuestObjectExpired() ||
            object->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) ||
            !player->InSamePhase(object) || !player->canSeeOrDetect(object) ||
            !player->IsWithinDistInMap(object, 5.0f) || !player->IsWithinLOSInMap(object)) return true;
        auto found = consoles.find(player->GetGUID());
        if (found != consoles.end())
            if (Creature* previous = ObjectAccessor::GetCreature(*player, found->second))
                if (auto ai = dynamic_cast<npc_stormheim_might_actor*>(previous->AI()))
                {
                    if (ai->Valid()) return true;
                    ai->Stop();
                }
        player->CancelQuestObjects(QuestId);
        activating[player->GetGUID()] = object->GetGUID();
        if (Creature* console = player->SummonCreature(96176, 3546.24f, 2163.73f, 234.228f, 0.0f,
            TEMPSUMMON_MANUAL_DESPAWN, 0)) consoles[player->GetGUID()] = console->GetGUID();
        activating.erase(player->GetGUID());
        return true;
    }
    bool OnGossipHello(Player* player, GameObject* object) override { return Use(player, object); }
    struct ConsoleAI : SmartGameObjectAI
    {
        explicit ConsoleAI(GameObject* object) : SmartGameObjectAI(object), object(object) { }
        GameObject* object;
        bool GossipHello(Player* player, bool isUse) override
        {
            if (StormheimMight::Scope(object)) return Use(player, object);
            return SmartGameObjectAI::GossipHello(player, isUse);
        }
        bool GossipUse(Player* player) override
        {
            if (StormheimMight::Scope(object)) return Use(player, object);
            return SmartGameObjectAI::GossipUse(player);
        }
    };
    GameObjectAI* GetAI(GameObject* object) const override { return new ConsoleAI(object); }
};

void AddSC_stormheim_might()
{
    RegisterCreatureAI(npc_stormheim_might_actor);
    new go_stormheim_might_console();
}
