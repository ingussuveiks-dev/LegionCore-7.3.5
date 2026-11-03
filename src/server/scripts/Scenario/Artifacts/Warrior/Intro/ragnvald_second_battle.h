#pragma once

#include "ritual_transition.h"
#include "Timer.h"

namespace RagnvaldSecondBattle
{
    // Optional NPC speech follows the already-earned ritual handoff. Its
    // instance owner never repeats battle, casts, criteria or quest rewards.
    struct State
    {
        explicit State(Map* owner) : map(owner) { }
        Map* map;
        WarriorRitual::State const* handoff = nullptr; // same owning instance, destroyed after this State
        Scenario* origin = nullptr; // identity only, always resolved again
        ObjectGuid source, death;
        std::vector<MalgalorFinale::Member> members;
        uint32 began = 0, triedAt = 0;
        bool started = false, accepted = false, busy = false, ended = false;
        bool retrying = false, warned = false;

        bool Leaf(Scenario* scenario, uint32 id) const
        {
            auto progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
            auto found = progress->find(id);
            return found != progress->end() && found->second.Counter;
        }

        Player* Retained(MalgalorFinale::Member const& member) const
        {
            auto player = ObjectAccessor::FindPlayer(member.player);
            auto status = player ? player->getQuestStatus(member.quest) : nullptr;
            auto attempt = member.attempt.lock();
            return player && status && attempt && status->ScriptLifetime == attempt && player->GetMap() == map &&
                player->getClass() == CLASS_WARRIOR && MalgalorFinale::Quest(player) == member.quest &&
                player->GetQuestStatus(member.quest) == QUEST_STATUS_INCOMPLETE && !player->GetQuestObjectiveData(member.quest, 103661) ? player : nullptr;
        }

        bool Valid() const
        {
            if (!map || map->IsMapUnload() || map->GetId() != 1500 || !handoff || !handoff->resumed || handoff->pending ||
                handoff->lastDeath != death || handoff->ragnvald != source) return false;
            auto scenario = sScenarioMgr->GetScenario(map->GetInstanceId());
            return scenario && scenario == origin && scenario->GetScenarioId() == 933 &&
                scenario->GetCurrentStep() >= 3 && scenario->GetCurrentStep() <= 4 &&
                Leaf(scenario, 46912) && Leaf(scenario, 46929) && std::any_of(members.begin(), members.end(),
                    [this](MalgalorFinale::Member const& member) { return Retained(member) != nullptr; });
        }

        Creature* Actor() const
        {
            auto actor = map ? map->GetCreature(source) : nullptr;
            return actor && actor->GetGUID() == source && actor->GetMap() == map && actor->GetEntry() == 97167 && actor->GetDBTableGUIDLow() == 361094 &&
                actor->IsInWorld() && !actor->IsPreDelete() && actor->IsAlive() ? actor : nullptr;
        }

        bool Contact(Creature* actor) const
        {
            for (auto const& member : members)
                if (auto player = Retained(member))
                    if (player->IsInWorld() && player->CanContact() && !player->isGameMaster() && !player->HasPendingTeleport() &&
                        player->InSamePhase(actor) && !actor->ShouldHideFor(member.player) && player->canSeeOrDetect(actor) &&
                        actor->IsWithinDistInMap(player, actor->GetVisibilityRange())) return true;
            return false;
        }

        void Begin(WarriorRitual::State const& ritual, ObjectGuid guid)
        {
            // Only the actual instance's consumed two-spirit handoff can
            // transfer its original participants, before their vector clears.
            auto scenario = ritual.Progress();
            if (started || ritual.map != map || !ritual.resumed || ritual.pending || !ritual.departed ||
                guid != ritual.ragnvald || ritual.lastDeath.IsEmpty() || ritual.members.empty() ||
                ritual.spirits[0].IsEmpty() || ritual.spirits[1].IsEmpty() || !scenario ||
                scenario->GetCurrentStep() != 3 || !ritual.Leaf(46912) || !ritual.Leaf(46929)) return;
            source = guid; death = ritual.lastDeath; members = ritual.members; origin = scenario; handoff = &ritual;
            began = getMSTime(); started = true;
            // The instance updates this immediately after the original battle
            // handoff. Creation cannot reenter and retire its borrowed AI.
        }

        static void Discard(Conversation* conversation)
        {
            if (conversation->IsInWorld()) conversation->Remove();
            // Native AddToMap can return true after AddToWorld refused its
            // caster. The grid then owns this object even without world entry.
            else conversation->GetMap()->RemoveFromMap(conversation, true);
        }

        void Update()
        {
            if (!started || busy || accepted || ended) return;
            if (!Valid()) { ended = true; return; }
            if (GetMSTimeDiffToNow(began) >= 120000)
            {
                ended = true;
                TC_LOG_WARN("scripts", "Warrior second battle: original1412 conversation not accepted within120s; combat is not replayed");
                return;
            }
            if (retrying && GetMSTimeDiffToNow(triedAt) < 2000) return;
            auto actor = Actor();
            if (!actor || !Contact(actor)) return;
            struct Dispatch { bool& flag; explicit Dispatch(bool& value) : flag(value) { flag = true; } ~Dispatch() { flag = false; } } dispatch(busy);
            retrying = true; triedAt = getMSTime();
            auto conversation = new Conversation;
            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 1412, actor, nullptr, *actor))
            {
                delete conversation;
                if (!warned) TC_LOG_WARN("scripts", "Warrior second battle: native1412 factory refused; retaining original participant speech retry without replaying combat");
                warned = true; return;
            }
            // Native creation is external/reentrant. Re-resolve the same full
            // actor and original weak attempts before accepting the receipt.
            auto speaker = Actor();
            if (!Valid() || !speaker || speaker != actor || !Contact(speaker) || conversation->GetMap() != map ||
                !conversation->IsInWorld() || ObjectAccessor::GetConversation(*speaker, conversation->GetGUID()) != conversation ||
                conversation->GetCreatures().size() != 1)
            {
                Discard(conversation);
                if (!warned) TC_LOG_WARN("scripts", "Warrior second battle: native1412 has no live original actor/participant receipt; retaining speech retry");
                warned = true; return;
            }
            auto participant = conversation->GetCreature(0);
            if (!participant || participant->GuidLow != source.GetLowPart() || participant->GuidHight != source.GetHighPart())
            {
                Discard(conversation);
                if (!warned) TC_LOG_WARN("scripts", "Warrior second battle: native1412 selected a different actor; retaining original full-GUID speech retry");
                warned = true; return;
            }
            accepted = true;
            // Keep native NPC visibility, admission window and full-viewer
            // cleanup. A later missing object never causes speech replay.
        }
    };
}
