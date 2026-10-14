#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "QuestData.h"

namespace StormheimGrapple
{
constexpr uint32 ItemId = 138111, PickupQuest = 39775;
struct Branch { uint32 quest, tree, next, team, giver, receiver; };
Branch const branches[] = {{38312,55241,38318,ALLIANCE,91222,91481}, {38612,55243,38613,HORDE,92561,92566}};
bool Active(Player* player, uint32 quest)
{
    QuestStatus status = player->GetQuestStatus(quest);
    return status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE;
}
QuestObjective const* Objective(Branch const& branch)
{
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(branch.quest))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.Type == QUEST_OBJECTIVE_COMPLETE_CRITERIA_TREE && objective.ObjectID == branch.tree &&
                objective.StorageIndex == 0 && objective.Amount == 1 && !(objective.Flags & QUEST_OBJECTIVE_FLAG_OPTIONAL))
                return &objective;
    return nullptr;
}
bool Present(Player* player)
{
    return player && player->IsInWorld() && player->IsAlive() && !player->IsBeingTeleported();
}
bool Pickup(Player* player)
{
    if (player->GetQuestRewardStatus(PickupQuest)) return true;
    if (!player->GetItemByEntry(ItemId, true)) return false;
    //191831 only marks an out-of-log quest when it has TRACKING. The native
    //39775 row does not. Record actual acquisition with the same native
    //reward-status helper; preserve its flags and do not award another item.
    player->SetRewardedQuest(PickupQuest);
    return player->GetQuestRewardStatus(PickupQuest);
}
// Rebuild native criteria from the permanent pickup flag or an actual owned
// launcher, never from the old optional item counter. Entry lookup also sees
// loaded bag/bank items that have not entered the world yet during login.
void Sync(Player* player)
{
    if (!Present(player)) return;
    bool active = false;
    for (Branch const& branch : branches)
        if (player->GetTeam() == branch.team && Active(player, branch.quest))
            if (QuestObjective const* objective = Objective(branch))
            {
                active = true;
                if (!player->GetQuestRewardStatus(PickupQuest))
                {
                    Quest const* quest = sQuestDataStore->GetQuestTemplate(branch.quest);
                    player->SetQuestObjectiveData(quest, objective, 0);
                    if (player->GetQuestStatus(branch.quest) == QUEST_STATUS_COMPLETE)
                        player->SetQuestStatus(branch.quest, QUEST_STATUS_INCOMPLETE);
                }
            }
    if (!active) return;
    // Only an actual acquisition or its already-earned pickup flag qualifies.
    if (!Pickup(player)) return;
    player->UpdateAchievementCriteria(CRITERIA_TYPE_COMPLETE_QUEST, PickupQuest);
}
bool Scope(Creature* creature)
{
    return creature && creature->IsInWorld() && creature->GetMapId() == 1220 && creature->GetZoneId() == 7541;
}
bool Eligible(Player* player, Creature* creature, Branch const& branch)
{
    return Present(player) && Scope(creature) && creature->IsAlive() && !creature->IsQuestObjectExpired() &&
        player->GetTeam() == branch.team && Objective(branch) &&
        (creature->GetEntry() == branch.giver || creature->GetEntry() == branch.receiver) &&
        creature->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER) &&
        player->InSamePhase(creature) && player->canSeeOrDetect(creature) &&
        player->IsWithinDistInMap(creature, 5.0f) && player->IsWithinLOSInMap(creature);
}
void Supply(Player* player, Creature* creature)
{
    for (Branch const& branch : branches)
        if (Eligible(player, creature, branch) && (Active(player, branch.quest) || Active(player, branch.next)))
        {
            // Retain an existing launcher, including one in a loaded bag/bank.
            // Full bags or a failed grant must not synthesize pickup progress.
            if (!player->GetItemByEntry(ItemId, true) && !player->AddItem(ItemId, 1)) return;
            Sync(player);
            Pickup(player); // Legacy players already in No Wings Required.
            return;
        }
}
}

struct npc_stormheim_grapple_supply_ai : SmartAI
{
    explicit npc_stormheim_grapple_supply_ai(Creature* creature) : SmartAI(creature) { }
    void sQuestAccept(Player* player, Quest const* quest) override
    {
        using namespace StormheimGrapple;
        for (Branch const& branch : branches)
            if (quest->GetQuestId() == branch.quest && Eligible(player, me, branch))
            {
                Supply(player, me);
                return; // Suppress the duplicate legacy SmartAI item grant.
            }
        SmartAI::sQuestAccept(player, quest);
    }
};

class npc_stormheim_grapple_supply : public CreatureScript
{
public:
    npc_stormheim_grapple_supply() : CreatureScript("npc_stormheim_grapple_supply") { }
    bool OnGossipHello(Player* player, Creature* creature) override
    {
        StormheimGrapple::Supply(player, creature);
        return false; // Keep the native questgiver menu and hand-in.
    }
    CreatureAI* GetAI(Creature* creature) const override { return new npc_stormheim_grapple_supply_ai(creature); }
};

class player_stormheim_grapple_pickup : public PlayerScript
{
public:
    player_stormheim_grapple_pickup() : PlayerScript("player_stormheim_grapple_pickup") { }
    void OnLogin(Player* player) override { StormheimGrapple::Sync(player); }
};

void AddSC_stormheim_grapple()
{
    new npc_stormheim_grapple_supply();
    new player_stormheim_grapple_pickup();
}
