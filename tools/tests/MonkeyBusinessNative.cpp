#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "SmartAI.h"
#include "SmartScriptMgr.h"

void AddSC_SmartScripts();

namespace
{
constexpr uint32 FeedingQuest = 14019;
constexpr uint32 Bananas = 49028;
constexpr uint32 FeedSpell = 67917;
constexpr uint32 Monkey = 34699;
constexpr uint32 FeedCredit = 35760;
constexpr uint32 Bamm = 35758;

Quest const* LoadFeedingQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeMonkeyBusiness.inc"
    );
    Require(bool(row), "Missing Monkey Business");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[FeedingQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(FeedingQuest, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=14019");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == FeedCredit && objective->Fetch()[5].GetUInt32() == 10,
        "Expected ten native feeding credits");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=14019");
    Require(bool(addon), "Missing Monkey Business quest addon");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    Require(quest->SourceItemId == Bananas && quest->SourceItemIdCount == 1 && quest->PackageID == 9628,
        "Expected one stack of bananas and the native reward package");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_SmartScripts();
    sScriptMgr->SwapScriptContext(true);

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE monkey_smart AS SELECT * FROM smart_scripts WHERE source_type=0 AND entryorguid=34699");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM monkey_smart");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE monkey_ender AS SELECT * FROM creature_questender WHERE quest=14019");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM monkey_ender");
#ifndef MONKEY_BUSINESS_BASELINE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE monkey_conditions AS SELECT * FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=67917");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM monkey_conditions");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSmartScriptMgr->LoadSmartAIFromDB();
#ifndef MONKEY_BUSINESS_BASELINE
    sConditionMgr->LoadConditions();
    Require(sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL, FeedSpell).size() == 4,
        "The carried banana and living monkey gates did not load");
#endif
    return quest;
}

struct FeedingMonkey : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(new SmartAI(this)), "The monkey's native SmartAI did not initialize");
    }
};

SpellCastResult Feed(LootPlayer& player, Unit& target)
{
    Item* item = player.GetItemByEntry(Bananas);
    Require(item, "A real carried banana item is needed for the test");
    return player.CastSpell(&target, FeedSpell, false, item);
}

void CheckFeeding(Quest const* quest)
{
    auto bamm = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation,phaseMask FROM creature "
        "WHERE id=35758 AND map=648 AND areaId=4721");
    auto monkeys = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation,phaseMask FROM creature "
        "WHERE id=34699 AND map=648 AND areaId=4721 ORDER BY guid");
    Require(bamm && bamm->GetRowCount() == 1 && monkeys && monkeys->GetRowCount() >= 11 &&
        WorldDatabase.Query("SELECT 1 FROM creature_queststarter WHERE quest=14019 AND id=35758"),
        "Missing the native questgiver or live feeding targets");
    auto point = [](Field* row) { return Position{row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat()}; };
    Position receiverPoint = point(bamm->Fetch());
    Map map(648, 300, 0, DIFFICULTY_NONE);
    Actor receiver(&map, Bamm, bamm->Fetch()[0].GetUInt32(), receiverPoint);
    receiver.SetPhaseMask(bamm->Fetch()[5].GetUInt32(), false);
    Require(map.AddToMap(static_cast<Creature*>(&receiver)), "Could not publish the actual questgiver");
    Group party;
    party.*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(14019, 0);
    LootPlayer owner(SupplySession(), map, 14019, receiverPoint), other(SupplySession(), map, 14020, receiverPoint);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_GOBLIN);
        player->setFactionForRace(RACE_GOBLIN);
        player->InitDisplayIds();
        player->SetLevel(10);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        player->SetPhaseMask(2, false);
    }
    owner.AddQuestAndCheckCompletion(quest, &receiver);
    Require(owner.GetItemCount(Bananas) == 1 && !owner.CanRewardQuest(quest, false), "Acceptance should provide one bundle and no credit");
#ifndef MONKEY_BUSINESS_BASELINE
    Require(Feed(owner, receiver) != SPELL_CAST_OK, "Bananas accepted an unrelated creature");
#endif

    uint32 expectedOther = 0;
    for (uint32 index = 0; index < 11; ++index, monkeys->NextRow())
    {
        Field* fields = monkeys->Fetch();
        Position location = point(fields);
        Position facingMonkey{location.GetPositionX() - 2.0f, location.GetPositionY(), location.GetPositionZ(), 0.0f};
        SupplyMove(owner, facingMonkey);
        SupplyMove(other, facingMonkey);
        FeedingMonkey monkey(&map, Monkey, fields[0].GetUInt32(), location);
        Require((fields[5].GetUInt32() & 2) != 0, "The monkey is absent in the shared quest phase");
        monkey.SetPhaseMask(fields[5].GetUInt32(), false);
        Require(map.AddToMap(static_cast<Creature*>(&monkey)), "Could not publish the actual monkey");
        if (index == 0)
        {
            GiveSupply(other, Bananas, 1);
#ifdef MONKEY_BUSINESS_BASELINE
            Require(Feed(other, monkey) == SPELL_CAST_OK, "Expected the original unguarded banana cast");
            Tick(other, 3000);
            Require(!monkey.IsAlive() && !other.GetQuestObjectiveData(FeedingQuest, FeedCredit),
                "Expected the original spell to expend a quest target without an active quest");
            std::cout << "EXPECTED_BROKEN a retained bundle used a Bomb-Throwing Monkey without an active quest\n";
            return;
#else
            Require(Feed(other, monkey) != SPELL_CAST_OK, "A retained bundle worked without an active quest");
            other.DestroyItemCount(Bananas, 1, true);
            other.AddQuestAndCheckCompletion(quest, &receiver);
            other.DestroyItemCount(Bananas, 1, true);
            Require(other.CastSpell(&monkey, FeedSpell, false) != SPELL_CAST_OK, "An accepted quest alone enabled feeding without the bundle");
            GiveSupply(other, Bananas, 1);
            monkey.SetLootRecipient(&other);
            Require(Feed(owner, monkey) == SPELL_FAILED_CANT_CAST_ON_TAPPED && monkey.IsAlive() &&
                !other.GetQuestObjectiveData(FeedingQuest, FeedCredit), "The spell did not preserve its native combat tap restriction");
            monkey.SetLootRecipient(nullptr);

            Require(Feed(owner, monkey) == SPELL_CAST_OK, "The native missile did not launch while the target was untapped");
            Tick(owner, 20);
            monkey.SetLootRecipient(&other);
            Tick(owner, 3000);
#ifdef MONKEY_BUSINESS_FLIGHT_BASELINE
            Require(!owner.GetQuestObjectiveData(FeedingQuest, FeedCredit) && other.GetQuestObjectiveData(FeedingQuest, FeedCredit) == 1 &&
                !monkey.IsAlive(), "Expected the old missile to credit the unrelated combat tap at impact");
            std::cout << "EXPECTED_BROKEN a combat tap received another player's banana while it was in flight\n";
            return;
#else
            Require(!owner.GetQuestObjectiveData(FeedingQuest, FeedCredit) && !other.GetQuestObjectiveData(FeedingQuest, FeedCredit) &&
                monkey.IsAlive(), "A foreign combat tap between launch and impact still consumed the target or gained credit");
            monkey.SetLootRecipient(nullptr);

            Require(Feed(owner, monkey) == SPELL_CAST_OK, "The untapped target did not admit a follow-up item cast");
            Tick(owner, 20);
            AbandonSupply(owner, FeedingQuest);
            owner.AddQuestAndCheckCompletion(quest, &receiver);
            Tick(owner, 3000);
            Require(owner.GetItemCount(Bananas) == 1 && !owner.GetQuestObjectiveData(FeedingQuest, FeedCredit) && monkey.IsAlive(),
                "A projectile from the abandoned quest affected the newly accepted quest");
#endif
#endif
        }
        if (index == 1)
        {
            for (auto player : {&owner, &other})
            {
                Group::MemberSlot member;
                member.Guid = player->GetGUID();
                member.Class = player->getClass();
                const_cast<Group::MemberSlotList&>(party.GetMemberSlots()).push_back(member);
                player->SetGroup(&party, 0);
            }
            sGroupMgr->AddGroup(&party);
            monkey.SetLootRecipient(&other);
            expectedOther = 1;
        }
        if (index == 2)
            monkey.SetLootRecipient(&owner);
        if (index == 10)
        {
            Require(Feed(owner, monkey) != SPELL_CAST_OK && monkey.IsAlive(), "A completed quest accepted additional feeding");
            other.Kill(&monkey);
            Require(other.GetQuestObjectiveData(FeedingQuest, FeedCredit) == expectedOther, "Ordinary combat death advanced the feeding objective");
            break;
        }
        SpellCastResult result = Feed(owner, monkey);
        if (result != SPELL_CAST_OK)
            std::cout << "CASE feeding result=" << result << " monkey=" << index << " faction=" << monkey.getFaction() << '\n';
        Require(result == SPELL_CAST_OK, "The native banana spell rejected a valid delivery");
        Tick(owner, 3000);
        if (index == 1)
        {
            owner.SetGroup(nullptr);
            other.SetGroup(nullptr);
            sGroupMgr->RemoveGroup(&party);
        }
        Require(owner.GetQuestObjectiveData(FeedingQuest, FeedCredit) == index + 1 &&
            other.GetQuestObjectiveData(FeedingQuest, FeedCredit) == expectedOther &&
            owner.GetItemCount(Bananas) == 1 && !monkey.IsAlive(),
            "Expected one credit for the feeder and any eligible nearby party member, and the authored monkey death");
        Require(Feed(other, monkey) != SPELL_CAST_OK, "A dead monkey accepted another feed");
    }

    uint32 reward = 0;
    if (auto choices = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (auto entry : *choices)
            if (owner.CanSelectQuestPackageItem(entry))
            {
                reward = entry->ItemID;
                break;
            }
    SupplyMove(owner, receiverPoint);
    Require(reward && owner.CanRewardQuest(quest, reward, false), "Ten feeds should admit the native reward choice");
    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = receiver.GetGUID();
    packet.QuestID = FeedingQuest;
    packet.ItemChoiceID = reward;
    owner.GetSession()->HandleQuestGiverChooseReward(packet);
    Require(owner.GetQuestRewardStatus(FeedingQuest) && owner.GetItemCount(reward) && !owner.GetItemCount(Bananas),
        "Native turn-in must award the chosen item and remove the banana bundle");
    AbandonSupply(other, FeedingQuest);
    Require(!other.GetItemCount(Bananas), "Abandon must remove the banana bundle");
    std::cout << "PASS ten native monkey feeds, contested tap, dead/foreign targets, native reward and abandon cleanup\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{648, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{648, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The feeding test requires the isolated character schema");
        auto quest = LoadFeedingQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckFeeding(quest);
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
