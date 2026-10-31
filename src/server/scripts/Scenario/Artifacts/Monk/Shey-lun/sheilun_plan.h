#ifndef SHEILUN_PLAN_H
#define SHEILUN_PLAN_H

namespace SheilunPlan
{
struct Role { uint32 entry, spawn, menu, event, receipt; };
Role const Roles[] = {
    {101882, 362835, 19225, 48446, 46636},
    {101882, 362865, 19225, 48446, 46636},
    {101883, 362834, 19226, 48447, 46637},
    {101883, 362869, 19226, 48447, 46637},
    {101885, 362831, 19227, 48448, 46638},
    {101885, 362867, 19227, 48448, 46638}
};

inline Scenario* World(Creature* actor)
{
    if (!actor || !actor->IsInWorld() || !actor->IsAlive() || actor->IsPreDelete() ||
        actor->HasQuestObjectScope() || actor->GetMapId() != 1541) return nullptr;
    Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 1007 ? scenario : nullptr;
}

inline bool Receipt(Scenario* scenario, uint32 tree)
{
    auto const* progress = scenario ? scenario->GetAchievementMgr().GetCriteriaProgressMap() : nullptr;
    if (!progress) return false;
    auto found = progress->find(tree);
    return found != progress->end() && found->second.Counter >= 1;
}

inline bool Planned(Scenario* scenario)
{
    return Receipt(scenario, 46636) && Receipt(scenario, 46637) && Receipt(scenario, 46638);
}

inline bool PlayerContext(Player* player, Creature* actor)
{
    if (!player || !actor || !player->IsInWorld() || !player->IsAlive() || !player->CanContact() ||
        player->isGameMaster() || player->HasPendingTeleport() || player->getClass() != CLASS_MONK ||
        player->GetMap() != actor->GetMap() || !player->InSamePhase(actor) ||
        !player->canSeeOrDetect(actor) || !player->IsWithinLOSInMap(actor) ||
        !player->IsWithinDistInMap(actor, INTERACTION_DISTANCE) || player->GetQuestRewardStatus(41003))
        return false;
    QuestStatus status = player->GetQuestStatus(41003);
    return status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE;
}

inline bool Menu(Player* player, uint32 menu, uint32 option, uint32 expected, uint32 last)
{
    if (!player || !player->PlayerTalkClass || menu != expected || option > last) return false;
    auto const& gossip = player->PlayerTalkClass->GetGossipMenu();
    auto const& items = gossip.GetMenuItems();
    auto found = items.find(option);
    return gossip.GetMenuId() == menu && found != items.end() &&
        found->second.OptionNpc == GossipOptionNpc::None && !found->second.Sender &&
        !found->second.Action && !found->second.IsCoded && !found->second.BoxMoney;
}

inline void Refresh(Creature* actor)
{
    Scenario* scenario = World(actor);
    if (scenario && actor->GetEntry() == 101881 && actor->GetDBTableGUIDLow() == 362837 &&
        scenario->GetCurrentStep() == 3 && Planned(scenario) && !Receipt(scenario, 46662) &&
        !actor->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP))
        actor->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
}

inline void Close(Player* player)
{
    if (player && player->PlayerTalkClass)
    {
        // SendCloseGossip alone sends a packet and retains the menu items.
        // Consume this offer before native criteria can call back into scripts.
        player->PlayerTalkClass->ClearMenus();
        player->CLOSE_GOSSIP_MENU();
    }
}

#include "sheilun_tactics.h"

inline void Select(Player* player, Creature* actor, uint32 menu, uint32 option)
{
    Role const* role = nullptr;
    for (auto const& row : Roles)
        if (actor && actor->GetEntry() == row.entry && actor->GetDBTableGUIDLow() == row.spawn)
            role = &row;
    bool offered = role && Menu(player, menu, option, role->menu, 1);
    Close(player);
    Scenario* scenario = World(actor);
    if (!offered || !PlayerContext(player, actor) || !scenario || scenario->GetCurrentStep() != 2 ||
        !actor->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP) || Receipt(scenario, role->receipt)) return;
    player->AddQuestDelayedEvent(41003, 0, []() { });
    auto status = player->getQuestStatus(41003);
    if (!status || !status->ScriptLifetime) return;
    std::weak_ptr<uint8> attempt = status->ScriptLifetime;
    Map* map = actor->GetMap();
    ObjectGuid recipient = player->GetGUID(), source = actor->GetGUID();
    uint32 receipt = role->receipt, entry = role->entry, spawn = role->spawn;
    if (!ApplyTactic(player, actor, *role, option, map, recipient, source, attempt)) return;
    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, role->event);
    // Criteria callbacks can replace a quest attempt, change maps or remove
    // actors. Resolve both original objects after the real native delivery.
    player = ObjectAccessor::FindPlayer(map, recipient);
    actor = map->GetCreature(source);
    scenario = World(actor);
    status = player ? player->getQuestStatus(41003) : nullptr;
    if (attempt.expired() || !status || status->ScriptLifetime != attempt.lock() ||
        !PlayerContext(player, actor) || !scenario || actor->GetEntry() != entry ||
        actor->GetDBTableGUIDLow() != spawn || scenario->GetCurrentStep() < 2 ||
        scenario->GetCurrentStep() > 3 || !Receipt(scenario, receipt)) return;
    actor->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
    if (Creature* taran = actor->FindNearestCreature(101881, 100.0f, true)) Refresh(taran);
}

inline void Start(Player* player, Creature* actor, uint32 menu, uint32 option)
{
    bool offered = Menu(player, menu, option, 19235, 0);
    Close(player);
    Scenario* scenario = World(actor);
    if (!offered || !PlayerContext(player, actor) || !scenario || actor->GetEntry() != 101881 ||
        actor->GetDBTableGUIDLow() != 362837 || scenario->GetCurrentStep() != 3 || !Planned(scenario) ||
        Receipt(scenario, 46662) || !actor->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP)) return;
    player->AddQuestDelayedEvent(41003, 0, []() { });
    auto status = player->getQuestStatus(41003);
    if (!status || !status->ScriptLifetime) return;
    std::weak_ptr<uint8> attempt = status->ScriptLifetime;
    Map* map = actor->GetMap();
    ObjectGuid recipient = player->GetGUID(), source = actor->GetGUID();
    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 48494);
    player = ObjectAccessor::FindPlayer(map, recipient);
    actor = map->GetCreature(source);
    scenario = World(actor);
    status = player ? player->getQuestStatus(41003) : nullptr;
    if (attempt.expired() || !status || status->ScriptLifetime != attempt.lock() ||
        !PlayerContext(player, actor) || !scenario || actor->GetEntry() != 101881 ||
        actor->GetDBTableGUIDLow() != 362837 || scenario->GetCurrentStep() != 3 ||
        !Planned(scenario) || !Receipt(scenario, 46662)) return;
    actor->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
    actor->GetMotionMaster()->MovePoint(0, -1019.15f, -2795.95f, 38.91f);
    if (Creature* mob = actor->FindNearestCreature(101882, 107.0f, true))
    {
        mob->GetMotionMaster()->MovePoint(0, -1019.15f, -2795.95f, 38.91f);
        if (CreatureAI* ai = mob->AI()) ai->Talk(4);
        mob->CastSpell(mob, 60958);
    }
    if (Creature* mob = actor->FindNearestCreature(101885, 107.0f, true))
        mob->GetMotionMaster()->MovePoint(0, -1019.15f, -2795.95f, 38.91f);
    if (Creature* mob = actor->FindNearestCreature(101883, 107.0f, true))
        mob->GetMotionMaster()->MovePoint(0, -1019.15f, -2795.95f, 38.91f);
    if (GameObject* go = actor->FindNearestGameObject(246464, 100.0f)) go->SetPhaseMask(2, true);
}
}
#endif
