/*
* Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "AccountMgr.h"
#include "AreaTriggerData.h"
#include "CharacterPackets.h"
#include "Chat.h"
#include "ContributionMgr.h"
#include "Conversation.h"
#include "ConditionMgr.h"
#include "EmbassyAllyChoice.h"
#include "Corpse.h"
#include "InstancePackets.h"
#include "LFGMgr.h"
#include "MiscPackets.h"
#include "ObjectMgr.h"
#include "OutdoorPvP.h"
#include "PlayerDefines.h"
#include "QuestData.h"
#include "QuestItemObjectUse.h"
#include "RestMgr.h"
#include "ScriptMgr.h"
#include "Warden.h"
#include "ArtifactPackets.h"

void WorldSession::HandleRepopRequest(WorldPackets::Misc::RepopRequest& /*packet*/)
{
    if (GetPlayer()->IsAlive() || GetPlayer()->HasFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
        return;

    if (GetPlayer()->HasAuraType(SPELL_AURA_PREVENT_RESURRECTION))
        return; // silently return, client should display the error by itself

    // the world update order is sessions, players, creatures
    // the netcode runs in parallel with all of these
    // creatures can kill players
    // so if the server is lagging enough the player can
    // release spirit after he's killed but before he is updated
    if (GetPlayer()->getDeathState() == JUST_DIED)
    {
        TC_LOG_DEBUG("network", "HandleRepopRequestOpcode: got request after player %s(%d) was killed and before he was updated", GetPlayer()->GetName(), GetPlayer()->GetGUIDLow());
        GetPlayer()->KillPlayer();
    }

    //this is spirit release confirm?
    GetPlayer()->RemovePet(NULL, PET_SAVE_NOT_IN_SLOT, true);
    GetPlayer()->BuildPlayerRepop();
    GetPlayer()->RepopAtGraveyard();
}

void WorldSession::HandleLogoutCancel(WorldPackets::Character::LogoutCancel& /*packet*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    LogoutRequest(0);

    SendPacket(WorldPackets::Character::LogoutCancelAck().Write());

    if (player->CanFreeMove())
    {
        player->SetRooted(false);
        player->SetStandState(UNIT_STAND_STATE_STAND);
        player->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
    }

    player->PetSpellInitialize();
}

void WorldSession::HandleTogglePvP(WorldPackets::Misc::TogglePvP& /*packet*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    bool inPvP = player->HasFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP);

    player->ApplyModFlag(PLAYER_FIELD_PLAYER_FLAGS, inPvP ? PLAYER_FLAGS_PVP_TIMER : PLAYER_FLAGS_IN_PVP, inPvP);

    if (player->HasFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP))
    {
        if (!player->IsPvP() || player->pvpInfo.endTimer)
            player->UpdatePvP(true, true);
    }
    else
    {
        if (!player->pvpInfo.inHostileArea && player->IsPvP())
            player->pvpInfo.endTimer = GameTime::GetGameTime();
    }
}

void WorldSession::HandleSetPvP(WorldPackets::Misc::SetPvP& packet)
{
    Player* player = GetPlayer();
    if (!player || player->IsFFAPvP())
        return;

    player->ApplyModFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP, packet.EnablePVP);
    player->ApplyModFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_PVP_TIMER, !packet.EnablePVP);

    if (player->HasFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP))
    {
        if (!player->IsPvP() || player->pvpInfo.endTimer)
            player->UpdatePvP(true, true);
    }
    else
    {
        if (!player->pvpInfo.inHostileArea && player->IsPvP())
            player->pvpInfo.endTimer = GameTime::GetGameTime();
    }
}

void WorldSession::HandlePortGraveyard(WorldPackets::Misc::PortGraveyard& /*packet*/)
{
    if (GetPlayer()->IsAlive() || !GetPlayer()->HasFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
        return;

    GetPlayer()->RepopAtGraveyard();
}

void WorldSession::HandleSetSelectionOpcode(WorldPackets::Misc::SetSelection& packet)
{
    _player->SetSelection(packet.Selection);
}

void WorldSession::HandleStandStateChangeOpcode(WorldPackets::Misc::StandStateChange& packet)
{
    _player->SetStandState(packet.StandState);
}

void WorldSession::HandleReclaimCorpse(WorldPackets::Misc::ReclaimCorpse& /*packet*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    if (player->IsAlive() || player->InArena() || !player->HasFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
        return;

    Corpse* corpse = player->GetCorpse();
    if (!corpse)
        return;

    if (time_t(corpse->GetGhostTime() + player->GetCorpseReclaimDelay(corpse->GetType() == CORPSE_RESURRECTABLE_PVP)) > time_t(GameTime::GetGameTime()))
        return;

    if (!corpse->IsWithinDist(player, CORPSE_RECLAIM_RADIUS, true) || !corpse->IsInMap(player))
        return;

    player->ResurrectPlayer(player->InBattleground() ? 1.0f : 0.5f);
    player->SpawnCorpseBones();
}

void WorldSession::HandleResurrectResponse(WorldPackets::Misc::ResurrectResponse& packet)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    if (player->IsAlive())
        return;

    if (packet.Response != 0)
    {
        player->ClearResurrectRequestData();
        return;
    }

    if (!player->IsRessurectRequestedBy(packet.Resurrecter))
        return;

    player->ResurectUsingRequestData();
}

void WorldSession::HandleAreaTrigger(WorldPackets::Misc::AreaTrigger& packet)
{
    Player* player = GetPlayer();
    if (player->isInFlight())
    {
        TC_LOG_DEBUG("network", "HandleAreaTrigger: Player '%s' (GUID: %u) in flight, ignore Area Trigger ID:%u",
            player->GetName(), player->GetGUIDLow(), packet.AreaTriggerID);
        return;
    }

    AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(packet.AreaTriggerID);
    if (!atEntry)
    {
        TC_LOG_DEBUG("network", "HandleAreaTrigger: Player '%s' (GUID: %u) send unknown (by DBC) Area Trigger ID:%u",
            player->GetName(), player->GetGUIDLow(), packet.AreaTriggerID);
        return;
    }

    if (packet.Entered && !player->IsInAreaTriggerRadius(atEntry))
    {
        TC_LOG_DEBUG("network", "HandleAreaTrigger: Player '%s' (GUID: %u) too far (trigger map: %u player map: %u), ignore Area Trigger ID: %u",
            player->GetName(), atEntry->ContinentID, player->GetMapId(), player->GetGUIDLow(), packet.AreaTriggerID);
        return;
    }

    if (player->isDebugAreaTriggers)
        ChatHandler(player).PSendSysMessage(packet.Entered ? LANG_DEBUG_AREATRIGGER_ENTERED : LANG_DEBUG_AREATRIGGER_LEFT, packet.AreaTriggerID);

    // set for future scrip using.
    player->SetLastAreaTrigger(atEntry);

    if (sScriptMgr->OnAreaTrigger(player, atEntry, packet.Entered))
        return;

    if (player->IsAlive())
    {
        if (std::unordered_set<uint32> const* quests = sAreaTriggerDataStore->GetQuestsForAreaTrigger(packet.AreaTriggerID))
        {
            for (uint32 questId : *quests)
            {
                if (questId == AmaniCatacombs::Quest && (!packet.Entered || packet.AreaTriggerID != AmaniCatacombs::Entrance))
                    continue;
                Quest const* qInfo = sQuestDataStore->GetQuestTemplate(questId);
                if (qInfo && player->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE)
                {
                    for (QuestObjective const& obj : qInfo->GetObjectives())
                    {
                        if (obj.Type != QUEST_OBJECTIVE_AREATRIGGER)
                            continue;

                        player->SetQuestObjectiveData(qInfo, &obj, int32(true));
                        break;
                    }

                    if (questId == AmaniCatacombs::Quest)
                    {
                        if (player->CanCompleteQuest(questId))
                            player->CompleteQuest(questId);
                        else
                            player->IncompleteQuest(questId);
                        continue;
                    }
                    player->AreaExploredOrEventHappens(questId);
                }
            }
        }
    }  

    if (sAreaTriggerDataStore->IsTavernAreaTrigger(packet.AreaTriggerID))
    {
        // set resting flag we are in the inn
        player->GetRestMgr().SetRestFlag(REST_FLAG_IN_TAVERN, atEntry->ID);

        if (sWorld->IsFFAPvPRealm())
        {
            player->RemoveByteFlag(UNIT_FIELD_BYTES_2, UNIT_BYTES_2_OFFSET_PVP_FLAG, UNIT_BYTE2_FLAG_FFA_PVP);
            player->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_STATUS);
        }

        return;
    }

    if (OutdoorPvP* pvp = player->GetOutdoorPvP())
        pvp->HandleAreaTrigger(_player, packet.AreaTriggerID, packet.Entered);

    AreaTriggerStruct const* at = sAreaTriggerDataStore->GetAreaTrigger(packet.AreaTriggerID);
    if (!at)
        return;

    bool teleported = false;
    if (player->GetMapId() != at->target_mapId)
    {
        if (!sMapMgr->CanPlayerEnter(at->target_mapId, player, false))
            return;

        if (Group* group = player->GetGroup())
            if (group->isLFGGroup() && player->GetMap()->IsDungeon())
                teleported = player->TeleportToBGEntryPoint();

        MapEntry const* targetMap = sMapStore.LookupEntry(at->target_mapId);
        if (targetMap->IsDungeon() && player->GetGroup())
        {
            if (uint32 dungeonId = sLFGMgr->GetDungeon(player->GetGroup()->GetGUID(), true))
                if (lfg::LFGDungeonData const* dungeon = sLFGMgr->GetLFGDungeon(dungeonId))
                    if (dungeon->map == targetMap->ID)
                        teleported = player->TeleportTo(dungeon->map, dungeon->x, dungeon->y, dungeon->z, dungeon->o, TELE_TO_NOT_LEAVE_TRANSPORT);
        }
    }

    if (!teleported)
        player->TeleportTo(at->target_mapId, at->target_X, at->target_Y, at->target_Z, at->target_Orientation, TELE_TO_NOT_LEAVE_TRANSPORT);
}

void WorldSession::HandleCompleteCinematic(WorldPackets::Misc::CompleteCinematic& /*packet*/) { }

void WorldSession::HandleNextCinematicCamera(WorldPackets::Misc::NextCinematicCamera& /*packet*/) { }

void WorldSession::HandleCompleteMovie(WorldPackets::Misc::CompleteMovie& /*packet*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    TC_LOG_INFO("entities.player", "Movie completion/skip received for %s (watching: %u)",
        player->GetName(), uint32(player->isWatchingMovie()));
    player->setWatchinMovie(false);
    player->SetCanDelayTeleport(false);
}

void WorldSession::HandleFarSight(WorldPackets::Misc::FarSight& packet)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    if (!packet.Enable)
        player->SetSeer(player);
    else if (WorldObject* target = player->GetViewpoint())
        player->SetSeer(target);

    player->UpdateVisibilityForPlayer();
}

void WorldSession::HandleResetInstances(WorldPackets::Instance::ResetInstances& /*packet*/)
{
    Player* player = GetPlayer();
    if (!player || player->GetDungeonDifficultyID() == DIFFICULTY_MYTHIC_KEYSTONE)
        return;

    if (Group* group = player->GetGroup())
    {
        if (group->InChallenge())
            return;

        if (group->IsLeader(player->GetGUID()))
            group->ResetInstances(INSTANCE_RESET_ALL, false, false, player);
    }
    else
        player->ResetInstances(INSTANCE_RESET_ALL, false, false);
}

void WorldSession::HandleSetDungeonDifficulty(WorldPackets::Misc::SetDungeonDifficulty& packet)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    DifficultyEntry const* difficultyEntry = sDifficultyStore.LookupEntry(packet.DifficultyID);
    if (!difficultyEntry)
        return;

    if (difficultyEntry->InstanceType != MAP_INSTANCE)
        return;

    if (!(difficultyEntry->Flags & DIFFICULTY_FLAG_CAN_SELECT))
        return;

    if (Difficulty(packet.DifficultyID) == player->GetDungeonDifficultyID())
        return;

    Map* map = player->FindMap();
    if (map && map->IsDungeon())
        return;

    Group* group = player->GetGroup();
    if (group)
    {
        if (group->InChallenge())
            return;

        if (group->IsLeader(player->GetGUID()))
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* groupGuy = itr->getSource();
                if (!groupGuy)
                    continue;

                if (!groupGuy->IsInMap(groupGuy))
                    return;

                if (groupGuy->GetMap()->IsNonRaidDungeon())
                    return;
            }

            group->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, false, false, player);
            group->SetDungeonDifficultyID(Difficulty(packet.DifficultyID));
            player->SendDungeonDifficulty();
            group->SendUpdate();
        }
    }
    else
    {
        player->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, false, false);
        player->SetDungeonDifficultyID(Difficulty(packet.DifficultyID));
        player->SendDungeonDifficulty();
    }
}

void WorldSession::HandleSetRaidDifficulty(WorldPackets::Misc::SetRaidDifficulty& packet)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    DifficultyEntry const* difficultyEntry = sDifficultyStore.LookupEntry(packet.DifficultyID);
    if (!difficultyEntry)
        return;

    if (difficultyEntry->InstanceType != MAP_RAID)
        return;

    if (!(difficultyEntry->Flags & DIFFICULTY_FLAG_CAN_SELECT))
        return;

    if (((difficultyEntry->Flags & DIFFICULTY_FLAG_LEGACY) >> 5) != packet.Legacy)
        return;

    auto difficultyID = Difficulty(difficultyEntry->ID);
    if (difficultyID == (packet.Legacy ?  player->GetLegacyRaidDifficultyID() : player->GetRaidDifficultyID()))
        return;

    Map* map = player->FindMap();
    if (map && map->IsDungeon())
        return;

    Group* group = player->GetGroup();
    if (group)
    {
        if (group->InChallenge())
            return;

        if (group->IsLeader(player->GetGUID()))
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* groupGuy = itr->getSource();
                if (!groupGuy)
                    continue;

                if (!groupGuy->IsInMap(groupGuy))
                    return;

                if (groupGuy->GetMap()->IsRaid())
                    return;
            }

            group->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, true, packet.Legacy != 0, player);
            if (packet.Legacy)
                group->SetLegacyRaidDifficultyID(difficultyID);
            else
                group->SetRaidDifficultyID(difficultyID);
            player->SendRaidDifficulty(packet.Legacy != 0);
            group->SendUpdate();
        }
    }
    else
    {
        player->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, true, packet.Legacy != 0);
        if (packet.Legacy)
            player->SetLegacyRaidDifficultyID(difficultyID);
        else
            player->SetRaidDifficultyID(difficultyID);

        player->SendRaidDifficulty(packet.Legacy != 0);
    }
}

void WorldSession::HandleRequestPetInfo(WorldPackets::PetPackets::RequestPetInfo& /*packet*/)
{
    // Handle the packet CMSG_REQUEST_PET_INFO - sent when player does ingame /reload command

    // Packet sent when player has a pet
    if (_player->GetPet())
        _player->PetSpellInitialize();
    else if (Unit* charm = _player->GetCharm())
    {
        // Packet sent when player has a possessed unit
        if (charm->HasUnitState(UNIT_STATE_POSSESSED))
            _player->PossessSpellInitialize();
        // Packet sent when player controlling a vehicle
        else if (charm->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
            _player->VehicleSpellInitialize();
        // Packet sent when player has a charmed unit
        else
            _player->CharmSpellInitialize();
    }
}

void WorldSession::HandleUITimeRequest(WorldPackets::Misc::UITimeRequest& /*request*/)
{
    WorldPackets::Misc::UITime response;
    response.Time = GameTime::GetGameTime();
    SendPacket(response.Write());
}

void WorldSession::SendSetPhaseShift(std::vector<WorldPackets::Misc::PhaseShiftDataPhase> phases, std::vector<uint16> const& visibleMapIDs, std::vector<uint16> const& uiWorldMapAreaIDSwaps, std::vector<uint16> const& preloadMapIDs, uint32 phaseShiftFlags /*= 0x1F*/)
{
    WorldPackets::Misc::PhaseShift phaseShift;
    phaseShift.Client = _player->GetGUID();
    phaseShift.Phaseshift.PersonalGUID = ObjectGuid::Empty;
    phaseShift.Phaseshift.PhaseShiftFlags = phaseShiftFlags | 0x08;
    phaseShift.Phaseshift.Phases = std::move(phases);
    phaseShift.PreloadMapIDs = preloadMapIDs;
    phaseShift.VisibleMapIDs = visibleMapIDs;
    phaseShift.UiWorldMapAreaIDSwaps = uiWorldMapAreaIDSwaps;
    SendPacket(phaseShift.Write());
}

void WorldSession::HandleInstanceLockResponse(WorldPackets::Instance::InstanceLockResponse& packet)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    if (!player->HasPendingBind())
        return;

    if (packet.AcceptLock)
        player->BindToInstance();
    else
        player->RepopAtGraveyard(true);

    player->SetPendingBind(0, 0);
}

void WorldSession::HandleViolenceLevel(WorldPackets::Misc::ViolenceLevel& /*packet*/) { }

void WorldSession::HandleObjectUpdateFailed(WorldPackets::Misc::ObjectUpdateFailed& packet)
{
    if (_player->GetGUID() == packet.ObjectGUID)
    {
        LogoutPlayer(true);
        return;
    }

    _player->RemoveClient(packet.ObjectGUID);
}

void WorldSession::HandleObjectUpdateRescued(WorldPackets::Misc::ObjectUpdateRescued& packet)
{
    _player->AddClient(packet.ObjectGUID);
}

void WorldSession::HandleNeutralPlayerSelectFaction(WorldPackets::Character::NeutralPlayerSelectFaction& packet)
{
    constexpr uint32 NewFate = 31450;
    constexpr uint32 FactionCredit = 64594;
    constexpr uint32 ShangXi = 56013;
    constexpr uint32 JoinAlliance = 1;
    constexpr uint32 JoinHorde = 0;

    Player* player = GetPlayer();
    if (!player || !player->IsInWorld() || !player->IsAlive() || player->getRace() != RACE_PANDAREN_NEUTRAL ||
        player->HasPendingTeleport() || player->isWatchingMovie() || (packet.Faction != JoinAlliance && packet.Faction != JoinHorde))
        return;

    QuestStatus status = player->GetQuestStatus(NewFate);
    bool activeQuest = status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE;
    // Keep the administrator's explicit selectfaction command available.
    if (GetSecurity() < SEC_ADMINISTRATOR)
    {
        if (player->GetMapId() != 860 || (!activeQuest && status != QUEST_STATUS_REWARDED))
            return;
        Creature* guide = player->FindNearestCreature(ShangXi, INTERACTION_DISTANCE);
        if (!guide || !player->GetNPCIfCanInteractWith(guide->GetGUID(), UNIT_NPC_FLAG_GOSSIP) || !player->IsWithinLOSInMap(guide))
            return;
    }

    bool joinAlliance = packet.Faction == JoinAlliance;
    uint8 newRace = joinAlliance ? RACE_PANDAREN_ALLIANCE : RACE_PANDAREN_HORDE;
    WorldLocation arrival = joinAlliance ? WorldLocation(0, -9076.77f, 424.74f, 92.42f, float(M_PI)) :
        WorldLocation(1, 1349.72f, -4374.50f, 26.15f, float(M_PI));
    player->SetByteValue(UNIT_FIELD_BYTES_0, 0, newRace);
    player->setFactionForRace(newRace);

    {
        Player::ScopedTeleportDelay delay(*player);
        if (!player->TeleportTo(arrival))
        {
            player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_PANDAREN_NEUTRAL);
            player->setFactionForRace(RACE_PANDAREN_NEUTRAL);
            return;
        }
    }

    if (activeQuest)
    {
        if (!player->GetQuestObjectiveData(NewFate, FactionCredit))
        {
            // Recover quests the earlier handler marked complete before a selection.
            if (status == QUEST_STATUS_COMPLETE)
                player->IncompleteQuest(NewFate);
            player->KilledMonsterCredit(FactionCredit);
        }
        // The legacy exploration flag also expects the actual faction event.
        if (player->GetQuestStatus(NewFate) == QUEST_STATUS_INCOMPLETE && player->GetQuestObjectiveData(NewFate, FactionCredit) == 1)
            player->CompleteQuest(NewFate);
    }

    player->SetHomebind(arrival, joinAlliance ? 9 : 363);
    player->learnSpell(joinAlliance ? 668 : 669, false);
    player->learnSpell(108127, false);
    player->SendMovieStart(116);
    player->SaveToDB();
}

void WorldSession::HandleRequestCemeteryList(WorldPackets::Misc::RequestCemeteryList& /*packet*/)
{
    uint32 team = _player->GetTeam();

    std::vector<uint32> graveyardIds;
    auto range = sObjectMgr->GraveYardStore.equal_range(_player->GetCurrentZoneID());

    for (auto it = range.first; it != range.second && graveyardIds.size() < 16; ++it) // client max
    {
        if (it->second.team == 0 || it->second.team == team)
            graveyardIds.push_back(it->first);
    }

    if (graveyardIds.empty())
        return;

    WorldPackets::Misc::RequestCemeteryListResponse packet;
    packet.IsGossipTriggered = false;
    packet.CemeteryID.reserve(graveyardIds.size());

    for (uint32 const& id : graveyardIds)
        packet.CemeteryID.push_back(id);

    SendPacket(packet.Write());
}

void WorldSession::HandleForcedReactions(WorldPackets::Reputation::RequestForcedReactions& /*packet*/)
{
    _player->GetReputationMgr().SendForceReactions();
}

void WorldSession::HandleMountSpecialAnim(WorldPackets::Misc::MountSpecialAnim& /*packet*/)
{
    _player->SendMessageToSet(WorldPackets::Misc::SpecialMountAnim(_player->GetGUID()).Write(), false);
}

void WorldSession::HandleSummonResponse(WorldPackets::Movement::SummonResponse& packet)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    if (!player->IsAlive() || player->isInCombat())
        return;

    player->SummonIfPossible(packet.Accept);
}

void WorldSession::HandleRandomRollClient(WorldPackets::Misc::RandomRollClient& packet)
{
    Player* player = GetPlayer();
    if (packet.Min > packet.Max || packet.Max > 10000 || !player)
        return;

    WorldPackets::Misc::RandomRoll randomRoll;
    randomRoll.Min = packet.Min;
    randomRoll.Max = packet.Max;
    randomRoll.Result = urand(packet.Min, packet.Max);
    randomRoll.Roller = player->GetGUID();
    randomRoll.RollerWowAccount = GetBattlenetAccountGUID();
    if (player->GetGroup())
        player->GetGroup()->BroadcastPacket(randomRoll.Write(), false);
    else
        SendPacket(randomRoll.Write());
}

void WorldSession::HandleOpeningCinematic(WorldPackets::Misc::OpeningCinematic& /*packet*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    if (player->GetUInt32Value(PLAYER_FIELD_XP))
        return;

    if (ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(player->getClass()))
    {
        if (classEntry->CinematicSequenceID)
            player->SendCinematicStart(classEntry->CinematicSequenceID);
        else if (ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(player->getRace()))
            player->SendCinematicStart(raceEntry->CinematicSequenceID);
    }
}

void WorldSession::HandleRequestResearchHistory(WorldPackets::Misc::RequestResearchHistory& /*packet*/)
{
    _player->SendCompletedProjects();
}

void WorldSession::HandleChoiceResponse(WorldPackets::Misc::ChoiceResponse& packet)
{
    auto playerChoice = sObjectMgr->GetPlayerChoice(packet.ChoiceID);
    if (!playerChoice)
        return;

    auto playerChoiceResponse = playerChoice->GetResponse(packet.ResponseID);
    if (!playerChoiceResponse)
        return;

    if (auto choice = EmbassyAllyChoice::Find(packet.ChoiceID))
    {
        auto ally = choice->FindAlly(packet.ResponseID);
        if (!ally || !playerChoiceResponse->Reward || playerChoiceResponse->Reward->SpellID != ally->Spell ||
            !EmbassyAllyChoice::CanSelect(*_player, *choice, *ally))
            return;
    }

    // Recheck at execution: a stale selection window must not restart a
    // completed artifact route or cast its hidden chosen-quest reward spell.
    if ((packet.ChoiceID == 236 || packet.ChoiceID == 245 || packet.ChoiceID == 240 || packet.ChoiceID == 242 || packet.ChoiceID == 265) &&
        (!sConditionMgr->IsObjectMeetToConditions(_player,
            sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_PLAYER_CHOICE, packet.ChoiceID)) ||
         !sConditionMgr->IsObjectMeetToConditions(_player,
            sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_PLAYER_CHOICE_RESPONS, packet.ResponseID))))
        return;

    if (auto reward = playerChoiceResponse->Reward)
        if (reward.has_value() && reward->SpellID)
            _player->CastSpell(_player, reward->SpellID, true);

    // Choice reward spells set the hidden "<specialization> Chosen" quest.
    // Notify scripts afterwards so class-hall scripts can advance the visible
    // selection objective and start the corresponding acquisition quest.
    sScriptMgr->OnPlayerChoiceResponse(_player, packet.ChoiceID, packet.ResponseID);
}

void WorldSession::HandleQueryWorldCountwodnTimer(WorldPackets::Instance::QueryWorldCountwodnTimer& packet)
{
    Player* player = GetPlayer();
    if (Battleground* bg = player->GetBattleground())
        bg->HandleStartTimer(packet.Type);
}

void WorldSession::HandleCloseInteraction(WorldPackets::Misc::CloseInteraction& packet)
{
    Player* player = GetPlayer();
    if (packet.ObjectGUID.IsGameObject())
    {
        if (GameObject* obj = player->GetMap()->GetGameObject(packet.ObjectGUID))
        {
            switch (obj->GetGoType())
            {
                case GAMEOBJECT_TYPE_ARTIFACT_FORGE:     //47
                    player->SendDirectMessage(WorldPackets::Artifact::NullSmsg(SMSG_ARTIFACT_FORGE_CLOSE).Write());
                    break;
                default:
                    break;
            }
        }
    }
}

void WorldSession::HandleMountSetFavorite(WorldPackets::Misc::MountSetFavorite& packet)
{
    GetPlayer()->GetCollectionMgr()->MountSetFavorite(packet.MountSpellID, packet.IsFavorite);
}

void WorldSession::HandleRequestConsumptionConversionInfo(WorldPackets::Misc::RequestConsumptionConversionInfo& /*packet*/)
{ }

void WorldSession::HandleContributionGetState(WorldPackets::Misc::ContributionGetState& packet)
{
    sContributionMgr.ContributionGetState(GetPlayer(), packet.ContributionID, packet.ContributionGUID);
}

void WorldSession::HandleConversationLineStarted(WorldPackets::Misc::ConversationLineStarted& packet)
{
    Player* player = GetPlayer();
    if (!player || !player->IsInWorld() || !packet.ConversationGUID.IsConversation())
        return;

    // Resolve on the sender's current map, never by client-supplied entry alone.
    Conversation* conversation = ObjectAccessor::GetConversation(*player, packet.ConversationGUID);
    if (!conversation)
    {
        TC_LOG_DEBUG("network", "Conversation line start: expired or unknown conversation %s, line %u",
            packet.ConversationGUID.ToString().c_str(), packet.LineID);
        return;
    }

    bool accepted = conversation->NotifyLineStarted(player, packet.LineID);
    TC_LOG_DEBUG("network", "Conversation line start: conversation %s, line %u, accepted %u",
        packet.ConversationGUID.ToString().c_str(), packet.LineID, uint32(accepted));
}

void WorldSession::HandleContributionCollectorContribute(WorldPackets::Misc::ContributionCollectorContribute& packet)
{
    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(packet.ContributionTableNpcGuid, UNIT_NPC_FLAG2_CONTRIBUTION_NPC);
    sContributionMgr.Contribute(GetPlayer(), unit, packet.OrderIndex);
}
