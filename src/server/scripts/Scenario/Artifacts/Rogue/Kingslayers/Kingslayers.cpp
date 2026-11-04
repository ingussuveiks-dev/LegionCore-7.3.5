/*
    Rogue: Kingslayers
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "Kingslayers.h"
#include "GameObjectAI.h"
#include "ObjectAccessor.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "Item.h"
#include "TemporarySummon.h"
#include "MapManager.h"
#include "ScriptsData.h"
#include "KingslayersGuardProgress.h"

// 107979 108015
class npc_garona_halforcen : public CreatureScript
{
public:
    npc_garona_halforcen() : CreatureScript("npc_garona_halforcen") { }

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        if (quest->GetQuestId() == 42504 || quest->GetQuestId() == 42627)
        { //49470 30966
            Conversation* conversation = new Conversation;
            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2381, player, NULL, *player))
                delete conversation;
            player->SetScenarioId(1123);
            player->TeleportTo(1620, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation());
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_garona_halforcenAI(creature);
    }

    struct npc_garona_halforcenAI : ScriptedAI
    {
        npc_garona_halforcenAI(Creature* creature) : ScriptedAI(creature)
        {
            introDone = false;
        }

        bool introDone;
        ObjectGuid recipientGuid;
        EventMap events;

        Player* GetRecipient()
        {
            if (!me->IsInWorld())
                return nullptr;
            Player* player = ObjectAccessor::GetPlayer(*me, recipientGuid);
            return player && player->GetGUID() == recipientGuid && player->IsInWorld() && player->CanContact() &&
                player->GetMap() == me->GetMap() && player->InSamePhase(me) ? player : nullptr;
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (me->GetEntry() != 108015)
                return;

            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            if (me->GetExactDist2d(who) >= 20.0f)
                return;

            if (introDone)
                return;

            introDone = true;
            recipientGuid = who->GetGUID();
            Conversation* conversation = new Conversation;
            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2361, who, NULL, *who))
                delete conversation;

            me->GetMotionMaster()->MovePoint(0, -9091.40f, 414.15f, 92.11f);
            me->RemoveAura(132653);

            events.ScheduleEvent(EVENT_1, 15000);
            events.ScheduleEvent(EVENT_2, 30000);
        }

        void UpdateAI(uint32 diff) override
        {
            ScriptedAI::UpdateAI(diff);
            events.Update(diff);
            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                    {
                        std::list<Creature*> adds;
                        GetCreatureListWithEntryInGrid(adds, me, 98094, 70.0f);
                        GetCreatureListWithEntryInGrid(adds, me, 108058, 70.0f);
                        if (!adds.empty())
                            for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                            {
                                (*itr)->GetMotionMaster()->MovePoint(0, -8996.06f, 487.59f, 96.62f);
                                (*itr)->SetWalk(true);
                                (*itr)->DespawnOrUnsummon(6000);
                            }

                        if (Player* player = GetRecipient())
                        {
                            Conversation* conversation = new Conversation;
                            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2377, player, NULL, *player))
                                delete conversation;
                        }
                        break;
                    }
                    case EVENT_2:
                    {
                        me->GetMotionMaster()->MovePoint(0, -9074.46f, 427.19f, 92.07f);
                        me->SetWalk(true);
                        me->DespawnOrUnsummon(4000);
                        if (Player* player = GetRecipient())
                        {
                            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 51418);
                            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 52672);
                            Conversation* conversation = new Conversation;
                            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2384, player, NULL, *player))
                                delete conversation;
                        }
                        break;
                    }
                }
            }
        }

    };
};


// 108058 108059 108057 108056 108054 108107 108149 109966 108435 108399 108376 108177
class npc_kingslayers_small_events : public CreatureScript
{
public:
    npc_kingslayers_small_events() : CreatureScript("npc_kingslayers_small_events") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_kingslayers_small_eventsAI(creature);
    }

    struct npc_kingslayers_small_eventsAI : ScriptedAI
    {
        npc_kingslayers_small_eventsAI(Creature* creature) : ScriptedAI(creature)
        {
            introDone = false;
            me->SetReactState(REACT_AGGRESSIVE);
            check = false;
            timer = 1000;
        }

        bool introDone;
        uint32 timer;
        bool check;

        void MoveInLineOfSight(Unit* who) override
        {
            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            {
                if (me->getVictim())
                    return;

                if (me->canStartAttack(who, false) && me->GetExactDist2d(who) <= 5.0f)
                    AttackStart(who);
            }

            if (!me->isMoving() && me->GetExactDist2d(who) <= 15.0f)
            {
                if (me->GetExactDist2d(who) <= 3.0f)
                    me->SetFacingToObject(who);
                else
                    me->SetOrientation(me->GetHomePosition().GetOrientation());
            }
            if (introDone)
                return;
            if (me->GetEntry() == 108399 && me->GetExactDist2d(who) <= 130.0f)
            {
                introDone = true;
                Conversation* conversation = new Conversation;
                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2483, who, NULL, *who))
                    delete conversation;

                me->AddDelayedEvent(5000, [this]() -> void
                {
                    Conversation* conversation = new Conversation;
                    if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2476, me, NULL, *me))
                        delete conversation;
                    me->GetMotionMaster()->MovePoint(0, -8527.94f, 442.00f, 106.01f);

                });
                return;
            }

            if (me->GetExactDist2d(who) >= 14.0f)
                return;

            if (me->GetEntry() == 108054)
            {
                if (who->GetPositionZ() < 100.0f)
                    return;
                Conversation* conversation = new Conversation;
                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2395, who, NULL, *who))
                    delete conversation;
            }

            introDone = true;
            if (me->GetEntry() == 108057)
            {
                Conversation* conversation = new Conversation;
                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2384, who, NULL, *who))
                    delete conversation;
            }
            else if (me->GetEntry() == 108056)
            {
                Conversation* conversation = new Conversation;
                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2394, who, NULL, *who))
                    delete conversation;
            }
            else if (me->GetEntry() == 108107)
            {
                Conversation* conversation = new Conversation;
                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), (urand(1, 2) == 1 ? 2392 : 2408), who, NULL, *who))
                    delete conversation;
            }
            else if (me->GetEntry() == 108149)
            {
                Conversation* conversation = new Conversation;
                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2407, who, NULL, *who))
                    delete conversation;
            }
            else
            {
                if (urand(1, 2) == 2)
                    Talk(0);
            }
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void UpdateAI(uint32 diff) override
        {
            ScriptedAI::UpdateAI(diff);

            if (me->GetEntry() == 108177 && !check)
            {
                if (timer <= diff)
                {
                    if (Player* pl = me->FindNearestPlayer(15.0f))
                    {
                        check = true;
                        pl->AddAura(227061, pl);
                        pl->CastSpell(pl, 227061);
                    }
                    timer = 1000;
                }
                else
                    timer -= diff;
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

    };
};


// 214645
class spell_kingslayers_smoke_bomb : public SpellScriptLoader
{
public:
    spell_kingslayers_smoke_bomb() : SpellScriptLoader("spell_kingslayers_smoke_bomb") { }

    class spell_kingslayers_smoke_bomb_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_kingslayers_smoke_bomb_SpellScript);

        void HandleAfterCast()
        {
            if (Unit* caster = GetCaster())
            {
                if (caster->GetTypeId() != TYPEID_PLAYER)
                    return;
                if (caster->GetMapId() != 1620 || caster->GetAreaId() != 8228)
                    return;
                if (caster->GetExactDist2d(-8822.94f, 657.59f) >= 13.0f)
                    return;

                caster->CastSpell(caster, 214683);
                caster->CastSpell(caster, 214734);
                caster->ToPlayer()->PlayDirectSound(15003, NULL);

                if (Creature* targ = caster->FindNearestCreature(108057, 70.0f, true))
                {
                    targ->GetMotionMaster()->Clear();
                    targ->GetMotionMaster()->MovePoint(0, -8844.79f, 647.28f, 96.65f);
                }

                Conversation* conversation = new Conversation;
                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2412, caster, NULL, *caster))
                    delete conversation;


                std::list<Creature*> adds;
                GetCreatureListWithEntryInGrid(adds, caster, 108159, 30.0f);
                GetCreatureListWithEntryInGrid(adds, caster, 108155, 30.0f);
                GetCreatureListWithEntryInGrid(adds, caster, 108178, 30.0f);
                GetCreatureListWithEntryInGrid(adds, caster, 108158, 30.0f);
                GetCreatureListWithEntryInGrid(adds, caster, 108177, 30.0f);
                GetCreatureListWithEntryInGrid(adds, caster, 108161, 30.0f);
                GetCreatureListWithEntryInGrid(adds, caster, 108179, 30.0f);
                GetCreatureListWithEntryInGrid(adds, caster, 108157, 30.0f);
                GetCreatureListWithEntryInGrid(adds, caster, 108160, 30.0f);
                GetCreatureListWithEntryInGrid(adds, caster, 108107, 30.0f);
                if (!adds.empty())
                    for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                    {
                        (*itr)->SetWalk(false);
                        (*itr)->CastSpell((*itr), 214304);
                        (*itr)->GetMotionMaster()->MoveRandom(3.0f);
                    }
            }
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_kingslayers_smoke_bomb_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_kingslayers_smoke_bomb_SpellScript();
    }
};

// 251053
class go_kingslayers_door : public GameObjectScript
{
public:
    go_kingslayers_door() : GameObjectScript("go_kingslayers_door") { }

    struct go_kingslayers_doorAI : public GameObjectAI
    {
        go_kingslayers_doorAI(GameObject* go) : GameObjectAI(go) {}

        bool opening = false;

        bool Source() const
        {
            return go && go->GetEntry() == 251053 && go->GetDBTableGUIDLow() == 133953 &&
                go->GetMapId() == 1620 && go->GetGoType() == GAMEOBJECT_TYPE_DOOR;
        }

        bool Leaf(Scenario* scenario, uint32 id) const
        {
            auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
            auto found = progress->find(id);
            return found != progress->end() && found->second.Counter;
        }

        bool Activated() const
        {
            return go->getLootState() == GO_ACTIVATED && go->GetGoState() == GO_STATE_ACTIVE &&
                go->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE);
        }

        Scenario* Context(Player* player, bool activated) const
        {
            // Match native door interaction distance. A closed door's own
            // model can occlude a ray to its center while remaining usable.
            if (!Source() || !go->IsInWorld() || !go->isSpawned() || go->IsQuestObjectExpired() ||
                go->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE | GO_FLAG_LOCKED) ||
                (activated ? !Activated() : go->getLootState() != GO_READY || go->GetGoState() != GO_STATE_READY ||
                    go->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE)) ||
                !player || !player->IsInWorld() || player->IsPreDelete() || !player->CanContact() || !player->IsAlive() ||
                player->isGameMaster() || player->getClass() != CLASS_ROGUE || player->HasPendingTeleport() ||
                player->GetMap() != go->GetMap() || !player->InSamePhase(go) ||
                !player->canSeeOrDetect(go) ||
                !player->IsWithinDistInMap(go, go->GetInteractionDistance()) || player->IsQuestObjectExpired() ||
                !player->CanTargetQuestObject(go) || !go->CanTargetQuestObject(player))
                return nullptr;

            bool quest = false;
            for (uint32 id : {42504u, 42627u})
                if (!player->GetQuestRewardStatus(id) &&
                    (player->GetQuestStatus(id) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(id) == QUEST_STATUS_COMPLETE))
                    quest = true;
            Scenario* scenario = quest ? sScenarioMgr->GetScenario(go->GetInstanceId()) : nullptr;
            return scenario && scenario->GetScenarioId() == 1123 &&
                (scenario->CanUpdateCriteria(48577) || scenario->CanUpdateCriteria(48597) || Leaf(scenario, 48597)) ? scenario : nullptr;
        }

        void Open(Player* player)
        {
            if (opening || !Source() || !Context(player, Activated()))
                return;
            opening = true;
            bool const transition = !Activated();
            if (transition)
                go->UseDoorOrButton(0, false, player);

            // Native activation (including its state callbacks) must finish
            // before either event. The two authored leaves belong to different
            // steps, so recheck the current scenario between their updates.
            if (Scenario* scenario = Context(player, true))
            {
                if (scenario->CanUpdateCriteria(48577) && !Leaf(scenario, 48577))
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 51549);
            }
            if (Scenario* scenario = Context(player, true))
            {
                if (scenario->CanUpdateCriteria(48597) && !Leaf(scenario, 48597))
                {
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 51654);
                    scenario = Context(player, true);
                    if (scenario && Leaf(scenario, 48597))
                    {
                        if (Creature* garona = player->FindNearestCreature(108214, 60.0f, true))
                            if (garona->CanTargetQuestObject(player) && player->CanTargetQuestObject(garona))
                            {
                                garona->GetMotionMaster()->MovePoint(0, -8625.50f, 415.93f, 103.7f);
                                garona->RemoveAura(132653);
                            }
                    }
                }
            }

            // Preserve the original native door script dispatch once per real
            // activation. An explicit retry on an already open door can recover
            // missing progress, without toggling it or replaying its scripts.
            if (transition && Source() && go->IsInWorld() && Activated())
                go->GetMap()->ScriptsStart(sGameObjectScripts, go->GetDBTableGUIDLow(), player, go);
            opening = false;
        }

        bool GossipUse(Player* player) override
        {
            if (!Source())
                return false;
            Open(player);
            return true;
        }

        bool GossipHello(Player* player, bool isUse) override
        {
            if (!Source())
                return false;
            if (isUse)
                Open(player); // native completed Opening, never report-use
            return true;
        }

    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return new go_kingslayers_doorAI(go);
    }
};

// Conversation 2431 is requested by JustDied, when the original Althea is
// already dead. Keep its real creature actor instead of searching for a live
// duplicate. The Player/owner remains the caster and personal/group recipient.
class AltheaDeathConversation final : public Conversation
{
public:
    explicit AltheaDeathConversation(ObjectGuid actor) : _actor(actor) { }

    void AddToWorld() override
    {
        // The conversation now has its own GUID. Register that GUID in the
        // source attempt before publishing it, so cancellation removes it too.
        Creature* actor = ObjectAccessor::GetCreature(*this, _actor);
        if (actor && InheritQuestObjectScope(actor))
            Conversation::AddToWorld();
    }

protected:
    Creature* FindCreatureActor(uint32 index, uint32 entry, WorldObject* searcher) override
    {
        if (index != 0 || entry != 108218 || !searcher || searcher->GetMapId() != 1620)
            return nullptr;
        Creature* actor = ObjectAccessor::GetCreature(*searcher, _actor);
        if (!actor || actor->GetEntry() != entry || !actor->IsInWorld() || actor->IsPreDelete() ||
            actor->IsQuestObjectExpired() || searcher->IsQuestObjectExpired() ||
            !searcher->IsWithinDistInMap(actor, searcher->GetVisibilityRange()) ||
            !searcher->InSamePhase(actor) || !searcher->InSamePhaseId(actor) ||
            !actor->CanTargetQuestObject(searcher) || !searcher->CanTargetQuestObject(actor))
            return nullptr;
        return actor;
    }

private:
    ObjectGuid const _actor;
};

// 108218
class npc_sister_althea_ebonlocke : public CreatureScript
{
public:
    npc_sister_althea_ebonlocke() : CreatureScript("npc_sister_althea_ebonlocke") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_sister_althea_ebonlockeAI(creature);
    }

    struct npc_sister_althea_ebonlockeAI : ScriptedAI
    {
        npc_sister_althea_ebonlockeAI(Creature* creature) : ScriptedAI(creature)
        {
            introDone = false;
            introDone2 = false;
            checkdies = false;
            SetCanSeeEvenInPassiveMode(true);
        }

        bool introDone, introDone2, checkdies;
        uint32 timerforcheck;
        EventMap events;
        bool introBusy = false, introReleased = false;
        bool introVoices[3] = {}, introWarnings[3] = {};
        uint32 introRetries[3] = {};
        uint32 introCreditRetry = 0;
        bool introCreditWarning = false;

        bool IntroLeaf(Scenario* scenario, uint32 id) const
        {
            auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
            auto found = progress->find(id);
            return found != progress->end() && found->second.Counter;
        }

        Player* IntroPlayer(Unit* who, float range) const
        {
            Player* player = who ? who->ToPlayer() : nullptr;
            if (!me->IsInWorld() || me->IsPreDelete() || !me->IsAlive() || me->GetEntry() != 108218 ||
                me->GetMapId() != 1620 || me->IsQuestObjectExpired() || !player || !player->IsInWorld() ||
                player->IsPreDelete() || !player->IsAlive() || !player->CanContact() || player->isGameMaster() ||
                player->getClass() != CLASS_ROGUE || player->HasPendingTeleport() || player->GetMap() != me->GetMap() ||
                !player->InSamePhase(me) || me->GetExactDist2d(player) > range || player->IsQuestObjectExpired() ||
                !me->CanTargetQuestObject(player) || !player->CanTargetQuestObject(me))
                return nullptr;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1123 ||
                (!scenario->CanUpdateCriteria(48577) && !scenario->CanUpdateCriteria(48585)) || IntroLeaf(scenario, 48585))
                return nullptr;
            for (uint32 quest : {42504u, 42627u})
                if (!player->GetQuestRewardStatus(quest) &&
                    (player->GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(quest) == QUEST_STATUS_COMPLETE))
                    return player;
            return nullptr;
        }

        Player* IntroOpponent() const
        {
            std::list<Player*> players;
            me->GetPlayerListInGrid(players, 50.0f);
            for (Player* player : players)
                if (IntroPlayer(player, 50.0f))
                    return player;
            return nullptr;
        }

        void IntroVoice(Player* player, uint32 slot)
        {
            if (introVoices[slot] || introRetries[slot])
                return;
            introRetries[slot] = 1000;
            uint32 const entries[3] = {2422, 2424, 2425};
            Conversation* conversation = new Conversation;
            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), entries[slot], player, NULL, *player))
            {
                delete conversation;
                if (!introWarnings[slot])
                    TC_LOG_WARN("scripts", "Althea intro: conversation %u unavailable; retain bounded-rate presentation retry independently of progression", entries[slot]);
                introWarnings[slot] = true;
                return;
            }
            introVoices[slot] = true;
        }

        void UpdateIntro(uint32 diff)
        {
            if (introBusy)
                return;
            for (uint32& retry : introRetries)
                retry = retry > diff ? retry - diff : 0;
            introCreditRetry = introCreditRetry > diff ? introCreditRetry - diff : 0;
            introBusy = true;
            struct Release { bool& busy; ~Release() { busy = false; } } release{introBusy};
            if (checkdies)
            {
                if (timerforcheck > diff)
                    timerforcheck -= diff;
                else
                {
                    timerforcheck = 1000;
                    InstanceScript* data = me->GetInstanceScript();
                    if (data && data->GetData(KingslayersGuards::AllDefeated))
                        if (Player* player = IntroOpponent())
                        {
                            checkdies = false;
                            introReleased = true;
                            me->GetMotionMaster()->MoveJump(-8614.58f, 403.17f, 102.92f, me->GetOrientation(), 5.0f, 5.0f);
                            me->RemoveAura(214873);
                            me->AI()->AttackStart(player);
                        }
                }
            }
            if (introReleased && !introVoices[2] && !introRetries[2])
                if (Player* player = IntroOpponent())
                    IntroVoice(player, 2);
        }

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 3000); // convers
            events.RescheduleEvent(EVENT_5, 27000); // 202839
            events.RescheduleEvent(EVENT_6, 15000); // 203109
            events.RescheduleEvent(EVENT_7, 25000); // 207673
            me->SetHomePosition(me->GetPosition());
            if (Creature* garona = me->FindNearestCreature(108214, 60.0f, true))
                garona->AI()->AttackStart(me);
        }

        Player* GetVictoryPlayer(Player* killer)
        {
            if (!me->IsInWorld() || me->IsPreDelete() || me->GetMapId() != 1620 || me->IsQuestObjectExpired())
                return nullptr;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1123 || !scenario->CanUpdateCriteria(48585))
                return nullptr;
            auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
            auto victory = progress->find(48585);
            if (victory != progress->end() && victory->second.Counter)
                return nullptr;

            auto eligible = [this](Player* player)
            {
                if (!player || !player->IsInWorld() || player->IsPreDelete() || !player->CanContact() ||
                    player->isGameMaster() || player->getClass() != CLASS_ROGUE || player->HasPendingTeleport() ||
                    player->GetMap() != me->GetMap() || !player->InSamePhase(me) ||
                    !player->IsWithinDistInMap(me, 100.0f) || player->IsQuestObjectExpired() ||
                    !me->CanTargetQuestObject(player) || !player->CanTargetQuestObject(me))
                    return false;
                for (uint32 quest : {42504u, 42627u})
                    if (!player->GetQuestRewardStatus(quest) &&
                        (player->GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(quest) == QUEST_STATUS_COMPLETE))
                        return true;
                return false;
            };

            // One native event updates this instance's shared scenario. Prefer
            // the actual killer/owner; Garona can also land the killing blow.
            if (eligible(killer))
                return killer;
            std::list<Player*> players;
            me->GetPlayerListInGrid(players, 100.0f);
            for (Player* player : players)
                if (eligible(player))
                    return player;
            return nullptr;
        }

        void JustDied(Unit* who) override
        {
            Unit* speaker = who;
            if (who)
                if (Player* owner = who->GetCharmerOrOwnerPlayerOrPlayerItself())
                    speaker = owner;
            if (Player* pl = GetVictoryPlayer(speaker ? speaker->ToPlayer() : nullptr))
                pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 51613);
            if (speaker)
            {
                Conversation* conversation = new AltheaDeathConversation(me->GetGUID());
                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2431, speaker, NULL, *speaker))
                    delete conversation;
            }

            if (Creature* garona = me->FindNearestCreature(108214, 60.0f, true))
            {
                garona->AddDelayedEvent(5000, [garona]() -> void
                {
                    Conversation* conversation = new Conversation;
                    if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2432, garona, NULL, *garona))
                        delete conversation;
                    garona->DespawnOrUnsummon(5000);

                });
            }
        }

        void MoveInLineOfSight(Unit* who) override
        {
            Player* player = IntroPlayer(who, 55.0f);
            if (!player || introBusy)
                return;
            introBusy = true;
            struct Release { bool& busy; ~Release() { busy = false; } } release{introBusy};
            if (!introDone)
            {
                Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
                if (scenario->CanUpdateCriteria(48577) && !IntroLeaf(scenario, 48577))
                {
                    if (introCreditRetry)
                        return;
                    introCreditRetry = 1000;
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 51549);
                }
                if (!IntroPlayer(player, 55.0f))
                    return;
                scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
                if (!IntroLeaf(scenario, 48577))
                {
                    if (!introCreditWarning)
                        TC_LOG_WARN("scripts", "Althea intro: native51549 not acknowledged; retain bounded-rate progression retry");
                    introCreditWarning = true;
                    return;
                }
                introDone = true; // Only actual native51549 acknowledgement consumes progression.
            }
            IntroVoice(player, 0); // Optional presentation cannot block native progression.
            if (!IntroPlayer(player, 35.0f))
                return;
            if (!introDone2)
            {
                Creature* garona = me->FindNearestCreature(108214, 60.0f, true);
                if (!garona || !garona->AI() || !garona->CanTargetQuestObject(player) || !player->CanTargetQuestObject(garona))
                    return; // A missing mandatory companion must not consume the guard handoff.
                InstanceScript* data = me->GetInstanceScript();
                if (!data)
                    return;
                std::vector<std::pair<uint32, ObjectGuid>> adds;
                for (uint32 i = 0; i < 3; ++i)
                {
                    uint32 spawn = KingslayersGuards::Spawns[i];
                    ObjectGuid guid = data->GetGuidData(spawn);
                    Creature* guard = ObjectAccessor::GetCreature(*me, guid);
                    if (guard && guard->IsInWorld() && guard->IsAlive())
                    {
                        if (guard->GetEntry() != KingslayersGuards::Entries[i] || guard->GetDBTableGUIDLow() != spawn ||
                            !guard->AI() || !guard->InSamePhase(player) ||
                            !guard->CanTargetQuestObject(player) || !player->CanTargetQuestObject(guard))
                            return;
                        adds.emplace_back(spawn, guid);
                    }
                    else if (!data->GetData(spawn))
                        return;
                }
                introDone2 = true;
                timerforcheck = 1000;
                checkdies = true;
                ObjectGuid companion = garona->GetGUID();
                for (auto const& actor : adds)
                {
                    garona = ObjectAccessor::GetCreature(*me, companion);
                    if (!IntroPlayer(player, 35.0f) || !garona || !garona->IsInWorld() || !garona->IsAlive() ||
                        !garona->CanTargetQuestObject(player) || !player->CanTargetQuestObject(garona))
                    {
                        introDone2 = checkdies = false;
                        return;
                    }
                    Creature* guard = ObjectAccessor::GetCreature(*me, actor.second);
                    if (!guard || !guard->IsInWorld() || !guard->IsAlive() || !guard->AI())
                    {
                        if (data->GetData(actor.first))
                            continue; // A real synchronous death already satisfies this guard.
                        introDone2 = checkdies = false;
                        return; // Retry a retired/missing actor instead of calling its old AI.
                    }
                    guard->AI()->AttackStart(garona);
                }
            }
            if (IntroPlayer(player, 35.0f))
                IntroVoice(player, 1);
        }

        void UpdateAI(uint32 diff) override
        {
            UpdateIntro(diff);

            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING) || me->HasUnitState(UNIT_STATE_STUNNED))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                    {
                        Conversation* conversation = new Conversation;
                        if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2426, me, NULL, *me))
                            delete conversation;
                        events.RescheduleEvent(EVENT_2, 6000);
                    }
                    break;
                    case EVENT_2:
                    {
                        Conversation* conversation = new Conversation;
                        if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2428, me, NULL, *me))
                            delete conversation;
                        events.RescheduleEvent(EVENT_3, 3000);
                    }
                    break;
                    case EVENT_3:
                    {
                        Conversation* conversation = new Conversation;
                        if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2429, me, NULL, *me))
                            delete conversation;
                        events.RescheduleEvent(EVENT_4, 5000);
                    }
                    break;
                    case EVENT_4:
                    {
                        Conversation* conversation = new Conversation;
                        if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2430, me, NULL, *me))
                            delete conversation;
                    }
                    break;
                    case EVENT_5:
                        DoCast(202839);
                        events.RescheduleEvent(EVENT_5, 27000); // 202839
                        break;
                    case EVENT_6:
                        DoCast(203109);
                        events.RescheduleEvent(EVENT_6, 15000); // 203109
                        break;
                    case EVENT_7:
                        DoCast(207673);
                        events.RescheduleEvent(EVENT_7, 25000); // 207673
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};

// 107831
class npc_merlis_malagan : public CreatureScript
{
public:
    npc_merlis_malagan() : CreatureScript("npc_merlis_malagan") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_merlis_malaganAI(creature);
    }

    struct npc_merlis_malaganAI : ScriptedAI
    {
        npc_merlis_malaganAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            introDone = false;
            timer = 1000;
            SetCanSeeEvenInPassiveMode(true);
        }

        bool introDone;
        uint32 timer;
        uint32 convid;
        EventMap events;
        SummonList summons;

        ObjectGuid introRecipient, introCompanions[2];
        uint32 introDelay = 0, introRetry = 0, introSceneRetry = 0;
        bool introBusy = false, introArriving = false, introVoice = false, introScene = false;
        uint8 introWarnings = 0;

        bool IntroReceipt() const
        {
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1123)
                return false;
            auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
            auto found = progress->find(48587);
            return found != progress->end() && found->second.Counter;
        }

        Player* IntroPlayer(Player* player, bool pending = false, bool admission = true) const
        {
            if (!me->IsInWorld() || me->IsPreDelete() || !me->IsAlive() || me->GetEntry() != 107831 ||
                me->GetMapId() != 1620 || me->IsQuestObjectExpired() || !player || !player->IsInWorld() ||
                player->IsPreDelete() || !player->IsAlive() || !player->CanContact() || player->isGameMaster() ||
                player->getClass() != CLASS_ROGUE || (!pending && player->HasPendingTeleport()) ||
                player->GetMap() != me->GetMap() || !player->InSamePhase(me) || me->GetExactDist(player) > 70.0f ||
                player->IsQuestObjectExpired() || !me->CanTargetQuestObject(player) || !player->CanTargetQuestObject(me))
                return nullptr;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1123 ||
                (admission && (!scenario->CanUpdateCriteria(48587) || IntroReceipt())))
                return nullptr;
            for (uint32 quest : {42504u, 42627u})
                if (!player->GetQuestRewardStatus(quest) &&
                    (player->GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(quest) == QUEST_STATUS_COMPLETE))
                    return player;
            return nullptr;
        }

        bool IntroDestination(Player* player) const
        {
            auto const& destination = player->GetTeleportDest();
            return destination.GetMapId() == 1620 && destination.GetExactDist(-8342.62f, 271.15f, 155.34f) < 0.1f;
        }

        void IntroCancel()
        {
            introRecipient.Clear();
            for (auto& guid : introCompanions)
                guid.Clear();
            introArriving = introVoice = introScene = false;
            introDelay = introRetry = introSceneRetry = 0;
            timer = 1000;
        }

        void IntroPresentation(Player* player)
        {
            if (introRetry)
                return;
            introRetry = 1000;
            if (!introVoice)
            {
                Conversation* conversation = new Conversation;
                if (conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2484, player, NULL, *player))
                    introVoice = true;
                else
                {
                    delete conversation;
                    if (!(introWarnings & 1))
                        TC_LOG_WARN("scripts", "Merlis intro: conversation2484 unavailable; retry presentation independently of travel");
                    introWarnings |= 1;
                }
            }
        }

        void IntroScene(Player* player)
        {
            if (introScene || introSceneRetry)
                return;
            introSceneRetry = 1000;
            introScene = player->HasAura(214942) || player->CastSpell(player, 214942) == SPELL_CAST_OK;
            if (!introScene && !(introWarnings & 8))
            {
                TC_LOG_WARN("scripts", "Merlis intro: optional214942 scene cast refused; travel remains independent");
                introWarnings |= 8;
            }
        }

        void IntroCommit(Player* player)
        {
            if (!IntroDestination(player) || (!player->HasPendingTeleport() &&
                player->GetExactDist(-8342.62f, 271.15f, 155.34f) > 2.0f)) { IntroCancel(); return; }
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 51614);
            if (!IntroReceipt())
            {
                if (!(introWarnings & 4))
                    TC_LOG_WARN("scripts", "Merlis intro: native51614 not acknowledged after queued travel; retain bounded-rate retry");
                introWarnings |= 4;
                return;
            }
            introDone = true;
            player = IntroPlayer(ObjectAccessor::GetPlayer(*me, introRecipient), true, false);
            uint32 const companionEntries[2] = {108333, 64367};
            if (player && IntroDestination(player))
                for (uint32 i = 0; i < 2; ++i)
                    if (Creature* actor = ObjectAccessor::GetCreature(*me, introCompanions[i]))
                        if (actor->GetEntry() == companionEntries[i] && actor->IsInWorld() && actor->IsAlive() &&
                            !actor->IsPreDelete() && !actor->IsQuestObjectExpired() && actor->InSamePhase(player) &&
                            actor->CanTargetQuestObject(player) && player->CanTargetQuestObject(actor))
                            actor->DespawnOrUnsummon();
            IntroCancel();
        }

        void UpdateIntro(uint32 diff)
        {
            if (introBusy || introDone)
                return;
            introBusy = true;
            struct Release { bool& busy; ~Release() { busy = false; } } release{introBusy};
            introRetry = introRetry > diff ? introRetry - diff : 0;
            introSceneRetry = introSceneRetry > diff ? introSceneRetry - diff : 0;
            if (IntroReceipt())
            {
                introDone = true;
                IntroCancel();
                return; // Another legitimate native delivery already completed this shared step.
            }
            if (!introRecipient)
            {
                if (timer > diff) { timer -= diff; return; }
                timer = 1000;
                std::list<Player*> players;
                me->GetPlayerListInGrid(players, 70.0f);
                for (Player* candidate : players)
                    if (IntroPlayer(candidate))
                    {
                        introRecipient = candidate->GetGUID();
                        introDelay = 5000;
                        IntroPresentation(candidate);
                        break;
                    }
                return;
            }
            Player* player = IntroPlayer(ObjectAccessor::GetPlayer(*me, introRecipient), introArriving);
            if (!player) { IntroCancel(); return; }
            IntroPresentation(player);
            if (introDelay > diff) { introDelay -= diff; return; }
            introDelay = 1000;
            uint32 const companionEntries[2] = {108333, 64367};
            if (introArriving)
            {
                if (!IntroDestination(player)) { IntroCancel(); return; }
                IntroScene(player);
                player = IntroPlayer(ObjectAccessor::GetPlayer(*me, introRecipient), true);
                if (!player) { IntroCancel(); return; }
                IntroCommit(player); // Retry a refused native event, never reissue the accepted travel.
                return;
            }
            IntroScene(player);
            // Cast/factory callbacks may change admission. Re-resolve the full GUID.
            player = IntroPlayer(ObjectAccessor::GetPlayer(*me, introRecipient));
            if (!player) { IntroCancel(); return; }
            for (uint32 i = 0; i < 2; ++i)
            {
                introCompanions[i].Clear();
                if (Creature* actor = player->FindNearestCreature(companionEntries[i], 50.0f, true))
                    if (actor->InSamePhase(player) && actor->CanTargetQuestObject(player) && player->CanTargetQuestObject(actor))
                        introCompanions[i] = actor->GetGUID();
            }
            bool accepted = player->TeleportTo(1620, -8342.62f, 271.15f, 155.34f, 0.0f);
            player = IntroPlayer(ObjectAccessor::GetPlayer(*me, introRecipient), true);
            if (!player) { IntroCancel(); return; }
            if (accepted && player->HasPendingTeleport() && IntroDestination(player))
            {
                introArriving = true;
                IntroCommit(player); // Keep stage7 available to a pet/guardian killing blow before client ACK.
            }
            else
            {
                if (!(introWarnings & 2))
                    TC_LOG_WARN("scripts", "Merlis intro: native travel not queued; retain bounded-rate retry without progress or cleanup");
                introWarnings |= 2;
            }
        }

        void Reset() override
        {
            events.Reset();
            summons.DespawnAll();
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 5000); // 15 218359 + 218356 + summon
            events.RescheduleEvent(EVENT_3, 25000);
            convid = 2442;
        }

        void JustSummoned(Creature* summon) override
        {
            summons.Summon(summon);
            DoZoneInCombat(summon, 150.0f);
        }

        void JustDied(Unit* who) override
        {
            Player* player = who ? who->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            summons.DespawnAll();
            if (Creature* oldgarona = me->FindNearestCreature(108222, 60.0f, true))
                oldgarona->DespawnOrUnsummon();
            if (Creature* garona = me->SummonCreature(108222, -8310.60f, 284.40f, 156.83f))
            {
                garona->AI()->DoAction(true);
                garona->SetVisible(true);
            }
            if (player)
            {
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 51615);
                Conversation* conversation = new Conversation;
                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2448, player, NULL, *player))
                    delete conversation;
            }

            std::list<Creature*> adds;
            GetCreatureListWithEntryInGrid(adds, me, 108397, 70.0f);
            if (!adds.empty())
                for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                {
                    (*itr)->DespawnOrUnsummon();
                }
        }

        void UpdateAI(uint32 diff) override
        {
            UpdateIntro(diff);

            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING) || me->HasUnitState(UNIT_STATE_STUNNED))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(218359);
                        events.RescheduleEvent(EVENT_1, 18000);
                        events.RescheduleEvent(EVENT_2, 4000);
                        if (convid != 2448)
                        {
                            Conversation* conversation = new Conversation;
                            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), convid, me, NULL, *me))
                                delete conversation;
                            ++convid;
                        }
                        break;
                    case EVENT_2:
                        DoCast(218356);
                        if (convid != 2448)
                        {
                            Conversation* conversation = new Conversation;
                            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), convid, me, NULL, *me))
                                delete conversation;
                            ++convid;
                        }
                        break;
                    case EVENT_3:
                        for (int i = 0; i < 6; ++i)
                        {
                            if (Creature* add = me->SummonCreature(108324, -8315.46f, 293.38f, 159.24f))
                            {
                                add->GetMotionMaster()->MoveJump(me->GetPositionX() + irand(-5, 5), me->GetPositionY() + irand(-5, 5), me->GetPositionZ(), add->GetOrientation(), 5.0f, 5.0f);
                                add->CastSpell(add, 158342);
                            }
                            if (convid != 2448)
                            {
                                Conversation* conversation = new Conversation;
                                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), convid, me, NULL, *me))
                                    delete conversation;
                                ++convid;
                            }
                        }
                        events.RescheduleEvent(EVENT_3, 25000);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};

// 108222
class npc_garona_halforcen_outro : public CreatureScript
{
public:
    npc_garona_halforcen_outro() : CreatureScript("npc_garona_halforcen_outro") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_garona_halforcen_outroAI(creature);
    }

    struct npc_garona_halforcen_outroAI : ScriptedAI
    {
        npc_garona_halforcen_outroAI(Creature* creature) : ScriptedAI(creature)
        {
            introDone = false;
            SetCanSeeEvenInPassiveMode(true);
            me->SetVisible(true);
        }

        bool introDone;
        uint32 convid;
        EventMap events;
        bool artifactReady = false, visualReady = false, revealMoved = false;
        uint8 revealWarnings = 0;

        void Reset() override
        {
            // This friendly actor owns an earned reveal. An incidental reset
            // must not cancel its remaining delay or accepted factory results.
            if (!introDone)
                events.Reset();
        }

        void DoAction(int32 const action) override
        {
            if (introDone)
                return;
            introDone = true;
            me->RemoveAura(214940);
            me->GetMotionMaster()->MovePoint(0, -8316.88f, 283.96f, 156.83f);
            Conversation* conversation = new Conversation;
            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2449, me, NULL, *me))
                delete conversation;
            events.RescheduleEvent(EVENT_1, 23000);
        }

        void UpdateAI(uint32 diff) override
        {

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING) || me->HasUnitState(UNIT_STATE_STUNNED))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                    {
                        Conversation* conversation = new Conversation;
                        if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2450, me, NULL, *me))
                            delete conversation;
                        events.RescheduleEvent(EVENT_2, 11000);
                    }
                    break;
                    case EVENT_2:
                        if (!visualReady)
                        {
                            visualReady = me->SummonCreature(108325, -8318.08f, 283.807f, 156.916f, 6.26344f) != nullptr;
                            if (!visualReady && !(revealWarnings & 1))
                                TC_LOG_WARN("scripts", "Kingslayers reveal: native108325 creation refused; retain bounded-rate retry");
                            if (!visualReady) revealWarnings |= 1;
                        }
                        if (!artifactReady)
                        {
                            artifactReady = me->SummonGameObject(251107, -8318.08f, 283.807f, 156.833f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, DAY) != nullptr;
                            if (!artifactReady && !(revealWarnings & 2))
                                TC_LOG_WARN("scripts", "Kingslayers reveal: native251107 creation refused; retain bounded-rate retry");
                            if (!artifactReady) revealWarnings |= 2;
                        }
                        if (artifactReady && !revealMoved)
                        {
                            revealMoved = true;
                            me->GetMotionMaster()->MovePoint(0, -8312.67f, 284.65f, 156.83f);
                        }
                        if (!artifactReady || !visualReady)
                            events.RescheduleEvent(EVENT_2, 2000);
                        break;
                }
            }
        }

    };
};

namespace KingslayersAcquisition
{
    uint32 Quest(Player* player)
    {
        if (player)
            for (uint32 id : {42504u, 42627u})
                if (!player->GetQuestRewardStatus(id) &&
                    (player->GetQuestStatus(id) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(id) == QUEST_STATUS_COMPLETE))
                    return id;
        return 0;
    }

    bool Leaf(Scenario* scenario, uint32 id)
    {
        if (!scenario) return false;
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(id);
        return found != progress->end() && found->second.Counter;
    }

    bool CanTake(Player* player, GameObject* object, bool atObject = true)
    {
        if (!player || !object || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() ||
            player->HasPendingTeleport() || player->getClass() != CLASS_ROGUE || !Quest(player) ||
            !object->IsInWorld() || !object->isSpawned() || object->GetEntry() != 251107 || object->GetMapId() != 1620 ||
            player->GetMap() != object->GetMap() || player->GetInstanceId() != object->GetInstanceId() ||
            !player->InSamePhase(object) || (atObject && (!player->canSeeOrDetect(object) || !player->IsWithinLOSInMap(object) ||
            !player->IsWithinDistInMap(object, object->GetInteractionDistance()))) ||
            object->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE)) return false;
        Scenario* scenario = sScenarioMgr->GetScenario(object->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1123 && Leaf(scenario, 48589) &&
            (scenario->GetCurrentStep() == 8 || (scenario->GetCurrentStep() == 9 && Leaf(scenario, 48591) &&
                player->HasItemCount(128870, 1, true)));
    }

    bool Equipped(Player* player)
    {
        Item* parent = player->GetItemByEntry(128870);
        Item* child = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
        return parent && player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND) == parent &&
            child && child->GetEntry() == 128869 && parent->GetChildItem() == child->GetGUID();
    }

    Creature* Outro(Player* player, ObjectGuid guid, uint32 instance, uint32 quest, std::weak_ptr<uint8> const& lifetime)
    {
        QuestStatusData* status = player ? player->getQuestStatus(quest) : nullptr;
        if (!player || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() || player->HasPendingTeleport() ||
            player->getClass() != CLASS_ROGUE || Quest(player) != quest || lifetime.expired() || !status ||
            status->ScriptLifetime != lifetime.lock() || player->GetMapId() != 1620 || player->GetInstanceId() != instance)
            return nullptr;
        Scenario* scenario = sScenarioMgr->GetScenario(instance);
        if (!scenario || scenario->GetScenarioId() != 1123 || scenario->GetCurrentStep() != 9 || !Leaf(scenario, 48591))
            return nullptr;
        Creature* actor = ObjectAccessor::GetCreature(*player, guid);
        return actor && actor->IsInWorld() && actor->IsAlive() && !actor->IsPreDelete() && actor->GetEntry() == 108367 &&
            player->InSamePhase(actor) && actor->ToTempSummon() && actor->ToTempSummon()->GetSummonerGUID() == player->GetGUID() ? actor : nullptr;
    }

    Creature* WorldOutro(ObjectGuid owner, ObjectGuid guid, uint32 instance)
    {
        Map* map = sMapMgr->FindMap(1620, instance);
        Scenario* scenario = map ? sScenarioMgr->GetScenario(instance) : nullptr;
        if (!scenario || scenario->GetScenarioId() != 1123 || scenario->GetCurrentStep() != 9 || !Leaf(scenario, 48591))
            return nullptr;
        Creature* actor = map->GetCreature(guid);
        return actor && actor->IsInWorld() && actor->IsAlive() && !actor->IsPreDelete() && actor->GetEntry() == 108367 &&
            actor->ToTempSummon() && actor->ToTempSummon()->GetSummonerGUID() == owner ? actor : nullptr;
    }

    bool StartOutro(Player* player)
    {
        uint32 instance = player->GetInstanceId(), quest = Quest(player);
        QuestStatusData* status = player->getQuestStatus(quest);
        if (!status || !status->ScriptLifetime) return false;
        std::weak_ptr<uint8> lifetime = status->ScriptLifetime;
        Creature* actor = player->SummonCreature(108367, -8345.60f, 267.80f, 155.34f, 0.69f);
        if (!actor) return false;
        ObjectGuid owner = player->GetGUID(), guid = actor->GetGUID();
        if (!Outro(player, guid, instance, quest, lifetime))
        {
            actor->DespawnOrUnsummon();
            return false;
        }
        // Player/quest-owned callbacks keep only identities. Removal/replacement
        // of the actor, logout or a new quest attempt cannot reuse raw pointers.
        player->AddQuestDelayedEvent(quest, 7000, [owner, guid, instance, quest, lifetime]()
        {
            Player* player = ObjectAccessor::FindPlayer(owner);
            if (Creature* actor = Outro(player, guid, instance, quest, lifetime))
            {
                actor->GetMotionMaster()->MovePoint(0, -8334.08f, 277.72f, 156.83f);
                player->CreateConversation(2452);
            }
        });
        player->AddQuestDelayedEvent(quest, 13000, [owner, guid, instance, quest, lifetime]()
        {
            Player* player = ObjectAccessor::FindPlayer(owner);
            if (Outro(player, guid, instance, quest, lifetime)) player->CreateConversation(2453);
        });
        // Earned world cleanup does not depend on the player remaining online
        // or alive. The actual actor owns this identity-only callback.
        actor->AddDelayedEvent(27000, [owner, guid, instance]()
        {
            if (Creature* actor = WorldOutro(owner, guid, instance))
            {
                actor->GetMotionMaster()->MovePoint(0, -8332.05f, 274.51f, 156.83f);
                if (GameObject* doors = actor->FindNearestGameObject(251106, 60.0f)) doors->SetPhaseMask(2, true);
                if (Creature* garona = actor->FindNearestCreature(108222, 70.0f, true))
                {
                    garona->GetMotionMaster()->MovePoint(0, -8426.63f, 213.47f, 155.34f);
                    garona->DespawnOrUnsummon(15000);
                }
            }
        });
        return true;
    }
}

// 251107: native loot first, then215112/215114/215113 and linked equipment.
class go_the_kingslayers : public GameObjectScript
{
    struct AI : GameObjectAI
    {
        explicit AI(GameObject* object) : GameObjectAI(object) { }
        ObjectGuid claimant;
        std::weak_ptr<uint8> lifetime;
        uint32 quest = 0, elapsed = 0, retry = 0, creditRetry = 0;
        bool finished = false, busy = false, warned = false, creditWarned = false;
        void Cancel() { claimant.Clear(); lifetime.reset(); }
        bool GossipHello(Player* player, bool) override
        {
            return finished || !claimant.IsEmpty() || !KingslayersAcquisition::CanTake(player, go);
        }
        ObjectGuid GetGUID(int32 = 0) override { return claimant; }
        uint32 GetData(uint32) override { return finished; }
        void SetGUID(ObjectGuid const& guid, int32 = 0) override
        {
            if (finished || busy || !claimant.IsEmpty()) return;
            Player* player = ObjectAccessor::GetPlayer(*go, guid);
            if (!KingslayersAcquisition::CanTake(player, go) || !player->GetItemByEntry(128870)) return;
            quest = KingslayersAcquisition::Quest(player);
            QuestStatusData* status = player->getQuestStatus(quest);
            if (!status) return;
            if (!status->ScriptLifetime) status->ScriptLifetime = std::make_shared<uint8>();
            claimant = guid; lifetime = status->ScriptLifetime; elapsed = creditRetry = 0;
            SpellCastResult result = player->CastSpell(player, 215112, true);
            if (result != SPELL_CAST_OK)
            {
                Cancel();
                TC_LOG_WARN("scripts", "Kingslayers pickup: native215112 rejected (%u); retain item/object for retry", uint32(result));
            }
        }
        void UpdateAI(uint32 diff) override
        {
            if (claimant.IsEmpty() || busy) return;
            Player* player = ObjectAccessor::GetPlayer(*go, claimant);
            QuestStatusData* status = player ? player->getQuestStatus(quest) : nullptr;
            if (lifetime.expired() || !status || status->ScriptLifetime != lifetime.lock() ||
                !KingslayersAcquisition::CanTake(player, go, !finished) || !player->GetItemByEntry(128870))
            { Cancel(); return; }
            busy = true;
            struct Release { bool& flag; ~Release() { flag = false; } } release{busy};
            if (!finished)
            {
                elapsed = std::min<uint32>(15000, elapsed + std::min<uint32>(diff, 15000));
                if (!KingslayersAcquisition::Equipped(player))
                {
                    if (elapsed == 15000)
                    {
                        Cancel();
                        TC_LOG_WARN("scripts", "Kingslayers pickup: native215113 did not equip linked items; retain retry");
                    }
                    return;
                }
                Scenario* scenario = sScenarioMgr->GetScenario(go->GetInstanceId());
                if (!KingslayersAcquisition::Leaf(scenario, 48591))
                {
                    if (creditRetry > diff) { creditRetry -= diff; return; }
                    creditRetry = 2000;
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 51616);
                }
                scenario = sScenarioMgr->GetScenario(go->GetInstanceId());
                if (!scenario || scenario->GetCurrentStep() != 9 || !KingslayersAcquisition::Leaf(scenario, 48591))
                {
                    if (!creditWarned) TC_LOG_WARN("scripts", "Kingslayers pickup: native51616 not accepted; retain equipped claim");
                    creditWarned = true;
                    return;
                }
                finished = true; // native accepted pickup, never explicit scenario completion
            }
            status = player->getQuestStatus(quest);
            if (lifetime.expired() || !status || status->ScriptLifetime != lifetime.lock() ||
                !KingslayersAcquisition::CanTake(player, go, false)) { Cancel(); return; }
            if (retry > diff) { retry -= diff; return; }
            retry = 2000;
            if (!KingslayersAcquisition::StartOutro(player))
            {
                if (!warned) TC_LOG_WARN("scripts", "Kingslayers pickup: earned outro108367 creation failed; retain retry");
                warned = true;
                return;
            }
            Cancel();
            go->SetLootState(GO_JUST_DEACTIVATED);
        }
    };
public:
    go_the_kingslayers() : GameObjectScript("go_the_kingslayers") { }
    GameObjectAI* GetAI(GameObject* object) const override { return new AI(object); }
    bool OnGossipHello(Player* player, GameObject* object) override
    {
        if (!KingslayersAcquisition::CanTake(player, object) || object->AI()->GetData(0) || !object->AI()->GetGUID(0).IsEmpty()) return true;
        if (player->HasItemCount(128870, 1, true))
        {
            if (player->GetItemByEntry(128870)) object->SetLootState(GO_JUST_DEACTIVATED, player);
            return true;
        }
        return false;
    }
    void OnLootStateChanged(GameObject* object, uint32 state, Unit* target) override
    {
        Player* player = target ? target->ToPlayer() : nullptr;
        if (!object || state != GO_JUST_DEACTIVATED || object->getLootState() != GO_JUST_DEACTIVATED ||
            !KingslayersAcquisition::CanTake(player, object) || !player->GetItemByEntry(128870)) return;
        object->SetLootState(GO_READY); // hold summoned object across native equip/retry
        object->AI()->SetGUID(player->GetGUID(), 0);
    }
};

#include "kingslayers_return.h"

void AddSC_Kingslayers()
{
    new npc_garona_halforcen();
    new npc_kingslayers_small_events();

    new spell_kingslayers_smoke_bomb();
    new go_kingslayers_door();
    new npc_sister_althea_ebonlocke();
    new npc_merlis_malagan();
    new npc_garona_halforcen_outro();
    new go_the_kingslayers();
    RegisterGameObjectAI(go_kingslayers_return);
    RegisterSpellScript(spell_kingslayers_return);
};
