#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

namespace
{
constexpr uint32 Language = 9538;
constexpr uint32 NextTotem = 9539;
constexpr uint32 Primer = 23818;
constexpr uint32 Study = 30419;
constexpr uint32 Aurren = 17232;
constexpr uint32 Totem = 17360;

Quest const* LoadLanguageQuest()
{
    auto rows = WorldDatabase.Query(
#include "NativeStillpinePrimer.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "Missing the original primer and next-totem quests");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto addons = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN(9538,9539) ORDER BY ID");
    Require(addons && addons->GetRowCount() == 2 && addons->Fetch()[5].GetUInt32() == NextTotem,
        "Missing the original next-totem relation");
    do
    {
        auto fields = addons->Fetch();
        quests[fields[0].GetUInt32()]->LoadQuestTemplateAddon(fields);
    } while (addons->NextRow());
    auto quest = quests[Language];
    Require(quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT) && quest->GetObjectives().empty() &&
        quest->SourceItemId == Primer && quest->SourceItemIdCount == 1 &&
        !WorldDatabase.Query("SELECT ID FROM quest_objectives WHERE QuestID=9538 LIMIT 1"),
        "Missing the native objective-free event or supplied primer");
    auto spell = sSpellMgr->GetSpellInfo(Study);
    Require(spell && spell->GetEffect(EFFECT_0)->Effect == SPELL_EFFECT_QUEST_COMPLETE &&
        spell->GetEffect(EFFECT_0)->MiscValue == Language && spell->GetEffect(EFFECT_0)->TargetA.GetTarget() == TARGET_UNIT_CASTER,
        "Native Study must complete only the caster's event");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE primer_conditions AS SELECT * FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=30419");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM primer_conditions");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE primer_ends AS SELECT * FROM creature_questender WHERE quest IN(9538,9539)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM primer_ends");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE primer_starts AS SELECT * FROM creature_queststarter WHERE quest IN(9538,9539)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM primer_starts");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sQuestDataStore->LoadCreatureQuestRelations();
    auto gates = WorldDatabase.Query("SELECT COUNT(*) FROM conditions");
    Require(bool(gates), "Could not read the scoped conditions");
    if (gates->Fetch()[0].GetUInt32())
        sConditionMgr->LoadConditions();
    return quest;
}

void CheckPrimer(Quest const* quest, bool baseline)
{
    auto placement = [](uint32 entry)
    {
        auto row = WorldDatabase.PQuery("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
            "WHERE id=%u AND map=530 AND zoneId=3524", entry);
        Require(row && row->GetRowCount() == 1, "Expected a placed Azuremyst actor");
        auto f = row->Fetch();
        return std::make_pair(f[0].GetUInt32(), Position{f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat(),f[4].GetFloat()});
    };
    auto giver = placement(Aurren), receiver = placement(Totem);
    Map map(530, 300, 0, DIFFICULTY_NONE);
    Actor aurren(&map, Aurren, giver.first, giver.second), coo(&map, Totem, receiver.first, receiver.second);
    Require(map.AddToMap(static_cast<Creature*>(&aurren)) && map.AddToMap(static_cast<Creature*>(&coo)), "Could not publish the original quest actors");
    Require(aurren.hasQuest(Language) && coo.hasInvolvedQuest(Language) && coo.hasQuest(NextTotem), "Offer or original totem transition is missing");
    LootPlayer owner(SupplySession(), map, 9538, giver.second), other(SupplySession(), map, 9539, giver.second);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_DRAENEI);
        player->setFactionForRace(RACE_DRAENEI);
        player->InitDisplayIds();
        player->SetLevel(10);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 10000);
    }
    auto study = [](LootPlayer& player, Unit& target)
    {
        auto book = player.GetItemByEntry(Primer);
        Require(book, "Expected an actual carried primer");
        return player.CastSpell(&target, Study, false, book);
    };
    uint32 const duration = sSpellMgr->GetSpellInfo(Study)->CalcCastTime(&owner) + 2000;
    std::cout << "CASE study cast window=" << duration << std::endl;
    GiveSupply(other, Primer, 1);
    auto noQuest = study(other, other);
    Tick(other, duration);
    if (baseline)
    {
        Require(noQuest == SPELL_CAST_OK && !other.GetItemCount(Primer) && other.GetQuestStatus(Language) == QUEST_STATUS_NONE,
            "The old native cast must reproduce loss of an unaccepted primer");
        std::cout << "EXPECTED_BROKEN inactive native self-study consumed a held primer\n";
        return;
    }
    Require(noQuest != SPELL_CAST_OK && other.GetItemCount(Primer) == 1, "Inactive study consumed a retained primer");
    other.DestroyItemCount(Primer, 1, true);
    other.AddQuestAndCheckCompletion(quest, &aurren);
    owner.AddQuestAndCheckCompletion(quest, &aurren);
    Require(owner.GetItemCount(Primer) == 1 && !owner.CanRewardQuest(quest, false), "Acceptance did not supply one primer and wait for studying");
    auto spell = study(owner, other);
    Require(spell == SPELL_CAST_OK, "The real primer self-study did not start");
    Tick(owner, duration);
    std::cout << "CASE native study owner books=" << owner.GetItemCount(Primer) << " status=" << unsigned(owner.GetQuestStatus(Language))
        << " other books=" << other.GetItemCount(Primer) << " status=" << unsigned(other.GetQuestStatus(Language)) << std::endl;
    Require(!owner.GetItemCount(Primer) && owner.CanRewardQuest(quest, false) &&
        other.GetItemCount(Primer) == 1 && !other.CanRewardQuest(quest, false), "Study did not stay on its own caster and consume one primer");
    AbandonSupply(other, Language);
    Require(!other.GetItemCount(Primer), "Abandon left the unread primer in the bag");
    SupplyMove(owner, receiver.second);
    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = coo.GetGUID();
    packet.QuestID = Language;
    packet.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(packet);
    auto next = owner.GetNextQuest(coo.GetGUID(), quest);
    Require(owner.GetQuestRewardStatus(Language) && !owner.GetItemCount(Primer) &&
        next && next->Id == NextTotem, "Totem of Coo did not complete the study and offer its next quest");
    std::cout << "PASS native study on its own caster, one consumed primer, abandon and real Totem of Coo handoff\n";
}
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(CrashTrace);
    try
    {
        InitializeNative(argc, argv);
        opcodeTable.Initialize();
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{530, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{530, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The primer test requires the isolated character schema");
        auto quest = LoadLanguageQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
#ifdef STILLPINE_PRIMER_BASELINE
        constexpr bool baseline = true;
#else
        constexpr bool baseline = false;
#endif
        auto gates = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL, Study);
        Require(gates.size() == (baseline ? 0 : 2), "Primer condition migration does not match the test mode");
        CheckPrimer(quest, baseline);
        CharacterDatabase.Close();
        LoginDatabase.Close();
        HotfixDatabase.Close();
        WorldDatabase.Close();
        return 0;
    }
    catch (std::exception const& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
