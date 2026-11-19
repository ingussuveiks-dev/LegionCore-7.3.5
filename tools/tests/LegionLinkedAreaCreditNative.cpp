// Real outdoor SmartEvent conditions, linked conversations and native credit.
#define STORY_DIALOGUE_SUPPORT_ONLY
#include "LegionStoryDialogueNative.cpp"

namespace
{
struct Route
{
    uint32 Event, Quest, Credit, Conversation;
};
constexpr Route Routes[] = {
    {199, 43958, 112029, 3423},
    {234, 46107, 118968, 4504},
    {236, 45796, 118047, 4325},
    {250, 42224, 107446, 2306},
    {322, 45065, 116020, 4023},
    {323, 45065,      0, 4024}
};
constexpr char Entries[] = "199,234,236,250,322,323";

void SetAttempt(DungeonPlayer& player, Route const& route, QuestStatus status)
{
    player.SetQuestStatus(route.Quest, status);
    if (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE)
        player.SetQuestSlot(0, route.Quest);
}

void ExpectConversation(DungeonPlayer& player, Route const& route, bool expected)
{
    auto conversations = LiveConversations();
    Require(conversations.size() == unsigned(expected), "Unexpected conversation publication");
    if (expected)
        Require(conversations.front()->GetEntry() == route.Conversation &&
            conversations.front()->GetCasterGUID() == player.GetGUID() &&
            ObjectAccessor::GetConversation(player, conversations.front()->GetGUID()) == conversations.front(),
            "Conversation lost its actual entering player");
}

void LoadLinkedRoutes()
{
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    ShadowDialogue("smart_scripts", std::string("source_type IN(10,13) AND entryorguid IN(") + Entries + ")");
    ShadowDialogue("conditions", std::string("SourceTypeOrReferenceId=22 AND SourceId IN(10,13) AND SourceEntry IN(") + Entries + ")");
    auto rows = WorldDatabase.Query("SELECT COUNT(*),SUM(source_type<>13 OR event_flags<>0 OR event_chance<>100 OR target_type<>7) FROM smart_scripts");
    Require(rows && rows->Fetch()[0].GetUInt32() == 11 && !rows->Fetch()[1].GetUInt32(), "Unexpected linked area graph");
    auto missingLink = WorldDatabase.Query("SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=323 AND id<>0");
    Require(missingLink && !missingLink->Fetch()[0].GetUInt32(), "EO323 has acquired a real continuation");
#ifdef LINKED_AREA_BASELINE
    // Isolate the wrong-source counterfactual. The separate live graph
    // inspection establishes EO323's dangling link before deployment.
    WorldDatabase.DirectExecute("UPDATE conditions SET SourceId=10");
    WorldDatabase.DirectExecute("UPDATE smart_scripts SET link=0 WHERE entryorguid=323 AND id=0");
#else
    auto path = std::getenv("LEGION_AREA_DIALOGUE_MIGRATION");
    Require(path, "Explicit linked area migration required");
    for (unsigned replay = 0; replay < 2; ++replay)
    {
        std::ifstream input(path);
        Require(bool(input), "Linked area migration unavailable");
        std::string line, statement;
        while (std::getline(input, line))
        {
            if (line.rfind("--", 0) == 0)
                continue;
            statement += line + '\n';
            if (!line.empty() && line.back() == ';')
            {
                WorldDatabase.DirectExecute(statement.c_str());
                statement.clear();
            }
        }
    }
#endif
    auto gates = WorldDatabase.Query("SELECT COUNT(*),MIN(SourceId),MAX(SourceId) FROM conditions");
#ifdef LINKED_AREA_BASELINE
    constexpr uint32 source = 10;
#else
    constexpr uint32 source = 13;
#endif
    Require(gates && gates->Fetch()[0].GetUInt32() == 7 && gates->Fetch()[1].GetUInt32() == source &&
        gates->Fetch()[2].GetUInt32() == source, "Linked area conditions failed their native source binding");
    auto finalRoot = WorldDatabase.Query("SELECT link FROM smart_scripts WHERE entryorguid=323 AND id=0 AND action_type=206 AND action_param1=4024");
    Require(finalRoot && !finalRoot->Fetch()[0].GetUInt32(), "EO323 dangling link remains");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_SmartScripts();
    sScriptMgr->SwapScriptContext(true);
    sEventObjectDataStore->LoadEventObjectTemplates();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sConditionMgr->LoadConditions();
    sConversationDataStore->LoadConversationData();
    HashMapHolder<EventObject>::SetSize(1000);
    HashMapHolder<Conversation>::SetSize(20000);
}

unsigned VerifyRoutes()
{
    unsigned cases = 0;
    for (auto const& route : Routes)
    {
        auto quest = sQuestDataStore->GetQuestTemplate(route.Quest);
        Require(quest && quest->Type == QUEST_TYPE_NORMAL && !quest->HasFlag(QUEST_FLAGS_TRACKING),
            "Expected an ordinary manually rewarded quest");
        auto objective = std::find_if(quest->Objectives.begin(), quest->Objectives.end(),
            [&](QuestObjective const& value) { return value.ObjectID == int32(route.Credit); });
        if (route.Credit)
            Require(objective != quest->Objectives.end() && objective->Type == QUEST_OBJECTIVE_MONSTER &&
                objective->Amount == 1 && !objective->Bugged, "Native credit objective changed");
        for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
        for (auto status : {QUEST_STATUS_NONE, QUEST_STATUS_INCOMPLETE, QUEST_STATUS_COMPLETE,
                           QUEST_STATUS_REWARDED, QUEST_STATUS_FAILED})
        {
            try
            {
                Map map(1220, 300, 0, DIFFICULTY_NONE);
                DungeonPlayer player(map, race);
                SetAttempt(player, route, status);
                std::vector<std::unique_ptr<StoryActor>> actors;
                std::set<uint32> publishedActors;
                if (!sConversationDataStore->GetConversationActor(route.Conversation))
                    if (auto data = sConversationDataStore->GetConversationCreature(route.Conversation))
                        for (auto const& actor : *data)
                            if (actor.creatureId && publishedActors.insert(actor.creatureId).second)
                                actors.push_back(std::make_unique<StoryActor>(map, player, actor.creatureId, 180500 + unsigned(actors.size())));
                AreaDialogueEvent event(map, player, route.Event);
                auto origin = player.GetPosition();
                player.SetPhaseMask(2, false);
                event.Update(1);
                ExpectConversation(player, route, false);
                player.SetPhaseMask(1, false);
#ifdef LINKED_AREA_BASELINE
                bool expected = true;
#else
                bool expected = status == (route.Credit ? QUEST_STATUS_INCOMPLETE : QUEST_STATUS_COMPLETE);
#endif
                event.Update(1);
                ExpectConversation(player, route, expected);
                event.Update(1);
                ExpectConversation(player, route, expected);
                ClearConversations(map);
                if (route.Credit)
                    Require(player.GetQuestObjectiveData(quest, objective->StorageIndex) == unsigned(status == QUEST_STATUS_INCOMPLETE),
                        "Native linked credit did not respect the actual quest log");
                Require(player.getRewardedQuests().empty(), "Area event rewarded a quest");
                player.Relocate(origin.GetPositionX() + event.eventTemplate->radius + 10, origin.GetPositionY(), origin.GetPositionZ(), 0);
                event.Update(1);
                Require(!event.ActivatedForPlayer(&player), "Area exit failed to release entry receipt");
                player.Relocate(origin);
                event.Update(1);
#ifndef LINKED_AREA_BASELINE
                if (route.Event == 199)
                    expected = false; // Authored gate requires the credit to be absent.
                else if (route.Credit)
                    expected = player.GetQuestStatus(route.Quest) == QUEST_STATUS_INCOMPLETE;
#endif
                ExpectConversation(player, route, expected);
                ClearConversations(map);
                if (route.Credit)
                    Require(player.GetQuestObjectiveData(quest, objective->StorageIndex) == unsigned(status == QUEST_STATUS_INCOMPLETE),
                        "Area re-entry duplicated linked credit");
                ++cases;
            }
            catch (...)
            {
                std::cerr << "Route " << route.Event << " race " << unsigned(race) << " state " << unsigned(status) << '\n';
                throw;
            }
        }
    }
    return cases;
}

#ifndef LINKED_AREA_BASELINE
unsigned VerifyGroupCredit()
{
    unsigned cases = 0;
    for (auto const& route : Routes)
    {
        if (!route.Credit)
            continue;
        for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
        for (unsigned memberState : {0u, 1u, 2u})
        {
            Map map(1220, 300, 0, DIFFICULTY_NONE);
            DungeonPlayer player(map, race);
            DungeonPlayer member(map, race, 102);
            auto origin = player.GetPosition();
            if (memberState == 2)
                member.Relocate(origin.GetPositionX() + 10 + sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE),
                    origin.GetPositionY(), origin.GetPositionZ(), 0);
            SetAttempt(player, route, QUEST_STATUS_INCOMPLETE);
            SetAttempt(member, route, memberState ? QUEST_STATUS_INCOMPLETE : QUEST_STATUS_NONE);
            AreaDialogueEvent event(map, player, route.Event);
            Group party;
            player.SetGroup(&party, 0);
            member.SetGroup(&party, 0);
            // Invoke the registered callback once for the entering player.
            // Native group iteration must deliver the same eligible credits.
            bool handled = sScriptMgr->OnEventObject(&player, &event, true);
            member.SetGroup(nullptr);
            player.SetGroup(nullptr);
            Require(handled && player.GetQuestObjectiveData(route.Quest, route.Credit) == 1 &&
                member.GetQuestObjectiveData(route.Quest, route.Credit) == unsigned(memberState == 1),
                "Native area credit changed party eligibility");
            // EO199 deliberately starts with its actual credit and can
            // finish it even when its presentation actor is unavailable.
            ExpectConversation(player, route, route.Event != 199);
            ClearConversations(map);
            ++cases;
        }
    }
    return cases;
}

void VerifyCompletionHandoff()
{
    Route const& arrival = Routes[4];
    Route const& completed = Routes[5];
    for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
    for (bool completionTriggerFirst : {false, true})
    {
        Map map(1220, 300, 0, DIFFICULTY_NONE);
        DungeonPlayer player(map, race);
        auto quest = sQuestDataStore->GetQuestTemplate(arrival.Quest);
        SetAttempt(player, arrival, QUEST_STATUS_INCOMPLETE);
        for (auto const& objective : quest->Objectives)
            if (objective.ObjectID != int32(arrival.Credit))
                player.SetQuestObjectiveData(quest, &objective, objective.Amount);
        AreaDialogueEvent arrivalEvent(map, player, arrival.Event);
        AreaDialogueEvent completedEvent(map, player, completed.Event);
        auto origin = player.GetPosition();
        if (completionTriggerFirst)
        {
            completedEvent.Update(1);
            ExpectConversation(player, completed, false);
#ifdef LINKED_AREA_COMPLETION_BASELINE
            Require(completedEvent.ActivatedForPlayer(&player), "Original premature entry consumption did not reproduce");
#else
            Require(!completedEvent.ActivatedForPlayer(&player), "Incomplete quest consumed the completion trigger");
#endif
        }
        arrivalEvent.Update(1);
        ExpectConversation(player, arrival, true);
        Require(player.GetQuestStatus(arrival.Quest) == QUEST_STATUS_COMPLETE,
            "Native arrival credit did not complete the staffed quest log");
        ClearConversations(map);
        completedEvent.Update(1);
#ifdef LINKED_AREA_COMPLETION_BASELINE
        if (completionTriggerFirst)
        {
            ExpectConversation(player, completed, false);
            player.Relocate(origin.GetPositionX() + completedEvent.eventTemplate->radius + 10,
                origin.GetPositionY(), origin.GetPositionZ(), 0);
            completedEvent.Update(1);
            player.Relocate(origin);
            completedEvent.Update(1);
        }
#endif
        ExpectConversation(player, completed, true);
        Require(player.GetQuestObjectiveData(arrival.Quest, arrival.Credit) == 1 && player.getRewardedQuests().empty(),
            "Completion introduction changed progress or granted rewards");
        completedEvent.Update(1);
        ExpectConversation(player, completed, true);
        ClearConversations(map);
        completedEvent.Update(1);
        ExpectConversation(player, completed, false);

        // A new entrance retains the original complete-quest gate.
        player.Relocate(origin.GetPositionX() + completedEvent.eventTemplate->radius + 10,
            origin.GetPositionY(), origin.GetPositionZ(), 0);
        completedEvent.Update(1);
        player.Relocate(origin);
        completedEvent.Update(1);
        ExpectConversation(player, completed, true);
        ClearConversations(map);
    }
}
#endif
}

int main(int argc, char** argv)
{
    try
    {
        InitializeDungeon(argc, argv, true);
        DisableMgr::LoadDisables();
        sObjectMgr->LoadItemTemplates();
        sObjectMgr->LoadItemTemplateAddon();
        sQuestDataStore->LoadQuests();
        std::unordered_map<uint32, std::vector<uint32>> outdoor{{1220, {}}};
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe(outdoor);
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe(outdoor);
        LoadLinkedRoutes();
        auto count = VerifyRoutes();
#ifdef LINKED_AREA_BASELINE
        std::cout << "COUNTERFACTUAL " << count << " original wrong-source cases with isolated EO323 link; disallowed stages publish dialogue\n";
#else
        std::cout << "PASS " << count << " native linked area cases with original quest gates, objective caps and entry/re-entry\n";
        std::cout << "PASS " << VerifyGroupCredit() << " native party credit cases with nearby, absent-quest and distant members\n";
        VerifyCompletionHandoff();
#ifdef LINKED_AREA_COMPLETION_BASELINE
        std::cout << "COUNTERFACTUAL EO323 consumes an incomplete player's entry before EO322 credit, requiring re-entry in both affected cases\n";
#else
        std::cout << "PASS 4 native EO322 to EO323 handoffs in both orders, with no duplicate inside and ordinary re-entry\n";
#endif
#endif
        CharacterDatabase.Close();
        LoginDatabase.Close();
        WorldDatabase.Close();
        HotfixDatabase.Close();
        MySQL::Library_End();
        return 0;
    }
    catch (std::exception const& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
