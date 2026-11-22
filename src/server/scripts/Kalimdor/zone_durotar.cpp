/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "SmartAI.h"
#include "Vehicle.h"

/*######
## Quest 25134: Lazy Peons
## npc_lazy_peon
######*/

enum LazyPeonYells
{
    SAY_SPELL_HIT = 0
};

enum LazyPeon
{
    QUEST_LAZY_PEONS    = 25134,
    GO_LUMBERPILE       = 175784,
    SPELL_BUFF_SLEEP    = 17743,
    SPELL_AWAKEN_PEON   = 19938
};

class npc_lazy_peon : public CreatureScript
{
public:
    npc_lazy_peon() : CreatureScript("npc_lazy_peon") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_lazy_peonAI(creature);
    }

    struct npc_lazy_peonAI : public ScriptedAI
    {
        npc_lazy_peonAI(Creature* creature) : ScriptedAI(creature) {}

        ObjectGuid PlayerGUID;

        uint32 RebuffTimer;
        bool work;

        void Reset() override
        {
            PlayerGUID.Clear();
            RebuffTimer = 0;
            work = false;
        }

        void MovementInform(uint32 /*type*/, uint32 id) override
        {
            if (id == 1)
                work = true;
        }

        void SpellHit(Unit* caster, const SpellInfo* spell) override
        {
            if (spell->Id == SPELL_AWAKEN_PEON && caster->IsPlayer() && CAST_PLR(caster)->GetQuestStatus(QUEST_LAZY_PEONS) == QUEST_STATUS_INCOMPLETE)
            {
                caster->ToPlayer()->KilledMonsterCredit(me->GetEntry(), me->GetGUID());
                Talk(SAY_SPELL_HIT, caster->GetGUID());
                me->RemoveAllAuras();

                if (GameObject* Lumberpile = me->FindNearestGameObject(GO_LUMBERPILE, 20))
                    me->GetMotionMaster()->MovePoint(1, Lumberpile->GetPositionX()-1, Lumberpile->GetPositionY(), Lumberpile->GetPositionZ());
            }
        }

        void UpdateAI(uint32 Diff) override
        {
            if (work == true)
                me->HandleEmoteCommand(EMOTE_ONESHOT_WORK_CHOPWOOD);

            if (RebuffTimer <= Diff)
            {
                DoCast(me, SPELL_BUFF_SLEEP);
                RebuffTimer = 300000; //Rebuff agian in 5 minutes
            }
            else
                RebuffTimer -= Diff;

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

enum VoodooSpells
{
    SPELL_BREW      = 16712, // Special Brew
    SPELL_GHOSTLY   = 16713, // Ghostly
    SPELL_HEX1      = 16707, // Hex
    SPELL_HEX2      = 16708, // Hex
    SPELL_HEX3      = 16709, // Hex
    SPELL_GROW      = 16711, // Grow
    SPELL_LAUNCH    = 16716, // Launch (Whee!)
};

// 17009
class spell_voodoo : public SpellScriptLoader
{
    public:
        spell_voodoo() : SpellScriptLoader("spell_voodoo") {}

        class spell_voodoo_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_voodoo_SpellScript);

            bool Validate(SpellInfo const* /*spell*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_BREW) || !sSpellMgr->GetSpellInfo(SPELL_GHOSTLY) ||
                    !sSpellMgr->GetSpellInfo(SPELL_HEX1) || !sSpellMgr->GetSpellInfo(SPELL_HEX2) ||
                    !sSpellMgr->GetSpellInfo(SPELL_HEX3) || !sSpellMgr->GetSpellInfo(SPELL_GROW) ||
                    !sSpellMgr->GetSpellInfo(SPELL_LAUNCH))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                uint32 spellid = RAND(SPELL_BREW, SPELL_GHOSTLY, RAND(SPELL_HEX1, SPELL_HEX2, SPELL_HEX3), SPELL_GROW, SPELL_LAUNCH);
                if (Unit* target = GetHitUnit())
                    GetCaster()->CastSpell(target, spellid, false);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_voodoo_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_voodoo_SpellScript();
        }
};

class npc_100874_illidari_enforcer_dh_questline : public CreatureScript
{
public:
    npc_100874_illidari_enforcer_dh_questline() : CreatureScript("npc_100874_illidari_enforcer_dh_questline") { }

    struct npc_100874_illidari_enforcer_dh_questlineAI : ScriptedAI
    {
        npc_100874_illidari_enforcer_dh_questlineAI(Creature* creature) : ScriptedAI(creature) {}

        EventMap events;

        void Reset() override
        {
            events.RescheduleEvent(EVENT_1, 2000);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_1:
                    events.ScheduleEvent(EVENT_1, 2000);

                    std::list<Player*> list;
                    me->GetPlayerListInGrid(list, 20.0f);
                    for (Player* player : list)
                        if (player->HasAura(188501) && player->GetQuestStatus(40982) == QUEST_STATUS_INCOMPLETE)
                            player->KilledMonsterCredit(102563);

                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_100874_illidari_enforcer_dh_questlineAI(creature);
    }
};

namespace
{
constexpr uint32 SwiftclawQuest = 24626;
constexpr uint32 BloodtalonLasso = 50053;
constexpr uint32 RideSwiftclaw = 46598;
constexpr uint32 Swiftclaw = 37989;
constexpr uint32 SwiftclawSpawn = 80616;
constexpr uint32 RaptorPens = 5675;
constexpr uint32 PensController = 38003;
constexpr uint32 PensControllerSpawn = 240751;
constexpr uint32 CaptureSwiftclaw = 70943;
constexpr uint32 ReturnSwiftclaw = 70941;
constexpr uint32 ReturnedSwiftclaw = 38002;

class SwiftclawQuestAI : public SmartAI
{
public:
    explicit SwiftclawQuestAI(Creature* creature) : SmartAI(creature) { }

    bool CanLasso(Player const& player, float radius) const
    {
        Vehicle const* vehicle = me->GetVehicleKit();
        return me->GetDBTableGUIDLow() == SwiftclawSpawn && me->IsAlive() && !me->IsDespawn() && !_rider && !_delivered &&
            vehicle && vehicle->HasEmptySeat(0) && !player.GetVehicle() && player.IsAlive() && player.GetMapId() == 1 &&
            player.GetZoneId() == 6453 && player.GetQuestStatus(SwiftclawQuest) == QUEST_STATUS_INCOMPLETE &&
            !player.GetQuestObjectiveData(SwiftclawQuest, ReturnedSwiftclaw) && player.IsWithinDistInMap(me, radius) && player.IsWithinLOSInMap(me);
    }

    void ArmRide(Player& player, float radius)
    {
        if (!CanLasso(player, radius))
            return;

        _attempt = player.GetQuestCancellationReceipt(SwiftclawQuest);
        if (!_attempt || _attempt->load(std::memory_order_acquire))
            return;

        _rider = player.GetGUID();
        _boardingMs = 5000;
        _rideDelayMs = 200;
        _lassoRadius = radius;
    }

    void PassengerBoarded(Unit* passenger, int8 seat, bool apply) override
    {
        SmartAI::PassengerBoarded(passenger, seat, apply);
        if (seat != 0 || passenger->GetGUID() != _rider)
            return;

        if (!apply)
        {
            ClearRider();
            if (!_delivered)
                me->GetMotionMaster()->MoveTargetedHome();
            return;
        }

        if (Player* player = passenger->ToPlayer(); IsCurrentRider(player))
        {
            _boardingMs = 0;
            me->CastSpell(player, CaptureSwiftclaw, true);
        }
    }

    void SetData(uint32 id, uint32 value) override
    {
        if (id != 1 || value != 1)
        {
            SmartAI::SetData(id, value);
            return;
        }

        Player* player = GetSeatedPlayer();
        if (_delivered || !IsCurrentRider(player) || !player->IsInAreaTriggerRadius(RaptorPens) ||
            !player->GetQuestObjectiveData(SwiftclawQuest, Swiftclaw) || player->GetQuestObjectiveData(SwiftclawQuest, ReturnedSwiftclaw))
            return;

        Creature* handler = me->FindNearestCreature(PensController, 10.0f, true);
        if (!handler || handler->GetDBTableGUIDLow() != PensControllerSpawn)
            return;

        handler->CastSpell(player, ReturnSwiftclaw, true);
        if (!player->GetQuestObjectiveData(SwiftclawQuest, ReturnedSwiftclaw))
            return;

        _delivered = true;
        player->ExitVehicle();
        me->DespawnOrUnsummon(1);
    }

    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        if (_delivered)
            return;

        if (Vehicle* vehicle = me->GetVehicleKit())
            if (Unit* passenger = vehicle->GetPassenger(0); passenger && !IsCurrentRider(passenger->ToPlayer()))
            {
                passenger->ExitVehicle();
                return;
            }

        if (_boardingMs && diff >= _boardingMs)
        {
            ClearRider();
            return;
        }
        if (_boardingMs)
            _boardingMs -= diff;

        if (!_rideDelayMs)
            return;
        if (diff < _rideDelayMs)
        {
            _rideDelayMs -= diff;
            return;
        }

        // Keep the original 200 ms ride delay and recheck the attempt before the native cast.
        _rideDelayMs = 0;
        Player* player = ObjectAccessor::GetPlayer(*me, _rider);
        Vehicle* vehicle = me->GetVehicleKit();
        if (!IsAcceptedRider(player) || !vehicle || !vehicle->HasEmptySeat(0) || player->GetVehicle() ||
            !player->HasItemCount(BloodtalonLasso, 1, false) || !player->IsWithinDistInMap(me, _lassoRadius) ||
            !player->IsWithinLOSInMap(me) || player->CastSpell(me, RideSwiftclaw, false) != SPELL_CAST_OK)
            ClearRider();
    }

    void JustRespawned() override
    {
        ClearRider();
        _delivered = false;
        SmartAI::JustRespawned();
    }

private:
    Player* GetSeatedPlayer() const
    {
        Vehicle* vehicle = me->GetVehicleKit();
        Unit* passenger = vehicle ? vehicle->GetPassenger(0) : nullptr;
        return passenger ? passenger->ToPlayer() : nullptr;
    }

    bool IsCurrentRider(Player* player) const
    {
        return IsAcceptedRider(player) && player == GetSeatedPlayer();
    }

    bool IsAcceptedRider(Player* player) const
    {
        return player && player->GetGUID() == _rider && player->IsAlive() && player->IsInMap(me) &&
            player->GetQuestStatus(SwiftclawQuest) == QUEST_STATUS_INCOMPLETE && _attempt &&
            !_attempt->load(std::memory_order_acquire) && player->GetQuestCancellationReceipt(SwiftclawQuest) == _attempt;
    }

    void ClearRider()
    {
        _rider.Clear();
        _attempt.reset();
        _boardingMs = 0;
        _rideDelayMs = 0;
    }

    ObjectGuid _rider;
    std::shared_ptr<QuestCancellationReceipt const> _attempt;
    uint32 _boardingMs = 0;
    uint32 _rideDelayMs = 0;
    float _lassoRadius = 0.0f;
    bool _delivered = false;
};

class npc_swiftclaw_quest_ride : public CreatureScript
{
public:
    npc_swiftclaw_quest_ride() : CreatureScript("npc_swiftclaw_quest_ride") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new SwiftclawQuestAI(creature);
    }
};

class spell_swiftclaw_quest_lasso : public SpellScript
{
    PrepareSpellScript(spell_swiftclaw_quest_lasso);

    SpellCastResult CheckLasso()
    {
        Player* player = GetCaster()->ToPlayer();
        Item* item = GetCastItem();
        if (!player || !item || item->GetEntry() != BloodtalonLasso || !Player::IsInventoryPos(item->GetBagSlot(), item->GetSlot()) ||
            !player->HasItemCount(BloodtalonLasso, 1, false))
            return SPELL_FAILED_BAD_TARGETS;

        float radius = GetSpellInfo()->GetEffect(EFFECT_0)->CalcRadius(player);
        ObjectGuid target = GetOriginalTargetGUID();
        Creature* raptor = target && target != player->GetGUID() ? ObjectAccessor::GetCreature(*player, target) :
            player->FindNearestCreature(Swiftclaw, radius, true);
        auto ai = raptor && raptor->GetEntry() == Swiftclaw ? dynamic_cast<SwiftclawQuestAI*>(raptor->AI()) : nullptr;
        if (!ai || !ai->CanLasso(*player, radius))
            return SPELL_FAILED_BAD_TARGETS;

        _target = raptor->GetGUID();
        return SPELL_CAST_OK;
    }

    void SelectSwiftclaw(std::list<WorldObject*>& targets)
    {
        targets.remove_if([this](WorldObject const* target) { return target->GetGUID() != _target; });
    }

    void ArmSwiftclaw(SpellEffIndex /*effect*/)
    {
        Player* player = GetCaster()->ToPlayer();
        Creature* raptor = GetHitCreature();
        auto ai = raptor && raptor->GetGUID() == _target ? dynamic_cast<SwiftclawQuestAI*>(raptor->AI()) : nullptr;
        if (player && ai)
            ai->ArmRide(*player, GetSpellInfo()->GetEffect(EFFECT_0)->CalcRadius(player));
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_swiftclaw_quest_lasso::CheckLasso);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_swiftclaw_quest_lasso::SelectSwiftclaw, EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
        OnEffectHitTarget += SpellEffectFn(spell_swiftclaw_quest_lasso::ArmSwiftclaw, EFFECT_0, SPELL_EFFECT_DUMMY);
    }

    ObjectGuid _target;
};
}

void AddSC_durotar()
{
    new npc_lazy_peon();
    new spell_voodoo();
    new npc_100874_illidari_enforcer_dh_questline();
    new npc_swiftclaw_quest_ride();
    RegisterSpellScript(spell_swiftclaw_quest_lasso);
}
