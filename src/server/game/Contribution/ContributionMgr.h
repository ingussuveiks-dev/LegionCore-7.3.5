/*
Created by d7561985@gmail.com
*/
#pragma once

#ifndef ContributionMgr_h__
#define ContributionMgr_h__

#include <map>

class Creature;
class Player;

namespace ContributionData
{
    enum ContributionState : uint8
    {
        CONTRIBUTION_STATE_NONE = 0,
        CONTRIBUTION_STATE_BUILDING = 1,
        CONTRIBUTION_STATE_ACTIVE = 2,
        CONTRIBUTION_STATE_UNDERATTACK = 3,
        CONTRIBUTION_STATE_DESTROYED = 4
    };

    enum ContributionResult : uint8
    {
        CONTRIBUTION_RESULT_SUCCESS = 0,
        CONTRIBUTION_RESULT_MUST_BE_NEAR = 1,
        CONTRIBUTION_RESULT_INCORRECT_STATE = 2,
        CONTRIBUTION_RESULT_INVALID_ID = 3,
        CONTRIBUTION_RESULT_QUEST_DATA_MISSING = 4,
        CONTRIBUTION_RESULT_FAILED_CONDITION_CHECK = 5,
        CONTRIBUTION_RESULT_UNABLE_TO_COMPLETE_TURN_IN = 6,
        CONTRIBUTION_RESULT_INTERNAL_ERROR = 7
    };
}

struct ContributionLifeData
{
    uint32 WorldStateVariables[3] = { 0, 0, 0 };
    ContributionData::ContributionState State = ContributionData::CONTRIBUTION_STATE_NONE;
    uint32 UpTimeSecs = 0;
    uint32 DownTimeSecs = 0;
    uint32 RemainingTimeSecs = 0;
    uint32 AccumulationTargetValue = 0;
    uint32 AccumulationAmountPerMinute = 0;
};

class ContributionMgr
{
public:
    static ContributionMgr& Instance();

    ContributionMgr();

    void Update(uint32 diff);

    void Initialize();
    void Contribute(Player* player, Creature const* collector, uint32 orderIndex);
    void ContributionGetState(Player* player, uint32 contributionID, uint32 contributionGuid);
    void UpdatePlayerBuffs(Player* player) const;

private:
    void ChangeContributionState(uint32 managedWorldStateID, ContributionData::ContributionState newState);
    void SendContributionResult(Player* player, uint32 contributionID, uint32 contributionGuid, ContributionData::ContributionResult result) const;
    void UpdateTimedState(uint32 managedWorldStateID, ContributionLifeData& data, uint32 elapsedSeconds);
    void AdvanceOccurrence(ContributionLifeData const& data);
    void RefreshPlayerBuffs() const;

    std::map<uint32, ContributionLifeData> _contributionObjects;
    uint32 _nextUpdate;
};

#define sContributionMgr ContributionMgr::Instance()

#endif // ContributionMgr_h__
