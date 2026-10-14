#include "ScriptMgr.h"
#include "GameObjectAI.h"
#include "Player.h"
#include "QuestData.h"

namespace StormheimImpalement
{
constexpr uint32 Launcher = 251257;
bool Scope(GameObject* object)
{
    return object && object->IsInWorld() && object->GetEntry() == Launcher &&
        object->GetMapId() == 1220 && object->GetZoneId() == 7541;
}
Quest const* Definition(uint32 id)
{
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(id))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.Type == QUEST_OBJECTIVE_GAMEOBJECT && objective.ObjectID == Launcher &&
                objective.StorageIndex == 0 && objective.Amount == 7 && !(objective.Flags & QUEST_OBJECTIVE_FLAG_OPTIONAL))
                return quest;
    return nullptr;
}
Quest const* Active(Player* player)
{
    if (!player) return nullptr;
    uint32 id = player->GetTeam() == ALLIANCE ? 38410 : 38615;
    return player->GetQuestStatus(id) == QUEST_STATUS_INCOMPLETE ? Definition(id) : nullptr;
}
bool Ready(Player* player, GameObject* object)
{
    return Active(player) && player->IsInWorld() && player->IsAlive() && !player->IsBeingTeleported() &&
        !player->GetVehicle() && !player->GetTransport() && Scope(object) && object->isSpawned() &&
        !object->IsQuestObjectExpired() && !object->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) &&
        object->GetGoType() == GAMEOBJECT_TYPE_GOOBER && object->GetGOInfo()->goober.open == 43 &&
        player->InSamePhase(object) && player->canSeeOrDetect(object) && !object->ShouldHideFor(player->GetGUID()) &&
        player->IsWithinDistInMap(object, object->GetInteractionDistance()) && player->IsWithinLOSInMap(object);
}
void Credit(Player* player, GameObject* object)
{
    if (!Ready(player, object)) return;
    Quest const* quest = Active(player);
    uint32 count = player->GetQuestObjectiveData(quest->GetQuestId(), Launcher);
    if (count >= 7) return;
    // Consume the shared object's current spawn before credit. A second hit or
    // another user must wait for its ordinary180s respawn, never double-count.
    object->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE);
    object->SetLootState(GO_JUST_DEACTIVATED, player);
    for (QuestObjective const& objective : quest->GetObjectives())
        if (objective.Type == QUEST_OBJECTIVE_GAMEOBJECT && objective.ObjectID == Launcher)
        {
            player->SetQuestObjectiveData(quest, &objective, count + 1);
            player->SendQuestUpdateAddCredit(quest, object->GetGUID(), objective, count + 1);
            if (player->CanCompleteQuest(quest->GetQuestId())) player->CompleteQuest(quest->GetQuestId());
            return;
        }
}
void RepairLegacy(Player* player)
{
    uint32 id = player->GetTeam() == ALLIANCE ? 38410 : 38615;
    if (!Definition(id) || player->GetQuestStatus(id) != QUEST_STATUS_COMPLETE || player->GetQuestObjectiveData(id, Launcher) >= 7) return;
    player->SetQuestStatus(id, QUEST_STATUS_INCOMPLETE);
}
}

struct go_stormheim_impalement_ai : GameObjectAI
{
    explicit go_stormheim_impalement_ai(GameObject* object) : GameObjectAI(object) { }
    bool GossipHello(Player* player, bool opened) override
    {
        if (!StormheimImpalement::Scope(go)) return false;
        // Only the successful native EffectOpenLock path passes true. Client
        // report-use passes false; ordinary Use is blocked by the script below.
        if (opened) StormheimImpalement::Credit(player, go);
        return true;
    }
};

class go_stormheim_impalement : public GameObjectScript
{
public:
    go_stormheim_impalement() : GameObjectScript("go_stormheim_impalement") { }
    bool OnGossipHello(Player*, GameObject* object) override { return StormheimImpalement::Scope(object); }
    GameObjectAI* GetAI(GameObject* object) const override { return new go_stormheim_impalement_ai(object); }
};
class player_stormheim_impalement : public PlayerScript
{
public:
    player_stormheim_impalement() : PlayerScript("player_stormheim_impalement") { }
    void OnLogin(Player* player) override { StormheimImpalement::RepairLegacy(player); }
};
void AddSC_stormheim_impalement()
{
    new go_stormheim_impalement();
    new player_stormheim_impalement();
}
