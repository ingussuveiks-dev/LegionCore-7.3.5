#pragma once

namespace WarriorFirstWave
{
    // The actual instance death owns this receipt. Danica's summon/AI and a
    // nearest-player query cannot establish who earned the shared scenario.
    struct State
    {
        explicit State(Map* owner) : map(owner) { }
        Map* map;
        ObjectGuid source;
        std::vector<MalgalorFinale::Member> members;
        uint32 retry = 0;
        bool pending = false, creditWarned = false;

        Scenario* Progress() const
        {
            if (!map || map->IsMapUnload() || map->GetId() != 1500) return nullptr;
            Scenario* scenario = sScenarioMgr->GetScenario(map->GetInstanceId());
            return scenario && scenario->GetScenarioId() == 933 ? scenario : nullptr;
        }

        bool Leaf(uint32 id) const
        {
            Scenario* scenario = Progress();
            if (!scenario) return false;
            auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
            auto found = progress->find(id);
            return found != progress->end() && found->second.Counter;
        }

        Player* Current(MalgalorFinale::Member const& member) const
        {
            Player* player = MalgalorFinale::Original(member);
            if (!player || player->GetMap() != map || player->HasPendingTeleport() ||
                player->GetQuestStatus(member.quest) != QUEST_STATUS_INCOMPLETE ||
                player->GetQuestObjectiveData(member.quest, 103661)) return nullptr;
            // A removed corpse must not erase a genuine death. Retain phase
            // admission while the original actor is still present.
            Creature* actor = map->GetCreature(source);
            return !actor || !actor->IsInWorld() || actor->IsPreDelete() || player->InSamePhase(actor) ? player : nullptr;
        }

        bool Retained(MalgalorFinale::Member const& member) const
        {
            Player* player = ObjectAccessor::FindPlayer(member.player);
            auto status = player ? player->getQuestStatus(member.quest) : nullptr;
            auto attempt = member.attempt.lock();
            return player && status && attempt && status->ScriptLifetime == attempt &&
                player->GetMap() == map && player->getClass() == CLASS_WARRIOR &&
                MalgalorFinale::Quest(player) == member.quest && player->GetQuestStatus(member.quest) == QUEST_STATUS_INCOMPLETE &&
                !player->GetQuestObjectiveData(member.quest, 103661);
        }

        void Death(Creature* actor, Unit* killer)
        {
            Scenario* scenario = Progress();
            if (pending || !scenario || scenario->GetCurrentStep() != 1 || !actor ||
                actor->GetMap() != map || !actor->IsInWorld() || actor->IsPreDelete() || actor->IsAlive() ||
                actor->GetEntry() != 102970 || actor->GetDBTableGUIDLow() != 361138 ||
                actor->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1)) return;
            members.clear();
            for (auto const& reference : map->GetPlayers())
                if (Player* player = reference.getSource())
                {
                    uint32 quest = MalgalorFinale::Quest(player);
                    if (!quest || player->GetQuestStatus(quest) != QUEST_STATUS_INCOMPLETE ||
                        player->getClass() != CLASS_WARRIOR || player->isGameMaster() || !player->IsInWorld() ||
                        !player->CanContact() || player->HasPendingTeleport() || player->GetMap() != map ||
                        !player->InSamePhase(actor) || player->GetQuestObjectiveData(quest, 103661)) continue;
                    player->AddQuestDelayedEvent(quest, 0, [] { });
                    auto status = player->getQuestStatus(quest);
                    if (status && status->ScriptLifetime)
                        members.push_back({player->GetGUID(), quest, actor->GetInstanceId(), 0, status->ScriptLifetime});
                }
            if (members.empty()) return;
            Player* owner = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            if (owner)
                for (auto member = members.begin(); member != members.end(); ++member)
                    if (member->player == owner->GetGUID()) { std::iter_swap(members.begin(), member); break; }
            source = actor->GetGUID(); retry = 0; creditWarned = false; pending = true;
            Update(0);
        }

        void Update(uint32 diff)
        {
            if (!pending) return;
            Scenario* scenario = Progress();
            if (!scenario || scenario->GetCurrentStep() != 1 ||
                std::none_of(members.begin(), members.end(), [this](MalgalorFinale::Member const& member) { return Retained(member); }))
            { pending = false; members.clear(); return; }
            if (retry > diff) { retry -= diff; return; }
            retry = 2000;
            for (auto const& member : members)
                if (Player* player = Current(member))
                {
                    if (!Leaf(45771)) player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46878);
                    // Re-resolve after the external event, retaining the exact
                    // quest attempt and the native current-tree restriction.
                    scenario = Progress(); player = Current(member);
                    if (scenario && scenario->GetCurrentStep() == 1 && player && Leaf(45771) && !Leaf(46175))
                        player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 47474);
                    if (Leaf(45771) && Leaf(46175))
                    { pending = false; members.clear(); return; }
                }
            if (std::none_of(members.begin(), members.end(), [this](MalgalorFinale::Member const& member) { return Retained(member); }))
            { pending = false; members.clear(); return; }
            if (!creditWarned)
                TC_LOG_WARN("scripts", "Warrior first wave: real361138 death46878/47474 not acknowledged; retaining original quest attempts for retry");
            creditWarned = true;
        }
    };
}
