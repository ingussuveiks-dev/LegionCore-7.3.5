#ifndef GARRISON_CAMPAIGN_ACTORS_H
#define GARRISON_CAMPAIGN_ACTORS_H
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "TemporarySummon.h"
#include "GameObject.h"
#include "Map.h"
#include <map>
#include <cmath>
namespace CampaignRecovery
{
inline bool Active(Player* player, uint32 quest)
{
    auto status = player->GetQuestStatus(quest);
    return status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE;
}

inline bool Owned(Creature* creature, Player* player)
{
    TempSummon* summon = creature->ToTempSummon();
    return summon && summon->GetSummonerGUID() == player->GetGUID();
}

struct Actor
{
    ObjectGuid guid;
    bool gameObject = false;
    bool needed = false;
};

struct Encounter
{
    uint32 timer = 0;
    uint32 introRemaining = 0;
    std::map<uint32, Actor> actors;

    void Remove(Player* player, Actor& actor)
    {
        if (actor.gameObject)
        {
            if (GameObject* object = ObjectAccessor::GetGameObject(*player, actor.guid))
                object->Delete();
        }
        else if (Creature* creature = ObjectAccessor::GetCreature(*player, actor.guid))
            creature->DespawnOrUnsummon();
        actor.guid.Clear();
    }

    void Begin()
    {
        for (auto& pair : actors)
            pair.second.needed = false;
    }
    void End(Player* player)
    {
        for (auto it = actors.begin(); it != actors.end();)
            if (!it->second.needed)
            {
                Remove(player, it->second);
                it = actors.erase(it);
            }
            else
                ++it;
    }

    void At(Player* player, uint32 key, uint32 entry, float x, float y, bool object = false, bool hostile = false,
            bool questgiver = false)
    {
        if (!player->IsAlive() || player->GetDistance2d(x, y) > 85.0f || player->isInFlight() || player->IsFlying())
            return;
        float z = player->GetMap()->GetHeight(x, y, player->GetPositionZ() + 15.0f);
        if (z <= INVALID_HEIGHT || std::abs(z - player->GetPositionZ()) > 25.0f)
            return;
        Actor& actor = actors[key];
        actor.gameObject = object;
        actor.needed = true;
        if (object)
        {
            GameObject* go = ObjectAccessor::GetGameObject(*player, actor.guid);
            if (!go)
                go = player->SummonGameObject(entry, x, y, z, 0, 0, 0, 0, 1, 90, player->GetGUID());
            if (go)
            {
                go->SetRespawnTime(90);
                actor.guid = go->GetGUID();
            }
        }
        else
        {
            Creature* creature = ObjectAccessor::GetCreature(*player, actor.guid);
            if (!creature)
            {
                creature =
                    player->SummonCreature(entry, x, y, z, 0, TEMPSUMMON_TIMED_DESPAWN, 90000, player->GetGUID());
                if (creature)
                {
                    actor.guid = creature->GetGUID();
                    creature->SetReactState(hostile ? REACT_AGGRESSIVE : REACT_PASSIVE);
                    if (hostile)
                    {
                        creature->setFaction(14);
                        creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC |
                                                                   UNIT_FLAG_NOT_SELECTABLE);
                    }
                    if (questgiver)
                        creature->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                }
            }
            if (creature)
                if (TempSummon* summon = creature->ToTempSummon())
                    if (summon->GetTimer() < 90000)
                        summon->AddDuration(90000 - summon->GetTimer());
        }
    }
};
} // namespace CampaignRecovery
#endif
