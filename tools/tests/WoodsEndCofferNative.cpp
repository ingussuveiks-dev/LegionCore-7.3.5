#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "ConditionMgr.h"
#include "GilneasQuestNative.h"
#include "QuestItemObjectUse.h"
#include "SmartAI.h"
#include "SmartScriptMgr.h"
#include "TemporarySummon.h"

namespace
{
constexpr uint32 CofferEntry = 201939;
constexpr uint32 CofferSpawn = 47222;
constexpr uint32 FollowUp = 24593;
constexpr uint32 OriginalPhaseQuest = 14386;

void LoadWoodsInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeWoodsEndCoffer.inc"
    );
    Require(rows && rows->GetRowCount() == 3, "The actual Woods' End quest and phase/follow-up records are absent");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (rows->NextRow());

    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN(24593,24646)");
    Require(addon && addon->GetRowCount() == 2, "The actual Horn and Crowley handoff addon rows are absent");
    do
    {
        auto row = addon->Fetch();
        Quest* quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        if (quest->PrevQuestID)
            quest->prevQuests.push_back(quest->PrevQuestID);
    } while (addon->NextRow());

    Quest* quest = quests[WoodsEndCoffer::Quest];
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    auto goal = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=24646");
    Require(goal && goal->GetRowCount() == 1, "The actual artifact objective is absent");
    quest->LoadQuestObjective(goal->Fetch());
    Require(quest->SourceItemId == WoodsEndCoffer::Horn && quests[FollowUp]->PrevQuestID == WoodsEndCoffer::Quest &&
        quest->GetObjectives()[0].ObjectID == WoodsEndCoffer::Artifact && quest->GetObjectives()[0].Amount == 1,
        "The original Horn, artifact objective or follow-up changed");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto horn = sObjectMgr->GetItemTemplate(WoodsEndCoffer::Horn);
    auto blow = sSpellMgr->GetSpellInfo(WoodsEndCoffer::BlowHorn);
    auto rally = sSpellMgr->GetSpellInfo(WoodsEndCoffer::Rally)->GetEffect(EFFECT_0);
    auto tracker = sSpellMgr->GetSpellInfo(WoodsEndCoffer::SummonTracker);
    auto retreat = sSpellMgr->GetSpellInfo(WoodsEndCoffer::Retreat)->GetEffect(EFFECT_0);
    Require(horn && std::any_of(horn->Effects.begin(), horn->Effects.end(), [](ItemEffectEntry const* effect)
        { return effect && effect->SpellID == WoodsEndCoffer::BlowHorn && !effect->Charges; }) &&
        blow->GetEffect(EFFECT_0)->Effect == SPELL_EFFECT_SEND_EVENT && blow->GetEffect(EFFECT_0)->MiscValue == 23338 &&
        rally->Effect == SPELL_EFFECT_FORCE_CAST && rally->TriggerSpell == WoodsEndCoffer::SummonTracker &&
        tracker->GetEffect(EFFECT_0)->MiscValue == WoodsEndCoffer::Tracker && tracker->GetEffect(EFFECT_0)->MiscValueB == 66 &&
        retreat->Effect == SPELL_EFFECT_DUMMY && sBroadcastTextStore.LookupEntry(37795) && sBroadcastTextStore.LookupEntry(37796),
        "The native item, spell effects or rally/retreat speech changed");
    std::cout << "rally and retreat radii " << rally->CalcRadius() << ' ' << retreat->CalcRadius()
        << ", tracker duration " << tracker->GetDuration() << '\n';

    auto& table = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto original = table;
    table += " WHERE Entry=201939";
    auto loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    table = original;
    Require(loaded == 1, "Could not load the actual Worn Coffer loot table");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(24593,24646)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=24646");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE spell=68483 AND area=4714 AND quest_start=14386");
#ifndef WOODS_END_COFFER_BASELINE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 AND entryorguid=38022");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry IN(71011,71018)");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSpellMgr->LoadSpellAreas();
#ifndef WOODS_END_COFFER_BASELINE
    sConditionMgr->LoadConditions();
    sSmartScriptMgr->LoadSmartAIFromDB();
#endif
}

void AcceptWoods(LootPlayer& player, Creature& crowley)
{
    MoveGilneasPlayer(player, crowley.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = crowley.GetGUID();
    accept.QuestID = WoodsEndCoffer::Quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(WoodsEndCoffer::Quest) == QUEST_STATUS_INCOMPLETE && player.GetItemCount(WoodsEndCoffer::Horn) == 1,
        "Darius did not provide the actual Horn on native acceptance");
}

SpellCastResult BlowWoodsHorn(LootPlayer& player, Item* item)
{
    SpellCastTargets targets;
    targets.SetCaster(&player);
    return player.CastSpell(targets, sSpellMgr->GetSpellInfo(WoodsEndCoffer::BlowHorn), nullptr, TRIGGERED_NONE, item);
}

std::vector<TempSummon*> WoodsTrackers(LootPlayer& player)
{
    std::vector<TempSummon*> result;
    for (ObjectGuid const& guid : *player.GetSummonList(WoodsEndCoffer::Tracker))
        if (Creature* tracker = player.GetMap()->GetCreature(guid); tracker && tracker->IsAlive())
            if (TempSummon* summon = tracker->ToTempSummon())
                result.push_back(summon);
    return result;
}

void CheckWoodsEndCoffer()
{
    auto cofferRow = WorldDatabase.Query("SELECT guid,id,spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,phaseMask "
        "FROM gameobject WHERE id=201939 AND guid=47222 AND map=654 AND zoneId=4714 AND areaId=4730");
    auto crowleyRow = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE id=37195 AND guid=149508 AND map=654");
    auto tobiasRow = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE id=38029 AND guid=149545 AND map=654");
    Require(cofferRow && crowleyRow && tobiasRow, "The original Woods' End actors or Coffer are missing");
    Field* row = cofferRow->Fetch();
    Map map(654, 604, 0, DIFFICULTY_NONE);
    GilneasActor crowley(map, crowleyRow->Fetch()), tobias(map, tobiasRow->Fetch());
    Position origin = ReadGilneasPoint(row + 3);
    map.LoadGrid(origin.GetPositionX(), origin.GetPositionY());
    Supply coffer(map, {CofferEntry, CofferSpawn, row[2].GetUInt32(), origin,
        G3D::Quat(row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat(), row[10].GetFloat())});
    coffer.SetPhaseMask(row[11].GetUInt32(), false);
    Require(map.GetGameObject(coffer.GetGUID()) == &coffer && coffer.GetGOInfo()->GetLockId() == 1691 &&
        coffer.GetGOInfo()->displayId == 5743, "The original physical Coffer or lock changed");
    LootPlayer owner(SupplySession(), map, 2464601, crowley.GetPosition()), observer(SupplySession(), map, 2464602, crowley.GetPosition());
    for (LootPlayer* player : {&owner, &observer})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_WORGEN);
        player->setFactionForRace(RACE_WORGEN);
        player->InitDisplayIds();
        player->GetReputationMgr().LoadFromDB({});
        player->SetLevel(12);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE, UNIT_FIELD_MOD_HASTE_REGEN})
            player->SetFloatValue(field, 1.0f);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        player->SetRewardedQuest(OriginalPhaseQuest);
        MoveGilneasPlayer(*player, origin);
        Require(player->HasAura(68483) && player->InSamePhase(&tobias) && player->InSamePhase(&coffer), "The actual Blackwald phase did not apply");
    }

    auto rangerRows = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature "
        "WHERE id=38022 AND map=654 AND areaId=4730 ORDER BY guid");
    observer.SendLoot(coffer.GetGUID(), LOOT_SKINNING);
#ifdef WOODS_END_COFFER_BASELINE
    Require(!rangerRows && observer.GetLootGUID() == coffer.GetGUID(), "The original missing Rangers or inactive Coffer no longer reproduce");
    ReleaseSupply(observer, coffer);
    if (coffer.getLootState() == GO_JUST_DEACTIVATED)
        coffer.Rearm();
#else
    Require(rangerRows && rangerRows->GetRowCount() == 10 && !observer.GetLootGUID() && !coffer.ActivateToQuest(&observer) &&
        !coffer.IsNeverVisible(&observer), "The coffer outside the quest or its Ranger placements changed");
    std::vector<std::unique_ptr<GilneasActor>> rangers;
    do
    {
        auto ranger = std::make_unique<GilneasActor>(map, rangerRows->Fetch());
        Require(ranger->AIM_Initialize(new SmartAI(ranger.get())) && tobias.IsWithinDistInMap(ranger.get(), 60.0f),
            "A restored Ranger did not load native combat AI in the real cabin area");
        rangers.push_back(std::move(ranger));
    } while (rangerRows->NextRow());
    Require(coffer.respawn == 5, "The native shared stock changed");
#endif

    AcceptWoods(owner, crowley);
    auto useHorn = [&owner]() { return BlowWoodsHorn(owner, owner.GetItemByEntry(WoodsEndCoffer::Horn)); };
#ifdef WOODS_END_COFFER_BASELINE
    Require(useHorn() == SPELL_CAST_OK, "The original Horn no longer casts away from the cabin");
    Tick(owner, 9000);
    owner.RemoveSpellCooldown(WoodsEndCoffer::BlowHorn);
    MoveGilneasPlayer(owner, tobias.GetPosition());
    Require(useHorn() == SPELL_CAST_OK, "Original Horn did not cast by Tobias");
    Tick(owner, 9000);
    Require(WoodsTrackers(owner).empty(), "The original Horn unexpectedly rallied actual actors");
    MoveGilneasPlayer(owner, origin);
    OpenGilneasQuestChest(owner, coffer);
    Require(TakeSupply(owner, coffer, WoodsEndCoffer::Artifact), "Original Coffer failed to produce its objective");
    ReleaseSupply(owner, coffer);
    AbandonSupply(owner, WoodsEndCoffer::Quest);
    Require(owner.GetItemCount(WoodsEndCoffer::Artifact) == 1, "Original abandonment no longer retains the artifact");
    std::cout << "EXPECTED_BROKEN absent Rangers, inactive native loot window, ineffective off-site Horn and retained artifact\n";
#else
    Require(useHorn() == SPELL_FAILED_BAD_TARGETS, "The Horn admitted Darius' distant camp");
    MoveGilneasPlayer(owner, tobias.GetPosition());
    Require(owner.GetAreaId() == 4730 && BlowWoodsHorn(owner, nullptr) == SPELL_FAILED_BAD_TARGETS,
        "The horn can be cast without the actual item in the original area");
    Require(useHorn() == SPELL_CAST_OK, "Native Horn failed to cast near the real Tobias");
    Tick(owner, 8000);
    auto trackers = WoodsTrackers(owner);
    std::cout << "Rallied " << trackers.size() << " trackers for " << rangers.size() << " native Rangers\n";
    Require(trackers.size() == rangers.size(), "The Horn did not call all cabin trackers");
    for (TempSummon* tracker : trackers)
    {
        Unit* victim = tracker->getVictim();
        Require(tracker->HasQuestObjectScope() && tracker->GetSummonerGUID() == owner.GetGUID() && victim &&
            victim->GetEntry() == WoodsEndCoffer::Ranger && victim->getVictim() == tracker && tracker->IsValidAttackTarget(victim) &&
            tracker->getFaction() == 2207 && victim->getFaction() == 2213 && !tracker->IsHostileTo(&owner) &&
            owner.canSeeOrDetect(tracker) && !observer.canSeeOrDetect(tracker),
            "A true tracker was not scoped to its owner and in actual Ranger combat");
    }
    owner.RemoveSpellCooldown(WoodsEndCoffer::BlowHorn);
    Require(useHorn() != SPELL_CAST_OK, "A second horn replaced an active cabin distraction");
    Tick(owner, 30000);
    for (TempSummon* tracker : trackers)
        tracker->Unit::Update(500);
    std::cout << "Retreat stopped " << std::count_if(trackers.begin(), trackers.end(), [](TempSummon* tracker)
        { return tracker->GetReactState() == REACT_PASSIVE; }) << " trackers and stunned "
        << std::count_if(rangers.begin(), rangers.end(), [](auto const& ranger) { return ranger->HasAura(WoodsEndCoffer::WarStomp); })
        << " Rangers" << std::endl;
    Require(std::all_of(trackers.begin(), trackers.end(), [](TempSummon* tracker) { return tracker->GetReactState() == REACT_PASSIVE; }) &&
        std::any_of(rangers.begin(), rangers.end(), [](auto const& ranger) { return ranger->HasAura(WoodsEndCoffer::WarStomp); }),
        "The real retreat did not trigger War Stomp and release the trackers");
    for (TempSummon* tracker : trackers)
        tracker->Update(4500);
    map.RemoveAllObjectsInRemoveList();
    Require(WoodsTrackers(owner).empty(), "Retreated trackers did not clean up");

    MoveGilneasPlayer(owner, origin);
    OpenGilneasQuestChest(owner, coffer);
    Require(TakeSupply(owner, coffer, WoodsEndCoffer::Artifact) && owner.GetQuestStatus(WoodsEndCoffer::Quest) == QUEST_STATUS_COMPLETE &&
        !coffer.ActivateToQuest(&owner), "The actual Worn Coffer did not complete the sole artifact objective");
    ReleaseSupply(owner, coffer);
    AbandonSupply(owner, WoodsEndCoffer::Quest);
    Require(!owner.GetItemCount(WoodsEndCoffer::Horn) && !owner.GetItemCount(WoodsEndCoffer::Artifact), "Abandonment retained a quest item");
    coffer.Rearm();

    AcceptWoods(owner, crowley);
    MoveGilneasPlayer(owner, tobias.GetPosition());
    owner.RemoveSpellCooldown(WoodsEndCoffer::BlowHorn);
    Require(useHorn() == SPELL_CAST_OK, "A fresh acceptance could not retry the Horn");
    AbandonSupply(owner, WoodsEndCoffer::Quest);
    Tick(owner, 8000);
    Require(WoodsTrackers(owner).empty(), "A canceled horn created trackers from a stale acceptance");

    AcceptWoods(owner, crowley);
    MoveGilneasPlayer(owner, tobias.GetPosition());
    owner.RemoveSpellCooldown(WoodsEndCoffer::BlowHorn);
    Require(useHorn() == SPELL_CAST_OK, "A canceled rally prevented a legitimate retry");
    Tick(owner, 8000);
    auto retry = WoodsTrackers(owner);
    Require(retry.size() == rangers.size(), "The renewed Horn did not rally its own actors");
    AbandonSupply(owner, WoodsEndCoffer::Quest);
    for (TempSummon* tracker : retry)
        Require(tracker->IsQuestObjectExpired(), "Abandonment left an old tracker in its active scope");
    map.RemoveAllObjectsInRemoveList();
    Require(WoodsTrackers(owner).empty(), "Active combat trackers survived abandonment");

    AcceptWoods(observer, crowley);
    MoveGilneasPlayer(observer, origin);
    OpenGilneasQuestChest(observer, coffer);
    Require(TakeSupply(observer, coffer, WoodsEndCoffer::Artifact), "The other eligible player could not recover the Coffer after native stock refill");
    ReleaseSupply(observer, coffer);
    MoveGilneasPlayer(observer, crowley.GetPosition());
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = WoodsEndCoffer::Quest;
    reward.QuestGiverGUID = crowley.GetGUID();
    reward.ItemChoiceID = 0;
    observer.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(observer.GetQuestRewardStatus(WoodsEndCoffer::Quest) && !observer.GetItemCount(WoodsEndCoffer::Artifact) &&
        !observer.GetItemCount(WoodsEndCoffer::Horn) && crowley.hasQuest(FollowUp) && !coffer.ActivateToQuest(&observer),
        "The actual Darius reward did not consume both items and retain the next quest");
    std::cout << "PASS original cabin horn, real Ranger combat and retreat, native coffer stock, cancellation and Darius reward\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Woods' End checks require isolated characters");
        LoadWoodsInputs();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckWoodsEndCoffer();
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
