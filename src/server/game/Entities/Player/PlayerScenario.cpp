#include "Player.h"
#include "LFGMgr.h"
#include "MapManager.h"
#include "ScenarioMgr.h"
#include "WorldSession.h"
#include "ObjectMgr.h"
#include "Creature.h"
#include "SpellMgr.h"
#include "BrokenShoreClassRoute.h"

namespace
{
bool Resumable(MapEntry const* entry, uint32 scenarioId)
{
    ScenarioEntry const* scenario = sScenarioStore.LookupEntry(scenarioId);
    return entry && entry->IsScenario() && !entry->IsBattlegroundOrArena() &&
        !entry->IsGarrison() && entry->ID != 1191 && scenario &&
        scenario->Type != SCENARIO_TYPE_CHALLENGE_MODE;
}

lfg::LFGDungeonData const* ResumeDungeon(Player const* player, uint32 scenarioId, uint32 mapId)
{
    if (BrokenShoreClassRoute::Applies(mapId, player->getClass()))
    {
        auto difficulty = sDB2Manager.GetDefaultMapDifficulty(mapId);
        return difficulty ? BrokenShoreClassRoute::Dungeon(player, Difficulty(difficulty->DifficultyID), scenarioId) : nullptr;
    }
    if (auto data = sLFGMgr->GetLFGDungeon(uint16(scenarioId),uint16(mapId)))
        return data;

    // A faction/class scenario can share another scenario's native LFG row.
    // Resolve the same map route used at construction, without accepting an
    // arbitrary checkpoint for another faction, class or dungeon.
    auto difficulty = sDB2Manager.GetDefaultMapDifficulty(mapId);
    if (!difficulty)
        return nullptr;
    auto data = sLFGMgr->GetLFGDungeon(mapId,Difficulty(difficulty->DifficultyID),player->GetTeam());
    if (!data)
        return nullptr;
    auto route = sObjectMgr->GetScenarioOnMap(mapId,data->difficulty,player->GetTeam(),player->getClass(),data->id);
    if (!route || route->ScenarioID != scenarioId ||
        (route->Team && route->Team != player->GetTeam()) ||
        (route->Class && route->Class != player->getClass()) ||
        (route->DifficultyID && route->DifficultyID != data->difficulty) ||
        (route->LfgDungeonID && route->LfgDungeonID != data->id))
        return nullptr;
    return data;
}
}

void Player::PrepareScenarioLogin(uint32 mapId, uint32 instanceId)
{
    MapEntry const* entry = sMapStore.LookupEntry(mapId);
    if (!m_scenarioResume.scenario || m_scenarioResume.map != mapId ||
        !Resumable(entry,m_scenarioResume.scenario) ||
        !MapManager::IsValidMapCoord(mapId,m_scenarioResume.x,m_scenarioResume.y,m_scenarioResume.z,m_scenarioResume.o))
    {
        m_scenarioResume = {};
        return;
    }
    // Native scenario/map routing must still exist after a data update.
    auto data = ResumeDungeon(this,m_scenarioResume.scenario,mapId);
    if (!data || !sScenarioMgr->GetScenarioSteps(m_scenarioResume.scenario))
    {
        m_scenarioResume = {};
        return;
    }
    m_scenarioLoginId = m_scenarioResume.scenario;
    SetScenarioId(uint16(m_scenarioLoginId));
    Scenario* live = m_scenarioResume.boot == uint64(GameTime::GetStartTime()) &&
        instanceId == m_scenarioResume.instance ? sScenarioMgr->GetScenario(instanceId) : nullptr;
    if (live && live->GetMap()->GetId() == mapId && live->GetScenarioId() == m_scenarioLoginId)
    {
        if (live->IsCompleted(false))
        {
            m_scenarioResume = {};
            m_scenarioLoginId = 0;
            SetScenarioId(0);
            return;
        }
        m_scenarioLoginInstance = instanceId;
        TC_LOG_INFO("entities.player", "Scenario resume: player %u rejoins live scenario %u instance %u",GetGUIDLow(),m_scenarioLoginId,instanceId);
        return;
    }
    // Criteria and actors are transient. Never restore an arbitrary later
    // coordinate into a fresh instance that is starting at step zero.
    Relocate(m_scenarioResume.x,m_scenarioResume.y,m_scenarioResume.z,m_scenarioResume.o);
    TC_LOG_INFO("entities.player", "Scenario resume: player %u restarts scenario %u at its recorded entrance",GetGUIDLow(),m_scenarioLoginId);
}

bool Player::CanResumeScenarioLogin(uint32 scenarioId) const
{
    return m_scenarioLoginId && m_scenarioLoginId == scenarioId &&
        m_scenarioResume.map == GetMapId() && Resumable(GetMap()->GetEntry(),scenarioId);
}

void Player::RecordScenarioEntrance()
{
    if (!IsInWorld() || !GetMap()) return;
    Scenario* progress = GetMap()->IsScenario() ? sScenarioMgr->GetScenario(GetInstanceId()) : nullptr;
    if (!progress || !Resumable(GetMap()->GetEntry(),progress->GetScenarioId()) || progress->IsCompleted(false))
    {
        m_scenarioResume = {};
        m_scenarioLoginId = m_scenarioLoginInstance = 0;
        return;
    }
    if (m_scenarioResume.map != GetMapId() || m_scenarioResume.scenario != progress->GetScenarioId())
    {
        Position entrance = GetPosition();
        // A legacy live run may first be recorded in a later wing or aboard
        // a transport. Use its native LFG entrance in that case.
        if (GetSession()->PlayerLoading() || progress->GetCurrentStep() || GetTransport() || GetVehicle())
        {
            auto data = ResumeDungeon(this,progress->GetScenarioId(),GetMapId());
            if (!data || (!data->x && !data->y && !data->z) ||
                !MapManager::IsValidMapCoord(GetMapId(),data->x,data->y,data->z,data->o)) return;
            entrance.Relocate(data->x,data->y,data->z,data->o);
        }
        if (!MapManager::IsValidMapCoord(GetMapId(),entrance.GetPositionX(),entrance.GetPositionY(),entrance.GetPositionZ(),entrance.GetOrientation())) return;
        m_scenarioResume.map = GetMapId();
        m_scenarioResume.scenario = progress->GetScenarioId();
        m_scenarioResume.x = entrance.GetPositionX();
        m_scenarioResume.y = entrance.GetPositionY();
        m_scenarioResume.z = entrance.GetPositionZ();
        m_scenarioResume.o = entrance.GetOrientation();
    }
    m_scenarioResume.instance = GetInstanceId();
    m_scenarioResume.boot = uint64(GameTime::GetStartTime());
    if (m_scenarioLoginId == progress->GetScenarioId()) SetScenarioId(0);
}

bool Player::ExitCompletedBrokenShoreLogin()
{
    uint32 const questId = GetTeam() == ALLIANCE ? 42740 : 40518;
    if (GetMapId() != 1460 ||
        (m_scenarioLoginId && m_scenarioLoginId != (GetTeam() == ALLIANCE ? 786 : 1189)) ||
        (GetQuestStatus(questId) != QUEST_STATUS_COMPLETE && !IsQuestRewarded(questId)))
        return false;

    // Use the same destination as the native finale, even if its delayed
    // teleport was cancelled by logout. The quest remains available to turn in.
    auto destination = sSpellMgr->GetSpellTargetPosition(GetTeam() == ALLIANCE ? 208514 : 208517);
    if (!destination || !MapManager::IsValidMapCoord(destination->target_mapId,
        destination->target_X, destination->target_Y, destination->target_Z, destination->target_Orientation) ||
        !TeleportTo(destination->target_mapId, destination->target_X, destination->target_Y,
            destination->target_Z, destination->target_Orientation))
        return false;

    m_scenarioResume = {};
    m_scenarioLoginId = m_scenarioLoginInstance = 0;
    SetScenarioId(0);
    return true;
}

bool Player::ExitCompletedDarkRidersLogin()
{
    // Completed scenarios intentionally have no restart checkpoint. The final
    // chest's persisted claim still proves this run reached its end, even if
    // an older server omitted the second quest credit. Return to the outdoor
    // quest ender instead of recreating the finale or using the homebind.
    if (GetMapId() != 1533 || getClass() != CLASS_WARLOCK ||
        (m_scenarioLoginId && m_scenarioLoginId != 988) || IsQuestRewarded(40623) ||
        (GetQuestStatus(40623) != QUEST_STATUS_INCOMPLETE && GetQuestStatus(40623) != QUEST_STATUS_COMPLETE) ||
        GetQuestObjectiveData(40623, 103473) < 1)
        return false;

    CreatureData const* revil = sObjectMgr->GetCreatureData(373228);
    if (!revil || revil->id != 100812 || revil->mapid != 0 || !revil->PhaseID.empty() ||
        !MapManager::IsValidMapCoord(revil->mapid, revil->posX, revil->posY, revil->posZ, revil->orientation))
        return false;

    if (!TeleportTo(revil->mapid, revil->posX, revil->posY, revil->posZ, revil->orientation))
        return false;

    m_scenarioResume = {};
    m_scenarioLoginId = m_scenarioLoginInstance = 0;
    SetScenarioId(0);
    TC_LOG_INFO("entities.player", "Dark Riders: completed scenario login returns player %u to outdoor Revil", GetGUIDLow());
    return true;
}
