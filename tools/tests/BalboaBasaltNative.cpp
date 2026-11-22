#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "CreatureTextMgr.h"
#include "SmartAI.h"
#include "SmartScriptMgr.h"
#include "TemporarySummon.h"

void AddSC_SmartScripts();

namespace
{
constexpr uint32 BasaltQuest = 14250;
constexpr uint32 Kit = 49038;
constexpr uint32 Basalt = 49036;
constexpr uint32 PlaceCharge = 67922;
constexpr uint32 Charge = 35762;
constexpr uint32 Chest = 195602;

void ShadowBalboaRows(char const* name, char const* filter)
{
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE balboa_%s AS SELECT * FROM %s WHERE %s",
        name, name, filter).c_str());
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE %s AS SELECT * FROM balboa_%s", name, name).c_str());
}

Quest const* LoadBalboaQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeBalboaBasalt.inc"
    );
    Require(bool(row), "Missing Renewable Resource");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[BasaltQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(BasaltQuest, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=14250");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM &&
        objective->Fetch()[4].GetUInt32() == Basalt && objective->Fetch()[5].GetUInt32() == 5, "Expected five actual basalt pieces");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=14250");
    Require(bool(addon), "Missing Renewable Resource quest addon");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    Require(quest->SourceItemId == Kit && quest->SourceItemIdCount == 1, "Expected one Arcane Charge kit");

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
    ShadowBalboaRows("smart_scripts", "source_type=0 AND entryorguid IN(35759,35762,35772)");
    ShadowBalboaRows("creature_questender", "quest=14250");
    ShadowBalboaRows("creature_text", "CreatureID=35759");
    ShadowBalboaRows("conditions", "(SourceTypeOrReferenceId=22 AND SourceEntry=35762) OR "
        "(SourceTypeOrReferenceId=13 AND SourceEntry=67915) OR (SourceTypeOrReferenceId=17 AND SourceEntry=67922)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sConditionMgr->LoadConditions();
    sCreatureTextMgr->LoadCreatureTexts();
    auto& table = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto original = table;
    table += " WHERE Entry=195602";
    auto loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    table = original;
    Require(loaded == 1, "Missing the actual quest-only chest stock");
    return quest;
}

struct BalboaActor : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(new SmartAI(this)), "The original Balboa AI could not initialize");
    }
};

Position PlacedPoint(Field* row)
{
    return {row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat()};
}

SpellCastResult Plant(LootPlayer& player)
{
    auto item = player.GetItemByEntry(Kit);
    Require(item, "Expected the real quest item");
    return player.CastSpell(&player, PlaceCharge, false, item);
}

TempSummon* FindCharge(LootPlayer& player)
{
    for (ObjectGuid const& guid : *player.GetSummonList(Charge))
        if (Creature* found = ObjectAccessor::GetCreature(player, guid))
            if (!found->IsDespawn())
                return found->ToTempSummon();
    return nullptr;
}

std::list<GameObject*> Pieces(WorldObject const& origin)
{
    std::list<GameObject*> pieces;
    origin.GetGameObjectListWithEntryInGrid(pieces, Chest, 30.0f);
    return pieces;
}

void AdvanceBalboa(BalboaActor& balboa, std::initializer_list<TempSummon*> charges)
{
    for (TempSummon* charge : charges)
        charge->AI()->UpdateAI(350);
    for (uint32 elapsed = 0; elapsed < 1800; elapsed += 10)
        balboa.Unit::Update(10);
}

void TakePiece(LootPlayer& player, GameObject& chest)
{
    auto before = player.GetItemCount(Basalt);
    SupplyMove(player, chest.GetPosition());
    chest.Update(3000);
    Require(chest.getLootState() == GO_READY, "The generated chest did not become ready on its native update");
    Require(player.HasQuestForItem(Basalt), "An accepted player has no native need for another piece");
    OpenSupply(player, chest);
    Require(player.GetLootGUID() == chest.GetGUID(), "The real basalt chest did not open for its owner");
    Loot& loot = chest.IsPersonal() ? player.personalLoot.at(chest.GetGUID()) : chest.loot;
    WorldPackets::Loot::LootResponse response;
    loot.BuildLootResponse(response, &player, ALL_PERMISSION);
    std::cout << "CASE basalt before=" << before << " slots=" << response.Items.size() << " stored=" << loot.items.size()
        << " personal=" << chest.IsPersonal() << " data=" << chest.GetGOInfo()->GetLootId() << std::endl;
    Require(response.Items.size() == 1 && response.Items.front().Loot.ItemID == Basalt && response.Items.front().Quantity == 1,
        "The real chest did not advertise one basalt piece");
    WorldPackets::Loot::AutoStoreLootItem packet{WorldPacket(CMSG_LOOT_ITEM)};
    packet.Loot.push_back({loot.GetGUID(), response.Items.front().LootListID});
    player.GetSession()->HandleAutostoreLootItemOpcode(packet);
    ReleaseSupply(player, chest);
    Require(player.GetItemCount(Basalt) == before + 1, "The native loot packet did not store the picked-up piece");
}

void CheckBalboa(Quest const* quest)
{
    auto giver = WorldDatabase.Query("SELECT c.guid,c.position_x,c.position_y,c.position_z,c.orientation FROM creature c "
        "JOIN creature_queststarter s ON s.id=c.id AND s.quest=14250 JOIN creature_questender e ON e.id=c.id AND e.quest=14250 "
        "WHERE c.id=35755 AND c.guid=99375 AND c.map=1 AND c.zoneId=16");
    auto actor = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=35759 AND guid=99327 AND map=1 AND zoneId=16 AND MovementType=2");
    auto path = WorldDatabase.Query("SELECT w.point,w.position_x,w.position_y,w.position_z,w.orientation FROM waypoint_data w "
        "JOIN creature_addon a ON a.path_id=w.id WHERE a.guid=99327 ORDER BY w.point");
    Require(giver && actor && path && path->GetRowCount() == 10, "Missing original giver, Balboa or ten waypoints");
    Position receiverPoint = PlacedPoint(giver->Fetch());
    Position routePoint = PlacedPoint(path->Fetch());
    Map map(1, 500, 0, DIFFICULTY_NONE);
    Actor receiver(&map, 35755, giver->Fetch()[0].GetUInt32(), receiverPoint);
    BalboaActor balboa(&map, 35759, actor->Fetch()[0].GetUInt32(), PlacedPoint(actor->Fetch()));
    Require(map.AddToMap(static_cast<Creature*>(&receiver)) && map.AddToMap(static_cast<Creature*>(&balboa)),
        "Could not publish the original actors");
    LootPlayer owner(SupplySession(), map, 14250, routePoint), guest(SupplySession(), map, 14251, routePoint);
    for (auto player : {&owner, &guest})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_GOBLIN);
        player->setFactionForRace(RACE_GOBLIN);
        player->InitDisplayIds();
        player->SetLevel(16);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        player->GetZoneAndAreaId(player->m_zoneId, player->m_areaId);
        Require(player->GetCurrentZoneID() == 16, "The actual waypoint must resolve to Azshara");
    }
    GiveSupply(guest, Kit, 1);
#ifdef BALBOA_BASALT_BASELINE
    Require(Plant(guest) == SPELL_CAST_OK, "The old spell did not expose unguarded use");
    Tick(guest, 650);
    auto oldCharge = FindCharge(guest);
    Require(oldCharge && !oldCharge->HasQuestObjectScope(), "The old spell did not create an unowned quest charge");
    balboa.NearTeleportTo(routePoint);
    map.MoveAllCreaturesInMoveList();
    AdvanceBalboa(balboa, {oldCharge});
    auto oldPieces = Pieces(balboa);
    Require(oldPieces.size() == 5 && !oldPieces.front()->HasQuestObjectScope(), "Native old Balboa did not leave five public chests");
    std::cout << "EXPECTED_BROKEN inactive item use reached Balboa and published five public quest chests\n";
#else
    Require(Plant(guest) != SPELL_CAST_OK && !FindCharge(guest), "A leftover kit could plant after quest end");
    guest.DestroyItemCount(Kit, 1, true);
    owner.AddQuestAndCheckCompletion(quest, &receiver);
    guest.AddQuestAndCheckCompletion(quest, &receiver);
    Require(owner.CastSpell(&owner, PlaceCharge, false) != SPELL_CAST_OK, "An arbitrary cast bypassed the carried item");
    Require(Plant(owner) == SPELL_CAST_OK, "An accepted quest could not begin planting");
    Tick(owner, 200);
    AbandonSupply(owner, BasaltQuest);
    owner.AddQuestAndCheckCompletion(quest, &receiver);
    Tick(owner, 600);
    Require(!FindCharge(owner), "An obsolete cast survived abandon and reaccept");

    Require(Plant(owner) == SPELL_CAST_OK && Plant(guest) == SPELL_CAST_OK, "Two accepted players could not place their charges");
    Tick(owner, 650);
    Tick(guest, 650);
    auto first = FindCharge(owner);
    auto second = FindCharge(guest);
    Require(first && second && first != second && first->GetSummoner() == &owner && second->GetSummoner() == &guest &&
        first->HasQuestObjectScope() && second->HasQuestObjectScope(), "The native charge did not retain the correct accepted owner");
    AdvanceBalboa(balboa, {first, second});
    Require(Pieces(balboa).empty(), "The placed charge fired before Balboa reached the waypoint");
    balboa.NearTeleportTo(routePoint);
    map.MoveAllCreaturesInMoveList();
    balboa.SetHealth(1);
    AdvanceBalboa(balboa, {first, second});
    Require(balboa.IsAlive(), "The original quest explosion killed the shared source");

    std::vector<GameObject*> owned;
    std::vector<ObjectGuid> guestChests;
    auto pieces = Pieces(balboa);
    std::cout << "CASE simultaneous charges published " << pieces.size() << " chests\n";
    Require(pieces.size() == 10, "The native events did not produce five results per simultaneous charge");
    for (auto piece : pieces)
    {
        auto scope = piece->GetQuestObjectScope().lock();
        Require(scope && piece->GetRespawnDelay() == 35, "Expected a private chest with its original intended 35-second lifespan");
        if (scope->Owner == owner.GetGUID())
        {
            Require(piece->CanCollideWith(&owner) && !piece->CanCollideWith(&guest) && !piece->CanTargetQuestObject(&guest),
                "An unrelated player could target or collide with an exposed piece");
            owned.push_back(piece);
        }
        else
        {
            Require(scope->Owner == guest.GetGUID(), "A chest belonged to somebody who did not place a charge");
            guestChests.push_back(piece->GetGUID());
        }
    }
    Require(owned.size() == 5 && guestChests.size() == 5 && !owner.GetItemCount(Basalt) && !guest.GetItemCount(Basalt),
        "Charges must publish pieces without awarding items automatically");
    SupplyMove(guest, owned.front()->GetPosition());
    guest.CastSpell(owned.front(), 3365, false);
    Tick(guest, 6000);
    Require(!guest.GetLootGUID(), "A second quest holder opened a foreign chest");
    AbandonSupply(guest, BasaltQuest);
    map.RemoveAllObjectsInRemoveList();
    for (auto guid : guestChests)
        Require(!map.GetGameObject(guid), "Abandon preserved a foreign quest chest");
    Require(!guest.GetItemCount(Kit) && !FindCharge(guest), "Abandon preserved a charge or kit");

    for (auto piece : owned)
        TakePiece(owner, *piece);
    Require(owner.GetItemCount(Basalt) == 5 && owner.CanRewardQuest(quest, false), "Five genuine pickups did not enable the actual reward");
    SupplyMove(owner, receiverPoint);
    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = receiver.GetGUID();
    packet.QuestID = BasaltQuest;
    packet.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(packet);
    map.RemoveAllObjectsInRemoveList();
    Require(owner.GetQuestRewardStatus(BasaltQuest) && !owner.GetItemCount(Basalt) && !owner.GetItemCount(Kit) &&
        Pieces(balboa).empty(), "Turn-in left the pieces, the kit, or the private world objects behind");
    std::cout << "PASS actual Arcane Charge, original path and native links, independent private chunks, packet loot and quest cleanup\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The test requires the isolated character schema");
        auto quest = LoadBalboaQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckBalboa(quest);
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
