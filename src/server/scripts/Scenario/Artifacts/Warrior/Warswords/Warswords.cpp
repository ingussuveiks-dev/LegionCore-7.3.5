/*
    Warrior: Warswords
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "GameObjectAI.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include <algorithm>

// 245188
class go_mystic_bonfire : public GameObjectScript
{
public:
    go_mystic_bonfire() : GameObjectScript("go_mystic_bonfire") { }

    struct go_mystic_bonfireAI : public GameObjectAI
    {
        go_mystic_bonfireAI(GameObject* go) : GameObjectAI(go) {}

        bool GossipHello(Player* player, bool isUse) override
        {
            if (!isUse)
                return true;

            if (InstanceScript* script = go->GetInstanceScript())
                if (script->getScenarionStep() == 0)
                {
                    // player->CastSpell(player, 198513);
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 47272);
                    // delete smoge
                    std::list<GameObject*> goList;
                    go->GetGameObjectListWithEntryInGrid(goList, 244751, 45.0f);
                    if (!goList.empty())
                        for (std::list<GameObject*>::const_iterator itr = goList.begin(); itr != goList.end(); ++itr)
                            (*itr)->SetPhaseMask(4, true);
                    return true;
                }
            return false;
        }

    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return new go_mystic_bonfireAI(go);
    }
};

// 244940
class go_prison_runestone : public GameObjectScript
{
public:
    go_prison_runestone() : GameObjectScript("go_prison_runestone") { }

    struct go_prison_runestoneAI : public GameObjectAI
    {
        go_prison_runestoneAI(GameObject* go) : GameObjectAI(go)
        {
            check = false;
        }

        bool check;

        bool GossipHello(Player* player, bool isUse) override
        {
            if (!isUse)
                return true;

            if (check)
                return false;

            if (InstanceScript* script = go->GetInstanceScript())
                if (script->getScenarionStep() == 3)
                {
                    check = true;
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46970);
                    std::list<Creature*> adds;
                    GetCreatureListWithEntryInGrid(adds, player, 98524, 10.0f);
                    if (!adds.empty())
                        for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                            (*itr)->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, 10);
                    return true;
                }
            return false;
        }

    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return new go_prison_runestoneAI(go);
    }
};

// 98007
class npc_vigfus_bladewind : public CreatureScript
{
public:
    npc_vigfus_bladewind() : CreatureScript("npc_vigfus_bladewind") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_vigfus_bladewindAI(creature);
    }

    struct npc_vigfus_bladewindAI : ScriptedAI
    {
        npc_vigfus_bladewindAI(Creature* creature) : ScriptedAI(creature), summons(me) {}

        EventMap events;
        SummonList summons;
        uint32 count_adds;
        bool phase, check_first_rephase;
        bool secondRetreatStarted;
        ObjectGuid spectres[2] = {};
        bool spectreDefeated[2] = {};
        bool spectreActive = false;
        uint32 spectreSlot = 2;
        uint32 spectreRetryTimer = 0;
        uint32 spectreGeneration = 0;

        void CancelSpectreRush()
        {
            ++spectreGeneration;
            spectreActive = false;
            spectreSlot = 2;
            spectreRetryTimer = 0;
            count_adds = 0;
            for (uint32 index = 0; index < 2; ++index)
            {
                spectres[index] = {};
                spectreDefeated[index] = false;
            }
        }

        void Reset() override
        {
            phase = false;
            check_first_rephase = false;
            secondRetreatStarted = false;

            events.Reset();
            CancelSpectreRush();
            summons.DespawnAll();
            me->RemoveAura(195996);
            count_adds = 0;
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void EnterCombat(Unit* who) override
        {
            Talk(0);
            events.RescheduleEvent(EVENT_1, 7000); // 195574
            events.RescheduleEvent(EVENT_2, 10000); // 195575 23 + after 195996
            events.RescheduleEvent(EVENT_3, 13000); // 195996
        }

        void JustSummoned(Creature* summon) override
        {
            if (spectreActive && spectreSlot < 2 && summon->GetEntry() == 99609 && summon->IsInWorld() && summon->IsAlive())
                spectres[spectreSlot] = summon->GetGUID();
            summons.Summon(summon);
            summon->CastSpell(summon, 197852);
            summon->GetMotionMaster()->MoveRandom(10.0f);
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
        {
            if (!spectreActive || !me->IsAlive() || !summon || summon->IsAlive() || summon->GetEntry() != 99609)
                return;
            for (uint32 index = 0; index < 2; ++index)
                if (!spectreDefeated[index] && spectres[index] == summon->GetGUID())
                {
                    spectreDefeated[index] = true;
                    if (++count_adds == 2)
                    {
                        spectreActive = false;
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->RemoveAura(195996);
                    }
                    return;
                }
        }

        void SummonedCreatureDespawn(Creature* summon) override
        {
            if (!summon)
                return;
            summons.Despawn(summon);
            if (!spectreActive)
                return;
            for (uint32 index = 0; index < 2; ++index)
                if (!spectreDefeated[index] && spectres[index] == summon->GetGUID())
                    spectres[index] = {};
        }

        void UpdateSpectreRush(uint32 diff)
        {
            if (phase || !spectreActive || !me->IsAlive() || !me->IsInWorld())
                return;
            if (spectreRetryTimer > diff)
            {
                spectreRetryTimer -= diff;
                return;
            }
            spectreRetryTimer = 1000;
            uint32 generation = spectreGeneration;
            for (uint32 index = 0; index < 2; ++index)
            {
                if (spectreDefeated[index])
                    continue;
                if (Creature* spectre = ObjectAccessor::GetCreature(*me, spectres[index]))
                    if (spectre->IsAlive())
                        continue;
                spectres[index] = {};
                spectreSlot = index;
                me->SummonCreature(99609, me->GetPositionX() + irand(-5, 5), me->GetPositionY() + irand(-5, 5), me->GetPositionZ(), 0);
                if (generation != spectreGeneration || !spectreActive)
                    return;
                spectreSlot = 2;
            }
        }

        void BeginSpectreRush()
        {
            if (phase || spectreActive)
                return;
            CancelSpectreRush();
            spectreActive = true;
            uint32 generation = spectreGeneration;
            me->StopAttack();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetReactState(REACT_PASSIVE);
            DoCast(195996);
            if (generation != spectreGeneration || !spectreActive)
                return;
            UpdateSpectreRush(0);
            if (generation == spectreGeneration)
                events.RescheduleEvent(EVENT_3, 51000);
        }

        void JustDied(Unit*) override
        {
            CancelSpectreRush();
            summons.DespawnAll();
        }

        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType dmgType) override
        {
            if (check_first_rephase)
            {
                check_first_rephase = false;
                secondRetreatStarted = true;
                Talk(7);
                me->RemoveAura(128264);
                std::list<Creature*> adds;
                GetCreatureListWithEntryInGrid(adds, me, 98795, 30.0f);
                if (!adds.empty())
                    for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                    {
                        (*itr)->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                        (*itr)->AI()->AttackStart(attacker);
                    }
                me->GetMotionMaster()->MovePoint(2, 3514.32f, 1800.86f, 0.6f);
                me->DespawnOrUnsummon(7000);
            }
        }

        void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType) override
        {
            if (!attacker || !damage || !me->IsAlive() || me->GetEntry() != 98007 || me->GetMapId() != 1511)
                return;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 944 || scenario->GetCurrentStep() < 4 || scenario->GetCurrentStep() > 5)
                return;
            if (phase)
            {
                damage = 0;
                return;
            }
            if (scenario->GetCurrentStep() != 4)
                return;
            uint64 health = me->GetHealth(attacker);
            if (damage < health && health - damage >= me->CountPctFromMaxHealth(50, attacker))
                return;
            damage = 0;
            uint64 retreatHealth = std::max<uint64>(1, me->CountPctFromMaxHealth(50));
            if (me->GetHealth() > retreatHealth)
                me->SetHealth(retreatHealth);
            Player* player = attacker->GetCharmerOrOwnerPlayerOrPlayerItself();
            if (!player || !player->IsAlive() || !player->CanContact() || player->getClass() != CLASS_WARRIOR ||
                player->GetMap() != me->GetMap() || !player->InSamePhase(me) || !player->IsWithinDistInMap(me, 100.0f) ||
                player->GetQuestRewardStatus(40043) ||
                (player->GetQuestStatus(40043) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40043) != QUEST_STATUS_COMPLETE))
                return;
            phase = true;
            CancelSpectreRush();
            summons.DespawnAll();
            me->RemoveAura(195996);
            Talk(5);
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46982);
            me->StopAttack();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
            me->GetMotionMaster()->MovePoint(1, 3444.66f, 1703.69f, 0.43f);
            events.Reset();
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == 1 && phase && !check_first_rephase && !secondRetreatStarted)
            {
                check_first_rephase = true;
                DoCast(128264);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
            }
        }


        void UpdateAI(uint32 diff) override
        {
            UpdateSpectreRush(diff);
            if (!UpdateVictim())
                return;

            if (spectreActive || me->HasAura(195996))
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING) || me->HasUnitState(UNIT_STATE_STUNNED))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(195574);
                        events.RescheduleEvent(EVENT_1, 11000); // 195574
                        break;
                    case EVENT_2:
                        DoCast(195575);
                        events.RescheduleEvent(EVENT_2, 23000); // 195575 23 + after 195996
                        break;
                    case EVENT_3:
                        BeginSpectreRush();
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};


// 98602
class npc_vigfus_bladewind_last : public CreatureScript
{
public:
    npc_vigfus_bladewind_last() : CreatureScript("npc_vigfus_bladewind_last") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_vigfus_bladewind_lastAI(creature);
    }

    struct npc_vigfus_bladewind_lastAI : ScriptedAI
    {
        npc_vigfus_bladewind_lastAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            introDone = false;
        }

        EventMap events;
        SummonList summons;
        uint32 count_adds;

        bool phase1, phase2, introDone;
        bool introPending = false;
        uint32 introGeneration = 0;
        uint32 introRetryTimer = 0;
        ObjectGuid spectres[2] = {};
        bool spectreDefeated[2] = {};
        bool spectreActive = false;
        uint32 spectreSlot = 2;
        uint32 spectreRetryTimer = 0;
        uint32 spectreGeneration = 0;

        void CancelSpectreRush()
        {
            ++spectreGeneration;
            spectreActive = false;
            spectreSlot = 2;
            spectreRetryTimer = 0;
            count_adds = 0;
            for (uint32 index = 0; index < 2; ++index)
            {
                spectres[index] = {};
                spectreDefeated[index] = false;
            }
        }

        void Reset() override
        {
            ++introGeneration;
            if (introPending)
                introDone = false;
            introPending = false;
            introRetryTimer = 0;
            phase1 = false;
            phase2 = false;
            events.Reset();
            CancelSpectreRush();
            summons.DespawnAll();
            me->RemoveAura(195996);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetReactState(REACT_AGGRESSIVE);
            count_adds = 0;
        }

        bool IsEncounterReady() const
        {
            if (me->GetEntry() != 98602 || me->GetMapId() != 1511)
                return true;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            return scenario && scenario->GetScenarioId() == 944 && scenario->GetCurrentStep() == 5;
        }

        bool CanAIAttack(Unit const*) const override
        {
            return IsEncounterReady();
        }

        bool CanBeVisibleFor(WorldObject const*) const override
        {
            if (me->GetEntry() != 98602 || me->GetMapId() != 1511)
                return true;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            return scenario && scenario->GetScenarioId() == 944 &&
                scenario->GetCurrentStep() >= 5 && scenario->GetCurrentStep() <= 7;
        }

        void AttackStart(Unit* target) override
        {
            if (IsEncounterReady())
                ScriptedAI::AttackStart(target);
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 7000); // 195574
            events.RescheduleEvent(EVENT_2, 10000); // 195575 23 + after 195996
            events.RescheduleEvent(EVENT_3, 13000); // 195996
            events.RescheduleEvent(EVENT_4, 54000); // 196135
        }

        void JustSummoned(Creature* summon) override
        {
            if (spectreActive && spectreSlot < 2 && summon->GetEntry() == 99609 && summon->IsInWorld() && summon->IsAlive())
                spectres[spectreSlot] = summon->GetGUID();
            summons.Summon(summon);
            summon->CastSpell(summon, 197852);
            summon->GetMotionMaster()->MoveRandom(10.0f);
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
        {
            if (!spectreActive || !me->IsAlive() || !summon || summon->IsAlive() || summon->GetEntry() != 99609)
                return;
            for (uint32 index = 0; index < 2; ++index)
                if (!spectreDefeated[index] && spectres[index] == summon->GetGUID())
                {
                    spectreDefeated[index] = true;
                    if (++count_adds == 2)
                    {
                        spectreActive = false;
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->RemoveAura(195996);
                    }
                    return;
                }
        }

        void SummonedCreatureDespawn(Creature* summon) override
        {
            if (!summon)
                return;
            summons.Despawn(summon);
            if (!spectreActive)
                return;
            for (uint32 index = 0; index < 2; ++index)
                if (!spectreDefeated[index] && spectres[index] == summon->GetGUID())
                    spectres[index] = {};
        }

        void UpdateSpectreRush(uint32 diff)
        {
            if (!spectreActive || !me->IsAlive() || !me->IsInWorld())
                return;
            if (spectreRetryTimer > diff)
            {
                spectreRetryTimer -= diff;
                return;
            }
            spectreRetryTimer = 1000;
            uint32 generation = spectreGeneration;
            for (uint32 index = 0; index < 2; ++index)
            {
                if (spectreDefeated[index])
                    continue;
                if (Creature* spectre = ObjectAccessor::GetCreature(*me, spectres[index]))
                    if (spectre->IsAlive())
                        continue;
                spectres[index] = {};
                spectreSlot = index;
                me->SummonCreature(99609, me->GetPositionX() + irand(-5, 5), me->GetPositionY() + irand(-5, 5), me->GetPositionZ(), 0);
                if (generation != spectreGeneration || !spectreActive)
                    return;
                spectreSlot = 2;
            }
        }

        void BeginSpectreRush()
        {
            if (spectreActive)
                return;
            CancelSpectreRush();
            spectreActive = true;
            uint32 generation = spectreGeneration;
            me->StopAttack();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetReactState(REACT_PASSIVE);
            DoCast(195996);
            if (generation != spectreGeneration || !spectreActive)
                return;
            UpdateSpectreRush(0);
            if (generation == spectreGeneration)
                events.RescheduleEvent(EVENT_3, 51000);
        }

        bool IsIntroTarget(Player* player, float distance) const
        {
            if (!player || !me->IsInWorld() || !me->IsAlive() || me->GetMapId() != 1511 || !player->IsInWorld() ||
                !player->IsAlive() || !player->CanContact() || player->IsBeingTeleported() ||
                player->getClass() != CLASS_WARRIOR || player->GetMap() != me->GetMap() || !player->InSamePhase(me) ||
                me->GetExactDist2d(player) > distance || player->GetQuestRewardStatus(40043) ||
                (player->GetQuestStatus(40043) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40043) != QUEST_STATUS_COMPLETE))
                return false;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            return scenario && scenario->GetScenarioId() == 944 && scenario->GetCurrentStep() == 5;
        }

        void MoveInLineOfSight(Unit* who) override
        {
            Player* player = who ? who->ToPlayer() : nullptr;
            if (introDone || !IsIntroTarget(player, 40.0f))
                return;
            introDone = true;
            introPending = true;
            uint32 generation = introGeneration;
            ObjectGuid playerGuid = player->GetGUID();
            player->CreateConversation(843);
            if (Creature* helya = me->FindNearestCreature(99043, 100.0f, true))
                helya->CastSpell(helya, 195062);
            me->AddDelayedEvent(8000, [this, generation, playerGuid]() -> void
            {
                if (generation != introGeneration || !introPending)
                    return;
                introPending = false;
                Player* player = ObjectAccessor::GetPlayer(*me, playerGuid);
                if (!IsIntroTarget(player, 100.0f))
                {
                    introDone = false;
                    return;
                }
                me->RemoveAura(205424);
                AttackStart(player);
            });
        }

        void UpdateIntro(uint32 diff)
        {
            if (introDone)
                return;
            if (introRetryTimer > diff)
            {
                introRetryTimer -= diff;
                return;
            }
            introRetryTimer = 1000;
            if (!me->IsInWorld() || !me->IsAlive() || me->GetMapId() != 1511 || me->HasUnitState(UNIT_STATE_SIGHTLESS) ||
                (!me->HasReactState(REACT_AGGRESSIVE) && !CanSeeEvenInPassiveMode()))
                return;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 944 || scenario->GetCurrentStep() != 5)
                return;
            std::list<Player*> players;
            me->GetPlayerListInGrid(players, 40.0f);
            for (Player* player : players)
                if (!player->isInFlight() && me->canSeeOrDetect(player, false, true) && IsIntroTarget(player, 40.0f))
                {
                    MoveInLineOfSight(player);
                    break;
                }
        }

        void JustDied(Unit* who) override
        {
            CancelSpectreRush();
            summons.DespawnAll();
            Talk(2);
            if (Player* player = who ? who->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr)
            {
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46987);
                player->CreateConversation(848);
            }
        }

        void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType) override
        {
            if (!IsEncounterReady())
            {
                damage = 0;
                return;
            }
            if (!attacker || !damage || !me->IsAlive() || me->GetEntry() != 98602 || me->GetMapId() != 1511)
                return;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 944 || scenario->GetCurrentStep() != 5)
                return;
            uint64 health = me->GetHealth(attacker);
            uint64 remaining = damage >= health ? 0 : health - damage;
            uint32 generation = introGeneration;
            if (!phase1 && remaining < me->CountPctFromMaxHealth(15, attacker))
            {
                phase1 = true;
                Talk(0);
            }
            if (generation != introGeneration || !me->IsAlive())
                return;
            if (!phase2 && remaining < me->CountPctFromMaxHealth(7, attacker))
            {
                phase2 = true;
                Talk(1);
                if (generation != introGeneration || !me->IsAlive())
                    return;
                me->StopAttack();
                me->GetMotionMaster()->MovePoint(0, 3515.14f, 1800.60f, 0.54f);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            UpdateIntro(diff);
            UpdateSpectreRush(diff);
            if (!UpdateVictim())
                return;

            if (spectreActive || me->HasAura(195996))
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING) || me->HasUnitState(UNIT_STATE_STUNNED))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(195574);
                        events.RescheduleEvent(EVENT_1, 11000); // 195574
                        break;
                    case EVENT_2:
                        DoCast(195575);
                        events.RescheduleEvent(EVENT_2, 23000); // 195575 23 + after 195996
                        break;
                    case EVENT_3:
                        BeginSpectreRush();
                        break;
                    case EVENT_4:
                        DoCast(196135);
                        events.RescheduleEvent(EVENT_4, 53000);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};

namespace WarswordsAcquisition
{
    bool CanTake(Player* player, GameObject* weapon)
    {
        if (!player || !weapon || weapon->GetEntry() != 248832 || weapon->GetMapId() != 1511 ||
            !player->IsAlive() || !player->CanContact() || player->getClass() != CLASS_WARRIOR ||
            player->GetMap() != weapon->GetMap() || !player->InSamePhase(weapon) ||
            !player->IsWithinDistInMap(weapon, weapon->GetInteractionDistance()) || player->GetQuestRewardStatus(40043) ||
            (player->GetQuestStatus(40043) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40043) != QUEST_STATUS_COMPLETE))
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(weapon->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 944 && scenario->GetCurrentStep() == 6;
    }
}

// 248832
class go_art_warswords : public GameObjectScript
{
public:
    go_art_warswords() : GameObjectScript("go_art_warswords") {}

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (!WarswordsAcquisition::CanTake(player, go))
            return true;
        if (player->HasItemCount(128908, 1, true))
        {
            go->SetLootState(GO_JUST_DEACTIVATED, player);
            return true;
        }
        return false;
    }

    void OnLootStateChanged(GameObject* go, uint32 state, Unit* target) override
    {
        if (target && state == GO_JUST_DEACTIVATED && WarswordsAcquisition::CanTake(target->ToPlayer(), go) &&
            go->getLootState() == GO_JUST_DEACTIVATED && target->ToPlayer()->HasItemCount(128908, 1, true))
        {
            if (target->GetTypeId() == TYPEID_PLAYER)
            {
                target->ToPlayer()->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 47017);
                target->ToPlayer()->UpdateAchievementCriteria(CRITERIA_TYPE_COMPLETE_SCENARIO, 944, 1);
                if (Creature* trigger = go->FindNearestCreature(98678, 10.0f))
                    trigger->DespawnOrUnsummon(1000);

                target->CastSpell(target, 205443, true);

                target->ToPlayer()->KilledMonsterCredit(98033);
                target->CreateConversation(849);
                target->AddDelayedEvent(12000, [target]() -> void
                {
                    target->CastSpell(target, 192085);
                });
            }
        }
    }
};

namespace WarswordsPresentation
{
    bool Complete(Player* player, SpellScene const* scene, std::string const& trigger)
    {
        if (!scene || scene->MiscValue != 1218 || scene->SceneScriptPackageID != 1580)
            return false;
        if (trigger != "complete" || !player || !player->HasSceneStatus(1218, SCENE_COMPLETE) ||
            !player->IsInWorld() || !player->CanContact() || !player->IsAlive() || player->IsBeingTeleported() ||
            player->getClass() != CLASS_WARRIOR || player->GetMapId() != 1511 || player->GetQuestRewardStatus(40043) ||
            (player->GetQuestStatus(40043) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40043) != QUEST_STATUS_COMPLETE) ||
            !player->GetItemByEntry(128908))
            return true;
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 944 || scenario->GetCurrentStep() < 6 || scenario->GetCurrentStep() > 7)
            return true;
        SpellCastResult result = player->CastSpell(player, 194201, true);
        if (result != SPELL_CAST_OK)
        {
            TC_LOG_WARN("scripts", "Warswords pickup: native equip 194201 failed with result %u for player %u; existing items retained", uint32(result), player->GetGUIDLow());
            return true;
        }
        Item* weapon = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        Item* child = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
        if (!weapon || weapon->GetEntry() != 128908 || !child || child->GetEntry() != 134553 || weapon->GetChildItem() != child->GetGUID())
            TC_LOG_WARN("scripts", "Warswords pickup: native equip 194201 accepted but parent/child is not equipped for player %u; retain native inventory errors", player->GetGUIDLow());
        return true;
    }
}

class scene_warswords_pickup : public SceneTriggerScript
{
public:
    scene_warswords_pickup() : SceneTriggerScript("scene_warswords_pickup") { }
    bool OnTrigger(Player* player, SpellScene const* scene, std::string trigger) override
    {
        return WarswordsPresentation::Complete(player, scene, trigger);
    }
};

void AddSC_Warswords()
{
    new go_mystic_bonfire();
    new go_prison_runestone();

    new npc_vigfus_bladewind();
    new npc_vigfus_bladewind_last();

    new go_art_warswords();
    new scene_warswords_pickup();
};
