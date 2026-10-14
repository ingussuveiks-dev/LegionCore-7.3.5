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

namespace StormheimValor
{
constexpr uint32 QuestId = 39591;
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
struct npc_stormheim_valor_actor : SmartAI
{
    explicit npc_stormheim_valor_actor(Creature* creature) : SmartAI(creature) { }
    ObjectGuid owner;
    ObjectGuid accessObject, recording, unpowered;
    std::shared_ptr<uint8> token;
    std::weak_ptr<QuestObjectScope> scope;
    uint32 elapsed = 0, lastEnd = 21000, next = 0;
    bool scoped = false, stopped = false, ready = false;

    ~npc_stormheim_valor_actor()
    {
        RestoreConsole();
        auto found = StormheimValor::consoles.find(owner);
        if (found != StormheimValor::consoles.end() && found->second == me->GetGUID())
            StormheimValor::consoles.erase(found);
    }
    Player* Owner() const { return ObjectAccessor::GetPlayer(*me, owner); }
    void RestoreConsole()
    {
        auto found = StormheimValor::consoles.find(owner);
        if (found != StormheimValor::consoles.end() && found->second != me->GetGUID())
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
        return !stopped && StormheimValor::Active(player) && token &&
            player->getQuestStatus(StormheimValor::QuestId)->ScriptLifetime == token &&
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
        if (player && player->getQuestStatus(StormheimValor::QuestId) &&
            player->getQuestStatus(StormheimValor::QuestId)->ScriptLifetime == token)
            player->CancelQuestObjects(StormheimValor::QuestId);
        me->DespawnOrUnsummon();
    }
    void IsSummonedBy(Unit* summoner) override
    {
        using namespace StormheimValor;
        Player* player = summoner ? summoner->ToPlayer() : nullptr;
        auto parent = summoner && summoner->ToCreature() ?
            dynamic_cast<npc_stormheim_valor_actor*>(summoner->ToCreature()->AI()) : nullptr;
        if (parent && parent->scoped && parent->Valid()) player = parent->Owner();
        // Only the console explicitly starts the private chain. Other summons
        // must inherit its scope; arbitrary player casts cannot grant credit.
        scoped = Scope(me) && Active(player) &&
            ((me->GetEntry() == 96122 && activating.count(player->GetGUID())) ||
                (parent && parent->scoped && me->HasQuestObjectScope()));
        if (!scoped) { SmartAI::IsSummonedBy(summoner); return; }
        owner = player->GetGUID();
        player->AddQuestDelayedEvent(QuestId, 0, [] { });
        token = player->getQuestStatus(QuestId)->ScriptLifetime;
        player->BindQuestObject(QuestId, me);
        scope = player->getQuestStatus(QuestId)->ScriptObjects;
        if (me->GetEntry() == 96078)
        {
            if (parent->me->GetEntry() != 96122 || parent->next != 3 || parent->accessObject.IsEmpty())
            {
                Stop();
                return;
            }
            accessObject = parent->accessObject;
            ready = player->CastSpell(me, 233562, true) == SPELL_CAST_OK &&
                me->CastSpell(me, 190549, true) == SPELL_CAST_OK;
            if (!ready) Stop();
        }
        else if (me->GetEntry() == 96122)
        {
            unpowered = activating.at(owner);
            Talk(0, owner);
            // Public archive lights must not be switched off by another
            // player's run. Copy the installed native helpers into this scope.
            float const positions[][3] = {{3451.76f,2089.14f,278.213f},{3433.22f,2089.33f,278.265f},
                {3433.96f,2107.42f,278.718f},{3451.91f,2107.31f,278.439f},{3442.68f,2098.47f,278.34f}};
            for (auto const& pos : positions)
                if (Creature* light = me->SummonCreature(34527, pos[0], pos[1], pos[2], 0.0f,
                    TEMPSUMMON_TIMED_DESPAWN, 300000)) light->CastSpell(light, 190746, true);
        }
        else SmartAI::IsSummonedBy(summoner);
    }
    void JustDied(Unit* killer) override
    {
        if (!scoped || me->GetEntry() != 96121) { SmartAI::JustDied(killer); return; }
        // Pet kills and player kills both enable the exact owner's console.
        // Native 7.3.5 requires a separate "Accessing Record" interaction.
        if (Valid() && killer && killer->GetCharmerOrOwnerPlayerOrPlayerItself() == Owner())
        {
            Talk(1, owner);
            auto found = StormheimValor::consoles.find(owner);
            Creature* console = found == StormheimValor::consoles.end() ? nullptr :
                ObjectAccessor::GetCreature(*me, found->second);
            auto ai = console ? dynamic_cast<npc_stormheim_valor_actor*>(console->AI()) : nullptr;
            if (!ai || !ai->Valid() || !ai->PowerUp()) Stop();
        }
        else Stop();
    }
    bool PowerUp()
    {
        if (next != 2 || !Valid()) return false;
        GameObject* original = ObjectAccessor::GetGameObject(*me, unpowered);
        if (!original) return false;
        GameObject* access = me->SummonGameObject(243801, original->GetPositionX(), original->GetPositionY(),
            original->GetPositionZ(), original->GetOrientation(),
            0.0f, 0.0f, 0.0f, 1.0f, 300);
        if (!access) return false;
        accessObject = access->GetGUID();
        original->AddToHideList(owner);
        original->DestroyForPlayer(Owner());
        Talk(1, owner);
        next = 3;
        return true;
    }
    void JustSummoned(Creature* creature) override
    {
        if (scoped && me->GetEntry() == 96122 && creature->GetEntry() == 96078)
            recording = creature->GetGUID();
        else if (!scoped) SmartAI::JustSummoned(creature);
    }
    bool Access(GameObject* object)
    {
        if (!Valid() || next != 3 || object->GetGUID() != accessObject || object->IsQuestObjectExpired()) return false;
        recording = ObjectGuid::Empty;
        if (me->CastSpell(static_cast<Unit*>(nullptr), 190557, true) != SPELL_CAST_OK ||
            recording.IsEmpty() || !ObjectAccessor::GetCreature(*me, recording)) return false;
        next = 4;
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
        if (me->GetEntry() == 96122)
        {
            if (next == 0 && elapsed >= 2000)
            {
                if (!me->SummonCreature(90493, 3442.33f, 2082.73f, 277.681f, 1.53392f,
                    TEMPSUMMON_TIMED_DESPAWN, 10000)) { Stop(); return; }
                ++next;
            }
            if (next == 1 && elapsed >= 7000)
            {
                if (!me->SummonCreature(96121, 3442.37f, 2082.66f, 279.403f, 1.51469f,
                    TEMPSUMMON_MANUAL_DESPAWN, 0)) { Stop(); return; }
                ++next;
            }
            return;
        }
        if (me->GetEntry() != 96078) { SmartAI::UpdateAI(diff); return; }
        if (!ready) { Stop(); return; }
        uint32 const times[] = {2000, 9000, 16000};
        uint32 const spells[] = {197661, 197662, 197663};
        if (next < 3 && elapsed >= times[next])
        {
            if (!Record(776 + next, spells[next])) { Stop(); return; }
            ++next;
        }
        if (next == 3 && elapsed >= lastEnd)
        {
            Owner()->KillCreditGO(243801, accessObject);
            Stop();
        }
    }
};

class go_stormheim_valor_console : public GameObjectScript
{
public:
    go_stormheim_valor_console() : GameObjectScript("go_stormheim_valor_console") { }
    static bool Use(Player* player, GameObject* object)
    {
        using namespace StormheimValor;
        if (!Scope(object) || (object->GetEntry() != 243808 && object->GetEntry() != 243801)) return false;
        if (!Active(player) || !object->isSpawned() || object->IsQuestObjectExpired() ||
            object->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) ||
            !player->InSamePhase(object) || !player->canSeeOrDetect(object) ||
            !player->IsWithinDistInMap(object, 5.0f) || !player->IsWithinLOSInMap(object)) return true;
        auto found = consoles.find(player->GetGUID());
        if (object->GetEntry() == 243801)
        {
            Creature* console = found == consoles.end() ? nullptr : ObjectAccessor::GetCreature(*player, found->second);
            auto ai = console ? dynamic_cast<npc_stormheim_valor_actor*>(console->AI()) : nullptr;
            if (ai) ai->Access(object);
            return true;
        }
        if (found != consoles.end())
            if (Creature* previous = ObjectAccessor::GetCreature(*player, found->second))
                if (!previous->IsQuestObjectExpired()) return true;
        player->CancelQuestObjects(QuestId);
        // Native event_scripts 243808 position and five-minute lifetime. Handle
        // both packet paths here, without starting the same map event twice.
        activating[player->GetGUID()] = object->GetGUID();
        if (Creature* console = player->SummonCreature(96122, 3442.71f, 2100.63f, 278.883f, 0.0f,
            TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000))
        {
            if (auto ai = dynamic_cast<npc_stormheim_valor_actor*>(console->AI())) ai->unpowered = object->GetGUID();
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
            if (StormheimValor::Scope(object)) return Use(player, object);
            return SmartGameObjectAI::GossipHello(player, isUse);
        }
        bool GossipUse(Player* player) override
        {
            if (StormheimValor::Scope(object)) return Use(player, object);
            return SmartGameObjectAI::GossipUse(player);
        }
    };
    GameObjectAI* GetAI(GameObject* object) const override { return new ConsoleAI(object); }
};

void AddSC_stormheim_valor()
{
    RegisterCreatureAI(npc_stormheim_valor_actor);
    new go_stormheim_valor_console();
}
