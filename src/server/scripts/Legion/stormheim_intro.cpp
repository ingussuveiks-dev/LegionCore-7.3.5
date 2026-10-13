#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "QuestData.h"

namespace StormheimIntro
{
struct Journey
{
    uint32 team, introduction, summons, reader, arrivalCredit, portal, spell, map;
    Position destination;
};
Journey const AllianceJourney = {ALLIANCE,39735,38035,96644,96663,244567,192465,0,{-8479.7f,1402.06f,205.0f,3.26342f}};
Journey const HordeJourney = {HORDE,39864,38307,96683,96686,244571,192477,1,{1496.85f,-5199.62f,11.24f,4.49946f}};

bool Ready(Player* player)
{
    return player && player->IsInWorld() && player->IsAlive() && !player->IsBeingTeleported() &&
        !player->GetVehicle() && !player->GetTransport() && !player->isInFlight() && !player->IsFlying();
}

// The shared arrival marker 97377 belongs to other Legion introductions too.
// Update only the active quest for this faction, using its configured objective.
void Credit(Player* player, uint32 questId, uint32 entry)
{
    if (player->GetQuestStatus(questId) != QUEST_STATUS_INCOMPLETE || player->GetQuestObjectiveData(questId,entry)) return;
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(questId))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.ObjectID == entry)
            {
                player->SetQuestObjectiveData(quest,&objective,1);
                player->SendQuestUpdateAddCredit(quest,ObjectGuid::Empty,objective,1);
                if (player->CanCompleteQuest(questId)) player->CompleteQuest(questId);
                break;
            }
}

void ConfirmArrival(Player* player, Journey const& journey, uint32 retries)
{
    if (!player->IsAlive() || player->GetTeam() != journey.team ||
        player->GetQuestStatus(journey.summons) != QUEST_STATUS_INCOMPLETE ||
        !player->GetQuestObjectiveData(journey.summons,journey.reader)) return;
    if (player->IsBeingTeleported())
    {
        // Far teleports temporarily remove the player from the old map.
        if (retries) player->AddQuestDelayedEvent(journey.summons,500,[player,&journey,retries]() { ConfirmArrival(player,journey,retries-1); });
        return;
    }
    if (Ready(player) && player->GetMapId() == journey.map && player->GetExactDist(&journey.destination) < 12.0f)
        Credit(player,journey.summons,journey.arrivalCredit);
}
}

struct npc_stormheim_introduction : SmartAI
{
    explicit npc_stormheim_introduction(Creature* creature) : SmartAI(creature) { }
    void MoveInLineOfSight(Unit* who) override
    {
        Player* player = who ? who->ToPlayer() : nullptr;
        StormheimIntro::Journey const& journey = me->GetEntry() == 96644 ? StormheimIntro::AllianceJourney : StormheimIntro::HordeJourney;
        if (StormheimIntro::Ready(player) && player->GetTeam() == journey.team &&
            player->GetMapId() == 1220 && me->GetMapId() == 1220 && me->GetZoneId() == 7502 && player->GetExactDist(me) < 15.0f &&
            player->InSamePhase(me) && player->canSeeOrDetect(me) && player->IsWithinLOSInMap(me))
            StormheimIntro::Credit(player,journey.introduction,97377);
        if (who) SmartAI::MoveInLineOfSight(who);
    }
};

class go_stormheim_summons_portal : public GameObjectScript
{
public:
    go_stormheim_summons_portal() : GameObjectScript("go_stormheim_summons_portal") { }
    bool OnGossipHello(Player* player, GameObject* go) override
    {
        StormheimIntro::Journey const& journey = go->GetEntry() == 244567 ? StormheimIntro::AllianceJourney : StormheimIntro::HordeJourney;
        if (!StormheimIntro::Ready(player) || player->GetTeam() != journey.team ||
            player->GetMapId() != 1220 || go->GetMapId() != 1220 || go->GetZoneId() != 7502 ||
            !go->isSpawned() || go->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) ||
            !player->InSamePhase(go) || !player->canSeeOrDetect(go) || !player->IsWithinLOSInMap(go) ||
            player->GetExactDist(go) > 5.0f || player->GetQuestStatus(journey.summons) != QUEST_STATUS_INCOMPLETE ||
            !player->GetQuestObjectiveData(journey.summons,journey.reader)) return true;
        // Preserve the existing native spell/destination; confirm after its ack.
        if (player->CastSpell(player,journey.spell,true) == SPELL_CAST_OK)
            player->AddQuestDelayedEvent(journey.summons,500,[player,&journey]() { StormheimIntro::ConfirmArrival(player,journey,20); });
        return true;
    }
};

class player_stormheim_introduction : public PlayerScript
{
public:
    player_stormheim_introduction() : PlayerScript("player_stormheim_introduction") { }
    void OnLogin(Player* player) override
    {
        StormheimIntro::Journey const& journey = player->GetTeam() == ALLIANCE ? StormheimIntro::AllianceJourney : StormheimIntro::HordeJourney;
        // A logout can discard the acknowledgement callback after the teleport.
        // Recover only a read summons at its real destination, never in Dalaran.
        StormheimIntro::ConfirmArrival(player,journey,20);
    }
};

void AddSC_stormheim_intro()
{
    RegisterCreatureAI(npc_stormheim_introduction);
    new go_stormheim_summons_portal();
    new player_stormheim_introduction();
}
