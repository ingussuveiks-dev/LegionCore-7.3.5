#pragma once

namespace MalgalorFinale
{
    constexpr uint32 IntroStartedData = 93301;
    constexpr uint32 IntroReady = 1, IntroStarted = 2;
    struct Member
    {
        ObjectGuid player;
        uint32 quest = 0, instance = 0, newAuras = 0;
        std::weak_ptr<uint8> attempt;
    };

    inline uint32 Quest(Player* player)
    {
        if (player)
            for (uint32 id : {38904u, 42815u})
                if (!player->GetQuestRewardStatus(id) &&
                    (player->GetQuestStatus(id) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(id) == QUEST_STATUS_COMPLETE)) return id;
        return 0;
    }

    inline Scenario* Progress(Creature* actor)
    {
        if (!actor || actor->GetEntry() != 92608 || actor->GetDBTableGUIDLow() != 361167 || actor->GetMapId() != 1500) return nullptr;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 933 ? scenario : nullptr;
    }

    inline bool Victory(Scenario* scenario)
    {
        if (!scenario) return false;
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(45777);
        return found != progress->end() && found->second.Counter;
    }

    inline std::vector<Member> Capture(Creature* actor)
    {
        std::vector<Member> result;
        Scenario* scenario = Progress(actor);
        if (!scenario || scenario->GetCurrentStep() != 4 || Victory(scenario)) return result;
        // These are the actual participants of this private scenario, not
        // arbitrary nearby outdoor players. Allies may legitimately land the
        // final blow; never assign player credit to their Creature GUID.
        for (auto const& reference : actor->GetMap()->GetPlayers())
            if (Player* player = reference.getSource())
            {
                uint32 quest = Quest(player);
                if (!quest || player->getClass() != CLASS_WARRIOR || !player->IsInWorld() || !player->CanContact() ||
                    player->isGameMaster() || player->HasPendingTeleport() || player->GetMap() != actor->GetMap() || !player->InSamePhase(actor)) continue;
                player->AddQuestDelayedEvent(quest, 0, [] { });
                auto status = player->getQuestStatus(quest);
                if (status && status->ScriptLifetime)
                    result.push_back({player->GetGUID(), quest, actor->GetInstanceId(), 0, status->ScriptLifetime});
            }
        return result;
    }

    inline Player* Original(Member const& member, bool sameMap = true)
    {
        Player* player = ObjectAccessor::FindPlayer(member.player);
        auto status = player ? player->getQuestStatus(member.quest) : nullptr;
        return player && status && !member.attempt.expired() && status->ScriptLifetime == member.attempt.lock() &&
            Quest(player) == member.quest && player->getClass() == CLASS_WARRIOR && player->IsInWorld() && player->CanContact() &&
            !player->isGameMaster() && (!sameMap || (player->GetMapId() == 1500 && player->GetInstanceId() == member.instance)) ? player : nullptr;
    }

    inline void Clean(Player* player, uint32 mask)
    {
        uint32 index = 0;
        for (uint32 id : {185627u, 185673u, 185674u})
        {
            if (mask & (1u << index)) player->RemoveAurasDueToSpell(id, player->GetGUID());
            ++index;
        }
    }

    inline uint32 AuraMask(Player* player)
    {
        uint32 mask = 0, index = 0;
        for (uint32 id : {185627u, 185673u, 185674u})
        {
            if (player->HasAura(id, player->GetGUID())) mask |= 1u << index;
            ++index;
        }
        return mask;
    }

    inline void Depart(Member member, uint32 retries = 0)
    {
        Player* present = ObjectAccessor::FindPlayer(member.player);
        if (!present || !present->IsInWorld()) return;
        Player* player = Original(member);
        Scenario* scenario = player ? sScenarioMgr->GetScenario(member.instance) : nullptr;
        bool eligible = player && !player->HasPendingTeleport() && scenario && scenario->GetScenarioId() == 933 &&
            Victory(scenario) && player->GetQuestObjectiveData(member.quest, 103661);
        if (eligible)
            if (Creature* valkyr = player->FindNearestCreature(93818, 50.0f, true))
                if (TempSummon* summon = valkyr->ToTempSummon())
                    if (summon->GetSummonerGUID() == player->GetGUID()) valkyr->CastSpell(player, 185714);
        // Presentation cleanup is independent of quest abandonment/map change.
        // Only auras newly created by this receipt are removed.
        Clean(present, member.newAuras);
        member.newAuras = 0;
        player = Original(member);
        if (!eligible || !player || player->HasPendingTeleport()) return;
        if (!player->TeleportTo(1479, 1071.81f, 7224.32f, 97.891f, 3.15048f))
        {
            TC_LOG_WARN("scripts", "Malgalor finale: native Skyhold departure refused; real victory retained");
            if (retries < 5) player->AddDelayedEvent(2000, [member, retries] { Depart(member, retries + 1); });
        }
    }

    inline void Death(Creature* actor, std::vector<Member> members, ObjectGuid primary)
    {
        Scenario* scenario = Progress(actor);
        if (!scenario || scenario->GetCurrentStep() != 4 || Victory(scenario) || actor->IsAlive() ||
            actor->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1)) return;
        std::vector<Member> accepted;
        for (auto member : members)
            if (Player* player = Original(member))
            {
                if (!player->InSamePhase(actor)) continue;
                // Persistent native objective proves the actual death even if
                // logout cancels presentation. Never force CompleteQuest.
                if (!player->GetQuestObjectiveData(member.quest, 103661)) player->KilledMonsterCredit(103661);
                if (!player->GetQuestObjectiveData(member.quest, 103661))
                {
                    TC_LOG_WARN("scripts", "Malgalor finale: native103661 refused after real death; no finale teleport");
                    continue;
                }
                if (!Victory(scenario)) player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46881);
                if (!Victory(scenario))
                {
                    TC_LOG_WARN("scripts", "Malgalor finale: native46881 not accepted after real death; no finale teleport");
                    continue;
                }
                member.newAuras = AuraMask(player);
                accepted.push_back(member);
            }
        if (accepted.empty()) return;
        Player* owner = nullptr;
        for (auto const& member : accepted)
            if (member.player == primary) owner = Original(member);
        if (!owner) owner = Original(accepted.front());
        if (owner && owner->CastSpell(owner, 191979) != SPELL_CAST_OK)
            TC_LOG_WARN("scripts", "Malgalor finale: native191979 presentation refused; real victory retained");
        for (auto member : accepted)
            if (Player* player = Original(member))
            {
                member.newAuras = AuraMask(player) & ~member.newAuras;
                if (player == owner)
                    player->AddQuestDelayedEvent(member.quest, 4000, [member]
                    {
                        if (Player* original = Original(member)) original->CreateConversation(826);
                    });
                // Deliberately not quest-bound: its owned-aura cleanup must
                // still run after abandonment; Original gates the teleport.
                player->AddDelayedEvent(8000, [member] { Depart(member); });
            }
    }
}
