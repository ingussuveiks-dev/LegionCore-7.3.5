#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "QuestObjectScope.h"
#include "SmartAI.h"
#include "SmartScriptMgr.h"

void AddSC_darkshore();
void AddSC_SmartScripts();

namespace
{
constexpr uint32 Terminal = 13918;
constexpr uint32 Detector = 46388;
constexpr uint32 Fragment = 46702;
constexpr uint32 Device = 46387;
constexpr uint32 Dig = 65300;
constexpr uint32 Assemble = 65435;
constexpr uint32 Debris = 195055;
constexpr uint32 Marker = 34356;
constexpr uint32 Ferd = 34342;

void ShadowTerminalRows(char const* table, char const* restriction)
{
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE terminal_%s AS SELECT * FROM %s WHERE %s",
        table, table, restriction).c_str());
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE %s AS SELECT * FROM terminal_%s", table, table).c_str());
}

Quest const* LoadTerminal()
{
    auto row = WorldDatabase.Query(
#include "NativeTitansTerminal.inc"
    );
    Require(bool(row), "Missing The Titans' Terminal quest");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Terminal] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Terminal, quest);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=13918 ORDER BY ObjectID");
    Require(objectives && objectives->GetRowCount() == 2 && objectives->Fetch()[4].GetUInt32() == Device,
        "Missing the assembled device objective");
    quest->LoadQuestObjective(objectives->Fetch());
    objectives->NextRow();
    Require(objectives->Fetch()[4].GetUInt32() == Detector, "Missing return of the provided detector");
    quest->LoadQuestObjective(objectives->Fetch());
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=13918");
    Require(addon && addon->Fetch()[4].GetUInt32() == 13912, "Missing the original previous quest");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    Require(quest->SourceItemId == Detector && quest->SourceItemIdCount == 1, "The quest must provide the original detector");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_darkshore();
    AddSC_SmartScripts();
    sScriptMgr->SwapScriptContext(true);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    ShadowTerminalRows("conditions", "SourceEntry IN (65300,65435,46702)");
    ShadowTerminalRows("smart_scripts", "source_type=0 AND entryorguid=34356");
    ShadowTerminalRows("spell_script_names", "spell_id=65300");
    ShadowTerminalRows("creature_questender", "quest=13918");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sScriptDataStore->LoadSpellScriptNames();
    sScriptDataStore->ValidateSpellScripts();
    auto& table = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto original = table;
    table += " WHERE Entry=195055";
    auto loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    table = original;
    Require(loaded == 4, "Missing the four actual Buried Debris loot rows");
    sConditionMgr->LoadConditions();
    Require(sSpellMgr->GetSpellInfo(Dig)->CastingReq.RequiresSpellFocus == 1612, "Detector lost the original beach focus");
    return quest;
}

struct BeachMarker : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(new SmartAI(this)), "The original beach-marker AI could not initialize");
    }
};

struct BeachFocus : GameObject
{
    ~BeachFocus()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

std::pair<uint32, Position> PlacedActor(uint32 entry, uint32 guid)
{
    auto row = WorldDatabase.PQuery("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=%u AND guid=%u AND map=1 AND zoneId=148", entry, guid);
    Require(bool(row), "Missing a declared original beach actor");
    auto f = row->Fetch();
    return {f[0].GetUInt32(), {f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat(),f[4].GetFloat()}};
}

std::vector<GameObject*> OwnDebris(LootPlayer& player)
{
    std::list<GameObject*> candidates;
    player.GetGameObjectListWithEntryInGrid(candidates, Debris, 50.0f);
    std::vector<GameObject*> owned;
    for (auto object : candidates)
        if (object->GetSummonerGUID() == player.GetGUID() && object->IsInWorld() && !object->IsPreDelete() &&
            object->isSpawned() && object->getLootState() != GO_JUST_DEACTIVATED && !object->IsQuestObjectExpired())
            owned.push_back(object);
    return owned;
}

void CheckTerminal(Quest const* quest)
{
    auto markerCount = WorldDatabase.Query("SELECT COUNT(*) FROM creature WHERE id=34356 AND map=1 AND zoneId=148");
    Require(markerCount && markerCount->Fetch()[0].GetUInt32() == 90, "Original beach marker placement changed");
    Require(bool(WorldDatabase.Query("SELECT 1 FROM creature_queststarter WHERE quest=13918 AND id=34342")), "Ferd is missing the original offer");
    auto main = PlacedActor(Marker, 122462), otherMarker = PlacedActor(Marker, 122459), receiver = PlacedActor(Ferd, 122370);
    Position searchPoint{(main.second.GetPositionX() * 0.55f + otherMarker.second.GetPositionX() * 0.45f),
        (main.second.GetPositionY() * 0.55f + otherMarker.second.GetPositionY() * 0.45f),
        (main.second.GetPositionZ() + otherMarker.second.GetPositionZ()) * 0.5f};
    Map map(1, 300, 0, DIFFICULTY_NONE);
    BeachMarker nearest(&map, Marker, main.first, main.second), farther(&map, Marker, otherMarker.first, otherMarker.second);
    Require(map.AddToMap(static_cast<Creature*>(&nearest)) && map.AddToMap(static_cast<Creature*>(&farther)), "Could not publish native buried markers");
    Actor ferd(&map, Ferd, receiver.first, receiver.second);
    Require(map.AddToMap(static_cast<Creature*>(&ferd)), "Could not publish Ferd at his live placement");
    auto row = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE guid=101171 AND id=301014 AND map=1");
    Require(bool(row), "Missing the original beach focus");
    auto f = row->Fetch();
    Position point{f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat(),f[4].GetFloat()};
    BeachFocus focus;
    Require(focus.Create(f[0].GetUInt32(), 301014, &map, 1, point,
        G3D::Quat(f[5].GetFloat(),f[6].GetFloat(),f[7].GetFloat(),f[8].GetFloat()), 255, GO_STATE_READY) &&
        map.AddToMap(static_cast<GameObject*>(&focus)), "Could not publish the actual spell focus");
    LootPlayer owner(SupplySession(), map, 13918, searchPoint), other(SupplySession(), map, 13919, searchPoint);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_NIGHTELF);
        player->setFactionForRace(RACE_NIGHTELF);
        player->InitDisplayIds();
        player->SetLevel(18);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    auto use = [](LootPlayer& player, uint32 item, uint32 spell)
    {
        auto castItem = player.GetItemByEntry(item);
        Require(castItem, "Expected the original carried item");
        return player.CastSpell(&player, spell, false, castItem);
    };
    GiveSupply(other, Detector, 1);
    Require(use(other, Detector, Dig) != SPELL_CAST_OK, "A retained detector was usable without the quest");
    other.DestroyItemCount(Detector, 1, true);
    owner.AddQuestAndCheckCompletion(quest, &ferd);
    other.AddQuestAndCheckCompletion(quest, &ferd);
    SupplyMove(owner, receiver.second);
    Require(use(owner, Detector, Dig) != SPELL_CAST_OK, "Detector activated at the quest giver outside the beach focus");
    SupplyMove(owner, searchPoint);
    auto dig = [&](LootPlayer& player)
    {
        // The existing SmartAI exposes debris on a 25% roll. Exercise the
        // actual cast path repeatedly without changing the authored odds.
        for (uint32 cast = 0; cast < 50; ++cast)
        {
            Require(use(player, Detector, Dig) == SPELL_CAST_OK, "The real beach detector cast was refused");
            Tick(player, 5000);
            if (!OwnDebris(player).empty())
                break;
        }
        auto results = OwnDebris(player);
        Require(results.size() == 1 && results.front()->GetExactDist2d(nearest) < 0.1f, "Detector did not expose just the nearest private debris");
        results.front()->Update(3000);
        return results.front();
    };
    GameObject* first = dig(owner);
    for (uint32 attempt = 0; attempt < 20; ++attempt)
        Require(dig(owner) == first, "Repeated detection stacked up private debris in one location");
    GameObject* second = dig(other);
    auto ownerScope = first->GetQuestObjectScope().lock();
    Require(second != first && ownerScope && ownerScope->Owner == owner.GetGUID() && first->CanCollideWith(&owner) &&
        !first->CanCollideWith(&other) && !first->CanTargetQuestObject(&other), "The two private dig results leaked access or collision");
    SupplyMove(other, main.second);
    other.CastSpell(first, 3365, false);
    Tick(other, 6000);
    Require(!other.GetLootGUID(), "Another player opened the private loot from interaction distance");
    auto secondGuid = second->GetGUID();
    AbandonSupply(other, Terminal);
    map.RemoveAllObjectsInRemoveList();
    Require(!map.GetGameObject(secondGuid), "Abandon left the scoped debris active");
    Require(!other.GetItemCount(Detector), "Abandon retained the detector provided by the quest");
    GiveSupply(other, Fragment, 5);
    Require(use(other, Fragment, Assemble) != SPELL_CAST_OK && other.GetItemCount(Fragment) == 5,
        "An abandoned quest could assemble retained fragments");
    other.AddQuestAndCheckCompletion(quest, &ferd);
    Require(use(other, Fragment, Assemble) == SPELL_CAST_OK, "The new attempt could not use a complete fragment stack");
    Tick(other, 5000);
    Require(other.GetItemCount(Device) == 1, "The second assembly did not produce the actual device");
    AbandonSupply(other, Terminal);
    Require(!other.GetItemCount(Device) && !other.GetItemCount(Detector), "Abandon retained a fully assembled quest device");

    for (unsigned attempt = 0; attempt < 120 && owner.GetItemCount(Fragment) < 5; ++attempt)
    {
        SupplyMove(owner, searchPoint);
        GameObject* debris = dig(owner);
        SupplyMove(owner, debris->GetPosition());
        auto guid = debris->GetGUID();
        std::cout << "CASE loot attempt=" << attempt << " debris=" << guid.GetCounter() << " state=" << unsigned(debris->getLootState())
            << " live=" << (map.GetGameObject(guid) == debris) << " carried=" << owner.GetItemCount(Fragment) << std::endl;
        OpenSupply(owner, *debris);
        Require(owner.GetLootGUID() == guid, "The real debris lock did not open its loot");
        WorldPackets::Loot::LootResponse response;
        debris->loot.BuildLootResponse(response, &owner, ALL_PERMISSION);
        if (!response.Items.empty())
        {
            WorldPackets::Loot::AutoStoreLootItem packet{WorldPacket(CMSG_LOOT_ITEM)};
            for (auto const& option : response.Items)
                packet.Loot.push_back({debris->loot.GetGUID(), option.LootListID});
            owner.GetSession()->HandleAutostoreLootItemOpcode(packet);
        }
        ReleaseSupply(owner, *debris);
        debris->Update(3000);
        map.RemoveAllObjectsInRemoveList();
        for (uint32 junk : {46390, 46391, 46703})
            if (owner.GetItemCount(junk))
                owner.DestroyItemCount(junk, owner.GetItemCount(junk), true);
        if (owner.GetItemCount(Fragment) == 4)
            Require(use(owner, Fragment, Assemble) != SPELL_CAST_OK && owner.GetItemCount(Fragment) == 4,
                "Assembly accepted an incomplete fragment stack");
    }
    Require(owner.GetItemCount(Fragment) == 5 && !owner.CanRewardQuest(quest, false), "Native loot did not produce the five required fragments");
    Require(use(owner, Fragment, Assemble) == SPELL_CAST_OK, "Five actual fragments did not start native assembly");
    Tick(owner, 5000);
    Require(owner.GetItemCount(Device) == 1 && !owner.GetItemCount(Fragment) && owner.CanRewardQuest(quest, false),
        "Native assembly failed to consume five fragments or produce the final device");
    SupplyMove(owner, receiver.second);
    uint32 reward = 0;
    if (auto choices = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (auto item : *choices)
            if (owner.CanSelectQuestPackageItem(item))
            {
                reward = item->ItemID;
                break;
            }
    Require(reward, "Missing eligible native reward");
    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = ferd.GetGUID();
    packet.QuestID = Terminal;
    packet.ItemChoiceID = reward;
    owner.GetSession()->HandleQuestGiverChooseReward(packet);
    Require(owner.GetQuestRewardStatus(Terminal) && owner.GetItemCount(reward) && !owner.GetItemCount(Device) && !owner.GetItemCount(Detector),
        "Ferd's reward packet did not clean the device and detector from inventory");
    std::cout << "PASS real site focus and nearest scoped debris, five native loot fragments, assembly and Ferd reward cleanup\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The excavation test requires the isolated character schema");
        auto quest = LoadTerminal();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckTerminal(quest);
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
