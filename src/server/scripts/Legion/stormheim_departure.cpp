#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "QuestData.h"
#include "GossipDef.h"
#include "LFGMgr.h"
#include "Group.h"
#include "ScenarioMgr.h"
#include "CreatureTextMgr.h"
#include "ObjectAccessor.h"
#include "DBCEnums.h"
#include "stormheim_departure.h"
#include "stormheim_fleet.h"
#include <unordered_map>

namespace StormheimDeparture
{
struct Route { uint32 team, quest, map, npc, dungeon, scenario; };
Route const AllianceRoute = {ALLIANCE,39800,0,96644,969,826};
Route const HordeRoute = {HORDE,39801,1,96686,998,824};
Route const& For(Player* player) { return player->GetTeam() == ALLIANCE ? AllianceRoute : HordeRoute; }
bool Ready(Player* player)
{
    return player && player->IsInWorld() && player->IsAlive() && !player->IsBeingTeleported() &&
        !player->isInCombat() && !player->GetVehicle() && !player->GetTransport() && !player->isInFlight() && !player->IsFlying();
}
bool Eligible(Player* player, Creature* npc)
{
    if (!Ready(player) || !npc) return false;
    Route const& route = For(player);
    return npc->IsAlive() && npc->GetEntry() == route.npc && player->GetMapId() == route.map && npc->GetMapId() == route.map &&
        player->GetQuestStatus(route.quest) == QUEST_STATUS_INCOMPLETE && player->InSamePhase(npc) &&
        player->canSeeOrDetect(npc) && player->IsWithinLOSInMap(npc) && player->GetExactDist(npc) <= 6.0f;
}
// Weak quest lifetimes allow a fresh attempt after abandonment or logout.
// The generation prevents an older callback from consuming a newer attempt.
struct Attempt { std::weak_ptr<uint8> lifetime; uint64 generation; };
std::unordered_map<ObjectGuid,Attempt> pending;
uint64 nextGeneration = 0;
struct Queued { std::weak_ptr<uint8> lifetime; uint32 dungeon; };
std::unordered_map<ObjectGuid,Queued> queued;

void UpdateQueue(Player* player)
{
    auto found = queued.find(player->GetGUID());
    if (found == queued.end()) return;
    uint32 queueId = sLFGMgr->GetQueueId(player->GetGUID(),found->second.dungeon);
    uint32 state = sLFGMgr->GetState(player->GetGUID(),queueId);
    if (found->second.lifetime.expired())
    {
        lfg::LfgDungeonSet const& slots = sLFGMgr->GetSelectedDungeons(player->GetGUID(),queueId);
        if (state != lfg::LFG_STATE_NONE && state != lfg::LFG_STATE_DUNGEON && slots.size() == 1 &&
            (*slots.begin() & 0xFFFFF) == found->second.dungeon)
            sLFGMgr->LeaveLfg(player->GetGUID(),queueId);
        queued.erase(found);
    }
    else if (state == lfg::LFG_STATE_NONE || state == lfg::LFG_STATE_DUNGEON || player->GetMapId() == 1475)
        queued.erase(found);
}

bool CanQueue(Player* player, Route const& route)
{
    Group* group = player->GetGroup();
    if (group) return group->isLFGGroup() && sLFGMgr->GetDungeon(group->GetGUID()) == route.dungeon &&
        sLFGMgr->GetState(group->GetGUID(),sLFGMgr->GetQueueId(group->GetGUID())) == lfg::LFG_STATE_DUNGEON;
    return sLFGMgr->GetState(player->GetGUID(),sLFGMgr->GetQueueId(player->GetGUID())) == lfg::LFG_STATE_NONE;
}
bool Queue(Player* player, Route const& route)
{
    if (!CanQueue(player,route)) return false;
    Group* group = player->GetGroup();
    if (group && group->isLFGGroup())
    {
        uint32 dungeon = sLFGMgr->GetDungeon(group->GetGUID());
        if (dungeon != route.dungeon) return false;
        if (sLFGMgr->GetState(group->GetGUID(),sLFGMgr->GetQueueId(group->GetGUID())) == lfg::LFG_STATE_DUNGEON)
        {
            sLFGMgr->TeleportPlayer(player,false);
            return true;
        }
    }
    // Never replace an unrelated queue or enroll another group member.
    if (group || sLFGMgr->GetState(player->GetGUID(),sLFGMgr->GetQueueId(player->GetGUID())) != lfg::LFG_STATE_NONE) return false;
    lfg::LfgDungeonSet slots{route.dungeon};
    uint32 previousScenario=player->GetScenarioId();
    player->SetScenarioId(route.scenario);
    sLFGMgr->JoinLfg(player,lfg::PLAYER_ROLE_DAMAGE,slots);
    if(sLFGMgr->GetState(player->GetGUID(),sLFGMgr->GetQueueId(player->GetGUID()))==lfg::LFG_STATE_NONE &&
        player->GetScenarioId()==route.scenario)player->SetScenarioId(previousScenario);
    return true;
}
void Continue(Player* player, ObjectGuid npcGuid, uint64 generation, uint32 remaining)
{
    auto found = pending.find(player->GetGUID());
    if (found == pending.end() || found->second.generation != generation) return;
    Creature* npc = ObjectAccessor::GetCreature(*player,npcGuid);
    if (!Eligible(player,npc)) { pending.erase(found); return; }
    if (player->isWatchingMovie())
    {
        if (remaining)
        {
            player->AddQuestDelayedEvent(For(player).quest,500,[player,npcGuid,generation,remaining]() { Continue(player,npcGuid,generation,remaining-1); });
            return;
        }
        // Missing movie acknowledgements must not leave departure locked.
        player->setWatchinMovie(false);
    }
    if (Queue(player,For(player))) queued[player->GetGUID()] = {found->second.lifetime,For(player).dungeon};
    pending.erase(found);
}
void Start(Player* player, Creature* npc)
{
    if (!Eligible(player,npc) || !CanQueue(player,For(player))) return;
    auto found = pending.find(player->GetGUID());
    if (found != pending.end() && !found->second.lifetime.expired()) return;
    // Establish the same lifetime used by the delayed callback wrapper.
    player->AddQuestDelayedEvent(For(player).quest,0,[] { });
    uint64 generation = ++nextGeneration;
    pending[player->GetGUID()] = {player->getQuestStatus(For(player).quest)->ScriptLifetime,generation};
    player->PlayerTalkClass->SendCloseGossip();
    // 228560 is PLAY_MOVIE 483, not an LFG queue or mission completion.
    player->CastSpell(player,player->GetTeam()==HORDE?228560:186528,true);
    ObjectGuid guid = npc->GetGUID();
    player->AddQuestDelayedEvent(For(player).quest,500,[player,guid,generation]() { Continue(player,guid,generation,360); });
}
void Hello(Player* player, Creature* npc)
{
    if (!Eligible(player,npc)) return;
    player->PrepareGossipMenu(npc,npc->GetCreatureTemplate()->GossipMenuId,true);
    player->SendPreparedGossip(npc);
}
bool Select(Player* player, Creature* npc, uint32 menu, uint32 option)
{
    if (!npc || !player) return false;
    bool departure = (npc->GetEntry() == 96644 && npc->GetMapId() == 0 && menu == 19355 && option == 0) ||
        (npc->GetEntry() == 96686 && npc->GetMapId() == 1 && menu == 19342 && option == 0);
    if (!departure) return false;
    // AI receives the packet before Player::OnGossipSelect validates its menu.
    GossipMenu& gossip = player->PlayerTalkClass->GetGossipMenu();
    if (gossip.GetMenuId() != menu || !gossip.GetItem(option)) return true;
    Start(player,npc);
    return true;
}
bool Accept(Player* player, Creature* npc, Quest const* quest)
{
    if (!player || !npc || !quest) return false;
    if (npc->GetEntry() == 96663 && npc->GetMapId() == 0 && quest->GetQuestId() == 39800) { StormheimFleet::Accepted(player); return true; } // Depart with Rogers.
    if (npc->GetEntry() != 96686 || npc->GetMapId() != 1 || quest->GetQuestId() != 39801) return false;
    StormheimFleet::Accepted(player);
    Start(player,npc);
    return true;
}

// Only the opening report is present in the installed scenario population.
// It advances the native first criterion, never the mission's final credit.
struct Report
{
    Creature* me;
    ObjectGuid owner, nathanos;
    std::weak_ptr<uint8> lifetime;
    uint32 timer = 0, line = 0;
    explicit Report(Creature* creature) : me(creature) { }
    Scenario* Progress() const
    {
        if (me->GetMapId() != 1475 || me->GetEntry() != 94419) return nullptr;
        Scenario* progress = sScenarioMgr->GetScenario(me->GetInstanceId());
        return progress && progress->GetScenarioId() == 824 ? progress : nullptr;
    }
    bool Valid(Player* player, Creature* companion) const
    {
        Scenario* progress = Progress();
        return Ready(player) && player->GetTeam() == HORDE && player->GetQuestStatus(39801) == QUEST_STATUS_INCOMPLETE &&
            player->GetInstanceId() == me->GetInstanceId() && progress && progress->GetCurrentStep() == 0 &&
            me->IsAlive() && companion && companion->IsAlive() && companion->GetEntry() == 94420 &&
            companion->GetInstanceId() == me->GetInstanceId() && player->InSamePhase(me) && player->InSamePhase(companion) &&
            player->canSeeOrDetect(me) && player->canSeeOrDetect(companion) && player->IsWithinLOSInMap(me) &&
            player->GetExactDist(me) <= 15.0f && player->GetExactDist(companion) <= 15.0f;
    }
    void Clear() { owner.Clear(); nathanos.Clear(); lifetime.reset(); timer = line = 0; }
    void Start(Player* player)
    {
        if (!owner.IsEmpty() && !lifetime.expired()) return;
        Clear();
        Creature* companion = me->FindNearestCreature(94420,20.0f,true);
        if (!Valid(player,companion) || player->GetExactDist(me) > 6.0f) return;
        player->AddQuestDelayedEvent(39801,0,[] { });
        lifetime = player->getQuestStatus(39801)->ScriptLifetime;
        owner = player->GetGUID(); nathanos = companion->GetGUID();
        Update(0);
    }
    void Update(uint32 diff)
    {
        if (owner.IsEmpty()) return;
        Player* player = ObjectAccessor::FindPlayer(owner);
        Creature* companion = ObjectAccessor::GetCreature(*me,nathanos);
        if (lifetime.expired() || !Valid(player,companion)) { Clear(); return; }
        if (timer > diff) { timer -= diff; return; }
        static uint32 const groups[] = {1,3,1,2,3};
        // Retain the installed text rows' minimum times, including Nathanos'
        // final line; the old 1-second final credit interrupted this report.
        static uint32 const delays[] = {11865,11865,11865,11865,31340};
        if (line < 5)
        {
            Creature* speaker = line == 2 || line == 4 ? companion : me;
            sCreatureTextMgr->SendChat(speaker,groups[line],owner,CHAT_MSG_MONSTER_WHISPER);
            timer = delays[line++];
            return;
        }
        player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,45340);
        Clear();
    }
};
}

struct npc_stormheim_fleet_report : SmartAI
{
    explicit npc_stormheim_fleet_report(Creature* creature) : SmartAI(creature), report(creature) { }
    StormheimDeparture::Report report;
    void sGossipHello(Player* player) override
    {
        if (report.Progress()) { player->PlayerTalkClass->SendCloseGossip(); report.Start(player); }
        else SmartAI::sGossipHello(player);
    }
    void UpdateAI(uint32 diff) override { if (report.Progress()) report.Update(diff); else SmartAI::UpdateAI(diff); }
};
class player_stormheim_departure : public PlayerScript
{
public:
    player_stormheim_departure() : PlayerScript("player_stormheim_departure") { }
    void OnLogin(Player* player) override
    {
        StormheimDeparture::Route const& route = StormheimDeparture::For(player);
        // Recover the old AUTOCOMPLETE status without rewriting rewarded history.
        if (player->GetQuestStatus(route.quest) == QUEST_STATUS_COMPLETE && !player->GetQuestObjectiveData(route.quest,113320))
            player->SetQuestStatus(route.quest,QUEST_STATUS_INCOMPLETE);
    }
    void OnUpdate(Player* player, uint32) override { StormheimDeparture::UpdateQueue(player); }
    void OnLogout(Player* player) override { StormheimDeparture::pending.erase(player->GetGUID()); StormheimDeparture::queued.erase(player->GetGUID()); }
};
void AddSC_stormheim_departure()
{
    RegisterCreatureAI(npc_stormheim_fleet_report);
    new player_stormheim_departure();
}
