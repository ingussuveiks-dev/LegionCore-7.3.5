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

#include "DynamicTree.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Item.h"
#include "KyleLunch.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"

/*######
# npc_skorn_whitecloud
######*/

#define GOSSIP_SW "Tell me a story, Skorn."

class npc_skorn_whitecloud : public CreatureScript
{
public:
    npc_skorn_whitecloud() : CreatureScript("npc_skorn_whitecloud") {}

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF)
            player->SEND_GOSSIP_MENU(523, creature->GetGUID());

        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->isQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (!player->GetQuestRewardStatus(770))
            player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, GOSSIP_SW, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

        player->SEND_GOSSIP_MENU(522, creature->GetGUID());

        return true;
    }

};

/*#####
# npc_kyle_frenzied
######*/

struct npc_kyle_frenzied : ScriptedAI
{
    explicit npc_kyle_frenzied(Creature* creature) : ScriptedAI(creature) { }

    bool CanAcceptLunch() const
    {
        return _stage == Stage::Idle && me->GetEntry() == KyleLunch::Frenzied && me->IsAlive() && !me->isInCombat();
    }

    void Reset() override
    {
        if (GameObject* food = ObjectAccessor::GetGameObject(*me, _food))
            food->Delete();
        _food.Clear();
        _owner.Clear();
        _stage = Stage::Idle;
        _timer = 0;
        me->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, EMOTE_STATE_NONE);
        if (me->GetEntry() == KyleLunch::Friendly)
            me->UpdateEntry(KyleLunch::Frenzied);
    }

    void SpellHit(Unit* caster, SpellInfo const* spell) override
    {
        Player* player = caster->ToPlayer();
        if (spell->Id != KyleLunch::Spell || !CanAcceptLunch() || !CanFeed(player))
            return;

        GameObject* food = player->GetGameObject(KyleLunch::Spell);
        if (!food || food->GetEntry() != KyleLunch::Food || !food->HasQuestObjectScope() || food->IsQuestObjectExpired())
            return;

        _food = food->GetGUID();
        _owner = player->GetGUID();
        me->GetMotionMaster()->Clear(false);
        me->GetMotionMaster()->MoveIdle();
        me->StopMoving();
        Talk(0);
        me->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, EMOTE_ONESHOT_CREATURE_SPECIAL);
        _stage = Stage::Notice;
        _timer = 5000;
    }

    void MovementInform(uint32 type, uint32 point) override
    {
        if (type != POINT_MOTION_TYPE || point != LunchPoint || _stage != Stage::Approach)
            return;
        if (GameObject* food = ObjectAccessor::GetGameObject(*me, _food))
            ReachFood(food);
    }

    void UpdateAI(uint32 diff) override
    {
        if (_stage == Stage::Idle)
            return;

        Player* owner = ObjectAccessor::GetPlayer(*me, _owner);
        GameObject* food = ObjectAccessor::GetGameObject(*me, _food);
        if (_stage != Stage::Dance && (!CanFeed(owner) || !me->IsWithinDistInMap(owner, 40.0f) ||
            !food || food->GetOwnerGUID() != _owner || food->IsQuestObjectExpired() || me->isInCombat()))
        {
            ResumeMovement();
            return;
        }

        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }

        switch (_stage)
        {
            case Stage::Notice:
                _stage = Stage::Approach;
                _timer = 12000;
                if (!ReachFood(food))
                    me->GetMotionMaster()->MovePoint(LunchPoint, food->GetPosition());
                break;
            case Stage::Eat:
                if (me->GetExactDist(food) > 2.5f)
                {
                    ResumeMovement();
                    break;
                }
                food->Delete();
                _food.Clear();
                owner->TalkedToCreature(KyleLunch::Frenzied, me->GetGUID());
                Talk(2);
                me->UpdateEntry(KyleLunch::Friendly);
                me->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, EMOTE_STATE_DANCESPECIAL);
                _stage = Stage::Dance;
                _timer = 30000;
                break;
            case Stage::Approach:
            case Stage::Dance:
                ResumeMovement();
                break;
            case Stage::Idle:
                break;
        }
    }

    static bool CanFeed(Player const* player)
    {
        return player && player->IsAlive() && player->GetMapId() == 1 &&
            player->GetQuestStatus(KyleLunch::Quest) == QUEST_STATUS_INCOMPLETE;
    }

private:
    enum class Stage { Idle, Notice, Approach, Eat, Dance };
    static constexpr uint32 LunchPoint = 1;

    bool ReachFood(GameObject* food)
    {
        if (me->GetExactDist(food) > 2.5f || !me->IsWithinLOS(food->GetPositionX(), food->GetPositionY(), food->GetPositionZ()))
            return false;
        Talk(1);
        me->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, EMOTE_STATE_USE_STANDING);
        _stage = Stage::Eat;
        _timer = 5000;
        return true;
    }

    void ResumeMovement()
    {
        Reset();
        me->StopMoving();
        me->GetMotionMaster()->Initialize();
    }

    ObjectGuid _owner;
    ObjectGuid _food;
    uint32 _timer = 0;
    Stage _stage = Stage::Idle;
};

class spell_kyle_lunch : public SpellScript
{
    PrepareSpellScript(spell_kyle_lunch);

    SpellCastResult CheckLunch()
    {
        Player* player = GetCaster()->ToPlayer();
        Item* item = GetCastItem();
        return npc_kyle_frenzied::CanFeed(player) && item && item->GetEntry() == KyleLunch::Item &&
            !player->GetGameObject(KyleLunch::Spell) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }

    void SelectKyle(WorldObject*& target)
    {
        Creature* creature = target ? target->ToCreature() : nullptr;
        auto kyle = creature ? dynamic_cast<npc_kyle_frenzied*>(creature->AI()) : nullptr;
        if (!kyle || !kyle->CanAcceptLunch() || !GetCaster()->IsWithinLOSInMap(creature))
            target = nullptr;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_kyle_lunch::CheckLunch);
        OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_kyle_lunch::SelectKyle, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
    }
};

namespace
{
constexpr uint32 OurTribeImprisoned = 24852;
constexpr uint32 QuilboarCage = 202112;
constexpr uint32 CapturedBrave = 38345;
constexpr uint32 RunFromCage = 1;

// The original occupants are paired with their cages by their world spawns.
struct CagePair { uint64 Cage; uint64 Brave; };
constexpr CagePair Captives[] = {
    {32629, 93501}, {32634, 93658}, {32635, 93664}, {32636, 93665}, {32639, 93702},
    {32640, 93706}, {32641, 93707}, {51369, 169785}, {51370, 169830}, {51371, 169831}
};

bool IsReadyQuilboarCage(GameObject const* cage)
{
    return cage && cage->isSpawned() && cage->GetGoState() == GO_STATE_READY && cage->getLootState() == GO_READY &&
        !cage->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE);
}

struct npc_mulgore_captured_brave : ScriptedAI
{
    explicit npc_mulgore_captured_brave(Creature* creature) : ScriptedAI(creature) { }

    void InitializeAI() override
    {
        if (me->GetMapId() != 1)
            return;
        for (CagePair const& pair : Captives)
            if (pair.Brave == me->GetDBTableGUIDLow())
            {
                ObjectGuid cage = ObjectGuid::Create<HighGuid::GameObject>(1, QuilboarCage, pair.Cage);
                if (!IsReadyQuilboarCage(ObjectAccessor::GetGameObject(*me, cage)))
                {
                    _cage = cage;
                    _state = WaitingForCage;
                    me->SetVisible(false);
                }
                return;
            }
    }

    bool IsCaptive() const { return _state == Captive && me->IsAlive(); }

    void BeginEscape(GameObject const& cage)
    {
        _cage = cage.GetGUID();
        _state = Opening;
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (_state == Escaping && type == POINT_MOTION_TYPE && id == RunFromCage)
            _timer = 0;
    }

    void UpdateAI(uint32 diff) override
    {
        switch (_state)
        {
            case Captive:
                return;
            case Opening:
            {
                GameObject* cage = ObjectAccessor::GetGameObject(*me, _cage);
                if (!cage || cage->GetGoState() != GO_STATE_ACTIVE)
                {
                    _cage.Clear();
                    _state = Captive;
                    return;
                }

                // Native Use opens the model after OnStateChanged. Start
                // pathing on the next update through the already open door.
                Position destination = me->GetHomePosition();
                me->MovePositionToFirstCollision(destination, 12.0f, destination.GetOrientation() - me->GetOrientation());
                _state = Escaping;
                _timer = 8 * IN_MILLISECONDS;
                Talk(0);
                me->SetWalk(false);
                me->GetMotionMaster()->MovePoint(RunFromCage, destination);
                return;
            }
            case Escaping:
                if (_timer > diff)
                {
                    _timer -= diff;
                    return;
                }

                me->SetVisible(false);
                me->GetMotionMaster()->Clear(false);
                me->GetMotionMaster()->MoveIdle();
                me->NearTeleportTo(me->GetHomePosition());
                _state = WaitingForCage;
                _timer = IN_MILLISECONDS;
                return;
            case WaitingForCage:
                if (_timer > diff)
                {
                    _timer -= diff;
                    return;
                }

                _timer = IN_MILLISECONDS;
                if (IsReadyQuilboarCage(ObjectAccessor::GetGameObject(*me, _cage)))
                {
                    _cage.Clear();
                    _state = Captive;
                    me->SetVisible(true);
                }
                return;
        }
    }

private:
    enum State { Captive, Opening, Escaping, WaitingForCage };
    State _state = Captive;
    ObjectGuid _cage;
    uint32 _timer = 0;
};

struct go_mulgore_quilboar_cage : GameObjectAI
{
    explicit go_mulgore_quilboar_cage(GameObject* object) : GameObjectAI(object) { }

    bool GossipUse(Player* player) override
    {
        _opener.Clear();
        _brave.Clear();
        if (go->GetMapId() != 1 || go->GetEntry() != QuilboarCage || !IsReadyQuilboarCage(go) ||
            !player->IsAlive() || !player->CanContact() || player->IsSpectator() ||
            player->GetQuestStatus(OurTribeImprisoned) != QUEST_STATUS_INCOMPLETE ||
            !go->IsWithinDistInMap(player, go->GetInteractionDistance()) || !player->canSeeOrDetect(go) ||
            !CanReachCage(*player))
            return true;

        for (CagePair const& pair : Captives)
            if (pair.Cage == go->GetDBTableGUIDLow())
            {
                // Outdoor spawns use their original database GUIDs.
                Creature* captive = go->GetMap()->GetCreature(ObjectGuid::Create<HighGuid::Creature>(1, CapturedBrave, pair.Brave));
                auto ai = captive ? dynamic_cast<npc_mulgore_captured_brave*>(captive->AI()) : nullptr;
                if (ai && ai->IsCaptive() && go->IsWithinDistInMap(captive, 1.0f) && player->canSeeOrDetect(captive))
                {
                    _opener = player->GetGUID();
                    _brave = captive->GetGUID();
                    return false;
                }
                break;
            }
        return true;
    }

    void OnStateChanged(uint32 state, Unit* user) override
    {
        if (state != GO_ACTIVATED || !user || user->GetGUID() != _opener)
            return;
        if (Creature* captive = ObjectAccessor::GetCreature(*go, _brave))
            if (auto ai = dynamic_cast<npc_mulgore_captured_brave*>(captive->AI()))
                ai->BeginEscape(*go);
        _opener.Clear();
        _brave.Clear();
    }

private:
    bool CanReachCage(Player const& player) const
    {
        // The cage bars cover its interaction point. Exclude this cage only
        // from the ray to that point, keeping all surrounding collision.
        DynamicTreeCallback filter;
        filter.filter = [cage = go](GameObject const* obstruction) { return obstruction != cage; };
        Position target = go->GetHitSpherePointFor(player.GetPosition());
        return go->GetMap()->isInLineOfSight(player.GetPositionX(), player.GetPositionY(), player.GetPositionZ() + 2.0f,
            target.GetPositionX(), target.GetPositionY(), target.GetPositionZ() + 2.0f,
            player.GetPhases(), VMAP::ModelIgnoreFlags::Nothing, &filter, &player);
    }

    ObjectGuid _opener;
    ObjectGuid _brave;
};
}

void AddSC_mulgore()
{
    new npc_skorn_whitecloud();
    RegisterCreatureAI(npc_kyle_frenzied);
    RegisterSpellScript(spell_kyle_lunch);
    RegisterCreatureAI(npc_mulgore_captured_brave);
    RegisterGameObjectAI(go_mulgore_quilboar_cage);
}
