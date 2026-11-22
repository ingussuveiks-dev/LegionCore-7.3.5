#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

void AddSC_ashenvale();

namespace
{
constexpr uint32 SiphonQuest = 13730;
constexpr uint32 Canister = 45478;
constexpr uint32 Siphon = 63565;
constexpr uint32 FelFire = 33736;
constexpr uint32 Draaka = 33726;

Quest const* LoadSiphonQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeFelfireCanister.inc"
    );
    Require(bool(row), "Missing Playing With Felfire template");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[SiphonQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(SiphonQuest, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=13730");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetInt32() == FelFire && objective->Fetch()[5].GetInt32() == 7, "Missing seven-fire objective");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=13730");
    Require(bool(addon), "Missing original canister provision");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    Require(quest->SourceItemId == Canister && quest->SourceItemIdCount == 1, "Quest must supply one canister");
    Require(bool(WorldDatabase.Query("SELECT 1 FROM creature c JOIN creature_queststarter s ON s.id=c.id "
        "JOIN creature_questender e ON e.id=c.id AND e.quest=s.quest WHERE s.quest=13730 AND c.id=33726 AND c.map=1 AND c.zoneId=331")),
        "Draaka must have a placed offer and turn-in in Ashenvale");
    auto spell = sSpellMgr->GetSpellInfo(Siphon);
    Require(spell && spell->GetEffect(EFFECT_0)->ApplyAuraName == SPELL_AURA_PERIODIC_TRIGGER_SPELL &&
        spell->GetEffect(EFFECT_0)->TriggerSpell == 63566, "Missing the real periodic capture and credit spell");
    auto creditSpell = sSpellMgr->GetSpellInfo(63566);
    Require(creditSpell, "Missing native 63566");
    std::cout << "CASE capture channeled=" << spell->IsChanneled() << " duration=" << spell->GetDuration()
        << " range=" << spell->GetMaxRange(spell->IsPositive())
        << " credit-caster=" << creditSpell->NeedsToBeTriggeredByCaster(spell, DIFFICULTY_NONE) << std::endl;
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_ashenvale();
    sScriptMgr->SwapScriptContext(true);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE siphon_conditions AS SELECT * FROM conditions "
        "WHERE SourceTypeOrReferenceId IN (13,17) AND SourceEntry=63565");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM siphon_conditions");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE siphon_script AS SELECT * FROM spell_script_names WHERE spell_id=63565");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM siphon_script");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE siphon_questender AS SELECT * FROM creature_questender WHERE quest=13730");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM siphon_questender");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sScriptDataStore->LoadSpellScriptNames();
    sScriptDataStore->ValidateSpellScripts();
    sConditionMgr->LoadConditions();
    Require(sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL, Siphon).size() == 2 &&
        spell->GetEffect(EFFECT_0)->ImplicitTargetConditions && spell->GetEffect(EFFECT_0)->ImplicitTargetConditions->size() == 2,
        "Native conditional admission did not attach both cast and target gates");
    return quest;
}

void CheckSiphon(Quest const* quest)
{
    auto rows = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature WHERE id=33736 "
        "AND map=1 AND zoneId=331 ORDER BY guid");
    Require(rows && rows->GetRowCount() == 16, "Missing sixteen actual Fel Fire placements");
    std::vector<std::pair<uint32, Position>> sources;
    do
    {
        auto row = rows->Fetch();
        sources.emplace_back(row[0].GetUInt32(), Position{row[1].GetFloat(),row[2].GetFloat(),row[3].GetFloat(),row[4].GetFloat()});
    } while (rows->NextRow());
    Map map(1, 300, 0, DIFFICULTY_NONE);
    auto const& point = sources.front().second;
    LootPlayer owner(SupplySession(), map, 13730, point), other(SupplySession(), map, 13731, point);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_ORC);
        player->setFactionForRace(RACE_ORC);
        player->InitDisplayIds();
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    auto cast = [](LootPlayer& player)
    {
        auto item = player.GetItemByEntry(Canister);
        Require(item, "Expected a real carried canister");
        return player.CastSpell(&player, Siphon, false, item);
    };
    GiveSupply(other, Canister, 1);
    Require(cast(other) != SPELL_CAST_OK, "A retained canister started casting without the quest");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    Require(owner.GetItemCount(Canister) == 1 && !owner.CanRewardQuest(quest, false), "Acceptance must supply one canister with zero credit");
    Actor unrelated(&map, Draaka, 60001, point);
    Require(map.AddToMap(static_cast<Creature*>(&unrelated)), "Could not publish an unrelated valid unit");
    cast(owner);
    Tick(owner, 6000);
    Require(!unrelated.HasAura(Siphon) && !owner.GetQuestObjectiveData(SiphonQuest, FelFire),
        "An unrelated unit received the siphon or awarded credit");
    auto pump = [&](Actor& fire, uint32 milliseconds)
    {
        for (uint32 elapsed = 0; elapsed < milliseconds; elapsed += 10)
        {
            owner.Unit::Update(10);
            fire.Unit::Update(10);
        }
    };
    {
        Actor fire(&map, FelFire, sources.front().first, point);
        Require(map.AddToMap(static_cast<Creature*>(&fire)), "Could not publish the fire for cancellation checks");
        Require(cast(owner) == SPELL_CAST_OK, "Initial channel did not start");
        pump(fire, 1000);
        Require(fire.HasAura(Siphon, owner.GetGUID()), "Initial channel did not anchor on the actual fire");
        owner.InterruptSpell(CURRENT_CHANNELED_SPELL, false);
        pump(fire, 6000);
        Require(!owner.GetQuestObjectiveData(SiphonQuest, FelFire), "Interrupted capture gave credit");
        Require(cast(owner) == SPELL_CAST_OK, "Could not begin the abandon check");
        pump(fire, 1000);
        Require(fire.HasAura(Siphon, owner.GetGUID()), "Abandon case did not begin an actual channel");
        AbandonSupply(owner, SiphonQuest);
        Require(!owner.GetItemCount(Canister), "Abandon left the real canister in the bag");
        owner.AddQuestAndCheckCompletion(quest, nullptr);
        pump(fire, 6000);
        Require(!owner.GetQuestObjectiveData(SiphonQuest, FelFire), "A previous acceptance credited the new attempt");
    }
    for (uint32 index = 0; index < 7; ++index)
    {
        auto const& placement = sources[index];
        SupplyMove(owner, placement.second);
        Actor fire(&map, FelFire, placement.first, placement.second);
        Require(map.AddToMap(static_cast<Creature*>(&fire)), "Could not publish the original Fel Fire creature");
        Require(cast(owner) == SPELL_CAST_OK, "The real canister refused a placed living fire");
        pump(fire, 2500);
        Require(fire.HasAura(Siphon, owner.GetGUID()) && owner.GetQuestObjectiveData(SiphonQuest, FelFire) == index,
            "Capture had no actual target or advanced before the native tick");
        pump(fire, 4000);
        auto credit = owner.GetQuestObjectiveData(SiphonQuest, FelFire);
        std::cout << "CASE capture " << (index + 1) << " credit=" << credit << " aura=" << fire.HasAura(Siphon) << std::endl;
        Require(credit == index + 1 && !other.GetQuestObjectiveData(SiphonQuest, FelFire),
            "A full capture did not give exactly one credit to the actual owner");
    }
    Require(owner.GetItemCount(Canister) == 1 && owner.CanRewardQuest(quest, false), "Seven fires did not admit the actual turn-in");
    Require(cast(owner) != SPELL_CAST_OK, "A completed objective still allowed another siphon");
    auto giver = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature WHERE id=33726 "
        "AND map=1 AND zoneId=331");
    Require(giver && giver->GetRowCount() == 1, "Missing the original Draaka placement");
    auto row = giver->Fetch();
    Position receiverPoint{row[1].GetFloat(),row[2].GetFloat(),row[3].GetFloat(),row[4].GetFloat()};
    SupplyMove(owner, receiverPoint);
    Actor receiver(&map, Draaka, row[0].GetUInt32(), receiverPoint);
    Require(map.AddToMap(static_cast<Creature*>(&receiver)), "Could not publish Draaka at the real turn-in location");
    uint32 reward = 0;
    if (auto choices = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (auto entry : *choices)
            if (owner.CanSelectQuestPackageItem(entry))
            {
                reward = entry->ItemID;
                break;
            }
    Require(reward && owner.CanRewardQuest(quest, reward, false), "The actual DB2 reward package had no eligible choice");
    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = receiver.GetGUID();
    packet.QuestID = SiphonQuest;
    packet.ItemChoiceID = reward;
    owner.GetSession()->HandleQuestGiverChooseReward(packet);
    Require(owner.GetQuestRewardStatus(SiphonQuest) && owner.GetItemCount(reward) && !owner.GetItemCount(Canister),
        "Turn-in did not award the selected item and consume the supplied canister");
    other.AddQuestAndCheckCompletion(quest, nullptr);
    AbandonSupply(other, SiphonQuest);
    Require(!other.GetItemCount(Canister), "Abandon left the canister in the bag");
    std::cout << "PASS native supply, wrong unit and unaccepted gates, seven periodic captures, reward and abandon cleanup\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The canister test requires the isolated character schema");
        auto quest = LoadSiphonQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckSiphon(quest);
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
