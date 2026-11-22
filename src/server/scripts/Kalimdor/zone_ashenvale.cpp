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
SDName: Ashenvale
SD%Complete: 70
SDComment: Quest support: 6544, 6482
SDCategory: Ashenvale Forest
EndScriptData */

/* ContentData
npc_torek
npc_ruul_snowhoof
EndContentData */

#include "../QuestItemChestAI.h"
#include "ObjectAccessor.h"
#include "QuestObjectScope.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"

/*####
# npc_torek
####*/

enum TorekSays
{
    SAY_READY                  = 0,
    SAY_MOVE                   = 1,
    SAY_PREPARE                = 2,
    SAY_WIN                    = 3,
    SAY_END                    = 4,
};

enum TorekSpells
{
    SPELL_REND                  = 11977,
    SPELL_THUNDERCLAP           = 8078,
};

enum TorekMisc
{
    QUEST_TOREK_ASSULT          = 6544,

    ENTRY_SPLINTERTREE_RAIDER   = 12859,
    ENTRY_DURIEL                = 12860,
    ENTRY_SILVERWING_SENTINEL   = 12896,
    ENTRY_SILVERWING_WARRIOR    = 12897,
};

class npc_torek : public CreatureScript
{
    public:

        npc_torek() : CreatureScript("npc_torek")
        {
        }

        struct npc_torekAI : public npc_escortAI
        {
            npc_torekAI(Creature* creature) : npc_escortAI(creature) {}

            uint32 Rend_Timer;
            uint32 Thunderclap_Timer;
            bool Completed;

            void WaypointReached(uint32 waypointId)
            {
                if (Player* player = GetPlayerForEscort())
                {
                    switch (waypointId)
                    {
                        case 1:
                            Talk(SAY_MOVE, player->GetGUID());
                            break;
                        case 8:
                            Talk(SAY_PREPARE, player->GetGUID());
                            break;
                        case 19:
                            //TODO: verify location and creatures amount.
                            me->SummonCreature(ENTRY_DURIEL, 1776.73f, -2049.06f, 109.83f, 1.54f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 25000);
                            me->SummonCreature(ENTRY_SILVERWING_SENTINEL, 1774.64f, -2049.41f, 109.83f, 1.40f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 25000);
                            me->SummonCreature(ENTRY_SILVERWING_WARRIOR, 1778.73f, -2049.50f, 109.83f, 1.67f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 25000);
                            break;
                        case 20:
                            DoScriptText(SAY_WIN, me, player);
                            Completed = true;
                            player->GroupEventHappens(QUEST_TOREK_ASSULT, me);
                            break;
                        case 21:
                            Talk(SAY_END, player->GetGUID());
                            break;
                    }
                }
            }

            void Reset()
            {
                Rend_Timer = 5000;
                Thunderclap_Timer = 8000;
                Completed = false;
            }

            void EnterCombat(Unit* /*who*/)
            {
            }

            void JustSummoned(Creature* summoned)
            {
                summoned->AI()->AttackStart(me);
            }

            void UpdateAI(uint32 diff)
            {
                npc_escortAI::UpdateAI(diff);

                if (!UpdateVictim())
                    return;

                if (Rend_Timer <= diff)
                {
                    DoCast(me->getVictim(), SPELL_REND);
                    Rend_Timer = 20000;
                } else Rend_Timer -= diff;

                if (Thunderclap_Timer <= diff)
                {
                    DoCast(me, SPELL_THUNDERCLAP);
                    Thunderclap_Timer = 30000;
                } else Thunderclap_Timer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_torekAI(creature);
        }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == QUEST_TOREK_ASSULT)
            {
                //TODO: find companions, make them follow Torek, at any time (possibly done by core/database in future?)
                creature->AI()->Talk(SAY_READY, player->GetGUID());
                creature->setFaction(113);

                if (npc_escortAI* pEscortAI = CAST_AI(npc_torekAI, creature->AI()))
                    pEscortAI->Start(true, true, player->GetGUID());
            }

            return true;
        }
};

/*####
# npc_ruul_snowhoof
####*/

enum RuulSnowhoof
{
    NPC_THISTLEFUR_URSA         = 3921,
    NPC_THISTLEFUR_TOTEMIC      = 3922,
    NPC_THISTLEFUR_PATHFINDER   = 3926,

    QUEST_FREEDOM_TO_RUUL       = 6482,

    GO_CAGE                     = 178147
};

Position const RuulSnowhoofSummonsCoord[6] =
{
    {3449.218018f, -587.825073f, 174.978867f, 4.714445f},
    {3446.384521f, -587.830872f, 175.186279f, 4.714445f},
    {3444.218994f, -587.835327f, 175.380600f, 4.714445f},
    {3508.344482f, -492.024261f, 186.929031f, 4.145029f},
    {3506.265625f, -490.531006f, 186.740128f, 4.239277f},
    {3503.682373f, -489.393799f, 186.629684f, 4.349232f}
};

class npc_ruul_snowhoof : public CreatureScript
{
    public:
        npc_ruul_snowhoof() : CreatureScript("npc_ruul_snowhoof") { }

        struct npc_ruul_snowhoofAI : public npc_escortAI
        {
            npc_ruul_snowhoofAI(Creature* creature) : npc_escortAI(creature) { }

            void WaypointReached(uint32 waypointId)
            {
                Player* player = GetPlayerForEscort();
                if (!player)
                    return;

                switch (waypointId)
                {
                    case 0:
                        me->SetUInt32Value(UNIT_FIELD_BYTES_1, 0);
                        if (GameObject* Cage = me->FindNearestGameObject(GO_CAGE, 20))
                            Cage->SetGoState(GO_STATE_ACTIVE);
                        break;
                    case 13:
                        me->SummonCreature(NPC_THISTLEFUR_TOTEMIC, RuulSnowhoofSummonsCoord[0], TEMPSUMMON_DEAD_DESPAWN, 60000);
                        me->SummonCreature(NPC_THISTLEFUR_URSA, RuulSnowhoofSummonsCoord[1], TEMPSUMMON_DEAD_DESPAWN, 60000);
                        me->SummonCreature(NPC_THISTLEFUR_PATHFINDER, RuulSnowhoofSummonsCoord[2], TEMPSUMMON_DEAD_DESPAWN, 60000);
                        break;
                    case 19:
                        me->SummonCreature(NPC_THISTLEFUR_TOTEMIC, RuulSnowhoofSummonsCoord[3], TEMPSUMMON_DEAD_DESPAWN, 60000);
                        me->SummonCreature(NPC_THISTLEFUR_URSA, RuulSnowhoofSummonsCoord[4], TEMPSUMMON_DEAD_DESPAWN, 60000);
                        me->SummonCreature(NPC_THISTLEFUR_PATHFINDER, RuulSnowhoofSummonsCoord[5], TEMPSUMMON_DEAD_DESPAWN, 60000);
                        break;
                    case 21:
                        player->GroupEventHappens(QUEST_FREEDOM_TO_RUUL, me);
                        break;
                }
            }

            void EnterCombat(Unit* /*who*/) {}

            void Reset()
            {
                if (GameObject* Cage = me->FindNearestGameObject(GO_CAGE, 20))
                    Cage->SetGoState(GO_STATE_READY);
            }

            void JustSummoned(Creature* summoned)
            {
                summoned->AI()->AttackStart(me);
            }

            void UpdateAI(uint32 diff)
            {
                npc_escortAI::UpdateAI(diff);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_ruul_snowhoofAI(creature);
        }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == QUEST_FREEDOM_TO_RUUL)
            {
                creature->setFaction(113);

                if (npc_escortAI* pEscortAI = CAST_AI(npc_ruul_snowhoofAI, (creature->AI())))
                    pEscortAI->Start(true, false, player->GetGUID());
            }

            return true;
        }
};

enum Muglash
{
    SAY_MUG_START1          = -1800054,
    SAY_MUG_START2          = -1800055,
    SAY_MUG_BRAZIER         = -1800056,
    SAY_MUG_BRAZIER_WAIT    = -1800057,
    SAY_MUG_ON_GUARD        = -1800058,
    SAY_MUG_REST            = -1800059,
    SAY_MUG_DONE            = -1800060,
    SAY_MUG_GRATITUDE       = -1800061,
    SAY_MUG_PATROL          = -1800062,
    SAY_MUG_RETURN          = -1800063,

    QUEST_VORSHA            = 6641,

    GO_NAGA_BRAZIER         = 178247,

    NPC_WRATH_RIDER         = 3713,
    NPC_WRATH_SORCERESS     = 3717,
    NPC_WRATH_RAZORTAIL     = 3712,

    NPC_WRATH_PRIESTESS     = 3944,
    NPC_WRATH_MYRMIDON      = 3711,
    NPC_WRATH_SEAWITCH      = 3715,

    NPC_VORSHA              = 12940,
    NPC_MUGLASH             = 12717
};

Position const FirstNagaCoord[3] =
{
    {3603.504150f, 1122.631104f, 1.635f, 0.0f},         // rider
    {3589.293945f, 1148.664063f, 5.565f, 0.0f},         // sorceress
    {3609.925537f, 1168.759521f, -1.168f, 0.0f}         // razortail
};

Position const SecondNagaCoord[3] =
{
    {3609.925537f, 1168.759521f, -1.168f, 0.0f},        // witch
    {3645.652100f, 1139.425415f, 1.322f, 0.0f},         // priest
    {3583.602051f, 1128.405762f, 2.347f, 0.0f}          // myrmidon
};

Position const VorshaCoord = {3633.056885f, 1172.924072f, -5.388f, 0.0f};

class npc_muglash : public CreatureScript
{
    public:
        npc_muglash() : CreatureScript("npc_muglash") { }

        struct npc_muglashAI : public npc_escortAI
        {
            npc_muglashAI(Creature* creature) : npc_escortAI(creature) { }

            uint8 WaveId;
            uint32 EventTimer;
            bool IsBrazierExtinguished;

            void JustSummoned(Creature* summoned)
            {
                summoned->AI()->AttackStart(me);
            }

            void WaypointReached(uint32 waypointId)
            {
                if (Player* player = GetPlayerForEscort())
                {
                    switch (waypointId)
                    {
                        case 0:
                            DoScriptText(SAY_MUG_START2, me, player);
                            break;
                        case 24:
                            DoScriptText(SAY_MUG_BRAZIER, me, player);

                            if (GameObject* go = GetClosestGameObjectWithEntry(me, GO_NAGA_BRAZIER, INTERACTION_DISTANCE*2))
                            {
                                go->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                                SetEscortPaused(true);
                            }
                            break;
                        case 25:
                            DoScriptText(SAY_MUG_GRATITUDE, me);
                            player->GroupEventHappens(QUEST_VORSHA, me);
                            break;
                        case 26:
                            DoScriptText(SAY_MUG_PATROL, me);
                            break;
                        case 27:
                            DoScriptText(SAY_MUG_RETURN, me);
                            break;
                    }
                }
            }

            void EnterCombat(Unit* /*who*/)
            {
                if (Player* player = GetPlayerForEscort())
                    if (HasEscortState(STATE_ESCORT_PAUSED))
                    {
                        if (urand(0, 1))
                            DoScriptText(SAY_MUG_ON_GUARD, me, player);
                        return;
                    }
            }

            void Reset()
            {
                EventTimer = 10000;
                WaveId = 0;
                IsBrazierExtinguished = false;
            }

            void JustDied(Unit* /*killer*/)
            {
                if (HasEscortState(STATE_ESCORT_ESCORTING))
                    if (Player* player = GetPlayerForEscort())
                        player->FailQuest(QUEST_VORSHA);
            }

            void DoWaveSummon()
            {
                switch (WaveId)
                {
                    case 1:
                        me->SummonCreature(NPC_WRATH_RIDER,     FirstNagaCoord[0], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        me->SummonCreature(NPC_WRATH_SORCERESS, FirstNagaCoord[1], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        me->SummonCreature(NPC_WRATH_RAZORTAIL, FirstNagaCoord[2], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        break;
                    case 2:
                        me->SummonCreature(NPC_WRATH_PRIESTESS, SecondNagaCoord[0], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        me->SummonCreature(NPC_WRATH_MYRMIDON,  SecondNagaCoord[1], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        me->SummonCreature(NPC_WRATH_SEAWITCH,  SecondNagaCoord[2], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        break;
                    case 3:
                        me->SummonCreature(NPC_VORSHA, VorshaCoord, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        break;
                    case 4:
                        SetEscortPaused(false);
                        DoScriptText(SAY_MUG_DONE, me);
                        break;
                }
            }

            void UpdateAI(uint32 uiDiff)
            {
                npc_escortAI::UpdateAI(uiDiff);

                if (!me->getVictim())
                {
                    if (HasEscortState(STATE_ESCORT_PAUSED) && IsBrazierExtinguished)
                    {
                        if (EventTimer < uiDiff)
                        {
                            ++WaveId;
                            DoWaveSummon();
                            EventTimer = 10000;
                        }
                        else
                            EventTimer -= uiDiff;
                    }
                    return;
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_muglashAI(creature);
        }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == QUEST_VORSHA)
            {
                if (npc_muglashAI* pEscortAI = CAST_AI(npc_muglashAI, creature->AI()))
                {
                    DoScriptText(SAY_MUG_START1, creature);
                    creature->setFaction(113);

                    pEscortAI->Start(true, false, player->GetGUID());
                }
            }
            return true;
        }
};

class go_naga_brazier : public GameObjectScript
{
    public:
        go_naga_brazier() : GameObjectScript("go_naga_brazier") { }

        bool OnGossipHello(Player* /*player*/, GameObject* go)
        {
            if (Creature* creature = GetClosestCreatureWithEntry(go, NPC_MUGLASH, INTERACTION_DISTANCE*2))
            {
                if (npc_muglash::npc_muglashAI* pEscortAI = CAST_AI(npc_muglash::npc_muglashAI, creature->AI()))
                {
                    DoScriptText(SAY_MUG_BRAZIER_WAIT, creature);

                    pEscortAI->IsBrazierExtinguished = true;
                    return false;
                }
            }

            return true;
        }
};

struct go_ashenvale_satyrnaar_fel_wood : QuestItemChestAI
{
    explicit go_ashenvale_satyrnaar_fel_wood(GameObject* object) : QuestItemChestAI(object, 26454, 24081) { }
};

struct npc_ashenvale_durak : ScriptedAI
{
    explicit npc_ashenvale_durak(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _owner.Clear();
        _receipt.reset();
        _stage = Stage::Idle;
        _delay = 0;
        _casts = 0;
        me->RemoveAurasDueToSpell(RevealSpell);
        me->RestoreFaction();
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
    }

    void sGossipSelect(Player* player, uint32 menu, uint32 option) override
    {
        if (_stage != Stage::Idle || menu != Durak || option != 0 || !me->IsAlive() ||
            !player->IsAlive() || player->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE ||
            player->GetQuestObjectiveData(QuestId, Durak) || !me->IsWithinDistInMap(player, INTERACTION_DISTANCE))
            return;

        _receipt = player->GetQuestCancellationReceipt(QuestId);
        if (!_receipt)
            return;

        _owner = player->GetGUID();
        _stage = Stage::SecondLine;
        _delay = 5000;
        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        player->PlayerTalkClass->SendCloseGossip();
        Talk(0, _owner);
    }

    void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType) override
    {
        Player* owner = ObjectAccessor::GetPlayer(*me, _owner);
        Player* player = attacker->GetCharmerOrOwnerPlayerOrPlayerItself();
        if (_stage != Stage::Combat || !HasCurrentAttempt(owner, 60.0f) || !player ||
            (player != owner && (!owner->GetGroup() || owner->GetGroup() != player->GetGroup())))
            damage = 0;
    }

    void UpdateAI(uint32 diff) override
    {
        if (_stage == Stage::Idle)
            return;

        Player* owner = ObjectAccessor::GetPlayer(*me, _owner);
        if (_stage == Stage::Combat)
        {
            if (!HasCurrentAttempt(owner, 60.0f))
                EnterEvadeMode();
            else if (UpdateVictim())
                DoMeleeAttackIfReady();
            return;
        }

        if (!HasCurrentAttempt(owner, 25.0f) || !owner->IsAlive())
        {
            Reset();
            return;
        }

        if (_delay > diff)
        {
            _delay -= diff;
            return;
        }

        switch (_stage)
        {
            case Stage::SecondLine:
                Talk(1, _owner);
                _stage = Stage::ThirdLine;
                _delay = 6000;
                break;
            case Stage::ThirdLine:
                Talk(2, _owner);
                _stage = Stage::Reveal;
                _delay = 6000;
                break;
            case Stage::Reveal:
                Reveal(owner);
                break;
            default:
                break;
        }
    }

private:
    enum class Stage { Idle, SecondLine, ThirdLine, Reveal, Combat };
    static constexpr uint32 QuestId = 13842;
    static constexpr uint32 Durak = 44414;
    static constexpr uint32 Persuaded = 44416;
    static constexpr uint32 RevealSpell = 82969;

    bool HasCurrentAttempt(Player* owner, float distance) const
    {
        return owner && _receipt && !_receipt->load() && me->IsWithinDistInMap(owner, distance) &&
            owner->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE &&
            owner->GetQuestCancellationReceipt(QuestId) == _receipt;
    }

    void Reveal(Player* owner)
    {
        if (!me->HasAura(RevealSpell, me->GetGUID()))
            DoCastSelf(RevealSpell, true);
        if (!me->HasAura(RevealSpell, me->GetGUID()))
        {
            if (++_casts < 3)
                _delay = 300;
            else
            {
                TC_LOG_WARN("scripts", "Durak 44414: native transform 82969 failed to apply after three attempts");
                Reset();
            }
            return;
        }

        // The Legion spell transforms the display. The actor's original entry
        // must survive for the separate, native death objective.
        owner->RewardPlayerAndGroupAtEvent(Persuaded, owner);
        _stage = Stage::Combat;
        me->setFaction(14);
        me->SetReactState(REACT_DEFENSIVE);
        me->SetLootRecipient(owner);
        AttackStart(owner);
    }

    Stage _stage = Stage::Idle;
    ObjectGuid _owner;
    std::shared_ptr<QuestCancellationReceipt const> _receipt;
    uint32 _delay = 0;
    uint8 _casts = 0;
};

void AddSC_ashenvale()
{
    new npc_torek();
    new npc_ruul_snowhoof();
    new npc_muglash();
    new go_naga_brazier();
    RegisterGameObjectAI(go_ashenvale_satyrnaar_fel_wood);
    RegisterCreatureAI(npc_ashenvale_durak);
}
