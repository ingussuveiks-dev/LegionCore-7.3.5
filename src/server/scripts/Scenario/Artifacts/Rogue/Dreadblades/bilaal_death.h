#pragma once

namespace BilaalDeath
{
    struct Route
    {
        uint32 entry, spawn, scenario, quest, klass, phase, victory;
    };
    Route const priest{104502, 370522, 1065, 41631, CLASS_PRIEST, 6140, 47117};
    Route const mage{106708, 370551, 1101, 42011, CLASS_MAGE, 6068, 47995};

    Route const* Match(Creature* actor)
    {
        if (!actor || !actor->IsInWorld() || actor->IsAlive() || actor->IsPreDelete() ||
            actor->GetMapId() != 1583 || actor->HasQuestObjectScope() ||
            !actor->IsDamageEnoughForLootingAndReward()) return nullptr;
        for (Route const* route : {&priest, &mage})
            if (actor->GetEntry() == route->entry && actor->GetDBTableGUIDLow() == route->spawn &&
                actor->GetPhases().count(route->phase)) return route;
        return nullptr;
    }

    Scenario* Progress(Map* map, Route const* route)
    {
        Scenario* scenario = map ? sScenarioMgr->GetScenario(map->GetInstanceId()) : nullptr;
        return scenario && route && scenario->GetScenarioId() == route->scenario ? scenario : nullptr;
    }

    bool Victory(Scenario* scenario, Route const* route)
    {
        if (!scenario || !route) return false;
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(route->victory);
        return found != progress->end() && found->second.Counter;
    }

    bool Eligible(Player* player, Creature* actor, Route const* route)
    {
        if (!player || !actor || !route || !player->IsInWorld() || !player->CanContact() ||
            player->isGameMaster() || player->HasPendingTeleport() || player->getClass() != route->klass ||
            player->GetMap() != actor->GetMap() || !player->InSamePhase(actor) ||
            !player->IsAtGroupRewardDistance(actor) || player->GetQuestRewardStatus(route->quest)) return false;
        QuestStatus status = player->GetQuestStatus(route->quest);
        // Unit::Kill has already run native player/group kill rewards. Native
        // Accept Quest232939/232948 can also prefill this objective: the count
        // is admission, not provenance. Actual death, damage eligibility and
        // the real reward owner/group remain mandatory; never grant a kill.
        return (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE) &&
            player->getQuestStatus(route->quest) && player->GetQuestObjectiveData(route->quest, route->entry) > 0;
    }

    Player* Recipient(Creature* actor, Unit* killer, Route const* route)
    {
        Player* root = actor->GetLootRecipient();
        if (!root) root = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (Eligible(root, actor, route)) return root;
        if (root)
            if (Group* group = root->GetGroup())
                for (GroupReference* member = group->GetFirstMember(); member; member = member->next())
                    if (Eligible(member->getSource(), actor, route)) return member->getSource();
        return nullptr;
    }

    struct State
    {
        Route const* route = nullptr;
        ObjectGuid source, owner, killerGuid, receiver;
        std::weak_ptr<uint8> attempt;
        uint32 phaseMask = 0, timer = 0;
        std::set<uint32> phases;
        bool earned = false, done = false, trying = false, creditWarning = false, spawnWarning = false;

        Player* Original(Map* map)
        {
            if (!map || !route || attempt.expired()) return nullptr;
            for (auto const& reference : map->GetPlayers())
                if (Player* player = reference.getSource())
                    if (player->GetGUID() == owner)
                    {
                        QuestStatusData* quest = player->getQuestStatus(route->quest);
                        if (quest && quest->ScriptLifetime == attempt.lock()) return player;
                        return nullptr;
                    }
            return nullptr;
        }

        Creature* OriginalSource(Map* map)
        {
            Creature* actor = map ? map->GetCreature(source) : nullptr;
            return Match(actor) == route ? actor : nullptr;
        }

        bool CurrentReceipt(Player* player, Map* map)
        {
            if (!player || !player->IsInWorld() || !player->CanContact() || player->isGameMaster() ||
                player->HasPendingTeleport() || player->GetMap() != map || player->getClass() != route->klass ||
                !player->InSamePhase(phaseMask) || !player->InSamePhaseId(phases, false) ||
                player->GetQuestRewardStatus(route->quest)) return false;
            QuestStatus status = player->GetQuestStatus(route->quest);
            return (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE) &&
                player->GetQuestObjectiveData(route->quest, route->entry) > 0;
        }

        // Only this map-owned, properties-null Azuregos is created here. This
        // is the matching native factory sequence with a captured phase and
        // summoner GUID, so removal of the dead source cannot erase the earned
        // world transition. No borrowed Creature/AI is kept between updates.
        TempSummon* CreateReceiver(Map* map)
        {
            Creature* actor = OriginalSource(map);
            TempSummon* summon = new TempSummon(nullptr, actor, false);
            Position const pos{4219.38f, 7124.89f, 268.198f, 4.28748f};
            if (!summon->Create(sObjectMgr->GetGenerator<HighGuid::Creature>()->Generate(), map, phaseMask,
                104546, 0, 0, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation()))
            {
                delete summon;
                return nullptr;
            }
            summon->SetPhaseId(phases, false);
            summon->SetSummonerGUID(source);
            summon->SetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL, 0);
            summon->SetGuidValue(UNIT_FIELD_DEMON_CREATOR, source);
            summon->SetTargetGUID(ObjectGuid::Empty);
            summon->SetHomePosition(pos);
            summon->InitStats(0);
            if (!map->AddToMap(summon->ToCreature()))
            {
                summon->CleanupsBeforeDelete();
                delete summon;
                return nullptr;
            }
            summon->InitSummon();
            summon->SetTempSummonType(TEMPSUMMON_MANUAL_DESPAWN);
            return summon;
        }

        void Try(Map* map, Unit* immediateKiller = nullptr)
        {
            if (done || trying || !route || !map) return;
            trying = true;
            struct Release { bool& value; ~Release() { value = false; } } release{trying};
            Scenario* scenario = Progress(map, route);
            if (!scenario || scenario->GetCurrentStep() < 4 || scenario->GetCurrentStep() > 5)
            {
                done = true;
                return;
            }
            if (!earned)
            {
                if (attempt.expired()) { done = true; return; }
                Player* player = Original(map);
                // Eligibility/range were certified by the actual death. A
                // removed corpse must not erase that receipt while the same
                // accepted quest/player remains in the captured map/phase.
                if (!CurrentReceipt(player, map) || scenario->GetCurrentStep() != 4) return;
                // This real death's original accepted quest is the only credit
                // receipt. Never retry into a newly accepted quest or respawn.
                Unit* reference = immediateKiller && immediateKiller->GetGUID() == killerGuid ? immediateKiller :
                    ObjectAccessor::GetUnit(*player, killerGuid);
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49810, 0, 0, reference, true);
                scenario = Progress(map, route);
                earned = scenario && scenario->GetCurrentStep() == 5 && Victory(scenario, route);
                if (!earned)
                {
                    if (!creditWarning)
                    {
                        creditWarning = true;
                        TC_LOG_ERROR("scripts", "Bilaal: actual death event49810 was not accepted for entry%u; original attempt retained", route->entry);
                    }
                    return;
                }
            }
            if (scenario->GetCurrentStep() != 5 || !Victory(scenario, route)) { done = true; return; }
            // Keep the earlier104902/106747 Azuregos origin separate. A source
            // GUID and actual map registration identify this death's receiver.
            if (Creature* existing = map->GetCreature(receiver))
                if (existing->IsInWorld() && existing->IsAlive() && existing->GetEntry() == 104546 &&
                    existing->ToTempSummon() && existing->ToTempSummon()->GetSummonerGUID() == source)
                { done = true; return; }
            TempSummon* summon = CreateReceiver(map);
            if (!summon)
            {
                if (!spawnWarning)
                {
                    spawnWarning = true;
                    TC_LOG_ERROR("scripts", "Bilaal: earned Azuregos104546 creation failed for entry%u; retry retained", route->entry);
                }
                return;
            }
            receiver = summon->GetGUID();
            done = true;
            if (creditWarning || spawnWarning)
                TC_LOG_INFO("scripts", "Bilaal: recovered accepted death/Azuregos transition for entry%u", route->entry);
        }

        void Death(Map* map, Creature* actor, Unit* killer)
        {
            if (trying || !map || !actor || actor->GetMap() != map) return;
            if (route)
            {
                if (earned || !attempt.expired()) return;
                // Only another real CreatureDies can begin a new receipt. An
                // old timer never rebinds after abandonment/logout/retake.
                *this = State{};
            }
            Route const* candidate = Match(actor);
            Scenario* scenario = Progress(map, candidate);
            if (!candidate || !scenario || scenario->GetCurrentStep() != 4) return;
            Player* player = Recipient(actor, killer, candidate);
            if (!Eligible(player, actor, candidate)) return;
            route = candidate;
            source = actor->GetGUID(); owner = player->GetGUID();
            killerGuid = killer ? killer->GetGUID() : ObjectGuid::Empty;
            attempt = player->getQuestStatus(route->quest)->ScriptLifetime;
            phaseMask = actor->GetPhaseMask(); phases = actor->GetPhases();
            timer = 2000;
            Try(map, killer);
        }

        void Update(Map* map, uint32 diff)
        {
            if (done || !route) return;
            if (timer > diff) { timer -= diff; return; }
            timer = 2000;
            Try(map);
        }
    };
}
