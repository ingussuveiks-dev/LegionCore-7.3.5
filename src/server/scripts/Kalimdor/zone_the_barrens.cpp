/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: The_Barrens
SD%Complete: 90
SDComment: Quest support: 863
SDCategory: Barrens
EndScriptData */

/* ContentData
npc_wizzlecrank_shredder
EndContentData */

#include "ConditionMgr.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "SmartAI.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "Vehicle.h"

/*#####
## npc_wizzlecrank_shredder
#####*/

enum Wizzlecrank
{
    SAY_START           = -1000298,
    SAY_STARTUP1        = -1000299,
    SAY_STARTUP2        = -1000300,
    SAY_MERCENARY       = -1000301,
    SAY_PROGRESS_1      = -1000302,
    SAY_PROGRESS_2      = -1000303,
    SAY_PROGRESS_3      = -1000304,
    SAY_END             = -1000305,

    QUEST_ESCAPE        = 863,
    FACTION_RATCHET     = 637,
    NPC_PILOT_WIZZ      = 3451,
    NPC_MERCENARY       = 3282,
};

class npc_wizzlecrank_shredder : public CreatureScript
{
public:
    npc_wizzlecrank_shredder() : CreatureScript("npc_wizzlecrank_shredder") { }

    struct npc_wizzlecrank_shredderAI : public npc_escortAI
    {
        npc_wizzlecrank_shredderAI(Creature* creature) : npc_escortAI(creature)
        {
            IsPostEvent = false;
            PostEventTimer = 1000;
            PostEventCount = 0;
        }

        bool IsPostEvent;
        uint32 PostEventTimer;
        uint32 PostEventCount;

        void Reset()
        {
            if (!HasEscortState(STATE_ESCORT_ESCORTING))
            {
                if (me->getStandState() == UNIT_STAND_STATE_DEAD)
                     me->SetStandState(UNIT_STAND_STATE_STAND);

                IsPostEvent = false;
                PostEventTimer = 1000;
                PostEventCount = 0;
            }
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
                case 0:
                    DoScriptText(SAY_STARTUP1, me);
                    break;
                case 9:
                    SetRun(false);
                    break;
                case 17:
                    if (Creature* temp = me->SummonCreature(NPC_MERCENARY, 1128.489f, -3037.611f, 92.701f, 1.472f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000))
                    {
                        DoScriptText(SAY_MERCENARY, temp);
                        me->SummonCreature(NPC_MERCENARY, 1160.172f, -2980.168f, 97.313f, 3.690f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                    }
                    break;
                case 24:
                    IsPostEvent = true;
                    break;
            }
        }

        void WaypointStart(uint32 PointId)
        {
            Player* player = GetPlayerForEscort();

            if (!player)
                return;

            switch (PointId)
            {
                case 9:
                    DoScriptText(SAY_STARTUP2, me, player);
                    break;
                case 18:
                    DoScriptText(SAY_PROGRESS_1, me, player);
                    SetRun();
                    break;
            }
        }

        void JustSummoned(Creature* summoned)
        {
            if (summoned->GetEntry() == NPC_PILOT_WIZZ)
                me->SetStandState(UNIT_STAND_STATE_DEAD);

            if (summoned->GetEntry() == NPC_MERCENARY)
                summoned->AI()->AttackStart(me);
        }

        void UpdateEscortAI(const uint32 Diff)
        {
            if (!UpdateVictim())
            {
                if (IsPostEvent)
                {
                    if (PostEventTimer <= Diff)
                    {
                        switch (PostEventCount)
                        {
                            case 0:
                                DoScriptText(SAY_PROGRESS_2, me);
                                break;
                            case 1:
                                DoScriptText(SAY_PROGRESS_3, me);
                                break;
                            case 2:
                                DoScriptText(SAY_END, me);
                                break;
                            case 3:
                                if (Player* player = GetPlayerForEscort())
                                {
                                    player->GroupEventHappens(QUEST_ESCAPE, me);
                                    me->SummonCreature(NPC_PILOT_WIZZ, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 180000);
                                }
                                break;
                        }

                        ++PostEventCount;
                        PostEventTimer = 5000;
                    }
                    else
                        PostEventTimer -= Diff;
                }

                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_ESCAPE)
        {
            creature->setFaction(FACTION_RATCHET);
            if (npc_escortAI* pEscortAI = CAST_AI(npc_wizzlecrank_shredder::npc_wizzlecrank_shredderAI, creature->AI()))
                pEscortAI->Start(true, false, player->GetGUID());
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_wizzlecrank_shredderAI(creature);
    }

};

namespace CrossroadsDelivery
{
    constexpr uint32 QuestId = 13975;
    constexpr uint32 Credit = 65892;
    constexpr uint32 Sweeper = 46714;
    constexpr uint32 Rocco = 34578;
    constexpr uint32 Kodo = 34577;
    constexpr uint32 SummonKodo = 65662;
    constexpr uint32 Raider = 34594;
    constexpr uint32 ArrivalPoint = 23;
    constexpr int8 RiderSeat = 0;
    Position const Arrival{-453.165f, -2652.06f, 95.5692f, 0.0f};

    bool HasDelivery(Player const* player)
    {
        return player && player->IsAlive() && player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE &&
            player->HasItemCount(Sweeper, 1, false);
    }

    Creature const* FindKodo(Player* player)
    {
        for (ObjectGuid const& guid : *player->GetSummonList(Kodo))
            if (Creature const* kodo = ObjectAccessor::GetCreature(*player, guid))
                if (kodo->IsAlive() && kodo->HasQuestObjectScope() && !kodo->IsQuestObjectExpired())
                    return kodo;
        return nullptr;
    }
}

struct npc_barrens_rocco_delivery : SmartAI
{
    explicit npc_barrens_rocco_delivery(Creature* creature) : SmartAI(creature) { }

    void sGossipSelect(Player* player, uint32 menu, uint32 option) override
    {
        using namespace CrossroadsDelivery;
        if (menu != Rocco || option != 0 || !HasDelivery(player) || player->GetVehicle() ||
            player->GetQuestObjectiveData(QuestId, Credit) || !me->IsWithinDistInMap(player, INTERACTION_DISTANCE))
            return;

        player->PlayerTalkClass->SendCloseGossip();
        if (!FindKodo(player) && player->CastSpell(player, SummonKodo, true) == SPELL_CAST_OK && FindKodo(player))
            SmartAI::sGossipSelect(player, menu, option);
    }
};

struct npc_barrens_delivery_kodo : SmartAI
{
    explicit npc_barrens_delivery_kodo(Creature* creature) : SmartAI(creature) { }

    void IsSummonedBy(Unit* summoner) override
    {
        using namespace CrossroadsDelivery;
        auto player = summoner->ToPlayer();
        if (!HasDelivery(player) || !me->HasQuestObjectScope() ||
            me->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) != SummonKodo)
        {
            me->DespawnOrUnsummon(1);
            return;
        }

        _rider = player->GetGUID();
        me->SetReactState(REACT_PASSIVE);
        SetAutoAttack(false);
        SetCombatMove(false);
        SmartAI::IsSummonedBy(summoner);
    }

    bool CanBoard(Player const* player) const
    {
        using namespace CrossroadsDelivery;
        auto vehicle = me->GetVehicleKit();
        return !_boarded && !_arrived && me->IsAlive() && !me->IsQuestObjectExpired() && vehicle &&
            vehicle->HasEmptySeat(RiderSeat) && HasDelivery(player) && player->GetGUID() == _rider &&
            !player->GetQuestObjectiveData(QuestId, Credit) && !player->GetVehicle() &&
            !me->HasAuraTypeWithCaster(SPELL_AURA_CONTROL_VEHICLE, _rider);
    }

    bool HasActiveRider() const
    {
        using namespace CrossroadsDelivery;
        auto vehicle = me->GetVehicleKit();
        auto passenger = vehicle ? vehicle->GetPassenger(RiderSeat) : nullptr;
        return _boarded && !_arrived && !me->IsQuestObjectExpired() && passenger &&
            passenger->GetGUID() == _rider && HasDelivery(passenger->ToPlayer());
    }

    void PassengerBoarded(Unit* passenger, int8 seat, bool apply) override
    {
        using namespace CrossroadsDelivery;
        if (passenger->GetGUID() != _rider || seat != RiderSeat || (apply && !HasDelivery(passenger->ToPlayer())))
        {
            if (apply)
            {
                auto guid = passenger->GetGUID();
                me->AddDelayedEvent(1, [kodo = me, guid] {
                    if (Unit* rider = ObjectAccessor::GetUnit(*kodo, guid))
                        if (rider->GetVehicleBase() == kodo)
                            rider->ExitVehicle();
                });
            }
            return;
        }

        if (!apply)
        {
            if (_boarded && !_arrived)
                me->DespawnOrUnsummon(1);
            _boarded = false;
            return;
        }

        if (_boarded || _arrived || me->IsQuestObjectExpired())
            return;
        _boarded = true;
        SmartAI::PassengerBoarded(passenger, seat, true);
        if (!HasEscortState(SMART_ESCORT_ESCORTING))
        {
            TC_LOG_WARN("scripts", "Caravan delivery: kodo 34577 could not start its native waypoint path");
            me->DespawnOrUnsummon(1);
        }
    }

    void MovementInform(uint32 type, uint32 point) override
    {
        using namespace CrossroadsDelivery;
        if (point == ArrivalPoint && type == POINT_MOTION_TYPE && HasEscortState(SMART_ESCORT_ESCORTING) &&
            me->GetExactDist(&Arrival) < 4.0f && HasActiveRider())
        {
            auto player = me->GetVehicleKit()->GetPassenger(RiderSeat)->ToPlayer();
            player->KilledMonsterCredit(Credit);
            if (player->GetQuestObjectiveData(QuestId, Credit))
            {
                _arrived = true;
                // The legacy quest-event effect marks every objective. The
                // real delivery only completes after seated arrival and credit.
                player->CompleteQuest(QuestId);
            }
        }

        SmartAI::MovementInform(type, point);
    }

    void UpdateAI(uint32 diff) override
    {
        using namespace CrossroadsDelivery;
        if (!_arrived)
        {
            Player* player = ObjectAccessor::GetPlayer(*me, _rider);
            if (!HasDelivery(player) || me->IsQuestObjectExpired() || (_boarded && !HasActiveRider()) ||
                (!_boarded && _mountWindow <= diff))
            {
                me->DespawnOrUnsummon(1);
                return;
            }
            if (!_boarded)
                _mountWindow -= diff;
        }
        SmartAI::UpdateAI(diff);
    }

private:
    ObjectGuid _rider;
    uint32 _mountWindow = 45000;
    bool _boarded = false;
    bool _arrived = false;
};

class condition_barrens_delivery_rider : public ConditionScript
{
public:
    condition_barrens_delivery_rider() : ConditionScript("condition_barrens_delivery_rider") { }

    bool OnConditionCheck(Condition*, ConditionSourceInfo& source) override
    {
        auto actor = source.mConditionTargets[1] ? source.mConditionTargets[1]->ToCreature() : nullptr;
        auto ai = actor && actor->IsAIEnabled ? CAST_AI(npc_barrens_delivery_kodo, actor->AI()) : nullptr;
        return ai && ai->CanBoard(source.mConditionTargets[0] ? source.mConditionTargets[0]->ToPlayer() : nullptr);
    }
};

class player_barrens_delivery_recovery : public PlayerScript
{
public:
    player_barrens_delivery_recovery() : PlayerScript("player_barrens_delivery_recovery") { }

    void OnLogin(Player* player) override
    {
        using namespace CrossroadsDelivery;
        if (player->GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE && !player->GetQuestRewardStatus(QuestId) &&
            player->GetQuestObjectiveData(QuestId, Credit) < 1)
            player->IncompleteQuest(QuestId);
    }
};

class spell_barrens_delivery_sweeper : public SpellScript
{
    PrepareSpellScript(spell_barrens_delivery_sweeper);

    SpellCastResult CheckCast()
    {
        using namespace CrossroadsDelivery;
        if (GetCaster()->GetEntry() != Kodo)
            return SPELL_CAST_OK;

        auto ai = CAST_AI(npc_barrens_delivery_kodo, GetCaster()->GetAI());
        if (!ai || !ai->HasActiveRider())
            return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
        if (m_scriptSpellId == 96791)
        {
            Unit* target = GetExplTargetUnit();
            if (!target || target->GetEntry() != Raider)
                return SPELL_FAILED_BAD_TARGETS;
        }
        else if (!FindRaider())
            return SPELL_FAILED_NO_VALID_TARGETS;
        return SPELL_CAST_OK;
    }

    Creature* FindRaider()
    {
        auto caster = GetCaster();
        auto target = caster->FindNearestCreature(CrossroadsDelivery::Raider,
            GetSpellInfo()->GetMaxRange(false, caster), true);
        return target && caster->IsWithinLOSInMap(target) ? target : nullptr;
    }

    void SelectRaider(WorldObject*& target)
    {
        if (GetCaster()->GetEntry() == CrossroadsDelivery::Kodo)
            target = FindRaider();
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_barrens_delivery_sweeper::CheckCast);
        if (m_scriptSpellId == 96792)
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_barrens_delivery_sweeper::SelectRaider, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
    }
};

void AddSC_the_barrens()
{
    new npc_wizzlecrank_shredder();
    RegisterCreatureAI(npc_barrens_rocco_delivery);
    RegisterCreatureAI(npc_barrens_delivery_kodo);
    new condition_barrens_delivery_rider();
    new player_barrens_delivery_recovery();
    RegisterSpellScript(spell_barrens_delivery_sweeper);
}
