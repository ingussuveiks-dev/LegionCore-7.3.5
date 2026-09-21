#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Chat.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
constexpr uint32 ACTION_USE_SERVER_RATE = GOSSIP_ACTION_INFO_DEF;
constexpr uint32 ACTION_USE_PERSONAL_RATE = GOSSIP_ACTION_INFO_DEF + 100;
constexpr uint32 MAX_PERSONAL_RATE = 10;

class npc_change_rates : public CreatureScript
{
public:
    npc_change_rates() : CreatureScript("npc_change_rates") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        player->PlayerTalkClass->ClearMenus();

        float serverRate = sWorld->getRate(RATE_XP_KILL);
        float currentRate = player->GetSession()->GetPersonalXPRate();
        if (currentRate <= 0.0f)
            currentRate = serverRate;

        char text[128];
        std::snprintf(text, sizeof(text), "Current XP rate: x%.1f", currentRate);
        player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, text, GOSSIP_SENDER_MAIN, 0);

        std::snprintf(text, sizeof(text), "Use the server XP rate (x%.1f)", serverRate);
        player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, text, GOSSIP_SENDER_MAIN, ACTION_USE_SERVER_RATE);

        uint32 maximumSelectableRate = std::min<uint32>(MAX_PERSONAL_RATE,
            std::max<uint32>(1, static_cast<uint32>(std::floor(serverRate))));

        for (uint32 rate = 1; rate <= maximumSelectableRate; ++rate)
        {
            if (std::fabs(serverRate - static_cast<float>(rate)) < 0.001f)
                continue;

            std::snprintf(text, sizeof(text), "Use personal XP rate x%u", rate);
            player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, text, GOSSIP_SENDER_MAIN,
                ACTION_USE_PERSONAL_RATE + rate);
        }

        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();

        if (sender != GOSSIP_SENDER_MAIN)
        {
            player->CLOSE_GOSSIP_MENU();
            return true;
        }

        if (action == 0)
            return OnGossipHello(player, creature);

        if (action == ACTION_USE_SERVER_RATE)
        {
            player->GetSession()->SetPersonalXPRate(0.0f);
            ChatHandler(player).PSendSysMessage("Your personal XP rate was reset to the server rate (x%.1f).",
                sWorld->getRate(RATE_XP_KILL));
            player->CLOSE_GOSSIP_MENU();
            return true;
        }

        if (action > ACTION_USE_PERSONAL_RATE)
        {
            uint32 requestedRate = action - ACTION_USE_PERSONAL_RATE;
            uint32 maximumSelectableRate = std::min<uint32>(MAX_PERSONAL_RATE,
                std::max<uint32>(1, static_cast<uint32>(std::floor(sWorld->getRate(RATE_XP_KILL)))));

            if (requestedRate >= 1 && requestedRate <= maximumSelectableRate)
            {
                player->GetSession()->SetPersonalXPRate(static_cast<float>(requestedRate));
                ChatHandler(player).PSendSysMessage("Your personal XP rate is now x%u.", requestedRate);
            }
        }

        player->CLOSE_GOSSIP_MENU();
        return true;
    }
};
}

void AddSC_npc_change_rates()
{
    new npc_change_rates();
}
