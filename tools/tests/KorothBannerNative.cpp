#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "QuestItemObjectUse.h"

namespace
{
constexpr uint32 Marsh = 24468;
constexpr uint32 OriginalPhaseQuest = 14386;
constexpr uint32 Stormglen = 24483;
constexpr uint32 Ogre = 36293;
constexpr uint32 Koroth = 36294;
constexpr uint32 BannerObject = 201594;
constexpr uint32 BannerPhase = 16384;

Position ReadBannerPoint(Field* row)
{
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

struct GilneasActor : Actor
{
    GilneasActor(Map& map, Field* row) : Actor(&map, row[1].GetUInt32(), row[0].GetUInt32(), ReadBannerPoint(row + 2))
    {
        m_DBTableGuid = row[0].GetUInt32();
        SetPhaseMask(row[6].GetUInt32(), false);
        map.LoadGrid(GetPositionX(), GetPositionY());
        Require(map.AddToMap(static_cast<Creature*>(this)), "Could not publish the original Gilneas actor");
    }

    ~GilneasActor()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void LoadBannerInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeKorothBanner.inc"
    );
    Require(rows && rows->GetRowCount() == 4, "The banner quest, prerequisites or follow-up are absent");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (rows->NextRow());
    auto quest = quests[KorothBanner::Quest];
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN(24472,24483)");
    Require(addon && addon->GetRowCount() == 2, "The Prince's original quest prerequisites are absent");
    do
    {
        auto row = addon->Fetch();
        auto item = quests[row[0].GetUInt32()];
        item->LoadQuestTemplateAddon(row);
        item->prevQuests.push_back(item->PrevQuestID);
    } while (addon->NextRow());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER | QUEST_SPECIAL_FLAGS_KILL);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=24472 ORDER BY ID");
    Require(objectives && objectives->GetRowCount() == 2, "The two native objectives are missing");
    do
    {
        quest->LoadQuestObjective(objectives->Fetch());
    } while (objectives->NextRow());
    Require(quest->PrevQuestID == Marsh && quests[Stormglen]->PrevQuestID == KorothBanner::Quest &&
        quest->GetObjectives()[0].ObjectID == Ogre && quest->GetObjectives()[0].Amount == 4 &&
        quest->GetObjectives()[1].ObjectID == KorothBanner::Banner && quest->GetObjectives()[1].Amount == 1,
        "The original quest chain or objectives changed");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto item = sObjectMgr->GetItemTemplate(KorothBanner::Banner);
    auto effect = sSpellMgr->GetSpellInfo(KorothBanner::Swiped)->GetEffect(EFFECT_0);
    Require(item && std::any_of(item->Effects.begin(), item->Effects.end(), [](ItemEffectEntry const* row)
        { return row && row->SpellID == KorothBanner::Swiped && row->TriggerType == ITEM_SPELLTRIGGER_ON_NO_DELAY_USE; }) &&
        effect->Effect == SPELL_EFFECT_SEND_EVENT && effect->MiscValue == 22657 &&
        sBroadcastTextStore.LookupEntry(36914) && sBroadcastTextStore.LookupEntry(36913), "The original pickup event or speech is absent");

    auto& table = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto original = table;
    table += " WHERE Entry=201594";
    auto loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    table = original;
    Require(loaded == 1, "Could not load the actual Banner's single loot table");

    auto& creatureLoot = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto originalCreatureLoot = creatureLoot;
    creatureLoot += " WHERE Entry=36293";
    loaded = (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    creatureLoot = originalCreatureLoot;
    Require(loaded == 24 && LootTemplates_Creature.HaveLootFor(Ogre), "Could not load the actual Ogre Minion corpse loot");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(24472,24483)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=24472");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE spell=68483 AND area=4714 AND quest_start=14386");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSpellMgr->LoadSpellAreas();
}

void MoveBannerPlayer(LootPlayer& player, Position const& point)
{
    SupplyMove(player, point);
    player.GetMap()->GetZoneAndAreaId(player.m_zoneId, player.m_areaId, point.GetPositionX(), point.GetPositionY(), point.GetPositionZ());
    player.UpdateZoneDependentAuras(player.GetZoneId());
    player.GetPhaseMgr().Recalculate();
    player.GetPhaseMgr().Update();
}

void AcceptBanner(LootPlayer& player, Creature& prince)
{
    MoveBannerPlayer(player, prince.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = prince.GetGUID();
    accept.QuestID = KorothBanner::Quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(KorothBanner::Quest) == QUEST_STATUS_INCOMPLETE, "Prince Liam did not give the original quest");
}

void OpenOriginalBanner(LootPlayer& player, Supply& banner)
{
    constexpr float HalfTurn = 3.14159265358979f;
    for (float offset : {0.0f, HalfTurn / 2.0f, -HalfTurn / 2.0f, HalfTurn})
    {
        float facing = banner.GetOrientation() + offset;
        Position approach{banner.GetPositionX() - 1.5f * std::cos(facing), banner.GetPositionY() - 1.5f * std::sin(facing),
            banner.GetPositionZ(), Position::NormalizeOrientation(facing)};
        MoveBannerPlayer(player, approach);
        if (player.IsWithinLOSInMap(&banner))
            break;
    }
    OpenSupply(player, banner);
    std::cout << "CASE Banner ready=" << banner.isSpawned() << " type=" << uint32(banner.GetGoType())
        << " state=" << uint32(banner.getLootState()) << " allowed=" << banner.ActivateToQuest(&player) << " distance=" << player.GetDistance(&banner)
        << " LOS=" << player.IsWithinLOSInMap(&banner) << " questItem=" << player.HasQuestForItem(KorothBanner::Banner)
        << " lootGUID=" << (player.GetLootGUID() == banner.GetGUID()) << ' ' << banner.loot.items.size() << '\n';
}

void CheckKorothBanner()
{
    auto bannerRow = WorldDatabase.Query("SELECT guid,id,spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,phaseMask "
        "FROM gameobject WHERE id=201594 AND guid=47029 AND map=654 AND zoneId=4714");
    auto prince = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE id=37065 AND guid=149396 AND map=654");
    auto koroth = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE id=36294 AND guid=149430 AND map=654");
    auto ogres = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE id=36293 AND map=654 ORDER BY guid");
    Require(bannerRow && prince && koroth && ogres && ogres->GetRowCount() == 10, "The actual Banner, Prince, Koroth and ten Ogre Minions are missing");
    Field* row = bannerRow->Fetch();
    Require(row[11].GetUInt32() == BannerPhase, "The original Banner phase changed");
    Map map(654, 603, 0, DIFFICULTY_NONE);
    GilneasActor liam(map, prince->Fetch()), pursuer(map, koroth->Fetch());
    Require(pursuer.AIM_Initialize(), "Actual Koroth AI failed to initialize");
    Position origin = ReadBannerPoint(row + 3);
    map.LoadGrid(origin.GetPositionX(), origin.GetPositionY());
    Supply banner(map, {BannerObject, 47029, row[2].GetUInt32(), origin,
        G3D::Quat(row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat(), row[10].GetFloat())});
    banner.SetPhaseMask(BannerPhase, false);
    Require(map.GetGameObject(banner.GetGUID()) == &banner && banner.GetGOInfo()->GetLockId() == 1691 &&
        banner.GetGOInfo()->GetLootId() == BannerObject, "The actual Banner registration, lock or loot changed");
    LootPlayer owner(SupplySession(), map, 2447201, liam.GetPosition()), observer(SupplySession(), map, 2447202, liam.GetPosition());
    for (LootPlayer* player : {&owner, &observer})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_WORGEN);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(RACE_WORGEN);
        player->InitDisplayIds();
        player->SetLevel(12);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE, UNIT_FIELD_MOD_HASTE_REGEN})
            player->SetFloatValue(field, 1.0f);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        player->SetRewardedQuest(OriginalPhaseQuest);
        player->SetRewardedQuest(Marsh);
        MoveBannerPlayer(*player, origin);
        Require(player->HasAura(68483) && player->HasPhaseId(183) && player->InSamePhase(&banner), "The original post-marsh phase did not apply");
    }
    observer.SendLoot(banner.GetGUID(), LOOT_SKINNING);
#ifdef KOROTH_BANNER_BASELINE
    std::cout << "CASE original Banner visible=" << !banner.IsNeverVisible(&observer)
        << " inactive loot opened=" << (observer.GetLootGUID() == banner.GetGUID()) << '\n';
    Require(!banner.IsNeverVisible(&observer) && observer.GetLootGUID() == banner.GetGUID(), "The original chest no longer opens a loot window outside the quest");
    ReleaseSupply(observer, banner);
    if (banner.getLootState() == GO_JUST_DEACTIVATED)
        banner.Rearm();
    AcceptBanner(owner, liam);
    MoveBannerPlayer(owner, origin);
    OpenOriginalBanner(owner, banner);
    Require(TakeSupply(owner, banner, KorothBanner::Banner) && pursuer.getVictim() != &owner,
        "The original native loot no longer reproduces the missing Koroth response");
    ReleaseSupply(owner, banner);
    AbandonSupply(owner, KorothBanner::Quest);
    Require(owner.GetItemCount(KorothBanner::Banner) == 1, "The original abandonment no longer retains the Banner");
    std::cout << "EXPECTED_BROKEN banner loot window outside the quest, pickup had no Koroth response, item survived abandon\n";
#else
    Require(!banner.IsNeverVisible(&observer) && !banner.ActivateToQuest(&observer) && !observer.GetLootGUID(),
        "An inactive player opened the Banner or the shared physical model was hidden");
    AcceptBanner(owner, liam);
    MoveBannerPlayer(owner, origin);
    Require(!banner.IsNeverVisible(&owner) && banner.ActivateToQuest(&owner), "The accepted quest did not show its exact Banner");
    GiveSupply(owner, KorothBanner::Banner, 1);
    Tick(owner, 1500);
    Require(!pursuer.getVictim(), "An inventory-only Banner started the actual Koroth encounter");
    owner.DestroyItemCount(KorothBanner::Banner, 1, true);
    Tick(owner, 150);
    OpenOriginalBanner(owner, banner);
    Require(TakeSupply(owner, banner, KorothBanner::Banner), "The actual Banner offered no native loot");
    Tick(owner, 300);
    Require(pursuer.getVictim() == &owner && !banner.ActivateToQuest(&owner), "Genuine Banner loot did not start the original Koroth response and close personal access");
    ReleaseSupply(owner, banner);
    AbandonSupply(owner, KorothBanner::Quest);
    Require(!owner.GetItemCount(KorothBanner::Banner) && !banner.ActivateToQuest(&owner), "Abandonment retained the Banner or access to the original object");
    pursuer.CombatStop(true);
    owner.CombatStop(true);

    banner.Rearm();
    AcceptBanner(observer, liam);
    LootPet pet(observer, map);
    pet.SetLevel(observer.getLevel());
    pet.SetEffectiveLevel(observer.GetEffectiveLevel());
    pet.SetPhaseMask(observer.GetPhaseMask(), false);
    pet.SetPhaseId(observer.GetPhases(), false);
    pet.SetOwnerGUID(ObjectGuid::Empty);
    observer.SetMinion(&pet, true);
    Require(pet.IsControlledByPlayer() && pet.GetOwner() == &observer, "The genuine Hunter pet did not bind to its owner");
    for (uint32 count = 1; count <= 4; ++count)
    {
        GilneasActor minion(map, ogres->Fetch());
        MoveBannerPlayer(observer, minion.GetPosition());
        Unit& killer = count == 1 ? static_cast<Unit&>(pet) : static_cast<Unit&>(observer);
        if (count == 1)
            pet.Relocate(minion.GetPosition());
        Require(killer.DealDamage(&minion, 1000000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) && minion.isDead(false),
            "An original Ogre Minion did not die to a real player or pet hit");
        Tick(observer, 100);
        Require(observer.GetQuestObjectiveData(KorothBanner::Quest, Ogre) == int32(count), "The real death did not credit the player or pet owner");
        if (count < 4)
            Require(ogres->NextRow(), "The actual Ogre list ended early");
    }
    observer.SetMinion(&pet, false);
    MoveBannerPlayer(observer, origin);
    OpenOriginalBanner(observer, banner);
    Require(TakeSupply(observer, banner, KorothBanner::Banner) && observer.GetQuestStatus(KorothBanner::Quest) == QUEST_STATUS_COMPLETE,
        "Four actual kills and genuine loot did not complete the quest");
    ReleaseSupply(observer, banner);
    pursuer.CombatStop(true);
    observer.CombatStop(true);
    MoveBannerPlayer(observer, liam.GetPosition());
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = KorothBanner::Quest;
    reward.QuestGiverGUID = liam.GetGUID();
    reward.ItemChoiceID = 0;
    observer.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(observer.GetQuestRewardStatus(KorothBanner::Quest) && !observer.GetItemCount(KorothBanner::Banner) && !banner.ActivateToQuest(&observer) &&
        liam.hasQuest(Stormglen), "Prince Liam did not take the Banner or retain the follow-up");
    std::cout << "PASS actual Banner, original phase and loot event, four native kills including a pet, abandonment and Prince Liam reward\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{654, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{654, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Banner checks require isolated characters");
        LoadBannerInputs();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckKorothBanner();
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
