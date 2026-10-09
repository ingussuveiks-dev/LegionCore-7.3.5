#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "TemporarySummon.h"
#include "GameObject.h"
#include <map>
#include <mutex>

namespace AzsunaScythe
{
constexpr uint32 QuestId = 37660;
constexpr uint32 Allari = 90401;
constexpr uint32 SoulScreen = 179183;
// Existing Allari waypoint 3 and the existing exit-gem location.
Position const Meeting = {-154.62f, 6906.46f, 13.2f, 0.0f};
Position const InnerSoul = {-165.6f, 6902.02f, 12.8942f, 0.414018f};

bool Active(Player* player)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 &&
           player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE &&
           player->GetDistance(Meeting) < 110.0f;
}

bool Done(Player* player, uint32 object)
{
    return player->GetQuestObjectiveData(QuestId, object) != 0;
}

Creature* OwnedSummon(Player* player, uint32 entry)
{
    for (ObjectGuid const& guid : *player->GetSummonList(entry))
        if (Creature* creature = ObjectAccessor::GetCreature(*player, guid))
            if (creature->IsAlive() && creature->ToTempSummon() &&
                creature->ToTempSummon()->GetSummonerGUID() == player->GetGUID())
                return creature;
    return nullptr;
}

void Cleanup(Player* player)
{
    player->RemoveAurasDueToSpell(SoulScreen);
    for (uint32 entry : {Allari, 90402u, 89276u, 89673u})
    {
        GuidList summons = *player->GetSummonList(entry);
        for (ObjectGuid const& guid : summons)
            if (Creature* creature = ObjectAccessor::GetCreature(*player, guid))
                creature->DespawnOrUnsummon();
    }
}

bool CanRelease(Player* player, uint32 gem)
{
    if (!Active(player) || !Done(player, 90403) || player->HasAura(SoulScreen))
        return false;
    if (gem == 239338)
        return !Done(player, 90402);
    if (gem == 239332)
        return Done(player, 90402) && !Done(player, 89276);
    return false;
}
} // namespace AzsunaScythe

// Personal guide. Progress survives interruption; an unfinished release can be
// retried. Only a successful compel hit followed by the live dialogue earns credit.
class npc_azsuna_allari_q37660 : public CreatureScript
{
public:
    npc_azsuna_allari_q37660() : CreatureScript("npc_azsuna_allari_q37660") {}

    bool OnGossipSelect(Player* player, Creature* creature, uint32, uint32 action) override
    {
        using namespace AzsunaScythe;
        if (action != 0 || !Active(player) ||
            OwnedSummon(player, Allari) != creature || !player->IsWithinDistInMap(creature, 5.0f) ||
            !Done(player, Allari) || Done(player, 90403))
            return false;
        player->PlayerTalkClass->SendCloseGossip();
        player->KilledMonsterCredit(90403);
        creature->AI()->Talk(1, player->GetGUID());
        return true;
    }

    struct AI : public ScriptedAI
    {
        AI(Creature* creature) : ScriptedAI(creature) {}
        ObjectGuid owner, soul;
        uint32 credit = 0, timer = 0;
        bool hit = false;

        Player* Owner() { return ObjectAccessor::GetPlayer(*me, owner); }

        void IsSummonedBy(Unit* who) override
        {
            using namespace AzsunaScythe;
            if (!who || !Active(who->ToPlayer()))
            {
                me->DespawnOrUnsummon();
                return;
            }
            owner = who->GetGUID();
            me->AddPlayerInPersonnalVisibilityList(owner);
            me->SetReactState(REACT_PASSIVE);
            me->GetMotionMaster()->MovePoint(1, Meeting);
        }

        void SetGUID(ObjectGuid const& guid, int32 gem) override
        {
            using namespace AzsunaScythe;
            Player* player = Owner();
            GameObject* object = ObjectAccessor::GetGameObject(*me, guid);
            if (timer || !CanRelease(player, gem) || !object || object->GetEntry() != uint32(gem) ||
                !player->IsWithinDistInMap(object, 5.0f) || !me->IsWithinDistInMap(object, 40.0f))
                return;
            uint32 entry = gem == 239338 ? 90402 : 89276;
            Creature* demon = player->SummonCreature(entry, object->GetPosition(),
                TEMPSUMMON_TIMED_DESPAWN, 20000, 0, owner);
            if (!demon)
                return;
            soul = demon->GetGUID();
            credit = entry;
            hit = false;
            timer = 6500;
            player->QuestObjectiveSatisfy(gem, 1, QUEST_OBJECTIVE_GAMEOBJECT);
            me->CastSpell(demon, 178939, true);
            Talk(2, owner);
        }

        void SpellHitTarget(Unit* target, SpellInfo const* spell) override
        {
            if (timer && spell->Id == 178939 && target->GetGUID() == soul)
                hit = true;
        }

        void UpdateAI(uint32 diff) override
        {
            using namespace AzsunaScythe;
            Player* player = Owner();
            if (!Active(player))
            {
                if (Creature* demon = ObjectAccessor::GetCreature(*me, soul))
                    demon->DespawnOrUnsummon();
                me->DespawnOrUnsummon();
                return;
            }
            if (!Done(player, Allari) && me->GetDistance(Meeting) < 3.0f &&
                player->IsWithinDistInMap(me, 15.0f))
                player->KilledMonsterCredit(Allari);
            if (!timer)
                return;
            Creature* demon = ObjectAccessor::GetCreature(*me, soul);
            if (!demon || !demon->IsAlive() || !player->IsWithinDistInMap(demon, 40.0f))
            {
                if (demon)
                    demon->DespawnOrUnsummon();
                timer = 0;
                return;
            }
            if (timer > diff)
            {
                timer -= diff;
                return;
            }
            timer = 0;
            if (hit && !Done(player, credit))
                player->KilledMonsterCredit(credit);
            demon->DespawnOrUnsummon();
            soul.Clear();
            if (hit)
                Talk(credit == 90402 ? 3 : 7, owner);
        }
    };
    CreatureAI* GetAI(Creature* creature) const override { return new AI(creature); }
};

class go_azsuna_soul_gem : public GameObjectScript
{
public:
    go_azsuna_soul_gem() : GameObjectScript("go_azsuna_soul_gem") {}
    bool OnGossipHello(Player* player, GameObject* go) override
    {
        using namespace AzsunaScythe;
        // Consume rejected uses too: the generic goober handler must not grant
        // release/entry credit or cast its spell when prerequisites failed.
        if (!Active(player) || !player->IsWithinDistInMap(go, 5.0f))
            return true;
        if (go->GetEntry() == 239338 || go->GetEntry() == 239332)
        {
            if (CanRelease(player, go->GetEntry()))
                if (Creature* allari = OwnedSummon(player, Allari))
                    allari->AI()->SetGUID(go->GetGUID(), go->GetEntry());
        }
        else if (go->GetEntry() == 237017 && Done(player, 89276) && !Done(player, 89673))
        {
            if (OwnedSummon(player, 89673))
                return true;
            Creature* demon = player->SummonCreature(89673, InnerSoul,
                TEMPSUMMON_TIMED_DESPAWN, 300000, 0, player->GetGUID());
            if (!demon)
                return true;
            demon->setFaction(14);
            demon->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC |
                UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_NPC);
            demon->SetReactState(REACT_AGGRESSIVE);
            // Native screen effect only. Its obsolete trigger spells are absent
            // in 26972; personal actors provide the isolated recovery encounter.
            player->AddAura(SoulScreen, player);
            player->QuestObjectiveSatisfy(237017, 1, QUEST_OBJECTIVE_GAMEOBJECT);
        }
        else if (go->GetEntry() == 240012 && player->HasAura(SoulScreen))
        {
            player->QuestObjectiveSatisfy(240012, 1, QUEST_OBJECTIVE_GAMEOBJECT);
            Cleanup(player);
        }
        return true;
    }
};

class player_azsuna_scythe : public PlayerScript
{
public:
    player_azsuna_scythe() : PlayerScript("player_azsuna_scythe") {}
    std::map<ObjectGuid, uint32> timers;
    std::mutex mutex;

    void OnLogout(Player* player) override
    {
        AzsunaScythe::Cleanup(player);
        std::lock_guard<std::mutex> lock(mutex);
        timers.erase(player->GetGUID());
    }
    void OnMapChanged(Player* player) override
    {
        if (player->GetMapId() != 1220)
            OnLogout(player);
    }
    void OnUpdate(Player* player, uint32 diff) override
    {
        using namespace AzsunaScythe;
        if (player->GetMapId() != 1220)
            return;
        {
            std::lock_guard<std::mutex> lock(mutex);
            uint32& timer = timers[player->GetGUID()];
            timer += diff;
            if (timer < 1000)
                return;
            timer = 0;
        }
        if (!Active(player))
        {
            Cleanup(player);
            return;
        }
        if (!Done(player, 89276) && !OwnedSummon(player, Allari))
            player->SummonCreature(Allari, Meeting, TEMPSUMMON_TIMED_DESPAWN, 300000, 0, player->GetGUID());
        if (player->HasAura(SoulScreen) && !Done(player, 89673) && !OwnedSummon(player, 89673))
            player->RemoveAurasDueToSpell(SoulScreen); // Re-enter after a failed attempt.
        if (Done(player, 89673) && !Done(player, 89398))
            if (Creature* allari = player->FindNearestCreature(89398, 10.0f))
                if (player->IsWithinLOSInMap(allari))
                {
                    player->RemoveAurasDueToSpell(SoulScreen);
                    player->KilledMonsterCredit(89398);
                }
    }
};

void AddSC_azsuna_scythe()
{
    new npc_azsuna_allari_q37660();
    new go_azsuna_soul_gem();
    new player_azsuna_scythe();
}
