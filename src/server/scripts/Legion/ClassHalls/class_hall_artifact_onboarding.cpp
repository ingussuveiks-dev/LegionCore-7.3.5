/*
 * This file is part of the LegionCore Project.
 *
 * Artifact selection rewards cast a client-data spell which records the
 * hidden "<specialization> Chosen" quest.  The visible class-hall quest still
 * needs its selection credit and the selected acquisition quest must start.
 *
 * Do not reward an artifact here.  The weapon belongs at the end of its
 * acquisition quest/scenario, not in the PlayerChoice response handler.
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "QuestData.h"

#include <array>

namespace
{
    struct ArtifactRoute
    {
        uint32 ChoiceId;
        uint32 ResponseId;
        uint8 ClassId;
        uint32 ChosenQuestId;
        uint32 QuestId;
    };

    // IDs are the 7.3.5 PlayerChoice rows in the world database.  Several
    // public Legion forks use older response IDs, which do not match this DB.
    std::array<ArtifactRoute, 36> constexpr ArtifactRoutes =
    {{
        // Warrior: Strom'kar, Scale of the Earth-Warder, Warswords
        { 236, 491, CLASS_WARRIOR,      40582, 41105 },
        { 236, 492, CLASS_WARRIOR,      40580, 39191 },
        { 236, 493, CLASS_WARRIOR,      40581, 40043 },

        // Paladin: The Silver Hand, Truthguard, Ashbringer
        { 235, 488, CLASS_PALADIN,      40410, 42231 },
        { 235, 489, CLASS_PALADIN,      40411, 42000 },
        { 235, 490, CLASS_PALADIN,      40409, 42770 },

        // Hunter: Titanstrike, Talonclaw, Thas'dorah
        { 240, 504, CLASS_HUNTER,       40621, 41541 },
        { 240, 505, CLASS_HUNTER,       40619, 41542 },
        { 240, 506, CLASS_HUNTER,       40620, 41540 },

        // Rogue: The Kingslayers, The Dreadblades, Fangs of the Devourer
        { 262, 568, CLASS_ROGUE,        40842, 42501 },
        { 262, 569, CLASS_ROGUE,        40843, 40847 },
        { 262, 570, CLASS_ROGUE,        40844, 41919 },

        // Priest: T'uure, Xal'atath, Light's Wrath
        { 248, 532, CLASS_PRIEST,       40708, 41957 },
        { 248, 533, CLASS_PRIEST,       40707, 40710 },
        { 248, 534, CLASS_PRIEST,       40709, 41625 },

        // Death knight: Maw of the Damned, Blades of the Fallen Prince, Apocalypse
        { 253, 541, CLASS_DEATH_KNIGHT, 40722, 40740 },
        { 253, 542, CLASS_DEATH_KNIGHT, 40723, 38990 },
        { 253, 543, CLASS_DEATH_KNIGHT, 40724, 40930 },

        // Shaman: Doomhammer, Fist of Ra-den, Sharas'dal
        { 266, 587, CLASS_SHAMAN,       41328, 40224 },
        { 266, 588, CLASS_SHAMAN,       41329, 39771 },
        { 266, 589, CLASS_SHAMAN,       41330, 40341 },

        // Mage: Aluneth, Felo'melorn, Ebonchill
        { 265, 584, CLASS_MAGE,         41079, 42001 },
        { 265, 585, CLASS_MAGE,         41080, 40267 },
        { 265, 586, CLASS_MAGE,         41081, 42452 },

        // Warlock: Ulthalesh, Skull of the Man'ari, Scepter of Sargeras
        { 245, 523, CLASS_WARLOCK,      40686, 40495 },
        { 245, 524, CLASS_WARLOCK,      40688, 42128 },
        { 245, 525, CLASS_WARLOCK,      40687, 43100 },

        // Monk: Fu Zan, Sheilun, Fists of the Heavens
        { 242, 511, CLASS_MONK,         40640, 42762 },
        { 242, 512, CLASS_MONK,         40639, 41003 },
        { 242, 513, CLASS_MONK,         40638, 40569 },

        // Druid: Scythe of Elune, Fangs of Ashamane, Claws of Ursoc, G'Hanir
        { 247, 529, CLASS_DRUID,        40781, 40783 },
        { 247, 530, CLASS_DRUID,        40701, 42428 },
        { 247, 531, CLASS_DRUID,        40702, 41468 },
        { 247, 545, CLASS_DRUID,        40703, 40649 },

        // Demon hunter: Twinblades of the Deceiver, Aldrachi Warblades
        { 255, 546, CLASS_DEMON_HUNTER, 40817, 40819 },
        { 255, 547, CLASS_DEMON_HUNTER, 40818, 41803 }
    }};

    void CreditArtifactSelectionQuest(Player* player)
    {
        switch (player->getClass())
        {
            case CLASS_WARRIOR:
            case CLASS_PRIEST:
                player->KilledMonsterCredit(100583);
                break;
            case CLASS_PALADIN:
                player->KilledMonsterCredit(90369);
                break;
            case CLASS_HUNTER:
                player->KilledMonsterCredit(104634);
                break;
            case CLASS_ROGUE:
                player->KilledMonsterCredit(105953);
                break;
            case CLASS_DEATH_KNIGHT:
                player->KilledMonsterCredit(101441);
                break;
            case CLASS_SHAMAN:
                player->KilledMonsterCredit(96527);
                break;
            case CLASS_MAGE:
                player->KilledMonsterCredit(103037);
                player->KilledMonsterCredit(110559);
                player->KilledMonsterCredit(113614);
                break;
            case CLASS_WARLOCK:
                player->KilledMonsterCredit(101095);
                break;
            case CLASS_MONK:
                player->KilledMonsterCredit(100438);
                break;
            case CLASS_DRUID:
                player->KilledMonsterCredit(101296);
                player->KilledMonsterCredit(112077);
                player->KilledMonsterCredit(113814);
                break;
            case CLASS_DEMON_HUNTER:
                player->KilledMonsterCredit(105177);
                break;
            default:
                break;
        }
    }

    void StartArtifactQuest(Player* player, uint32 questId)
    {
        if (player->GetQuestStatus(questId) != QUEST_STATUS_NONE)
            return;

        if (Quest const* quest = sQuestDataStore->GetQuestTemplate(questId))
            if (player->CanTakeQuest(quest, false) && player->CanAddQuest(quest, false))
                player->AddQuest(quest, nullptr);
    }
}

class class_hall_artifact_onboarding : public PlayerScript
{
public:
    class_hall_artifact_onboarding() : PlayerScript("class_hall_artifact_onboarding") { }

    void OnLogin(Player* player) override
    {
        if (!player || player->getLevel() < 98)
            return;

        bool selectionCredited = false;
        for (ArtifactRoute const& route : ArtifactRoutes)
        {
            if (route.ClassId != player->getClass() || !player->IsQuestRewarded(route.ChosenQuestId))
                continue;

            if (!selectionCredited)
            {
                CreditArtifactSelectionQuest(player);
                selectionCredited = true;
            }

            StartArtifactQuest(player, route.QuestId);
        }
    }

    void OnPlayerChoiceResponse(Player* player, uint32 choiceId, uint32 responseId) override
    {
        if (!player || player->getLevel() < 98)
            return;

        for (ArtifactRoute const& route : ArtifactRoutes)
        {
            if (route.ChoiceId != choiceId || route.ResponseId != responseId || route.ClassId != player->getClass())
                continue;

            CreditArtifactSelectionQuest(player);
            StartArtifactQuest(player, route.QuestId);
            return;
        }
    }
};

void AddSC_class_hall_artifact_onboarding()
{
    new class_hall_artifact_onboarding();
}
