#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

struct ForestSpellConditionTag
{
    using Type = ConditionContainer ConditionMgr::*;
    friend Type Access(ForestSpellConditionTag);
};
template struct FixtureAccess<ForestSpellConditionTag, &ConditionMgr::ConditionStore>;

namespace
{
constexpr uint32 ForestQuest = 13796;
constexpr uint32 Spirit = 45571;
constexpr uint32 Key = 45572;
constexpr uint32 Heart = 45573;
constexpr uint32 Combine = 63682;
constexpr uint32 Unlock = 63697;

Quest const* LoadForest()
{
    auto row = WorldDatabase.Query(
#include "NativeForestHeartQuest.inc"
    );
    Require(row && row->GetRowCount() == 1, "Missing Forest Heart quest");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[ForestQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(ForestQuest, quest);
    auto objective = WorldDatabase.Query(
        "SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=13796");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == Heart, "Missing Forest Heart objective");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    Require(quest->ItemDrop[1] == Key && quest->ItemDropQuantity[1] == 1 &&
        quest->ItemDrop[2] == Spirit && quest->ItemDropQuantity[2] == 7, "Original key or spirit quantity changed");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto loadLoot = [](LootStore& store, char const* selection)
    {
        auto& table = store.*Access(LootTableNameTag{});
        auto original = table;
        table += selection;
        auto count = (store.*Access(LoadLootTableTag{}))();
        table = original;
        return count;
    };
    Require(loadLoot(LootTemplates_Gameobject, " WHERE Entry=26899") == 1, "Missing real Forest Heart chest loot");
    auto rows = loadLoot(LootTemplates_Creature, " WHERE Entry IN(3799,3803) AND Item IN(45571,45573)");
    auto conds = WorldDatabase.Query("SELECT SourceEntry,ConditionTypeOrReference,ConditionValue1 FROM conditions "
        "WHERE SourceTypeOrReferenceId=17 AND SourceEntry IN(63682,63697) AND SourceGroup=0 AND ElseGroup=0 AND ConditionTarget=0");
#ifdef FOREST_HEART_BASELINE
    Require(rows == 4 && !conds, "Counterfactual fixture no longer has the original routes");
#else
    Require(rows == 2 && conds && conds->GetRowCount() == 2, "Missing actual source and quest admission migration");
    do
    {
        auto f = conds->Fetch();
        auto condition = new Condition;
        condition->SourceType = CONDITION_SOURCE_TYPE_SPELL;
        condition->SourceEntry = f[0].GetUInt32();
        condition->ConditionType = ConditionTypes(f[1].GetUInt32());
        condition->ConditionValue1 = f[2].GetUInt32();
        Require(condition->ConditionType == CONDITION_QUESTTAKEN && condition->ConditionValue1 == ForestQuest &&
            (sConditionMgr->*Access(ValidateSourceTag{}))(condition) && sConditionMgr->isConditionTypeValid(condition),
            "Native spell condition did not validate");
        (sConditionMgr->*Access(ForestSpellConditionTag{}))[condition->SourceType][condition->SourceEntry].push_back(condition);
    } while (conds->NextRow());
#endif
    auto recipe = sSpellMgr->GetSpellInfo(Combine);
    Require(recipe->Reagents.Reagent[0] == Spirit && recipe->Reagents.ReagentCount[0] == 6 &&
        recipe->GetEffect(EFFECT_0)->ItemType == Key, "Original combination should use six reagents and the cast item");
    return quest;
}

Placement ForestObject(uint32 entry)
{
    auto rows = WorldDatabase.PQuery(
        "SELECT guid,spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=%u AND map=1 ORDER BY guid LIMIT 1", entry);
    Require(bool(rows), "Missing Forest Heart or control object placement");
    auto f = rows->Fetch();
    return {entry,f[0].GetUInt32(),f[1].GetUInt32(),{f[2].GetFloat(),f[3].GetFloat(),f[4].GetFloat(),f[5].GetFloat()},
        G3D::Quat(f[6].GetFloat(),f[7].GetFloat(),f[8].GetFloat(),f[9].GetFloat())};
}

uint32 ForestVictim = 41100;
void ForestKill(Map& map, LootPlayer& owner, Unit& killer, uint32 entry, bool expectSpirit, bool expectHeart)
{
    auto row = WorldDatabase.PQuery(
        "SELECT position_x,position_y,position_z,orientation FROM creature WHERE id=%u AND map=1 AND zoneId=331 ORDER BY guid LIMIT 1", entry);
    Require(bool(row), "Missing living Severed Druid or Keeper source");
    auto f = row->Fetch();
    Position point{f[0].GetFloat(),f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat()};
    SupplyMove(owner, point);
    Actor victim(&map, entry, ++ForestVictim, point);
    Require(map.AddToMap(static_cast<Creature*>(&victim)), "Could not publish original spirit source");
    victim.SetLootRecipient(&owner);
    victim.LowerPlayerDamageReq(victim.GetMaxHealth());
    killer.Kill(&victim, false);
    Tick(owner, 100);
    Require(victim.isDead(false), "The native kill did not reach loot");
    bool spirit = false;
    bool heart = false;
    for (Loot* cache : {&victim.loot, owner.GetPersonalLoot(victim.GetGUID())})
    {
        if (!cache)
            continue;
        WorldPackets::Loot::LootResponse view;
        cache->BuildLootResponse(view, &owner, ALL_PERMISSION);
        for (auto const& loot : view.Items)
        {
            if (loot.Loot.ItemID == Spirit)
            {
                Require(!spirit, "One corpse exposed duplicate spirits");
                spirit = true;
                auto count = owner.GetItemCount(Spirit);
                WorldPackets::Loot::AutoStoreLootItem packet{WorldPacket(CMSG_LOOT_ITEM)};
                packet.Loot.push_back({cache->GetGUID(), loot.LootListID});
                owner.GetSession()->HandleAutostoreLootItemOpcode(packet);
                Require(owner.GetItemCount(Spirit) == count + 1, "Native spirit loot did not enter the bag");
            }
            if (loot.Loot.ItemID == Heart)
                heart = true;
        }
    }
    std::cout << "CASE corpse=" << entry << " quest=" << unsigned(owner.GetQuestStatus(ForestQuest))
        << " spirit=" << spirit << " heart=" << heart << '\n';
    Require(spirit == expectSpirit && heart == expectHeart, "Incorrect spirit/finished Heart corpse loot");
}

SpellCastResult ForgeKey(LootPlayer& player)
{
    auto spirit = player.GetItemByEntry(Spirit);
    Require(spirit, "Missing the actual use item");
    auto result = player.CastSpell(&player, Combine, false, spirit);
    Tick(player, 8000);
    return result;
}

SpellCastResult UseKey(LootPlayer& player, Supply& supply, Item* key)
{
    SupplyMove(player, supply.GetPosition());
    auto result = player.CastSpell(&supply, Unlock, false, key);
    Tick(player, 8000);
    return result;
}

void ForestRoute(Quest const* quest)
{
    auto placement = ForestObject(194549);
    Map map(1, 300, 0, DIFFICULTY_NONE);
    LootPlayer owner(SupplySession(), map, 13796, placement.position), other(SupplySession(), map, 13797, placement.position);
    for (auto player : {&owner,&other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_NIGHTELF);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(RACE_NIGHTELF);
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, sObjectMgr->GetXPForLevel(20));
    }
    Supply chest(map, placement);
    Require(chest.GetGOInfo()->GetLockId() == 1928 && chest.GetGOInfo()->GetLootId() == 26899, "Original Heart lock or loot ID changed");
    auto lock = sLockStore.LookupEntry(1928);
    Require(lock && lock->Type[0] == LOCK_KEY_ITEM && lock->Index[0] == Key, "Original Heart lock does not admit its key");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
#ifdef FOREST_HEART_BASELINE
    ForestKill(map, other, other, 3799, true, false);
    ForestKill(map, owner, owner, 3799, true, true);
    ForestKill(map, owner, owner, 3803, true, true);
    std::cout << "EXPECTED_BROKEN unaccepted spirit loot and both corpses offer the finished Heart despite the native key requirement\n";
#else
    ForestKill(map, other, other, 3799, false, false);
    GiveSupply(other, Spirit, 7);
    GiveSupply(other, Key, 1);
    Require(ForgeKey(other) != SPELL_CAST_OK && other.GetItemCount(Spirit) == 7, "Unaccepted quest allowed the combination");
    Require(UseKey(other, chest, other.GetItemByEntry(Key)) != SPELL_CAST_OK && chest.getLootState() == GO_READY,
        "Unaccepted quest allowed the actual key to open the chest");
    LootPet pet(owner, map);
    for (uint32 i = 0; i < 6; ++i)
        ForestKill(map, owner, i % 2 ? static_cast<Unit&>(owner) : static_cast<Unit&>(pet), i % 2 ? 3799 : 3803, true, false);
    Require(owner.GetItemCount(Spirit) == 6 && ForgeKey(owner) != SPELL_CAST_OK && owner.GetItemCount(Spirit) == 6,
        "The native key could be made with six spirits");
    ForestKill(map, owner, owner, 3799, true, false);
    ForestKill(map, owner, pet, 3803, false, false);
    Require(owner.GetItemCount(Spirit) == 7 && ForgeKey(owner) == SPELL_CAST_OK &&
        !owner.GetItemCount(Spirit) && owner.GetItemCount(Key) == 1, "Seven real spirits did not become one key");
    Supply decoy(map, ForestObject(195042));
    Require(UseKey(owner, decoy, owner.GetItemByEntry(Key)) != SPELL_CAST_OK && owner.GetItemCount(Key) == 1,
        "Power of Nature was usable on the wrong lock");
    Require(UseKey(owner, chest, nullptr) != SPELL_CAST_OK && chest.getLootState() == GO_READY, "The Heart opened without the real key");
    Require(UseKey(owner, chest, owner.GetItemByEntry(Key)) == SPELL_CAST_OK && TakeSupply(owner, chest, Heart),
        "The key did not allow native loot from the Forest Heart");
    ReleaseSupply(owner, chest);
    chest.Rearm();
    Require(owner.CanRewardQuest(quest, false), "The real Forest Heart did not admit turn-in");
    owner.RewardQuest(quest, 0, nullptr, false);
    Require(owner.IsQuestRewarded(ForestQuest) && !owner.GetItemCount(Heart) && !owner.GetItemCount(Key) && !owner.GetItemCount(Spirit),
        "Forest Heart turn-in left quest pieces in bags");
    GiveSupply(owner, Key, 1);
    Require(UseKey(owner, chest, owner.GetItemByEntry(Key)) != SPELL_CAST_OK && chest.getLootState() == GO_READY,
        "A rewarded quest kept access to the Heart");
    other.AddQuestAndCheckCompletion(quest, nullptr);
    Require(UseKey(other, chest, other.GetItemByEntry(Key)) == SPELL_CAST_OK && TakeSupply(other, chest, Heart),
        "Second eligible player could not open restored Heart");
    ReleaseSupply(other, chest);
    AbandonSupply(other, ForestQuest);
    Require(!other.GetItemCount(Heart) && !other.GetItemCount(Key) && !other.GetItemCount(Spirit), "Abandon left forest quest pieces");
    std::cout << "PASS seven real spirit drops, native recipe, exact key lock, Forest Heart loot, reward and abandon cleanup\n";
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
        auto quest = LoadForest();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_go_scripts();
        sScriptMgr->SwapScriptContext(true);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
#ifndef FOREST_HEART_BASELINE
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Reward writes require a disposable character schema");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto fixture = CharacterDatabase.Query("SELECT DATABASE(), (SELECT COUNT(*) FROM characters)");
        Require(fixture && fixture->Fetch()[0].GetString() == "fixture_char" && !fixture->Fetch()[1].GetUInt32(), "Reward requires an empty fixture");
#endif
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        ForestRoute(quest);
#ifndef FOREST_HEART_BASELINE
        CharacterDatabase.Close();
#endif
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
