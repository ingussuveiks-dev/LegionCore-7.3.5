#ifndef FARONAAR_RITUAL_H
#define FARONAAR_RITUAL_H

#include "InstanceScript.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
#include <array>
#include <cmath>
#include <map>

namespace FaronaarRitual
{
constexpr uint32 MapId = 1616;
constexpr uint32 ScenarioId = 1122;
constexpr uint32 Quest = 42479;
constexpr uint32 Meryl = 108097;
constexpr uint32 PortalEmitter = 107691;
constexpr uint32 RitualFocus = 108855;
constexpr uint32 ManaGem = 108854;
constexpr uint32 PortalVisual = 214695;
constexpr uint32 RealmTransfer = 214730;
constexpr uint32 RitualOverride = 214607;
constexpr uint32 CenterReachedTree = 48464;
constexpr uint32 FocusReadyData = 108855;
constexpr uint32 AltarStep = 2;
constexpr uint32 RitualStep = 3;
constexpr uint32 ShowdownStep = 4;
constexpr float AltarRadius = 28.0f;
constexpr uint32 UpdateInterval = 500;

Position const Center{-418.688f, 8009.09f, 66.65f};
std::array<Position, 4> const GemPositions{{
    {-412.69f, 8009.09f, 66.65f}, {-418.69f, 8015.09f, 66.65f},
    {-424.69f, 8009.09f, 66.65f}, {-418.69f, 8003.09f, 66.65f}
}};

class State
{
public:
    void Observe(Creature const* creature)
    {
        if (creature->GetEntry() == Meryl && creature->GetDBTableGUIDLow() == 369855)
            _meryl = creature->GetGUID();
        else if (creature->GetEntry() == PortalEmitter && creature->GetDBTableGUIDLow() == 369856)
            _portalEmitter = creature->GetGUID();
    }

    void Update(InstanceMap& map, uint32 diff)
    {
        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }
        _timer = UpdateInterval;

        Scenario* scenario = sScenarioMgr->GetScenario(map.GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != ScenarioId)
            return;

        uint32 const step = scenario->GetCurrentStep();
        if (step == AltarStep || step == RitualStep)
        {
            if (step == AltarStep && !scenario->GetAchievementMgr().GetCriteriaProgressMap()->count(CenterReachedTree))
                return;

            if (Creature* meryl = map.GetCreature(_meryl); meryl && meryl->IsAlive())
                PrepareRitual(map, *meryl, step);
            return;
        }

        if (step >= ShowdownStep)
            ClearCrystals(map);

        if (step == ShowdownStep)
            TransferToRealm(map);
    }

    void Leave(Player const& player)
    {
        _transfers.erase(player.GetGUID());
    }

    bool IsFocusReady() const { return _focusReady; }

    void EnableFocus(InstanceMap& map)
    {
        Scenario* scenario = sScenarioMgr->GetScenario(map.GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != ScenarioId || scenario->GetCurrentStep() != AltarStep ||
            !scenario->GetAchievementMgr().GetCriteriaProgressMap()->count(CenterReachedTree))
            return;

        _focusReady = true;
        if (Creature* focus = map.GetCreature(_focus))
            focus->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    }

private:
    static bool CanParticipate(Player const& player)
    {
        return player.IsAlive() && player.IsInWorld() && !player.IsBeingTeleported() &&
            player.getClass() == CLASS_MAGE && player.GetQuestStatus(Quest) == QUEST_STATUS_INCOMPLETE;
    }

    static bool IsAtAltar(Player const& player)
    {
        return player.GetDistance2d(Center.GetPositionX(), Center.GetPositionY()) <= AltarRadius &&
            std::fabs(player.GetPositionZ() - Center.GetPositionZ()) < 6.0f;
    }

    static Creature* EnsureCrystal(InstanceMap& map, Creature& meryl, ObjectGuid& guid, uint32 entry,
        uint32 display, Position const& position)
    {
        if (Creature* crystal = map.GetCreature(guid))
            return crystal;

        TempSummon* crystal = meryl.SummonCreature(entry, position);
        if (crystal)
        {
            crystal->SetDisplayId(display);
            crystal->SetReactState(REACT_PASSIVE);
            guid = crystal->GetGUID();
        }
        return crystal;
    }

    void PrepareRitual(InstanceMap& map, Creature& meryl, uint32 step)
    {
        Player* participant = nullptr;
        for (auto const& reference : map.GetPlayers())
            if (Player* player = reference.getSource(); player && CanParticipate(*player) && IsAtAltar(*player))
            {
                participant = player;
                break;
            }

        if (!participant)
            return;

        if (Creature* focus = EnsureCrystal(map, meryl, _focus, RitualFocus, 70744, Center))
            if (_focusReady || step == RitualStep)
                focus->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        for (std::size_t i = 0; i < _gems.size(); ++i)
            EnsureCrystal(map, meryl, _gems[i], ManaGem, 70743, GemPositions[i]);

        if (step == RitualStep && !_announcedRitual)
        {
            _announcedRitual = true;
            meryl.AI()->Talk(4, participant->GetGUID());
        }

        if (step == RitualStep)
            for (auto const& reference : map.GetPlayers())
                if (Player* player = reference.getSource(); player && CanParticipate(*player) && IsAtAltar(*player) &&
                    !player->HasAura(RitualOverride))
                    player->CastSpell(player, RitualOverride, true);
    }

    static void ClearCrystal(InstanceMap& map, ObjectGuid& guid)
    {
        if (Creature* crystal = map.GetCreature(guid))
            crystal->DespawnOrUnsummon();
        guid.Clear();
    }

    void ClearCrystals(InstanceMap& map)
    {
        _focusReady = false;
        ClearCrystal(map, _focus);
        for (ObjectGuid& gem : _gems)
            ClearCrystal(map, gem);
    }

    void TransferToRealm(InstanceMap& map)
    {
        Creature* portal = map.GetCreature(_portalEmitter);
        SpellTargetPosition const* destination = sSpellMgr->GetSpellTargetPosition(RealmTransfer);
        if (!portal || !portal->HasAura(PortalVisual) || !destination || destination->target_mapId != MapId)
            return;

        for (auto const& reference : map.GetPlayers())
        {
            Player* player = reference.getSource();
            if (!player || !CanParticipate(*player))
                continue;

            if (player->GetDistance2d(destination->target_X, destination->target_Y) < 45.0f)
            {
                player->RemoveAurasDueToSpell(RitualOverride);
                _transfers.erase(player->GetGUID());
                continue;
            }

            if (!IsAtAltar(*player) || !player->InSamePhase(portal) || player->GetVehicle() || player->isInFlight())
                continue;

            PendingTransfer& transfer = _transfers[player->GetGUID()];
            if (transfer.wait > UpdateInterval)
            {
                transfer.wait -= UpdateInterval;
                continue;
            }

            if (transfer.attempts >= 3)
                continue;

            ++transfer.attempts;
            transfer.wait = 2500;
            player->CastSpell(player, RealmTransfer, true);
        }
    }

    struct PendingTransfer
    {
        uint32 wait = 2500;
        uint8 attempts = 0;
    };

    uint32 _timer = UpdateInterval;
    bool _focusReady = false;
    bool _announcedRitual = false;
    ObjectGuid _meryl;
    ObjectGuid _portalEmitter;
    ObjectGuid _focus;
    std::array<ObjectGuid, 4> _gems;
    std::map<ObjectGuid, PendingTransfer> _transfers;
};
}

#endif
