#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "ConditionMgr.h"
#include "CreatureTextMgr.h"
#include "GilneasQuestNative.h"
#include "ScriptsData.h"
#include "SmartScriptMgr.h"
#include "TemporarySummon.h"
#include "Vehicle.h"

void AddSC_redridge_mountains();

namespace
{
constexpr uint32 OslowQuest = 26520;
constexpr uint32 EttinQuest = 26519;
constexpr uint32 Orb = 58895;
constexpr uint32 UseOrb = 80704;
constexpr uint32 Ettin = 43197;
constexpr uint32 Boulder = 43196;
constexpr uint32 Foreman = 341;

Quest const* LoadOslow()
{
    auto rows = WorldDatabase.Query(
#include "NativeForemanOslow.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "The original Ettin quests are required");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(OslowQuest + 1);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto row = new Quest(rows->Fetch());
        quests[row->Id] = row;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(row->Id, row);
    } while (rows->NextRow());
    Quest* rescue = quests[OslowQuest];
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=26520");
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=26520");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == Foreman && addon,
        "The original rescue requires actual Foreman credit");
    rescue->LoadQuestObjective(objective->Fetch());
    rescue->LoadQuestTemplateAddon(addon->Fetch());
    rescue->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    rescue->prevQuests.push_back(EttinQuest);

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE map=0 AND zoneId=44 "
        "AND id IN (341,344,648,649,650,651,652,653,43094,43196)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_addon AS SELECT a.* FROM legion_world.creature_addon a JOIN creature c ON a.guid=c.guid");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_template_addon AS SELECT * FROM legion_world.creature_template_addon "
        "WHERE entry IN (341,344,648,649,650,651,652,653,43094,43196,43197)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_equip_template AS SELECT * FROM legion_world.creature_equip_template "
        "WHERE CreatureID IN (341,344,648,649,650,651,652,653,43094,43196,43197)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=26520");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject AS SELECT * FROM legion_world.gameobject WHERE guid=18302 AND id=204351");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_queststarter AS SELECT * FROM legion_world.gameobject_queststarter WHERE quest=26520");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text WHERE CreatureID IN (341,648,649,650,651,652,653,43197)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceTypeOrReferenceId IN (13,17) AND SourceEntry IN (80702,80704,80739,82566)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE entryorguid=43094 AND source_type=0");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE quest_start=26520 OR quest_end=26520");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id IN (80702,80704,80739,82566)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_redridge_mountains();
    sScriptMgr->SwapScriptContext(true);
    sScriptDataStore->LoadSpellScriptNames();
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sObjectMgr->LoadGameobjects();
    sObjectMgr->LoadEquipmentTemplates();
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadCreatureTemplateAddons();
    sObjectMgr->LoadCreatureAddons();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sQuestDataStore->LoadGameobjectQuestRelations();
    sCreatureTextMgr->LoadCreatureTexts();
    sConditionMgr->LoadConditions();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sSpellMgr->LoadSpellAreas();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    return rescue;
}

Creature* Spawn(Map& map, uint32 entry, uint32 id)
{
    auto data = sObjectMgr->GetCreatureData(id);
    Require(data && data->id == entry, "The actual Redridge placement is required");
    map.LoadGrid(data->posX, data->posY);
    auto actor = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(0, entry, id));
    Require(actor && actor->IsAlive(), "The actual Redridge actor must load");
    return actor;
}

void Accept(LootPlayer& player, GameObject const& source)
{
    player.setFactionForRace(player.getRace());
    player.InitDisplayIds();
    player.SetLevel(20);
    player.SetRewardedQuest(EttinQuest);
    MoveGilneasPlayer(player, source.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = source.GetGUID();
    accept.QuestID = OslowQuest;
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(OslowQuest) == QUEST_STATUS_INCOMPLETE && player.GetItemCount(Orb) == 1,
        "The real Orb questgiver must accept the player and issue the item");
}

Creature* ControlEttin(LootPlayer& player, Creature& canyon)
{
    MoveGilneasPlayer(player, canyon.GetPosition());
    auto result = player.CastSpell(&player, UseOrb, false, player.GetItemByEntry(Orb));
    Tick(player, 6500);
    auto const summons = player.GetSummonList(Ettin);
    std::cout << "CASE Oslow control spell=" << uint32(result) << " owned=" << summons->size() << " zone=" << player.GetCurrentZoneID() << std::endl;
    Require(result == SPELL_CAST_OK && summons->size() == 1, "The real Orb must create exactly one owned Ettin");
    auto actor = player.GetMap()->GetCreature(*summons->begin());
    Require(actor && actor->ToTempSummon() && actor->ToTempSummon()->GetSummonerGUID() == player.GetGUID(), "The summoned Ettin must belong to its actual player");
    return actor;
}

void CheckOslow(Quest const* rescue, bool releaseEarly)
{
    Map map(0, 300, 0, DIFFICULTY_NONE);
    auto placement = sObjectMgr->GetGOData(18302);
    Require(placement, "Missing the original placed Orb");
    map.LoadGrid(placement->posX, placement->posY);
    auto source = map.GetGameObject(ObjectGuid::Create<HighGuid::GameObject>(0, 204351, 18302));
    Require(source && source->hasQuest(OslowQuest), "The actual Orb must offer 26520");
    Group group;
    LootPlayer owner(SupplySession(), map, 2652001, source->GetPosition());
    LootPlayer other(SupplySession(), map, 2652002, source->GetPosition());
    LootPlayer nearby(SupplySession(), map, 2652003, source->GetPosition());
    LootPlayer remote(SupplySession(), map, 2652004, source->GetPosition());
    Accept(owner, *source);
    Accept(other, *source);
    Accept(nearby, *source);
    Accept(remote, *source);

    auto ownedEttin = ControlEttin(owner, *Spawn(map, 43094, 38410));
    auto otherEttin = ControlEttin(other, *Spawn(map, 43094, 38466));
    auto boulder = Spawn(map, Boulder, 38727);
    auto position = boulder->GetPosition();
    MoveGilneasPlayer(owner, position);
    MoveGilneasPlayer(other, position);
    Position ownedPosition = position;
    ownedPosition.m_positionX += 8.0f;
    otherEttin->UpdatePosition(position, true);
    ownedEttin->UpdatePosition(ownedPosition, true);
    map.MoveAllCreaturesInMoveList();
    auto result = owner.CastSpell(&owner, UseOrb, false, owner.GetItemByEntry(Orb));
    Tick(owner, 1000);
    boulder->Update(1500);
    auto first = ownedEttin->GetMotionMaster()->GetCurrentMovementGeneratorType();
    auto second = otherEttin->GetMotionMaster()->GetCurrentMovementGeneratorType();
    std::cout << "CASE Oslow lift cast=" << uint32(result) << " owner movement=" << uint32(first) << " other movement=" << uint32(second) << std::endl;
    Require(result == SPELL_CAST_OK && first == POINT_MOTION_TYPE && second != POINT_MOTION_TYPE,
        "The Orb must move the caster's actual Ettin, never the closest foreign follower");
    Require(other.CastSpell(&other, UseOrb, false, other.GetItemByEntry(Orb)) == SPELL_CAST_OK,
        "A second player's use must not cancel the existing attempt");
    Tick(other, 1000);
    Require(boulder->AI()->GetGUID(2) == ownedEttin->GetGUID() && owner.GetQuestStatus(OslowQuest) == QUEST_STATUS_INCOMPLETE,
        "The shared rock changed owner or granted progress before lifting");

    ObjectGuid ownerEttinId = ownedEttin->GetGUID();
    ObjectGuid otherEttinId = otherEttin->GetGUID();
    ObjectGuid boulderId = boulder->GetGUID();
    if (releaseEarly)
    {
        for (uint32 elapsed = 0; elapsed < 15000 && boulder->GetVehicleBase() != ownedEttin; elapsed += 100)
        {
            Tick(owner, 100);
            boulder->Update(100);
            ownedEttin->Update(100);
            map.MoveAllCreaturesInMoveList();
        }
        Require(boulder->GetVehicleBase() == ownedEttin, "The early-exit case must reach real boulder boarding");
        boulder->ExitVehicle();
        for (uint32 elapsed = 0; elapsed < 1000; elapsed += 100)
        {
            boulder->Update(100);
            if (Creature* actor = map.GetCreature(ownerEttinId))
                actor->Update(100);
            map.MoveAllCreaturesInMoveList();
            map.RemoveAllObjectsInRemoveList();
        }
        std::cout << "CASE Oslow early dismount vehicle=" << bool(boulder->GetVehicleBase())
            << " binding=" << !boulder->AI()->GetGUID(2).IsEmpty() << " status=" << owner.GetQuestStatus(OslowQuest)
            << " foreman=" << owner.GetQuestObjectiveData(OslowQuest, Foreman) << std::endl;
        Require(owner.GetQuestObjectiveData(OslowQuest, Foreman) == 0 && owner.GetQuestStatus(OslowQuest) == QUEST_STATUS_INCOMPLETE &&
            boulder->AI()->GetGUID(2).IsEmpty(), "An early boulder release must end the attempt without awarding credit");
        std::cout << "CASE an acknowledged early boulder dismount aborts the attempt without rescue credit\n";
    }
    AbandonSupply(owner, OslowQuest);
    for (uint32 step = 0; step < 10; ++step)
    {
        boulder->Update(100);
        if (Creature* actor = map.GetCreature(ownerEttinId))
            actor->Update(100);
        map.MoveAllCreaturesInMoveList();
        map.RemoveAllObjectsInRemoveList();
    }
    std::cout << "CASE Oslow abandon own=" << (map.GetCreature(ownerEttinId) != nullptr)
        << " other=" << (map.GetCreature(otherEttinId) == otherEttin)
        << " binding=" << !boulder->AI()->GetGUID(2).IsEmpty() << std::endl;
    Require(!map.GetCreature(ownerEttinId) && map.GetCreature(otherEttinId) == otherEttin && boulder->AI()->GetGUID(2).IsEmpty(),
        "Abandoning must release only the active player's binding and permit another rescue");

    group.*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(26520, 0);
    for (LootPlayer* player : {&other, &nearby, &remote})
    {
        Group::MemberSlot slot;
        slot.Guid = player->GetGUID();
        slot.Class = player->getClass();
        const_cast<Group::MemberSlotList&>(group.GetMemberSlots()).push_back(slot);
        player->SetGroup(&group, 0);
    }
    MoveGilneasPlayer(nearby, position);
    MoveGilneasPlayer(other, position);
    Require(other.CastSpell(&other, UseOrb, false, other.GetItemByEntry(Orb)) == SPELL_CAST_OK,
        "The next eligible player should be able to try the actual rock");
    Tick(other, 1000);

    bool sawPassenger = false;
    bool sawRelease = false;
    uint32 elapsed = 0;
    for (; elapsed < 122000 && other.GetQuestStatus(OslowQuest) == QUEST_STATUS_INCOMPLETE; elapsed += 100)
    {
        Tick(other, 100);
        if (Creature* actor = map.GetCreature(otherEttinId))
        {
            actor->Update(100);
            sawPassenger |= actor->GetVehicleKit() && actor->GetVehicleKit()->GetPassenger(0) == boulder;
            sawRelease |= sawPassenger && actor->GetVehicleKit() && !actor->GetVehicleKit()->GetPassenger(0);
            if (elapsed % 10000 == 0)
                std::cout << "CASE Oslow path " << elapsed << ' ' << actor->GetPositionX() << '/' << actor->GetPositionY()
                    << " movement=" << uint32(actor->GetMotionMaster()->GetCurrentMovementGeneratorType())
                    << " boarded=" << sawPassenger << " released=" << sawRelease << '\n';
        }
        if (Creature* rock = map.GetCreature(boulderId))
            rock->Update(100);
        std::list<Creature*> workers;
        for (uint32 actor : {341u, 648u, 649u, 650u, 651u, 652u, 653u})
            other.GetCreatureListWithEntryInGrid(workers, actor, 90.0f);
        for (Creature* worker : workers)
            worker->Update(100);
        map.MoveAllCreaturesInMoveList();
        map.RemoveAllObjectsInRemoveList();
    }
    std::cout << "CASE Oslow elapsed=" << elapsed << " boarding=" << sawPassenger << " release=" << sawRelease
        << " status=" << other.GetQuestStatus(OslowQuest) << " group=" << nearby.GetQuestObjectiveData(OslowQuest, Foreman)
        << '/' << remote.GetQuestObjectiveData(OslowQuest, Foreman) << std::endl;
    Require(sawPassenger && sawRelease && other.GetQuestObjectiveData(OslowQuest, Foreman) == 1 &&
        other.CanRewardQuest(rescue, false) && owner.GetQuestStatus(OslowQuest) == QUEST_STATUS_NONE &&
        nearby.GetQuestObjectiveData(OslowQuest, Foreman) == 1 && remote.GetQuestObjectiveData(OslowQuest, Foreman) == 0,
        "The actual lift must give one rescue to its owner and nearby group member, and exclude a distant member");

    auto magistrate = Spawn(map, 344, 38626);
    MoveGilneasPlayer(other, magistrate->GetPosition());
    uint32 choice = 0;
    if (auto items = sDB2Manager.GetQuestPackageItems(rescue->PackageID))
        for (auto item : *items)
            if (other.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    if (!choice)
        if (auto fallback = sDB2Manager.GetQuestPackageItemsFallback(rescue->PackageID); fallback && !fallback->empty())
            choice = fallback->front()->ItemID;
    Require(choice && magistrate->hasInvolvedQuest(OslowQuest) && other.CanRewardQuest(rescue, choice, false),
        "Magistrate Solomon must offer the genuine class reward");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = magistrate->GetGUID();
    reward.QuestID = OslowQuest;
    reward.ItemChoiceID = choice;
    other.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(other.GetQuestRewardStatus(OslowQuest) && other.GetItemCount(choice) && !other.GetItemCount(Orb),
        "The actual questgiver must reward the rescue and retrieve the Orb");
    for (uint32 elapsed = 0; elapsed < 25000 && map.GetCreature(otherEttinId); elapsed += 100)
    {
        if (auto actor = map.GetCreature(otherEttinId))
            actor->Update(100);
        map.MoveAllCreaturesInMoveList();
        map.RemoveAllObjectsInRemoveList();
    }
    Require(!map.GetCreature(otherEttinId), "The original Ettin departure must end even after turn-in");
    std::cout << "CASE actual group credit, reward and Ettin departure complete\n";
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
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "The isolated login pool is required");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "The isolated character pool is required");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The test must use isolated fixture characters");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        Quest const* rescue = LoadOslow();
        CheckOslow(rescue, false);
        CheckOslow(rescue, true);
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
