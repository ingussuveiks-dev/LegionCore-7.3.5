#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "CreatureTextMgr.h"
#include <map>
#include <mutex>

namespace Audience
{
constexpr uint32 QuestId = 39374;
constexpr uint32 ActorTag = 39374;
// Existing Oakin spawn positions. The installed ground navmesh connects them.
Position const Start = {5221.07f,4902.25f,711.837f,2.49574f};
Position const Destination = {5335.47f,4919.65f,691.969f,0.876705f};
bool Wanted(Player* player)
{
    if (!player || !player->IsAlive() || player->GetMapId() != 1220 ||
        player->IsBeingTeleported() || player->isInFlight() || player->IsFlying() || player->GetVehicle()) return false;
    auto status = player->GetQuestStatus(QuestId);
    return (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE) &&
        (player->GetExactDist(&Start) < 80.0f || player->GetExactDist(&Destination) < 80.0f);
}
Player* Owner(Creature* npc)
{
    TempSummon* summon = npc->ToTempSummon();
    Unit* owner = summon ? summon->GetSummoner() : nullptr;
    return owner ? owner->ToPlayer() : nullptr;
}
Creature* Existing(Player* player, bool clear = false)
{
    auto guids = *player->GetSummonList(95256);
    for (ObjectGuid const& guid : guids)
        if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
            if (Owner(npc) == player && npc->AI()->GetData(ActorTag))
            {
                if (clear) npc->DespawnOrUnsummon();
                else if (npc->IsAlive()) return npc;
            }
    return nullptr;
}
void Credit(Player* player, ObjectGuid guid)
{
    if (!Wanted(player) || player->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE) return;
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.ObjectID == 95256)
            {
                player->SetQuestObjectiveData(quest,&objective,1);
                player->SendQuestUpdateAddCredit(quest,guid,objective,1);
                if (player->CanCompleteQuest(QuestId)) player->CompleteQuest(QuestId);
                return;
            }
}
}

struct npc_audience_oakin : SmartAI
{
    npc_audience_oakin(Creature* creature) : SmartAI(creature) { }
    bool audience = false, moving = false, arrived = false;
    uint32 travel = 0, dialogue = 0;
    ObjectGuid guardGuid;
    std::weak_ptr<uint8> lifetime;

    uint32 GetData(uint32 id) const override
    { return id == Audience::ActorTag ? uint32(audience) : SmartAI::GetData(id); }

    void IsSummonedBy(Unit* summoner) override
    {
        Player* player = summoner->ToPlayer();
        if (!Audience::Wanted(player)) { SmartAI::IsSummonedBy(summoner); return; }
        audience = true;
        player->AddQuestDelayedEvent(Audience::QuestId,0,[] { });
        lifetime = player->getQuestStatus(Audience::QuestId)->ScriptLifetime;
        me->AddPlayerInPersonnalVisibilityList(player->GetGUID());
        me->SetReactState(REACT_PASSIVE);
        me->SetWalk(true);
        // Do not run the generic JUST_SUMMONED path: that is the subsequent
        // cave quest's path and would take this escort past the audience.
        if (player->GetQuestStatus(Audience::QuestId) == QUEST_STATUS_COMPLETE) arrived = true;
        else
        {
            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_QUESTGIVER);
            Talk(0,player->GetGUID());
        }
    }
    void MovementInform(uint32 type, uint32 id) override
    {
        if (!audience) { SmartAI::MovementInform(type,id); return; }
        if (moving && type == POINT_MOTION_TYPE && id == 1 && me->GetExactDist(&Audience::Destination) < 3.0f)
        { moving = false; arrived = true; }
    }
    void sQuestAccept(Player* player, Quest const* quest) override
    {
        // Keep the existing cave acceptance timeline and summoned waypoint AI.
        // This actor now belongs to that timeline, not the audience cleanup.
        if (audience && quest->GetQuestId() == 39455 && Audience::Owner(me) == player) audience = false;
        SmartAI::sQuestAccept(player,quest);
    }
    void sQuestReward(Player* player, Quest const* quest, uint32 opt) override
    {
        if (audience && quest->GetQuestId() == Audience::QuestId && Audience::Owner(me) == player)
        {
            // Keep this questgiver long enough to accept the follow-up after
            // reward removes the audience's quest lifetime and recovery state.
            audience = false;
            me->DespawnOrUnsummon(60000);
        }
        SmartAI::sQuestReward(player,quest,opt);
    }
    void UpdateAI(uint32 diff) override
    {
        if (!audience) { SmartAI::UpdateAI(diff); return; }
        Player* player = Audience::Owner(me);
        if (lifetime.expired() || !Audience::Wanted(player) || !player->InSamePhase(me) || player->GetDistance(me) > 80.0f)
        { me->DespawnOrUnsummon(); return; }
        if (player->GetQuestStatus(Audience::QuestId) == QUEST_STATUS_COMPLETE)
        { me->SetFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_QUESTGIVER); return; }
        if (!arrived)
        {
            if (moving)
            {
                if (player->GetDistance(me) > 30.0f) { me->GetMotionMaster()->Clear(); moving = false; return; }
                if (travel <= diff) me->DespawnOrUnsummon();
                else travel -= diff;
            }
            else if (player->GetDistance(me) <= 20.0f)
            {
                moving = true; travel = 120000;
                me->GetMotionMaster()->MovePoint(1,Audience::Destination);
            }
            return;
        }
        Creature* guard = dialogue ? ObjectAccessor::GetCreature(*me,guardGuid) : me->FindNearestCreature(98904,10.0f,true);
        if (!guard || !guard->IsAlive() || !player->InSamePhase(guard) || !player->canSeeOrDetect(guard) ||
            player->GetDistance(me) > 15.0f || player->GetDistance(guard) > 15.0f || me->GetDistance(guard) > 10.0f)
        { dialogue = 0; guardGuid.Clear(); return; }
        if (!dialogue)
        {
            Talk(1,player->GetGUID()); guardGuid = guard->GetGUID(); dialogue = 5000;
            return;
        }
        if (dialogue > diff) { dialogue -= diff; return; }
        // Use the existing localized guard line, delivered only to this owner.
        sCreatureTextMgr->SendChat(guard,0,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER);
        Audience::Credit(player,me->GetGUID());
        me->SetFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_QUESTGIVER);
        dialogue = 0;
    }
};

class player_audience_oakin : public PlayerScript
{
    struct State { uint32 timer = 0; std::weak_ptr<uint8> lifetime; };
    std::mutex mutex;
    std::map<ObjectGuid,State> states;
public:
    player_audience_oakin() : PlayerScript("player_audience_oakin") { }
    void OnLogout(Player* player) override
    {
        bool serviced;
        { std::lock_guard<std::mutex> lock(mutex); serviced = states.erase(player->GetGUID()) != 0; }
        if (serviced) Audience::Existing(player,true);
    }
    void OnMapChanged(Player* player) override { OnLogout(player); }
    void OnUpdate(Player* player, uint32 diff) override
    {
        if (!Audience::Wanted(player)) { OnLogout(player); return; }
        bool stale;
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto inserted = states.emplace(player->GetGUID(),State{});
            State& state = inserted.first->second;
            stale = !inserted.second && state.lifetime.expired();
            if (inserted.second || stale)
            {
                player->AddQuestDelayedEvent(Audience::QuestId,0,[] { });
                state = State{}; state.lifetime = player->getQuestStatus(Audience::QuestId)->ScriptLifetime;
            }
            if (state.timer > diff) { state.timer -= diff; return; }
            state.timer = 1000;
        }
        if (stale) Audience::Existing(player,true);
        if (!Audience::Existing(player))
            player->SummonCreature(95256,player->GetPositionX(),player->GetPositionY(),player->GetPositionZ(),player->GetOrientation(),
                TEMPSUMMON_TIMED_DESPAWN,600000,player->GetGUID());
    }
};

void AddSC_highmountain_audience()
{
    RegisterCreatureAI(npc_audience_oakin);
    new player_audience_oakin();
}
