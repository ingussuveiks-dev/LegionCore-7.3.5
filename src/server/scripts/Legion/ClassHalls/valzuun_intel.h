#pragma once
#include <map>

namespace ValzuunIntel
{
    struct Flow
    {
        std::weak_ptr<uint8> lifetime;
        uint32 wait = 5000, remaining = 90000;
        uint8 next = 1;
    };

    bool Eligible(Creature* actor, Player* player, float distance)
    {
        return actor && actor->GetEntry() == 105333 && actor->IsInWorld() && actor->IsAlive() &&
            !actor->isInCombat() && player && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
            !player->HasPendingTeleport() && player->GetMap() == actor->GetMap() &&
            player->InSamePhase(actor) && player->IsWithinDistInMap(actor, distance) &&
            player->IsWithinLOSInMap(actor) && (!actor->HasQuestObjectScope() || actor->CanTargetQuestObject(player)) &&
            !player->GetQuestRewardStatus(41919) && player->GetQuestStatus(41919) == QUEST_STATUS_INCOMPLETE;
    }

    bool Current(Creature* actor, Player* player, Flow const& flow)
    {
        QuestStatusData* quest = player ? player->getQuestStatus(41919) : nullptr;
        return Eligible(actor, player, CreatureTextMgr::GetRangeForChatType(CHAT_MSG_MONSTER_SAY)) && quest &&
            !flow.lifetime.expired() && quest->ScriptLifetime == flow.lifetime.lock();
    }
}

// The old actor-wide list was replaced by the next player's selection, and
// credited before even its first line. Each real selection now owns its lease.
struct npc_valzuun_intel : SmartAI
{
    explicit npc_valzuun_intel(Creature* actor) : SmartAI(actor) { }
    std::map<ObjectGuid, ValzuunIntel::Flow> flows;

    void Reset() override
    {
        flows.clear();
        SmartAI::Reset();
    }

    void sGossipSelect(Player* player, uint32 sender, uint32 action) override
    {
        if (sender != 19472 || action != 0)
        {
            SmartAI::sGossipSelect(player, sender, action);
            return;
        }
        if (!ValzuunIntel::Eligible(me, player, INTERACTION_DISTANCE))
            return;
        auto existing = flows.find(player->GetGUID());
        if (existing != flows.end() && ValzuunIntel::Current(me, player, existing->second))
            return;
        for (uint8 group = 0; group < 4; ++group)
            if (!sCreatureTextMgr->TextExist(105333, group))
            {
                TC_LOG_ERROR("scripts.ai", "Valzuun: missing native intel text group %u; no dialogue credit", uint32(group));
                return;
            }
        QuestStatusData* quest = player->getQuestStatus(41919);
        if (!quest)
            return;
        if (!quest->ScriptLifetime)
            quest->ScriptLifetime = std::make_shared<uint8>();
        ValzuunIntel::Flow flow;
        flow.lifetime = quest->ScriptLifetime;
        flows[player->GetGUID()] = flow;
        player->PlayerTalkClass->SendCloseGossip();
        Talk(0, player->GetGUID());
    }

    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        std::vector<std::pair<ObjectGuid, ValzuunIntel::Flow>> finished;
        for (auto it = flows.begin(); it != flows.end();)
        {
            Player* player = ObjectAccessor::GetPlayer(*me, it->first);
            auto& flow = it->second;
            if (!ValzuunIntel::Current(me, player, flow) || diff >= flow.remaining)
            {
                it = flows.erase(it);
                continue;
            }
            flow.remaining -= diff;
            if (diff < flow.wait)
            {
                flow.wait -= diff;
                ++it;
                continue;
            }
            if (flow.next < 4)
            {
                if (!sCreatureTextMgr->TextExist(105333, flow.next))
                {
                    TC_LOG_ERROR("scripts.ai", "Valzuun: native intel text group %u disappeared; no dialogue credit", uint32(flow.next));
                    it = flows.erase(it);
                    continue;
                }
                uint8 group = flow.next++;
                // Retain native list gaps5s/10s; footage adds8s then the final2s.
                flow.wait = group == 1 ? 10000 : group == 2 ? 8000 : 2000;
                Talk(group, player->GetGUID());
                ++it;
            }
            else
            {
                finished.emplace_back(it->first, flow);
                it = flows.erase(it); // native credit may re-enter quest/AI callbacks
            }
        }
        for (auto const& completion : finished)
            if (Player* player = ObjectAccessor::GetPlayer(*me, completion.first))
                if (ValzuunIntel::Current(me, player, completion.second))
                    player->RewardPlayerAndGroupAtEvent(105333, player);
    }
};
