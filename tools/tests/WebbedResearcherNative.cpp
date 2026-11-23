#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

void AddSC_bloodmyst_isle();

namespace
{
constexpr uint32 Rescue = 9670;
constexpr uint32 Web = 17680;
constexpr uint32 Researcher = 17681;
constexpr uint32 CorneliusSpawn = 17228;
constexpr uint32 SilkCover = 26017;

Quest const* LoadRescue()
{
    auto row = WorldDatabase.Query(
#include "NativeWebbedResearcher.inc"
    );
    Require(bool(row), "Missing Bloodmyst researcher quest");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Rescue] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Rescue, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=9670");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == Researcher && objective->Fetch()[5].GetUInt32() == 5,
        "Missing five-rescuer objective");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    sObjectMgr->LoadItemTemplates();
    return quest;
}

void LoadCornelius()
{
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid=17228");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=9670");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=9670");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
}

struct CorneliusActor : Creature
{
    explicit CorneliusActor(Map& map) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(CorneliusSpawn);
        Require(data && data->id == 17686, "Researcher Cornelius is missing his original placement");
        sObjectMgr->RemoveCreatureFromGrid(CorneliusSpawn, data);
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(CorneliusSpawn, &map, true), "Could not publish Researcher Cornelius");
    }

    ~CorneliusActor()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

struct WebActor : Actor
{
    using Actor::Actor;

    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        auto ai = sScriptMgr->GetCreatureAI(this);
        Require(ai && AIM_Initialize(ai), "Original webbed-creature AI could not initialize");
    }
};

struct RescuePet : LootPet
{
    RescuePet(LootPlayer& owner, Map& map) : LootPet(owner, map) { m_ControlledByPlayer = true; }
};

struct RescueParty : Group
{
    RescueParty(LootPlayer& owner, LootPlayer& receiver) : members{&owner, &receiver}
    {
        this->*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(Rescue, 0);
        for (auto player : members)
        {
            MemberSlot slot;
            slot.Guid = player->GetGUID();
            slot.Class = player->getClass();
            const_cast<MemberSlotList&>(GetMemberSlots()).push_back(slot);
            player->SetGroup(this, 0);
        }
    }

    ~RescueParty()
    {
        for (auto player : members)
            player->SetGroup(nullptr);
    }

    LootPlayer* members[2];
};

void CheckRescue(Quest const* quest)
{
    auto source = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature WHERE id=17680 "
        "AND map=530 AND zoneId=3525 ORDER BY guid LIMIT 1");
    auto count = WorldDatabase.Query("SELECT COUNT(*) FROM creature WHERE id=17680 AND map=530 AND zoneId=3525");
    Require(source && count && count->Fetch()[0].GetUInt32() == 39, "Expected the existing Amberweb Pass placements");
    auto row = source->Fetch();
    Position point{row[1].GetFloat(),row[2].GetFloat(),row[3].GetFloat(),row[4].GetFloat()};
    LoadCornelius();
    Map map(530, 300, 0, DIFFICULTY_NONE);
    CorneliusActor cornelius(map);
    LootPlayer owner(SupplySession(), map, 9670, point), member(SupplySession(), map, 9671, point);
    for (auto player : {&owner, &member})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_DRAENEI);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(RACE_DRAENEI);
        player->InitDisplayIds();
        player->GetReputationMgr().LoadFromDB({});
        player->SetLevel(19);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    uint32 guid = 60000;
    auto breakWeb = [&](Unit& killer)
    {
        WebActor source(&map, Web, ++guid, point);
        Require(map.AddToMap(static_cast<Creature*>(&source)), "Could not publish the original webbed creature");
        auto ownerBefore = owner.GetQuestObjectiveData(Rescue, Researcher);
        auto memberBefore = member.GetQuestObjectiveData(Rescue, Researcher);
        killer.DealDamage(&source, uint32(source.GetMaxHealth()) * 2);
        Require(source.isDead(false), "Native hit did not break the web");
        auto freed = source.FindNearestCreature(Researcher, 2.0f, true);
        bool rescued = freed != nullptr;
        if (freed)
        {
            freed->DespawnOrUnsummon();
            map.RemoveAllObjectsInRemoveList();
        }
        Player* recipient = killer.GetCharmerOrOwnerPlayerOrPlayerItself();
        auto expected = [recipient, rescued](Player& player, int32 before)
        {
            return before + (rescued && recipient == &player && player.GetQuestStatus(Rescue) != QUEST_STATUS_NONE && before < 5 ? 1 : 0);
        };
        Require(owner.GetQuestObjectiveData(Rescue, Researcher) == expected(owner, ownerBefore) &&
            member.GetQuestObjectiveData(Rescue, Researcher) == expected(member, memberBefore),
            "The original script credited a wrong outcome or recipient");
        return rescued;
    };
    auto findResearcher = [&](Unit& killer)
    {
        for (unsigned attempt = 0; attempt < 80; ++attempt)
            if (breakWeb(killer))
                return;
        Require(false, "No researcher appeared after eighty native outcomes");
    };
    findResearcher(owner);
    Require(!owner.GetQuestObjectiveData(Rescue, Researcher), "Researcher outside the quest gave credit");
    auto accept = [&](LootPlayer& player)
    {
        player.Relocate(cornelius.GetPosition());
        WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
        packet.QuestGiverGUID = cornelius.GetGUID();
        packet.QuestID = Rescue;
        player.GetSession()->HandleQuestGiverAcceptQuest(packet);
        Require(player.GetQuestStatus(Rescue) == QUEST_STATUS_INCOMPLETE, "Cornelius did not admit the original quest");
        player.Relocate(point);
    };
    accept(owner);
    accept(member);
    RescueParty party(owner, member);
    findResearcher(member);
    Require(!owner.GetQuestObjectiveData(Rescue, Researcher) && member.GetQuestObjectiveData(Rescue, Researcher) == 1,
        "The initial member rescue did not remain personal");
    findResearcher(owner);
    RescuePet pet(owner, map);
    pet.SetLevel(19);
    findResearcher(pet);
    while (owner.GetQuestObjectiveData(Rescue, Researcher) < 5)
        findResearcher(pet);
    Require(owner.CanRewardQuest(quest, false), "Five successful rescues did not allow turn-in");
    Tick(member, 100);
    Require(member.GetQuestObjectiveData(Rescue, Researcher) == 1 && !member.CanRewardQuest(quest, false),
        "The local personal researcher receipt reached a nearby party member");
    AbandonSupply(member, Rescue);
    accept(member);
    Require(!member.GetQuestObjectiveData(Rescue, Researcher), "The new acceptance inherited old rescue credit");

    owner.Relocate(cornelius.GetPosition());
    Require(cornelius.hasInvolvedQuest(Rescue) && owner.CanInteractWithQuestGiver(&cornelius),
        "Cornelius could not be reached for the actual turn-in");
    auto items = sDB2Manager.GetQuestPackageItems(quest->PackageID);
    Require(quest->PackageID == 18595 && items && items->size() == 1 &&
        items->front()->ID == 40360 && items->front()->ItemID == SilkCover && items->front()->ItemQuantity == 1,
        "The matching Legion reward package changed");
    for (uint8 playerClass = CLASS_WARRIOR; playerClass < MAX_CLASSES; ++playerClass)
    {
        owner.SetByteValue(UNIT_FIELD_BYTES_0, 1, playerClass);
        Require(owner.CanSelectQuestPackageItem(items->front()), "The native cover was unavailable to a class");
    }
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
    Require(owner.CanRewardQuest(quest, SilkCover, false), "The actual Cornelius reward cannot be stored");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = cornelius.GetGUID();
    reward.QuestID = Rescue;
    reward.ItemChoiceID = SilkCover;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    std::cout << "CASE reward returned=" << owner.GetQuestRewardStatus(Rescue) << " copies=" << owner.GetItemCount(SilkCover)
        << " native package=" << quest->PackageID << '\n';
    Require(owner.GetQuestRewardStatus(Rescue), "Cornelius did not accept the actual quest turn-in");
    Require(owner.GetItemCount(SilkCover) == 1, "Cornelius did not grant the actual Venomous Silk Cover reward");
    std::cout << "PASS Cornelius acceptance, personal player and pet-owner credit in a party, five real rescues, abandon/retry and actual Silk Cover reward\n";
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
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_bloodmyst_isle();
        sScriptMgr->SwapScriptContext(true);
        auto quest = LoadRescue();
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The Bloodmyst check requires isolated characters");
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckRescue(quest);
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
