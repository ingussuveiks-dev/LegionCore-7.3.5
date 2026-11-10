#include "../VantusRuneLifecycle.h"
#include "antorus.h"
#include "AntorusIntro.h"
#include "QuestData.h"

// 127233
struct npc_atbt_flamewear : public ScriptedAI
{
    npc_atbt_flamewear(Creature* cre) : ScriptedAI(cre) {}

    EventMap events;
    std::vector<ObjectGuid> targs{};

    void Reset() override
    {
        events.Reset();
    }

    void EnterCombat(Unit* who) override
    {
        events.RescheduleEvent(EVENT_1, 1500);
        events.RescheduleEvent(EVENT_2, 10000);
    }

    void SetGUID(const ObjectGuid& guid, int32 type) override
    {
        targs.push_back(guid);
        if (targs.size() != 2)
            return;
        for (bool x : {false, true})
            if (Player * player = ObjectAccessor::GetPlayer(*me, targs[x]))
                if (Aura* aura = player->GetAura(252621))
                    aura->SetScriptGuid(0, targs[!x]);
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
                DoCast(252614);
                events.RescheduleEvent(EVENT_1, 3500);
                break;
            case EVENT_2:
                targs.clear();
                DoCast(252621);
                events.RescheduleEvent(EVENT_2, urand(1000, 14000));
                break;
            }
        }

        DoMeleeAttackIfReady();
    }

};

// 125819
struct npc_atbt_battleship_controller : public ScriptedAI
{
    npc_atbt_battleship_controller(Creature* cre) : ScriptedAI(cre)
    {
        SetCanSeeEvenInPassiveMode(true);
    }

    EventMap events;
    bool start = false;

    void Reset() override
    {
        events.Reset();
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (start)
            return;

        if (!who->IsPlayer())
            return;

        if (me->GetDistance(who) > 98.0f)
            return;

        start = true;

        events.RescheduleEvent(EVENT_1, 3000);

        if (Creature* cre = me->FindNearestCreature(125480, 90.0, true)) // really need it =C
            cre->AI()->Talk(0);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!start)
            return;

        events.Update(diff);

        if (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_1:
                me->SummonCreature(NPC_BATTLE_SHIP, -3268.71f, 10120.37f, -67.19f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 9000);
                events.RescheduleEvent(EVENT_1, urand(3000, 5000));
                break;
            }
        }
    }

};

// 125771
struct npc_atbt_battleship : public ScriptedAI
{
    npc_atbt_battleship(Creature* cre) : ScriptedAI(cre) {}

    void IsSummonedBy(Unit* owner) override
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetCanFly(true);
        me->GetMotionMaster()->MovePoint(0, -3312.39f, 9992.18f, -87.09f); 

        me->AddDelayedEvent(urand(2000, 3000), [this]() -> void
        {
            uint8 i = 0;
            for (float y = 10099.79f; y >= 9980.05f; y -= frand(2, 7))
            {
                me->AddDelayedEvent(100 * ++i, [this, y]() -> void
                {
                    float x = frand(-3305.59f, -3286.11f);
                    me->CastSpell(x, y, -115.73f, 249933, true);
                });
            }
        });
    }
};

// 128289
struct npt_atbt_teleport : public ScriptedAI
{
    npt_atbt_teleport(Creature* cre) : ScriptedAI(cre) {}

    void OnSpellClick(Unit* who) override
    {
        if (!who)
            return;

        if (who->IsMounted())
        {
            who->Dismount();
            return;
        }

        who->CastSpell(who, 253773, true);
        who->GetMotionMaster()->MoveIdle();
        who->GetMotionMaster()->MovePath(me->GetPositionX() < -3100.0f ? 12871196 : 12871197, false, frand(-2, 2), frand(-2, 2));
    }
};


//125720, 128303 128304
class npt_atbt_tele_gates : public CreatureScript
{
public:
    npt_atbt_tele_gates() : CreatureScript("npt_atbt_tele_gates") {}

    bool OnGossipHello(Player* player, Creature* me)  override
    {
        InstanceScript* instance = me->GetInstanceScript();
        if (!instance)
            return false;

        bool isDone = true;
        for (uint8 i = DATA_WORLDBREAKER; i <= DATA_EONAR; ++i)
            if (instance->GetBossState(i) != DONE)
                isDone = false;

        if (isDone && me->GetEntry() != 128303) 
            player->ADD_GOSSIP_ITEM_DB(21377, 0, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1); // imonar

        if (me->GetEntry() != 125720) 
            player->ADD_GOSSIP_ITEM_DB(21377, 3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2); // start

        isDone = true;
        for (uint8 i = DATA_IMONAR; i <= DATA_KINGAROTH; ++i)
            if (instance->GetBossState(i) != DONE)
                isDone = false;

        if (me->GetEntry() != 128304)
            player->ADD_GOSSIP_ITEM_DB(21377, 1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3); // Varimatras

        player->SEND_GOSSIP_MENU(32550, me->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* me, uint32 sender, uint32 action)  override
    {
        if (me->GetEntry() == 125720)
            player->CastSpell(player, 254498);
        else if (me->GetEntry() == 128303)
            player->CastSpell(player, 254499);

        switch (action)
        {
        case GOSSIP_ACTION_INFO_DEF + 1:
            player->NearTeleportTo(-10574.47f, 8772.07f, 1871.46f, 4.68f);
            break;
        case GOSSIP_ACTION_INFO_DEF + 2:
            player->NearTeleportTo(-3437.5f, 10156.9f, -150.022f, 0.0f);
            break;
        case GOSSIP_ACTION_INFO_DEF + 3:
            player->NearTeleportTo(-12633.91f, -3369.47f, 2513.82f, 0.0f);
            break;
        }

        player->CLOSE_GOSSIP_MENU();
        return true;
    }
};

// 128169
class npc_atbt_muradin : public CreatureScript
{
public:
    npc_atbt_muradin() : CreatureScript("npc_atbt_muradin") {}

    bool OnGossipSelect(Player* player, Creature* me, uint32 sender, uint32 action)  override
    {
        player->CastSpell(player, 254311);
        player->NearTeleportTo(2825.47f, -4567.23f, 291.94f, 0.0f);
        player->CLOSE_GOSSIP_MENU();
        return true;
    }
};

// 252621
class spell_atbt_bound_by_fel : public AuraScript
{
    PrepareAuraScript(spell_atbt_bound_by_fel);

    ObjectGuid second{};

    void OnTick(AuraEffect const* /*aurEff*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        if (target->GetAuraCount(252621) >= 20)
        {
            target->RemoveAurasDueToSpell(252621);
            return;
        }

        if(second.IsEmpty())
            return;

        Unit* second_targ = ObjectAccessor::GetPlayer(*target, second);
        if (!second_targ)
            return;

        target->CastSpell(second_targ, 252622);
        if (target->GetDistance(second_targ) <= 10)
            target->CastSpell(second_targ, 252623);
        else if (Unit* caster = GetCaster())
            if (caster->IsAlive())
                caster->CastSpell(target, 252621);
    }

    void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes mode)
    {
        if (!GetCaster() || !GetTarget())
            return;

        Creature* cre = GetCaster()->ToCreature();
        if (!cre)
            return;

        Unit* target = GetTarget();
        cre->AI()->SetGUID(target->GetGUID());
    }

    void SetGuid(uint32 type, ObjectGuid const& guid) override
    {
        second = guid;
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_atbt_bound_by_fel::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        OnEffectApply += AuraEffectApplyFn(spell_atbt_bound_by_fel::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 253600
class spell_atbt_soulburn : public AuraScript
{
    PrepareAuraScript(spell_atbt_soulburn);

    void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes mode)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        target->CastSpell(target, 253601);

        std::list<Player*> players;
        target->GetPlayerListInGrid(players, 8.0f);
        players.remove_if([](Player* player)
        {
            if (!player) 
                return true;
        
            return !player->HasAura(253600);
        });
        Trinity::Containers::RandomResizeList(players, 5);
        for (Player* player : players)
            player->CastSpell(player, 253600);
    }

    void Register() override
    {
        OnEffectRemove += AuraEffectApplyFn(spell_atbt_soulburn::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 252740
class spell_atbt_anihilation : public SpellScript
{
    PrepareSpellScript(spell_atbt_anihilation);

    bool check = false;

    void HandleAfterCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;
        
        for (uint8 i = 0; i < 3; ++i)
        {
            Position pos(caster->GetPosition());
            pos = caster->GetFirstCollisionPosition(15.0f * frand(0.3f, 1.0f), static_cast<float>(rand_norm()) * static_cast<float>(2.0f * M_PI));
            caster->CastSpell(pos, 252742, false);
            caster->CastSpell(pos, 252741, false);
        }
    }

    void Register()
    {
        AfterCast += SpellCastFn(spell_atbt_anihilation::HandleAfterCast);
    }
};

//800-812, 815-816
class eventobject_antorus_into : public EventObjectScript
{
public:
    eventobject_antorus_into() : EventObjectScript("eventobject_antorus_into") { }

    bool OnTrigger(Player* player, EventObject* eo, bool enter) override
    {
        if (!enter)
            return true;
        auto script = dynamic_cast<AntorusIntroInstance*>(player->GetInstanceScript());
        if (!script || player->GetMap() != eo->GetMap() || !player->IsAlive())
            return false;

        uint32 entry = eo->GetEntry();
        if (entry == 805)
        {
            if (!player->HasAura(SPELL_SURGE_OF_LIFE_OVERRIDE))
            {
                std::list<Creature*> mobs;
                player->GetCreatureListWithEntryInGrid(mobs, 127681, 30.0f);
                for (auto npc : mobs)
                    npc->CastSpell(player, SPELL_BLESSING_LIFEBINDER_VISUAL, true);
            }
            return true;
        }
        if (entry < 800 || entry > 812 || script->IntroPlayed(entry))
            return true;
        if (!script->BeginIntro(entry, player, eo))
            return true;

        switch (entry)
        {
            case 800:
                if (!script->IntroStarted(entry))
                {
                    if (!AntorusIntroInstance::StartConversation(player, 5545))
                    {
                        script->FinishIntro(entry, false);
                        break;
                    }
                    script->SetIntroStarted(entry);
                }
                script->QueueIntroPlayer(entry, player, 7000, [script, entry](Player* target)
                {
                    script->FinishIntro(entry, AntorusIntroInstance::StartConversation(target, 5522));
                });
                break;
            case 801:
                script->QueueIntroPlayer(entry, player, 1000, [script, entry](Player* target)
                {
                    script->FinishIntro(entry, AntorusIntroInstance::StartConversation(target, 5524));
                });
                break;
            case 802:
                script->QueueIntroWorld(entry, 8000, [script]()
                {
                    script->instance->LoadGrid(-3705.06f, 1357.21f);
                    if (Creature* boss = script->instance->GetCreature(script->GetGuidData(NPC_HASABEL)))
                    {
                        boss->AI()->ZoneTalk(0);
                        return true;
                    }
                    return false;
                });
                break;
            case 803:
                script->FinishIntro(entry, script->GetBossState(DATA_HASABEL) == DONE &&
                    eo->SummonCreature(NPC_IMAGE_OF_EONAR, eo->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 21000));
                break;
            case 804:
                if (Creature* eonar = eo->SummonCreature(NPC_EONAR_EVENT, -4299.92f, -11179.68f, 817.63f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 40000))
                {
                    // This optional conversation must not discard the accepted actor.
                    player->CreateConversation(5703);
                    eonar->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                    script->FinishIntro(entry, true);
                }
                else
                    script->FinishIntro(entry, false);
                break;
            case 807:
                if (script->GetBossState(DATA_IMONAR) == DONE)
                {
                    script->FinishIntro(entry, true);
                    break;
                }
                script->instance->LoadGrid(-10599.29f, 9036.34f);
                if (Creature* intro = eo->SummonCreature(NPC_IMONAR_INTRO, -10574.15f, 8606.92f, 1909.52f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 30000))
                {
                    if (!script->IntroStarted(entry) && AntorusIntroInstance::StartConversation(player, 5608))
                        script->SetIntroStarted(entry);
                    script->instance->LoadGrid(-10754.15f, 8606.92f);
                    intro->GetMotionMaster()->MovePoint(0, -10572.75f, 8715.77f, 1891.68f);
                    ObjectGuid guid = intro->GetGUID();
                    script->QueueIntroWorld(entry, 9000, [script, guid]()
                    {
                        Creature* intro = script->instance->GetCreature(guid);
                        Creature* boss = script->instance->GetCreature(script->GetGuidData(NPC_IMONAR));
                        if (!intro || !intro->IsAlive() || !boss || !boss->IsAlive())
                            return false;
                        boss->SetVisible(true);
                        boss->CastSpell(boss, 249569);
                        boss->AI()->Talk(0);
                        intro->AddDelayedEvent(3000, [intro]()
                        {
                            intro->GetMotionMaster()->MovePoint(0, -10791.75f, 8883.12f, 2056.20f);
                        });
                        return true;
                    }, [script, guid]()
                    {
                        if (Creature* intro = script->instance->GetCreature(guid))
                            intro->DespawnOrUnsummon();
                    });
                }
                else
                    script->FinishIntro(entry, false);
                break;
            case 810:
            {
                if (!script->IntroStarted(entry))
                {
                    uint32 const entries[3]{125683, 124913, 125682};
                    Position const pos[3]{
                        {-12639.84f, -3029.82f, 2498.75f, 1.57f},
                        {-12631.39f, -3029.82f, 2498.97f, 1.57f},
                        {-12626.20f, -3029.82f, 2498.97f, 1.57f}
                    };
                    std::vector<Creature*> actors;
                    for (uint8 i = 0; i < 3; ++i)
                        if (Creature* actor = eo->SummonCreature(entries[i], pos[i]))
                            actors.push_back(actor);
                    if (actors.size() != 3)
                    {
                        for (Creature* actor : actors)
                            actor->DespawnOrUnsummon();
                        script->FinishIntro(entry, false);
                        break;
                    }
                    for (Creature* actor : actors)
                    {
                        actor->SetReactState(REACT_ATTACK_OFF);
                        actor->AddDelayedEvent(1000, [actor]()
                        {
                            actor->GetMotionMaster()->MovePoint(1, actor->GetPositionX(), -2947.84f + frand(-3.0f, 3.0f), 2499.12f);
                        });
                    }
                    script->SetIntroStarted(entry);
                }
                script->QueueIntroPlayer(entry, player, 1000, [script, entry](Player* target)
                {
                    script->FinishIntro(entry, target->CastSpell(target, 250797) == SPELL_CAST_OK);
                });
                break;
            }
            case 812:
                script->instance->LoadGrid(-12633.20f, -2231.10f);
                script->QueueIntroPlayer(entry, player, 1000, [script, entry](Player* target)
                {
                    if (!script->IntroStarted(entry))
                    {
                        if (target->CastSpell(target, 250700) != SPELL_CAST_OK)
                        {
                            script->FinishIntro(entry, false);
                            return;
                        }
                        script->SetIntroStarted(entry);
                    }
                    script->QueueIntroPlayer(entry, target, 50000, [script, entry](Player* target)
                    {
                        script->FinishIntro(entry, target->CastSpell(target, 250728) == SPELL_CAST_OK);
                    });
                });
                break;
            default:
            {
                uint32 spell = entry == 806 ? 249441 : entry == 808 ? 249802 : entry == 809 ? 250695 : 250698;
                if (entry == 809)
                    script->instance->LoadGrid(-12694.59f, -3597.12f);
                script->QueueIntroPlayer(entry, player, 1000, [script, entry, spell](Player* target)
                {
                    script->FinishIntro(entry, target->CastSpell(target, spell) == SPELL_CAST_OK);
                });
                break;
            }
        }
        return true;
    }
};

class spell_vantus_rune_antorus : public SpellScriptLoader
{
public:
    spell_vantus_rune_antorus() : SpellScriptLoader("spell_vantus_rune_antorus") {}

    class spell_vantus_rune_antorus_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_vantus_rune_antorus_AuraScript);

        uint32 checkTimer = 1000;

        static std::array<LegionVantus::Encounter, 11> const& Encounters()
        {
            static std::array<LegionVantus::Encounter, 11> const entries = {{
                { 250153, 250152, DATA_WORLDBREAKER },
                { 250156, 250155, DATA_FELHOUNDS },
                { 250167, 250166, DATA_ANTORAN },
                { 250160, 250159, DATA_HASABEL },
                { 250150, 250149, DATA_EONAR },
                { 250158, 250157, DATA_IMONAR },
                { 250148, 250147, DATA_KINGAROTH },
                { 250165, 250164, DATA_VARIMATHRAS },
                { 250163, 250162, DATA_COVEN },
                { 250144, 250143, DATA_AGGRAMAR },
                { 250146, 250145, DATA_ARGUS },
            }};
            return entries;
        }

        void OnUpdate(uint32 diff, AuraEffect* /*aurEff*/)
        {
            LegionVantus::Update(GetUnitOwner(), GetId(), 1712, diff, checkTimer, Encounters());
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            LegionVantus::Cleanup(GetUnitOwner(), GetId(), Encounters());
        }

        void Register() override
        {
            OnEffectUpdate += AuraEffectUpdateFn(spell_vantus_rune_antorus_AuraScript::OnUpdate, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
            AfterEffectRemove += AuraEffectRemoveFn(spell_vantus_rune_antorus_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_vantus_rune_antorus_AuraScript();
    }

    class spell_vantus_rune_antorus_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_vantus_rune_antorus_SpellScript);

        SpellCastResult CheckCast()
        {
            if (auto player = GetCaster()->ToPlayer())
                if (!player->GetQuestRewardStatus(39695))
                    return SPELL_CAST_OK;

            SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_YOU_ALREADY_USED_VANTUS_RUNE);
            return SPELL_FAILED_CUSTOM_ERROR;
        }

        void HandleOnCast()
        {
            if (auto player = GetCaster()->ToPlayer())
                if (Quest const* quest = sQuestDataStore->GetQuestTemplate(39695))
                    if (player->CanTakeQuest(quest, false))
                        player->CompleteQuest(39695);
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_vantus_rune_antorus_SpellScript::CheckCast);
            OnCast += SpellCastFn(spell_vantus_rune_antorus_SpellScript::HandleOnCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_vantus_rune_antorus_SpellScript();
    }
};

class spell_vantus_rune_antorus_choose : public SpellScript
{
    PrepareSpellScript(spell_vantus_rune_antorus_choose);

    SpellCastResult CheckCast()
    {
        if (auto player = GetCaster()->ToPlayer())
        {
            if (player->GetMapId() != 1712)
                return SPELL_FAILED_NOT_HERE;

            if (player->GetQuestRewardStatus(39695))
            {
                SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_YOU_ALREADY_USED_VANTUS_RUNE);
                return SPELL_FAILED_CUSTOM_ERROR;
            }
        }

        return SPELL_CAST_OK;
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        Unit* boss = GetHitUnit();
        if (!boss)
            return;

        switch (boss->GetEntry())
        {
        case 122450:
            if (!caster->HasAura(250153))
                caster->CastSpell(caster, 250153, true);
            break;
        case 122477:
        case 122135:
            if (!caster->HasAura(250156))
                caster->CastSpell(caster, 250156, true);
            break;
        case 122367:
            if (!caster->HasAura(250167))
                caster->CastSpell(caster, 250167, true);
            break;
        case 122104:
            if (!caster->HasAura(250160))
                caster->CastSpell(caster, 250160, true);
            break;
        case 122500:
            if (!caster->HasAura(250150))
                caster->CastSpell(caster, 250150, true);
            break;
        case 124158:
            if (!caster->HasAura(250158))
                caster->CastSpell(caster, 250158, true);
            break;
        case 122578:
            if (!caster->HasAura(250148))
                caster->CastSpell(caster, 250148, true);
            break;
        case 122366:
            if (!caster->HasAura(250165))
                caster->CastSpell(caster, 250165, true);
            break;
        case 122468:
        case 122467:
        case 122469:
            if (!caster->HasAura(250163))
                caster->CastSpell(caster, 250163, true);
            break;
        case 124691:
            if (!caster->HasAura(250144))
                caster->CastSpell(caster, 250144, true);
            break;
        case 124828:
            if (!caster->HasAura(250146))
                caster->CastSpell(caster, 250146, true);
            break;
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_vantus_rune_antorus_choose::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        OnCheckCast += SpellCheckCastFn(spell_vantus_rune_antorus_choose::CheckCast);
    }
};

void AddSC_antorus()
{
    RegisterCreatureAI(npc_atbt_flamewear);
    RegisterCreatureAI(npc_atbt_battleship_controller);
    RegisterCreatureAI(npc_atbt_battleship);
    RegisterCreatureAI(npt_atbt_teleport);
    new npt_atbt_tele_gates;
    new npc_atbt_muradin();
    RegisterAuraScript(spell_atbt_bound_by_fel);
    RegisterAuraScript(spell_atbt_soulburn);
    RegisterSpellScript(spell_atbt_anihilation);
    RegisterSpellScript(spell_vantus_rune_antorus_choose);
    new eventobject_antorus_into();
    new spell_vantus_rune_antorus;
}
