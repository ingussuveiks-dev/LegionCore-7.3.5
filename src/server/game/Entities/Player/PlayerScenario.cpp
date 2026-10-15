#include "Player.h"
#include "LFGMgr.h"
#include "MapManager.h"
#include "ScenarioMgr.h"
#include "WorldSession.h"

namespace
{
bool Resumable(MapEntry const* entry, uint32 scenarioId)
{
    ScenarioEntry const* scenario = sScenarioStore.LookupEntry(scenarioId);
    return entry && entry->IsScenario() && !entry->IsBattlegroundOrArena() &&
        !entry->IsGarrison() && entry->ID != 1191 && scenario &&
        scenario->Type != SCENARIO_TYPE_CHALLENGE_MODE;
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
    auto data = sLFGMgr->GetLFGDungeon(uint16(m_scenarioResume.scenario),uint16(mapId));
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
            auto data = sLFGMgr->GetLFGDungeon(uint16(progress->GetScenarioId()),uint16(GetMapId()));
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
