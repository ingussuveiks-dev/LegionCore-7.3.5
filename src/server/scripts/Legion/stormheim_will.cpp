#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Conversation.h"
#include "SpellMgr.h"
#include <algorithm>
#include <map>
#include <memory>
#include <set>

namespace StormheimWill
{
constexpr uint32 QuestId = 39592;
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
}

// Preserve SmartAI for these entries outside this quest's private summon chain.
struct npc_stormheim_will_actor : SmartAI
{
    explicit npc_stormheim_will_actor(Creature* creature) : SmartAI(creature) { }
    ObjectGuid owner;
    ObjectGuid accessObject, recording, unpowered, dragon;
    std::set<ObjectGuid> defenders;
    std::shared_ptr<uint8> token;
    std::weak_ptr<QuestObjectScope> scope;
    uint32 elapsed = 0, phaseElapsed = 0, lastEnd = 18000, next = 0;
    bool scoped = false, stopped = false, ready = false, warmongerDefeated = false;

    ~npc_stormheim_will_actor()
    {
        RestoreConsole();
        auto found = StormheimWill::consoles.find(owner);
        if (found != StormheimWill::consoles.end() && found->second == me->GetGUID())
            StormheimWill::consoles.erase(found);
    }
    Player* Owner() const { return ObjectAccessor::GetPlayer(*me, owner); }
    void RestoreConsole()
    {
        auto found = StormheimWill::consoles.find(owner);
        if (found != StormheimWill::consoles.end() && found->second != me->GetGUID())
        {
            unpowered = ObjectGuid::Empty;
            return;
        }
        if (GameObject* object = ObjectAccessor::GetGameObject(*me, unpowered))
        {
            object->RemoveFromHideList(owner);
            if (Player* player = Owner()) player->UpdateVisibilityForPlayer();
        }
        unpowered = ObjectGuid::Empty;
    }
    bool Valid() const
    {
        Player* player = Owner();
        return !stopped && StormheimWill::Active(player) && token &&
            player->getQuestStatus(StormheimWill::QuestId)->ScriptLifetime == token &&
            !me->IsQuestObjectExpired() && player->InSamePhase(me) && player->IsWithinDistInMap(me, 80.0f);
    }
    void Stop()
    {
        if (stopped) return;
        stopped = true;
        RestoreConsole();
        // Expire the original scope even after logout or a map transfer, when
        // the owner cannot be looked up in this map. Never cancel a new retry.
        if (auto objects = scope.lock()) objects->Cancel();
        Player* player = Owner();
        if (player && player->getQuestStatus(StormheimWill::QuestId) &&
            player->getQuestStatus(StormheimWill::QuestId)->ScriptLifetime == token)
            player->CancelQuestObjects(StormheimWill::QuestId);
        me->DespawnOrUnsummon();
    }
    void IsSummonedBy(Unit* summoner) override
    {
        using namespace StormheimWill;
        Player* player = summoner ? summoner->ToPlayer() : nullptr;
        auto parent = summoner && summoner->ToCreature() ?
            dynamic_cast<npc_stormheim_will_actor*>(summoner->ToCreature()->AI()) : nullptr;
        if (parent && parent->scoped && parent->Valid()) player = parent->Owner();
        // Only the console explicitly starts the private chain. Other summons
        // must inherit its scope; arbitrary player casts cannot grant credit.
        scoped = Scope(me) && Active(player) &&
            ((me->GetEntry() == 96139 && activating.count(player->GetGUID())) ||
                (parent && parent->scoped && me->HasQuestObjectScope()));
        if (!scoped) { SmartAI::IsSummonedBy(summoner); return; }
        owner = player->GetGUID();
        player->AddQuestDelayedEvent(QuestId, 0, [] { });
        token = player->getQuestStatus(QuestId)->ScriptLifetime;
        player->BindQuestObject(QuestId, me);
        scope = player->getQuestStatus(QuestId)->ScriptObjects;
        if (me->GetEntry() == 96144)
        {
            if (parent->me->GetEntry() != 96139 || parent->next != 9 || parent->accessObject.IsEmpty())
            {
                Stop();
                return;
            }
            accessObject = parent->accessObject;
            ready = player->CastSpell(me, 233562, true) == SPELL_CAST_OK &&
                me->CastSpell(me, 190549, true) == SPELL_CAST_OK;
            if (!ready) Stop();
        }
        else if (me->GetEntry() == 96139)
        {
            unpowered = activating.at(owner);
            // Public archive lights must not be switched off by another
            // player's run. Copy the installed native helpers into this scope.
            float const positions[][3] = {{3333.55f,2230.94f,315.78f},{3333.16f,2212.85f,316.066f}};
            for (auto const& pos : positions)
                if (Creature* light = me->SummonCreature(34527, pos[0], pos[1], pos[2], 0.0f,
                    TEMPSUMMON_TIMED_DESPAWN, 300000)) light->CastSpell(light, 190746, true);
        }
        else
        {
            SmartAI::IsSummonedBy(summoner);
            if (me->GetEntry() == 96129 || me->GetEntry() == 108263) SmartAI::AttackStart(player);
        }
    }
    void JustDied(Unit* killer) override
    {
        if (!scoped || (me->GetEntry() != 108263 && me->GetEntry() != 96129)) { SmartAI::JustDied(killer); return; }
        // Pet kills and player kills both enable the exact owner's console.
        // Native 7.3.5 requires a separate "Accessing Record" interaction.
        if (Valid() && killer && killer->GetCharmerOrOwnerPlayerOrPlayerItself() == Owner())
        {
            auto found = StormheimWill::consoles.find(owner);
            Creature* console = found == StormheimWill::consoles.end() ? nullptr :
                ObjectAccessor::GetCreature(*me, found->second);
            auto ai = console ? dynamic_cast<npc_stormheim_will_actor*>(console->AI()) : nullptr;
            if (!ai || !ai->Valid()) { Stop(); return; }
            if (!ai->defenders.erase(me->GetGUID())) return;
            if (me->GetEntry() == 108263) ai->warmongerDefeated = true;
            if (ai->warmongerDefeated && ai->defenders.empty() && !ai->PowerUp()) Stop();
        }
        else Stop();
    }
    bool PowerUp()
    {
        if (next != 8 || !warmongerDefeated || !defenders.empty() || !Valid()) return false;
        GameObject* original = ObjectAccessor::GetGameObject(*me, unpowered);
        if (!original) return false;
        GameObject* access = me->SummonGameObject(243802, original->GetPositionX(), original->GetPositionY(),
            original->GetPositionZ(), original->GetOrientation(),
            0.0f, 0.0f, 0.0f, 1.0f, 300);
        if (!access) return false;
        accessObject = access->GetGUID();
        original->AddToHideList(owner);
        original->DestroyForPlayer(Owner());
        Talk(4, owner);
        next = 9;
        return true;
    }
    void JustSummoned(Creature* creature) override
    {
        if (scoped && me->GetEntry() == 96139)
        {
            if (creature->GetEntry() == 96144) recording = creature->GetGUID();
            if (creature->GetEntry() == 96067) dragon = creature->GetGUID();
            if (creature->GetEntry() == 96129 || creature->GetEntry() == 108263) defenders.insert(creature->GetGUID());
        }
        else if (!scoped) SmartAI::JustSummoned(creature);
    }
    bool Access(GameObject* object)
    {
        if (!Valid() || next != 9 || object->GetGUID() != accessObject || object->IsQuestObjectExpired()) return false;
        recording = ObjectGuid::Empty;
        if (me->CastSpell(static_cast<Unit*>(nullptr), 190707, true) != SPELL_CAST_OK ||
            recording.IsEmpty() || !ObjectAccessor::GetCreature(*me, recording)) return false;
        if (dragon.IsEmpty() || !ObjectAccessor::GetCreature(*me, dragon)) { Stop(); return false; }
        if (Creature* controller = ObjectAccessor::GetCreature(*me, recording))
            if (auto ai = dynamic_cast<npc_stormheim_will_actor*>(controller->AI())) ai->dragon = dragon;
        next = 10;
        return true;
    }
    bool Record(uint32 conversationId, uint32 spellId)
    {
        auto conversation = new Conversation;
        if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),
            conversationId, me, sSpellMgr->GetSpellInfo(spellId), *me))
        {
            delete conversation;
            return false;
        }
        // Creation and the complete server-side lifetime are prerequisites;
        // this is not evidence that a client rendered or heard the recording.
        lastEnd = std::max(lastEnd, elapsed + uint32(conversation->GetDuration()));
        return true;
    }
    void UpdateAI(uint32 diff) override
    {
        if (!scoped) { SmartAI::UpdateAI(diff); return; }
        if (!Valid()) { Stop(); return; }
        elapsed += diff;
        if (elapsed >= 300000) { Stop(); return; }
        if (me->GetEntry() == 96139)
        {
            // Native cumulative 1.5/4/6/36/41/43/88/93-second charge events.
            // Preserve each nonzero pause even after a delayed map update.
            for (ObjectGuid const& guid : defenders)
                if (!ObjectAccessor::GetCreature(*me, guid)) { Stop(); return; }
            uint32 const delays[] = {1500,2500,2000,30000,5000,2000,45000,5000};
            phaseElapsed += diff;
            if (next >= 8 || phaseElapsed < delays[next]) return;
            phaseElapsed = 0;
            if (next == 0) Talk(1, owner);
            else if (next == 1 || next == 4)
            {
                float const positions[][3] = {{3328.56f,2197.37f,312.742f},{3338.89f,2196.61f,313.129f},{3332.81f,2197.72f,312.897f}};
                uint32 count = next == 1 ? 2 : 3;
                for (uint32 i = 0; i < count; ++i)
                    if (!me->SummonCreature(96129, positions[i][0], positions[i][1], positions[i][2],
                        1.54f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 120000)) { Stop(); return; }
            }
            else if (next == 2 || next == 5)
            {
                for (ObjectGuid const& guid : defenders)
                    if (Creature* raider = ObjectAccessor::GetCreature(*me, guid))
                        if (raider->GetEntry() == 96129) raider->AI()->SetData(next == 2 ? 1 : 2, next == 2 ? 1 : 2);
            }
            else if (next == 3) Talk(0, owner);
            else if (next == 6) Talk(2, owner);
            else if (next == 7)
                if (!me->SummonCreature(108263,3332.81f,2197.72f,312.897f,1.54f,
                    TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,120000)) { Stop(); return; }
            ++next;
            return;
        }
        if (me->GetEntry() != 96144) { SmartAI::UpdateAI(diff); return; }
        if (!ready) { Stop(); return; }
        Creature* projection = ObjectAccessor::GetCreature(*me, dragon);
        if (!projection || !projection->IsAlive() || projection->IsQuestObjectExpired() ||
            !me->InSamePhase(projection)) { Stop(); return; }
        uint32 const times[] = {2000, 7000, 13000};
        uint32 const spells[] = {197658, 197659, 197660};
        if (next < 3 && elapsed >= times[next])
        {
            if (next == 1) projection->AI()->SetData(1,1);
            if (next == 2) projection->AI()->SetData(2,2);
            if (!Record(773 + next, spells[next])) { Stop(); return; }
            ++next;
        }
        if (next == 3 && elapsed >= lastEnd)
        {
            Owner()->KillCreditGO(243802, accessObject);
            Stop();
        }
    }
};

class go_stormheim_will_console : public GameObjectScript
{
public:
    go_stormheim_will_console() : GameObjectScript("go_stormheim_will_console") { }
    static bool Use(Player* player, GameObject* object)
    {
        using namespace StormheimWill;
        if (!Scope(object) || (object->GetEntry() != 243814 && object->GetEntry() != 243802)) return false;
        if (!Active(player) || !object->isSpawned() || object->IsQuestObjectExpired() ||
            object->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) ||
            !player->InSamePhase(object) || !player->canSeeOrDetect(object) ||
            !player->IsWithinDistInMap(object, 5.0f) || !player->IsWithinLOSInMap(object)) return true;
        auto found = consoles.find(player->GetGUID());
        if (object->GetEntry() == 243802)
        {
            Creature* console = found == consoles.end() ? nullptr : ObjectAccessor::GetCreature(*player, found->second);
            auto ai = console ? dynamic_cast<npc_stormheim_will_actor*>(console->AI()) : nullptr;
            if (ai) ai->Access(object);
            return true;
        }
        if (found != consoles.end())
            if (Creature* previous = ObjectAccessor::GetCreature(*player, found->second))
                if (!previous->IsQuestObjectExpired()) return true;
        player->CancelQuestObjects(QuestId);
        // Native event_scripts 243814 position and five-minute lifetime. Handle
        // both packet paths here, without starting the same map event twice.
        activating[player->GetGUID()] = object->GetGUID();
        if (Creature* console = player->SummonCreature(96139, 3333.15f, 2209.16f, 315.999f, 0.018657f,
            TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000))
        {
            if (auto ai = dynamic_cast<npc_stormheim_will_actor*>(console->AI())) ai->unpowered = object->GetGUID();
            consoles[player->GetGUID()] = console->GetGUID();
        }
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
            if (StormheimWill::Scope(object)) return Use(player, object);
            return SmartGameObjectAI::GossipHello(player, isUse);
        }
        bool GossipUse(Player* player) override
        {
            if (StormheimWill::Scope(object)) return Use(player, object);
            return SmartGameObjectAI::GossipUse(player);
        }
    };
    GameObjectAI* GetAI(GameObject* object) const override { return new ConsoleAI(object); }
};

void AddSC_stormheim_will()
{
    RegisterCreatureAI(npc_stormheim_will_actor);
    new go_stormheim_will_console();
}
