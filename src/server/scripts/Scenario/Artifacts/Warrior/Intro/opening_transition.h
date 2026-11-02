#pragma once

namespace WarriorOpening
{
    constexpr uint32 AIStatusData = 93320, RequestData = 93321, StartedData = 93322, ClaimData = 93323, SourceData = 93324;
    constexpr int32 StartAction = -93320;
    constexpr uint32 Idle = 1, Queued = 2;

    struct State
    {
        explicit State(Map* owner) : map(owner) { }
        Map* map;
        ObjectGuid source;
        MalgalorFinale::Member original;
        uint32 epoch = 1, retry = 0;
        bool pending = false, greeted = false, spoken = false, started = false, sawStepOne = false;
        bool greetingWarned = false, openingWarned = false, aiWarned = false;

        Scenario* Progress() const
        {
            if (!map || map->IsMapUnload() || map->GetId() != 1500) return nullptr;
            Scenario* scenario = sScenarioMgr->GetScenario(map->GetInstanceId());
            return scenario && scenario->GetScenarioId() == 933 ? scenario : nullptr;
        }

        bool Admitted() const
        {
            Scenario* scenario = Progress();
            if (!scenario || scenario->GetCurrentStep() != 1) return false;
            auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
            auto found = progress->find(45769);
            return found != progress->end() && found->second.Counter;
        }

        Creature* Source() const
        {
            Creature* actor = map ? map->GetCreature(source) : nullptr;
            return actor && actor->GetMap() == map && actor->IsInWorld() && !actor->IsPreDelete() && actor->IsAlive() &&
                actor->GetEntry() == 97167 && actor->GetDBTableGUIDLow() == 361094 ? actor : nullptr;
        }

        void Retire()
        {
            if (!++epoch) ++epoch;
            pending = spoken = false;
            original = {};
        }

        void Observe(Creature* actor)
        {
            if (!actor || actor->GetMap() != map || actor->GetEntry() != 97167 || actor->GetDBTableGUIDLow() != 361094) return;
            if (source != actor->GetGUID() && !started) Retire();
            source = actor->GetGUID();
        }

        bool Retained() const
        {
            Player* player = ObjectAccessor::FindPlayer(original.player);
            auto status = player ? player->getQuestStatus(original.quest) : nullptr;
            auto attempt = original.attempt.lock();
            return player && status && attempt && status->ScriptLifetime == attempt && player->GetMap() == map &&
                player->getClass() == CLASS_WARRIOR && MalgalorFinale::Quest(player) == original.quest &&
                player->GetQuestStatus(original.quest) == QUEST_STATUS_INCOMPLETE && !player->GetQuestObjectiveData(original.quest, 103661);
        }

        Player* Current(bool alive = true) const
        {
            Player* player = MalgalorFinale::Original(original);
            Creature* actor = Source();
            return Retained() && player && actor && !player->HasPendingTeleport() && (!alive || player->IsAlive()) &&
                player->InSamePhase(actor) ? player : nullptr;
        }

        Player* Approach(Creature* actor, float range) const
        {
            CreatureAI* ai = actor->AI();
            if (!ai || !actor->IsAIEnabled || actor->HasUnitState(UNIT_STATE_SIGHTLESS) ||
                (!actor->HasReactState(REACT_AGGRESSIVE) && !ai->CanSeeEvenInPassiveMode())) return nullptr;
            for (auto const& reference : map->GetPlayers())
                if (Player* player = reference.getSource())
                {
                    uint32 quest = MalgalorFinale::Quest(player);
                    if (quest && player->GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE && player->getClass() == CLASS_WARRIOR &&
                        !player->GetQuestObjectiveData(quest, 103661) && player->IsInWorld() && player->IsAlive() && player->CanContact() &&
                        !player->isGameMaster() && !player->HasPendingTeleport() && player->GetMap() == map && player->InSamePhase(actor) &&
                        !player->isInFlight() && actor->canSeeOrDetect(player, false, true) && actor->IsWithinDistInMap(player, range)) return player;
                }
            return nullptr;
        }

        bool Speak(Player* player, uint32 id)
        {
            Conversation* conversation = new Conversation;
            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), id, player, nullptr, *player))
            { delete conversation; return false; }
            if (!conversation->IsInWorld() || ObjectAccessor::GetConversation(*player, conversation->GetGUID()) != conversation)
            { conversation->Remove(); return false; }
            if (id == 1386)
            {
                if (conversation->GetCreatures().size() != 2)
                { conversation->Remove(); return false; }
                auto first = conversation->GetCreature(0);
                auto second = conversation->GetCreature(1);
                Creature* actor = Source();
                Creature* ally = actor ? actor->FindNearestCreature(97166, 100.0f, true) : nullptr;
                if (!actor || !ally || ally->GetDBTableGUIDLow() != 361091 || ally->GetMap() != map ||
                    !ally->IsInWorld() || ally->IsPreDelete() || !player->canSeeOrDetect(ally, false, true) ||
                    !first || !second || first->GuidLow != ally->GetGUID().GetLowPart() || first->GuidHight != ally->GetGUID().GetHighPart() ||
                    second->GuidLow != source.GetLowPart() || second->GuidHight != source.GetHighPart())
                { conversation->Remove(); return false; }
            }
            // Keep the native Player/group/caster visibility and finite Map
            // lifetime. Do not bind a shared dialogue to one member's quest.
            return true;
        }

        uint32 Request() const
        {
            return pending && spoken && !started && Admitted() && Current(false) ? epoch : 0;
        }

        void Claim(uint32 ticket)
        {
            if (ticket && Request() == ticket)
            { started = true; pending = false; }
        }

        void Greet(Creature* actor)
        {
            if (!greeted)
                if (Player* player = Approach(actor, 40.0f))
                {
                    greeted = Speak(player, 770);
                    if (!greeted && !greetingWarned) TC_LOG_WARN("scripts", "Warrior opening: native770 failed; retaining greeting retry");
                    greetingWarned = !greeted;
                }
        }

        void Update(uint32 diff)
        {
            Scenario* scenario = Progress();
            if (!scenario || scenario->GetCurrentStep() > 1)
            { if (pending) Retire(); return; }
            if (!scenario->GetCurrentStep() && sawStepOne)
            { Retire(); greeted = started = sawStepOne = false; }
            if (scenario->GetCurrentStep() == 1) sawStepOne = true;
            if (pending && !Retained()) Retire();
            if (retry > diff) { retry -= diff; return; }
            retry = 1000;
            Creature* actor = Source();
            if (!actor) return;
            if (started) { Greet(actor); return; }
            CreatureAI* ai = actor->AI();
            uint32 status = ai && actor->IsAIEnabled ? ai->GetData(AIStatusData) : 0;
            if (status != Idle && status != Queued)
            {
                if (!aiWarned) TC_LOG_WARN("scripts", "Warrior opening: original361094 has no ready Ragnvald AI; retaining native admission retry");
                aiWarned = true; return;
            }
            aiWarned = false;
            if (!pending)
                if (Player* player = Approach(actor, 15.0f))
                {
                    uint32 quest = MalgalorFinale::Quest(player);
                    player->AddQuestDelayedEvent(quest, 0, [] { });
                    auto entry = player->getQuestStatus(quest);
                    if (entry && entry->ScriptLifetime)
                    {
                        Retire(); pending = true;
                        original = {player->GetGUID(), quest, map->GetInstanceId(), 0, entry->ScriptLifetime};
                    }
                }
            if (Player* player = pending ? Current() : nullptr)
            {
                if (!scenario->GetCurrentStep()) player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46877);
                player = Current();
                if (player && Admitted())
                {
                    sawStepOne = true;
                    if (!spoken) spoken = Speak(player, 1386);
                    if (spoken && Request())
                    {
                        actor = Source(); ai = actor ? actor->AI() : nullptr;
                        if (ai && actor->IsAIEnabled && ai->GetData(AIStatusData) == Idle) ai->DoAction(StartAction);
                    }
                }
                bool failed = !Admitted() || !spoken;
                if (failed && !openingWarned) TC_LOG_WARN("scripts", "Warrior opening: native46877 or original1386 not acknowledged; no battle, retaining original quest retry");
                openingWarned = failed;
            }
            // Preserve the original first-arrival order at15yd:1386 precedes
            // independent cosmetic770. A failed greeting never blocks battle.
            actor = Source();
            if (actor) Greet(actor);
        }
    };
}
