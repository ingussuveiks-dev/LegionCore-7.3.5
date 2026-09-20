/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "AchievementMgr.h"
#include "GameTime.h"
#include "InstanceScript.h"
#include "Player.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "ScriptedCreature.h"
#include "celestial_tournament.h"

#include <array>
#include <map>
#include <set>

namespace
{
    constexpr uint32 MapCelestialTournament = 1161;

    std::array<std::array<uint32, 3>, 3> const WeeklyChampionGroups =
    {{
        {{ NPC_LOREWALKER_CHO, NPC_DR_ION_GOLDBLOOM, NPC_SULLY_MCLEARY }},
        {{ NPC_TARAN_ZHU, NPC_CHEN_STORMSTOUT, NPC_WRATHION }},
        {{ NPC_WISE_MARI, NPC_BLINGTRON_4000, NPC_SHADEMASTER_KIRYN }},
    }};

    std::array<uint32, 4> const CelestialOpponents =
    {{
        NPC_XU_FU,
        NPC_YULA,
        NPC_ZAO,
        NPC_CHI_CHI,
    }};

    bool IsChampion(uint32 entry)
    {
        return entry >= NPC_WRATHION && entry <= NPC_DR_ION_GOLDBLOOM && entry != 71925 && entry != 71928;
    }

    bool IsCelestial(uint32 entry)
    {
        for (uint32 celestial : CelestialOpponents)
            if (celestial == entry)
                return true;

        return false;
    }
}

class instance_celestial_tournament : public InstanceMapScript
{
public:
    instance_celestial_tournament() : InstanceMapScript("instance_celestial_tournament", MapCelestialTournament) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_celestial_tournament_InstanceMapScript(map);
    }

    struct instance_celestial_tournament_InstanceMapScript : public InstanceScript
    {
        instance_celestial_tournament_InstanceMapScript(InstanceMap* map) : InstanceScript(map) { }

        uint8 weeklyGroup = 0;
        uint8 championsDefeated = 0;
        uint8 celestialsDefeated = 0;
        bool tournamentComplete = false;
        ObjectGuid ownerGuid;
        std::map<uint32, ObjectGuid> opponentGuids;
        std::set<uint32> defeatedOpponents;

        Scenario* GetScenario() const
        {
            return sScenarioMgr->GetScenario(instance->GetInstanceId());
        }

        bool IsWeeklyChampion(uint32 entry) const
        {
            for (uint32 champion : WeeklyChampionGroups[weeklyGroup])
                if (champion == entry)
                    return true;

            return false;
        }

        void UpdateOpponentVisibility(uint32 entry)
        {
            auto itr = opponentGuids.find(entry);
            if (itr == opponentGuids.end())
                return;

            if (Creature* opponent = instance->GetCreature(itr->second))
            {
                bool visible = false;
                if (!tournamentComplete && defeatedOpponents.find(entry) == defeatedOpponents.end())
                    visible = IsChampion(entry) ? championsDefeated < 3 && IsWeeklyChampion(entry) : championsDefeated >= 3;

                opponent->SetVisible(visible);
            }
        }

        void UpdateAllOpponentVisibility()
        {
            for (auto const& opponent : opponentGuids)
                UpdateOpponentVisibility(opponent.first);
        }

        void SendScenarioCriteria(uint32 treeId, uint64 counter)
        {
            Scenario* scenario = GetScenario();
            CriteriaTree const* tree = sAchievementMgr->GetCriteriaTree(treeId);
            if (!scenario || !tree || !tree->Entry)
                return;

            CriteriaProgress progress;
            progress.Counter = counter;
            progress.date = time(nullptr);
            progress.PlayerGUID = ownerGuid;
            progress.criteriaTree = tree->Entry;
            scenario->SendCriteriaUpdate(&progress);
        }

        void CompleteTournament(Player* player)
        {
            if (tournamentComplete)
                return;

            tournamentComplete = true;
            player->KilledMonsterCredit(NPC_TOURNAMENT_KILL_CREDIT);

            if (Scenario* scenario = GetScenario())
                scenario->Reward(false, scenario->GetCurrentStep());

            UpdateAllOpponentVisibility();
        }

        void Initialize() override
        {
            // The retail rotation is three fixed trios which change weekly.
            weeklyGroup = uint8((GameTime::GetGameTime() / WEEK) % WeeklyChampionGroups.size());
            championsDefeated = 0;
            celestialsDefeated = 0;
            tournamentComplete = false;
            ownerGuid.Clear();
            opponentGuids.clear();
            defeatedOpponents.clear();
        }

        void OnPlayerEnter(Player* player) override
        {
            if (ownerGuid.IsEmpty())
                ownerGuid = player->GetGUID();

            player->CastSpell(player, SPELL_BATTLE_PET_CHALLENGE, true);

            if (Scenario* scenario = GetScenario())
                scenario->SetCurrentStep(championsDefeated < 3 ? 0 : 1);

            UpdateAllOpponentVisibility();
        }

        void OnPlayerLeave(Player* player) override
        {
            player->RemoveAurasDueToSpell(SPELL_BATTLE_PET_CHALLENGE);
        }

        void OnCreatureCreate(Creature* creature) override
        {
            uint32 entry = creature->GetEntry();
            if (!IsChampion(entry) && !IsCelestial(entry))
                return;

            opponentGuids[entry] = creature->GetGUID();
            UpdateOpponentVisibility(entry);
        }

        void SetData(uint32 type, uint32 data) override
        {
            switch (type)
            {
                case DATA_CHAMPIONS_DEFEATED:
                    championsDefeated = uint8(data);
                    break;
                case DATA_CELESTIALS_DEFEATED:
                    celestialsDefeated = uint8(data);
                    break;
                case DATA_TOURNAMENT_COMPLETE:
                    tournamentComplete = data != 0;
                    break;
                default:
                    break;
            }
        }

        void SetGuidData(uint32 type, ObjectGuid guid) override
        {
            if (tournamentComplete || (!IsChampion(type) && !IsCelestial(type)))
                return;

            Player* player = ObjectAccessor::FindPlayer(guid);
            if (!player || player->GetMap() != instance || player->GetGUID() != ownerGuid || defeatedOpponents.find(type) != defeatedOpponents.end())
                return;

            if (IsChampion(type))
            {
                if (championsDefeated >= 3 || !IsWeeklyChampion(type))
                    return;

                defeatedOpponents.insert(type);
                ++championsDefeated;
                SendScenarioCriteria(CRITERIA_TREE_CHAMPIONS, championsDefeated);

                if (championsDefeated == 3)
                {
                    if (Scenario* scenario = GetScenario())
                        scenario->SetCurrentStep(1);

                    UpdateAllOpponentVisibility();
                }
                else
                    UpdateOpponentVisibility(type);

                return;
            }

            if (championsDefeated < 3)
                return;

            defeatedOpponents.insert(type);
            ++celestialsDefeated;

            switch (type)
            {
                case NPC_XU_FU:   SendScenarioCriteria(CRITERIA_TREE_XU_FU, 1); break;
                case NPC_YULA:    SendScenarioCriteria(CRITERIA_TREE_YULA, 1); break;
                case NPC_ZAO:     SendScenarioCriteria(CRITERIA_TREE_ZAO, 1); break;
                case NPC_CHI_CHI: SendScenarioCriteria(CRITERIA_TREE_CHI_CHI, 1); break;
                default: break;
            }

            UpdateOpponentVisibility(type);
            if (celestialsDefeated == CelestialOpponents.size())
                CompleteTournament(player);
        }

        ObjectGuid GetGuidData(uint32 type) const override
        {
            auto itr = opponentGuids.find(type);
            if (itr != opponentGuids.end())
                return itr->second;

            return ObjectGuid::Empty;
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_CHAMPIONS_DEFEATED:
                    return championsDefeated;
                case DATA_CELESTIALS_DEFEATED:
                    return celestialsDefeated;
                case DATA_TOURNAMENT_COMPLETE:
                    return tournamentComplete ? 1 : 0;
                default:
                    return 0;
            }
        }
    };
};

void AddSC_instance_celestial_tournament()
{
    new instance_celestial_tournament();
}
