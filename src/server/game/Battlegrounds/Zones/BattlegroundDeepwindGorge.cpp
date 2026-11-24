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

#include "Battleground.h"
#include "BattlegroundDeepwindGorge.h"
#include "BattlegroundPackets.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "BattlegroundPackets.h"
#include "WorldStatePackets.h"

BattlegroundDeepwindGorge::BattlegroundDeepwindGorge(): _flagsUpdTimer(0)
{
    BgObjects.resize(BG_DG_OBJECT_MAX);
    BgCreatures.resize(BG_DG_UNIT_MAX);

    _goldUpdate = Seconds(5);

    for (uint8 i = TEAM_ALLIANCE; i < MAX_TEAMS; ++i)
        _carts[i] = nullptr;

    for (uint8 i = BG_DG_UNIT_FLAG_BOT; i <= BG_DG_UNIT_FLAG_TOP; ++i)
        _points[i] = nullptr;
}

BattlegroundDeepwindGorge::~BattlegroundDeepwindGorge()
{
    for (uint8 i = BG_DG_UNIT_FLAG_BOT; i <= BG_DG_UNIT_FLAG_TOP; ++i)
        delete _points[i];

    for (uint8 i = TEAM_ALLIANCE; i < MAX_TEAMS; ++i)
        delete _carts[i];
}

void BattlegroundDeepwindGorge::StartingEventCloseDoors()
{
    for (uint32 i = BG_DG_DOOR_1; i <= BG_DG_DOOR_4; ++i)
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);
}

void BattlegroundDeepwindGorge::StartingEventOpenDoors()
{
    for (uint32 i = BG_DG_DOOR_1; i <= BG_DG_DOOR_4; ++i)
        DoorOpen(i);
}

bool BattlegroundDeepwindGorge::UpdatePlayerScore(Player* player, uint32 type, uint64 value, bool doAddHonor /*= true*/)
{
    if (!Battleground::UpdatePlayerScore(player, type, value, doAddHonor))
        return false;

    switch (type)
    {
        case SCORE_POINTS_CAPTURED:
            player->UpdateAchievementCriteria(CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, DG_OBJECTIVE_CAPTURE_FLAG, 1);
            break;
        case SCORE_POINTS_DEFENDED:
            player->UpdateAchievementCriteria(CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, DG_OBJECTIVE_DEFENDED_FLAG, 1);
            break;
        default:
            break;
    }

    return true;
}

WorldSafeLocsEntry const* BattlegroundDeepwindGorge::GetClosestGraveYard(Player* player)
{
    static uint32 const locs[2] = { BG_DG_LOC_SPIRIT_ALLIANCE_BOT, BG_DG_LOC_SPIRIT_HORDE_TOP };
    static uint32 const spirits[2] = { BG_DG_LOC_SPIRIT_ALLIANCE_TOP, BG_DG_LOC_SPIRIT_HORDE_BOT };

    float x, y;
    player->GetPosition(x, y);

    WorldSafeLocsEntry const* graveyard = sWorldSafeLocsStore.LookupEntry(locs[player->GetBGTeamId()]);
    WorldSafeLocsEntry const* entry = sWorldSafeLocsStore.LookupEntry(spirits[player->GetBGTeamId()]);
    if ((entry->Loc.X - x)*(entry->Loc.X - x) + (entry->Loc.Y - y)*(entry->Loc.Y - y) < (graveyard->Loc.X - x)*(graveyard->Loc.X - x) + (graveyard->Loc.Y - y)*(graveyard->Loc.Y - y))
        graveyard = entry;

    return graveyard;
}

void BattlegroundDeepwindGorge::AddPlayer(Player* player)
{
    Battleground::AddPlayer(player);
    PlayerScores[player->GetGUID()] = new BattlegroundDGScore(player->GetGUID(), player->GetBGTeamId());

    player->SendDirectMessage(WorldPackets::Battleground::Init(BG_DG_MAX_TEAM_SCORE).Write());
    Battleground::SendBattleGroundPoints(player->GetBGTeamId() != TEAM_ALLIANCE, m_TeamScores[player->GetBGTeamId()], false, player);
}

void BattlegroundDeepwindGorge::RemovePlayer(Player* player, ObjectGuid /*guid*/, uint32 /*team*/)
{
    if (GetStatus() == STATUS_WAIT_LEAVE)
        return;

    if (player)
        EventPlayerDroppedFlag(player);
}

void BattlegroundDeepwindGorge::HandleKillPlayer(Player* player, Player* killer)
{
    if (!player || GetStatus() != STATUS_IN_PROGRESS)
        return;

    for (uint8 i = TEAM_ALLIANCE; i < MAX_TEAMS; ++i)
        if (_carts[i] && _carts[i]->TakePlayerWhoDroppedFlag() == player->GetGUID())
        {
            UpdatePlayerScore(killer, SCORE_CARTS_DEFENDED, 1, false);
            if (killer != player)
                killer->UpdateAchievementCriteria(CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, DG_OBJECTIVE_RETURN_CART, 1);
        }

    EventPlayerDroppedFlag(player);
    Battleground::HandleKillPlayer(player, killer);
}

void BattlegroundDeepwindGorge::HandlePointCapturing(Player* player, Creature* creature)
{
    for (uint8 i = BG_DG_UNIT_FLAG_BOT; i <= BG_DG_UNIT_FLAG_TOP; ++i)
        if (_points[i]->GetCreaturePoint() == creature)
            _points[i]->PointClicked(player);
}

void BattlegroundDeepwindGorge::EventPlayerUsedGO(Player* player, GameObject* go)
{
    for (uint8 i = TEAM_ALLIANCE; i < MAX_TEAMS; ++i)
        if (_carts[i] && _carts[i]->GetGameObject() == go)
        {
            player->CastSpell(player, player->GetBGTeamId() == TEAM_ALLIANCE ? BG_DG_AURA_PLAYER_FLAG_HORDE : BG_DG_AURA_PLAYER_FLAG_ALLIANCE, true);
            _carts[i]->ToggleCaptured(player);
            SendBroadcastText(BgDGCartChecked[player->GetBGTeamId()], CHAT_MSG_BG_SYSTEM_NEUTRAL, player);
        }
}

void BattlegroundDeepwindGorge::EventPlayerDroppedFlag(Player* player)
{
    for (uint8 i = TEAM_ALLIANCE; i < MAX_TEAMS; ++i)
        if (_carts[i] && _carts[i]->ControlledByPlayerWithGuid() == player->GetGUID())
        {
            _carts[i]->CartDropped();
            SendBroadcastText(BgDGCartRetured[player->GetBGTeamId()], CHAT_MSG_BG_SYSTEM_NEUTRAL, player);
        }
}

ObjectGuid BattlegroundDeepwindGorge::GetFlagPickerGUID(int32 team) const
{
    if (_carts[team == TEAM_ALLIANCE ? TEAM_ALLIANCE : TEAM_HORDE])
        if (Player* player = _carts[team == TEAM_ALLIANCE ? TEAM_ALLIANCE : TEAM_HORDE]->ControlledBy())
            return player->GetGUID();

    return ObjectGuid::Empty;
}

uint32 BattlegroundDeepwindGorge::ModGold(TeamId teamId, int32 val)
{
    m_TeamScores[teamId] = (val < 0 && int32(m_TeamScores[teamId] + val) < 0) ? 0 : m_TeamScores[teamId] + val;
    UpdateWorldState(teamId == TEAM_ALLIANCE ? WorldStates::DG_ALLIANCE_POINTS : WorldStates::DG_HORDE_POINTS, m_TeamScores[teamId]);

    if (m_TeamScores[teamId] > BG_DG_MAX_TEAM_SCORE)
        m_TeamScores[teamId] = BG_DG_MAX_TEAM_SCORE;

    Battleground::SendBattleGroundPoints(teamId != TEAM_ALLIANCE, m_TeamScores[teamId]);

    return m_TeamScores[teamId];
}

void BattlegroundDeepwindGorge::PostUpdateImpl(Milliseconds diff)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    for (uint8 i = BG_DG_UNIT_FLAG_BOT; i <= BG_DG_UNIT_FLAG_TOP; ++i)
        _points[i]->Update(diff);


    if (_goldUpdate < Milliseconds(0))
    {
        // wrong value, but idk blizz values
        ModGold(TEAM_ALLIANCE, _GetCapturedNodesForTeam(TEAM_ALLIANCE) * 16);
        ModGold(TEAM_HORDE, _GetCapturedNodesForTeam(TEAM_HORDE) * 16);

        _goldUpdate = Seconds(5);
    }
    else
        _goldUpdate -= diff;

    if (m_TeamScores[TEAM_ALLIANCE] >= BG_DG_MAX_TEAM_SCORE)
        EndBattleground(ALLIANCE);
    if (m_TeamScores[TEAM_HORDE] >= BG_DG_MAX_TEAM_SCORE)
        EndBattleground(HORDE);
}

void BattlegroundDeepwindGorge::_CheckPositions(uint32 diff)
{
    for (auto const& itr : GetPlayers())
    {
        Player* player = ObjectAccessor::FindPlayer(GetBgMap(), itr.first);
        if (!player)
            continue;

        if (player->IsInAreaTriggerRadius(9013) && _carts[TEAM_ALLIANCE] && _carts[TEAM_ALLIANCE]->ControlledByPlayerWithGuid() == player->GetGUID()) // Alliance cart point
        {
            _carts[TEAM_ALLIANCE]->CartDelivered();
            SendBroadcastText(BgDGCartCaptured[TEAM_ALLIANCE], CHAT_MSG_BG_SYSTEM_NEUTRAL, player);
            break;
        }

        if (player->IsInAreaTriggerRadius(9012) && _carts[TEAM_HORDE] && _carts[TEAM_HORDE]->ControlledByPlayerWithGuid() == player->GetGUID()) // Horde cart point
        {
            _carts[TEAM_HORDE]->CartDelivered();
            SendBroadcastText(BgDGCartCaptured[TEAM_HORDE], CHAT_MSG_BG_SYSTEM_NEUTRAL, player);
            break;
        }
    }

    Battleground::_CheckPositions(diff);
}

void BattlegroundDeepwindGorge::FillInitialWorldStates(WorldPackets::WorldState::InitWorldStates& packet)
{
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7858), 0);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7859), 0);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7862), 0);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7863), 0);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7866), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7867), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7870), 1);
    packet.Worldstates.emplace_back(WorldStates::DG_ALLIANCE_POINTS, m_TeamScores[TEAM_ALLIANCE]);
    packet.Worldstates.emplace_back(WorldStates::DG_HORDE_POINTS, m_TeamScores[TEAM_HORDE]);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7885), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7886), 1);
    packet.Worldstates.emplace_back(WorldStates::DG_HORDE_CART_ASSAULT, (_carts[TEAM_ALLIANCE] && !_carts[TEAM_ALLIANCE]->ControlledByPlayerWithGuid().IsEmpty() ? 2 : 1));
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7892), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7893), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7894), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7895), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7897), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7898), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7899), 1);
    packet.Worldstates.emplace_back(WorldStates::DG_ALLIANCE_CART_ASSAULT, (_carts[TEAM_HORDE] && !_carts[TEAM_HORDE]->ControlledByPlayerWithGuid().IsEmpty() ? 2 : 1));
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7905), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7906), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7907), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7908), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7909), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7910), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7911), 1);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7933), 0);
    packet.Worldstates.emplace_back(static_cast<WorldStates>(7937), 0);
    packet.Worldstates.emplace_back(WorldStates::DG_ALLIANCE_NODES, _GetCapturedNodesForTeam(TEAM_ALLIANCE));
    packet.Worldstates.emplace_back(WorldStates::DG_HORDE_NODES, _GetCapturedNodesForTeam(TEAM_HORDE));

    for (uint8 i = BG_DG_UNIT_FLAG_BOT; i <= BG_DG_UNIT_FLAG_TOP; i++)
    {
        if (!_points[i] || _points[i]->GetState() == POINT_STATE_NEUTRAL)
        {
            packet.Worldstates.emplace_back(DgNodeStates[i][POINT_STATE_NEUTRAL], 1);
        }
        else if (_points[i]->GetState() > POINT_STATE_NEUTRAL && _points[i]->GetState() < POINT_STATE_CAPTURED_ALLIANCE)
        {
            packet.Worldstates.emplace_back(DgNodeStates[i][_points[i]->GetState()], 1);
        }
        else
        {
            packet.Worldstates.emplace_back(DgNodeStates[i][POINT_STATE_CAPTURED], _points[i]->GetState() == POINT_STATE_CAPTURED_ALLIANCE ? 2 : 1); // 1 horde, 2 alliance OMG
        }
    }
}

void BattlegroundDeepwindGorge::Reset()
{
    Battleground::Reset();

    _goldUpdate = Seconds(5);

    for (uint8 i = 0; i < BG_DG_UNIT_MAX; ++i)
        if (!BgCreatures[i].IsEmpty())
            DelCreature(i);

    for (uint8 i = TEAM_ALLIANCE; i < MAX_TEAMS; ++i)
        _carts[i] = nullptr;

    for (uint8 i = BG_DG_UNIT_FLAG_BOT; i <= BG_DG_UNIT_FLAG_TOP; ++i)
        _points[i] = nullptr;
}

static bool AddDeepwindGates(Battleground& bg)
{
    return bg.AddObject(BG_DG_DOOR_1, BG_DG_ENTRY_DOOR_1, -263.455f, 218.163f, 132.43f, 4.72984f, 0, 0, 1.43143f, 4.48416f, RESPAWN_IMMEDIATELY)
        && bg.AddObject(BG_DG_DOOR_2, BG_DG_ENTRY_DOOR_2, -213.712f, 201.043f, 132.488f, 3.9619f, 0, 0, 1.43143f, 4.48416f, RESPAWN_IMMEDIATELY)
        && bg.AddObject(BG_DG_DOOR_3, BG_DG_ENTRY_DOOR_3, -69.8785f, 781.837f, 132.43f, 1.58825f, 0, 0, 1.43143f, 4.48416f, RESPAWN_IMMEDIATELY)
        && bg.AddObject(BG_DG_DOOR_4, BG_DG_ENTRY_DOOR_4, -119.621f, 798.957f, 132.488f, 0.820303f, 0, 0, 1.43143f, 4.48416f, RESPAWN_IMMEDIATELY);
}

bool BattlegroundDeepwindGorge::SetupBattleground()
{
    _points[0] = new BotPoint(this);
    _points[1] = new MiddlePoint(this);
    _points[2] = new TopPoint(this);

    if (!AddDeepwindGates(*this)
        || !AddObject(BG_DG_CART_ALLIANCE, BG_DG_ENTRY_CART_ALLIANCE, -241.741f, 208.611f, 133.747f, 0.84278f, 0, 0, 1.93617f, 4.48416f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_DG_CART_HORDE, BG_DG_ENTRY_CART_HORDE, -91.6163f, 791.361f, 133.747f, 4.02356f, 0, 0, 1.93617f, 4.48416f, RESPAWN_IMMEDIATELY))
        return false;

    for (uint8 i = TEAM_ALLIANCE; i < MAX_TEAMS; ++i)
    {
        _carts[i] = new Cart(this);
        _carts[i]->SetTeamId(static_cast<TeamId>(i));
    }

    _carts[0]->SetGameObject(GetBgMap()->GetGameObject(BgObjects[BG_DG_CART_ALLIANCE]), BG_DG_CART_ALLIANCE);
    _carts[1]->SetGameObject(GetBgMap()->GetGameObject(BgObjects[BG_DG_CART_HORDE]), BG_DG_CART_HORDE);

    if (!AddCreature(BG_DG_ENTRY_FLAG, BG_DG_UNIT_FLAG_MID, TEAM_NEUTRAL, -167.5035f, 499.059f, 92.62903f, 1.249549f) ||
        !AddCreature(BG_DG_ENTRY_FLAG, BG_DG_UNIT_FLAG_TOP, TEAM_NEUTRAL, 68.39236f, 431.1771f, 111.4928f, 1.229f) ||
        !AddCreature(BG_DG_ENTRY_FLAG, BG_DG_UNIT_FLAG_BOT, TEAM_NEUTRAL, -397.7726f, 574.368f, 111.0529f, 1.437866f))
        return false;

    for (uint8 i = BG_DG_UNIT_FLAG_BOT; i <= BG_DG_UNIT_FLAG_TOP; ++i)
        if (Creature* flag = GetBgMap()->GetCreature(BgCreatures[i]))
        {
            flag->AddUnitState(UNIT_STATE_CANNOT_TURN);
            flag->AddUnitState(UNIT_STATE_NOT_MOVE);
            flag->SetInt32Value(UNIT_FIELD_INTERACT_SPELL_ID, BG_DG_CAPTURE_SPELL);
            _points[i]->SetCreaturePoint(flag);
            _points[i]->UpdateState(POINT_STATE_NEUTRAL);
        }

    WorldSafeLocsEntry const* sg = sWorldSafeLocsStore.LookupEntry(BG_DG_LOC_SPIRIT_ALLIANCE_BOT);
    if (!sg || !AddSpiritGuide(BG_DG_SPIRIT_ALLIANCE_BOT, sg->Loc, TEAM_ALLIANCE))
    {
        TC_LOG_ERROR("sql.sql", "BatteGroundDG: Failed to spawn Alliance spirit guide! Battleground not created!");
        return false;
    }

    sg = sWorldSafeLocsStore.LookupEntry(BG_DG_LOC_SPIRIT_HORDE_BOT);
    if (!sg || !AddSpiritGuide(BG_DG_SPIRIT_HORDE_BOT, sg->Loc, TEAM_HORDE))
    {
        TC_LOG_ERROR("sql.sql", "BatteGroundDG: Failed to spawn Horde spirit guide! Battleground not created!");
        return false;
    }

    sg = sWorldSafeLocsStore.LookupEntry(BG_DG_LOC_SPIRIT_ALLIANCE_TOP);
    if (!sg || !AddSpiritGuide(BG_DG_SPIRIT_ALLIANCE_TOP, sg->Loc, TEAM_ALLIANCE))
    {
        TC_LOG_ERROR("sql.sql", "BatteGroundDG: Failed to spawn Alliance spirit guide! Battleground not created!");
        return false;
    }

    sg = sWorldSafeLocsStore.LookupEntry(BG_DG_LOC_SPIRIT_HORDE_TOP);
    if (!sg || !AddSpiritGuide(BG_DG_SPIRIT_HORDE_TOP, sg->Loc, TEAM_HORDE))
    {
        TC_LOG_ERROR("sql.sql", "BatteGroundDG: Failed to spawn Horde spirit guide! Battleground not created!");
        return false;
    }

    return true;
}

uint8 BattlegroundDeepwindGorge::_GetCapturedNodesForTeam(TeamId teamID)
{
    uint8 nodes = 0;

    for (uint8 i = BG_DG_UNIT_FLAG_BOT; i <= BG_DG_UNIT_FLAG_TOP; ++i)
    {
        if (!_points[i])
            return nodes;

        if (teamID == TEAM_ALLIANCE)
            if (_points[i]->GetState() == POINT_STATE_CAPTURED_ALLIANCE)
                ++nodes;
        
        if (teamID == TEAM_HORDE)
            if (_points[i]->GetState() == POINT_STATE_CAPTURED_HORDE)
                ++nodes;
    }

    return nodes;
}

BattlegroundDeepwindGorge::Point::Point(BattlegroundDeepwindGorge* bg) : m_prevAura(0), m_bg(bg)
{
    m_state = POINT_STATE_NEUTRAL;
    m_timer = Milliseconds(0);
    m_goldCredit = 0;
    m_currentWorldState = std::make_pair(WorldStates::WS_NONE, 0);
    m_point = nullptr;
}

BattlegroundDeepwindGorge::Point::~Point() = default;

void BattlegroundDeepwindGorge::Point::UpdateState(PointStates state)
{
    if (m_prevAura)
    {
        m_point->RemoveAura(m_prevAura);
        m_prevAura = 0;
    }

    switch (state)
    {
        case POINT_STATE_CONTESTED_ALLIANCE:
            m_point->AddAura(BG_DG_AURA_ALLIANCE_CONTEST, m_point);
            m_prevAura = BG_DG_AURA_ALLIANCE_CONTEST;
            m_timer = Seconds(40);
            GetBg()->PlaySoundToAll(m_state == POINT_STATE_NEUTRAL ? BG_SOUND_FLAG_RESET : BG_SOUND_CAPTURE_POINT_ASSAULT_HORDE);
            break;
        case POINT_STATE_CONTESTED_HORDE:
            m_point->AddAura(BG_DG_AURA_HORDE_CONTEST, m_point);
            m_prevAura = BG_DG_AURA_HORDE_CONTEST;
            m_timer = Seconds(40);
            GetBg()->PlaySoundToAll(m_state == POINT_STATE_NEUTRAL ? BG_SOUND_FLAG_RESET : BG_SOUND_CAPTURE_POINT_ASSAULT_ALLIANCE);
            break;
        case POINT_STATE_CAPTURED_ALLIANCE:
            m_point->AddAura(BG_DG_AURA_ALLIANCE_CATURED, m_point);
            m_prevAura = BG_DG_AURA_ALLIANCE_CATURED;
            GetBg()->PlaySoundToAll(BG_SOUND_CAPTURE_POINT_CAPTURED_ALLIANCE);
            break;
        case POINT_STATE_CAPTURED_HORDE:
            m_point->AddAura(BG_DG_AURA_HORDE_CAPTURED, m_point);
            m_prevAura = BG_DG_AURA_HORDE_CAPTURED;
            GetBg()->PlaySoundToAll(BG_SOUND_CAPTURE_POINT_CAPTURED_HORDE);
            break;
        case POINT_STATE_NEUTRAL:
            m_point->AddAura(BG_DG_AURA_NEUTRAL, m_point);
            m_prevAura = BG_DG_AURA_NEUTRAL;
            break;
        default:
            break;
    }

    m_state = state;

    GetBg()->UpdateWorldState(WorldStates::DG_ALLIANCE_NODES, GetBg()->_GetCapturedNodesForTeam(TEAM_ALLIANCE));
    GetBg()->UpdateWorldState(WorldStates::DG_HORDE_NODES, GetBg()->_GetCapturedNodesForTeam(TEAM_HORDE));
}

void BattlegroundDeepwindGorge::Point::PointClicked(Player* player)
{
    PointStates newState = player->GetBGTeamId() == TEAM_ALLIANCE ? POINT_STATE_CONTESTED_ALLIANCE : POINT_STATE_CONTESTED_HORDE;
    if (newState == m_state || newState + 2 == m_state)
        return;

    if (m_state == POINT_STATE_NEUTRAL || m_state == POINT_STATE_CAPTURED_ALLIANCE || m_state == POINT_STATE_CAPTURED_HORDE)
        GetBg()->UpdatePlayerScore(player, SCORE_POINTS_CAPTURED, 1, false);
    else
        GetBg()->UpdatePlayerScore(player, SCORE_POINTS_DEFENDED, 1, false);

    UpdateState(newState);
}

void BattlegroundDeepwindGorge::Point::Update(Milliseconds diff)
{
    if (m_state == POINT_STATE_CONTESTED_ALLIANCE || m_state == POINT_STATE_CONTESTED_HORDE)
    {
        if (m_timer < Milliseconds(0))
        {
            m_timer = Milliseconds(0);
            UpdateState(m_state == POINT_STATE_CONTESTED_ALLIANCE ? POINT_STATE_CAPTURED_ALLIANCE : POINT_STATE_CAPTURED_HORDE);
        }
        else
            m_timer -= diff;
    }
}

void BattlegroundDeepwindGorge::TopPoint::UpdateState(PointStates state)
{
    PointStates oldstate = m_state;
    Point::UpdateState(state);

    if (m_currentWorldState.first != WorldStates::WS_NONE)
        GetBg()->UpdateWorldState(m_currentWorldState.first, !m_currentWorldState.second);

    switch (state)
    {
        case POINT_STATE_CONTESTED_ALLIANCE:
            if (oldstate == POINT_STATE_NEUTRAL)
                GetBg()->UpdateWorldState(WorldStates::DG_SHOW_PANDAREN_MINE_ICON, 0);
            GetBg()->UpdateWorldState(WorldStates::DG_PANDAREN_MINE_ALLIANCE_ASSAULT, 1);
            m_currentWorldState = std::make_pair(WorldStates::DG_PANDAREN_MINE_ALLIANCE_ASSAULT, 1);
            break;
        case POINT_STATE_CONTESTED_HORDE:
            if (oldstate == POINT_STATE_NEUTRAL)
                GetBg()->UpdateWorldState(WorldStates::DG_SHOW_PANDAREN_MINE_ICON, 0);
            GetBg()->UpdateWorldState(WorldStates::DG_PANDAREN_MINE_HORDE_ASSAULT, 1);
            m_currentWorldState = std::make_pair(WorldStates::DG_PANDAREN_MINE_HORDE_ASSAULT, 1);
            break;
        case POINT_STATE_CAPTURED_ALLIANCE:
            GetBg()->UpdateWorldState(WorldStates::DG_PANDAREN_MINE_CAPTURED_BY_TEAM, 2);
            m_currentWorldState = std::make_pair(WorldStates::DG_PANDAREN_MINE_CAPTURED_BY_TEAM, 2);
            break;
        case POINT_STATE_CAPTURED_HORDE:
            GetBg()->UpdateWorldState(WorldStates::DG_PANDAREN_MINE_CAPTURED_BY_TEAM, 1);
            m_currentWorldState = std::make_pair(WorldStates::DG_PANDAREN_MINE_CAPTURED_BY_TEAM, 1);
            break;
        default:
            break;
    }
}

void BattlegroundDeepwindGorge::BotPoint::UpdateState(PointStates state)
{
    PointStates oldstate = m_state;
    Point::UpdateState(state);

    if (m_currentWorldState.first != WorldStates::WS_NONE)
        GetBg()->UpdateWorldState(m_currentWorldState.first, !m_currentWorldState.second);

    switch (state)
    {
        case POINT_STATE_CONTESTED_ALLIANCE:
            if (oldstate == POINT_STATE_NEUTRAL)
                GetBg()->UpdateWorldState(WorldStates::DG_SHOW_GOBLIN_MINE_ICON, 0);
            GetBg()->UpdateWorldState(WorldStates::DG_GOBLIN_MINE_ALLIANCE_ASSAULT, 1);
            m_currentWorldState = std::make_pair(WorldStates::DG_GOBLIN_MINE_ALLIANCE_ASSAULT, 1);
            break;
        case POINT_STATE_CONTESTED_HORDE:
            if (oldstate == POINT_STATE_NEUTRAL)
                GetBg()->UpdateWorldState(WorldStates::DG_SHOW_GOBLIN_MINE_ICON, 0);
            GetBg()->UpdateWorldState(DG_GOBLIN_MINE_HORDE_ASSAULT, 1);
            m_currentWorldState = std::make_pair(static_cast<WorldStates>(DG_GOBLIN_MINE_HORDE_ASSAULT), 1);
            break;
        case POINT_STATE_CAPTURED_ALLIANCE:
            GetBg()->UpdateWorldState(WorldStates::DG_GOBLIN_MINE_CAPTURED_BY_TEAM, 2);
            m_currentWorldState = std::make_pair(WorldStates::DG_GOBLIN_MINE_CAPTURED_BY_TEAM, 2);
            break;
        case POINT_STATE_CAPTURED_HORDE:
            GetBg()->UpdateWorldState(WorldStates::DG_GOBLIN_MINE_CAPTURED_BY_TEAM, 1);
            m_currentWorldState = std::make_pair(WorldStates::DG_GOBLIN_MINE_CAPTURED_BY_TEAM, 1);
            break;
        default:
            break;
    }
}

void BattlegroundDeepwindGorge::MiddlePoint::UpdateState(PointStates state)
{
    PointStates oldstate = m_state;
    Point::UpdateState(state);

    if (m_currentWorldState.first != WorldStates::WS_NONE)
        GetBg()->UpdateWorldState(m_currentWorldState.first, !m_currentWorldState.second);

    switch (state)
    {
        case POINT_STATE_CONTESTED_ALLIANCE:
            if (oldstate == POINT_STATE_NEUTRAL)
                GetBg()->UpdateWorldState(WorldStates::DG_SHOW_MIDDLE_MINE_ICON, 0);

            GetBg()->UpdateWorldState(WorldStates::DG_MIDDLE_MINE_ALLIANCE_ASSAULT, 1);
            m_currentWorldState = std::make_pair(WorldStates::DG_MIDDLE_MINE_ALLIANCE_ASSAULT, 1);
            break;
        case POINT_STATE_CONTESTED_HORDE:
            if (oldstate == POINT_STATE_NEUTRAL)
                GetBg()->UpdateWorldState(WorldStates::DG_SHOW_MIDDLE_MINE_ICON, 0);

            GetBg()->UpdateWorldState(DG_MIDDLE_MINE_HORDE_ASSAULT, 1);
            m_currentWorldState = std::make_pair(static_cast<WorldStates>(DG_MIDDLE_MINE_HORDE_ASSAULT), 1);
            break;
        case POINT_STATE_CAPTURED_ALLIANCE:
            GetBg()->UpdateWorldState(WorldStates::DG_MIDDLE_MINE_CAPTURED_BY_TEAM, 2);
            m_currentWorldState = std::make_pair(WorldStates::DG_MIDDLE_MINE_CAPTURED_BY_TEAM, 2);
            break;
        case POINT_STATE_CAPTURED_HORDE:
            GetBg()->UpdateWorldState(WorldStates::DG_MIDDLE_MINE_CAPTURED_BY_TEAM, 1);
            m_currentWorldState = std::make_pair(WorldStates::DG_MIDDLE_MINE_CAPTURED_BY_TEAM, 1);
            break;
        default:
            break;
    }
}

BattlegroundDeepwindGorge::Cart::Cart(BattlegroundDeepwindGorge* bg) : m_bg(bg), m_goBgId(0)
{
    m_controlledBy.Clear();
    m_team = TEAM_NEUTRAL;
    m_goCart = nullptr;
    m_playerDroppedCart.Clear();
    m_stolenGold = 0;
}

void BattlegroundDeepwindGorge::Cart::ToggleCaptured(Player* player)
{
    if (!m_controlledBy.IsEmpty())
        return;

    uint32 cartAuraId[MAX_TEAMS] = {BG_DG_AURA_CART_HORDE, BG_DG_AURA_CART_ALLIANCE};
    WorldStates flagState[MAX_TEAMS] = {WorldStates::DG_ALLIANCE_CART_ASSAULT, WorldStates::DG_HORDE_CART_ASSAULT};
    uint32 cartEntry[MAX_TEAMS] = {71073, 71071};
    TeamId teamID = player->GetBGTeamId();

    GetBg()->PlayeCapturePointSound(NODE_STATUS_ASSAULT, teamID);

    CellCoord p(Trinity::ComputeCellCoord(player->GetPositionX(), player->GetPositionY()));
    Cell cell(p);
    cell.SetNoCreate();

    Creature* cart = nullptr;
    Trinity::AllCreaturesOfEntryInRange check(player, cartEntry[teamID], SIZE_OF_GRIDS);
    Trinity::CreatureSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(player, cart, check);

    cell.Visit(p, Trinity::makeGridVisitor(searcher), *(player->GetMap()), *player, SIZE_OF_GRIDS);

    if (cart)
    {
        cart->GetMotionMaster()->MoveFollow(player, 2.f, 0.f);
        cart->CastSpell(player, BG_DG_AURA_CARTS_CHAINS, true);
        cart->AddAura(cartAuraId[teamID], cart);
        // WTF? It's already set creature type, or u need set 0x10000? ->SetUInt16Value(OBJECT_FIELD_TYPE, 1, 1);
        // This nothing change before!
        //cart->SetUInt16Value(OBJECT_FIELD_TYPE, 0, 65545);
        cart->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_PACIFIED);
        cart->setFaction(35);
        cart->SetSpeed(MOVE_RUN, 3.f);

        GetBg()->UpdateWorldState(flagState[teamID], 2);

        m_controlledBy = player->GetGUID();

        uint32 goldBeffore = GetBg()->m_TeamScores[GetTeamId()];
        GetBg()->ModGold(GetTeamId(), -200);
        m_stolenGold = goldBeffore - GetBg()->m_TeamScores[GetTeamId()];
    }
}

void BattlegroundDeepwindGorge::Cart::CartDropped()
{
    m_playerDroppedCart = m_controlledBy;
    UnbindCartFromPlayer();
    GetBg()->ModGold(GetTeamId(), m_stolenGold);
    m_stolenGold = 0;
}

Player* BattlegroundDeepwindGorge::Cart::ControlledBy()
{
    return ObjectAccessor::FindPlayer(m_controlledBy);
}

void BattlegroundDeepwindGorge::Cart::CartDelivered()
{
    Player* player = ControlledBy();
    auto teamID = player->GetBGTeamId();

    GetBg()->UpdatePlayerScore(player, SCORE_CARTS_CAPTURED, 1, false);
    GetBg()->ModGold(teamID, m_stolenGold);
    m_stolenGold = 0;
    UnbindCartFromPlayer();

    player->UpdateAchievementCriteria(CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, DG_OBJECTIVE_CAPTURE_CART, 1);
    GetBg()->PlayeCapturePointSound(NODE_STATUS_CAPTURE, teamID);
}

void BattlegroundDeepwindGorge::Cart::UnbindCartFromPlayer()
{
    Player* player = ControlledBy();
    Unit* cart = nullptr;
    if (player)
        if (Aura* aura = player->GetAura(BG_DG_AURA_CARTS_CHAINS))
            if (Unit* unit = aura->GetCaster())
                cart = unit;

    if (cart && player)
    {
        player->RemoveAura(BG_DG_AURA_PLAYER_FLAG_HORDE);
        player->RemoveAura(BG_DG_AURA_PLAYER_FLAG_ALLIANCE);
        cart->ToCreature()->DespawnOrUnsummon();
        GetBg()->SpawnBGObject(m_goBgId, 0);
        m_controlledBy.Clear();
        GetBg()->UpdateWorldState(player->GetBGTeamId() == TEAM_ALLIANCE ? WorldStates::DG_ALLIANCE_CART_ASSAULT : WorldStates::DG_HORDE_CART_ASSAULT, 1);
    }
}

void BattlegroundDeepwindGorge::GetPlayerPositionData(std::vector<WorldPackets::Battleground::BattlegroundPlayerPosition>* positions) const
{
    if (_carts[TEAM_ALLIANCE])
    if (Player* player = _carts[TEAM_ALLIANCE]->ControlledBy())
    {
        WorldPackets::Battleground::BattlegroundPlayerPosition position;
        position.Guid = player->GetGUID();
        position.Pos = player->GetPosition();
        position.IconID = PLAYER_POSITION_ICON_ALLIANCE_FLAG;
        position.ArenaSlot = PLAYER_POSITION_ARENA_SLOT_NONE;
        positions->push_back(position);
    }

    if (_carts[TEAM_HORDE])
    if (Player* player = _carts[TEAM_HORDE]->ControlledBy())
    {
        WorldPackets::Battleground::BattlegroundPlayerPosition position;
        position.Guid = player->GetGUID();
        position.Pos = player->GetPosition();
        position.IconID = PLAYER_POSITION_ICON_HORDE_FLAG;
        position.ArenaSlot = PLAYER_POSITION_ARENA_SLOT_NONE;
        positions->push_back(position);
    }
}

namespace
{
enum DunkObjects : uint32
{
    BallHome = BG_DG_CART_ALLIANCE,
    BallDrop = BallHome + 3,
    Basket = BallDrop + 3,
    DunkObjectCount = Basket + MAX_TEAMS
};

struct DunkballPlacement
{
    uint32 HomeEntry;
    uint32 DropEntry;
    uint32 Aura;
    Position Home;
};

std::array<DunkballPlacement, 3> const Dunkballs =
{{
    {266600, 266599, 231529, {-167.5035f, 499.059f, 92.62903f, 1.249549f}},
    {266679, 266678, 231814, {68.39236f, 431.1771f, 111.4928f, 1.229f}},
    {266681, 266680, 231813, {-397.7726f, 574.368f, 111.0529f, 1.437866f}}
}};

constexpr uint32 LongRangeAction = 231658;
constexpr uint32 DunkRangeAction = 231660;
constexpr uint32 DunkScoreLimit = 1500;
constexpr float DunkRange = 5.0f;
constexpr float ShotRange = 60.0f;

struct DunkScore : BattlegroundScore
{
    DunkScore(ObjectGuid guid, TeamId team) : BattlegroundScore(guid, team) { }
    void BuildObjectivesBlock(std::vector<int32>&) override { }
};
}

BattlegroundDeepwindDunk::BattlegroundDeepwindDunk()
{
    BgObjects.resize(DunkObjectCount);
    BgCreatures.resize(MAX_TEAMS);
}

void BattlegroundDeepwindDunk::Reset()
{
    Battleground::Reset();
    _balls = {};
    _elapsed = Milliseconds::zero();
}

BattlegroundDeepwindDunk* BattlegroundDeepwindDunk::ForPlayer(Player* player)
{
    return player ? dynamic_cast<BattlegroundDeepwindDunk*>(player->GetBattleground()) : nullptr;
}

bool BattlegroundDeepwindDunk::SetupBattleground()
{
    if (!AddDeepwindGates(*this))
        return false;

    for (uint32 index = 0; index < Dunkballs.size(); ++index)
        if (!AddObject(BallHome + index, Dunkballs[index].HomeEntry, Dunkballs[index].Home, G3D::Quat()))
            return false;

    if (!AddObject(Basket + TEAM_ALLIANCE, 267282, Position(-241.976f, 208.649f, 133.746f, 0.84278f), G3D::Quat()) ||
        !AddObject(Basket + TEAM_HORDE, 267283, Position(-91.5573f, 791.441f, 133.747f, 4.02356f), G3D::Quat()))
        return false;

    for (TeamId team : {TEAM_ALLIANCE, TEAM_HORDE})
    {
        auto const* graveyard = sWorldSafeLocsStore.LookupEntry(team == TEAM_ALLIANCE ? BG_DG_LOC_SPIRIT_ALLIANCE_BOT : BG_DG_LOC_SPIRIT_HORDE_BOT);
        if (!graveyard || !AddSpiritGuide(team, graveyard->Loc, team))
            return false;
    }
    return true;
}

void BattlegroundDeepwindDunk::StartingEventCloseDoors()
{
    for (uint32 gate = BG_DG_DOOR_1; gate <= BG_DG_DOOR_4; ++gate)
        SpawnBGObject(gate, RESPAWN_IMMEDIATELY);
}

void BattlegroundDeepwindDunk::StartingEventOpenDoors()
{
    for (uint32 gate = BG_DG_DOOR_1; gate <= BG_DG_DOOR_4; ++gate)
        DoorOpen(gate);
}

void BattlegroundDeepwindDunk::AddPlayer(Player* player)
{
    Battleground::AddPlayer(player);
    PlayerScores[player->GetGUID()] = new DunkScore(player->GetGUID(), player->GetBGTeamId());
    player->SendDirectMessage(WorldPackets::Battleground::Init(DunkScoreLimit).Write());
    SendBattleGroundPoints(false, m_TeamScores[TEAM_ALLIANCE], false, player);
    SendBattleGroundPoints(true, m_TeamScores[TEAM_HORDE], false, player);
}

WorldSafeLocsEntry const* BattlegroundDeepwindDunk::GetClosestGraveYard(Player* player)
{
    return sWorldSafeLocsStore.LookupEntry(player->GetBGTeamId() == TEAM_ALLIANCE ? BG_DG_LOC_SPIRIT_ALLIANCE_BOT : BG_DG_LOC_SPIRIT_HORDE_BOT);
}

void BattlegroundDeepwindDunk::EventPlayerClickedOnFlag(Player* player, GameObject* object, bool& canRemove)
{
    canRemove = false;
    if (GetStatus() != STATUS_IN_PROGRESS || !player->IsAlive() || !object->isSpawned() ||
        !player->IsWithinDistInMap(object, INTERACTION_DISTANCE) || !player->IsWithinLOSInMap(object))
        return;

    ObjectGuid guid = player->GetGUID();
    for (Ball const& ball : _balls)
        if (ball.Carrier == guid || ball.CapturingPlayer == guid)
            return;

    for (uint32 index = 0; index < _balls.size(); ++index)
    {
        Ball& ball = _balls[index];
        auto target = object->GetGUID();
        bool atHome = target == BgObjects[BallHome + index] && ball.Returning == Milliseconds::zero();
        bool dropped = target == BgObjects[BallDrop + index] && ball.Returning > Milliseconds::zero();
        if ((!atHome && !dropped) || !ball.Carrier.IsEmpty() || !ball.CapturingPlayer.IsEmpty())
            continue;

        ball.CapturingPlayer = guid;
        ball.CaptureTarget = target;
        if (player->CastSpell(player, CaptureSpell, false) != SPELL_CAST_OK)
        {
            ball.CapturingPlayer.Clear();
            ball.CaptureTarget.Clear();
        }
        return;
    }
}

void BattlegroundDeepwindDunk::OnObjectiveCastFinished(Player* player, uint32 spellId, bool succeeded)
{
    if (spellId == CaptureSpell)
    {
        for (uint32 index = 0; index < _balls.size(); ++index)
        {
            Ball& ball = _balls[index];
            if (ball.CapturingPlayer != player->GetGUID())
                continue;

            ObjectGuid target = ball.CaptureTarget;
            ball.CapturingPlayer.Clear();
            ball.CaptureTarget.Clear();
            if (!succeeded || GetStatus() != STATUS_IN_PROGRESS || !player->IsAlive())
                return;

            auto* object = GetBgMap()->GetGameObject(target);
            if (!object || !object->isSpawned() ||
                !player->IsWithinDistInMap(object, INTERACTION_DISTANCE) || !player->IsWithinLOSInMap(object))
                return;

            auto const& placement = Dunkballs[index];
            if (player->CastSpell(player, placement.Aura, true) != SPELL_CAST_OK || !player->HasAura(placement.Aura))
                return;

            ball.Carrier = player->GetGUID();
            ball.Returning = Milliseconds::zero();
            if (!BgObjects[BallDrop + index].IsEmpty())
                DelObject(BallDrop + index);
            SpawnBGObject(BallHome + index, RESPAWN_ONE_DAY);
            UpdateShotAction(player);
            return;
        }
    }
    else if (succeeded && GetScoringBasket(player, spellId))
    {
        for (uint32 index = 0; index < _balls.size(); ++index)
            if (_balls[index].Carrier == player->GetGUID())
            {
                ReturnBall(index, player);
                AddPoints(player->GetBGTeamId(), 100);
                return;
            }
    }
}

GameObject* BattlegroundDeepwindDunk::GetScoringBasket(Player* player, uint32 spellId)
{
    if (GetStatus() != STATUS_IN_PROGRESS || !player->IsAlive() || player->GetBGTeamId() >= MAX_TEAMS ||
        (spellId != DunkSpell && spellId != ShotSpell))
        return nullptr;

    bool carrying = false;
    for (uint32 index = 0; index < _balls.size(); ++index)
        if (_balls[index].Carrier == player->GetGUID() && player->HasAura(Dunkballs[index].Aura))
            carrying = true;
    if (!carrying)
        return nullptr;

    auto* target = GetBGObject(Basket + MS::Battlegrounds::GetOtherTeamID(player->GetBGTeamId()));
    float range = spellId == DunkSpell ? DunkRange : ShotRange;
    return target && player->IsWithinDistInMap(target, range) && player->IsWithinLOSInMap(target) ? target : nullptr;
}

void BattlegroundDeepwindDunk::UpdateShotAction(Player* carrier)
{
    bool dunk = GetScoringBasket(carrier, DunkSpell) != nullptr;
    bool shot = !dunk && GetScoringBasket(carrier, ShotSpell) != nullptr;
    if (!dunk)
        carrier->RemoveAurasDueToSpell(DunkRangeAction);
    if (!shot)
        carrier->RemoveAurasDueToSpell(LongRangeAction);
    if (dunk && !carrier->HasAura(DunkRangeAction))
        carrier->CastSpell(carrier, DunkRangeAction, true);
    else if (shot && !carrier->HasAura(LongRangeAction))
        carrier->CastSpell(carrier, LongRangeAction, true);
}

void BattlegroundDeepwindDunk::ReturnBall(uint32 index, Player* carrier)
{
    Ball& ball = _balls[index];
    ball.Carrier.Clear();
    ball.CapturingPlayer.Clear();
    ball.CaptureTarget.Clear();
    ball.Returning = Seconds(20);
    if (carrier)
    {
        carrier->RemoveAurasDueToSpell(Dunkballs[index].Aura);
        carrier->RemoveAurasDueToSpell(DunkRangeAction);
        carrier->RemoveAurasDueToSpell(LongRangeAction);
    }
}

void BattlegroundDeepwindDunk::DropBall(uint32 index, Player* carrier)
{
    ReturnBall(index, carrier);
    if (carrier && GetStatus() == STATUS_IN_PROGRESS &&
        !AddObject(BallDrop + index, Dunkballs[index].DropEntry, carrier->GetPosition(), G3D::Quat()))
        TC_LOG_ERROR("bg.battleground", "Deepwind Dunk: could not spawn dropped ball %u.", Dunkballs[index].DropEntry);
}

void BattlegroundDeepwindDunk::RemovePlayer(Player* player, ObjectGuid guid, uint32 /*team*/)
{
    for (uint32 index = 0; index < _balls.size(); ++index)
    {
        Ball& ball = _balls[index];
        if (ball.Carrier == guid)
            ReturnBall(index, player);
        if (ball.CapturingPlayer == guid)
        {
            ball.CapturingPlayer.Clear();
            ball.CaptureTarget.Clear();
        }
    }
}

void BattlegroundDeepwindDunk::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS || !player)
        return;

    bool enemyKill = killer && IsPlayerInBattleground(killer->GetGUID()) && player->GetBGTeam() != killer->GetBGTeam();
    Battleground::HandleKillPlayer(player, enemyKill ? killer : nullptr);
    for (uint32 index = 0; index < _balls.size(); ++index)
        if (_balls[index].Carrier == player->GetGUID())
            DropBall(index, player);
    if (enemyKill)
        AddPoints(killer->GetBGTeamId(), 5);
}

void BattlegroundDeepwindDunk::AddPoints(TeamId team, uint32 points)
{
    auto& score = m_TeamScores[team];
    score = std::min(score + points, DunkScoreLimit);
    SendBattleGroundPoints(team == TEAM_HORDE, score);
    if (score == DunkScoreLimit)
        EndBattleground(MS::Battlegrounds::GetTeamByTeamId(team));
}

void BattlegroundDeepwindDunk::EndBattleground(uint32 victor)
{
    for (uint32 index = 0; index < _balls.size(); ++index)
        if (!_balls[index].Carrier.IsEmpty())
            ReturnBall(index, ObjectAccessor::FindPlayer(_balls[index].Carrier));
    Battleground::EndBattleground(victor);
}

void BattlegroundDeepwindDunk::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    _elapsed += Milliseconds(diff);
    if (_elapsed >= Minutes(15))
    {
        BattlegroundTimedWin();
        return;
    }

    for (uint32 index = 0; index < _balls.size(); ++index)
    {
        Ball& ball = _balls[index];
        if (!ball.Carrier.IsEmpty())
        {
            Player* carrier = ObjectAccessor::FindPlayer(ball.Carrier);
            if (!carrier || carrier->GetBattleground() != this)
                ReturnBall(index, carrier);
            else if (!carrier->IsAlive() || !carrier->HasAura(Dunkballs[index].Aura))
                DropBall(index, carrier);
            else
                UpdateShotAction(carrier);
        }
        else if (ball.Returning > Milliseconds::zero())
        {
            ball.Returning -= std::min(ball.Returning, Milliseconds(diff));
            if (ball.Returning == Milliseconds::zero())
            {
                if (!BgObjects[BallDrop + index].IsEmpty())
                    DelObject(BallDrop + index);
                ball.CapturingPlayer.Clear();
                ball.CaptureTarget.Clear();
                SpawnBGObject(BallHome + index, RESPAWN_IMMEDIATELY);
            }
        }
    }
}
