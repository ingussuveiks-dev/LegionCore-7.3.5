/*
    Warriors: Intro Scenario
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "GameObjectAI.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "ObjectAccessor.h"
#include "MapManager.h"
#include "TemporarySummon.h"
#include "malgalor_scene.h"
#include "malgalor_finale.h"


// 97167
class npc_ragnvald_dragonborn : public CreatureScript
{
public:
    npc_ragnvald_dragonborn() : CreatureScript("npc_ragnvald_dragonborn") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_ragnvald_dragonbornAI(creature);
    }

    struct npc_ragnvald_dragonbornAI : ScriptedAI
    {
        npc_ragnvald_dragonbornAI(Creature* creature) : ScriptedAI(creature)
        {
            introDone1 = false;
            introDone2 = false;
        }

        bool introDone1, introDone2;
        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 12000); // 13736
            events.RescheduleEvent(EVENT_2, 6000); // 12 15708
            events.RescheduleEvent(EVENT_3, 15000); // 15588
            events.RescheduleEvent(EVENT_4, 9000); // 13 16044
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            if (!introDone1 && me->IsWithinDistInMap(who, 15.0f))
            {
                introDone1 = true;
                who->ToPlayer()->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46877);
                Conversation* conversation = new Conversation;
                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 1386, who, NULL, *who))
                    delete conversation;
                me->AddDelayedEvent(2000, [this]() -> void // start battle
                {
                    std::list<Creature*> adds;
                    GetCreatureListWithEntryInGrid(adds, me, 102990, 90.0f);
                    GetCreatureListWithEntryInGrid(adds, me, 102995, 90.0f);
                    GetCreatureListWithEntryInGrid(adds, me, 97166, 90.0f);
                    GetCreatureListWithEntryInGrid(adds, me, 97167, 90.0f);
                    if (!adds.empty())
                        for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                        {
                            if (!(*itr)->IsAlive() || (*itr)->HasAura(105419))
                                continue;
                            if (Creature* targ = (*itr)->FindNearestCreature(97176, 120.0f, true))
                                (*itr)->AI()->AttackStart(targ);
                            (*itr)->SetHomePosition(-1988.32f + irand(-6, 6), -1231.07f + irand(-7, 7), 4.99f, 0);
                        }
                    if (Creature* targ = me->FindNearestCreature(97176, 120.0f, true))
                        AttackStart(targ);
                    me->SetHomePosition(-1988.32f + irand(-6, 6), -1231.07f + irand(-7, 7), 4.99f, 0);

                    if (Creature* targ = me->FindNearestCreature(102970, 100.0f, true))
                    {
                        if (Creature* attacker = me->FindNearestCreature(102964, 100.0f, true))
                            attacker->AI()->AttackStart(targ);
                        if (Creature* attacker = me->FindNearestCreature(102965, 100.0f, true))
                            attacker->AI()->AttackStart(targ);
                    }
                });
            }

            if (!introDone2 && me->IsWithinDistInMap(who, 40.0f))
            {
                introDone2 = true;
                Conversation* conversation = new Conversation;
                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 770, who, NULL, *who))
                    delete conversation;

            }

        }

        void DoAction(int32 const action) override
        {
            Conversation* conversation = new Conversation;
            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 1412, me, NULL, *me))
                delete conversation;
            if (Creature* targ = me->FindNearestCreature(98257, 250.0f, true))
            {
                std::list<Creature*> adds;
                GetCreatureListWithEntryInGrid(adds, me, 102990, 90.0f);
                GetCreatureListWithEntryInGrid(adds, me, 102995, 90.0f);
                GetCreatureListWithEntryInGrid(adds, me, 97166, 90.0f);
                if (!adds.empty())
                    for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                    {
                        if (!(*itr)->IsAlive() || (*itr)->HasAura(105419))
                            continue;
                        (*itr)->AI()->AttackStart(targ);
                    }
                AttackStart(targ);
            }

        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(13736);
                        events.RescheduleEvent(EVENT_1, 12000); // 13736
                        break;
                    case EVENT_2:
                        DoCast(15708);
                        events.RescheduleEvent(EVENT_2, 6000); // 12 15708
                        break;
                    case EVENT_3:
                        DoCast(15588);
                        events.RescheduleEvent(EVENT_3, 15000); // 15588
                        break;
                    case EVENT_4:
                        DoCast(16044);
                        events.RescheduleEvent(EVENT_4, 9000); // 13 16044
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};
// 103608
class npc_danica_the_reclaimer : public CreatureScript
{
public:
    npc_danica_the_reclaimer() : CreatureScript("npc_danica_the_reclaimer") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_danica_the_reclaimerAI(creature);
    }

    struct npc_danica_the_reclaimerAI : ScriptedAI
    {
        npc_danica_the_reclaimerAI(Creature* creature) : ScriptedAI(creature)
        {
            event = false;
            timerevent = 2000;
        }

        bool event;
        uint32 timerevent;

        void IsSummonedBy(Unit* owner) override
        {
            if (Player* targ = me->FindNearestPlayer(100.0f))
            {
                targ->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46878);
                targ->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 47474);
            }
            std::list<Creature*> trash;
            GetCreatureListWithEntryInGrid(trash, me, 102964, 90.0f);
            GetCreatureListWithEntryInGrid(trash, me, 102965, 90.0f);
            GetCreatureListWithEntryInGrid(trash, me, 92445, 90.0f);
            GetCreatureListWithEntryInGrid(trash, me, 92446, 90.0f);
            GetCreatureListWithEntryInGrid(trash, me, 92450, 90.0f);
            GetCreatureListWithEntryInGrid(trash, me, 92447, 90.0f);
            GetCreatureListWithEntryInGrid(trash, me, 97176, 90.0f);

            if (!trash.empty())
                for (std::list<Creature*>::iterator itr = trash.begin(); itr != trash.end(); ++itr)
                {
                    (*itr)->Kill((*itr));
                }

            if (Creature* targ = me->FindNearestCreature(97166, 100.0f, true))
                targ->AI()->Talk(0);

            me->GetMotionMaster()->MovePoint(0, -1980.04f, -1251.96f, 4.83f);

            std::list<Creature*> adds;
            GetCreatureListWithEntryInGrid(adds, me, 102990, 160.0f);
            GetCreatureListWithEntryInGrid(adds, me, 102995, 90.0f);
            GetCreatureListWithEntryInGrid(adds, me, 97166, 90.0f);
            GetCreatureListWithEntryInGrid(adds, me, 97167, 90.0f);
            if (!adds.empty())
                for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                {
                    if (!(*itr)->IsAlive() || (*itr)->HasAura(105419))
                        continue;
                    (*itr)->GetMotionMaster()->MovePoint(0, -1965.64f + irand(-6, 6), -1246.74f + irand(-6, 6), 2.71f);
                    // (*itr)->SetByteFlag(UNIT_FIELD_BYTES_1, 3, 8);
                }

            me->AddDelayedEvent(3000, [this]() -> void
            {
                Conversation* conversation = new Conversation;
                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 1405, me, NULL, *me))
                    delete conversation;
                DoCast(205287); // + filter
                if (Player* targ = me->FindNearestPlayer(100.0f))
                {
                    targ->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49203);
                    targ->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49262);
                }
            });
        }

        void SpellHitTarget(Unit* target, SpellInfo const* spell) override
        {
            if (spell->Id == 205287)
            {
                event = true;
                if (Creature* targ = target->ToCreature())
                {
                    if (targ->GetEntry() == 102964)
                        targ->CastSpell(targ, 205295);
                    if (targ->GetEntry() == 102965)
                        targ->CastSpell(targ, 205296);
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (event)
            {
                if (timerevent <= diff)
                {
                    if (Creature* targ = me->FindNearestCreature(103688, 100.0f, true))
                    {
                        targ->GetMotionMaster()->MovePoint(0, targ->GetPositionX(), targ->GetPositionY(), 40.0f);
                        targ->DespawnOrUnsummon(4000);
                    }
                    if (Creature* targ = me->FindNearestCreature(103687, 100.0f, true))
                    {
                        targ->GetMotionMaster()->MovePoint(0, targ->GetPositionX(), targ->GetPositionY(), 40.0f);
                        targ->DespawnOrUnsummon(4000);
                    }
                    me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), 40.0f);
                    me->DespawnOrUnsummon(4000);
                    event = false;
                    // if (Player* targ = me->FindNearestPlayer(100.0f))
                        // targ->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, КРИТЕРИЙ_2_1);
                    if (Creature* targ = me->FindNearestCreature(97167, 100.0f, true))
                        targ->AI()->DoAction(true);
                }
                else
                    timerevent -= diff;
            }
        }

    };
};

// 98257
class npc_bezzered : public CreatureScript
{
public:
    npc_bezzered() : CreatureScript("npc_bezzered") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_bezzeredAI(creature);
    }

    struct npc_bezzeredAI : ScriptedAI
    {
        npc_bezzeredAI(Creature* creature) : ScriptedAI(creature) { }

        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 16000); // 196814
            events.RescheduleEvent(EVENT_2, 23000); // 17 196808
        }

        void DoAction(int32 const action) override
        {
            if (Creature* targ = me->FindNearestCreature(97167, 200.0f, true))
            {
                std::list<Creature*> adds;
                GetCreatureListWithEntryInGrid(adds, me, 97176, 30.0f);
                if (!adds.empty())
                    for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                    {
                        if (!(*itr)->IsAlive() || (*itr)->HasAura(105419))
                            continue;
                        (*itr)->AI()->AttackStart(targ);
                    }
                AttackStart(targ);
            }
        }

        void JustDied(Unit*) override
        {
            if (Player* targ = me->FindNearestPlayer(100.0f))
            {
                targ->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46880);
                Conversation* conversation = new Conversation;
                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 771, targ, NULL, *targ))
                    delete conversation;

                targ->AddDelayedEvent(8000, [targ]() -> void
                {
                    targ->CastSpell(targ, 197593, true);
                });

                me->AddDelayedEvent(24000, [this]() -> void
                {
                    Conversation* conversation = new Conversation;
                    if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 1414, me, NULL, *me))
                        delete conversation;
                });

                if (Creature* boss = me->FindNearestCreature(92608, 300.0f, true))
                    boss->AI()->DoAction(true);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(196814);
                        events.RescheduleEvent(EVENT_1, 16000); // 196814
                        break;
                    case EVENT_2:
                        DoCast(196808);
                        events.RescheduleEvent(EVENT_2, 23000); // 17 196808
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};

// 92608
class npc_malgalor : public CreatureScript
{
public:
    npc_malgalor() : CreatureScript("npc_malgalor") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_malgalorAI(creature);
    }

    struct npc_malgalorAI : ScriptedAI
    {
        npc_malgalorAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
        }

        EventMap events;
        uint32 convers = 789;
        bool introStarted = false, defeating = false, deathHandled = false, activeLease = false, wasActive = false;
        uint32 introTimer = 0, defeatTimer = 0;
        uint8 introStage = 0;
        ObjectGuid primary;
        std::vector<MalgalorFinale::Member> members;

        void Reset() override
        {
            bool restartIntro = introStage != 0;
            defeating = deathHandled = false;
            defeatTimer = introTimer = introStage = 0;
            members.clear(); primary.Clear(); convers = 789;
            if (activeLease) me->setActive(wasActive);
            activeLease = false;
            events.Reset();
            if (restartIntro) { introStarted = false; DoAction(0); }
        }

        void EnterCombat(Unit* who) override
        {
            convers = 789;
            events.RescheduleEvent(EVENT_1, 6000);
            events.RescheduleEvent(EVENT_2, 11000); // 207942
            events.RescheduleEvent(EVENT_3, 17000); // 197816
            events.RescheduleEvent(EVENT_4, 21000); // 197818
        }

        void DoAction(int32 const action) override
        {
            if (introStarted) return;
            introStarted = true; introStage = 1; introTimer = 24000;
            wasActive = me->isActiveObject(); activeLease = true; me->setActive(true);
        }

        void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType) override
        {
            Scenario* scenario = MalgalorFinale::Progress(me);
            if (!scenario || !me->IsAlive()) return;
            if (defeating) { damage = 0; return; }
            if (!damage || scenario->GetCurrentStep() != 4 || MalgalorFinale::Victory(scenario)) return;
            uint64 health = me->GetHealth(attacker);
            uint64 threshold = std::max<uint64>(1, me->CountPctFromMaxHealth(20, attacker));
            uint64 remaining = damage >= health ? 0 : health - damage;
            if (remaining >= threshold && health > threshold) return;
            members = MalgalorFinale::Capture(me);
            if (members.empty()) return;
            if (Player* owner = attacker ? attacker->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr) primary = owner->GetGUID();
            defeating = true; defeatTimer = 5500; events.Reset();
            if (!activeLease) { wasActive = me->isActiveObject(); activeLease = true; }
            me->setActive(true);
            damage = health > threshold ? uint32(std::min<uint64>(damage, health - threshold)) : 0;
            DoCast(186382);
        }

        void JustDied(Unit* killer) override
        {
            if (deathHandled || !MalgalorFinale::Progress(me)) return;
            deathHandled = true;
            if (!defeating)
            {
                TC_LOG_WARN("scripts", "Malgalor finale: direct death bypassed scripted defeat; checking actual native death receipt");
                members = MalgalorFinale::Capture(me);
                if (Player* owner = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr) primary = owner->GetGUID();
            }
            MalgalorFinale::Death(me, members, primary);
            if (activeLease) me->setActive(wasActive);
            activeLease = false;
        }

        void UpdateAI(uint32 diff) override
        {
            uint32 introDiff = diff;
            while (introStage && introTimer <= introDiff)
            {
                introDiff -= introTimer;
                if (introStage == 1)
                {
                    me->GetMotionMaster()->MovePoint(0, -2092.75f, -1180.69f, 31.00f);
                    introStage = 2; introTimer = 6000;
                }
                else
                {
                    introStage = 0;
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                    if (Creature* ally = me->FindNearestCreature(97166, 200.0f, true))
                    { AttackStart(ally); if (ally->IsAIEnabled) ally->AI()->AttackStart(me); }
                }
            }
            if (introStage) introTimer -= introDiff;
            if (defeating)
            {
                if (defeatTimer > diff) { defeatTimer -= diff; return; }
                if (std::none_of(members.begin(), members.end(), [](MalgalorFinale::Member const& member) { return MalgalorFinale::Original(member) != nullptr; }))
                { defeating = false; EnterEvadeMode(); return; }
                me->Kill(me); // real death, then JustDied owns exactly-once credit
                return;
            }
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                    {
                        if (convers == 791)
                            break;
                        Conversation* conversation = new Conversation;
                        if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), convers, me, NULL, *me))
                            delete conversation;
                        ++convers;
                        events.RescheduleEvent(EVENT_1, 10000);
                    }
                    break;
                    case EVENT_2:
                        DoCast(207942);
                        events.RescheduleEvent(EVENT_2, 11000); // 207942
                        break;
                    case EVENT_3:
                        DoCast(197816);
                        events.RescheduleEvent(EVENT_3, 17000); // 197816
                        break;
                    case EVENT_4:
                        DoCast(197818);
                        events.RescheduleEvent(EVENT_4, 21000); // 197818
                        break;

                }
            }
            DoMeleeAttackIfReady();
        }

    };
};


void AddSC_wars_intro_scenario()
{
    new npc_ragnvald_dragonborn();
    new npc_danica_the_reclaimer();
    new npc_bezzered();
    new npc_malgalor();
    RegisterAuraScript(spell_warrior_malgalor_arrival);
};
