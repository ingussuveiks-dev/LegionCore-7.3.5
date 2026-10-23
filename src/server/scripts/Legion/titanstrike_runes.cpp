#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "Pet.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "ObjectAccessor.h"
#include "MotionMaster.h"
#include "titanstrike_rune_wave.h"
#include <vector>

struct npc_titanstrike_prustaga : SmartAI
{
    explicit npc_titanstrike_prustaga(Creature* creature) : SmartAI(creature) { }
    TitanstrikeRunes::Wave<ObjectGuid> wave;
    std::vector<ObjectGuid> actors;
    uint32 timer = 0;
    uint32 workshopShotTimer = 0;

    Scenario* Progress() const
    {
        auto progress = me->GetMapId() == 1495 ? sScenarioMgr->GetScenario(me->GetInstanceId()) : nullptr;
        return progress && progress->GetScenarioId() == 1068 ? progress : nullptr;
    }

    Player* Participant(bool requireAlive = true) const
    {
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (auto player = ref.getSource())
                if (player->getClass() == CLASS_HUNTER && player->GetQuestStatus(41574) == QUEST_STATUS_INCOMPLETE &&
                    (!requireAlive || player->IsAlive()) && me->InSamePhaseId(player) &&
                    (!requireAlive || me->GetDistance(player) < 100.0f))
                    return player;
        return nullptr;
    }

    void Cleanup()
    {
        for (auto guid : actors)
            if (auto actor = ObjectAccessor::GetCreature(*me, guid))
                actor->DespawnOrUnsummon();
        actors.clear();
        wave = {};
    }

    void ResetEncounter()
    {
        Cleanup();
        GetScript()->OnReset();
        for (uint32 entry : {106322, 106323, 106324})
            if (auto rune = me->FindNearestCreature(entry, 100.0f, false))
                rune->Respawn();
        if (auto door = me->FindNearestGameObject(248780, 100.0f))
            door->ResetDoorOrButton();
    }

    void JustDied(Unit* killer) override
    {
        Cleanup();
        SmartAI::JustDied(killer);
    }

    void SetData(uint32 id, uint32 value) override
    {
        auto progress = Progress();
        if (progress && progress->GetCurrentStep() == 2)
        {
            if (id == 2 && value == 2)
            {
                if (wave.started)
                    return;
                wave.Start();
            }
            else if (id == 7 && value >= 1 && value <= 3)
            {
                wave.Arm(value); // native ritual list reached this wave's summon point
                return;
            }
            else if (id >= 3 && id <= 5 && value == id)
                return; // legacy proximity/killer-based death reports cannot advance a wave
        }
        SmartAI::SetData(id, value);
    }

    ObjectGuid Spawn(TitanstrikeRunes::Enemy const& enemy, ObjectGuid parent)
    {
        Creature* summoner = parent.IsEmpty() ? me : ObjectAccessor::GetCreature(*me, parent);
        // A tracker can die before a failed wolf summon retries. Retain the
        // required wolf slot, using Prustaga if the parent's corpse is gone.
        if (!summoner)
            summoner = me;
        auto actor = summoner->SummonCreature(enemy.entry, enemy.x, enemy.y, enemy.z, 2.97149f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
        if (!actor)
        {
            TC_LOG_ERROR("scripts", "Titanstrike rune wave %u: failed to summon %u; retrying without progression", wave.index + 1, enemy.entry);
            return ObjectGuid::Empty;
        }
        actors.push_back(actor->GetGUID());
        if (auto player = Participant())
        {
            // The two trackers shoot from the rear; the guard and wolves charge.
            if (enemy.entry == 106341)
                if (auto ai = dynamic_cast<SmartAI*>(actor->AI()))
                    ai->SetCombatMove(false);
            actor->AI()->AttackStart(player);
        }
        return actor->GetGUID();
    }

    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff); // preserve all introduction, ritual and later-stage SmartAI
        if (timer > diff) { timer -= diff; return; }
        timer = 500;
        auto progress = Progress();
        if (!progress || !Participant(false))
        {
            if (wave.started)
                ResetEncounter();
            return;
        }
        if (progress->GetCurrentStep() == 3)
        {
            auto player = Participant();
            Unit* target = player ? player->getVictim() : nullptr;
            if (!target && player && player->GetPet())
                target = player->GetPet()->getVictim();
            if (workshopShotTimer > 500)
                workshopShotTimer -= 500;
            else if (target && target->IsAlive() && (target->GetEntry() == 106352 || target->GetEntry() == 106354) &&
                me->IsValidAttackTarget(target) && me->InSamePhaseId(target) && me->GetDistance(target) < 40.0f)
            {
                me->SetFacingToObject(target);
                me->CastSpell(target, 187207, false); // native Prustaga Thorim's Bolt
                workshopShotTimer = 3000;
            }
            return;
        }
        if (progress->GetCurrentStep() != 2 || !Participant())
            return;
        // A fresh/retried instance still at the rune objective can bootstrap
        // beside the door without depending on a consumed EventObject trigger.
        if (!wave.started && me->GetDistance(4891.58f, 314.941f, -36.4379f) < 35.0f)
            SetData(2, 2);
        if (wave.Update([this](TitanstrikeRunes::Enemy const& enemy, ObjectGuid parent) { return Spawn(enemy, parent); },
            [this](ObjectGuid guid) {
                auto actor = ObjectAccessor::GetCreature(*me, guid);
                return !actor ? TitanstrikeRunes::Life::Missing : actor->IsAlive() ? TitanstrikeRunes::Life::Alive : TitanstrikeRunes::Life::Dead;
            }))
            SmartAI::SetData(wave.index + 2, wave.index + 2);
    }
};

// Grif owns Theodore throughout Stage2 through the final stage, independently
// of Prustaga's rune controller and her later betrayal/despawn.
struct npc_titanstrike_grif : SmartAI
{
    explicit npc_titanstrike_grif(Creature* creature) : SmartAI(creature) { }
    ObjectGuid bearGuid;
    uint32 timer = 0, shotTimer = 0;
    bool repairing = false;

    void Cleanup()
    {
        if (auto bear = ObjectAccessor::GetCreature(*me, bearGuid))
            bear->DespawnOrUnsummon();
        bearGuid.Clear();
        if (repairing)
        {
            repairing = false;
            me->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, EMOTE_ONESHOT_NONE);
        }
    }

    void JustDied(Unit* killer) override
    {
        Cleanup();
        SmartAI::JustDied(killer);
    }

    void UpdateAI(uint32 diff) override
    {
        auto progress = me->GetMapId() == 1495 ? sScenarioMgr->GetScenario(me->GetInstanceId()) : nullptr;
        bool workshop = progress && progress->GetScenarioId() == 1068 && progress->GetCurrentStep() == 3;
        // StartPath refuses actors in combat. Release the previous rune fight
        // before the native one-shot workshop waypoint event is evaluated.
        if (workshop)
            me->CombatStop(true);
        SmartAI::UpdateAI(diff);
        if (timer > diff) { timer -= diff; return; }
        timer = 500;
        Player* player = nullptr;
        if (progress && progress->GetScenarioId() == 1068 && progress->GetCurrentStep() >= 1)
            for (auto const& ref : me->GetMap()->GetPlayers())
                if (auto candidate = ref.getSource())
                    if (candidate->getClass() == CLASS_HUNTER && candidate->GetQuestStatus(41574) == QUEST_STATUS_INCOMPLETE && me->InSamePhaseId(candidate))
                    { player = candidate; break; }
        if (!player)
        {
            Cleanup();
            return;
        }
        auto bear = ObjectAccessor::GetCreature(*me, bearGuid);
        if (bear && !bear->IsAlive())
        {
            bear->DespawnOrUnsummon();
            bear = nullptr;
        }
        if (!bear)
        {
            bear = me->SummonCreature(106289, me->GetPositionX() + 1.5f, me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN);
            if (!bear)
                return;
            bearGuid = bear->GetGUID();
            bear->setFaction(me->getFaction());
            bear->SetLevel(me->getLevel());
            bear->SetReactState(REACT_DEFENSIVE);
            bear->GetMotionMaster()->MoveFollow(me, 2.0f, 1.5f);
        }
        if (!player->IsAlive())
            return;
        if (workshop)
        {
            bear->CombatStop(true);
            bear->GetMotionMaster()->MoveFollow(me, 2.0f, 1.5f);
            auto robot = me->FindNearestCreature(105016, 6.0f, true);
            bool work = robot && robot->HasAura(210836);
            if (work)
                me->SetFacingToObject(robot);
            if (work != repairing)
            {
                repairing = work;
                me->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, work ? EMOTE_STATE_USE_STANDING : EMOTE_ONESHOT_NONE);
            }
            return; // Grif/Theodore repair; Prustaga assists the player's fight
        }
        if (repairing)
        {
            repairing = false;
            me->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, EMOTE_ONESHOT_NONE);
        }
        Unit* target = player->getVictim();
        if (!target)
            if (auto pet = player->GetPet())
                target = pet->getVictim();
        if (!target)
            for (auto attacker : *player->getAttackers())
                if (attacker->IsAlive() && me->IsValidAttackTarget(attacker))
                { target = attacker; break; }
        if (!target)
            target = bear->getVictim();
        if (!target || !target->IsAlive() || !me->IsValidAttackTarget(target) || me->GetDistance(target) > 80.0f || !me->InSamePhaseId(target))
        {
            if (!bear->isInCombat())
                bear->GetMotionMaster()->MoveFollow(me, 2.0f, 1.5f);
            return;
        }
        if (!bear->getVictim())
            bear->AI()->AttackStart(target);
        if (shotTimer <= 500)
        {
            shotTimer = 3000;
            me->SetFacingToObject(target);
            me->CastSpell(target, 6660, false);
        }
        else
            shotTimer -= 500;
    }
};

void AddSC_titanstrike_runes()
{
    RegisterCreatureAI(npc_titanstrike_prustaga);
    RegisterCreatureAI(npc_titanstrike_grif);
}
