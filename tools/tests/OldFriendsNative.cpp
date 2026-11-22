#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "CreatureTextMgr.h"
#include "GilneasQuestNative.h"
#include "OldFriendsFlight.h"
#include "ScriptSystem.h"
#include "Vehicle.h"

void AddSC_lost_isle();

struct SassyRepeatTag
{
    using Type = CreatureTextRepeatIds (CreatureTextMgr::*)(Creature*, uint8);
    friend Type Access(SassyRepeatTag);
};
template struct FixtureAccess<SassyRepeatTag, &CreatureTextMgr::GetRepeatGroup>;

namespace
{
constexpr uint32 VolcanothQuest = 24958;
constexpr uint32 OldFriends = OldFriendsFlight::Quest;
constexpr uint32 Flight = OldFriendsFlight::Bomber;
constexpr uint32 FlightSassy = OldFriendsFlight::Sassy;
constexpr uint32 BomberSpawn = OldFriendsFlight::CaveBomberSpawn;
constexpr uint32 ThrallSpawn = 242408;

struct FlightPlacement : Creature
{
    FlightPlacement(Map& map, uint32 spawn) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "An original flight actor placement is missing");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true) && IsInWorld(), "An original flight actor did not load into the map");
    }

    ~FlightPlacement()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void LoadOldFriendsInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeOldFriends.inc"
    );
    Require(rows && rows->GetRowCount() == 4, "Missing quest chain for Old Friends");
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
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN(25023,25024)");
    Require(addon && addon->GetRowCount() == 2, "Missing original Old Friends prerequisites");
    do
    {
        auto row = addon->Fetch();
        auto quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        quest->prevQuests.push_back(quest->PrevQuestID);
    } while (addon->NextRow());
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(242281,242408)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_addon AS SELECT * FROM legion_world.creature_addon WHERE guid IN(242281,242408)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_template_addon AS SELECT * FROM legion_world.creature_template_addon WHERE entry IN(38869,38918,38928,38929,38935)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells WHERE npc_entry IN(38918,38929)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE vehicle_template_accessory AS SELECT * FROM legion_world.vehicle_template_accessory WHERE EntryOrAura IN(38918,38929)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text WHERE CreatureID IN(38869,38928,38935)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(25023,25024)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=25023");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE area=4720 AND spell=73065");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_target_position AS SELECT * FROM legion_world.spell_target_position WHERE id IN(73105,73135)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE script_waypoint AS SELECT * FROM legion_world.script_waypoint WHERE entry=38918");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadCreatureTemplateAddons();
    sObjectMgr->LoadCreatureAddons();
    sObjectMgr->LoadNPCSpellClickSpells();
    sObjectMgr->LoadVehicleTemplateAccessories();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sCreatureTextMgr->LoadCreatureTexts();
    sSpellMgr->LoadSpellAreas();
    sSpellMgr->LoadSpellTargetPositions();
    sScriptSystemMgr->LoadScriptWaypoints();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_lost_isle();
    sScriptMgr->SwapScriptContext(true);
    Require(sScriptSystemMgr->GetPointMoveList(Flight).size() == 25, "Missing original Flying Bomber route");
}

void CheckOldFriends()
{
    for (uint32 spawn : {BomberSpawn, ThrallSpawn})
        if (auto data = sObjectMgr->GetCreatureData(spawn))
            sObjectMgr->RemoveCreatureFromGrid(spawn, data);
    Map map(648, 608, 0, DIFFICULTY_NONE);
    FlightPlacement caveAircraft(map, BomberSpawn), lookoutThrall(map, ThrallSpawn);
    auto caveBomber = &caveAircraft;
    auto thrall = &lookoutThrall;
    LootPlayer owner(SupplySession(), map, 2502301, caveBomber->GetPosition()), other(SupplySession(), map, 2502302, caveBomber->GetPosition());
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_GOBLIN);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(RACE_GOBLIN);
        player->InitDisplayIds();
        player->GetReputationMgr().LoadFromDB({});
        player->SetLevel(10);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE, UNIT_FIELD_MOD_HASTE_REGEN})
            player->SetFloatValue(field, 1.0f);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        player->SetRewardedQuest(VolcanothQuest);
        MoveGilneasPlayer(*player, caveBomber->GetPosition());
    }
    auto caveKit = caveBomber->GetVehicleKit();
    Require(caveKit && owner.InSamePhase(caveBomber), "Cave bomber or its actual post-Volcanoth phase is missing");
    for (unsigned i = 0; i < 200 && !caveKit->GetPassenger(0); ++i)
    {
        caveBomber->Update(10);
        if (Creature* passenger = caveBomber->FindNearestCreature(38928, 10.0f, true))
            passenger->Unit::Update(10);
    }
    auto sassy = caveKit->GetPassenger(0);
    Require(sassy && sassy->GetEntry() == 38928, "The real cave Sassy did not board");
    other.CastSpell(&other, OldFriendsFlight::Summon, true);
    Require(other.GetSummonList(Flight)->empty(), "Flight was created without an active Old Friends attempt");
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = sassy->GetGUID();
    accept.QuestID = OldFriends;
    owner.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(owner.GetQuestStatus(OldFriends) == QUEST_STATUS_COMPLETE && owner.m_homebindMapId == 648 &&
        std::fabs(owner.m_homebindY - 2720.48f) < 0.01f, "Native acceptance or the Warchief's Lookout bind failed");
    auto bomber = owner.FindNearestCreature(Flight, 40.0f, true);
    Require(bomber && bomber->GetVehicleKit() && bomber->CanFly(), "The original summoning spell did not create a flying bomber");
    auto kit = bomber->GetVehicleKit();
    Require(bomber->HasQuestObjectScope() && owner.canSeeOrDetect(bomber) && !other.canSeeOrDetect(bomber),
        "The actual bomber was shared or not visible to its owner");
    other.CastSpell(bomber, 73137, true);
    for (unsigned i = 0; i < 200 && (owner.GetVehicle() != kit || !kit->GetPassenger(0)); ++i)
    {
        bomber->Update(10);
        if (Creature* passenger = bomber->FindNearestCreature(FlightSassy, 10.0f, true))
            passenger->Unit::Update(10);
        Tick(owner, 10);
    }
    std::cout << "CASE flight seating player=" << (owner.GetVehicle() == kit)
        << " Sassy=" << (kit->GetPassenger(0) ? kit->GetPassenger(0)->GetEntry() : 0) << std::endl;
    Tick(other, 100);
    Require(owner.GetVehicle() == kit && !other.GetVehicle() && kit->GetPassenger(0) && kit->GetPassenger(0)->GetEntry() == FlightSassy &&
        kit->GetPassenger(0)->HasQuestObjectScope() && !other.canSeeOrDetect(kit->GetPassenger(0)),
        "Native seating did not keep the personal Sassy and the actual owner paired");
    auto flightGuid = bomber->GetGUID();
    auto pilotGuid = kit->GetPassenger(0)->GetGUID();
    for (unsigned i = 0; i < 2800 && owner.GetVehicle() == kit; ++i)
    {
        bomber->Update(100);
        map.MoveAllCreaturesInMoveList();
        Tick(owner, 100);
    }
    std::cout << "CASE flight endpoint " << bomber->GetPositionX() << ',' << bomber->GetPositionY() << ','
        << bomber->GetPositionZ() << " player=" << owner.GetPositionX() << ',' << owner.GetPositionY() << ','
        << owner.GetPositionZ() << std::endl;
    Require(!owner.GetVehicle() && std::fabs(bomber->GetPositionY() - 2684.93f) < 1.0f,
        "The original escort did not reach the lookout and exit the intended passenger");
    Tick(owner, 2000);
    float ground = map.GetHeight(owner.GetPositionX(), owner.GetPositionY(), owner.GetPositionZ());
    std::cout << "CASE terminal spline ended z=" << owner.GetPositionZ() << " ground=" << ground << std::endl;
    Require(std::fabs(owner.GetPositionZ() - ground) < 0.1f && owner.GetUnitBeingMoved() == &owner,
        "Terminal exit ended in the air or failed to restore normal mover control");
    constexpr uint32 messages[] = {38943, 38958, 38970, 38971, 38973, 38992};
    auto const& story = sCreatureTextMgr->GetTextMap().at(FlightSassy);
    bool dialogueComplete = true;
    for (uint8 group = 0; group < std::size(messages); ++group)
    {
        auto text = story.find(group);
        auto sent = (sCreatureTextMgr->*Access(SassyRepeatTag{}))(kit->GetPassenger(0)->ToCreature(), group);
        auto broadcast = text != story.end() && text->second.size() == 1 ? text->second.front().BroadcastTextID : 0;
        std::cout << "CASE Sassy dialogue group=" << uint32(group) << " broadcast=" << broadcast << " sent=" << sent.size() << std::endl;
        dialogueComplete = dialogueComplete && broadcast == messages[group] && sent.size() == 1;
    }
    Require(dialogueComplete, "A recovered native Sassy dialogue cue was absent or not triggered on the flight");
    MoveGilneasPlayer(owner, thrall->GetNearPosition(2.0f, 0.0f));
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = OldFriends;
    reward.QuestGiverGUID = thrall->GetGUID();
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(OldFriends) && thrall->hasQuest(25024) && owner.CanTakeQuest(sQuestDataStore->GetQuestTemplate(25024), false),
        "The real Thrall did not accept the completed trip and offer Repel the Paratroopers");
    other.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(other.GetQuestStatus(OldFriends) == QUEST_STATUS_COMPLETE, "Second player could not accept the actual Sassy offer");
    auto repeat = other.FindNearestCreature(Flight, 40.0f, true);
    Require(repeat && repeat->GetGUID() != flightGuid, "Second actual attempt did not obtain a distinct aircraft");
    auto abandoned = repeat->GetGUID();
    Tick(other, 100);
    Require(other.GetVehicleBase() == repeat, "Abandonment scenario requires an actual passenger");
    AbandonSupply(other, OldFriends);
    for (unsigned i = 0; i < 30; ++i)
    {
        for (ObjectGuid guid : {abandoned, flightGuid, pilotGuid})
            if (auto actor = map.GetCreature(guid))
                actor->Update(100);
        Tick(other, 100);
        map.RemoveAllObjectsInRemoveList();
    }
    std::cout << "CASE after actual abandon aircraft=" << (map.GetCreature(abandoned) != nullptr)
        << " rider=" << (other.GetVehicle() != nullptr) << " rewarded flight=" << (map.GetCreature(flightGuid) != nullptr)
        << " rewarded pilot=" << (map.GetCreature(pilotGuid) != nullptr) << std::endl;
    Require(!map.GetCreature(abandoned) && !other.GetVehicle() && other.GetUnitBeingMoved() == &other &&
        !map.GetCreature(flightGuid) && !map.GetCreature(pilotGuid),
        "Actual abandonment or reward retained one of the personal flight actors or a passenger seat");
    other.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(other.GetQuestStatus(OldFriends) == QUEST_STATUS_COMPLETE && other.FindNearestCreature(Flight, 40.0f, true),
        "The actual cave offer could not start a fresh attempt after abandonment");
    AbandonSupply(other, OldFriends);
    map.RemoveAllObjectsInRemoveList();
    std::cout << "PASS native Old Friends offer, authorized seats, full flight and dialogue, ground landing, Thrall reward, abandonment and retry\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{648, {}}, {659, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{648, {}}, {659, {}}});
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Old Friends checks require isolated characters");
        LoadOldFriendsInputs();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckOldFriends();
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
