#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "Creature.h"
#include "QuestDef.h"

namespace StormheimNewBeginning
{
struct Finale
{
    uint32 quest, team, actor, spawn, spell, firstCredit;
    float x, y, z, orientation;
};
Finale const AllianceFinale{39122, ALLIANCE, 95804, 338653, 190073, 95269, 2851.9f, 863.39f, 77.8f, 0.3f};
Finale const HordeFinale{38882, HORDE, 94228, 340424, 205551, 94915, 2370.01f, 1849.31f, 29.17f, 4.78f};

bool CanStart(Player* player, Creature* genn, Finale const& finale = AllianceFinale)
{
    if (!player || !genn || !player->IsInWorld() || !player->IsAlive() ||
        !genn->IsInWorld() || !genn->IsAlive() || player->GetTeam() != finale.team ||
        player->GetMapId() != 1220 || player->GetZoneId() != 7541 || player->GetInstanceId() ||
        genn->GetMapId() != 1220 || genn->GetInstanceId() || genn->GetEntry() != finale.actor ||
        genn->GetDBTableGUIDLow() != finale.spawn || !player->InSamePhase(genn) ||
        !player->IsWithinDistInMap(genn, INTERACTION_DISTANCE) || player->isWatchingMovie() ||
        player->IsBeingTeleported() || player->GetVehicle() ||
        player->GetTransport() || player->isInFlight())
        return false;

    auto status = player->GetQuestStatus(finale.quest);
    // A legacy completed quest or logout during playback can leave the player
    // at Genn without the camp transfer. Talking here safely replays the movie.
    return status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE;
}

void Start(Player* player, Creature* genn, Finale const& finale = AllianceFinale)
{
    if (!CanStart(player, genn, finale))
        return;

    player->PlayerTalkClass->SendCloseGossip();
    // SmartAI85 schedules this cast 200ms later; its linked teleport ran first.
    // Start the native movie synchronously, including its other effects.
    player->CastSpell(player, finale.spell, true);
    if (!player->isWatchingMovie())
        return; // Failed/missing movie: no credit/transfer; gossip can retry.

    // Do not share another player's cinematic completion with the group.
    player->KilledMonsterCredit(finale.firstCredit);
    player->KilledMonsterCredit(94431);
    // SendMovieStart sets CanDelayTeleport; native completion/skip releases it.
    // Keep the existing camp destination and normal pet/mover arrival handling.
    player->TeleportTo(1220, finale.x, finale.y, finale.z, finale.orientation);
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

struct npc_stormheim_undeath_sylvanas : SmartAI
{
    explicit npc_stormheim_undeath_sylvanas(Creature* creature) : SmartAI(creature) { }
    void sQuestAccept(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() == 38882)
            StormheimNewBeginning::Start(player, me, StormheimNewBeginning::HordeFinale);
        else
            SmartAI::sQuestAccept(player, quest);
    }
    void sGossipHello(Player* player) override
    {
        SmartAI::sGossipHello(player);
        StormheimNewBeginning::Start(player, me, StormheimNewBeginning::HordeFinale);
    }
};

void AddSC_stormheim_new_beginning()
{
    TC_LOG_INFO("server.loading", "Stormheim: New Beginning movie before camp transfer");
    RegisterCreatureAI(npc_stormheim_new_beginning_genn);
    TC_LOG_INFO("server.loading", "Stormheim: Undeath movie before personal camp transfer");
    RegisterCreatureAI(npc_stormheim_undeath_sylvanas);
}
