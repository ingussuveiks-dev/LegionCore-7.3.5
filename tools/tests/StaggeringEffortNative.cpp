#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "PoolMgr.h"
#include <thread>

namespace
{
constexpr uint32 Daily = 26557;
constexpr uint32 Bait = 58949;
constexpr uint32 Carp = 58946;
constexpr uint32 BaitSpell = 80868;
constexpr uint32 Stag = 35096;
constexpr uint32 City = 1637;
constexpr uint32 ValleyOfWisdom = 5171;
constexpr uint32 ValleyOfHonor = 5168;
constexpr uint32 Razgar = 43239;
constexpr uint32 Bobber = 35591;

Quest const* LoadFishingDaily()
{
    auto rows = WorldDatabase.Query(
#include "NativeStaggeringEffort.inc"
    );
    Require(rows && rows->GetRowCount() == 1, "Missing the original fishing daily");
    auto quest = new Quest(rows->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Daily] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Daily, quest);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=26557");
    Require(objectives && objectives->GetRowCount() == 1 && objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM &&
        objectives->Fetch()[4].GetUInt32() == Carp && objectives->Fetch()[5].GetUInt32() == 1, "Sandy Carp objective changed");
    quest->LoadQuestObjective(objectives->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=26557");
    Require(addon && addon->Fetch()[9].GetUInt32() == 356, "Missing fishing skill requirement");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);

    auto rotation = WorldDatabase.Query("SELECT p.pool_entry,t.max_limit FROM pool_quest p JOIN pool_template t ON t.entry=p.pool_entry WHERE p.entry=26557");
    Require(rotation && rotation->Fetch()[0].GetUInt32() == 5680 && rotation->Fetch()[1].GetUInt32() == 1,
        "The real Orgrimmar fishing rotation changed");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();

    auto eye = sObjectMgr->GetItemTemplate(Bait);
    Require(eye && std::any_of(eye->Effects.begin(), eye->Effects.end(), [](ItemEffectEntry const* effect)
        { return effect && effect->SpellID == BaitSpell && effect->Charges == -1; }), "Stag Eye is no longer the original consumable");

    auto loadLoot = [](LootStore& store, char const* filter)
    {
        auto& name = store.*Access(LootTableNameTag{});
        std::string original = name;
        name += filter;
        auto count = (store.*Access(LoadLootTableTag{}))();
        name = original;
        return count;
    };
    Require(loadLoot(LootTemplates_Fishing, " WHERE Entry IN(1637,5171)") &&
        LootTemplates_Fishing.GetLootFor(City) && LootTemplates_Fishing.GetLootFor(ValleyOfWisdom),
        "Expected the original city and subzone fishing loot");
    Require(loadLoot(LootTemplates_Creature, " WHERE Entry=35096") && LootTemplates_Creature.GetLootFor(Stag),
        "Missing original stag loot group");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE "
        "(SourceTypeOrReferenceId=1 AND SourceGroup=35096) OR (SourceTypeOrReferenceId=3 AND SourceGroup IN(1637,5171))");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE spell=80868");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=26557");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=26557");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sConditionMgr->LoadConditions();
    sSpellMgr->LoadSpellAreas();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    Require(sQuestDataStore->GetCreatureQuestRelationBounds(Razgar).first == sQuestDataStore->GetCreatureQuestRelationBounds(Razgar).second,
        "The daily is available before selection by the pool");
    PoolGroup<Quest> daily;
    daily.SetPoolId(5680);
    PoolObject selection(Daily, 0.0f);
    daily.AddEntry(selection, 1);
    ActivePoolData active;
    daily.SpawnObject(active, 1, 1);
    Require(active.IsActiveObject<Quest>(Daily), "The real daily pool could not select this quest");
    return quest;
}

Position FindWater(Map& map, Position const& origin, uint32 areaId)
{
    map.LoadGrid(origin.GetPositionX(), origin.GetPositionY());
    for (int y = -62; y <= 62; y += 2)
        for (int x = -62; x <= 62; x += 2)
        {
            float fishX = origin.GetPositionX() + x;
            float fishY = origin.GetPositionY() + y;
            float level = map.GetWaterLevel(fishX, fishY);
            if (std::abs(level - origin.GetPositionZ()) > 12.0f || !map.IsInWater(fishX, fishY, level - 0.3f))
                continue;
            uint32 zone = 0, area = 0;
            map.GetZoneAndAreaId(zone, area, fishX, fishY, level);
            if (zone == City && area == areaId)
                return {fishX, fishY, level};
        }
    throw std::runtime_error("No native water found in audited Orgrimmar subzone");
}

struct FishingBobber : GameObject
{
    FishingBobber(Map& map, uint32 guid, Position const& position)
    {
        Require(Create(guid, Bobber, &map, 1, position, G3D::Quat(0, 0, 0, 1), 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "Native fishing bobber creation failed");
        SetLootState(GO_READY);
    }

    ~FishingBobber()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

bool HasVisibleFish(Loot& loot, Player& player, uint32 item = Carp)
{
    WorldPackets::Loot::LootResponse packet;
    loot.BuildLootResponse(packet, &player, ALL_PERMISSION);
    return std::any_of(packet.Items.begin(), packet.Items.end(), [item](auto const& found) { return found.Loot.ItemID == item; });
}

void MoveTo(LootPlayer& player, Position const& destination)
{
    SupplyMove(player, destination);
    player.GetMap()->GetZoneAndAreaId(player.m_zoneId, player.m_areaId, destination.GetPositionX(), destination.GetPositionY(), destination.GetPositionZ());
    player.UpdateAreaDependentAuras(player.m_areaId);
}

SpellCastResult UseBait(LootPlayer& player)
{
    auto eye = player.GetItemByEntry(Bait);
    Require(eye, "The player does not carry a Stag Eye");
    GameTime::UpdateGameTimers();
    auto const spell = sSpellMgr->GetSpellInfo(BaitSpell);
    auto delay = std::max<double>(player.GetSpellCooldownDelay(BaitSpell) * 1000.0, spell->Cooldowns.StartRecoveryTime);
    if (delay > 0.0)
    {
        Require(delay < 10000.0, "The bait gained a long cooldown");
        std::this_thread::sleep_for(std::chrono::milliseconds(uint32(delay) + 60));
        GameTime::UpdateGameTimers();
    }
    auto result = player.CastSpell(&player, BaitSpell, false, eye);
    Tick(player, spell->CalcCastTime() + 1000);
    return result;
}

void CheckFishing(Quest const* quest, bool baseline)
{
    auto npc = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM creature WHERE guid=264856 AND id=43239 AND map=1 AND zoneId=1637");
    Require(bool(npc), "The original Razgar is missing");
    auto pos = npc->Fetch();
    Position const razgarPos(pos[0].GetFloat(), pos[1].GetFloat(), pos[2].GetFloat(), pos[3].GetFloat());
    Map map(1, 625, 0, DIFFICULTY_NONE);
    map.LoadGrid(razgarPos.GetPositionX(), razgarPos.GetPositionY());
    Actor razgar(&map, Razgar, 264856, razgarPos);
    Require(map.AddToMap(static_cast<Creature*>(&razgar)), "Could not publish Razgar");
    LootPlayer owner(SupplySession(), map, 2655701, razgarPos), outsider(SupplySession(), map, 2655702, razgarPos);
    for (auto player : {&owner, &outsider})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_ORC);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(RACE_ORC);
        player->InitDisplayIds();
        player->GetReputationMgr().LoadFromDB({});
        player->SetLevel(20);
        player->SetSkill(356, 1, 75, 75);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE})
            player->SetFloatValue(field, 1.0f);
    }
    auto accept = [&]
    {
        MoveTo(owner, razgarPos);
        WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
        packet.QuestGiverGUID = razgar.GetGUID();
        packet.QuestID = Daily;
        owner.GetSession()->HandleQuestGiverAcceptQuest(packet);
        Require(owner.GetQuestStatus(Daily) == QUEST_STATUS_INCOMPLETE, "Razgar did not admit the selected fishing daily");
    };
    accept();
    Position const wisdom = FindWater(map, {1882.75f, -4301.5f, 22.93f}, ValleyOfWisdom);
    Position const honor = FindWater(map, razgarPos, ValleyOfHonor);
    FishingBobber honorBobber(map, 2655711, honor), wisdomBobber(map, 2655712, wisdom);
    MoveTo(owner, honor);
    Loot scratch;
    bool cityCarp = false, cityFish = false;
    for (unsigned attempt = 0; attempt < 640; ++attempt)
    {
        honorBobber.getFishLoot(&scratch, &owner);
        cityCarp |= HasVisibleFish(scratch, owner);
        WorldPackets::Loot::LootResponse contents;
        scratch.BuildLootResponse(contents, &owner, ALL_PERMISSION);
        cityFish |= !contents.Items.empty();
    }
    Require(cityFish && cityCarp == baseline, "The city fallback did not exhibit the expected Sandy Carp eligibility");
    if (baseline)
    {
        std::cout << "EXPECTED_BROKEN native fishing awarded Sandy Carp in the Valley of Honor with no bait\n";
        return;
    }

    MoveTo(owner, wisdom);
    MoveTo(outsider, wisdom);
    for (unsigned attempt = 0; attempt < 12; ++attempt)
    {
        wisdomBobber.getFishLoot(&scratch, &owner);
        Require(!HasVisibleFish(scratch, owner), "The Valley of Wisdom gave quest fish before the Stag Eye was used");
    }
    GiveSupply(owner, Bait, 1);
    GiveSupply(outsider, Bait, 1);
    Require(UseBait(outsider) != SPELL_CAST_OK && !outsider.HasAura(BaitSpell) && outsider.GetItemCount(Bait) == 1,
        "A player without the quest consumed their old Stag Eye");
    auto cast = UseBait(owner);
    std::cout << "CASE eligible bait result=" << unsigned(cast) << " aura=" << owner.HasAura(BaitSpell)
        << " carried=" << owner.GetItemCount(Bait) << " zone=" << owner.GetZoneId() << " area=" << owner.GetAreaId() << std::endl;
    Require(cast == SPELL_CAST_OK && owner.HasAura(BaitSpell) && !owner.GetItemCount(Bait),
        "The genuine Stag Eye did not apply the aura and consume its one charge");
    MoveTo(owner, honor);
    for (unsigned attempt = 0; attempt < 12; ++attempt)
    {
        honorBobber.getFishLoot(&scratch, &owner);
        Require(!HasVisibleFish(scratch, owner), "The city fallback still gave Sandy Carp with the bait active outside its fishing spot");
    }
    AbandonSupply(owner, Daily);
    Require(!owner.HasAura(BaitSpell), "Abandoning at a different Orgrimmar subzone retained the former attempt's bait");

    accept();
    MoveTo(owner, wisdom);
    wisdomBobber.getFishLoot(&scratch, &owner);
    Require(!HasVisibleFish(scratch, owner), "A new attempt carried the old bait into the fishing spot");
    GiveSupply(owner, Bait, 1);
    auto retryCast = UseBait(owner);
    std::cout << "CASE reaccepted bait result=" << unsigned(retryCast) << " aura=" << owner.HasAura(BaitSpell)
        << " carried=" << owner.GetItemCount(Bait) << std::endl;
    Require(retryCast == SPELL_CAST_OK && owner.HasAura(BaitSpell), "The reaccepted daily could not prepare Stag Eye bait");
    owner.SendLoot(wisdomBobber.GetGUID(), LOOT_FISHING);
    WorldPackets::Loot::LootResponse contents;
    wisdomBobber.loot.BuildLootResponse(contents, &owner, ALL_PERMISSION);
    auto fish = std::find_if(contents.Items.begin(), contents.Items.end(), [](auto const& item) { return item.Loot.ItemID == Carp; });
    Require(fish != contents.Items.end() && fish->Quantity == 1, "Native fishing did not expose exactly one baited Sandy Carp");
    WorldPackets::Loot::AutoStoreLootItem take{WorldPacket(CMSG_LOOT_ITEM)};
    take.Loot.push_back({wisdomBobber.loot.GetGUID(), fish->LootListID});
    owner.GetSession()->HandleAutostoreLootItemOpcode(take);
    Require(owner.GetItemCount(Carp) == 1 && owner.GetQuestStatus(Daily) == QUEST_STATUS_COMPLETE && !owner.HasAura(BaitSpell),
        "Storing the real catch did not finish the objective and end the aura");
    GiveSupply(owner, Bait, 1);
    AbandonSupply(owner, Daily);
    Require(!owner.GetItemCount(Carp) && !owner.GetItemCount(Bait), "Abandonment left the old catch or bait in carried inventory");

    accept();
    MoveTo(owner, wisdom);
    GiveSupply(owner, Bait, 1);
    Require(UseBait(owner) == SPELL_CAST_OK && owner.HasAura(BaitSpell), "A fresh attempt could not prepare new bait");
    FishingBobber retry(map, 2655713, wisdom);
    owner.SendLoot(retry.GetGUID(), LOOT_FISHING);
    contents.Items.clear();
    retry.loot.BuildLootResponse(contents, &owner, ALL_PERMISSION);
    fish = std::find_if(contents.Items.begin(), contents.Items.end(), [](auto const& item) { return item.Loot.ItemID == Carp; });
    Require(fish != contents.Items.end(), "The new attempt could not obtain a fresh fish");
    take.Loot.clear();
    take.Loot.push_back({retry.loot.GetGUID(), fish->LootListID});
    owner.GetSession()->HandleAutostoreLootItemOpcode(take);
    MoveTo(owner, razgarPos);
    Require(owner.CanRewardQuest(quest, 0, false), "Razgar did not accept the completed native catch");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = Daily;
    reward.QuestGiverGUID = razgar.GetGUID();
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestStatus(Daily) == QUEST_STATUS_REWARDED && !owner.GetItemCount(Carp) && !owner.GetItemCount(Bait),
        "Razgar did not consume the catch and mark the daily rewarded");
    std::cout << "PASS actual subzone and fallback bobber loot, consumable, retry, inventory and Razgar daily reward\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The fishing probe requires isolated characters");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
#ifdef STAGGERING_EFFORT_BASELINE
        CheckFishing(LoadFishingDaily(), true);
#else
        CheckFishing(LoadFishingDaily(), false);
#endif
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
