#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SmartAI.h"
#include "Player.h"
#include "GossipDef.h"
#include "GameObject.h"
#include "QuestData.h"
#include <list>
#include <map>

namespace StormheimHelheimBargain
{
constexpr uint32 QuestId = 39848, ApproachCredit = 97696, SpeakCredit = 97479;
Position const Throne = {484.451f, 717.054f, 23.3577f, 0};
Position const Altar = {490.429f, 721.396f, 23.2744f, 3.80208f};

bool Active(Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive() || player->GetMapId() != 1463 ||
        player->GetInstanceId() || player->GetZoneId() != 7588 || player->HasPhaseId(8306) ||
        player->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE || player->IsBeingTeleported() ||
        player->GetVehicle() || player->GetTransport() || player->isInFlight() || player->isInCombat())
        return false;

    auto quest = sQuestDataStore->GetQuestTemplate(QuestId);
    if (!quest || quest->StartScript || quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT))
        return false;
    uint32 count = 0;
    for (auto const& objective : quest->GetObjectives())
    {
        if (objective.Type != QUEST_OBJECTIVE_MONSTER || objective.Amount != 1 ||
            !((objective.ObjectID == ApproachCredit && objective.StorageIndex == 0 && !objective.Flags) ||
              (objective.ObjectID == SpeakCredit && objective.StorageIndex == 2 && objective.Flags == 2)))
            return false;
        ++count;
    }
    return count == 2;
}

void Credit(Player* player, uint32 entry, ObjectGuid source)
{
    if (!Active(player) || player->GetQuestObjectiveData(QuestId, entry))
        return;
    auto quest = sQuestDataStore->GetQuestTemplate(QuestId);
    for (auto const& objective : quest->GetObjectives())
        if (objective.ObjectID == entry)
        {
            player->SetQuestObjectiveData(quest, &objective, 1);
            player->SendQuestUpdateAddCredit(quest, source, objective, 1);
            if (player->CanCompleteQuest(QuestId))
                player->CompleteQuest(QuestId);
            return;
        }
}

bool Source(Player* player, WorldObject* source, Position const& point, float range)
{
    return source && source->IsInWorld() && source->GetMapId() == 1463 &&
        source->GetExactDist(&point) < 1 && player->InSamePhase(source) &&
        player->canSeeOrDetect(source) && player->GetExactDist(source) <= range && player->IsWithinLOSInMap(source);
}

bool IsAltar(GameObject* altar)
{
    return altar && altar->GetEntry() == 244559 && altar->GetDBTableGUIDLow() == 127713 &&
        altar->GetGoType() == GAMEOBJECT_TYPE_QUESTGIVER && altar->GetGOInfo()->questgiver.gossipID == 18790;
}
}

// The duplicate native credit markers share a position. Each writes the same
// capped objective; neither Lady Sylvanas nor a pet supplies approach credit.
struct npc_stormheim_helheim_throne : ScriptedAI
{
    explicit npc_stormheim_helheim_throne(Creature* creature) : ScriptedAI(creature) { }
    uint32 timer = 0;
    void UpdateAI(uint32 diff) override
    {
        using namespace StormheimHelheimBargain;
        if (timer > diff) { timer -= diff; return; }
        timer = 500;
        if (me->GetEntry() != ApproachCredit || !me->IsAlive() ||
            (me->GetDBTableGUIDLow() != 338828 && me->GetDBTableGUIDLow() != 340857))
            return;
        std::list<Player*> players;
        me->GetPlayerListInGrid(players, 5);
        for (auto player : players)
            if (Active(player) && Source(player, me, Throne, 5))
                Credit(player, ApproachCredit, me->GetGUID());
    }
};

struct go_stormheim_helheim_bargain : SmartGameObjectAI
{
    explicit go_stormheim_helheim_bargain(GameObject* object) : SmartGameObjectAI(object) { }
    std::map<ObjectGuid, std::weak_ptr<uint8>> offered;

    bool GossipHello(Player* player, bool isUse) override
    {
        using namespace StormheimHelheimBargain;
        for (auto it = offered.begin(); it != offered.end();)
            if (it->second.expired()) it = offered.erase(it); else ++it;
        offered.erase(player->GetGUID());
        if (Active(player) && IsAltar(go) && go->isSpawned() &&
            !go->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) &&
            Source(player, go, Altar, go->GetInteractionDistance()))
        {
            player->AddQuestDelayedEvent(QuestId, 0, [] {});
            offered[player->GetGUID()] = player->getQuestStatus(QuestId)->ScriptLifetime;
        }
        return SmartGameObjectAI::GossipHello(player, isUse);
    }

    bool GossipSelect(Player* player, uint32 menu, uint32 option) override
    {
        using namespace StormheimHelheimBargain;
        // Retain the native39857 acceptance and later39855 exit option1.
        if (menu != 18790 || option != 0)
            return SmartGameObjectAI::GossipSelect(player, menu, option);
        auto offer = offered.find(player->GetGUID());
        auto status = player->getQuestStatus(QuestId);
        auto& gossip = player->PlayerTalkClass->GetGossipMenu();
        if (!Active(player) || !IsAltar(go) || !go->isSpawned() ||
            go->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) ||
            !Source(player, go, Altar, go->GetInteractionDistance()) ||
            !player->GetQuestObjectiveData(QuestId, ApproachCredit) ||
            player->GetQuestObjectiveData(QuestId, SpeakCredit) ||
            gossip.GetMenuId() != menu || !gossip.GetItem(option) || !status ||
            offer == offered.end() || offer->second.expired() || status->ScriptLifetime != offer->second.lock())
            return true;
        offered.erase(offer);
        player->PlayerTalkClass->ClearMenus();
        player->PlayerTalkClass->SendCloseGossip();
        // Native212898 creates conversation2251. Its failure must not consume
        // the player's actual, validated request without objective progress.
        Credit(player, SpeakCredit, go->GetGUID());
        player->CastSpell(player, 212898, true);
        return true;
    }
};

void AddSC_stormheim_helheim_bargain()
{
    RegisterCreatureAI(npc_stormheim_helheim_throne);
    RegisterGameObjectAI(go_stormheim_helheim_bargain);
}
