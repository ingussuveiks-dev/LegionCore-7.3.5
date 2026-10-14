#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "QuestData.h"
#include "TemporarySummon.h"
#include "MoveSpline.h"
#include "Map.h"
#include "MotionMaster.h"
#include <cmath>
#include <map>
#include <mutex>

namespace StormheimAnotherWay
{
struct Branch { uint32 quest, next, team, guide, spell; };
Branch const branches[] = {{38342,38412,ALLIANCE,91558,205485},{38617,38618,HORDE,92569,205487}};
// Both native quest POIs place98255 at2348,2380. Native VMAP gives the
// highest tower roof335.429, not the nearby300.469 grapple ledge.
constexpr float TowerX = 2348.0f, TowerY = 2380.0f, TowerZ = 335.429f;
constexpr int32 OwnerKey = -38342, ValidKey = 38342;
std::map<ObjectGuid,uint32> timers;
std::mutex timersLock;
bool Scope(Player* player)
{
    return player && player->IsInWorld() && player->IsAlive() && player->GetMapId() == 1220 &&
        player->GetZoneId() == 7541 && player->GetInstanceId() == 0 && !player->IsBeingTeleported();
}
Quest const* Definition(Branch const& branch)
{
    Quest const* quest = sQuestDataStore->GetQuestTemplate(branch.quest);
    if (!quest) return nullptr;
    bool boss = false, tower = false;
    for (QuestObjective const& objective : quest->GetObjectives())
        if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.Amount == 1 && !(objective.Flags & QUEST_OBJECTIVE_FLAG_OPTIONAL))
        {
            boss |= objective.ObjectID == 91240 && objective.StorageIndex == 0;
            tower |= objective.ObjectID == 98255 && objective.StorageIndex == 1 && (objective.Flags & QUEST_OBJECTIVE_FLAG_SEQUENCED);
        }
    return boss && tower ? quest : nullptr;
}
bool Active(Player* player, uint32 quest)
{
    QuestStatus status = player->GetQuestStatus(quest);
    return status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE;
}
bool Story(Player* player, Branch const& branch)
{
    return Scope(player) && player->GetTeam() == branch.team && Definition(branch) &&
        (Active(player, branch.quest) || Active(player, branch.next) ||
            (player->GetQuestRewardStatus(branch.quest) && !player->GetQuestRewardStatus(branch.next)));
}
void Tower(Player* player, Branch const& branch)
{
    if (!Scope(player) || player->GetTeam() != branch.team || !Definition(branch) ||
        player->GetQuestStatus(branch.quest) != QUEST_STATUS_INCOMPLETE ||
        !player->GetQuestObjectiveData(branch.quest, 91240) || player->GetQuestObjectiveData(branch.quest, 98255) ||
        player->GetVehicle() || player->GetTransport() || player->isInFlight() ||
        player->HasUnitMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR | MOVEMENTFLAG_FLYING) ||
        !player->movespline->Finalized() || player->GetExactDist2d(TowerX,TowerY) > 9.0f ||
        std::fabs(player->GetPositionZ()-TowerZ) > 2.0f) return;
    float floor = player->GetMap()->GetHeight(player->GetPositionX(),player->GetPositionY(),player->GetPositionZ()+2.0f,true,5.0f);
    if (floor < TowerZ-2.0f || std::fabs(player->GetPositionZ()-floor) > 1.0f) return;
    Quest const* quest = Definition(branch);
    for (QuestObjective const& objective : quest->GetObjectives())
        if (objective.ObjectID == 98255 && objective.StorageIndex == 1)
        {
            player->SetQuestObjectiveData(quest,&objective,1);
            player->SendQuestUpdateAddCredit(quest,ObjectGuid::Empty,objective,1);
            if (player->CanCompleteQuest(branch.quest)) player->CompleteQuest(branch.quest);
            return;
        }
}
void RepairLegacy(Player* player)
{
    for (Branch const& branch : branches)
        if (player->GetTeam() == branch.team && Definition(branch) && player->GetQuestStatus(branch.quest) == QUEST_STATUS_COMPLETE &&
            (!player->GetQuestObjectiveData(branch.quest,91240) || !player->GetQuestObjectiveData(branch.quest,98255)))
            player->SetQuestStatus(branch.quest,QUEST_STATUS_INCOMPLETE);
}
}

struct npc_stormheim_another_way_guide : SmartAI
{
    explicit npc_stormheim_another_way_guide(Creature* creature) : SmartAI(creature) { }
    ObjectGuid owner;
    StormheimAnotherWay::Branch const* branch = nullptr;
    std::weak_ptr<uint8> token;
    bool scoped = false, welcomed = false;
    Player* Owner() { return ObjectAccessor::GetPlayer(*me,owner); }
    bool Valid()
    {
        Player* player = Owner();
        if (!branch || !StormheimAnotherWay::Story(player,*branch)) return false;
        if (StormheimAnotherWay::Active(player,branch->quest))
        {
            auto status = player->getQuestStatus(branch->quest);
            return status && !token.expired() && status->ScriptLifetime == token.lock();
        }
        return player->GetQuestRewardStatus(branch->quest);
    }
    ObjectGuid GetGUID(int32 key) override { return key == StormheimAnotherWay::OwnerKey && scoped ? owner : SmartAI::GetGUID(key); }
    uint32 GetData(uint32 key) const override
    {
        return key == StormheimAnotherWay::ValidKey && scoped ? const_cast<npc_stormheim_another_way_guide*>(this)->Valid() : SmartAI::GetData(key);
    }
    void IsSummonedBy(Unit* summoner) override
    {
        SmartAI::IsSummonedBy(summoner);
        Player* player = summoner ? summoner->ToPlayer() : nullptr;
        if (!player || !me->ToTempSummon()) return;
        for (auto const& candidate : StormheimAnotherWay::branches)
            if (me->GetEntry() == candidate.guide && StormheimAnotherWay::Story(player,candidate))
            {
                branch = &candidate; owner = player->GetGUID(); scoped = true;
                if (auto status = player->getQuestStatus(branch->quest))
                {
                    if (!status->ScriptLifetime) status->ScriptLifetime = std::make_shared<uint8>();
                    token = status->ScriptLifetime;
                }
                me->AddPlayerInPersonnalVisibilityList(owner);
                me->SetReactState(REACT_PASSIVE);
                return;
            }
    }
    void UpdateAI(uint32 diff) override
    {
        if (scoped && !Valid()) { me->DespawnOrUnsummon(); return; }
        SmartAI::UpdateAI(diff);
    }
    void DoAction(int32 action) override
    {
        if (action == StormheimAnotherWay::ValidKey && scoped && Valid() && !welcomed)
        {
            welcomed = true;
            SmartAI::SetData(1,1); // Keep native98747/98748 talk; never a prerequisite for credit.
            return;
        }
        SmartAI::DoAction(action);
    }
};

namespace StormheimAnotherWay
{
void Guide(Player* player, Branch const& branch)
{
    if (!Story(player,branch)) return;
    auto info = sObjectMgr->GetCreatureTemplate(branch.guide);
    if (!info || sObjectMgr->GetScriptName(info->ScriptID) != "npc_stormheim_another_way_guide") return;
    Creature* guide = nullptr;
    auto candidates = *player->GetSummonList(branch.guide);
    for (ObjectGuid guid : candidates)
        if (Creature* actor = ObjectAccessor::GetCreature(*player,guid))
            if (actor->AI() && actor->AI()->GetGUID(OwnerKey) == player->GetGUID() && actor->AI()->GetData(ValidKey))
                { guide = actor; break; }
    if (!guide)
    {
        player->CastSpell(player,branch.spell,true); // Native91558/92569 personal guardian, retry if creation fails.
        return;
    }
    bool top = player->GetQuestObjectiveData(branch.quest,98255) || player->GetQuestRewardStatus(branch.quest);
    if (top && !player->GetVehicle() && !player->GetTransport() && !player->isInFlight() && player->movespline->Finalized())
    {
        // Native Follow cannot climb a wall jump. Keep the exact owner guide
        // available for the hand-in, without teleporting the player or pet.
        if (!guide->IsWithinDistInMap(player,3.0f))
        {
            guide->GetMotionMaster()->Clear();
            guide->NearTeleportTo(player->GetPositionX(),player->GetPositionY(),player->GetPositionZ(),player->GetOrientation());
            guide->GetMotionMaster()->MoveFollow(player,2.0f,0.0f);
        }
        if (guide->HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_QUESTGIVER) && player->InSamePhase(guide))
            guide->AI()->DoAction(ValidKey);
    }
}
}

class player_stormheim_another_way : public PlayerScript
{
public:
    player_stormheim_another_way() : PlayerScript("player_stormheim_another_way") { }
    void OnLogin(Player* player) override { StormheimAnotherWay::RepairLegacy(player); }
    void OnLogout(Player* player) override
    {
        std::lock_guard<std::mutex> guard(StormheimAnotherWay::timersLock);
        StormheimAnotherWay::timers.erase(player->GetGUID());
    }
    void OnUpdate(Player* player,uint32 diff) override
    {
        bool active = false;
        for (auto const& branch : StormheimAnotherWay::branches)
            if (StormheimAnotherWay::Story(player,branch))
            {
                active = true;
                {
                    std::lock_guard<std::mutex> guard(StormheimAnotherWay::timersLock);
                    uint32& timer = StormheimAnotherWay::timers[player->GetGUID()];
                    if (timer > diff) { timer -= diff; return; }
                    timer = 1000;
                }
                StormheimAnotherWay::Tower(player,branch);
                StormheimAnotherWay::Guide(player,branch);
            }
        if (!active)
        {
            std::lock_guard<std::mutex> guard(StormheimAnotherWay::timersLock);
            StormheimAnotherWay::timers.erase(player->GetGUID());
        }
    }
};
void AddSC_stormheim_another_way()
{
    RegisterCreatureAI(npc_stormheim_another_way_guide);
    new player_stormheim_another_way();
}
