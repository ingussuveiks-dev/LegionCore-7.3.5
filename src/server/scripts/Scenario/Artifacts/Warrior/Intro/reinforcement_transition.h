#pragma once

namespace WarriorReinforcement
{
    // Unit::CreateConversation is void in this core. Keep its native creation
    // and ownership path while retaining the real accepted/failed result.
    inline bool Speak(Unit* speaker, uint32 id)
    {
        Conversation* conversation = new Conversation;
        if (conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), id, speaker, nullptr, *speaker))
            return true;
        delete conversation;
        return false;
    }
    struct Participant
    {
        MalgalorFinale::Member original;
        bool dialogue = false, scene = false, sceneWarned = false, dialogueWarned = false;
        uint32 retry = 0, dialogueRetry = 0;
    };

    // Owned and updated by the actual instance, not by a corpse/borrowed AI.
    struct State
    {
        explicit State(Map* owner) : map(owner) { }
        Map* map;
        ObjectGuid source, boss;
        std::vector<Participant> members;
        uint64 elapsed = 0, introAt = 0;
        uint32 retry = 0, bossRetry = 0, corpseRetry = 0;
        bool pending = false, introduced = false, spoken = false, creditWarned = false, actorWarned = false;

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

        void Observe(Creature* actor)
        {
            if (actor && actor->GetMap() == map && actor->GetEntry() == 92608 && actor->GetDBTableGUIDLow() == 361167)
                boss = actor->GetGUID();
        }

        Creature* Boss() const
        {
            Creature* actor = map ? map->GetCreature(boss) : nullptr;
            return actor && actor->IsInWorld() && !actor->IsPreDelete() && actor->IsAlive() && actor->GetMap() == map &&
                actor->GetEntry() == 92608 && actor->GetDBTableGUIDLow() == 361167 ? actor : nullptr;
        }

        Player* Current(Participant const& participant, bool phase = true) const
        {
            Player* player = MalgalorFinale::Original(participant.original);
            if (!player || player->GetMap() != map || player->HasPendingTeleport() ||
                player->GetQuestStatus(participant.original.quest) != QUEST_STATUS_INCOMPLETE) return nullptr;
            if (!phase) return player;
            Creature* anchor = map->GetCreature(source);
            if (!anchor || !anchor->IsInWorld() || anchor->IsPreDelete()) anchor = Boss();
            return anchor && player->InSamePhase(anchor) ? player : nullptr;
        }

        void Death(Creature* actor, Unit* killer)
        {
            Scenario* scenario = Progress();
            if (pending || !scenario || scenario->GetCurrentStep() != 3 || Leaf(45773) || !actor ||
                actor->GetMap() != map || !actor->IsInWorld() || actor->IsPreDelete() || actor->GetEntry() != 98257 ||
                actor->GetDBTableGUIDLow() != 361151 || actor->IsAlive() ||
                actor->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1)) return;
            members.clear();
            for (auto const& reference : map->GetPlayers())
                if (Player* player = reference.getSource())
                {
                    uint32 quest = MalgalorFinale::Quest(player);
                    if (!quest || player->GetQuestStatus(quest) != QUEST_STATUS_INCOMPLETE ||
                        player->getClass() != CLASS_WARRIOR || player->isGameMaster() || !player->IsInWorld() ||
                        !player->CanContact() || player->HasPendingTeleport() || player->GetMap() != map || !player->InSamePhase(actor)) continue;
                    player->AddQuestDelayedEvent(quest, 0, [] { });
                    auto status = player->getQuestStatus(quest);
                    if (status && status->ScriptLifetime)
                        members.push_back({{player->GetGUID(), quest, actor->GetInstanceId(), 0, status->ScriptLifetime}});
                }
            if (members.empty()) return;
            // Prefer the actual player/charmer/owner; native scenario criteria
            // still own the group update when an ally lands the killing blow.
            Player* owner = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            if (owner)
                for (auto found = members.begin(); found != members.end(); ++found)
                    if (found->original.player == owner->GetGUID()) { std::iter_swap(members.begin(), found); break; }
            source = actor->GetGUID(); elapsed = introAt = retry = bossRetry = corpseRetry = 0;
            pending = true; introduced = spoken = creditWarned = actorWarned = false;
            Update(0);
        }

        void Update(uint32 diff)
        {
            if (!pending) return;
            Scenario* scenario = Progress();
            if (!scenario || scenario->GetCurrentStep() < 3 || scenario->GetCurrentStep() > 4 || Leaf(45777) ||
                (!Leaf(45773) && std::none_of(members.begin(), members.end(),
                    [this](Participant const& member) { return Current(member, false) != nullptr; })))
            { pending = false; members.clear(); return; }
            elapsed += diff;
            if (!Leaf(45773))
            {
                if (retry > diff) { retry -= diff; return; }
                retry = 2000;
                if (scenario->GetCurrentStep() != 3) return;
                for (auto const& member : members)
                    if (Player* player = Current(member))
                    {
                        player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46880);
                        if (Leaf(45773)) break;
                    }
                if (!Leaf(45773))
                {
                    if (!creditWarned) TC_LOG_WARN("scripts", "Warrior reinforcement: real361151 death event46880 not acknowledged; retaining original attempt for retry");
                    creditWarned = true;
                    return;
                }
            }
            if (scenario->GetCurrentStep() != 4) return;
            Creature* actor = Boss();
            uint32 status = actor && actor->IsAIEnabled && actor->AI() ? actor->AI()->GetData(MalgalorFinale::IntroStartedData) : 0;
            bool starting = status == MalgalorFinale::IntroReady;
            if (starting)
            {
                if (bossRetry > diff) bossRetry -= diff;
                else { bossRetry = 2000; actor->AI()->DoAction(0); }
            }
            bool ready = actor && actor->IsAIEnabled && actor->AI() &&
                actor->AI()->GetData(MalgalorFinale::IntroStartedData) == MalgalorFinale::IntroStarted;
            if (ready && (!introduced || starting)) { introduced = true; introAt = elapsed; }
            if (!ready && !actorWarned)
                TC_LOG_WARN("scripts", "Warrior reinforcement: earned Malgalor361167 intro has no ready native AI; retaining retry");
            actorWarned = !ready;
            for (auto& member : members)
                if (Player* player = Current(member))
                {
                    if (!player->IsAlive()) continue;
                    if (!member.dialogue)
                    {
                        if (introduced && actor && !actor->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC)) member.dialogue = true;
                        else if (member.dialogueRetry > diff) member.dialogueRetry -= diff;
                        else
                        {
                            member.dialogueRetry = 2000;
                            member.dialogue = Speak(player, 771);
                            if (!member.dialogue && !member.dialogueWarned)
                                TC_LOG_WARN("scripts", "Warrior reinforcement: native771 conversation failed; retaining original attempt retry");
                            member.dialogueWarned = !member.dialogue;
                        }
                    }
                    if (member.scene || !introduced || elapsed - introAt < 8000 || !actor || !player->IsAlive() ||
                        !player->InSamePhase(actor) || !player->IsWithinDistInMap(actor, 300.0f)) continue;
                    // A late/rejoining participant must not be stunned and hide
                    // an already active combat boss for an obsolete cinematic.
                    if (!actor->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC)) { member.scene = true; continue; }
                    if (member.retry > diff) { member.retry -= diff; continue; }
                    member.retry = 2000;
                    if (!player->HasAura(197593, player->GetGUID())) player->CastSpell(player, 197593, true);
                    if (Current(member) && player->HasAura(197593, player->GetGUID())) member.scene = true;
                    else if (!member.sceneWarned)
                    {
                        member.sceneWarned = true;
                        TC_LOG_WARN("scripts", "Warrior reinforcement: native197593 arrival presentation did not apply; retaining original attempt retry");
                    }
                }
            if (!spoken && introduced && elapsed - introAt >= 24000)
            {
                if (corpseRetry > diff) { corpseRetry -= diff; return; }
                corpseRetry = 2000;
                if (Creature* corpse = map->GetCreature(source))
                    if (corpse->IsInWorld() && !corpse->IsPreDelete() && corpse->GetEntry() == 98257 && corpse->GetDBTableGUIDLow() == 361151)
                    {
                        spoken = Speak(corpse, 1414);
                        return;
                    }
                spoken = true; // Optional corpse speech cannot outlive its real actor.
            }
        }
    };
}
