#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

namespace
{
constexpr uint32 LaidToRest = 24602;
constexpr uint32 PatriarchsBlessing = 24679;
constexpr uint32 KeelHarbor = 24680;
constexpr uint32 HarborFollowUp = 24681;
constexpr uint32 Memento = 49921;
constexpr uint32 Offerings = 51956;
constexpr uint32 PlaceOfferings = 72853;

std::vector<Placement> ReadPlacements(uint32 entry, uint32 first, uint32 last)
{
    auto rows = WorldDatabase.PQuery("SELECT id,guid,spawntimesecs,position_x,position_y,position_z,orientation,"
        "rotation0,rotation1,rotation2,rotation3 FROM gameobject WHERE map=654 AND id=%u AND guid BETWEEN %u AND %u ORDER BY guid",
        entry, first, last);
    Require(bool(rows), "Missing original object placements");
    std::vector<Placement> result;
    do
    {
        auto row = rows->Fetch();
        result.push_back({ row[0].GetUInt32(), row[1].GetUInt32(), row[2].GetUInt32(),
            { row[3].GetFloat(), row[4].GetFloat(), row[5].GetFloat(), row[6].GetFloat() },
            { row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat(), row[10].GetFloat() } });
    } while (rows->NextRow());
    return result;
}

void LoadReposeData()
{
    auto rows = WorldDatabase.Query(
#include "NativeAdericRepose.inc"
    );
    Require(rows && rows->GetRowCount() == 8, "Missing Gilneas quest templates");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto addons = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon "
        "WHERE ID IN(24602,24679,24680,24681)");
    Require(addons && addons->GetRowCount() == 4, "Missing original chain prerequisites");
    do
    {
        auto row = addons->Fetch();
        auto quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        quest->prevQuests.push_back(quest->PrevQuestID);
    } while (addons->NextRow());
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN(24602,24679)");
    Require(objectives && objectives->GetRowCount() == 2, "Missing native memento or offering objective");
    do
    {
        auto row = objectives->Fetch();
        auto quest = quests[row[1].GetUInt32()];
        quest->LoadQuestObjective(row);
        if (row[2].GetUInt8() == QUEST_OBJECTIVE_ITEM)
            quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
        else
            quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    } while (objectives->NextRow());

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    for (auto const& input : { std::make_pair(&LootTemplates_Gameobject, " WHERE Entry=201871 AND Item=49921"),
        std::make_pair(&LootTemplates_Creature, " WHERE Entry=37884 AND Item=49921") })
    {
        auto& table = *input.first.*Access(LootTableNameTag{});
        auto original = table;
        table += input.second;
        auto count = (input.first->*Access(LoadLootTableTag{}))();
        table = original;
        Require(count == 1, "The native loot loader did not admit the original memento row");
    }
    auto source = WorldDatabase.Query("SELECT LootMode,QuestRequired,Chance FROM creature_loot_template WHERE Entry=37884 AND Item=49921");
    Require(source && source->Fetch()[0].GetUInt16() == 1 && source->Fetch()[1].GetBool() &&
        std::abs(source->Fetch()[2].GetFloat() - 0.2036f) < 0.00001f, "Wrong spirit loot mode, quest gate or original drop chance");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(24602,24679,24680,24681)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest IN(24602,24679,24680,24681)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE spell IN(69486,70695)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE phase_definitions AS SELECT * FROM legion_world.phase_definitions WHERE zoneId=4714");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=4714");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSpellMgr->LoadSpellAreas();
    sObjectMgr->LoadPhaseDefinitions();
    sConditionMgr->LoadConditions();
}

struct ReposeQuestGiver : Actor
{
    ReposeQuestGiver(Map& map, Field* row) : Actor(&map, row[1].GetUInt32(), row[0].GetUInt32(),
        Position(row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat(), row[5].GetFloat()))
    {
        m_DBTableGuid = row[0].GetUInt32();
        SetPhaseMask(row[6].GetUInt32(), false);
        map.LoadGrid(GetPositionX(), GetPositionY());
        Require(map.AddToMap(static_cast<Creature*>(this)), "Could not publish actual questgiver");
    }
};

struct ReposeTomb : GameObject
{
    ReposeTomb(Map& map, Placement const& placement)
    {
        map.LoadGrid(placement.position.GetPositionX(), placement.position.GetPositionY());
        Require(Create(placement.guid, placement.entry, &map, 65535, placement.position, placement.rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "Could not publish the actual Aderic tomb");
    }

    ~ReposeTomb()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

void Travel(LootPlayer& player, Position const& point)
{
    SupplyMove(player, point);
    player.GetMap()->GetZoneAndAreaId(player.m_zoneId, player.m_areaId, point.GetPositionX(), point.GetPositionY(), point.GetPositionZ());
    player.UpdateZoneDependentAuras(player.GetZoneId());
    player.GetPhaseMgr().Recalculate();
    player.GetPhaseMgr().Update();
}

void Accept(LootPlayer& player, Actor& giver, uint32 quest)
{
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = giver.GetGUID();
    accept.QuestID = quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
}

void Reward(LootPlayer& player, Actor& giver, uint32 quest)
{
    Require(player.InSamePhase(&giver) && giver.hasInvolvedQuest(quest), "Quest's actual turn-in actor is not available");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = quest;
    reward.QuestGiverGUID = giver.GetGUID();
    reward.ItemChoiceID = sQuestDataStore->GetQuestTemplate(quest)->RewardChoiceItemId[0];
    player.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(player.GetQuestRewardStatus(quest), "Native quest turn-in failed");
}

SpellCastResult UseOfferings(LootPlayer& player)
{
    auto item = player.GetItemByEntry(Offerings);
    Require(item, "Missing quest-provided Blessed Offerings");
    SpellCastTargets targets;
    targets.SetCaster(&player);
    auto result = player.CastSpell(targets, sSpellMgr->GetSpellInfo(PlaceOfferings), nullptr, TRIGGERED_NONE, item);
    Tick(player, 6000);
    return result;
}

void CheckRepose()
{
    auto questgivers = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature "
        "WHERE map=654 AND guid IN(148251,148175) ORDER BY guid DESC");
    Require(questgivers && questgivers->GetRowCount() == 2, "Missing Krennan or Darius");
    Map map(654, 594, 0, DIFFICULTY_NONE);
    ReposeQuestGiver krennan(map, questgivers->Fetch());
    questgivers->NextRow();
    ReposeQuestGiver darius(map, questgivers->Fetch());
    LootPlayer owner(SupplySession(), map, 2460201, krennan.GetPosition()), observer(SupplySession(), map, 2460202, krennan.GetPosition());
    for (auto player : { &owner, &observer })
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_WORGEN);
        player->setFactionForRace(RACE_WORGEN);
        player->InitDisplayIds();
        player->SetLevel(15);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        player->SetRewardedQuest(14386);
        player->SetRewardedQuest(14466);
        player->SetRewardedQuest(24903);
        player->SetRewardedQuest(24678);
        Travel(*player, krennan.GetPosition());
    }
    Require(owner.HasAura(69486) && owner.InSamePhase(&krennan) && !owner.HasAura(70695), "Incorrect initial progression phase");
    Travel(owner, darius.GetPosition());
    Require(!owner.InSamePhase(&darius), "Darius is visible at the harbor before the offerings were placed");
    Travel(owner, krennan.GetPosition());
    Require(!owner.CanTakeQuest(sQuestDataStore->GetQuestTemplate(PatriarchsBlessing), false) &&
        !owner.CanTakeQuest(sQuestDataStore->GetQuestTemplate(KeelHarbor), false), "The downstream offers opened too early");
    Accept(owner, krennan, LaidToRest);
    Require(owner.GetQuestStatus(LaidToRest) == QUEST_STATUS_INCOMPLETE, "Krennan did not offer Laid to Rest");
    Require(LootTemplates_Creature.HaveQuestLootForPlayer(37884, &owner) &&
        !LootTemplates_Creature.HaveQuestLootForPlayer(37884, &observer), "Spirit memento eligibility is incorrect");

    auto sites = ReadPlacements(201871, 47129, 47134);
    Require(sites.size() == 6, "Expected six original soil placements");
    for (size_t i = 0; i < sites.size(); ++i)
    {
        auto const& site = sites[i];
        map.LoadGrid(site.position.GetPositionX(), site.position.GetPositionY());
        auto phase = WorldDatabase.PQuery("SELECT phaseMask FROM gameobject WHERE map=654 AND guid=%u", site.guid);
        Require(bool(phase), "Missing actual soil phase mask");
        Supply soil(map, site, phase->Fetch()[0].GetUInt32());
        Travel(owner, site.position);
        if (!i)
        {
            Travel(observer, site.position);
            Require(!soil.ActivateToQuest(&observer), "Unqualified player saw the soil quest marker");
            observer.SendLoot(soil.GetGUID(), LOOT_SKINNING);
            OpenSupply(observer, soil);
            Require(soil.getLootState() == GO_READY && observer.GetLootGUID().IsEmpty(), "Unqualified player took the shared stock");
        }
        if (i == 5)
        {
            Require(!soil.ActivateToQuest(&owner), "Completed objective still highlights unused soil");
            OpenSupply(owner, soil);
            Require(soil.getLootState() == GO_READY && owner.GetLootGUID().IsEmpty(), "The sixth site was consumed after quest completion");
            break;
        }
        Require(soil.ActivateToQuest(&owner), "Eligible player could not see a soil quest marker");
        Require(map.GetGameObject(soil.GetGUID()) == &soil && owner.InSamePhase(&soil), "Original soil is unavailable in the correct phase");
        OpenSupply(owner, soil);
        Require(TakeSupply(owner, soil, Memento) && owner.GetItemCount(Memento) == i + 1, "A qualifying soil roll did not supply one memento");
        ReleaseSupply(owner, soil);
    }
    Require(owner.GetQuestStatus(LaidToRest) == QUEST_STATUS_COMPLETE, "Five actual mementos did not complete Laid to Rest");
    Travel(owner, krennan.GetPosition());
    Reward(owner, krennan, LaidToRest);
    Require(!owner.GetItemCount(Memento), "Turn-in did not consume the mementos");
    Accept(owner, krennan, PatriarchsBlessing);
    Require(owner.GetQuestStatus(PatriarchsBlessing) == QUEST_STATUS_INCOMPLETE && owner.GetItemCount(Offerings) == 1 &&
        !owner.HasAura(70695), "Krennan did not provide the offering in the original pre-harbor phase");

    Require(UseOfferings(owner) != SPELL_CAST_OK && owner.GetItemCount(Offerings) == 1 &&
        owner.GetQuestStatus(PatriarchsBlessing) == QUEST_STATUS_INCOMPLETE, "Away from the tomb the original offering was cast or consumed");
    AbandonSupply(owner, PatriarchsBlessing);
    Require(!owner.GetItemCount(Offerings), "Abandonment did not remove the offerings");
    Accept(owner, krennan, PatriarchsBlessing);
    ReposeTomb tomb(map, ReadPlacements(201964, 47128, 47128).front());
    Travel(owner, tomb.GetPosition());
    auto result = UseOfferings(owner);
    auto offering = owner.GetGameObject(PlaceOfferings);
    std::cout << "Offering cast=" << uint32(result) << " complete=" << uint32(owner.GetQuestStatus(PatriarchsBlessing))
        << " count=" << owner.GetItemCount(Offerings) << " object=" << (offering ? offering->GetEntry() : 0) << '\n';
    Require(result == SPELL_CAST_OK && owner.GetQuestStatus(PatriarchsBlessing) == QUEST_STATUS_COMPLETE &&
        owner.GetItemCount(Offerings) == 1 && offering && offering->GetEntry() == 202319 &&
        observer.GetQuestStatus(PatriarchsBlessing) == QUEST_STATUS_NONE && !owner.HasAura(70695),
        "The actual offering did not place its object and award only personal progress");
    owner.RemoveGameObject(offering, true);

    Travel(owner, krennan.GetPosition());
    Reward(owner, krennan, PatriarchsBlessing);
    Require(!owner.GetItemCount(Offerings) && owner.HasAura(70695) && owner.HasPhaseId(189),
        "The native post-offering phase did not reveal Darius to the eligible player");
    Accept(owner, krennan, KeelHarbor);
    Travel(owner, darius.GetPosition());
    Travel(observer, darius.GetPosition());
    Require(owner.InSamePhase(&darius) && !observer.InSamePhase(&darius), "Darius at the harbor did not respect the post-offering phase");
    Reward(owner, darius, KeelHarbor);
    Require(darius.hasQuest(HarborFollowUp) && owner.CanTakeQuest(sQuestDataStore->GetQuestTemplate(HarborFollowUp), false),
        "Keel Harbor did not open the next original quest");
    std::cout << "PASS six soil placements, five native loot packets, spirit eligibility, offering retry and Keel Harbor turn-in\n";
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
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Aderic's Repose probes require isolated characters");
        LoadReposeData();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckRepose();
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
