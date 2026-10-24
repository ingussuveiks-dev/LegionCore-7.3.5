#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using QuestStatus = uint32;
enum { CONDITION_QUESTTAKEN = 9, CONDITION_SCENARION_STEP = 44, QUEST_STATUS_INCOMPLETE = 3,
    PHASE_UPDATE_FLAG_CLIENTSIDE_CHANGED = 8, PHASE_UPDATE_FLAG_SERVERSIDE_CHANGED = 16 };
#define TC_LOG_DEBUG(...)
struct Player;
struct InstanceMap { uint32 identity = 42; uint32 GetInstanceId() const { return identity; } };
struct Map : InstanceMap { InstanceMap* ToInstanceMap() { return this; } } instanceMap;
struct WorldObject
{
    bool player = false, owned = false, ignore = false, inInstance = true;
    uint32 mask = 1;
    Map* map = &instanceMap;
    std::set<uint32> m_phaseId;
    std::vector<bool> m_phaseBit;
    bool IsPlayer() const { return player; }
    bool IsUnitOwnedByPlayer() const { return owned; }
    bool IgnorePhaseId() const { return ignore; }
    uint32 GetPhaseMask() const { return mask; }
    bool InInstance() const { return inInstance; }
    Map* GetMap() const { return map; }
    Player* ToPlayer();
    bool IsCreature() const { return false; }
    struct Unit* ToUnit() { assert(false); return nullptr; }
    std::set<uint32> const& GetPhases() const { return m_phaseId; }
    void SetPhaseId(std::set<uint32> const&, bool);
    bool InSamePhase(uint32 value) const { return (mask & value) != 0; }
    bool InSamePhase(WorldObject const*) const;
    bool InSamePhaseId(std::set<uint32> const&, bool) const;
    bool InSamePhaseId(WorldObject const*) const;
};
struct ThreatManager { struct Unit* getHostilTarget() { return nullptr; } };
struct Unit : WorldObject
{
    ThreatManager threat;
    ThreatManager& getThreatManager() { return threat; }
    bool isPet() const { return false; }
    Unit* GetOwner() { return nullptr; }
};
struct Player : WorldObject
{
    uint32 status = QUEST_STATUS_INCOMPLETE;
    bool NeedPhaseRecalculate = true;
    Player() { player = true; }
    uint32 GetCurrentZoneID() const { return 7541; }
    uint32 GetQuestStatus(uint32 identity) const { assert(identity == 40043); return status; }
};
Player* WorldObject::ToPlayer() { return player ? static_cast<Player*>(this) : nullptr; }
#include "SetPhases.inc"
#include "SamePhaseIds.inc"
#include "SameObjectPhaseIds.inc"
#include "SamePhase.inc"
struct Scenario
{
    uint32 identity = 944, step = 0;
    uint32 GetScenarioId() const { return identity; }
    uint32 GetCurrentStep() const { return step; }
};
struct ScenarioMgr
{
    Scenario scenario;
    bool available = true;
    Scenario* GetScenario(uint32 identity) { return available && identity == 42 ? &scenario : nullptr; }
} scenarioManager;
auto* sScenarioMgr = &scenarioManager;
struct ConditionSourceInfo { WorldObject* object; };
struct Condition
{
    uint32 entry = 0, ElseGroup = 0, ConditionType = 0, ConditionValue1 = 0, ConditionValue2 = 0, ReferenceId = 0;
    bool isLoaded() const { return ConditionType != 0; }
    bool Meets(ConditionSourceInfo& source) const
    {
        auto* object = source.object;
        bool condMeets = false;
        switch (ConditionType)
        {
#include "PhaseConditions.inc"
        default: assert(false);
        }
        return condMeets;
    }
};
using ConditionList = std::list<Condition*>;
using ConditionReferenceContainer = std::map<uint32, ConditionList>;
struct ConditionMgr
{
    std::vector<Condition> rows;
    ConditionReferenceContainer ConditionReferenceStore;
    bool IsObjectMeetToConditionList(ConditionSourceInfo&, ConditionList const&) const;
    bool IsObjectMeetToConditions(WorldObject* player, ConditionList const& conditions)
    {
        ConditionSourceInfo source{player};
        return conditions.empty() || IsObjectMeetToConditionList(source, conditions);
    }
    ConditionList GetConditionsForPhaseDefinition(uint32 zone, uint32 entry)
    {
        assert(zone == 7541);
        ConditionList result;
        for (auto& row : rows)
            if (row.entry == entry)
                result.push_back(&row);
        return result;
    }
} conditionManager;
auto* sConditionMgr = &conditionManager;
#include "PhaseElseGroups.inc"
struct PhaseDefinition
{
    std::list<uint16> phaseId;
    uint32 zoneId = 7541, entry = 0, phasemask = 3, terrainswapmap = 0, wmAreaId = 0, flags = 16;
    bool IsLastDefinition() const { return flags & 2; }
    bool IsOverwritingExistingPhases() const { return flags & 1; }
    bool IsNegatingPhasemask() const { return flags & 4; }
};
using PhaseDefinitionContainer = std::list<PhaseDefinition>;
using PhaseDefinitionStore = std::map<uint32, PhaseDefinitionContainer>;
struct PhaseInfo { uint32 phaseId = 0; };
using PhaseInfoContainer = std::map<uint32, PhaseInfo>;
struct PhaseData
{
    std::list<PhaseDefinition const*> activePhaseDefinitions;
    uint32 _PhasemaskThroughDefinitions = 0;
    PhaseInfoContainer spellPhaseInfo;
    bool HasActiveDefinitions() const { return !activePhaseDefinitions.empty(); }
    void ResetDefinitions() { _PhasemaskThroughDefinitions = 0; activePhaseDefinitions.clear(); }
    void AddPhaseDefinition(PhaseDefinition const*);
};
#include "AddDefinition.inc"
struct PhaseMgr
{
    Player* player;
    PhaseData phaseData;
    PhaseDefinitionStore* _PhaseDefinitionStore;
    uint32 _UpdateFlags = 0;
    std::mutex _updateLock;
    void Recalculate();
    bool CheckDefinition(PhaseDefinition const*);
};
#include "CheckDefinition.inc"
#include "Recalculate.inc"
int main(int argc, char** argv)
{
    assert(argc == 4);
    std::ifstream input(argv[1]);
    assert(input.good());
    std::string line;
    while (std::getline(input, line))
    {
        std::istringstream row(line);
        Condition condition;
        row >> condition.entry >> condition.ElseGroup >> condition.ConditionType >> condition.ConditionValue1 >> condition.ConditionValue2;
        assert(row && (condition.ConditionType == 9 || condition.ConditionType == 44));
        conditionManager.rows.push_back(condition);
    }
    assert(conditionManager.rows.size() == 4);
    PhaseDefinitionStore store;
    for (auto identity : {50u, 51u, 52u})
    {
        PhaseDefinition definition;
        definition.entry = identity;
        definition.phaseId = {uint16(identity == 50 ? 7000 : identity == 51 ? 7001 : 7003)};
        store[7541].push_back(definition);
    }
    WorldObject weapon, oldWeapon, environment, firstBoss, soul;
    for (auto entry : {std::make_pair(&oldWeapon, argv[2]), std::make_pair(&weapon, argv[3])})
    {
        std::ifstream phases(entry.second);
        assert(phases.good());
        std::set<uint32> identities;
        uint32 identity = 0;
        while (phases >> identity)
            identities.insert(identity);
        assert(!identities.empty());
        entry.first->SetPhaseId(identities, false);
    }
    environment.SetPhaseId({7003}, false);
    firstBoss.SetPhaseId({7000}, false);
    soul.SetPhaseId({7001}, false);
    uint32 earlyBaseline = 0;
    for (uint32 faction : {0u, 1u})
        for (uint32 stage = 0; stage <= 7; ++stage)
            for (uint32 status : {0u, 1u, 3u, 6u})
            {
                Player player;
                player.status = status;
                PhaseMgr phase{&player, {}, &store};
                scenarioManager.scenario = {944, stage};
                phase.Recalculate();
                assert(!player.NeedPhaseRecalculate);
                assert(player.InSamePhase(&environment) == (status == QUEST_STATUS_INCOMPLETE));
                assert(player.InSamePhase(&firstBoss) == (stage == 4 || stage == 5));
                assert(player.InSamePhase(&soul) == (stage == 6));
                assert(player.InSamePhase(&weapon) == (stage == 6));
                if (status == QUEST_STATUS_INCOMPLETE && stage < 6)
                {
                    assert(player.InSamePhase(&oldWeapon));
                    ++earlyBaseline;
                }
                assert(player.InSamePhase(&oldWeapon) == (status == QUEST_STATUS_INCOMPLETE || stage == 6));
                player.owned = faction != 0;
                assert(player.InSamePhase(&weapon) == (stage == 6));
                scenarioManager.scenario.step = 6;
                phase.Recalculate();
                assert(player.InSamePhase(&weapon));
                scenarioManager.scenario.step = 0;
                phase.Recalculate();
                assert(!player.InSamePhase(&weapon));
                scenarioManager.available = false;
                phase.Recalculate();
                assert(!player.InSamePhase(&weapon));
                scenarioManager.available = true;
                scenarioManager.scenario.identity = 909;
                phase.Recalculate();
                assert(!player.InSamePhase(&weapon));
            }
    assert(earlyBaseline == 12);
    std::cout << "PASS: actual source23 condition/ElseGroup, PhaseMgr recalculation and native phase-ID intersection: 12 early baseline exposures, final-stage-only replacement, transitions/re-entry, shared7003 environment and7000 boss retained. Controlled world/visibility services; not client or cached-GUID Use admission.\n";
}
