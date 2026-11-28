#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"

#include "SpellPackets.h"
#include "SmartScriptMgr.h"

void AddSC_hinterlands();

namespace
{
constexpr uint32 Spiderling = 42689;
constexpr uint32 Pickup = 79591;
constexpr uint32 ItemId = 58120;
constexpr uint32 HordeQuest = 26310;
constexpr uint32 AllianceQuest = 26523;

WorldSession* PickupSession()
{
    return new WorldSession(0, "Spiderling native probe", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
}

void LoadPickup()
{
    auto rows = WorldDatabase.Query(
#include "NativeSkitteringSpiderling.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "Missing the two original quests");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN(26310,26523)");
    Require(objectives && objectives->GetRowCount() == 2, "Missing native item objectives");
    do
    {
        auto objective = objectives->Fetch();
        Require(objective[2].GetUInt8() == QUEST_OBJECTIVE_ITEM && objective[4].GetUInt32() == ItemId &&
            objective[5].GetUInt32() == 8, "Original item or quota differs");
        auto quest = quests[objective[1].GetUInt32()];
        quest->LoadQuestObjective(objective);
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    } while (objectives->NextRow());

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(103768,103775)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells WHERE npc_entry=42689");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=42689");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE entryorguid=42689 AND source_type=0");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id=79591");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadNPCSpellClickSpells();
    sConditionMgr->LoadConditions();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sScriptDataStore->LoadSpellScriptNames();
#ifndef SKITTERING_SPIDERLING_BASELINE
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_hinterlands();
    sScriptMgr->SwapScriptContext(true);
#endif
    sScriptDataStore->ValidateSpellScripts();
    auto pickup = sSpellMgr->GetSpellInfo(Pickup);
    Require(pickup && pickup->GetEffect(EFFECT_0)->Effect == SPELL_EFFECT_CREATE_ITEM &&
        pickup->GetEffect(EFFECT_0)->ItemType == ItemId, "Build 26972 create-item spell differs");
}

struct PlacedSpiderling : Creature
{
    PlacedSpiderling(Map& map, uint32 spawn) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "Missing original Hinterlands placement");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true) && IsInWorld() && GetEntry() == Spiderling &&
            GetZoneId() == 47 && CanClick(), "Original spiderling did not load with its click flag");
    }

    bool CanClick() const { return HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK); }

    ~PlacedSpiderling()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        if (FindMap())
            ResetMap();
    }
};

void Click(LootPlayer& player, PlacedSpiderling& spiderling)
{
    WorldPackets::Spells::SpellClick packet{WorldPacket(CMSG_SPELL_CLICK)};
    packet.SpellClickUnitGuid = spiderling.GetGUID();
    player.GetSession()->HandleSpellClick(packet);
}

void Prepare(LootPlayer& player, uint8 race, uint32 quest)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
    player.setFactionForRace(race);
    player.SetQuestStatus(quest, QUEST_STATUS_INCOMPLETE);
    player.SetQuestSlot(0, quest);
    for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE, UNIT_FIELD_MOD_HASTE_REGEN})
        player.SetFloatValue(field, 1.0f);
}

void CheckPickup()
{
    for (uint32 spawn : {103768u, 103775u})
        if (auto data = sObjectMgr->GetCreatureData(spawn))
            sObjectMgr->RemoveCreatureFromGrid(spawn, data);
    Map map(0, 300, 0, DIFFICULTY_NONE);
    PlacedSpiderling first(map, 103768);
    LootPlayer owner(PickupSession(), map, 2631001, first.GetNearPosition(1.0f, 0.0f));
    Prepare(owner, RACE_ORC, HordeQuest);
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        ItemPosCountVec position{{uint16(INVENTORY_SLOT_BAG_0 << 8 | slot), 1}};
        Require(owner.StoreNewItem(position, 2589, true), "Could not fill the ordinary inventory");
    }
    ItemPosCountVec slots;
    Require(owner.CanStoreNewItem(NULL_BAG, NULL_SLOT, slots, ItemId, 1) == EQUIP_ERR_INV_FULL,
        "The test player still has somewhere to place the spiderling");
    Click(owner, first);
    first.Unit::Update(50);
    Require(!owner.GetItemCount(ItemId), "Full inventory received a spiderling");

#ifdef SKITTERING_SPIDERLING_BASELINE
    // The original SmartAI is invisible for five seconds before final despawn.
    first.AI()->UpdateAI(50);
    first.AI()->UpdateAI(5100);
    Require(!first.IsAlive(), "The original click should remove the source even with nowhere to store the item");
    std::cout << "EXPECTED_BROKEN full bags removed the original spiderling without storing the item\n";
#else
    Require(first.IsInWorld() && first.IsAlive() && first.CanClick(), "Failed storage consumed the spiderling or disabled retry");
    LootPlayer newcomer(PickupSession(), map, 2652301, first.GetNearPosition(1.0f, 0.0f));
    Click(newcomer, first);
    Require(!newcomer.GetItemCount(ItemId) && first.CanClick(), "An ineligible click consumed the original source");
    owner.DestroyItem(INVENTORY_SLOT_BAG_0, INVENTORY_SLOT_ITEM_START, true);
    first.RemoveCreatureSpellCooldown(Pickup);
    first.GetGlobalCooldownMgr().CancelGlobalCooldown(sSpellMgr->GetSpellInfo(Pickup));
    Click(owner, first);
    Require(owner.GetItemCount(ItemId) == 1 && first.IsDespawn(), "Horde retry did not store one item and retire the source");
    Click(owner, first);
    Require(owner.GetItemCount(ItemId) == 1 && !newcomer.GetItemCount(ItemId), "A repeat or foreign click gave extra credit");
    first.Unit::Update(50);
    Require(!first.IsAlive(), "Collected source did not despawn");
    first.Respawn();
    Require(first.IsAlive() && first.CanClick() && !first.IsDespawn(), "Normal source respawn did not restore pickup availability");

    PlacedSpiderling second(map, 103775);
    Position nearby = second.GetNearPosition(1.0f, 0.0f);
    map.PlayerRelocation(&newcomer, nearby.GetPositionX(), nearby.GetPositionY(), nearby.GetPositionZ(), nearby.GetOrientation());
    Prepare(newcomer, RACE_DWARF, AllianceQuest);
    Require(newcomer.AddItem(ItemId, 7) && newcomer.GetQuestStatus(AllianceQuest) == QUEST_STATUS_INCOMPLETE,
        "Could not prepare the last Alliance pickup");
    Click(newcomer, second);
    Require(newcomer.GetItemCount(ItemId) == 8 && newcomer.GetQuestStatus(AllianceQuest) == QUEST_STATUS_COMPLETE &&
        second.IsDespawn(), "The eighth native pickup did not complete the Alliance quest or retire its source");
    std::cout << "PASS native click packet, full-bag retry, ineligible click, duplicate click and both faction quests\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}});
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Respawn requires the disposable character schema");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto fixture = CharacterDatabase.Query("SELECT DATABASE(), (SELECT COUNT(*) FROM characters)");
        Require(fixture && fixture->Fetch()[0].GetString() == "fixture_char" && !fixture->Fetch()[1].GetUInt32(),
            "Respawn requires an empty character fixture");
        LoadPickup();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckPickup();
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
