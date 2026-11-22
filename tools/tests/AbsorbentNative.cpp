#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "SmartAI.h"
#include "SmartScriptMgr.h"

void AddSC_SmartScripts();

namespace
{
constexpr uint32 SpongeQuest = 14323;
constexpr uint32 Sponge = 49168;
constexpr uint32 Droplet = 49172;
constexpr uint32 Spongy = 68413;
constexpr uint32 Pulse = 68411;
constexpr uint32 Splash = 36131;
constexpr uint32 Ergll = 35142;

void ShadowAbsorbentRows(char const* name, char const* filter)
{
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE absorbent_%s AS SELECT * FROM %s WHERE %s",
        name, name, filter).c_str());
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE %s AS SELECT * FROM absorbent_%s", name, name).c_str());
}

Quest const* LoadAbsorbentQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeAbsorbent.inc"
    );
    Require(bool(row), "Missing Absorbent");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[SpongeQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(SpongeQuest, quest);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=14323 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 2 && objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM &&
        objectives->Fetch()[4].GetUInt32() == Droplet && objectives->Fetch()[5].GetUInt32() == 20, "Expected 20 actual droplets");
    quest->LoadQuestObjective(objectives->Fetch());
    objectives->NextRow();
    Require(objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM && objectives->Fetch()[4].GetUInt32() == Sponge &&
        objectives->Fetch()[5].GetUInt32() == 1, "The original sponge must be returned");
    quest->LoadQuestObjective(objectives->Fetch());
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=14323");
    Require(bool(addon), "Missing Absorbent quest addon");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    Require(quest->SourceItemId == Sponge && quest->SourceItemIdCount == 1 && quest->HasFlag(QUEST_FLAGS_AUTOCOMPLETE),
        "Expected the provided sponge and original remote handoff");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_SmartScripts();
    sScriptMgr->SwapScriptContext(true);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    ShadowAbsorbentRows("smart_scripts", "source_type=0 AND entryorguid=36131");
    ShadowAbsorbentRows("creature_questender", "quest=14323");
    ShadowAbsorbentRows("conditions", "(SourceTypeOrReferenceId=22 AND SourceEntry=36131) OR "
        "(SourceTypeOrReferenceId=17 AND SourceEntry IN(68411,68413)) OR (SourceTypeOrReferenceId=13 AND SourceEntry=68411)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sConditionMgr->LoadConditions();
    return quest;
}

struct VileSplash : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(new SmartAI(this)), "The original Vile Splash AI could not initialize");
    }
};

Position AbsorptionPosition(Field* row)
{
    return {row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat()};
}

void FillSpongeTestBackpack(LootPlayer& player)
{
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < player.GetInventoryEndSlot(); ++slot)
        if (!player.GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            ItemPosCountVec dest;
            dest.emplace_back(uint16(INVENTORY_SLOT_BAG_0 << 8) | slot, 1);
            Require(player.StoreNewItem(dest, 2589, true), "Native backpack setup failed");
        }
}

void CheckAbsorption(Quest const* quest)
{
    auto giver = WorldDatabase.Query("SELECT c.guid,c.position_x,c.position_y,c.position_z,c.orientation FROM creature c "
        "JOIN creature_queststarter s ON s.id=c.id AND s.quest=14323 JOIN creature_questender e ON e.id=c.id AND e.quest=14323 "
        "WHERE c.id=35142 AND c.guid=99620 AND c.map=1 AND c.zoneId=16");
    auto placements = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=36131 AND map=1 AND zoneId=16 ORDER BY guid");
    Require(giver && placements && placements->GetRowCount() == 84, "Missing Ergll or the existing Vile Splashes");
    Position shore = AbsorptionPosition(placements->Fetch());
    Position receiver = AbsorptionPosition(giver->Fetch());
    Map map(1, 500, 0, DIFFICULTY_NONE);
    Actor ergll(&map, Ergll, giver->Fetch()[0].GetUInt32(), receiver);
    Require(map.AddToMap(static_cast<Creature*>(&ergll)), "Ergll could not load in the original world");
    LootPlayer owner(SupplySession(), map, 14323, receiver), foreign(SupplySession(), map, 14324, receiver);
    for (auto player : {&owner, &foreign})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_GOBLIN);
        player->setFactionForRace(RACE_GOBLIN);
        player->InitDisplayIds();
        player->SetLevel(17);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        player->GetZoneAndAreaId(player->m_zoneId, player->m_areaId);
    }
    GiveSupply(foreign, Sponge, 1);
    Tick(foreign, 2500);
#ifdef ABSORBENT_BASELINE
    Require(foreign.HasAura(Spongy), "The old sponge must show inactive acquisition without quest admission");
    owner.AddQuestAndCheckCompletion(quest, &ergll);
    Tick(owner, 2500);
    Require(owner.HasAura(Spongy), "The original provided item did not apply its native proc");
    VileSplash splash(&map, Splash, placements->Fetch()[0].GetUInt32(), shore);
    map.LoadGrid(shore.GetPositionX(), shore.GetPositionY());
    Require(map.AddToMap(static_cast<Creature*>(&splash)), "Actual Vile Splash did not load");
    SupplyMove(owner, shore);
    owner.RemoveAurasDueToSpell(Spongy);
    Require(owner.CastSpell(&owner, Pulse, true) == SPELL_CAST_OK, "The old pulse did not admit a cast with no sponge aura");
    Tick(owner, 4500);
    std::cout << "CASE old pulse without aura: target alive=" << splash.IsAlive() << " droplets=" << owner.GetItemCount(Droplet) << std::endl;
    std::cout << "EXPECTED_BROKEN old kit applied without the quest and the pulse accepted a cast without its sponge aura\n";
#else
    Require(!foreign.HasAura(Spongy), "A retained sponge applied its aura without the quest");
    owner.AddQuestAndCheckCompletion(quest, &ergll);
    Tick(owner, 2500);
    Require(owner.HasAura(Spongy, owner.GetGUID()) && owner.GetItemCount(Sponge) == 1 && !owner.GetItemCount(Droplet),
        "Accepting the quest must install the actual carried sponge's periodic aura");
    FillSpongeTestBackpack(owner);
    for (uint32 index = 0; index < 20; ++index)
    {
        auto row = placements->Fetch();
        Position point = AbsorptionPosition(row);
        map.LoadGrid(point.GetPositionX(), point.GetPositionY());
        VileSplash splash(&map, Splash, row[0].GetUInt32(), point);
        Require(map.AddToMap(static_cast<Creature*>(&splash)), "Actual Vile Splash did not load");
        SupplyMove(owner, point);
        SupplyMove(foreign, point);
        if (!index)
        {
            Tick(owner, 3500);
            Require(splash.IsAlive() && !owner.GetItemCount(Droplet), "A full backpack consumed a Vile Splash");
            owner.DestroyItemCount(2589, 1, true);
            splash.SetLootRecipient(&foreign);
            Tick(owner, 3500);
            Require(splash.IsAlive() && !owner.GetItemCount(Droplet), "The sponge stole an unrelated player’s tapped target");
            splash.SetLootRecipient(nullptr);
            owner.RemoveAurasDueToSpell(Spongy);
            Require(owner.CastSpell(&owner, Pulse, true) != SPELL_CAST_OK, "An arbitrary pulse was admitted without its aura");
            Tick(owner, 1500);
            Require(splash.IsAlive() && !owner.GetItemCount(Droplet), "An arbitrary pulse worked without the sponge aura");
            Require(owner.CastSpell(&owner, Spongy, false, owner.GetItemByEntry(Sponge)) == SPELL_CAST_OK,
                "The original sponge could not be used again");
        }
        Tick(owner, 4500);
        for (uint32 elapsed = 0; elapsed < 3000; elapsed += 20)
            splash.Unit::Update(20);
        Require(owner.GetItemCount(Droplet) == index + 1 && !foreign.GetItemCount(Droplet),
            "The real pulse did not produce one native quest item for its owner");
        Tick(owner, 3500);
        Require(owner.GetItemCount(Droplet) == index + 1, "Repeated aura ticks consumed the same Vile Splash twice");
        placements->NextRow();
    }
    Require(owner.GetQuestStatus(SpongeQuest) == QUEST_STATUS_COMPLETE && owner.CanRewardQuest(quest, false),
        "Twenty native droplets did not enable the quest's actual handoff");
    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = owner.GetGUID();
    packet.QuestID = SpongeQuest;
    packet.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(packet);
    Require(owner.GetQuestRewardStatus(SpongeQuest) && !owner.GetItemCount(Droplet) && !owner.GetItemCount(Sponge) && !owner.HasAura(Spongy),
        "Remote handoff did not take the two actual requirements and their aura");

    foreign.DestroyItemCount(Sponge, 1, true);
    foreign.AddQuestAndCheckCompletion(quest, &ergll);
    Tick(foreign, 2500);
    Require(foreign.HasAura(Spongy), "The second acceptance must install a fresh aura");
    Field* extra = placements->Fetch();
    Position extraPoint = AbsorptionPosition(extra);
    map.LoadGrid(extraPoint.GetPositionX(), extraPoint.GetPositionY());
    VileSplash spare(&map, Splash, extra[0].GetUInt32(), extraPoint);
    Require(map.AddToMap(static_cast<Creature*>(&spare)), "The second player could not reach another actual Splash");
    SupplyMove(foreign, extraPoint);
    Tick(foreign, 4500);
    Require(foreign.GetItemCount(Droplet) == 1, "The second acceptance did not collect a real droplet");
    AbandonSupply(foreign, SpongeQuest);
    std::cout << "CASE abandon sponge=" << foreign.GetItemCount(Sponge) << " droplets=" << foreign.GetItemCount(Droplet)
        << " aura=" << foreign.HasAura(Spongy) << std::endl;
    Require(!foreign.GetItemCount(Sponge) && !foreign.GetItemCount(Droplet) && !foreign.HasAura(Spongy),
        "Abandon left the quest items or periodic selection active");
    std::cout << "PASS actual native sponge aura, full bags and tap retry, twenty world splashes, remote reward and cleanup\n";
#endif
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The sponge test requires the isolated character schema");
        auto quest = LoadAbsorbentQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckAbsorption(quest);
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
