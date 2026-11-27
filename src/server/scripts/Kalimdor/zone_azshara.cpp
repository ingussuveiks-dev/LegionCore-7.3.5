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
SDName: Azshara
SD%Complete: 0
SDComment: Quest support:
SDCategory: Azshara
EndScriptData */

/* ContentData
EndContentData */

#include "ScriptMgr.h"
#include "Creature.h"
#include "CreatureTextMgr.h"
#include "GameObject.h"
#include "Map.h"
#include "Player.h"

namespace
{
constexpr uint32 PushTheButton = 14477;
constexpr uint32 GoblinDetonator = 196870;
constexpr uint64 DetonatorSpawn = 181401;
constexpr uint32 Gorek = 36662;
constexpr uint64 GorekSpawn = 3101072;

class go_azshara_goblin_detonator : public GameObjectScript
{
public:
    go_azshara_goblin_detonator() : GameObjectScript("go_azshara_goblin_detonator") { }

    bool OnGossipHello(Player* player, GameObject* button) override
    {
        if (button->GetEntry() != GoblinDetonator || button->GetDBTableGUIDLow() != DetonatorSpawn ||
            button->GetMapId() != 1)
            return false;

        QuestStatus const status = player->GetQuestStatus(PushTheButton);
        if ((status != QUEST_STATUS_INCOMPLETE && status != QUEST_STATUS_COMPLETE) ||
            player->FindQuestSlot(PushTheButton) >= MAX_QUEST_LOG_SIZE ||
            player->GetQuestObjectiveData(PushTheButton, GoblinDetonator) ||
            !player->IsAlive() || !button->isSpawned() || button->getLootState() != GO_READY ||
            !button->IsWithinDistInMap(player, button->GetInteractionDistance()) ||
            !button->IsWithinLOSInMap(player) ||
            button->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE))
            return true;

        // Older logs could reach COMPLETE on acceptance with no detonation.
        if (status == QUEST_STATUS_COMPLETE)
            player->IncompleteQuest(PushTheButton);

        button->UseDoorOrButton(0, false, player);
        player->KillCreditGO(GoblinDetonator, button->GetGUID());

        ObjectGuid const speaker = ObjectGuid::Create<HighGuid::Creature>(1, Gorek, GorekSpawn);
        if (Creature* gorek = button->GetMap()->GetCreature(speaker))
            if (gorek->IsAlive() && gorek->IsWithinDistInMap(player, 15.0f))
                sCreatureTextMgr->SendChat(gorek, 0, player->GetGUID(), CHAT_MSG_MONSTER_SAY, LANG_UNIVERSAL, TEXT_RANGE_PERSONAL);

        player->AddQuestDelayedEvent(PushTheButton, 5500, [player, speaker]
        {
            if (!player->IsInWorld() || !player->IsAlive() || player->GetMapId() != 1 ||
                player->GetQuestStatus(PushTheButton) != QUEST_STATUS_COMPLETE ||
                !player->GetQuestObjectiveData(PushTheButton, GoblinDetonator))
                return;

            if (Creature* gorek = player->GetMap()->GetCreature(speaker))
                if (gorek->IsAlive() && gorek->IsWithinDistInMap(player, 15.0f))
                    sCreatureTextMgr->SendChat(gorek, 1, player->GetGUID(), CHAT_MSG_MONSTER_SAY, LANG_UNIVERSAL, TEXT_RANGE_PERSONAL);
        });
        return true;
    }
};
}

void AddSC_azshara()
{
    new go_azshara_goblin_detonator();
}
