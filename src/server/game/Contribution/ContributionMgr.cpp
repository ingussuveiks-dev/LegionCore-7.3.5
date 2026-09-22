/*
    Created by d7561985@gmail.com
*/
#include "ContributionMgr.h"
#include "ConditionMgr.h"
#include "Creature.h"
#include "DB2Stores.h"
#include "MiscPackets.h"
#include "Player.h"
#include "QuestData.h"
#include "World.h"
#include "WorldSession.h"
#include "WorldStateMgr.h"

#include <algorithm>
#include <limits>
#include <set>

namespace
{
    constexpr uint32 ContributionUpdateInterval = MINUTE * IN_MILLISECONDS;
    constexpr uint32 ContributionTurnInsToComplete = 100;
    constexpr uint32 DestroyedStateDuration = MINUTE;

    ContributionEntry const* GetContributionByOrderIndex(uint32 orderIndex)
    {
        for (ContributionEntry const* contribution : sContributionStore)
            if (contribution->OrderIndex == int32(orderIndex))
                return contribution;

        return nullptr;
    }

    bool IsCollectorForContribution(Creature const* collector, uint32 contributionID)
    {
        if (!collector)
            return false;

        for (CreatureXContributionEntry const* relation : sCreatureXContributionStore)
            if (relation->CreatureId == collector->GetEntry() && relation->ContributionID == int32(contributionID))
                return true;

        return false;
    }

    ManagedWorldStateInputEntry const* GetManagedInput(ContributionEntry const* contribution)
    {
        if (!contribution || contribution->ManagedWorldStateInputID <= 0)
            return nullptr;

        return sManagedWorldStateInputStore.LookupEntry(uint32(contribution->ManagedWorldStateInputID));
    }

    uint32 GetPersonalTracker(uint32 contributionID)
    {
        switch (contributionID)
        {
            case 1: // Mage Tower
                return CURRENCY_TYPE_LEGIONFALL_PERSONAL_TRACKER_MAGE_TOWER;
            case 3: // Command Center
                return CURRENCY_TYPE_LEGIONFALL_PERSONAL_TRACKER_COMMAND_TOWER;
            case 4: // Nether Disruptor
                return CURRENCY_TYPE_LEGIONFALL_PERSONAL_TRACKER_NETHER_TOWER;
            default:
                return 0;
        }
    }
}

ContributionMgr& ContributionMgr::Instance()
{
    static ContributionMgr instance;
    return instance;
}

ContributionMgr::ContributionMgr() : _nextUpdate(0)
{
}

void ContributionMgr::Update(uint32 diff)
{
    if (_contributionObjects.empty())
        return;

    _nextUpdate += diff;
    if (_nextUpdate < ContributionUpdateInterval)
        return;

    uint32 elapsedSeconds = _nextUpdate / IN_MILLISECONDS;
    _nextUpdate %= ContributionUpdateInterval;

    for (auto& contribution : _contributionObjects)
        UpdateTimedState(contribution.first, contribution.second, elapsedSeconds);
}

void ContributionMgr::Initialize()
{
    _contributionObjects.clear();
    _nextUpdate = 0;

    std::set<uint32> managedWorldStates;
    for (ContributionEntry const* contribution : sContributionStore)
        if (ManagedWorldStateInputEntry const* input = GetManagedInput(contribution))
            if (input->ManagedWorldStateID > 0)
                managedWorldStates.insert(uint32(input->ManagedWorldStateID));

    for (uint32 managedWorldStateID : managedWorldStates)
    {
        ManagedWorldStateEntry const* managedState = sManagedWorldStateStore.LookupEntry(managedWorldStateID);
        if (!managedState)
            continue;

        sWorldStateMgr.AddTemplate(managedState->CurrentStageWorldStateID, WorldStatesData::Types::World, 0,
            1 << WorldStatesData::Flags::InitialState, ContributionData::CONTRIBUTION_STATE_BUILDING);
        sWorldStateMgr.AddTemplate(managedState->ProgressWorldStateID, WorldStatesData::Types::World, 0,
            1 << WorldStatesData::Flags::InitialState, 0);
        sWorldStateMgr.AddTemplate(managedState->OccurrencesWorldStateID, WorldStatesData::Types::World, 0,
            1 << WorldStatesData::Flags::InitialState, 1);

        uint32 stateValue = sWorldStateMgr.GetWorldStateValue(managedState->CurrentStageWorldStateID);
        if (stateValue < ContributionData::CONTRIBUTION_STATE_BUILDING || stateValue > ContributionData::CONTRIBUTION_STATE_DESTROYED)
            stateValue = ContributionData::CONTRIBUTION_STATE_BUILDING;

        uint32 progressValue = sWorldStateMgr.GetWorldStateValue(managedState->ProgressWorldStateID);
        if (progressValue == std::numeric_limits<uint32>::max())
            progressValue = 0;

        uint32 occurrenceValue = sWorldStateMgr.GetWorldStateValue(managedState->OccurrencesWorldStateID);
        if (occurrenceValue == std::numeric_limits<uint32>::max() || !occurrenceValue)
            occurrenceValue = 1;

        ContributionLifeData& data = _contributionObjects[managedWorldStateID];
        data.WorldStateVariables[0] = managedState->CurrentStageWorldStateID;
        data.WorldStateVariables[1] = managedState->ProgressWorldStateID;
        data.WorldStateVariables[2] = managedState->OccurrencesWorldStateID;
        data.State = ContributionData::ContributionState(stateValue);
        data.UpTimeSecs = uint32(std::max(managedState->UpTimeSecs, 0));
        data.DownTimeSecs = uint32(std::max(managedState->DownTimeSecs, 0));
        data.AccumulationTargetValue = uint32(std::max(managedState->AccumulationStateTargetValue, 1));
        data.AccumulationAmountPerMinute = uint32(std::max(managedState->AccumulationAmountPerMinute, 0));

        switch (data.State)
        {
            case ContributionData::CONTRIBUTION_STATE_ACTIVE:
                data.RemainingTimeSecs = progressValue && progressValue <= data.UpTimeSecs ? progressValue : data.UpTimeSecs;
                progressValue = data.RemainingTimeSecs;
                break;
            case ContributionData::CONTRIBUTION_STATE_UNDERATTACK:
                data.RemainingTimeSecs = progressValue && progressValue <= data.DownTimeSecs ? progressValue : data.DownTimeSecs;
                progressValue = data.RemainingTimeSecs;
                break;
            case ContributionData::CONTRIBUTION_STATE_DESTROYED:
                data.RemainingTimeSecs = progressValue && progressValue <= DestroyedStateDuration ? progressValue : DestroyedStateDuration;
                progressValue = data.RemainingTimeSecs;
                break;
            case ContributionData::CONTRIBUTION_STATE_BUILDING:
            default:
                progressValue = std::min(progressValue, data.AccumulationTargetValue);
                break;
        }

        sWorldStateMgr.SetWorldState(data.WorldStateVariables[0], 0, data.State);
        sWorldStateMgr.SetWorldState(data.WorldStateVariables[1], 0, progressValue);
        sWorldStateMgr.SetWorldState(data.WorldStateVariables[2], 0, occurrenceValue);
    }
}

void ContributionMgr::UpdateTimedState(uint32 managedWorldStateID, ContributionLifeData& data, uint32 elapsedSeconds)
{
    switch (data.State)
    {
        case ContributionData::CONTRIBUTION_STATE_BUILDING:
        {
            if (!data.AccumulationAmountPerMinute)
                return;

            uint64 progress = sWorldStateMgr.GetWorldStateValue(data.WorldStateVariables[1]);
            progress += uint64(data.AccumulationAmountPerMinute) * elapsedSeconds / MINUTE;
            progress = std::min<uint64>(progress, data.AccumulationTargetValue);
            sWorldStateMgr.SetWorldState(data.WorldStateVariables[1], 0, uint32(progress));
            if (progress >= data.AccumulationTargetValue)
                ChangeContributionState(managedWorldStateID, ContributionData::CONTRIBUTION_STATE_ACTIVE);
            return;
        }
        case ContributionData::CONTRIBUTION_STATE_ACTIVE:
        case ContributionData::CONTRIBUTION_STATE_UNDERATTACK:
        case ContributionData::CONTRIBUTION_STATE_DESTROYED:
            break;
        default:
            return;
    }

    if (elapsedSeconds >= data.RemainingTimeSecs)
        data.RemainingTimeSecs = 0;
    else
        data.RemainingTimeSecs -= elapsedSeconds;

    sWorldStateMgr.SetWorldState(data.WorldStateVariables[1], 0, data.RemainingTimeSecs);
    if (data.RemainingTimeSecs)
        return;

    switch (data.State)
    {
        case ContributionData::CONTRIBUTION_STATE_ACTIVE:
            ChangeContributionState(managedWorldStateID, ContributionData::CONTRIBUTION_STATE_UNDERATTACK);
            break;
        case ContributionData::CONTRIBUTION_STATE_UNDERATTACK:
            ChangeContributionState(managedWorldStateID, ContributionData::CONTRIBUTION_STATE_DESTROYED);
            break;
        case ContributionData::CONTRIBUTION_STATE_DESTROYED:
            AdvanceOccurrence(data);
            ChangeContributionState(managedWorldStateID, ContributionData::CONTRIBUTION_STATE_BUILDING);
            break;
        default:
            break;
    }
}

void ContributionMgr::ChangeContributionState(uint32 managedWorldStateID, ContributionData::ContributionState newState)
{
    auto itr = _contributionObjects.find(managedWorldStateID);
    if (itr == _contributionObjects.end())
        return;

    ContributionLifeData& data = itr->second;
    data.State = newState;

    switch (newState)
    {
        case ContributionData::CONTRIBUTION_STATE_BUILDING:
            data.RemainingTimeSecs = 0;
            break;
        case ContributionData::CONTRIBUTION_STATE_ACTIVE:
            data.RemainingTimeSecs = data.UpTimeSecs;
            break;
        case ContributionData::CONTRIBUTION_STATE_UNDERATTACK:
            data.RemainingTimeSecs = data.DownTimeSecs;
            break;
        case ContributionData::CONTRIBUTION_STATE_DESTROYED:
            data.RemainingTimeSecs = DestroyedStateDuration;
            break;
        default:
            data.RemainingTimeSecs = 0;
            break;
    }

    sWorldStateMgr.SetWorldState(data.WorldStateVariables[0], 0, newState);
    sWorldStateMgr.SetWorldState(data.WorldStateVariables[1], 0, data.RemainingTimeSecs);
    RefreshPlayerBuffs();
}

void ContributionMgr::AdvanceOccurrence(ContributionLifeData const& data)
{
    uint32 currentOccurrence = sWorldStateMgr.GetWorldStateValue(data.WorldStateVariables[2]);
    if (currentOccurrence == std::numeric_limits<uint32>::max())
        currentOccurrence = 0;

    uint32 maxOccurrence = 0;
    for (ManagedWorldStateBuffEntry const* buff : sManagedWorldStateBuffStore)
    {
        if (buff->ManagedWorldStateID <= 0 || !buff->OccurrenceValue)
            continue;

        ManagedWorldStateEntry const* managedState = sManagedWorldStateStore.LookupEntry(uint32(buff->ManagedWorldStateID));
        if (managedState && managedState->OccurrencesWorldStateID == int32(data.WorldStateVariables[2]))
            maxOccurrence = std::max(maxOccurrence, buff->OccurrenceValue);
    }

    uint32 nextOccurrence = currentOccurrence + 1;
    if (maxOccurrence && nextOccurrence > maxOccurrence)
        nextOccurrence = 1;

    sWorldStateMgr.SetWorldState(data.WorldStateVariables[2], 0, nextOccurrence);
}

void ContributionMgr::Contribute(Player* player, Creature const* collector, uint32 orderIndex)
{
    if (!player)
        return;

    ContributionEntry const* contribution = GetContributionByOrderIndex(orderIndex);
    uint32 contributionID = contribution ? uint32(contribution->ID) : 0;
    uint32 contributionGuid = collector ? collector->GetGUIDLow() : 0;

    if (!contribution)
    {
        SendContributionResult(player, contributionID, contributionGuid, ContributionData::CONTRIBUTION_RESULT_INVALID_ID);
        return;
    }

    if (!IsCollectorForContribution(collector, contributionID))
    {
        SendContributionResult(player, contributionID, contributionGuid, ContributionData::CONTRIBUTION_RESULT_MUST_BE_NEAR);
        return;
    }

    ManagedWorldStateInputEntry const* input = GetManagedInput(contribution);
    if (!input || input->ManagedWorldStateID <= 0)
    {
        SendContributionResult(player, contributionID, contributionGuid, ContributionData::CONTRIBUTION_RESULT_QUEST_DATA_MISSING);
        return;
    }

    auto stateItr = _contributionObjects.find(uint32(input->ManagedWorldStateID));
    if (stateItr == _contributionObjects.end())
    {
        SendContributionResult(player, contributionID, contributionGuid, ContributionData::CONTRIBUTION_RESULT_INTERNAL_ERROR);
        return;
    }

    ContributionLifeData& state = stateItr->second;
    if (state.State != ContributionData::CONTRIBUTION_STATE_BUILDING)
    {
        SendContributionResult(player, contributionID, contributionGuid, ContributionData::CONTRIBUTION_RESULT_INCORRECT_STATE);
        return;
    }

    if (input->ValidInputConditionID && !sConditionMgr->IsPlayerMeetingCondition(player, input->ValidInputConditionID, false))
    {
        SendContributionResult(player, contributionID, contributionGuid, ContributionData::CONTRIBUTION_RESULT_FAILED_CONDITION_CHECK);
        return;
    }

    Quest const* rewardQuest = input->QuestID > 0 ? sQuestDataStore->GetQuestTemplate(uint32(input->QuestID)) : nullptr;
    if (!rewardQuest)
    {
        SendContributionResult(player, contributionID, contributionGuid, ContributionData::CONTRIBUTION_RESULT_QUEST_DATA_MISSING);
        return;
    }

    if (!player->CanRewardQuest(rewardQuest, 0, false))
    {
        SendContributionResult(player, contributionID, contributionGuid, ContributionData::CONTRIBUTION_RESULT_UNABLE_TO_COMPLETE_TURN_IN);
        return;
    }

    // The hidden reward quest owns both the 100 War Supplies cost and all contribution rewards.
    // Keeping that transaction in one place prevents double deductions and partial rewards.
    player->RewardQuest(rewardQuest, 0, player, true);

    if (uint32 trackerCurrency = GetPersonalTracker(contributionID))
        player->ModifyCurrency(trackerCurrency, 1, false, true);

    uint32 progress = sWorldStateMgr.GetWorldStateValue(state.WorldStateVariables[1]);
    if (progress == std::numeric_limits<uint32>::max())
        progress = 0;

    uint32 progressPerTurnIn = std::max<uint32>(1, (state.AccumulationTargetValue + ContributionTurnInsToComplete - 1) / ContributionTurnInsToComplete);
    uint64 newProgress = std::min<uint64>(uint64(progress) + progressPerTurnIn, state.AccumulationTargetValue);
    sWorldStateMgr.SetWorldState(state.WorldStateVariables[1], 0, uint32(newProgress));

    if (newProgress >= state.AccumulationTargetValue)
        ChangeContributionState(uint32(input->ManagedWorldStateID), ContributionData::CONTRIBUTION_STATE_ACTIVE);

    SendContributionResult(player, contributionID, contributionGuid, ContributionData::CONTRIBUTION_RESULT_SUCCESS);
}

void ContributionMgr::ContributionGetState(Player* player, uint32 contributionID, uint32 contributionGuid)
{
    ContributionData::ContributionResult result = sContributionStore.LookupEntry(contributionID)
        ? ContributionData::CONTRIBUTION_RESULT_SUCCESS
        : ContributionData::CONTRIBUTION_RESULT_INVALID_ID;
    SendContributionResult(player, contributionID, contributionGuid, result);
}

void ContributionMgr::SendContributionResult(Player* player, uint32 contributionID, uint32 contributionGuid, ContributionData::ContributionResult result) const
{
    if (!player)
        return;

    WorldPackets::Misc::ContributionResponse response;
    response.Data = result;
    response.ContributionID = contributionID;
    response.ContributionGUID = contributionGuid;
    player->SendDirectMessage(response.Write());
}

void ContributionMgr::UpdatePlayerBuffs(Player* player) const
{
    if (!player)
        return;

    std::map<uint32, bool> managedBuffs;
    for (ManagedWorldStateBuffEntry const* buff : sManagedWorldStateBuffStore)
    {
        if (buff->ManagedWorldStateID <= 0 || buff->BuffSpellID <= 0)
            continue;

        auto stateItr = _contributionObjects.find(uint32(buff->ManagedWorldStateID));
        if (stateItr == _contributionObjects.end())
            continue;

        bool& shouldHaveBuff = managedBuffs[uint32(buff->BuffSpellID)];
        if (shouldHaveBuff || stateItr->second.State != ContributionData::CONTRIBUTION_STATE_ACTIVE)
            continue;

        uint32 occurrence = sWorldStateMgr.GetWorldStateValue(stateItr->second.WorldStateVariables[2]);
        if (buff->OccurrenceValue && buff->OccurrenceValue != occurrence)
            continue;

        if (buff->PlayerConditionID && !sConditionMgr->IsPlayerMeetingCondition(player, buff->PlayerConditionID, false))
            continue;

        shouldHaveBuff = true;
    }

    for (auto const& managedBuff : managedBuffs)
    {
        if (managedBuff.second)
        {
            if (!player->HasAura(managedBuff.first))
                player->CastSpell(player, managedBuff.first, true);
        }
        else
            player->RemoveAurasDueToSpell(managedBuff.first);
    }
}

void ContributionMgr::RefreshPlayerBuffs() const
{
    for (auto const& session : sWorld->GetAllSessions())
        if (session.second && session.second->GetPlayer())
            UpdatePlayerBuffs(session.second->GetPlayer());
}
