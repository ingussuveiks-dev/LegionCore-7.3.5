#pragma once

#include "CreatureTextMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "Timer.h"
#include "Vehicle.h"

namespace MonkHaitOpening
{
constexpr uint32 AcceptData = 94354, AcceptedData = 94355;
inline bool Source(Creature* actor)
{
    return actor && actor->GetEntry() == 97778 && actor->GetMapId() == 1014 && actor->GetDBTableGUIDLow() == 362105;
}

// Native197120 and194020 receipts transfer the accepted opening from the
// transient AI to its private instance. No scenario credit is inferred here.
struct State
{
    explicit State(Map* origin) : map(origin) { }
    Map* map;
    ObjectGuid hait, chuan, demon, owner;
    std::weak_ptr<uint8> quest;
    uint32 began = 0;
    uint8 issued = 0;
    bool updating = false, ended = false, launched = false;

    Scenario* Progress() const
    {
        if (!map || map->IsMapUnload() || map->GetId() != 1014) return nullptr;
        auto scenario = sScenarioMgr->GetScenario(map->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 943 && scenario->GetCurrentStep() == 0 ? scenario : nullptr;
    }

    Player* Retained() const
    {
        auto player = ObjectAccessor::FindPlayer(owner);
        auto status = player ? player->getQuestStatus(12103) : nullptr;
        auto attempt = quest.lock();
        return player && status && attempt && status->ScriptLifetime == attempt && player->GetMap() == map &&
            player->getClass() == CLASS_MONK && !player->GetQuestRewardStatus(12103) &&
            player->GetQuestStatus(12103) == QUEST_STATUS_INCOMPLETE && player->FindQuestSlot(12103) < MAX_QUEST_LOG_SIZE ? player : nullptr;
    }

    Creature* Actor(ObjectGuid guid, uint32 entry, uint64 spawn, ObjectGuid viewer = ObjectGuid::Empty) const
    {
        auto actor = map ? map->GetCreature(guid) : nullptr;
        return actor && actor->GetMap() == map && actor->GetEntry() == entry && actor->GetDBTableGUIDLow() == spawn &&
            actor->IsInWorld() && !actor->IsPreDelete() && actor->IsAlive() && !actor->ShouldHideFor(owner.IsEmpty() ? viewer : owner) ? actor : nullptr;
    }

    void Observe(Creature* actor)
    {
        if (!actor || actor->GetMap() != map) return;
        if (Source(actor) && hait.IsEmpty()) hait = actor->GetGUID();
        if (actor->GetEntry() == 98001 && actor->GetDBTableGUIDLow() == 362111 && chuan.IsEmpty()) chuan = actor->GetGUID();
        if (actor->GetEntry() == 98011 && actor->GetDBTableGUIDLow() == 362135 && demon.IsEmpty()) demon = actor->GetGUID();
    }

    void Protect(Creature* actor, uint32& damage) const
    {
        // Preserve this original friendly actor's DamageTaken=0 even while
        // its AI is absent during the recovered Demon encounter.
        if (Source(actor) && actor->GetMap() == map && actor->GetGUID() == hait) damage = 0;
    }

    void Accept(ObjectGuid playerGuid)
    {
        if (!owner.IsEmpty() || !Progress()) return;
        auto player = ObjectAccessor::FindPlayer(playerGuid);
        auto status = player ? player->getQuestStatus(12103) : nullptr;
        auto actor = Actor(hait, 97778, 362105, playerGuid);
        auto pupil = Actor(chuan, 98001, 362111, playerGuid);
        auto foe = Actor(demon, 98011, 362135, playerGuid);
        if (!player || !status || !status->ScriptLifetime || !actor || !pupil || !foe ||
            !actor->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK) || player->GetMap() != map ||
            !player->IsInWorld() || !player->CanContact() || !player->IsAlive() || player->HasPendingTeleport() ||
            player->getClass() != CLASS_MONK || player->GetQuestRewardStatus(12103) ||
            player->GetQuestStatus(12103) != QUEST_STATUS_INCOMPLETE || player->FindQuestSlot(12103) >= MAX_QUEST_LOG_SIZE ||
            !player->InSamePhase(actor) || !player->canSeeOrDetect(actor) || !player->IsWithinLOSInMap(actor) ||
            !actor->IsWithinDistInMap(player, INTERACTION_DISTANCE) || actor->FindNearestCreature(98001, 40.0f, true) != pupil ||
            pupil->FindNearestCreature(98011, 90.0f, true) != foe || player->GetQuestObjectiveData(12103, 97778) <= 0 ||
            player->GetQuestObjectiveData(12103, 98514) <= 0) return;
        owner = playerGuid;
        quest = status->ScriptLifetime;
        began = getMSTime();
    }

    static void AttackDemon(Creature* demon, ObjectGuid haitGuid, uint8 remaining = 10)
    {
        Creature* hait = ObjectAccessor::GetCreature(*demon, haitGuid);
        if (!demon->IsAlive() || !hait || !hait->IsAlive()) return;
        if (CreatureAI* ai = demon->AI()) { ai->AttackStart(hait); return; }
        if (!remaining)
        {
            TC_LOG_ERROR("scripts", "Monk intro: demon %s has no AI for its delayed attack after 10 retries", demon->GetGUID().ToString().c_str());
            return;
        }
        demon->AddDelayedEvent(500, [demon, haitGuid, remaining]() { AttackDemon(demon, haitGuid, remaining - 1); });
    }

    bool Valid(Creature* actor) const
    {
        return !ended && Progress() && Retained() && Actor(hait, 97778, 362105) == actor;
    }

    void Launch(Creature* actor, Creature* pupil, Creature* foe)
    {
        auto valid = [this, actor, pupil, foe]()
        {
            return Valid(actor) && Actor(chuan, 98001, 362111) == pupil && Actor(demon, 98011, 362135) == foe &&
                actor->InSamePhase(pupil) && pupil->InSamePhase(foe);
        };
        // Reserve before external calls. Optional text/camera failures must
        // not block the ordinary native movement, boarding or cleanup.
        if (!(issued & 8)) { issued |= 8; sCreatureTextMgr->SendChat(actor, 3); }
        if (!valid()) return;
        if (!(issued & 16)) { issued |= 16; actor->CastSpell(actor, 193053); }
        if (!valid()) return;
        if (!(issued & 32))
        {
            issued |= 32;
            pupil->SetWalk(true);
            pupil->GetMotionMaster()->MovePoint(0, 3812.82f, 1810.81f, 950.99f);
        }
        if (!valid()) return;
        if (!(issued & 64))
        {
            issued |= 64;
            if (GameObject* door = pupil->FindNearestGameObject(244682, 40.0f)) door->UseDoorOrButton(0, false, pupil);
        }
        if (!valid()) return;
        launched = true;
        foe->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
        ObjectGuid const chuanGuid = chuan, haitGuid = hait;
        // Transfer both clocks to the actual Demon before its movement can
        // reenter. No State/AI/Player pointer survives in these callbacks.
        foe->AddDelayedEvent(20000, [foe, chuanGuid]()
        {
            if (Creature* pupil = ObjectAccessor::GetCreature(*foe, chuanGuid)) pupil->CastSpell(foe, 227140);
            foe->CastSpell(3825.61f, 1770.73f, 950.98f, 193268);
        });
        foe->AddDelayedEvent(23000, [foe, chuanGuid, haitGuid]()
        {
            if (foe->GetVehicleKit()) foe->GetVehicleKit()->RemoveAllPassengers();
            if (Creature* pupil = ObjectAccessor::GetCreature(*foe, chuanGuid))
            {
                pupil->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                pupil->CastSpell(pupil, 185627);
            }
            if (Creature* guide = ObjectAccessor::GetCreature(*foe, haitGuid)) sCreatureTextMgr->SendChat(guide, 4);
            foe->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
            AttackDemon(foe, haitGuid);
        });
        pupil->AddDelayedEvent(8000, [pupil]() { sCreatureTextMgr->SendChat(pupil, 0); });
        pupil->AddDelayedEvent(15000, [pupil]() { sCreatureTextMgr->SendChat(pupil, 1); });
        foe->GetMotionMaster()->MovePoint(0, 3812.91f, 1812.55f, 950.99f);
    }

    void Update(uint32 /*diff*/)
    {
        if (updating || ended || owner.IsEmpty()) return;
        if (!Progress() || !Retained()) { ended = true; return; }
        struct Dispatch { bool& busy; explicit Dispatch(bool& flag) : busy(flag) { busy = true; } ~Dispatch() { busy = false; } } dispatch(updating);
        auto actor = Actor(hait, 97778, 362105);
        if (actor) actor->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        if (launched) return;
        uint32 elapsed = GetMSTimeDiffToNow(began);
        if (elapsed >= 120000)
        {
            ended = true;
            TC_LOG_WARN("scripts", "Monk intro: accepted Hight opening did not retain its original actors within120s; no movement or progression inferred");
            return;
        }
        if (!actor) return;
        for (uint8 bit = 1, line = 0; bit != 8; bit <<= 1, ++line)
        {
            uint32 deadline = line == 0 ? 1000 : line == 1 ? 4000 : 7000;
            if (!(issued & bit) && elapsed >= deadline)
            {
                issued |= bit;
                auto player = Retained();
                if (line || (player && player->IsAlive() && player->IsInWorld() && player->CanContact() && !player->HasPendingTeleport() &&
                    player->InSamePhase(actor) && actor->IsWithinDistInMap(player, 40.0f)))
                    sCreatureTextMgr->SendChat(actor, line, line ? ObjectGuid::Empty : owner);
                if (!Valid(actor)) return;
            }
        }
        if (elapsed < 10000) return;
        auto pupil = Actor(chuan, 98001, 362111);
        auto foe = Actor(demon, 98011, 362135);
        if (!pupil || !foe || !actor->InSamePhase(pupil) || !pupil->InSamePhase(foe) ||
            actor->FindNearestCreature(98001, 40.0f, true) != pupil || pupil->FindNearestCreature(98011, 90.0f, true) != foe) return;
        Launch(actor, pupil, foe);
    }
};
}
