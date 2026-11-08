#include "Player.h"
#include "LFGMgr.h"
#include "MapManager.h"
#include "ScenarioMgr.h"
#include "WorldSession.h"
#include "ObjectMgr.h"
#include "Creature.h"
#include "Item.h"
#include "SpellMgr.h"
#include "BrokenShoreClassRoute.h"
#include "WarriorScenarioRoute.h"
#include "AcquisitionScenarioRoute.h"
#include "QuestData.h"

namespace
{
enum class TuureEntranceRecovery { Unchanged, Repaired, Refused };
enum class SharasdalEntranceRecovery { Unchanged, Repaired, Refused };

SharasdalEntranceRecovery RecoverLegacySharasdalEntrance(Player const* player, uint32 mapId, ScenarioResume& saved, lfg::LFGDungeonData const* data)
{
    if (mapId != 1600 || player->getClass() != CLASS_SHAMAN || saved.map != 1600 || saved.scenario != 1066 ||
        !(std::fabs(saved.x - 836.16f) < 0.001f && std::fabs(saved.y - 1035.4f) < 0.001f &&
          std::fabs(saved.z - 48.61f) < 0.001f && std::fabs(saved.o - 1.7935f) < 0.001f))
        return SharasdalEntranceRecovery::Unchanged;

    // Only the historical1229 tuple is repairable. Native207683 is the real
    // acquisition entrance; the separate5443 graveyard is not a travel spell.
    auto target = sSpellMgr->GetSpellTargetPosition(207683);
    if (!saved.instance || !saved.boot || !data || !data->dbc || data->id != 1229 || data->map != 1600 ||
        data->difficulty != DIFFICULTY_N_SCENARIO || data->dbc->ID != 1229 || data->dbc->MapID != 1600 ||
        data->dbc->ScenarioID != 0 || data->dbc->DifficultyID != DIFFICULTY_N_SCENARIO ||
        !(data->dbc->Flags & LFG_FLAG_USER_TELEPORT_NOT_ALLOWED) || !sScenarioMgr->GetScenarioSteps(1066) ||
        !target || target->target_mapId != 1600 ||
        !(std::fabs(target->target_X - (-629.42f)) < 0.001f && std::fabs(target->target_Y - 808.68f) < 0.001f &&
          std::fabs(target->target_Z - 247.67f) < 0.001f && std::fabs(target->target_Orientation - 6.2797f) < 0.001f) ||
        !MapManager::IsValidMapCoord(1600, data->x, data->y, data->z, data->o))
        return SharasdalEntranceRecovery::Refused;
    float dx = data->x - target->target_X, dy = data->y - target->target_Y, dz = data->z - target->target_Z;
    if (dx * dx + dy * dy + dz * dz >= 0.0001f || !(std::fabs(data->o - target->target_Orientation) < 0.001f))
        return SharasdalEntranceRecovery::Refused;
    saved.x = data->x; saved.y = data->y; saved.z = data->z; saved.o = data->o;
    return SharasdalEntranceRecovery::Repaired;
}

TuureEntranceRecovery RecoverLegacyTuureEntrance(Player const* player, uint32 mapId, ScenarioResume& saved, lfg::LFGDungeonData const* data)
{
    if (mapId != 1489 || player->getClass() != CLASS_PRIEST || saved.map != 1489 || saved.scenario != 1085 ||
        !(std::fabs(saved.x - (-828.85f)) < 0.001f && std::fabs(saved.y - 4372.0f) < 0.001f &&
          std::fabs(saved.z - 738.64f) < 0.001f && std::fabs(saved.o - 1.86768f) < 0.001f))
        return TuureEntranceRecovery::Unchanged;

    // Only the historical persisted1243 tuple is recoverable. Early login has
    // not loaded quests/inventory; this never creates a missing checkpoint or
    // changes ordinary quest admission, progress, instance ownership or boot.
    WorldSafeLocsEntry const* safe = sWorldSafeLocsStore.LookupEntry(5480);
    if (!saved.instance || !saved.boot || !data || !data->dbc || data->id != 1243 || data->map != 1489 ||
        data->difficulty != DIFFICULTY_N_SCENARIO || data->dbc->ID != 1243 || data->dbc->MapID != 1489 ||
        data->dbc->ScenarioID != 1085 || data->dbc->DifficultyID != DIFFICULTY_N_SCENARIO ||
        !(data->dbc->Flags & LFG_FLAG_USER_TELEPORT_NOT_ALLOWED) || !sScenarioMgr->GetScenarioSteps(1085) ||
        !safe || safe->ID != 5480 || safe->MapID != 1489 ||
        !(std::fabs(safe->Loc.X - (-43.1440964f)) < 0.001f && std::fabs(safe->Loc.Y - 1160.33679f) < 0.001f &&
          std::fabs(safe->Loc.Z - (-74.353569f)) < 0.001f && std::fabs(safe->Loc.O - 27.7250366f) < 0.001f) ||
        !MapManager::IsValidMapCoord(1489, data->x, data->y, data->z, data->o))
        return TuureEntranceRecovery::Refused;
    float dx = data->x - safe->Loc.X, dy = data->y - safe->Loc.Y, dz = data->z - safe->Loc.Z;
    // Native SQL FLOAT text output differs from DB2 by3.174mm here. Keep the
    // existing native loader and bound its geometry delta, including facing.
    if (dx * dx + dy * dy + dz * dz >= 0.0001f ||
        !(std::fabs(data->o - safe->Loc.O * 0.01745329251994329577f) < 0.001f))
        return TuureEntranceRecovery::Refused;
    saved.x = data->x; saved.y = data->y; saved.z = data->z; saved.o = data->o;
    return TuureEntranceRecovery::Repaired;
}

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
    if (auto route = AcquisitionScenarioRoute::Find(mapId, getClass()))
        if (m_scenarioResume.scenario && m_scenarioResume.scenario != route->scenario)
        {
            m_scenarioResume = {};
            m_scenarioLoginId = m_scenarioLoginInstance = 0;
            SetScenarioId(0);
            return;
        }
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
    TuureEntranceRecovery tuure = RecoverLegacyTuureEntrance(this, mapId, m_scenarioResume, data);
    SharasdalEntranceRecovery sharasdal = RecoverLegacySharasdalEntrance(this, mapId, m_scenarioResume, data);
    if (sharasdal == SharasdalEntranceRecovery::Refused)
    {
        m_scenarioResume = {};
        m_scenarioLoginId = m_scenarioLoginInstance = 0;
        SetScenarioId(0);
        TC_LOG_INFO("entities.player", "Scenario resume: refusing unsafe legacy Sharasdal entrance for player %u; receipt/source proof unavailable", GetGUIDLow());
        return;
    }
    if (tuure == TuureEntranceRecovery::Refused)
    {
        m_scenarioResume = {};
        m_scenarioLoginId = m_scenarioLoginInstance = 0;
        SetScenarioId(0);
        TC_LOG_INFO("entities.player", "Scenario resume: refusing unsafe legacy Tuure entrance for player %u; receipt/source proof unavailable", GetGUIDLow());
        return;
    }
    if (!data || !sScenarioMgr->GetScenarioSteps(m_scenarioResume.scenario))
    {
        m_scenarioResume = {};
        return;
    }
    if (tuure == TuureEntranceRecovery::Repaired)
        TC_LOG_INFO("entities.player", "Scenario resume: normalized legacy Tuure entrance for player %u", GetGUIDLow());
    if (sharasdal == SharasdalEntranceRecovery::Repaired)
        TC_LOG_INFO("entities.player", "Scenario resume: normalized legacy Sharasdal entrance for player %u", GetGUIDLow());
    m_scenarioLoginId = m_scenarioResume.scenario;
    SetScenarioId(uint16(m_scenarioLoginId));
    Scenario* live = m_scenarioResume.boot == uint64(GameTime::GetStartTime()) &&
        instanceId == m_scenarioResume.instance ? sScenarioMgr->GetScenario(instanceId) : nullptr;
    if (live && live->GetMap()->GetId() == mapId && live->GetScenarioId() == m_scenarioLoginId)
    {
        // Inventory/quest rows load later. Tentatively keep this exact saved
        // Monk outro; post-AddPlayer admission revalidates the owned pickup.
        if (live->IsCompleted(false) && !(mapId == 1541 && m_scenarioLoginId == 1007 && getClass() == CLASS_MONK))
        {
            m_scenarioResume = {};
            m_scenarioLoginId = 0;
            SetScenarioId(0);
            return;
        }
        m_scenarioLoginInstance = instanceId;
        // Preserve a legitimate live pose. A player still at the same proven
        // foreign-world XY may already be falling; recover that pose without
        // resetting this live scenario's stage, actors or instance identity.
        auto current = GetPosition();
        if (tuure == TuureEntranceRecovery::Repaired &&
            std::fabs(current.GetPositionX() - (-828.85f)) < 0.001f && std::fabs(current.GetPositionY() - 4372.0f) < 0.001f)
            Relocate(m_scenarioResume.x, m_scenarioResume.y, m_scenarioResume.z, m_scenarioResume.o);
        if (sharasdal == SharasdalEntranceRecovery::Repaired &&
            std::fabs(current.GetPositionX() - 836.16f) < 0.001f && std::fabs(current.GetPositionY() - 1035.4f) < 0.001f)
            Relocate(m_scenarioResume.x,m_scenarioResume.y,m_scenarioResume.z,m_scenarioResume.o);
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
    // Rewarded rows load after the early selector. Revalidate admission here,
    // when real active/rewarded quest state is available to the map check.
    if (auto route = AcquisitionScenarioRoute::Find(GetMapId(), getClass()))
    {
        if (scenarioId != route->scenario || GetQuestRewardStatus(route->quest) ||
            (GetQuestStatus(route->quest) != QUEST_STATUS_INCOMPLETE && GetQuestStatus(route->quest) != QUEST_STATUS_COMPLETE))
            return false;
        // The completed claim is loaded after the early selector. A new
        // instance must not replay an acquisition whose artifact was claimed.
        if (route->scenario == 973 && !m_scenarioLoginInstance && GetQuestObjectiveData(40385, 100112) >= 1)
            return false;
        // A claimed/completed entrance-only seed is return-only recovery. Its
        // actual owned artifact and all preceding receipts must survive load.
        if (route->scenario == 972 && !m_scenarioResume.instance && !m_scenarioResume.boot &&
            (GetQuestObjectiveData(40419, 103490) >= 1 || GetQuestStatus(40419) == QUEST_STATUS_COMPLETE) &&
            !HasEarnedMarksmanshipReturn())
            return false;
    }
    return m_scenarioLoginId && m_scenarioLoginId == scenarioId &&
        m_scenarioResume.map == GetMapId() && Resumable(GetMap()->GetEntry(),scenarioId);
}

void Player::RecordScenarioEntrance()
{
    if (!IsInWorld() || !GetMap()) return;
    Scenario* progress = GetMap()->IsScenario() ? sScenarioMgr->GetScenario(GetInstanceId()) : nullptr;
    if (!progress || !Resumable(GetMap()->GetEntry(),progress->GetScenarioId()) ||
        (progress->IsCompleted(false) && !(progress->GetScenarioId() == 1007 && HasPendingSheilunReturn())))
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
            if (!data) return;
            // These native LFG rows belong to Hunter/Paladin on the shared
            // map. Their entry cannot stand in for this Warrior acquisition.
            uint32 entrySpell = 0;
            if (getClass() == CLASS_WARRIOR)
            {
                if (GetMapId() == 1511 && progress->GetScenarioId() == 944) entrySpell = 193335;
                if (GetMapId() == 1539 && progress->GetScenarioId() == 1037) entrySpell = 203920;
            }
            if (entrySpell)
            {
                auto target = sSpellMgr->GetSpellTargetPosition(entrySpell);
                if (!target || target->target_mapId != GetMapId() ||
                    (!target->target_X && !target->target_Y && !target->target_Z) ||
                    !MapManager::IsValidMapCoord(GetMapId(),target->target_X,target->target_Y,target->target_Z,target->target_Orientation)) return;
                entrance.Relocate(target->target_X,target->target_Y,target->target_Z,target->target_Orientation);
            }
            else
            {
                if ((!data->x && !data->y && !data->z) ||
                    !MapManager::IsValidMapCoord(GetMapId(),data->x,data->y,data->z,data->o)) return;
                entrance.Relocate(data->x,data->y,data->z,data->o);
            }
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
    if (GetMapId() == 1500 && getClass() == CLASS_WARRIOR && (!m_scenarioLoginId || m_scenarioLoginId == 933))
        for (uint32 quest : {38904u, 42815u})
            if (GetQuestStatus(quest) == QUEST_STATUS_COMPLETE && GetQuestObjectiveData(quest, 103661))
            {
                // Real native victory was already saved; logout may only have
                // cancelled the optional eight-second rescue presentation.
                for (uint32 aura : {185627u, 185673u, 185674u}) RemoveAurasDueToSpell(aura, GetGUID());
                if (!TeleportTo(1479, 1071.81f, 7224.32f, 97.891f, 3.15048f)) return false;
                m_scenarioResume = {}; m_scenarioLoginId = m_scenarioLoginInstance = 0; SetScenarioId(0);
                return true;
            }
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

bool Player::HasPendingSheilunReturn() const
{
    if (GetMapId() != 1541 || getClass() != CLASS_MONK || isGameMaster() || IsQuestRewarded(41003) ||
        (GetQuestStatus(41003) != QUEST_STATUS_INCOMPLETE && GetQuestStatus(41003) != QUEST_STATUS_COMPLETE) ||
        GetQuestObjectiveData(41003, 102479) < 1) return false;
    Item* item = GetItemByEntry(128937, false);
    if (!item || item->GetOwnerGUID() != GetGUID() || !item->GetCount()) return false;
    Quest const* quest = sQuestDataStore->GetQuestTemplate(41003);
    if (quest) for (auto const& objective : quest->GetObjectives())
        if (objective.ID == 281599 && objective.ObjectID == 101880 && objective.StorageIndex == 2 && objective.Amount == 1)
            return GetQuestObjectiveData(quest, objective.StorageIndex) < objective.Amount;
    return false;
}

bool Player::ExitCompletedSheilunLogin()
{
    // Native232943 can prefill this quest's objectives. They alone do not
    // establish acquisition: require the actual owned inventory artifact too.
    // This rejected-login recovery routes the player; it grants no progress.
    if (GetMapId() != 1541 || getClass() != CLASS_MONK ||
        (m_scenarioLoginId && m_scenarioLoginId != 1007) ||
        (m_scenarioResume.scenario && m_scenarioResume.scenario != 1007) ||
        IsQuestRewarded(41003) ||
        (GetQuestStatus(41003) != QUEST_STATUS_INCOMPLETE && GetQuestStatus(41003) != QUEST_STATUS_COMPLETE) ||
        GetQuestObjectiveData(41003, 102479) < 1)
        return false;

    Scenario* progress = sScenarioMgr->GetScenario(GetInstanceId());
    if (progress && (progress->GetMap() != GetMap() || progress->GetScenarioId() != 1007))
        return false;

    // GetItemByEntry scans actual inventory/bag slots even before their world
    // insertion. Bag::GetItemCount deliberately excludes those loading items.
    Item* staff = GetItemByEntry(128937, false);
    if (!staff || staff->GetOwnerGUID() != GetGUID() || !staff->GetCount())
        return false;

    WorldSafeLocsEntry const* exit = sWorldSafeLocsStore.LookupEntry(5710);
    if (!exit || exit->MapID != 1514)
        return false;
    float facing = exit->Loc.O * float(M_PI / 180.0);
    if (!MapManager::IsValidMapCoord(exit->MapID, exit->Loc.X, exit->Loc.Y, exit->Loc.Z, facing) ||
        !TeleportTo(exit->MapID, exit->Loc.X, exit->Loc.Y, exit->Loc.Z, facing))
        return false;

    m_scenarioResume = {};
    m_scenarioLoginId = m_scenarioLoginInstance = 0;
    SetScenarioId(0);
    TC_LOG_INFO("entities.player", "Sheilun: login returns artifact owner %u to native Mistweaver exit", GetGUIDLow());
    return true;
}

bool Player::ExitCompletedTalonclawLogin()
{
    // A real persisted boarded arrival and manual claim may outlive the
    // instance. Return its artifact owner through the native Dalaran exit;
    // never replay combat or award the return receipt from this fallback.
    if (GetMapId() != 1511 || getClass() != CLASS_HUNTER || isGameMaster() ||
        (m_scenarioLoginId && m_scenarioLoginId != 973) || GetQuestRewardStatus(40385) ||
        (GetQuestStatus(40385) != QUEST_STATUS_INCOMPLETE && GetQuestStatus(40385) != QUEST_STATUS_COMPLETE) ||
        GetQuestObjectiveData(40385, 100111) < 1 || GetQuestObjectiveData(40385, 100112) < 1 ||
        GetQuestObjectiveData(40385, 103798) >= 1)
        return false;
    Scenario* progress = sScenarioMgr->GetScenario(GetInstanceId());
    if (progress && (progress->GetMap() != GetMap() || progress->GetScenarioId() != 973))
        return false;
    // The native slot scan also finds owned loading/bank bag items which
    // Bag::GetItemCount deliberately excludes before world insertion.
    Item* spear = GetItemByEntry(128808, true);
    if (!spear || spear->GetOwnerGUID() != GetGUID() || !spear->GetCount())
        return false;
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(190598);
    SpellTargetPosition const* exit = sSpellMgr->GetSpellTargetPosition(190598);
    if (!spell || !spell->HasEffect(SPELL_EFFECT_TELEPORT_L) || !exit || exit->target_mapId != 1220 ||
        !MapManager::IsValidMapCoord(exit->target_mapId, exit->target_X, exit->target_Y, exit->target_Z, exit->target_Orientation) ||
        sMapMgr->GetZoneId(exit->target_mapId, exit->target_X, exit->target_Y, exit->target_Z) != 7502)
        return false;
    if (IsBeingTeleported())
        return IsBeingTeleportedFar() && GetTeleportDest().GetMapId() == exit->target_mapId &&
            GetTeleportDest().GetExactDist(exit->target_X, exit->target_Y, exit->target_Z) < 0.01f;
    if (!TeleportTo(exit->target_mapId, exit->target_X, exit->target_Y, exit->target_Z, exit->target_Orientation))
        return false;
    RemoveAurasDueToSpell(205589);
    m_scenarioResume = {};
    m_scenarioLoginId = m_scenarioLoginInstance = 0;
    SetScenarioId(0);
    TC_LOG_INFO("entities.player", "Talonclaw: login returns claimed artifact owner %u through native Dalaran exit", GetGUIDLow());
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

bool Player::HasEarnedMarksmanshipReturn() const
{
    if (GetMapId() != 1489 || getClass() != CLASS_HUNTER || isGameMaster() ||
        GetQuestRewardStatus(40419) ||
        (GetQuestStatus(40419) != QUEST_STATUS_INCOMPLETE && GetQuestStatus(40419) != QUEST_STATUS_COMPLETE))
        return false;
    for (uint32 objective : {102128u, 103481u, 102129u, 102131u, 103490u})
        if (GetQuestObjectiveData(40419, objective) < 1)
            return false;
    // Loading bag items are deliberately invisible to Bag::GetItemCount.
    Item* bow = GetItemByEntry(128826, true);
    return bow && bow->GetOwnerGUID() == GetGUID() && bow->GetCount();
}
