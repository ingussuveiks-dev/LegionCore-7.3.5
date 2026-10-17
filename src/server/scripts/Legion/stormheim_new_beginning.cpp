#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "Creature.h"
#include "QuestDef.h"

namespace StormheimNewBeginning
{
bool CanStart(Player* player, Creature* genn)
{
    if (!player || !genn || !player->IsInWorld() || !player->IsAlive() ||
        !genn->IsInWorld() || !genn->IsAlive() || player->GetTeam() != ALLIANCE ||
        player->GetMapId() != 1220 || player->GetZoneId() != 7541 || player->GetInstanceId() ||
        genn->GetMapId() != 1220 || genn->GetInstanceId() || genn->GetEntry() != 95804 ||
        genn->GetDBTableGUIDLow() != 338653 || !player->InSamePhase(genn) ||
        !player->IsWithinDistInMap(genn, INTERACTION_DISTANCE) || player->isWatchingMovie() ||
        player->IsBeingTeleported() || player->GetVehicle() ||
        player->GetTransport() || player->isInFlight())
        return false;

    auto status = player->GetQuestStatus(39122);
    // A legacy completed quest or logout during playback can leave the player
    // at Genn without the camp transfer. Talking here safely replays the movie.
    return status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE;
}

void Start(Player* player, Creature* genn)
{
    if (!CanStart(player, genn))
        return;

    player->PlayerTalkClass->SendCloseGossip();
    // SmartAI85 schedules this cast 200ms later; its linked teleport ran first.
    // Start native PLAY_MOVIE494 synchronously, including its other effects.
    player->CastSpell(player, 190073, true);
    if (!player->isWatchingMovie())
        return; // Failed/missing movie: no credit/transfer; gossip can retry.

    // Do not share another player's cinematic completion with the group.
    player->KilledMonsterCredit(95269);
    player->KilledMonsterCredit(94431);
    // SendMovieStart sets CanDelayTeleport; native completion/skip releases it.
    // Keep the existing camp destination and normal pet/mover arrival handling.
    player->TeleportTo(1220, 2851.9f, 863.39f, 77.8f, 0.3f);
}
}

struct npc_stormheim_new_beginning_genn : SmartAI
{
    explicit npc_stormheim_new_beginning_genn(Creature* creature) : SmartAI(creature) { }

    void sQuestAccept(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() == 39122)
            StormheimNewBeginning::Start(player, me);
        else
            SmartAI::sQuestAccept(player, quest);
    }

    void sGossipHello(Player* player) override
    {
        SmartAI::sGossipHello(player);
        StormheimNewBeginning::Start(player, me);
    }
};

void AddSC_stormheim_new_beginning()
{
    TC_LOG_INFO("server.loading", "Stormheim: New Beginning movie before camp transfer");
    RegisterCreatureAI(npc_stormheim_new_beginning_genn);
}
