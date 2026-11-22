#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameEventMgr.h"
#include "SmartAI.h"
#include "SmartScriptMgr.h"
#include "SpellPackets.h"

namespace
{
constexpr uint16 Faire = 75;
constexpr uint32 Cooking = 29509;
constexpr uint32 Engineering = 29511;
constexpr uint32 PlumpFrogs = 72056;
constexpr uint32 BreadedFrog = 72057;
constexpr uint32 CrunchyFrog = 72058;
constexpr uint32 Wrench = 72110;
constexpr uint32 Repair = 101838;
constexpr uint32 DamageAura = 29266;
constexpr uint32 DamagedTonk = 54504;

void TempRows(char const* sql)
{
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute(sql);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
}

void LoadFairActors()
{
    auto rows = WorldDatabase.Query(
#include "NativeDarkmoonFairTargets.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "Missing the two profession quests");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto row = rows->Fetch();
        auto quest = new Quest(row);
        auto id = quest->GetQuestId();
        quests[id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(id, quest);
        auto objectives = WorldDatabase.PQuery("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
            "FROM quest_objectives WHERE QuestID=%u", id);
        Require(objectives && objectives->GetRowCount() == 1 && objectives->Fetch()[5].GetUInt32() == 5, "Wrong native profession objective");
        quest->LoadQuestObjective(objectives->Fetch());
        auto addon = WorldDatabase.PQuery("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
            "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
            "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=%u", id);
        Require(addon && addon->Fetch()[10].GetUInt32() == 75, "Wrong original profession requirement");
        quest->LoadQuestTemplateAddon(addon->Fetch());
        if (id == Cooking)
            quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
        else
            quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    } while (rows->NextRow());

    auto event = WorldDatabase.Query("SELECT occurence,length FROM game_event WHERE eventEntry=75 AND world_event=0");
    Require(bool(event), "The original Darkmoon event is missing");
    auto& calendar = const_cast<GameEventMgr::GameEventDataMap&>(sGameEventMgr->GetEventMap());
    calendar.resize(Faire + 1);
    calendar[Faire].occurence = event->Fetch()[0].GetUInt32();
    calendar[Faire].length = event->Fetch()[1].GetUInt32();

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    TempRows("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(29509,29511)");
    TempRows("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest IN(29509,29511)");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    TempRows("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells WHERE npc_entry=54504");
    sObjectMgr->LoadNPCSpellClickSpells();
    TempRows("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE "
        "(SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(29509,29511)) OR "
        "(SourceTypeOrReferenceId IN(13,17,18) AND SourceEntry IN(101838,102050,102053,102363))");
    sConditionMgr->LoadConditions();
    TempRows("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 AND entryorguid IN(54504,54551)");
    sSmartScriptMgr->LoadSmartAIFromDB();
}

struct FaireActor : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(GetCreatureTemplate()->AIName == "SmartAI" ? static_cast<CreatureAI*>(new SmartAI(this)) :
            new NullCreatureAI(this)), "The actual Faire actor did not initialize");
    }
};

Position Coordinates(Field* fields)
{
    return {fields[0].GetFloat(), fields[1].GetFloat(), fields[2].GetFloat(), fields[3].GetFloat()};
}

std::unique_ptr<FaireActor> PlaceActor(Map& map, uint32 entry, uint32 spawn)
{
    auto row = WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation FROM creature WHERE map=974 AND zoneId=5861 AND id=%u AND guid=%u",
        entry, spawn);
    Require(bool(row), "A genuine Faire creature is missing");
    Position position = Coordinates(row->Fetch());
    map.LoadGrid(position.GetPositionX(), position.GetPositionY());
    auto actor = std::make_unique<FaireActor>(&map, entry, spawn, position);
    Require(map.AddToMap(static_cast<Creature*>(actor.get())), "Could not publish the actual event actor");
    return actor;
}

void InitializePlayer(LootPlayer& player, uint16 skill)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_HUMAN);
    player.setFactionForRace(RACE_HUMAN);
    player.InitDisplayIds();
    player.SetLevel(20);
    player.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    player.SetSkill(skill, 1, 75, 75);
}

void Accept(LootPlayer& player, Creature& giver, uint32 quest)
{
    SupplyMove(player, giver.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    packet.QuestGiverGUID = giver.GetGUID();
    packet.QuestID = quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(packet);
    Require(player.GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE, "The placed Faire NPC did not start the quest");
}

void TurnIn(LootPlayer& player, Creature& giver, uint32 quest)
{
    SupplyMove(player, giver.GetPosition());
    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = giver.GetGUID();
    packet.QuestID = quest;
    packet.ItemChoiceID = 0;
    player.GetSession()->HandleQuestGiverChooseReward(packet);
    Require(player.FindQuestSlot(quest) == MAX_QUEST_LOG_SIZE, "The native receiver did not finish the profession quest");
}

SpellCastResult UseItem(LootPlayer& player, uint32 entry, uint32 spell, Unit& target)
{
    Item* item = player.GetItemByEntry(entry);
    Require(item, "Expected the original quest item");
    GameTime::UpdateGameTimers();
    return player.CastSpell(&target, spell, false, item);
}

void BreadBatch(LootPlayer& player)
{
    GiveSupply(player, 30817, 5);
    Require(UseItem(player, PlumpFrogs, 102050, player) == SPELL_CAST_OK, "Native breading did not start");
    Tick(player, 6500);
    Require(player.GetItemCount(BreadedFrog) == 5 && !player.GetItemCount(PlumpFrogs) && !player.GetItemCount(30817),
        "Native breading did not consume one bundle and five flour");
}

void CheckFrogs(Map& map)
{
    auto trainer = PlaceActor(map, 14845, 2160);
    auto cookpot = PlaceActor(map, 54551, 2161);
    auto alternate = PlaceActor(map, 54551, 2171);
    LootPlayer owner(SupplySession(), map, 2950901, trainer->GetPosition()), other(SupplySession(), map, 2950902, trainer->GetPosition());
    InitializePlayer(owner, 185);
    InitializePlayer(other, 185);
    GiveSupply(other, PlumpFrogs, 1);
    GiveSupply(other, 30817, 5);
#ifdef DARKMOON_FAIR_TARGETS_BASELINE
    Require(UseItem(other, PlumpFrogs, 102050, other) == SPELL_CAST_OK, "Baseline no longer prepares frogs outside the quest");
    Tick(other, 6500);
    Require(other.GetItemCount(BreadedFrog) == 5, "The baseline no longer makes the five native frogs outside the quest");
    SupplyMove(other, cookpot->GetPosition());
    Require(UseItem(other, BreadedFrog, 102053, other) == SPELL_CAST_OK, "Baseline no longer fries a frog without the quest");
    Tick(other, 6500);
    Require(other.GetItemCount(CrunchyFrog) == 1, "The live Cookpot did not emit the expected baseline result");
    Accept(owner, *trainer, Cooking);
    GiveSupply(owner, CrunchyFrog, 1);
    AbandonSupply(owner, Cooking);
    Require(owner.GetItemCount(CrunchyFrog) == 1, "The baseline no longer leaves the abandoned cooked frog");
    std::cout << "EXPECTED_BROKEN the real Cookpot and ingredients worked outside the quest and kept abandoned extra output\n";
#else
    Require(UseItem(other, PlumpFrogs, 102050, other) != SPELL_CAST_OK, "Cooking begins outside the active quest");
    Accept(owner, *trainer, Cooking);
    BreadBatch(owner);
    Position away = trainer->GetPosition();
    away.RelocateOffset(Position{30.0f, 0.0f, 0.0f, 0.0f});
    SupplyMove(owner, away);
    UseItem(owner, BreadedFrog, 102053, owner);
    Tick(owner, 6500);
    Require(owner.GetItemCount(BreadedFrog) == 5 && !owner.GetItemCount(CrunchyFrog), "Frying away from the Cookpot consumed a frog");
    SupplyMove(owner, cookpot->GetPosition());
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < owner.GetInventoryEndSlot(); ++slot)
        if (!owner.GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            ItemPosCountVec destination;
            destination.emplace_back(uint16(INVENTORY_SLOT_BAG_0 << 8) | slot, 1);
            Require(owner.StoreNewItem(destination, 2589, true), "Couldn't fill the test backpack");
        }
    Require(UseItem(owner, BreadedFrog, 102053, owner) != SPELL_CAST_OK && owner.GetItemCount(BreadedFrog) == 5,
        "The Cookpot consumed a frog when the result could not fit");
    owner.DestroyItemCount(2589, owner.GetItemCount(2589), true);

    Require(UseItem(owner, BreadedFrog, 102053, owner) == SPELL_CAST_OK, "Could not fry the genuine frog before abandonment");
    Tick(owner, 6500);
    Require(owner.GetItemCount(CrunchyFrog) == 1 && owner.GetItemCount(BreadedFrog) == 4, "The original Cookpot did not create the partial batch");
    AbandonSupply(owner, Cooking);
    Require(!owner.GetItemCount(CrunchyFrog) && !owner.GetItemCount(BreadedFrog), "Abandonment left real cooked or breaded frogs");
    Accept(owner, *trainer, Cooking);
    BreadBatch(owner);
    for (uint32 count = 0; count < 5; ++count)
    {
        SupplyMove(owner, (count % 2 ? alternate : cookpot)->GetPosition());
        Require(UseItem(owner, BreadedFrog, 102053, owner) == SPELL_CAST_OK, "The placed Cookpot did not accept its genuine frog");
        Tick(owner, 6500);
        Require(owner.GetItemCount(CrunchyFrog) == count + 1 && owner.GetItemCount(BreadedFrog) == 4 - count,
            "The native Cookpot result and charge are inconsistent");
    }
    Require(owner.GetQuestStatus(Cooking) == QUEST_STATUS_COMPLETE, "Five actual Cookpot casts did not complete the quest");
    TurnIn(owner, *trainer, Cooking);
    Require(!owner.GetItemCount(CrunchyFrog) && !owner.GetItemCount(BreadedFrog) && !owner.GetItemCount(PlumpFrogs),
        "Cooking reward retained a quest ingredient or result");
    std::cout << "PASS native breading, two actual Cookpots, missing target, five outputs and Stamp reward\n";
#endif
}

void ApproachTonk(LootPlayer& player, Creature const& tonk)
{
    for (auto const& offset : std::array<std::pair<float, float>, 9>{{
        {2.0f, 0.0f}, {-2.0f, 0.0f}, {0.0f, 2.0f}, {0.0f, -2.0f}, {2.0f, 2.0f},
        {-2.0f, -2.0f}, {3.5f, 0.0f}, {0.0f, 3.5f}, {0.0f, 0.0f}}})
    {
        float const x = tonk.GetPositionX() + offset.first;
        float const y = tonk.GetPositionY() + offset.second;
        float z = tonk.GetPositionZ();
        player.UpdateAllowedPositionZ(x, y, z);
        SupplyMove(player, Position{x, y, z, tonk.GetOrientation()});
        if (player.IsWithinLOSInMap(&tonk))
            return;
    }
    Require(false, "The real tonk has no nearby floor-supported repair position");
}

void CheckTonks(Map& map)
{
    auto trainer = PlaceActor(map, 14841, 2140);
    auto places = WorldDatabase.Query("SELECT guid FROM creature WHERE id=54504 AND map=974 AND zoneId=5861 ORDER BY guid");
    Require(places && places->GetRowCount() == 15, "Missing actual damaged tonk placements");
    LootPlayer owner(SupplySession(), map, 2951101, trainer->GetPosition()), other(SupplySession(), map, 2951102, trainer->GetPosition());
    InitializePlayer(owner, 202);
    InitializePlayer(other, 202);
    GiveSupply(other, Wrench, 1);
    Accept(owner, *trainer, Engineering);
    uint32 tested = 0, inspected = 0;
    do
    {
        auto tonk = PlaceActor(map, DamagedTonk, places->Fetch()[0].GetUInt32());
        SupplyMove(owner, tonk->GetPosition());
        SupplyMove(other, tonk->GetPosition());
#ifdef DARKMOON_FAIR_TARGETS_BASELINE
        tonk->AI()->UpdateAI(11000);
        tonk->Unit::Update(100);
        Require(tonk->HasAura(DamageAura), "The delayed baseline damaged state did not appear");
        Require(tonk->HandleSpellClick(&owner), "The original click no longer reports handled for the actual owner");
        tonk->Unit::Update(6000);
        Tick(owner, 6500);
        Require(!owner.GetQuestObjectiveData(Engineering, DamagedTonk), "The original wrong-caster click now awards the player credit");
        Require(tonk->HandleSpellClick(&other), "The original inactive click no longer reaches the wrong caster");
        std::cout << "EXPECTED_BROKEN the old row admitted an inactive click and reported a handled owner click without repair credit\n";
        return;
#else
        ++inspected;
        tonk->Unit::Update(100);
        Require(tonk->HasAura(DamageAura), "A new tonk was not initially damaged");
        ApproachTonk(owner, *tonk);
        ApproachTonk(other, *tonk);
        if (tested == 5)
        {
            Require(!tonk->HandleSpellClick(&owner), "A completed quest still operated a new damaged tonk");
            continue;
        }
        if (tested == 0)
        {
            auto& active = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
            active.erase(Faire);
            Require(!tonk->HandleSpellClick(&owner) && UseItem(owner, Wrench, Repair, *tonk) != SPELL_CAST_OK && tonk->HasAura(DamageAura),
                "An in-progress quest could repair its tonk after the Faire ended");
            active.insert(Faire);
        }
        Require(!tonk->HandleSpellClick(&other) && UseItem(other, Wrench, Repair, *tonk) != SPELL_CAST_OK,
            "A player without the active quest can still use the wrench or its click route");
        Require(UseItem(owner, Wrench, Repair, other) != SPELL_CAST_OK && owner.CastSpell(tonk.get(), Repair, false) != SPELL_CAST_OK,
            "An unrelated target or direct unreceipted spell cast was accepted");
        if (tested % 2)
        {
            WorldPackets::Spells::SpellClick packet{WorldPacket(CMSG_SPELL_CLICK)};
            packet.SpellClickUnitGuid = tonk->GetGUID();
            owner.GetSession()->HandleSpellClick(packet);
        }
        else
            Require(UseItem(owner, Wrench, Repair, *tonk) == SPELL_CAST_OK, "The original wrench refused the damaged tonk");
        Tick(owner, 6500);
        tonk->Unit::Update(100);
        ++tested;
        uint32 const credit = owner.GetQuestObjectiveData(Engineering, DamagedTonk);
        if (credit != tested || tonk->HasAura(DamageAura))
            std::cout << "CASE tonk=" << tonk->GetGUIDLow() << " step=" << tested << " owner-credit=" << credit
                << " damage-aura=" << tonk->HasAura(DamageAura) << '\n';
        Require(credit == tested && !tonk->HasAura(DamageAura),
            "A native repair did not remove the damage and give the correct player exactly one credit");
        Require(!tonk->HandleSpellClick(&owner), "A repaired tonk accepted a duplicate click");
#endif
    } while (places->NextRow());
#ifndef DARKMOON_FAIR_TARGETS_BASELINE
    Require(inspected == 15 && tested == 5 && owner.GetQuestStatus(Engineering) == QUEST_STATUS_COMPLETE && owner.GetItemCount(Wrench) == 1,
        "Five actual repairs did not complete the quest with the reusable tool intact");
    TurnIn(owner, *trainer, Engineering);
    Require(!owner.GetItemCount(Wrench), "The Engineering receiver did not take the wrench back");
    std::cout << "PASS fifteen reachable tonks, five actual repairs using item and click, wrong target and repeat denial, and Rinling reward\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{974, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{974, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Fair tests require an isolated character schema");
        LoadFairActors();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        auto& active = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
        active.insert(Faire);
        {
            Map map(974, 580, 0, DIFFICULTY_NONE);
            CheckFrogs(map);
            CheckTonks(map);
        }
        active.erase(Faire);
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
