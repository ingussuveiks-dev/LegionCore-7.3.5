#ifndef EmbassyAllyChoice_h__
#define EmbassyAllyChoice_h__

#include "ConditionMgr.h"
#include "Creature.h"
#include "Player.h"

namespace EmbassyAllyChoice
{
    struct Ally
    {
        uint32 Response;
        uint32 Spell;
        uint32 Marker;
        uint32 SecondCriteria;
    };

    struct Choice
    {
        uint32 Id;
        TeamId Team;
        uint32 Questgiver;
        uint32 FirstQuest;
        uint32 SecondQuest;
        uint32 FirstConversation;
        uint32 SecondConversation;
        Ally Allies[2];

        Ally const* FindAlly(uint32 response) const
        {
            for (Ally const& ally : Allies)
                if (ally.Response == response)
                    return &ally;
            return nullptr;
        }
    };

    inline constexpr Choice Choices[] =
    {
        { 308, TEAM_HORDE, 126065, 50242, 50254, 65012, 65014,
            {{ 752, 247978, 47957, 65105 }, { 753, 247980, 47958, 65106 }} },
        { 322, TEAM_ALLIANCE, 126301, 50239, 50248, 64937, 64940,
            {{ 783, 251724, 48602, 65025 }, { 784, 251725, 48603, 65026 }} },
    };

    inline Choice const* Find(uint32 choiceId)
    {
        for (Choice const& choice : Choices)
            if (choice.Id == choiceId)
                return &choice;
        return nullptr;
    }

    inline bool CanSelect(Player& player, Choice const& choice, Ally const& ally)
    {
        if (!player.IsInWorld() || !player.IsAlive() || player.GetTeamId() != choice.Team || player.GetQuestRewardStatus(ally.Marker))
            return false;

        // The first choice requires both embassy conversations. The second
        // choice can only pursue the ally whose selection is still missing.
        bool const alreadyChoseAlly = player.GetQuestRewardStatus(choice.Allies[0].Marker) || player.GetQuestRewardStatus(choice.Allies[1].Marker);
        bool const choosingFirst = player.GetQuestStatus(choice.FirstQuest) == QUEST_STATUS_INCOMPLETE && !alreadyChoseAlly &&
            player.GetQuestObjectiveData(choice.FirstQuest, choice.FirstConversation) &&
            player.GetQuestObjectiveData(choice.FirstQuest, choice.SecondConversation);
        bool const choosingSecond = player.GetQuestStatus(choice.SecondQuest) == QUEST_STATUS_INCOMPLETE &&
            player.GetQuestRewardStatus(choice.FirstQuest) && alreadyChoseAlly;
        if (!choosingFirst && !choosingSecond)
            return false;

        Creature* questgiver = player.FindNearestCreature(choice.Questgiver, INTERACTION_DISTANCE);
        return questgiver && player.GetNPCIfCanInteractWith(questgiver->GetGUID(), UNIT_NPC_FLAG_GOSSIP) &&
            sConditionMgr->IsObjectMeetToConditions(&player,
                sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_PLAYER_CHOICE_RESPONS, ally.Response));
    }

    inline bool CanCreditCriteria(Player const& player, uint32 treeId)
    {
        for (Choice const& choice : Choices)
            if (treeId == choice.Allies[0].SecondCriteria || treeId == choice.Allies[1].SecondCriteria)
                return player.GetQuestRewardStatus(choice.Allies[0].Marker) && player.GetQuestRewardStatus(choice.Allies[1].Marker);
        return true;
    }
}

#endif
