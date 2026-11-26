#ifndef WESTFALL_VISION_SCENE_H
#define WESTFALL_VISION_SCENE_H

#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "Vehicle.h"
#include "WestfallVision.h"

namespace
{
using namespace WestfallVision;

Position const VisionEntry{-97.6376f, -690.562f, 24.3914f};
Position const VisionExit{-19.5636f, -377.193f, 60.8038f};
G3D::Vector3 const VisionFlight[] = {
    {-96.52084f, -701.3229f, 26.23725f}, {-72.3125f, -732.9184f, 34.70948f},
    {-40.28125f, -763.783f, 43.5428f}, {-2.125f, -818.7014f, 57.0428f},
    {-27.65799f, -850.7101f, 57.0428f}, {-63.17188f, -829.3177f, 47.68164f},
};

constexpr uint32 Edwin = 42697;
constexpr uint32 Vanessa = 42371;
constexpr uint32 Warrior = 42699;
constexpr uint32 Rogue = 42700;
constexpr uint32 Hunter = 42701;
constexpr uint32 Mage = 42702;
constexpr uint32 Priest = 42703;
constexpr uint32 Blackguard = 42698;

struct VisionActor
{
    uint32 Entry;
    Position Placement;
};

VisionActor const VisionActors[] = {
    {Edwin, {-87.36979f, -819.8958f, 39.30045f, 0.0f}},
    {Vanessa, {-82.74338f, -819.6807f, 39.63371f, 6.253996f}},
    {Warrior, {-63.06597f, -819.1649f, 41.35357f, 3.054326f}},
    {Rogue, {-60.69618f, -819.1945f, 41.58845f, 3.054326f}},
    {Hunter, {-61.40104f, -821.882f, 41.52324f, 3.106686f}},
    {Mage, {-58.5382f, -819.1337f, 41.79993f, 3.071779f}},
    {Priest, {-61.44097f, -816.7239f, 41.51037f, 2.96706f}},
    {Blackguard, {-77.80729f, -815.0972f, 40.01878f, 3.263766f}},
    {Blackguard, {-78.27952f, -824.7847f, 40.00071f, 2.932153f}},
};

struct VisionCombatAI : ScriptedAI
{
    explicit VisionCombatAI(Creature* creature) : ScriptedAI(creature) {}

    void Begin(Creature* opponent)
    {
        if (!opponent || !opponent->IsAlive())
            return;
        me->SetReactState(REACT_AGGRESSIVE);
        AttackStart(opponent);
    }

    void DamageTaken(Unit*, uint32& damage, DamageEffectType) override
    {
        if (me->GetEntry() != Edwin)
            return;
        constexpr uint8 thresholds[] = {75, 50, 25};
        while (_healthLine < std::size(thresholds) && me->HealthBelowPctDamaged(thresholds[_healthLine], damage))
            Talk(11 + _healthLine++);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        if (_cast <= diff)
        {
            switch (me->GetEntry())
            {
                case Mage:
                    DoCastVictim(urand(0, 1) ? 11921 : 12675);
                    break;
                case Hunter:
                    DoCastVictim(78754);
                    break;
                default:
                    break;
            }
            _cast = 3000;
        }
        else
            _cast -= diff;

        DoMeleeAttackIfReady();
    }

private:
    uint32 _cast = 2000;
    uint8 _healthLine = 0;
};

struct npc_westfall_vision_camera : ScriptedAI
{
    explicit npc_westfall_vision_camera(Creature* creature) : ScriptedAI(creature) {}

    void PassengerBoarded(Unit* who, int8 seat, bool apply) override
    {
        Player* player = who->ToPlayer();
        if (seat != 0 || !player)
            return;
        if (!apply)
        {
            if (_rider == player->GetGUID() && !_closing)
            {
                player->RemoveAurasDueToSpell(Ride);
                player->AddDelayedEvent(1, [player]()
                {
                    if (player->IsInWorld() && player->GetMapId() == WestfallVision::Map && !player->GetVehicle())
                        player->NearTeleportTo(VisionExit);
                });
                Close(nullptr);
            }
            return;
        }

        auto scope = me->GetQuestObjectScope().lock();
        if (!scope || scope->Cancelled || scope->Owner != player->GetGUID() || !_rider.IsEmpty() ||
            player->GetQuestStatus(WestfallVision::Quest) != QUEST_STATUS_INCOMPLETE || player->GetVehicleBase() != me || !player->IsAlive())
            return;

        _rider = player->GetGUID();
        _lifetime = 180000;
        me->SetCanFly(true);
        me->SetDisableGravity(true);
        me->NearTeleportTo(VisionEntry);
        Talk(0, player->GetGUID());
        _events.ScheduleEvent(Fly, 3000);
    }

    uint32 GetData(uint32 type) const override
    {
        return type == WestfallVision::Quest && _canCredit && !_closing ? 1 : 0;
    }

    void CalcExitVehiclePos(Position& position) override
    {
        if (!_rider.IsEmpty())
            position = VisionExit;
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == EFFECT_MOTION_TYPE && id == Fly && _flying && !_closing &&
            me->IsWithinDist3d(VisionFlight[5].x, VisionFlight[5].y, VisionFlight[5].z, 3.0f))
        {
            _flying = false;
            _events.ScheduleEvent(Stage, 1);
        }
    }

    void SummonedCreatureDies(Creature* actor, Unit* killer) override
    {
        if (!_fighting || actor->GetGUID() != _edwin || !killer || !_castMembers.count(killer->GetGUID()))
            return;
        _fighting = false;
        _corpse = actor->GetPosition();
        if (Creature* warrior = ObjectAccessor::GetCreature(*me, _warrior))
            warrior->AI()->Talk(12);
        _events.ScheduleEvent(VanessaWalk, 9000);
    }

    void UpdateAI(uint32 diff) override
    {
        if (_closing)
            return;
        Player* rider = ObjectAccessor::GetPlayer(*me, _rider);
        if (_lifetime <= diff || me->IsQuestObjectExpired() ||
            (!_rider.IsEmpty() && (!rider || rider->GetVehicleBase() != me || !rider->IsAlive() ||
                rider->GetQuestStatus(WestfallVision::Quest) != QUEST_STATUS_INCOMPLETE)))
        {
            Close(rider);
            return;
        }
        _lifetime -= diff;

        _events.Update(diff);
        switch (_events.ExecuteEvent())
        {
            case Fly:
                _flying = true;
                me->GetMotionMaster()->MoveSmoothPath(Fly, VisionFlight, std::size(VisionFlight), false);
                break;
            case Stage:
                if (!PublishActors())
                {
                    Close(rider);
                    break;
                }
                if (Creature* warrior = ObjectAccessor::GetCreature(*me, _warrior))
                    warrior->AI()->Talk(10);
                _events.ScheduleEvent(Challenge, 4000);
                break;
            case Challenge:
                if (Creature* warrior = ObjectAccessor::GetCreature(*me, _warrior))
                    warrior->AI()->Talk(11);
                _events.ScheduleEvent(Battle, 4000);
                break;
            case Battle:
                if (!BeginCombat())
                    Close(rider);
                break;
            case VanessaWalk:
                if (Creature* warrior = ObjectAccessor::GetCreature(*me, _warrior); warrior && warrior->IsAlive())
                    warrior->AI()->Talk(13);
                if (Creature* child = ObjectAccessor::GetCreature(*me, _vanessa))
                {
                    Position destination = _corpse;
                    float angle = _corpse.GetAngle(child);
                    destination.m_positionX += 0.8f * std::cos(angle);
                    destination.m_positionY += 0.8f * std::sin(angle);
                    child->GetMotionMaster()->MovePoint(VanessaWalk, destination);
                    _events.ScheduleEvent(VanessaArrive, 6000);
                }
                else
                    Close(rider);
                break;
            case VanessaArrive:
                if (Creature* child = ObjectAccessor::GetCreature(*me, _vanessa); child && child->IsWithinDist3d(&_corpse, 3.0f))
                {
                    child->AI()->Talk(10);
                    _events.ScheduleEvent(Reward, 3000);
                }
                else
                    Close(rider);
                break;
            case Reward:
                if (rider)
                {
                    _canCredit = true;
                    rider->CastSpell(rider, Credit, true);
                    _canCredit = false;
                }
                Close(rider);
                break;
        }
    }

    void Close(Player* rider)
    {
        if (_closing)
            return;
        _closing = true;
        _events.Reset();

        if (rider && rider->GetVehicleBase() == me)
        {
            me->NearTeleportTo(VisionExit);
            rider->ExitVehicle(&VisionExit);
            rider->RemoveAurasDueToSpell(Ride);
        }

        for (ObjectGuid const& guid : _actors)
            if (Creature* actor = ObjectAccessor::GetCreature(*me, guid))
                actor->DespawnOrUnsummon();
        me->DespawnOrUnsummon();
    }

private:
    enum Event : uint32 { Fly = 1, Stage, Challenge, Battle, VanessaWalk, VanessaArrive, Reward };

    bool PublishActors()
    {
        for (VisionActor const& definition : VisionActors)
        {
            Creature* actor = me->SummonCreature(definition.Entry, definition.Placement, TEMPSUMMON_MANUAL_DESPAWN);
            if (!actor)
                return false;
            _actors.push_back(actor->GetGUID());
            actor->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            actor->SetReactState(REACT_PASSIVE);
            if (definition.Entry == Vanessa)
            {
                _vanessa = actor->GetGUID();
                continue;
            }
            if (!actor->AIM_Initialize(new VisionCombatAI(actor)))
                return false;
            if (definition.Entry == Rogue)
                actor->CastSpell(actor, 60847, true);
            else if (definition.Entry == Priest)
                actor->CastSpell(actor, 13864, true);
            if (definition.Entry == Edwin)
                _edwin = actor->GetGUID();
            else if (definition.Entry == Warrior)
                _warrior = actor->GetGUID();
            if (definition.Entry != Edwin && definition.Entry != Blackguard)
                _castMembers.insert(actor->GetGUID());
        }
        return true;
    }

    bool BeginCombat()
    {
        Creature* edwin = ObjectAccessor::GetCreature(*me, _edwin);
        Creature* warrior = ObjectAccessor::GetCreature(*me, _warrior);
        if (!edwin || !warrior)
            return false;
        _fighting = true;
        edwin->AI()->Talk(10);
        for (ObjectGuid const& guid : _actors)
            if (Creature* actor = ObjectAccessor::GetCreature(*me, guid); actor && actor->GetEntry() != Vanessa)
            {
                bool defias = actor->GetEntry() == Edwin || actor->GetEntry() == Blackguard;
                static_cast<VisionCombatAI*>(actor->AI())->Begin(defias ? warrior : edwin);
            }
        return true;
    }

    EventMap _events;
    ObjectGuid _rider;
    ObjectGuid _edwin;
    ObjectGuid _vanessa;
    ObjectGuid _warrior;
    std::vector<ObjectGuid> _actors;
    GuidUnorderedSet _castMembers;
    Position _corpse;
    uint32 _lifetime = 8000;
    bool _closing = false;
    bool _flying = false;
    bool _fighting = false;
    bool _canCredit = false;
};

class spell_westfall_vision_incense : public SpellScript
{
    PrepareSpellScript(spell_westfall_vision_incense);

    SpellCastResult CheckVision()
    {
        Item* incense = GetCastItem();
        return incense && incense->GetEntry() == Incense && CanStart(GetCaster()->ToPlayer()) ? SPELL_CAST_OK : SPELL_FAILED_NOT_READY;
    }

    void Register() override { OnCheckCast += SpellCheckCastFn(spell_westfall_vision_incense::CheckVision); }
};

class player_westfall_vision_cleanup : public PlayerScript
{
public:
    player_westfall_vision_cleanup() : PlayerScript("player_westfall_vision_cleanup") {}

    void OnBeforeLogout(Player* player) override
    {
        if (Creature* camera = player->GetVehicleCreatureBase(); camera && camera->GetEntry() == Camera &&
            camera->HasQuestObjectScope() && camera->IsAIEnabled)
            if (auto ai = dynamic_cast<npc_westfall_vision_camera*>(camera->AI()))
            {
                ai->Close(player);
                player->UpdatePosition(VisionExit, true);
            }
    }
};

void RegisterWestfallVision()
{
    RegisterCreatureAI(npc_westfall_vision_camera);
    RegisterSpellScript(spell_westfall_vision_incense);
    new player_westfall_vision_cleanup();
}
}

#endif
