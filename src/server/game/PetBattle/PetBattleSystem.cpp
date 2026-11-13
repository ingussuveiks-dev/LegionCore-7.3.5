
#include "PetBattleSystem.h"
#include "PetBattle.h"
#include "Player.h"
#include "Position.h"
#include "ObjectAccessor.h"
#include "WorldSession.h"
#include "WildBattlePet.h"
#include "MapManager.h"
#include <limits>

#define PETBATTLE_DELETE_INTERVAL (1 * 30 * IN_MILLISECONDS)
#define PETBATTLE_LFB_INTERVAL 500
#define PETBATTLE_LFB_PROPOSAL_TIMEOUT (1 * MINUTE)

namespace
{
    // Synchronous, thread-local ownership: never retain a departing Player in
    // a battle, delayed callback or another session's logout operation.
    thread_local Player* loggingOutPlayer = nullptr;
    thread_local ObjectGuid loggingOutBattle;
}

struct PetBattleMembersPositions
{
    PetBattleMembersPositions(uint32 mapID, uint32 team, Position const& firstPosition, Position const& secondPosition) : MapID(mapID), Team(team)
    {
        Positions[0] = firstPosition;
        Positions[1] = secondPosition;
    }

    Position Positions[2];
    uint32 MapID;
    uint32 Team;
};

PetBattleSystem::PetBattleSystem()
{
    _maxPetBattleID = 1;
    _deleteUpdateTimer.SetInterval(PETBATTLE_DELETE_INTERVAL);
    _LFBAvgWaitTime = 0;
    _LFBNumWaitTimeAvg = 0;
    _LFBRequestsUpdateTimer.SetInterval(PETBATTLE_LFB_INTERVAL);
}

PetBattleSystem::~PetBattleSystem()
{
    for (auto& itr : _petBattles)
        delete itr.second;

    for (auto& itr : _battleRequests)
        delete itr.second;
}

PetBattleSystem* PetBattleSystem::instance()
{
    static PetBattleSystem instance;
    return &instance;
}

PetBattle* PetBattleSystem::CreateBattle()
{
    auto battleID = ObjectGuid::Create<HighGuid::PetBattle>(++_maxPetBattleID);
    _petBattles[battleID] = new PetBattle();
    _petBattles[battleID]->ID = battleID;
    return _petBattles[battleID];
}

PetBattleRequest* PetBattleSystem::CreateRequest(ObjectGuid requesterGuid)
{
    RemoveRequest(requesterGuid);
    _battleRequests[requesterGuid] = new PetBattleRequest();
    _battleRequests[requesterGuid]->RequesterGuid = requesterGuid;
    return _battleRequests[requesterGuid];
}

PetBattle* PetBattleSystem::GetBattle(ObjectGuid battleID)
{
    return Trinity::Containers::MapGetValuePtr(_petBattles, battleID);
}

PetBattleRequest* PetBattleSystem::GetRequest(ObjectGuid requesterGuid)
{
    return Trinity::Containers::MapGetValuePtr(_battleRequests, requesterGuid);
}

void PetBattleSystem::RemoveBattle(ObjectGuid battleID)
{
    if (auto battle = GetBattle(battleID))
    {
        delete battle;

        _petBattles[battleID] = nullptr;
        _petBattles.erase(battleID);
    }
}

void PetBattleSystem::RemoveRequest(ObjectGuid requesterGuid)
{
    if (auto request = GetRequest(requesterGuid))
    {
        delete request;

        _battleRequests[requesterGuid] = nullptr;
        _battleRequests.erase(requesterGuid);
    }
}

eBattlePetRequests PetBattleSystem::CheckQueueEntry(Player* player)
{
    if (!player || !player->IsInWorld())
        return PETBATTLE_REQUEST_CREATE_FAILED;
    if (!player->IsAlive())
        return PETBATTLE_REQUEST_NOT_WHILE_DEAD;
    if (player->_petBattleId)
        return PETBATTLE_REQUEST_IN_BATTLE;
    if (player->isInCombat())
        return PETBATTLE_REQUEST_NOT_WHILE_IN_COMBAT;
    player->UpdateBattlePetCombatTeam();
    auto pets = player->GetBattlePetCombatTeam();
    bool populated = false;
    for (uint32 i = 0; i < MAX_PETBATTLE_SLOTS && i < player->GetUnlockedPetBattleSlot(); ++i)
        if (pets[i])
        {
            populated = true;
            if (pets[i]->Health > 0)
                return PETBATTLE_REQUEST_OK;
        }
    return populated ? PETBATTLE_REQUEST_ALL_PETS_DEAD : PETBATTLE_REQUEST_NO_PETS_IN_SLOT;
}

void PetBattleSystem::JoinQueue(Player* player)
{
    /// Pandaren case
    if (player->GetTeamId() == TEAM_NEUTRAL)
    {
        player->GetSession()->SendPetBattleQueueStatus(0, 1, LFBUpdateStatus::LFB_CANT_JOIN_DUE_TO_UNSELECTED_FACTION, 0);
        return;
    }

    // Load player pets
    auto petSlots = player->GetBattlePetCombatTeam();
    uint32 weight = 0;

    for (size_t i = 0; i < MAX_PETBATTLE_SLOTS; ++i)
        if (petSlots[i])
            weight += petSlots[i]->Level;

    std::lock_guard<std::mutex> l_Lock(_LFBRequestsMutex);

    if (_LFBRequests[player->GetGUID()] != nullptr)
        return;

    auto ticket = new LFBTicket();
    ticket->State = LFBState::LFB_STATE_QUEUED;
    ticket->JoinTime = GameTime::GetGameTime();
    ticket->TicketID = 1;
    ticket->MatchingOpponent = nullptr;
    ticket->ProposalAnswer = LFBAnswer::LFB_ANSWER_PENDING;
    ticket->Weight = weight;
    ticket->RequesterGUID = player->GetGUID();
    ticket->TeamID = player->GetTeamId();

    player->GetSession()->SendBattlePetJournalLockAcquired();
    player->GetSession()->SendPetBattleQueueStatus(ticket->JoinTime, ticket->TicketID, LFBUpdateStatus::LFB_JOIN_QUEUE, _LFBAvgWaitTime);

    _LFBRequests[player->GetGUID()] = ticket;
}

void PetBattleSystem::ProposalResponse(Player* player, bool accepted)
{
    std::lock_guard<std::mutex> l_Lock(_LFBRequestsMutex);

    if (_LFBRequests[player->GetGUID()] == nullptr)
        return;

    auto ticket = _LFBRequests[player->GetGUID()];
    if (ticket->State != LFBState::LFB_STATE_PROPOSAL)
        return;

    ticket->ProposalAnswer = accepted ? LFBAnswer::LFB_ANSWER_AGREE : LFBAnswer::LFB_ANSWER_DENY;
}

void PetBattleSystem::LeaveQueue(Player* player)
{
    std::lock_guard<std::mutex> l_Lock(_LFBRequestsMutex);
    auto ticket = _LFBRequests[player->GetGUID()];
    if (!ticket)
        return;

    switch (ticket->State)
    {
        case LFBState::LFB_STATE_PROPOSAL:
        {
            if (ticket->MatchingOpponent)
            {
                ticket->MatchingOpponent->MatchingOpponent = nullptr;
                ticket->MatchingOpponent->State = LFBState::LFB_STATE_QUEUED;

                if (auto opponent = ObjectAccessor::FindPlayer(ticket->MatchingOpponent->RequesterGUID))
                    opponent->GetSession()->SendPetBattleQueueStatus(ticket->MatchingOpponent->JoinTime, ticket->MatchingOpponent->TicketID, LFBUpdateStatus::LFB_JOIN_QUEUE, _LFBAvgWaitTime);
            }

            /// Continue to other case handlers
        }
        case LFBState::LFB_STATE_FINISHED:
        case LFBState::LFB_STATE_IN_COMBAT:
        case LFBState::LFB_STATE_QUEUED:
        {
            player->GetSession()->SendPetBattleQueueStatus(ticket->JoinTime, ticket->TicketID, LFBUpdateStatus::LFB_LEAVE_QUEUE, _LFBAvgWaitTime);
            player->GetSession()->SendBattlePetJournalLockDenied();
            player->UpdateBattlePetCombatTeam();

            delete _LFBRequests[player->GetGUID()];
            _LFBRequests[player->GetGUID()] = nullptr;

            break;
        }
        default:
            break;
    }
}

void PetBattleSystem::Update(uint32 diff)
{
    _deleteUpdateTimer.Update(diff);
    _LFBRequestsUpdateTimer.Update(diff);

    if (_deleteUpdateTimer.Passed())
    {
        _deleteUpdateTimer.Reset();

        while (!_petBattlesDeleteQueue.empty())
        {
            auto& front = _petBattlesDeleteQueue.front();
            RemoveBattle(front.first);

            _petBattlesDeleteQueue.pop();
        }
    }

    for (auto& itr : _petBattles)
    {
        auto battle = itr.second;
        if (!battle)
            continue;

        if (battle->BattleStatus == PETBATTLE_STATUS_RUNNING)
            battle->Update(diff);
        else if (battle->BattleStatus == PETBATTLE_STATUS_FINISHED)
        {
            battle->BattleStatus = PETBATTLE_STATUS_PENDING_DELETE;
            _petBattlesDeleteQueue.push(std::make_pair(itr.first, itr.second));
        }
    }

    if (_LFBRequestsUpdateTimer.Passed())
    {
        _LFBRequestsUpdateTimer.Reset();

        std::lock_guard<std::mutex> l_Lock(_LFBRequestsMutex);
        std::vector<ObjectGuid> ticketsToRemove;

        for (auto pair : _LFBRequests)
        {
            auto ticket = pair.second;
            if (!ticket)
            {
                ticketsToRemove.push_back(pair.first);
                continue;
            }

            auto count = std::count_if(ticketsToRemove.begin(), ticketsToRemove.end(), [ticket](ObjectGuid const& p_Ticket) -> bool
            {
                return p_Ticket == ticket->RequesterGUID;
            });

            if (count > 0)
                continue;

            auto elapsed = GameTime::GetGameTime() - ticket->JoinTime;
            uint32 queuedTime = elapsed > 0 ? uint32(std::min<uint64>(elapsed, std::numeric_limits<int32>::max())) : 0;
            uint64 samples = uint64(_LFBNumWaitTimeAvg) + 1;
            _LFBAvgWaitTime = int32((uint64(_LFBAvgWaitTime) * _LFBNumWaitTimeAvg + queuedTime) / samples);
            _LFBNumWaitTimeAvg = uint32(std::min<uint64>(samples, std::numeric_limits<uint32>::max()));

            switch (ticket->State)
            {
                case LFBState::LFB_STATE_QUEUED:
                {
                    if (auto player = ObjectAccessor::FindPlayer(ticket->RequesterGUID))
                    {
                        player->GetSession()->SendPetBattleQueueStatus(ticket->JoinTime, ticket->TicketID, LFBUpdateStatus::LFB_UPDATE_STATUS, _LFBAvgWaitTime);

                        std::vector<LFBTicket*> possibleOpponent;
                        for (auto v : _LFBRequests)
                        {
                            auto secondTicket = v.second;

                            if (!secondTicket || secondTicket->State != LFBState::LFB_STATE_QUEUED || secondTicket->TeamID != ticket->TeamID || abs(static_cast<int>(secondTicket->Weight - ticket->Weight)) > 5)
                                continue;

                            if (secondTicket->RequesterGUID == ticket->RequesterGUID)
                                continue;

                            if (ObjectAccessor::FindPlayer(secondTicket->RequesterGUID) == nullptr)
                                continue;

                            possibleOpponent.push_back(secondTicket);
                        }

                        std::sort(possibleOpponent.begin(), possibleOpponent.end(), [](LFBTicket const* a, LFBTicket const* b)
                        {
                            return a->Weight > b->Weight;
                        });

                        if (!possibleOpponent.empty())
                        {
                            auto l_Left = ticket;
                            auto l_Right = possibleOpponent[0];

                            auto leftPlayer = ObjectAccessor::FindPlayer(l_Left->RequesterGUID);
                            auto rightPlayer = ObjectAccessor::FindPlayer(l_Right->RequesterGUID);

                            l_Left->MatchingOpponent = l_Right;
                            l_Right->MatchingOpponent = l_Left;

                            l_Left->State = LFBState::LFB_STATE_PROPOSAL;
                            l_Right->State = LFBState::LFB_STATE_PROPOSAL;

                            l_Left->ProposalTime = GameTime::GetGameTime();
                            l_Right->ProposalTime = GameTime::GetGameTime();
                            l_Left->ProposalAnswer = LFBAnswer::LFB_ANSWER_PENDING;
                            l_Right->ProposalAnswer = LFBAnswer::LFB_ANSWER_PENDING;

                            leftPlayer->GetSession()->SendPetBattleQueueStatus(l_Left->JoinTime, l_Left->TicketID, LFBUpdateStatus::LFB_PROPOSAL_BEGIN, _LFBAvgWaitTime);
                            rightPlayer->GetSession()->SendPetBattleQueueStatus(l_Right->JoinTime, l_Right->TicketID, LFBUpdateStatus::LFB_PROPOSAL_BEGIN, _LFBAvgWaitTime);

                            leftPlayer->GetSession()->SendPetBattleQueueProposeMatch();
                            rightPlayer->GetSession()->SendPetBattleQueueProposeMatch();
                        }
                    }
                    else
                        ticketsToRemove.push_back(ticket->RequesterGUID);
                    break;
                }
                case LFBState::LFB_STATE_PROPOSAL:
                {
                    auto l_Left = ticket;
                    auto l_Right = ticket->MatchingOpponent;

                    auto leftPlayer = ObjectAccessor::FindPlayer(l_Left->RequesterGUID);
                    auto rightPlayer = ObjectAccessor::FindPlayer(l_Right->RequesterGUID);

                    /// Enter in combat
                    if (l_Left->ProposalAnswer == LFBAnswer::LFB_ANSWER_AGREE && l_Right->ProposalAnswer == LFBAnswer::LFB_ANSWER_AGREE)
                    {
                        auto leftEntry = CheckQueueEntry(leftPlayer);
                        auto rightEntry = CheckQueueEntry(rightPlayer);
                        if (leftEntry != PETBATTLE_REQUEST_OK || rightEntry != PETBATTLE_REQUEST_OK)
                        {
                            l_Left->MatchingOpponent = nullptr;
                            l_Right->MatchingOpponent = nullptr;
                            auto finishAdmission = [&](LFBTicket* request, Player* participant, eBattlePetRequests entry)
                            {
                                if (entry == PETBATTLE_REQUEST_OK)
                                {
                                    request->State = LFB_STATE_QUEUED;
                                    request->ProposalAnswer = LFB_ANSWER_PENDING;
                                    participant->GetSession()->SendPetBattleQueueStatus(request->JoinTime, request->TicketID, LFBUpdateStatus::LFB_JOIN_QUEUE, _LFBAvgWaitTime);
                                }
                                else
                                {
                                    request->State = LFB_STATE_FINISHED;
                                    ticketsToRemove.push_back(request->RequesterGUID);
                                    if (participant && participant->GetSession())
                                        participant->GetSession()->SendPetBattleRequestFailed(entry);
                                }
                            };
                            finishAdmission(l_Left, leftPlayer, leftEntry);
                            finishAdmission(l_Right, rightPlayer, rightEntry);
                            break;
                        }
                        if (leftPlayer && rightPlayer)
                        {
                            leftPlayer->GetSession()->SendPetBattleQueueStatus(l_Left->JoinTime, l_Left->TicketID, LFBUpdateStatus::LFB_PET_BATTLE_IS_STARTED, _LFBAvgWaitTime);
                            rightPlayer->GetSession()->SendPetBattleQueueStatus(l_Right->JoinTime, l_Right->TicketID, LFBUpdateStatus::LFB_PET_BATTLE_IS_STARTED, _LFBAvgWaitTime);

                            l_Left->State = LFBState::LFB_STATE_IN_COMBAT;
                            l_Right->State = LFBState::LFB_STATE_IN_COMBAT;

                            static PetBattleMembersPositions const gPetBattlePositions[7] =
                            {
                                PetBattleMembersPositions(0, TEAM_ALLIANCE, Position(-9502.376f, 114.492f, 59.822f), Position(-9493.934f, 119.854f, 58.459f)),
                                PetBattleMembersPositions(0, TEAM_ALLIANCE, Position(-10048.859f, 1231.028f, 40.881f), Position(-10054.330f, 1239.399f, 40.894f)),
                                PetBattleMembersPositions(0, TEAM_ALLIANCE, Position(-10909.911f, -362.280f, 39.643f), Position(-10899.923f, -362.773f, 39.265f)),
                                PetBattleMembersPositions(0, TEAM_ALLIANCE, Position(-10439.142f, -1939.163f, 104.313f), Position(-10439.306f, -1949.162f, 103.763f)),

                                PetBattleMembersPositions(1, TEAM_HORDE, Position(-954.766f, -3255.210f, 95.645f), Position(-958.212f, -3264.597f, 95.837f)),
                                PetBattleMembersPositions(1, TEAM_HORDE, Position(-2285.038f, -2155.838f, 95.843f), Position(-2281.738f, -2146.397f, 95.843f)),
                                //PetBattleMembersPositions(1, TEAM_HORDE, Position(-1369.247f, -2716.736f, 253.246f), Position(-1359.747f, -2713.613f, 253.390f)),
                                PetBattleMembersPositions(1, TEAM_HORDE, Position(-127.255f, -4959.972f, 20.903f), Position(-129.017f, -4950.128f, 21.378f))
                            };

                            std::vector<PetBattleMembersPositions> positions;
                            for (auto const& data : gPetBattlePositions)
                                if (data.Team == l_Left->TeamID)
                                    positions.push_back(data);

                            auto location = positions[urand(0, positions.size() - 1)];
                            std::shared_ptr<BattlePetInstance> playerPets[MAX_PETBATTLE_SLOTS];
                            std::shared_ptr<BattlePetInstance> playerOpposantPets[MAX_PETBATTLE_SLOTS];
                            size_t playerPetCount = 0;
                            size_t playerOpposantPetCount = 0;

                            auto battle = sPetBattleSystem->CreateBattle();
                            battle->PvPMatchMakingRequest.LocationResult = 0;
                            battle->PvPMatchMakingRequest.TeamPosition[PETBATTLE_TEAM_1] = location.Positions[0];
                            battle->PvPMatchMakingRequest.TeamPosition[PETBATTLE_TEAM_2] = location.Positions[1];

                            Position battleCenterPosition((location.Positions[0].GetPositionX() + location.Positions[1].GetPositionX()) / 2, (location.Positions[0].GetPositionY() + location.Positions[1].GetPositionY()) / 2, (location.Positions[0].GetPositionZ() + location.Positions[1].GetPositionZ()) / 2);
                            battle->PvPMatchMakingRequest.PetBattleCenterPosition = battleCenterPosition;

                            auto const& l_One = location.Positions[PETBATTLE_TEAM_1];
                            auto const& l_Second = location.Positions[PETBATTLE_TEAM_2];

                            float angle = atan2(l_Second.GetPositionY() - l_One.GetPositionY(), l_Second.GetPositionX() - l_One.GetPositionX());
                            battle->PvPMatchMakingRequest.PetBattleCenterPosition.SetOrientation((angle >= 0) ? angle : 2 * M_PI + angle);

                            for (size_t i = 0; i < MAX_PETBATTLE_SLOTS; ++i)
                            {
                                playerPets[i] = nullptr;
                                playerOpposantPets[i] = nullptr;
                            }

                            // Load player pets
                            auto petSlots = leftPlayer->GetBattlePetCombatTeam();

                            for (size_t i = 0; i < MAX_PETBATTLE_SLOTS; ++i)
                            {
                                if (!petSlots[i])
                                    continue;

                                if (playerPetCount >= MAX_PETBATTLE_SLOTS || playerPetCount >= leftPlayer->GetUnlockedPetBattleSlot())
                                    break;

                                playerPets[playerPetCount] = std::make_shared<BattlePetInstance>();
                                playerPets[playerPetCount]->CloneFrom(petSlots[i]);
                                playerPets[playerPetCount]->Slot = playerPetCount;
                                playerPets[playerPetCount]->OriginalBattlePet = petSlots[i];

                                ++playerPetCount;
                            }

                            auto petOpposantSlots = rightPlayer->GetBattlePetCombatTeam();

                            for (size_t i = 0; i < MAX_PETBATTLE_SLOTS; ++i)
                            {
                                if (!petOpposantSlots[i])
                                    continue;

                                if (playerOpposantPetCount >= MAX_PETBATTLE_SLOTS || playerOpposantPetCount >= rightPlayer->GetUnlockedPetBattleSlot())
                                    break;

                                playerOpposantPets[playerOpposantPetCount] = std::make_shared<BattlePetInstance>();
                                playerOpposantPets[playerOpposantPetCount]->CloneFrom(petOpposantSlots[i]);
                                playerOpposantPets[playerOpposantPetCount]->Slot = playerOpposantPetCount;
                                playerOpposantPets[playerOpposantPetCount]->OriginalBattlePet = petOpposantSlots[i];

                                ++playerOpposantPetCount;
                            }

                            // Add player pets
                            battle->Teams[PETBATTLE_TEAM_1]->OwnerGuid = leftPlayer->GetGUID();
                            battle->Teams[PETBATTLE_TEAM_1]->PlayerGuid = leftPlayer->GetGUID();
                            battle->Teams[PETBATTLE_TEAM_2]->OwnerGuid = rightPlayer->GetGUID();
                            battle->Teams[PETBATTLE_TEAM_2]->PlayerGuid = rightPlayer->GetGUID();

                            for (size_t i = 0; i < MAX_PETBATTLE_SLOTS; ++i)
                            {
                                if (playerPets[i])
                                    battle->AddPet(PETBATTLE_TEAM_1, playerPets[i]);

                                if (playerOpposantPets[i])
                                    battle->AddPet(PETBATTLE_TEAM_2, playerOpposantPets[i]);
                            }

                            battle->BattleType = PETBATTLE_TYPE_PVP_MATCHMAKING;
                            battle->PvPMatchMakingRequest.IsPvPReady[PETBATTLE_TEAM_1] = false;
                            battle->PvPMatchMakingRequest.IsPvPReady[PETBATTLE_TEAM_2] = false;

                            // Launch battle
                            leftPlayer->_petBattleId = battle->ID;
                            rightPlayer->_petBattleId = battle->ID;

                            leftPlayer->SetBattlegroundEntryPoint();
                            leftPlayer->ScheduleDelayedOperation(DELAYED_PET_BATTLE_INITIAL);
                            leftPlayer->SaveRecallPosition();
                            leftPlayer->TeleportTo(location.MapID, location.Positions[PETBATTLE_TEAM_1].GetPositionX() + 0.01f, location.Positions[PETBATTLE_TEAM_1].GetPositionY() + 0.01f, location.Positions[PETBATTLE_TEAM_1].GetPositionZ() + 0.01f, rightPlayer->GetOrientation() - M_PI);

                            rightPlayer->SetBattlegroundEntryPoint();
                            rightPlayer->ScheduleDelayedOperation(DELAYED_PET_BATTLE_INITIAL);
                            rightPlayer->SaveRecallPosition();
                            rightPlayer->TeleportTo(location.MapID, location.Positions[PETBATTLE_TEAM_2].GetPositionX() + 0.01f, location.Positions[PETBATTLE_TEAM_2].GetPositionY() + 0.01f, location.Positions[PETBATTLE_TEAM_2].GetPositionZ() + 0.01f, leftPlayer->GetOrientation() - M_PI);

                            ticketsToRemove.push_back(l_Left->RequesterGUID);
                            ticketsToRemove.push_back(l_Right->RequesterGUID);
                        }
                    }
                    /// Someone declined
                    else if (l_Left->ProposalAnswer == LFBAnswer::LFB_ANSWER_DENY || l_Right->ProposalAnswer == LFBAnswer::LFB_ANSWER_DENY)
                    {
                        bool p_LeftRemoved = false;
                        bool p_RightRemoved = false;

                        if (leftPlayer && rightPlayer)
                        {
                            // Detach the old pair before a survivor can be matched
                            // again later in this same update and cleanup pass.
                            l_Left->MatchingOpponent = nullptr;
                            l_Right->MatchingOpponent = nullptr;
                            if (l_Left->ProposalAnswer == LFBAnswer::LFB_ANSWER_DENY)
                            {
                                rightPlayer->GetSession()->SendPetBattleQueueStatus(l_Left->JoinTime, l_Left->TicketID, LFBUpdateStatus::LFB_OPPONENT_PROPOSAL_DECLINED, _LFBAvgWaitTime);
                                ticketsToRemove.push_back(l_Left->RequesterGUID);
                                l_Left->State = LFB_STATE_FINISHED;
                                p_LeftRemoved = true;
                            }

                            if (l_Right->ProposalAnswer == LFBAnswer::LFB_ANSWER_DENY)
                            {
                                leftPlayer->GetSession()->SendPetBattleQueueStatus(l_Left->JoinTime, l_Left->TicketID, LFBUpdateStatus::LFB_OPPONENT_PROPOSAL_DECLINED, _LFBAvgWaitTime);
                                ticketsToRemove.push_back(l_Right->RequesterGUID);
                                l_Right->State = LFB_STATE_FINISHED;
                                p_RightRemoved = true;
                            }

                            if (!p_LeftRemoved)
                            {
                                l_Left->State = LFBState::LFB_STATE_QUEUED;
                                leftPlayer->GetSession()->SendPetBattleQueueStatus(l_Left->JoinTime, l_Left->TicketID, LFBUpdateStatus::LFB_JOIN_QUEUE, _LFBAvgWaitTime);
                            }
                            if (!p_RightRemoved)
                            {
                                l_Right->State = LFBState::LFB_STATE_QUEUED;
                                rightPlayer->GetSession()->SendPetBattleQueueStatus(l_Right->JoinTime, l_Right->TicketID, LFBUpdateStatus::LFB_JOIN_QUEUE, _LFBAvgWaitTime);
                            }
                        }
                    }
                    /// Proposal expired
                    if ((GameTime::GetGameTime() - ticket->ProposalTime) > PETBATTLE_LFB_PROPOSAL_TIMEOUT)
                    {
                        l_Left->MatchingOpponent = nullptr;
                        l_Right->MatchingOpponent = nullptr;

                        ticketsToRemove.push_back(l_Left->RequesterGUID);
                        ticketsToRemove.push_back(l_Right->RequesterGUID);
                    }

                    break;
                }
                default:
                    break;
            }
        }

        for (auto& guid : ticketsToRemove)
        {
            auto ticket = _LFBRequests[guid];

            if (!ticket)
            {
                auto itr = _LFBRequests.find(guid);
                if (itr != _LFBRequests.end())
                    _LFBRequests.erase(itr);
                continue;
            }

            switch (ticket->State)
            {
                case LFBState::LFB_STATE_PROPOSAL:
                {
                    if (ticket->MatchingOpponent)
                    {
                        ticket->MatchingOpponent->MatchingOpponent = nullptr;
                        ticket->MatchingOpponent->State = LFBState::LFB_STATE_QUEUED;

                        if (auto opponent = ObjectAccessor::FindPlayer(ticket->MatchingOpponent->RequesterGUID))
                            opponent->GetSession()->SendPetBattleQueueStatus(ticket->MatchingOpponent->JoinTime, ticket->MatchingOpponent->TicketID, LFBUpdateStatus::LFB_JOIN_QUEUE, _LFBAvgWaitTime);
                    }

                    /// Continue to other case handlers
                }
                case LFBState::LFB_STATE_FINISHED:
                case LFBState::LFB_STATE_IN_COMBAT:
                case LFBState::LFB_STATE_QUEUED:
                {
                    if (auto player = ObjectAccessor::FindPlayer(ticket->RequesterGUID))
                    {
                        player->GetSession()->SendPetBattleQueueStatus(ticket->JoinTime, ticket->TicketID, LFBUpdateStatus::LFB_LEAVE_QUEUE, _LFBAvgWaitTime);
                        player->GetSession()->SendBattlePetJournalLockDenied();
                    }

                    delete _LFBRequests[guid];
                    _LFBRequests[guid] = nullptr;
                    break;
                }
                default:
                    break;
            }
        }
    }
}

void PetBattleSystem::ForfeitBattle(ObjectGuid battleID, ObjectGuid forfeiterGuid, bool ignoreAbandonPenalty)
{
    auto battle = GetBattle(battleID);
    if (!battle)
        return;

    uint32 forfeiterTeamID;
    for (forfeiterTeamID = 0; forfeiterTeamID < MAX_PETBATTLE_TEAM; ++forfeiterTeamID)
        if (battle->Teams[forfeiterTeamID]->OwnerGuid == forfeiterGuid)
            break;

    if (forfeiterTeamID == MAX_PETBATTLE_TEAM)
        return;

    if (battle->BattleStatus == PETBATTLE_STATUS_CREATION)
    {
        if (Player* player = ObjectAccessor::GetObjectInWorld(forfeiterGuid, static_cast<Player*>(nullptr)))
            if (battle->Teams[forfeiterTeamID]->PlayerGuid == forfeiterGuid)
                CancelUnstartedBattle(battle, player);
        return;
    }
    battle->Finish(!forfeiterTeamID, true, ignoreAbandonPenalty);
}

void PetBattleSystem::CancelUnstartedBattle(PetBattle* battle, Player* player)
{
    if (battle->BattleStatus != PETBATTLE_STATUS_CREATION)
        return;
    ObjectGuid const battleID = battle->ID;

    // No Begin means no active-pet selection, combat, penalty or reward.
    // In particular ActivePetID need not be initialized yet.
    battle->BattleStatus = PETBATTLE_STATUS_FINISHING;
    battle->RoundStatus = PETBATTLE_ROUND_FINISHED;
    for (auto team : battle->Teams)
        if (team && team->PlayerGuid)
        {
            Player* participant = team->PlayerGuid == player->GetGUID() ? player : ObjectAccessor::GetObjectInWorld(team->PlayerGuid, static_cast<Player*>(nullptr));
            if (!participant || participant->_petBattleId != battleID)
                continue;
            participant->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_NPC);
            participant->SetControlled(false, UNIT_STATE_ROOT);
            participant->_petBattleId.Clear();
            if (participant != player || !player->GetSession()->PlayerLogout())
                participant->GetSession()->SendPetBattleFinished();
            if (battle->BattleType == PETBATTLE_TYPE_PVP_MATCHMAKING && participant->m_recallLoc.GetMapId() != MAPID_INVALID && MapManager::IsValidMapCoord(participant->m_recallLoc))
            {
                if (participant == player && player->GetSession()->PlayerLogout())
                    participant->WorldRelocate(participant->m_recallLoc);
                else
                    participant->TeleportTo(participant->m_recallLoc);
            }
        }
    if (battle->BattleType == PETBATTLE_TYPE_PVE && battle->PveBattleType == PVE_PETBATTLE_WILD)
        for (auto pet : battle->Teams[PETBATTLE_PVE_TEAM_ID]->TeamPets)
            if (pet && pet->OriginalCreature)
                if (auto creature = ObjectAccessor::GetObjectInOrOutOfWorld(pet->OriginalCreature, static_cast<Creature*>(nullptr)))
                    if (creature->_petBattleId == battleID)
                    {
                        creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_PC);
                        creature->SetControlled(false, UNIT_STATE_ROOT);
                        creature->_petBattleId.Clear();
                        sWildBattlePetMgr->LeaveBattle(creature, false);
                    }
    battle->BattleStatus = PETBATTLE_STATUS_FINISHED;

}

Player* PetBattleSystem::GetLoggingOutPlayer(ObjectGuid battleID) const
{
    return loggingOutBattle == battleID ? loggingOutPlayer : nullptr;
}

void PetBattleSystem::OnPlayerLogout(Player* player)
{
    if (!player || !player->GetSession() || !player->GetSession()->PlayerLogout())
        return;

    RemoveRequest(player->GetGUID());
    // LeaveQueue's ordinary lookup inserts a null entry for an absent ticket.
    // Do not manufacture one for every character logging out.
    bool queued = false;
    {
        std::lock_guard<std::mutex> lock(_LFBRequestsMutex);
        auto ticket = _LFBRequests.find(player->GetGUID());
        if (ticket != _LFBRequests.end())
        {
            queued = ticket->second != nullptr;
            if (!queued)
                _LFBRequests.erase(ticket);
        }
    }
    if (queued)
        LeaveQueue(player);

    ObjectGuid const battleID = player->_petBattleId;
    PetBattle* battle = GetBattle(battleID);
    if (!battle)
    {
        player->_petBattleId.Clear();
        return;
    }
    uint32 teamID = 0;
    for (; teamID < MAX_PETBATTLE_TEAM; ++teamID)
        if (battle->Teams[teamID] && battle->Teams[teamID]->PlayerGuid == player->GetGUID() && battle->Teams[teamID]->OwnerGuid == player->GetGUID())
            break;
    if (teamID == MAX_PETBATTLE_TEAM)
    {
        player->_petBattleId.Clear();
        return;
    }
    auto const battleType = battle->BattleType;

    struct LogoutScope
    {
        Player* previousPlayer = loggingOutPlayer;
        ObjectGuid previousBattle = loggingOutBattle;
        LogoutScope(Player* player, ObjectGuid battle) { loggingOutPlayer = player; loggingOutBattle = battle; }
        ~LogoutScope() { loggingOutPlayer = previousPlayer; loggingOutBattle = previousBattle; }
    } scope(player, battleID);

    if (battle->BattleStatus == PETBATTLE_STATUS_CREATION)
        CancelUnstartedBattle(battle, player);
    else if (battle->BattleStatus == PETBATTLE_STATUS_RUNNING)
        battle->Finish(!teamID, true, false);

    // A prior detached Finish already copied the journal. Never penalize it
    // again, queue a companion summon, or teleport a deleting Player.
    if (player->_petBattleId == battleID)
        player->_petBattleId.Clear();
    player->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_NPC);
    player->SetControlled(false, UNIT_STATE_ROOT);
    if (battleType == PETBATTLE_TYPE_PVP_MATCHMAKING && player->m_recallLoc.GetMapId() != MAPID_INVALID && MapManager::IsValidMapCoord(player->m_recallLoc))
    {
        player->WorldRelocate(player->m_recallLoc);
        // SaveToDB uses the pending destination during a far transfer. Keep
        // the real loading state, but save the same validated return pose.
        if (player->IsBeingTeleportedFar())
            player->GetTeleportDest().WorldRelocate(player->m_recallLoc);
    }
}

eBattlePetRequests PetBattleSystem::CanPlayerEnterInPetBattle(Player* player, PetBattleRequest* petBattleRequest)
{
    if (!player || !player->IsInWorld())
        return PETBATTLE_REQUEST_CREATE_FAILED;

    if (player->isDead())
        return PETBATTLE_REQUEST_NOT_WHILE_DEAD;

    // Player can't be already in battle
    if (player->_petBattleId)
        return PETBATTLE_REQUEST_IN_BATTLE;

    if (petBattleRequest->OpponentGuid.IsPlayer())
    {
        if (auto player2 = ObjectAccessor::FindPlayer(petBattleRequest->OpponentGuid))
        {
            if (player2 == player)
                return PETBATTLE_REQUEST_TARGET_INVALID;
            if (!player2->IsInWorld())
                return PETBATTLE_REQUEST_TARGET_INVALID;
            if (player2->isDead())
                return PETBATTLE_REQUEST_NOT_WHILE_DEAD;
            if (player2->isInCombat())
                return PETBATTLE_REQUEST_NOT_WHILE_IN_COMBAT;
            if (player2->_petBattleId)
                return PETBATTLE_REQUEST_IN_BATTLE;

            if (!player->IsWithinDist3d(player2, INTERACTION_DISTANCE))
                return PETBATTLE_REQUEST_TARGET_OUT_OF_RANGE;

            if (!player->IsWithinLOSInMap(player2))
                return PETBATTLE_REQUEST_NOT_HERE_OBSTRUCTED;
        }
        else
            return PETBATTLE_REQUEST_TARGET_INVALID;
    }
    else if (petBattleRequest->OpponentGuid.IsCreature())
    {
        if (!player->GetNPCIfCanInteractWith(petBattleRequest->OpponentGuid, 0))
            return PETBATTLE_REQUEST_TARGET_INVALID;

        auto creature = sObjectAccessor->GetCreature(*player, petBattleRequest->OpponentGuid);
        if (creature->_petBattleId)
            return PETBATTLE_REQUEST_WILD_PET_TAPPED;

        if (!player->IsWithinDist3d(creature, INTERACTION_DISTANCE))
            return PETBATTLE_REQUEST_TARGET_OUT_OF_RANGE;

        if (!player->IsWithinLOSInMap(creature))
            return PETBATTLE_REQUEST_NOT_HERE_OBSTRUCTED;
    }

    if (player->isInCombat())
        return PETBATTLE_REQUEST_NOT_WHILE_IN_COMBAT;

    // Check positions
    for (const auto& itr : petBattleRequest->TeamPosition)
        if (player->GetMap()->getObjectHitPos(player->GetPhases(), true, petBattleRequest->PetBattleCenterPosition, itr, 0.0f))
            return PETBATTLE_REQUEST_NOT_HERE_UNEVEN_GROUND;

    auto petSlots = player->GetBattlePetCombatTeam();
    size_t playerPetCount = 0;
    size_t playerDeadPetCount = 0;

    for (size_t i = 0; i < MAX_PETBATTLE_SLOTS; ++i)
    {
        if (!petSlots[i])
            continue;

        if (playerPetCount >= MAX_PETBATTLE_SLOTS || playerPetCount >= player->GetUnlockedPetBattleSlot())
            break;

        if (petSlots[i]->Health < 1)
            playerDeadPetCount++;

        ++playerPetCount;
    }

    if (!playerPetCount)
        return PETBATTLE_REQUEST_NO_PETS_IN_SLOT;

    if (playerPetCount == playerDeadPetCount)
        return PETBATTLE_REQUEST_ALL_PETS_DEAD;

    return PETBATTLE_REQUEST_OK;
}

