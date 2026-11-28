#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"

#include "SmartScriptMgr.h"
#include "SpellPackets.h"

void AddSC_darkshore();

namespace
{
constexpr uint32 Ritual = 13566;

struct MaterialCase
{
    uint32 Actor;
    uint32 Spell;
    uint32 Item;
    uint32 Placements[3];
};

constexpr MaterialCase Materials[] = {{33127,62512,44969,{121797,121798,121799}},
    {33313,62788,45027,{122071,122072,122073}}, {33978,64116,45885,{122171,122172,122173}}};

Quest const* LoadRitual()
{
    auto rows = WorldDatabase.Query(
#include "NativeRitualMaterials.inc"
    );
    Require(rows && rows->GetRowCount() == 1, "Missing the original Ritual Materials quest");
    auto quest = new Quest(rows->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Ritual] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Ritual, quest);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=13566 ORDER BY ObjectID");
    Require(objectives && objectives->GetRowCount() == 3, "Missing the three original item counters");
    for (auto const& material : Materials)
    {
        auto objective = objectives->Fetch();
        Require(objective[2].GetUInt8() == QUEST_OBJECTIVE_ITEM && objective[4].GetUInt32() == material.Item &&
            objective[5].GetUInt32() == 3, "Expected a genuine three-item objective");
        quest->LoadQuestObjective(objective);
        objectives->NextRow();
        SpellInfo const* source = sSpellMgr->GetSpellInfo(material.Spell);
        Require(source && source->GetEffect(EFFECT_0)->Effect == SPELL_EFFECT_CREATE_ITEM &&
            source->GetEffect(EFFECT_0)->ItemType == material.Item, "Build 26972 pickup identity differs");
    }
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(121646,121797,121798,121799,122071,122072,122073,122171,122172,122173)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells WHERE npc_entry IN(33127,33313,33978)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup IN(33127,33313,33978)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 AND entryorguid IN(33127,33313,33978)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id IN(62512,62788,64116)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=13566");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sObjectMgr->LoadNPCSpellClickSpells();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sScriptDataStore->LoadSpellScriptNames();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_darkshore();
    sScriptMgr->SwapScriptContext(true);
    sScriptDataStore->ValidateSpellScripts();
    sConditionMgr->LoadConditions();
    return quest;
}

struct PlacedMaterialSource : Creature
{
    PlacedMaterialSource(Map& map, uint32 guid) : Creature(false)
    {
        auto spawn = sObjectMgr->GetCreatureData(guid);
        Require(spawn, "Missing original Darkshore placement");
        map.LoadGrid(spawn->posX, spawn->posY);
        Require(LoadCreatureFromDB(guid, &map, true) && IsInWorld() && GetZoneId() == 148 && AI(),
            "Original Darkshore actor did not load");
    }

    ~PlacedMaterialSource()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        if (FindMap())
            ResetMap();
    }
};

WorldSession* RitualSession()
{
    return new WorldSession(0, "Ritual Materials native probe", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
}

void MoveNear(LootPlayer& player, Creature& source)
{
    Position pos = source.GetNearPosition(1.0f, 0.0f);
    player.GetMap()->PlayerRelocation(&player, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
}

void Click(LootPlayer& player, Creature& source)
{
    WorldPackets::Spells::SpellClick packet{WorldPacket(CMSG_SPELL_CLICK)};
    packet.SpellClickUnitGuid = source.GetGUID();
    player.GetSession()->HandleSpellClick(packet);
}

void CheckMaterials(Quest const* quest)
{
    for (auto const& material : Materials)
        for (uint32 spawn : material.Placements)
            if (auto data = sObjectMgr->GetCreatureData(spawn))
                sObjectMgr->RemoveCreatureFromGrid(spawn, data);
    if (auto keeper = sObjectMgr->GetCreatureData(121646))
        sObjectMgr->RemoveCreatureFromGrid(121646, keeper);
    Map map(1, 300, 0, DIFFICULTY_NONE);
    PlacedMaterialSource keeper(map, 121646);
    LootPlayer owner(RitualSession(), map, 1356601, keeper.GetPosition()),
        visitor(RitualSession(), map, 1356602, keeper.GetPosition());
    for (auto player : {&owner, &visitor})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_NIGHTELF);
        player->setFactionForRace(RACE_NIGHTELF);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE, UNIT_FIELD_MOD_HASTE_REGEN})
            player->SetFloatValue(field, 1.0f);
    }
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    Require(owner.GetQuestStatus(Ritual) == QUEST_STATUS_INCOMPLETE, "The real quest did not enter the accepted state");

    for (auto const& material : Materials)
    {
        PlacedMaterialSource first(map, material.Placements[0]);
        MoveNear(owner, first);
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        {
            if (owner.GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                continue;
            ItemPosCountVec position{{uint16(INVENTORY_SLOT_BAG_0 << 8 | slot), 1}};
            Require(owner.StoreNewItem(position, 2589, true), "Could not fill the ordinary inventory");
        }
        ItemPosCountVec inventory;
        Require(owner.CanStoreNewItem(NULL_BAG, NULL_SLOT, inventory, material.Item, 1) == EQUIP_ERR_INV_FULL,
            "The test player still has space for the first material of this species");
        Click(owner, first);
        Require(!owner.GetItemCount(material.Item), "Full inventory received a material");
#ifdef RITUAL_MATERIALS_BASELINE
        first.AI()->UpdateAI(50);
        first.AI()->UpdateAI(5100);
        Require(!first.IsAlive(), "The original click should remove an animal even when the item does not fit");
#else
        first.AI()->UpdateAI(1100);
        Require(first.IsAlive() && !first.IsDespawn() && !first.getVictim() && owner.canSeeSpellClickOn(&first),
            "Failed storage removed the source, woke it or disabled retry");
        MoveNear(visitor, first);
        Click(visitor, first);
        Require(!visitor.GetItemCount(material.Item) && first.IsAlive(), "An unaccepted quest got a material");
#endif
        owner.DestroyItemCount(2589, INVENTORY_SLOT_ITEM_END - INVENTORY_SLOT_ITEM_START, true);

#ifndef RITUAL_MATERIALS_BASELINE
        auto collect = [&](PlacedMaterialSource& source, uint32 expected)
        {
            MoveNear(owner, source);
            source.RemoveCreatureSpellCooldown(material.Spell);
            source.GetGlobalCooldownMgr().CancelGlobalCooldown(sSpellMgr->GetSpellInfo(material.Spell));
            Click(owner, source);
            Require(owner.GetItemCount(material.Item) == expected && source.IsAlive() && !source.IsDespawn(),
                "Successful gathering did not store exactly one material while keeping its source in the world");
            Click(owner, source);
            Require(owner.GetItemCount(material.Item) == expected && !owner.canSeeSpellClickOn(&source),
                "The same source supplied a second material to this attempt");
            if (material.Actor == 33127)
            {
                Require(!source.getVictim(), "Moonstalker attacked before its wake delay");
                source.AI()->UpdateAI(1100);
                Require(source.getVictim() == &owner && source.IsAlive(), "Moonstalker did not wake and attack its collector");
            }
            else
            {
                source.AI()->UpdateAI(1100);
                Require(!source.getVictim() && source.IsAlive(), "The doe or feeding bear was removed or forced into combat");
            }
        };
        collect(first, 1);
        if (material.Actor == 33313)
        {
            visitor.AddQuestAndCheckCompletion(quest, nullptr);
            MoveNear(visitor, first);
            Require(visitor.canSeeSpellClickOn(&first), "One player's sample blocked another player");
            Click(visitor, first);
            Require(visitor.GetItemCount(material.Item) == 1 && owner.GetItemCount(material.Item) == 1,
                "The second player's native sample was lost or shared");
            auto slot = visitor.FindQuestSlot(Ritual);
            visitor.RemoveActiveQuest(Ritual);
            visitor.SetQuestSlot(slot, 0);
            visitor.DestroyItemCount(material.Item, 1, true);
            visitor.AddQuestAndCheckCompletion(quest, nullptr);
            Require(visitor.canSeeSpellClickOn(&first), "An abandoned attempt blocked the same animal on retake");
            slot = visitor.FindQuestSlot(Ritual);
            visitor.RemoveActiveQuest(Ritual);
            visitor.SetQuestSlot(slot, 0);
        }
        for (unsigned index = 1; index < 3; ++index)
        {
            PlacedMaterialSource next(map, material.Placements[index]);
            collect(next, index + 1);
        }
#endif
    }

#ifdef RITUAL_MATERIALS_BASELINE
    std::cout << "EXPECTED_BROKEN all three original material clicks remove a live source with full inventory\n";
#else
    MoveNear(owner, keeper);
    Require(owner.GetQuestStatus(Ritual) == QUEST_STATUS_COMPLETE && keeper.hasInvolvedQuest(Ritual) &&
        owner.CanRewardQuest(quest, false), "The nine native samples did not permit turn-in at Keeper Karithus");
    std::cout << "PASS Ritual Materials nine native samples, failed-storage retry, Moonstalker wake, separate collectors and retake\n";
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
        auto login = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
        Require(login.substr(login.find_last_of(';') + 1) == "fixture_auth", "Expected a disposable realm");
        LoginDatabase.SetConnectionInfo(login, 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Expected disposable characters");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto fixture = CharacterDatabase.Query("SELECT COUNT(*) FROM characters");
        Require(fixture && !fixture->Fetch()[0].GetUInt32(), "Expected an empty character fixture");
        Quest const* quest = LoadRitual();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckMaterials(quest);
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
