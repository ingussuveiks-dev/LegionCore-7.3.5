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
SDName: Ghostlands
SD%Complete: 100
SDComment:
SDCategory: Ghostlands
EndScriptData */

/* ContentData
npc_rathis_tomber
npc_ranger_lilatha
EndContentData */

#include "AnowynCrystal.h"
#include "QuestItemLock.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "SpellMgr.h"
#include "SpellScript.h"

/*######
## npc_rathis_tomber
######*/

class npc_rathis_tomber : public CreatureScript
{
public:
    npc_rathis_tomber() : CreatureScript("npc_rathis_tomber") {}

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_TRADE)
            player->GetSession()->SendListInventory(creature->GetGUID());
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->isQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (creature->isVendor() && player->GetQuestRewardStatus(9152))
        {
            player->ADD_GOSSIP_ITEM(GossipOptionNpc::Vendor, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);
            player->SEND_GOSSIP_MENU(8432, creature->GetGUID());
        }
        else
            player->SEND_GOSSIP_MENU(8431, creature->GetGUID());

        return true;
    }
};

/*######
## npc_ranger_lilatha
######*/

enum eEnums
{
    SAY_START           = 0,
    SAY_PROGRESS1       = 1,
    SAY_PROGRESS2       = 1,
    SAY_PROGRESS3       = 2,
    SAY_END1            = 3,
    SAY_END2            = 4,
    SAY_CAPTAIN_ANSWER  = 0,

    QUEST_ESCAPE_FROM_THE_CATACOMBS     = 9212,
    GO_CAGE             = 181152,
    NPC_CAPTAIN_HELIOS  = 16220,
    FACTION_SMOON_E     = 1603,
};

class npc_ranger_lilatha : public CreatureScript
{
public:
    npc_ranger_lilatha() : CreatureScript("npc_ranger_lilatha") {}

    struct npc_ranger_lilathaAI : public npc_escortAI
    {
        npc_ranger_lilathaAI(Creature* creature) : npc_escortAI(creature) {}

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

                    Talk(SAY_START, player->GetGUID());
                    break;
                case 5:
                    Talk(SAY_PROGRESS1, player->GetGUID());
                    break;
                case 11:
                    Talk(SAY_PROGRESS2, player->GetGUID());
                    me->SetOrientation(4.762841f);
                    break;
                case 18:
                    {
                        Talk(SAY_PROGRESS3, player->GetGUID());
                        Creature* Summ1 = me->SummonCreature(16342, 7627.083984f, -7532.538086f, 152.128616f, 1.082733f, TEMPSUMMON_DEAD_DESPAWN, 0);
                        Creature* Summ2 = me->SummonCreature(16343, 7620.432129f, -7532.550293f, 152.454865f, 0.827478f, TEMPSUMMON_DEAD_DESPAWN, 0);
                        if (Summ1 && Summ2)
                        {
                            Summ1->Attack(me, true);
                            Summ2->Attack(player, true);
                        }
                        me->AI()->AttackStart(Summ1);
                    }
                    break;
                case 19:
                    me->SetWalk(false);
                    break;
                case 25:
                    me->SetWalk(true);
                    break;
                case 30:
                    if (player->IsPlayer())
                        CAST_PLR(player)->GroupEventHappens(QUEST_ESCAPE_FROM_THE_CATACOMBS, me);
                    break;
                case 32:
                    me->SetOrientation(2.978281f);
                    Talk(SAY_END1, player->GetGUID());
                    break;
                case 33:
                    me->SetOrientation(5.858011f);
                    Talk(SAY_END2, player->GetGUID());
                    if (Creature* CaptainHelios = me->FindNearestCreature(NPC_CAPTAIN_HELIOS, 50.0f))
                        CaptainHelios->AI()->Talk(SAY_CAPTAIN_ANSWER, player->GetGUID());
                    break;
            }
        }

        void Reset() override
        {
            if (GameObject* Cage = me->FindNearestGameObject(GO_CAGE, 20))
                Cage->SetGoState(GO_STATE_READY);
        }
    };

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_ESCAPE_FROM_THE_CATACOMBS)
        {
            creature->setFaction(113);

            if (npc_escortAI* pEscortAI = CAST_AI(npc_ranger_lilatha::npc_ranger_lilathaAI, creature->AI()))
                pEscortAI->Start(true, false, player->GetGUID());
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ranger_lilathaAI(creature);
    }

};

namespace
{
using spell_apothecarys_poison = spell_quest_item_opening;
using spell_aquantion_medallions = spell_quest_item_opening;
using spell_anowyn_controlling_orb = spell_quest_item_opening;

constexpr uint32 SeasoningQuest = 9275;

struct go_seasoning_rack : QuestItemLockAI
{
    explicit go_seasoning_rack(GameObject* object) : QuestItemLockAI(object, 29517, 22796) { }

    bool CanOpen(Player const* player) const override
    {
        return player && player->IsAlive() && go->GetMapId() == 530 && go->IsInMap(player) &&
            go->GetEntry() >= 181250 && go->GetEntry() <= 181252 && go->GetGoType() == GAMEOBJECT_TYPE_GOOBER &&
            go->GetGOInfo()->GetLockId() == 1655 && go->isSpawned() && !go->IsNeverVisible(player) &&
            !go->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) && player->IsWithinLOSInMap(go) &&
            player->GetQuestStatus(SeasoningQuest) == QUEST_STATUS_INCOMPLETE &&
            player->GetQuestObjectiveData(SeasoningQuest, go->GetEntry()) == 0;
    }
};

constexpr uint32 AquantionQuest = 9174;
constexpr uint32 AquantionMedallions = 22675;
constexpr uint32 AquantionOpening = 28226;
constexpr uint32 AquantionEntry = 16292;
// The placement, delay and lifetime from the original event_scripts row 9980.
Position const AquantionPlacement{7941.15f, -7636.45f, 116.19f, 5.66f};
constexpr uint32 AquantionDelay = 2 * IN_MILLISECONDS;
constexpr uint32 AquantionLifetime = 15 * MINUTE * IN_MILLISECONDS;

struct go_aquantion_altar : QuestItemLockAI
{
    explicit go_aquantion_altar(GameObject* object) : QuestItemLockAI(object, AquantionOpening, AquantionMedallions) { }

    bool CanOpen(Player const* player) const override
    {
        if (_pendingPlayer || !CanContinue(player) || !go->isSpawned() ||
            go->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE))
            return false;

        auto current = ObjectAccessor::GetCreature(*go, _aquantion);
        return !current || !current->IsAlive();
    }

    bool OnOpened(Player* player) override
    {
        _attempt = player->GetQuestCancellationReceipt(AquantionQuest);
        if (!_attempt)
            return false;
        _pendingPlayer = player->GetGUID();
        _timer = AquantionDelay;
        return true;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!_pendingPlayer)
            return;

        Player* player = ObjectAccessor::GetPlayer(*go, _pendingPlayer);
        if (_attempt->load(std::memory_order_acquire) || !CanContinue(player))
        {
            _pendingPlayer.Clear();
            _attempt.reset();
            return;
        }
        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }

        _pendingPlayer.Clear();
        _attempt.reset();
        if (TempSummon* actor = player->SummonCreature(AquantionEntry, AquantionPlacement, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, AquantionLifetime))
            _aquantion = actor->GetGUID();
        else
            TC_LOG_WARN("scripts", "Vanquishing Aquantion: altar %u could not publish creature %u", go->GetEntry(), AquantionEntry);
    }

private:
    bool CanContinue(Player const* player) const
    {
        SpellInfo const* spell = sSpellMgr->GetSpellInfo(AquantionOpening);
        return player && player->IsAlive() && go->GetMapId() == 530 && go->IsInMap(player) && go->GetEntry() == 181157 &&
            go->GetGoType() == GAMEOBJECT_TYPE_GOOBER && go->GetGOInfo()->GetLockId() == 1637 && !go->IsNeverVisible(player) &&
            spell && go->IsWithinDistInMap(player, spell->GetMaxRange(spell->IsPositive())) && player->IsWithinLOSInMap(go) &&
            player->GetQuestStatus(AquantionQuest) == QUEST_STATUS_INCOMPLETE &&
            player->GetQuestObjectiveData(AquantionQuest, AquantionEntry) == 0 && player->HasItemCount(AquantionMedallions, 1);
    }

    ObjectGuid _pendingPlayer;
    ObjectGuid _aquantion;
    std::shared_ptr<QuestCancellationReceipt const> _attempt;
    uint32 _timer = 0;
};

struct go_anowyn_moon_crystal : QuestItemLockAI
{
    explicit go_anowyn_moon_crystal(GameObject* object) : QuestItemLockAI(object, AnowynCrystal::Opening, AnowynCrystal::Orb) { }

    bool CanOpen(Player const* player) const override
    {
        return player && player->IsAlive() && go->IsInMap(player) && go->GetEntry() == AnowynCrystal::Entry &&
            AnowynCrystal::IsQuestSpawn(go->GetMapId(), go->GetDBTableGUIDLow()) && go->GetGoType() == GAMEOBJECT_TYPE_GOOBER &&
            go->GetGOInfo()->GetLockId() == AnowynCrystal::Lock && go->isSpawned() && !go->IsNeverVisible(player) &&
            !go->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) && player->IsWithinLOSInMap(go) &&
            player->GetQuestStatus(AnowynCrystal::Quest) == QUEST_STATUS_INCOMPLETE &&
            player->GetQuestObjectiveData(AnowynCrystal::Quest, AnowynCrystal::Entry) == 0;
    }
};
}

void AddSC_ghostlands()
{
    new npc_rathis_tomber();
    new npc_ranger_lilatha();
    RegisterGameObjectAI(go_seasoning_rack);
    RegisterSpellScript(spell_apothecarys_poison);
    RegisterGameObjectAI(go_aquantion_altar);
    RegisterSpellScript(spell_aquantion_medallions);
    RegisterGameObjectAI(go_anowyn_moon_crystal);
    RegisterSpellScript(spell_anowyn_controlling_orb);
}
