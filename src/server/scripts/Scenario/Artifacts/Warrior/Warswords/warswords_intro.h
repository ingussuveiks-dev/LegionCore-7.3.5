#pragma once

#include "InstanceScript.h"
#include "Player.h"
#include "Creature.h"
#include "Map.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include <set>

namespace WarswordsIntro
{
    uint32 const HelyaVisible = 0x574148;

    struct Viewers
    {
        virtual ~Viewers() = default;
        ObjectGuid helya;
        std::set<ObjectGuid> finished;
    };

    inline bool Retire(Player* player)
    {
        if (!player || !player->IsInWorld() || player->GetMapId() != 1511 || player->getClass() != CLASS_WARRIOR ||
            player->GetQuestRewardStatus(40043) ||
            (player->GetQuestStatus(40043) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40043) != QUEST_STATUS_COMPLETE))
            return false;
        auto* viewers = dynamic_cast<Viewers*>(player->GetInstanceScript());
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        if (!viewers || !scenario || scenario->GetScenarioId() != 944 || scenario->GetCurrentStep() < 6 || scenario->GetCurrentStep() > 7)
            return false;
        if (viewers->finished.insert(player->GetGUID()).second)
            if (Creature* helya = player->GetMap()->GetCreature(viewers->helya))
                helya->UpdateObjectVisibility();
        return true;
    }
}
