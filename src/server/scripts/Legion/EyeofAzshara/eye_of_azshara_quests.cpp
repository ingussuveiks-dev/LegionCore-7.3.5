#include "ScriptMgr.h"
#include "GameObjectAI.h"
#include "GameObject.h"
#include "Player.h"

namespace EyeQuests
{
// Existing 7.3.5 spell_target_position and decorative teleport-pad spawns.
Position const Arrival = {-784.91f,4419.19f,602.48f,2.45236f};
Position const UpperPad = {-844.597f,4467.76f,736.042f,0};
Position const LowerPad = {-779.958f,4415.29f,602.629f,0};

void ConfirmTeleport(Player* player, uint32 retries)
{
    if (!player->IsAlive() || player->GetMapId() != 1220 ||
        (player->GetQuestStatus(42213) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40890) != QUEST_STATUS_INCOMPLETE))
        return;
    if (player->IsBeingTeleported())
    {
        if (retries) player->AddDelayedEvent(500, [player, retries]() { ConfirmTeleport(player, retries - 1); });
        return;
    }
    if (player->GetDistance(Arrival) >= 8.0f) return;
    if (player->GetQuestStatus(42213) == QUEST_STATUS_INCOMPLETE && !player->GetQuestObjectiveData(42213, 106815))
        player->KilledMonsterCredit(106815);
    if (player->GetQuestStatus(40890) == QUEST_STATUS_INCOMPLETE && !player->GetQuestObjectiveData(40890, 109750))
        player->KilledMonsterCredit(109750);
}
}

// The two visual pads are type 5 (not clickable). Walking onto the actual
// pad triggers its native teleport. Arrival points lie outside the pad's
// radius, so returning cannot immediately send the player back downstairs.
class go_eye_portrait_teleporter : public GameObjectScript
{
public:
    go_eye_portrait_teleporter() : GameObjectScript("go_eye_portrait_teleporter") { }
    struct AI : GameObjectAI
    {
        AI(GameObject* object) : GameObjectAI(object) { }
        uint32 timer = 0;
        void UpdateAI(uint32 diff) override
        {
            timer += diff;
            if (timer < 500) return;
            timer = 0;
            if (go->GetMapId() != 1220 || go->GetZoneId() != 7502) return;
            bool upper = go->GetEntry() == 244534;
            if ((!upper && go->GetEntry() != 244560) ||
                go->GetDistance(upper ? EyeQuests::UpperPad : EyeQuests::LowerPad) > 1.0f) return;
            std::list<Player*> players;
            go->GetPlayerListInGrid(players, 2.0f);
            for (Player* player : players)
            {
                if (!player->IsAlive() || player->IsBeingTeleported() || player->GetVehicle() ||
                    player->isInFlight() || player->GetDistance(go) > 2.0f) continue;
                player->CastSpell(player, upper ? 192293 : 192295, true);
                if (upper && (player->GetQuestStatus(42213) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(40890) == QUEST_STATUS_INCOMPLETE))
                    player->AddDelayedEvent(500, [player]() { EyeQuests::ConfirmTeleport(player, 20); });
            }
        }
    };
    GameObjectAI* GetAI(GameObject* go) const override { return new AI(go); }
};

void AddSC_eye_of_azshara_quests()
{
    new go_eye_portrait_teleporter();
}
