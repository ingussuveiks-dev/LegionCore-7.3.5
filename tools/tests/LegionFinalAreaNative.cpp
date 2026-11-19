#define STORY_DIALOGUE_SUPPORT_ONLY
#include "LegionStoryDialogueNative.cpp"
#include "SmartAI.h"

namespace
{
constexpr char Entries[] = "197,367,501,533";

struct ObservedReceiver : SmartAI
{
    using SmartAI::SmartAI;
    unsigned DeliveryHits = 0;

    void SpellHit(Unit* unit, SpellInfo const* spell) override
    {
        if (spell->Id == 206157)
            ++DeliveryHits;
        SmartAI::SpellHit(unit, spell);
    }
};

void ShadowIndexed(char const* table, std::string const& filter)
{
    std::string name = table;
    std::string seed = "final_area_seed_" + name;
    WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE " + seed + " LIKE " + name).c_str());
    WorldDatabase.DirectExecute(("INSERT INTO " + seed + " SELECT * FROM " + name + " WHERE " + filter).c_str());
    WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE " + name + " LIKE " + seed).c_str());
    WorldDatabase.DirectExecute(("INSERT INTO " + name + " SELECT * FROM " + seed).c_str());
}

void LoadAreaData()
{
    std::unordered_map<uint32, std::vector<uint32>> outdoor{{1220, {}}};
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe(outdoor);
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe(outdoor);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    ShadowIndexed("smart_scripts", std::string("(source_type IN(10,13) AND entryorguid IN(") + Entries +
        ")) OR (source_type=0 AND entryorguid=104208)");
    ShadowIndexed("conditions", std::string("(SourceTypeOrReferenceId=22 AND SourceId IN(10,13) AND SourceEntry IN(")
        + Entries + ")) OR (SourceTypeOrReferenceId=13 AND SourceEntry=206157)");
    ShadowIndexed("spell_area", "spell=206158");
    auto scripts = WorldDatabase.Query("SELECT COUNT(*),SUM(source_type<>13 OR target_type<>7 OR event_chance<>100 OR event_flags<>0) FROM smart_scripts WHERE entryorguid<>104208");
    Require(scripts && scripts->Fetch()[0].GetUInt32() == 8 && !scripts->Fetch()[1].GetUInt32(),
        "Unexpected action graph in remaining area roots");
#ifdef FINAL_AREA_BASELINE
    auto original = WorldDatabase.Query("SELECT COUNT(*),MIN(SourceId),MAX(SourceId) FROM conditions WHERE SourceTypeOrReferenceId=22");
    Require(original && original->Fetch()[0].GetUInt32() == 10 && original->Fetch()[1].GetUInt32() == 10 &&
        original->Fetch()[2].GetUInt32() == 10, "Baseline requires original wrong-source data and library");
#else
    auto path = std::getenv("LEGION_AREA_DIALOGUE_MIGRATION");
    Require(path, "Explicit remaining-area migration required");
    for (unsigned replay = 0; replay < 2; ++replay)
    {
        std::ifstream input(path);
        Require(bool(input), "Remaining-area migration unavailable");
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
    auto repaired = WorldDatabase.Query("SELECT COUNT(*),MIN(SourceId),MAX(SourceId) FROM conditions WHERE SourceTypeOrReferenceId=22");
    Require(repaired && repaired->Fetch()[0].GetUInt32() == 12 && repaired->Fetch()[1].GetUInt32() == 13 &&
        repaired->Fetch()[2].GetUInt32() == 13, "Two migration replays did not preserve the exact new gate set");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_SmartScripts();
    sScriptMgr->SwapScriptContext(true);
    sEventObjectDataStore->LoadEventObjectTemplates();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sConditionMgr->LoadConditions();
    sConversationDataStore->LoadConversationData();
    sSpellMgr->LoadSpellAreas();
    HashMapHolder<EventObject>::SetSize(1000000);
    HashMapHolder<Conversation>::SetSize(20000);
}

QuestObjective const& Objective(uint32 quest, uint32 credit)
{
    auto q = sQuestDataStore->GetQuestTemplate(quest);
    Require(q && !q->HasFlag(QUEST_FLAGS_TRACKING), "Expected a manually rewarded quest");
    for (auto const& obj : q->Objectives)
        if (obj.Type == QUEST_OBJECTIVE_MONSTER && obj.ObjectID == int32(credit))
            return obj;
    throw std::runtime_error("Expected authored objective");
}

void Seed(DungeonPlayer& player, uint32 quest, uint32 credit)
{
    auto q = sQuestDataStore->GetQuestTemplate(quest);
    auto& objective = Objective(quest, credit);
    player.SetQuestObjectiveData(q, &objective, objective.Amount);
}

void Accept(DungeonPlayer& player, uint32 quest)
{
    player.SetQuestStatus(quest, QUEST_STATUS_INCOMPLETE);
    player.SetQuestSlot(0, quest);
}

struct StagedRoute
{
    uint32 Event, Quest, Credit, Prerequisite, Other, Conversation, ExtraConversation;
};

constexpr StagedRoute Routes[] = {
    {197, 43686, 112623, 109923, 111147, 3237, 3216},
    {501, 44041, 112488, 112486, 0, 3477, 0}
};

void CompletePrereqs(DungeonPlayer& player, StagedRoute const& route)
{
    Seed(player, route.Quest, route.Prerequisite);
    if (route.Other)
        Seed(player, route.Quest, route.Other);
    else
        Require(player.AddAura(1784, &player), "Native Stealth aura unavailable");
}

std::vector<std::unique_ptr<StoryActor>> PopulateActors(Map& map, DungeonPlayer& player, uint32 conversation)
{
    std::vector<std::unique_ptr<StoryActor>> actors;
    if (auto data = sConversationDataStore->GetConversationCreature(conversation))
        for (auto const& actor : *data)
            if (actor.creatureId)
                actors.push_back(std::make_unique<StoryActor>(map, player, actor.creatureId, 180500 + unsigned(actors.size())));
    return actors;
}

unsigned VerifyStagedCredit()
{
    unsigned cases = 0;
    for (auto const& route : Routes)
    for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
    {
        Map map(1220, 300, 0, DIFFICULTY_NONE);
        DungeonPlayer player(map, race);
        Accept(player, route.Quest);
        auto actors = PopulateActors(map, player, route.Conversation);
        AreaDialogueEvent event(map, player, route.Event);
        event.Update(1);
#ifdef FINAL_AREA_BASELINE
        Require(player.GetQuestObjectiveData(route.Quest, route.Credit) == 1 && !LiveConversations().empty(),
            "Original wrong-source progression bypass did not reproduce");
        ClearConversations(map);
#else
        Require(!player.GetQuestObjectiveData(route.Quest, route.Credit) && !event.ActivatedForPlayer(&player),
            "Premature area entry advanced or consumed the quest stage");
        Seed(player, route.Quest, route.Prerequisite);
        event.Update(1);
        Require(!event.ActivatedForPlayer(&player), "Second prerequisite was ignored");
        CompletePrereqs(player, route);
        event.Update(1);
        auto conversations = LiveConversations();
        std::set<uint32> published;
        for (auto conversation : conversations)
        {
            Require(conversation->GetCasterGUID() == player.GetGUID() && conversation->IsInWorld(),
                "Original conversation lost its entering player");
            published.insert(conversation->GetEntry());
        }
        std::set<uint32> expected{route.Conversation};
        if (route.ExtraConversation)
            expected.insert(route.ExtraConversation);
        Require(published == expected && published.size() == conversations.size() &&
            player.GetQuestObjectiveData(route.Quest, route.Credit) == 1, "Ready player missed credit or authored dialogue");
        event.Update(1);
        Require(LiveConversations().size() == expected.size(), "Repeated pulse duplicated the dialogue");
        ClearConversations(map);
        auto position = player.GetPosition();
        player.Relocate(position.GetPositionX() + 20, position.GetPositionY(), position.GetPositionZ(), 0);
        event.Update(1);
        player.Relocate(position);
        event.Update(1);
        Require(LiveConversations().empty() && player.GetQuestObjectiveData(route.Quest, route.Credit) == 1,
            "Re-entry repeated a completed proximity objective");
#endif
        Require(player.getRewardedQuests().empty(), "Area credit granted quest reward");
        ++cases;
    }
    return cases;
}

unsigned VerifyParty()
{
    unsigned cases = 0;
    for (auto const& route : Routes)
    for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
    for (unsigned mode : {0u, 1u, 2u, 3u, 4u})
    {
        Map map(1220, 300, 0, DIFFICULTY_NONE);
        DungeonPlayer player(map, race);
        DungeonPlayer member(map, race, 102);
        Accept(player, route.Quest);
        CompletePrereqs(player, route);
        if (mode != 4)
            Accept(member, route.Quest);
        if (mode == 1 || mode == 2 || mode == 3)
            CompletePrereqs(member, route);
        if (mode == 2)
            member.Relocate(member.GetPositionX() + 10 + sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE),
                member.GetPositionY(), member.GetPositionZ(), 0);
        if (mode == 3)
            member.SetPhaseMask(2, false);
        AreaDialogueEvent event(map, player, route.Event);
        Group party;
        player.SetGroup(&party, 0);
        member.SetGroup(&party, 0);
        sScriptMgr->OnEventObject(&player, &event, true);
        member.SetGroup(nullptr);
        player.SetGroup(nullptr);
#ifdef FINAL_AREA_BASELINE
        bool credited = mode == 0 || mode == 1 || mode == 3;
#else
        bool credited = mode == 1;
#endif
        Require(player.GetQuestObjectiveData(route.Quest, route.Credit) == 1 &&
            member.GetQuestObjectiveData(route.Quest, route.Credit) == unsigned(credited),
            "Native staged party-credit eligibility differs");
        ClearConversations(map);
        ++cases;
    }
    return cases;
}

unsigned VerifyBlackRook()
{
    unsigned cases = 0;
    for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
    for (auto state : {QUEST_STATUS_NONE, QUEST_STATUS_INCOMPLETE, QUEST_STATUS_COMPLETE,
                       QUEST_STATUS_REWARDED, QUEST_STATUS_FAILED})
    for (bool marked : {false, true})
    {
        Map map(1220, 300, 0, DIFFICULTY_NONE);
        DungeonPlayer player(map, race);
        player.SetQuestStatus(44789, state);
        if (marked)
            Seed(player, 44789, 117311);
        AreaDialogueEvent event(map, player, 367);
        event.SetPhaseId({7515}, false);
        event.Update(1);
        Require(LiveConversations().empty(), "Black Rook dialogue leaked through its authored phase");
        player.SetPhaseId({7515}, false);
        event.Update(1);
#ifdef FINAL_AREA_BASELINE
        bool expected = true;
#else
        bool expected = state == QUEST_STATUS_INCOMPLETE && !marked;
#endif
        auto conversations = LiveConversations();
        Require(conversations.size() == unsigned(expected), "Black Rook shared-phase admission differs");
        if (expected)
            Require(conversations.front()->GetEntry() == 3958 &&
                conversations.front()->GetCasterGUID() == player.GetGUID(), "Wrong Black Rook conversation");
        Require(player.GetQuestStatus(44789) == state && player.GetQuestObjectiveData(44789, 117311) == unsigned(marked),
            "Dialogue altered the independent Black Rook progress");
        ClearConversations(map);
        ++cases;
    }
    return cases;
}

unsigned VerifyButton()
{
    auto overrideData = sOverrideSpellDataStore.LookupEntry(679);
    Require(overrideData && std::find(std::begin(overrideData->Spells), std::end(overrideData->Spells), 206157)
        != std::end(overrideData->Spells), "Native stone delivery override changed");
    auto spell = sSpellMgr->GetSpellInfo(206157);
    Require(spell && spell->GetEffect(EFFECT_1)->Effect == SPELL_EFFECT_KILL_CREDIT &&
        spell->GetEffect(EFFECT_1)->MiscValue == 104180, "Native stone delivery credit changed");
    unsigned cases = 0;
    for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
    for (unsigned mode : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u})
    {
        Map map(1220, 300, 0, DIFFICULTY_NONE);
        DungeonPlayer player(map, race);
        map.PlayerRelocation(&player, -681.903f, 4504.58f, 728.776f, 0.f);
        player.GetZoneAndAreaId(player.m_zoneId, player.m_areaId);
        Require(player.GetCurrentZoneID() == 7502 || player.GetCurrentAreaID() == 7502,
            "Delivery fixture did not enter the matching native zone");
        player.SetPhaseId({6112}, false);
        if (mode)
            Accept(player, 41397);
        if (mode == 6)
            Seed(player, 41397, 104180);
        AreaDialogueEvent event(map, player, 533);
        event.SetPhaseId({6112}, false);
        StoryActor speaker(map, player, 104201, 180501);
        speaker.SetPhaseId({6112}, false);
        event.Update(1);
        Require(!player.HasAura(206158), "Delivery button applied before its delayed cast");
        if (mode == 2)
            player.RemoveActiveQuest(41397);
        if (mode == 3)
            player.Relocate(-701.f, 4504.58f, 728.776f, 0.f);
        if (mode == 4)
            player.SetPhaseId({}, false);
        if (mode == 5)
            player.SetQuestStatus(41397, QUEST_STATUS_COMPLETE);
        player.Unit::Update(300);
#ifdef FINAL_AREA_BASELINE
        bool expected = true;
#else
        bool expected = mode == 1 || mode == 7;
#endif
        Require(player.HasAura(206158) == expected, "Delayed delivery button admission differs");
        if (expected)
            Require(player.GetUInt16Value(PLAYER_FIELD_BYTES_5, PLAYER_BYTES_2_OVERRIDE_SPELLS_UINT16_OFFSET) == 679,
                "Actual native override did not publish");
        Require(player.GetQuestObjectiveData(41397, 104180) == unsigned(mode == 6),
            "Button installation awarded a delivery");
#ifndef FINAL_AREA_BASELINE
        if (mode == 1)
        {
            player.Relocate(-677.846f, 4510.83f, 731.503f, 0.f);
            event.Update(1);
            Require(player.HasAura(206158) && !event.ActivatedForPlayer(&player),
                "Short walk to the delivery receiver removed the button");
            player.RemoveActiveQuest(41397);
            Require(!player.HasAura(206158), "Abandonment left the delivery override installed");
        }
        if (mode == 3)
        {
            ClearConversations(map);
            event.Update(1);
            player.Relocate(-681.903f, 4504.58f, 728.776f, 0.f);
            event.Update(1);
            player.Unit::Update(300);
            Require(player.HasAura(206158), "Re-entering after a canceled delivery cast did not restore the button");
        }
        if (mode == 7)
        {
            ClearConversations(map);
            player.CastSpell(&player, 206157, false);
            for (unsigned i = 0; i < 20; ++i)
                player.Unit::Update(100);
            Require(!player.GetQuestObjectiveData(41397, 104180) && player.HasAura(206158),
                "Missing native recipient consumed the button or awarded delivery");

            StoryActor receiver(map, player, 104208, 180502);
            receiver.Relocate(-677.846f, 4510.83f, 731.503f, 0.f);
            receiver.SetPhaseId({6112}, false);
            auto receiverAI = new ObservedReceiver(&receiver);
            Require(receiver.AIM_Initialize(receiverAI), "Native receiver SmartAI failed");
            player.Relocate(-677.846f, 4510.83f, 731.503f, 0.f);
            event.Update(1);
            Require(!event.ActivatedForPlayer(&player), "Walk to receiver did not leave the trigger");
            Require(player.CastSpell(&player, 206157, false) == SPELL_CAST_OK, "Native delivery cast failed");
            for (unsigned i = 0; i < 20; ++i)
                player.Unit::Update(100);
            Require(player.GetQuestObjectiveData(41397, 104180) == 1 && !player.HasAura(206158),
                "Real delivery did not credit exactly once and remove its override");
            auto conversations = LiveConversations();
            std::cout << "DELIVERY receiver_hits=" << receiverAI->DeliveryHits
                      << " conversations=" << conversations.size()
                      << " speaker_visible=" << bool(player.FindNearestCreature(104201, player.GetVisibilityRange()))
                      << " receiver_visible=" << bool(player.FindNearestCreature(104208, player.GetVisibilityRange()))
                      << std::endl;
            Require(receiverAI->DeliveryHits == 1 && conversations.size() == 1 && conversations.front()->GetEntry() == 1473 &&
                conversations.front()->GetCasterGUID() == player.GetGUID(), "The real receiver missed its delivery dialogue");
            Require(player.getRewardedQuests().empty(), "Delivery granted quest reward");

            player.Relocate(-681.903f, 4504.58f, 728.776f, 0.f);
            event.Update(1);
            player.Unit::Update(300);
            Require(!player.HasAura(206158) && player.GetQuestObjectiveData(41397, 104180) == 1,
                "Delivery area re-entry restored a button after credit");
        }
#endif
        ClearConversations(map);
        ++cases;
    }
    return cases;
}
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
        auto overrideData = sOverrideSpellDataStore.LookupEntry(679);
        Require(overrideData, "Missing 679 override metadata");
        std::set<uint32> spellIds{1784u, 206158u, 229844u};
        for (uint32 id : overrideData->Spells)
            if (id)
                spellIds.insert(id);
        for (uint32 id : spellIds)
        {
            auto spell = sSpellMgr->GetSpellInfo(id);
            auto record = sSpellStore.LookupEntry(id);
            Require(spell && record, "Expected a native spell record");
            std::cout << "SPELL " << id << ' ' << record->Name->Str[0]
                      << " time=" << spell->CalcCastTime() << " duration=" << spell->GetDuration() << '\n';
            for (unsigned i = 0; i < MAX_SPELL_EFFECTS; ++i)
                if (auto e = spell->GetEffect(SpellEffIndex(i)); e && e->Effect)
                    std::cout << "EFFECT " << id << ' ' << i << ' ' << e->Effect << ' '
                              << e->ApplyAuraName << ' ' << e->MiscValue << ' '
                              << e->TriggerSpell << ' ' << e->TargetA.GetTarget() << ' '
                              << e->TargetB.GetTarget() << '\n';
        }
        for (uint32 id : {43686u, 44789u, 44041u, 41397u})
        {
            auto quest = sQuestDataStore->GetQuestTemplate(id);
            Require(quest, "Expected a native quest");
            std::cout << "QUEST " << id << " class=" << quest->AllowableClasses
                      << " race=" << quest->AllowableRaces << " flags=" << quest->Flags << '\n';
        }
        LoadAreaData();
        auto credit = VerifyStagedCredit();
        auto party = VerifyParty();
        auto dialogue = VerifyBlackRook();
        auto button = VerifyButton();
#ifdef FINAL_AREA_BASELINE
        std::cout << "COUNTERFACTUAL ";
#else
        std::cout << "PASS ";
#endif
        std::cout << credit << " staged credit, " << party << " party, "
                  << dialogue << " Black Rook and " << button << " delivery cases\n";
        CharacterDatabase.Close();
        LoginDatabase.Close();
        WorldDatabase.Close();
        HotfixDatabase.Close();
        MySQL::Library_End();
        return 0;
    }
    catch (std::exception const& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
