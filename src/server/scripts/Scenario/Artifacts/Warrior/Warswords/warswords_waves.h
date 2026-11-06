#pragma once

#include <array>
#include "DB2Stores.h"

namespace WarswordsWaves
{
    // One authored wave belongs to the instance, not to each player in it.
    struct State
    {
        explicit State(Map* value) : map(value) { }
        Map* map;
        std::array<ObjectGuid, 3> actors{};
        std::array<bool, 3> dead{};
        ObjectGuid leader;
        bool leaderDead = false, active = false, warned = false;
        uint32 wave = 0, retry = 0;

        void Reset()
        {
            actors = {}; dead = {}; leader.Clear();
            leaderDead = active = warned = false;
            wave = retry = 0;
        }

        Scenario* Progress() const
        {
            Scenario* scenario = sScenarioMgr->GetScenario(map->GetInstanceId());
            return map->GetId() == 1511 && !map->IsMapUnload() && scenario &&
                scenario->GetScenarioId() == 944 && scenario->GetCurrentStep() == 1 ? scenario : nullptr;
        }

        uint64 Counter(uint32 id) const
        {
            Scenario* scenario = Progress();
            if (!scenario)
                return 0;
            auto const* records = scenario->GetAchievementMgr().GetCriteriaProgressMap();
            uint64 counter = 0;
            // Scenario progress is keyed by CriteriaTree.ID, not Criteria.ID.
            for (auto const& record : *records)
                if (CriteriaTreeEntry const* tree = sCriteriaTreeStore.LookupEntry(record.first))
                    if (tree->CriteriaID == id)
                        counter = std::max(counter, record.second.Counter);
            return counter;
        }

        Player* Participant() const
        {
            for (auto const& reference : map->GetPlayers())
                if (Player* player = reference.getSource())
                    if (WarswordsArrival::IsParticipant(player, map->GetInstanceId(), player->GetInstanceScript()) &&
                        !player->isGameMaster())
                        return player;
            return nullptr;
        }

        void Start()
        {
            if (active || !Progress())
                return;
            active = true;
            wave = uint32(std::min(uint64(3), Counter(29100)));
            Update(0);
        }

        void Death(Unit* unit)
        {
            if (!active || !Progress() || !unit || unit->IsAlive() || unit->GetMap() != map)
                return;
            // Full GUID membership, independent of killer/AI, includes pets,
            // guardians and native one-hit deaths. Removal is not a death.
            if (wave < 3)
                for (uint32 slot = 0; slot < actors.size(); ++slot)
                    if (!actors[slot].IsEmpty() && actors[slot] == unit->GetGUID())
                        dead[slot] = true;
            if (!leader.IsEmpty() && leader == unit->GetGUID())
                leaderDead = true;
            retry = 0;
            Update(0);
        }

        void Update(uint32 diff)
        {
            if (!active || !Progress())
                return;
            if (retry > diff) { retry -= diff; return; }
            retry = 1000;
            Player* player = Participant();
            if (!player)
                return;

            if (wave < 3 && dead[0] && dead[1] && dead[2])
            {
                if (Counter(29100) < wave + 1)
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 47289);
                if (!Progress() || Counter(29100) < wave + 1)
                    return; // Keep the real three-death receipt until native ACK.
                ++wave;
                actors = {}; dead = {};
            }
            if (wave < 3)
            {
                for (uint32 slot = 0; slot < actors.size(); ++slot)
                {
                    if (dead[slot])
                        continue;
                    Creature* actor = map->GetCreature(actors[slot]);
                    if (actor && actor->IsInWorld())
                        continue;
                    uint32 entry = slot ? 98069 : wave == 2 ? 97994 : 97992;
                    int32 spread = slot ? 7 : 5;
                    actor = player->SummonCreature(entry, 3427.55f + irand(-spread, spread), 1486.10f + irand(-spread, spread), 57.40f);
                    if (actor)
                    {
                        actors[slot] = actor->GetGUID();
                        if (actor->IsAIEnabled && actor->AI())
                            actor->AI()->AttackStart(player);
                    }
                    else if (!warned)
                    {
                        TC_LOG_WARN("scripts.ai", "Warswords wave: summon failed; retrying missing slot without wave credit");
                        warned = true;
                    }
                }
            }
            else if (leaderDead)
            {
                if (!Counter(29055))
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46804);
                if (!Progress() || Counter(29055))
                {
                    active = false;
                    player->CreateConversation(837);
                }
            }
            else
            {
                Creature* actor = map->GetCreature(leader);
                if (!actor || !actor->IsInWorld())
                    if (Creature* summoned = player->SummonCreature(97649, 3363.17f, 1469.68f, 57.40f))
                        leader = summoned->GetGUID();
            }
        }
    };
}
