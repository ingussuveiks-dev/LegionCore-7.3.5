#include "ScriptMgr.h"
#include "GameObject.h"
#include "Player.h"

// 242279 is the existing door in Sleeper's Barrow. 7.3.5 objective
// 280418 requires opening it in addition to killing Morphael.
class go_valsharah_bramble_wall : public GameObjectScript
{
public:
    go_valsharah_bramble_wall() : GameObjectScript("go_valsharah_bramble_wall") { }
    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (go->GetEntry() != 242279 || go->GetMapId() != 1220 || go->GetZoneId() != 7558)
            return false;
        if (player->GetQuestStatus(38147) != QUEST_STATUS_INCOMPLETE || !player->IsAlive() ||
            player->GetDistance(go) > 5.0f)
            return false;
        go->UseDoorOrButton(30, false, player);
        if (!player->GetQuestObjectiveData(38147, 99032))
            player->KilledMonsterCredit(99032);
        return true;
    }
};

void AddSC_valsharah_quest_support()
{
    new go_valsharah_bramble_wall();
}
