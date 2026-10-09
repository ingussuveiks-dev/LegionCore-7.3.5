// Recovery of the Bloodmaul campaign's quest actors. Quest items and rewards
// continue through the ordinary loot/quest handlers; no quest is auto-rewarded.
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Map.h"
#include "TemporarySummon.h"
#include "GameObject.h"
#include "draenor_campaign_recovery.h"
#include <map>
#include <mutex>

namespace
{
    struct CampaignActors
    {
        uint32 elapsed = 0;
        ObjectGuid giver;
        ObjectGuid receiver;
        ObjectGuid shackle;
    };

    void RemoveActor(Player* player, ObjectGuid& guid)
    {
        if (Creature* actor = ObjectAccessor::GetCreature(*player, guid))
            actor->DespawnOrUnsummon();
        guid.Clear();
    }

    void RemoveShackle(Player* player, ObjectGuid& guid)
    {
        if (GameObject* shackle = ObjectAccessor::GetGameObject(*player, guid))
            shackle->Delete();
        guid.Clear();
    }

    void EnsureActor(Player* player, ObjectGuid& guid, uint32 entry, float x, float y, float z)
    {
        if (Creature* actor = ObjectAccessor::GetCreature(*player, guid))
        {
            if (TempSummon* summon = actor->ToTempSummon())
                if (summon->GetTimer() < 60000)
                    summon->AddDuration(60000 - summon->GetTimer());
            return;
        }

        if (TempSummon* actor = player->SummonCreature(entry, x, y, z, 0.0f,
                TEMPSUMMON_TIMED_DESPAWN, 60000, player->GetGUID()))
        {
            actor->AddPlayerInPersonnalVisibilityList(player->GetGUID());
            actor->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            actor->SetReactState(REACT_PASSIVE);
            guid = actor->GetGUID();
        }
    }
}

class player_bloodmaul_campaign_recovery : public PlayerScript
{
public:
    player_bloodmaul_campaign_recovery() : PlayerScript("player_bloodmaul_campaign_recovery") { }

    void OnLogout(Player* player) override
    {
        std::lock_guard<std::mutex> lock(actorsMutex);
        auto itr = actors.find(player->GetGUID());
        if (itr == actors.end())
            return;
        RemoveActor(player, itr->second.giver);
        RemoveActor(player, itr->second.receiver);
        RemoveShackle(player, itr->second.shackle);
        actors.erase(itr);
    }

    void OnMapChanged(Player* player) override
    {
        std::lock_guard<std::mutex> lock(actorsMutex);
        // Stop refreshing actors outside Draenor; they expire after 60 seconds.
        // Retain their GUIDs so a quick return reuses an actor still in the map.
        auto itr = actors.find(player->GetGUID());
        if (itr != actors.end())
            itr->second.elapsed = 0;
    }

    void OnUpdate(Player* player, uint32 diff) override
    {
        if (player->GetMapId() != 1116 || player->GetTeamId() != TEAM_HORDE)
            return;

        std::lock_guard<std::mutex> lock(actorsMutex);
        auto& state = actors[player->GetGUID()];
        state.elapsed += diff;
        if (state.elapsed < 2000)
            return;
        state.elapsed = 0;

        using namespace BloodmaulCampaign;
        bool atPrisoner = player->GetDistance2d(7591.93f, 4337.79f) < 30.0f;
        bool atMeeting = player->GetDistance2d(7384.0f, 5027.0f) < 70.0f;
        bool geared = player->GetQuestRewardStatus(GearingUp);
        bool seekingRewarded = player->GetQuestRewardStatus(SeekingTruth);
        bool gateRewarded = player->GetQuestRewardStatus(ShadowGate);
        auto seeking = player->GetQuestStatus(SeekingTruth);

        // Objective 272536 is sequenced (0x02), not optional (0x04).
        // The key alone cannot free Bwu'ja: the player must use the shackle.
        if (atPrisoner && player->GetQuestStatus(OutOfTheChains) == QUEST_STATUS_INCOMPLETE &&
            !player->GetQuestObjectiveData(OutOfTheChains, 272536) && player->HasItemCount(110664, 1))
        {
            GameObject* shackle = ObjectAccessor::GetGameObject(*player, state.shackle);
            if (!shackle)
                shackle = player->SummonGameObject(229414, 7591.93f, 4337.79f, 48.5104f,
                    1.64645f, 0.0f, 0.0f, 0.73334f, 0.67986f, 60, player->GetGUID());
            if (shackle)
            {
                shackle->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_INTERACT_COND);
                shackle->SetRespawnTime(60);
                state.shackle = shackle->GetGUID();
            }
        }
        else
            RemoveShackle(player, state.shackle);

        if (ShouldOfferSeeking(geared, seekingRewarded, seeking == QUEST_STATUS_NONE) && atPrisoner)
            EnsureActor(player, state.giver, 78746, 7594.93f, 4337.79f, 48.5104f);
        else
            RemoveActor(player, state.giver);

        if (ShouldProvideReceiver(seeking == QUEST_STATUS_INCOMPLETE || seeking == QUEST_STATUS_COMPLETE,
                seekingRewarded, gateRewarded) && atMeeting)
        {
            float z = player->GetMap()->GetHeight(7384.0f, 5027.0f, player->GetPositionZ() + 20.0f);
            if (z > INVALID_HEIGHT && std::abs(z - player->GetPositionZ()) < 30.0f)
                EnsureActor(player, state.receiver, 78785, 7384.0f, 5027.0f, z);
        }
        else
            RemoveActor(player, state.receiver);

        // These are discovery objectives, not kill objectives. Arrival in the
        // client POI is required; merely accepting or spawning a quest actor
        // never advances the quest. The cave prisoner additionally must exist.
        if (player->IsAlive() && !player->isInFlight())
        {
            if (player->GetQuestStatus(SearchForBwuja) == QUEST_STATUS_INCOMPLETE && atPrisoner)
                if (Creature* prisoner = player->FindNearestCreature(78659, 8.0f))
                    if (player->IsWithinLOSInMap(prisoner))
                        player->KilledMonsterCredit(78060);

            if (seeking == QUEST_STATUS_INCOMPLETE && InsideBorgalPOI(player->GetPositionX(), player->GetPositionY()))
            {
                float z = player->GetMap()->GetHeight(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ() + 2.0f);
                if (AtGround(player->GetPositionZ(), z, INVALID_HEIGHT))
                    player->KilledMonsterCredit(78252);
            }
        }
    }

private:
    // Player updates may run on different map workers.
    std::mutex actorsMutex;
    std::map<ObjectGuid, CampaignActors> actors;
};

class go_bwuja_shackle : public GameObjectScript
{
public:
    go_bwuja_shackle() : GameObjectScript("go_bwuja_shackle") { }

    bool OnGossipHello(Player* player, GameObject* shackle) override
    {
        // Recheck at use time: the key can disappear after the object spawns.
        // Returning false runs the normal GO credit and native spell 159041.
        return shackle->GetOwnerGUID() != player->GetGUID() ||
            player->GetQuestStatus(BloodmaulCampaign::OutOfTheChains) != QUEST_STATUS_INCOMPLETE ||
            !player->HasItemCount(110664, 1);
    }
};

void AddSC_draenor_campaign_recovery()
{
    new player_bloodmaul_campaign_recovery();
    new go_bwuja_shackle();
}
