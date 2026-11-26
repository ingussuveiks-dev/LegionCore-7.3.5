#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "ConditionMgr.h"
#include "CreatureTextMgr.h"
#include "GilneasQuestNative.h"
#include "MapInstanced.h"
#include "MovementPackets.h"
#include "ScriptsData.h"
#include "SmartScriptMgr.h"
#include "Vehicle.h"
#include "WestfallVision.h"

void AddSC_deadmines();

namespace
{
constexpr uint32 Vision = 26320;
constexpr uint32 Secrets = 26319;
constexpr uint32 Incense = 58147;
constexpr uint32 IncenseSpell = 79586;
constexpr uint32 VisionActor = 42693;
constexpr uint32 Focus = 301550;

void LoadVision()
{
    auto rows = WorldDatabase.Query(
#include "NativeWestfallVision.inc"
    );
    Require(rows && rows->GetRowCount() == 3, "The original Westfall quest templates are required");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=26320");
    Require(bool(addon), "The original Vision quest requirements are missing");
    quests[Vision]->LoadQuestTemplateAddon(addon->Fetch());
    quests[Vision]->prevQuests.push_back(Secrets);
    quests[26322]->prevQuests.push_back(Vision);
    Require(quests[Vision]->SourceItemId == Incense && quests[Vision]->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT),
        "The original quest tool and event requirement are required");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE (guid=107503 AND id=42651) OR (guid=107102 AND id=234)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject AS SELECT * FROM legion_world.gameobject WHERE guid=106562 AND id=301550");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN (26320,26322)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text "
        "WHERE CreatureID IN (42651,42693,42371,42697,42699)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 AND entryorguid IN (42651,42693)");
#ifdef WESTFALL_VISION_SCENE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id=79586");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=26320");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE phase_definitions AS SELECT * FROM legion_world.phase_definitions WHERE zoneId=40 AND entry=6");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=40 AND SourceEntry=6");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sObjectMgr->LoadGameobjects();
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sCreatureTextMgr->LoadCreatureTexts();
    sSmartScriptMgr->LoadSmartAIFromDB();
#ifdef WESTFALL_VISION_SCENE
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sObjectMgr->LoadPhaseDefinitions();
    sConditionMgr->LoadConditions();
    sObjectMgr->LoadInstanceTemplate();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_deadmines();
    sScriptMgr->SwapScriptContext(true);
    sScriptDataStore->LoadSpellScriptNames();
#endif
}

void Prepare(LootPlayer& player)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
    player.setFactionForRace(player.getRace());
    player.InitDisplayIds();
    player.SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
    player.SetLevel(20);
    player.SetRewardedQuest(Secrets);
    MoveGilneasPlayer(player, player.GetPosition());
}

void CheckVisionCatalog()
{
    for (uint32 id : {79586u, 79587u, 79620u})
    {
        SpellInfo const* spell = sSpellMgr->GetSpellInfo(id);
        Require(spell, "The matching Legion Vision spell must be present");
        std::cout << "CASE Vision catalog spell=" << id << " duration=" << spell->GetDuration()
            << " areaGroup=" << spell->CastingReq.RequiredAreasID;
        for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            if (auto effect = spell->Effects[i]; effect && effect->Effect)
                std::cout << " effect=" << i << '/' << effect->Effect << '/' << effect->ApplyAuraName << '/'
                    << effect->MiscValue << '/' << effect->MiscValueB << '/' << effect->BasePoints << '/'
                    << effect->TriggerSpell << '/' << effect->TargetA.GetTarget() << '/' << effect->TargetB.GetTarget();
        std::cout << '\n';
    }
    auto properties = sSummonPropertiesStore.LookupEntry(827);
    auto vehicle = sVehicleStore.LookupEntry(WestfallVision::Vehicle);
    auto seat = vehicle ? sVehicleSeatStore.LookupEntry(vehicle->SeatID[0]) : nullptr;
    Require(properties && properties->Control == SUMMON_CATEGORY_VEHICLE && seat && seat->ID == 8111 && seat->CanEnterOrExit(),
        "The matching Legion vehicle summon must have the original usable seat");
    std::cout << "CASE Vision summon properties=" << properties->ID << " category=" << properties->Control
        << " title=" << properties->Title << " flags=" << properties->Flags << " slot=" << properties->Slot
        << " vehicle=" << vehicle->ID << " flags=" << vehicle->Flags << " flagsB=" << uint32(vehicle->FlagsB);
    for (uint16 id : vehicle->SeatID)
        if (auto seat = sVehicleSeatStore.LookupEntry(id))
            std::cout << " seat=" << id << '/' << seat->Flags << '/' << seat->FlagsB << '/' << seat->CanEnterOrExit();
    std::cout << '\n';
}

#ifdef WESTFALL_VISION_SCENE
void AcknowledgeVisionExit(LootPlayer& player)
{
    Tick(player, 100);
    if (player.IsBeingTeleportedNear())
    {
        WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};
        ack.MoverGUID = player.GetGUID();
        player.GetSession()->HandleMoveTeleportAck(ack);
    }
}

void CheckVisionScene(LootPlayer& inside, InstanceMap& deadmines)
{
    auto quest = sQuestDataStore->GetQuestTemplate(Vision);
    auto focus = inside.GetPosition();
    LootPlayer second(SupplySession(), deadmines, 2632003, focus);
    Prepare(second);
    second.AddQuestAndCheckCompletion(quest, nullptr);
    inside.AddQuestAndCheckCompletion(quest, nullptr);
    Require(inside.GetQuestStatus(Vision) == QUEST_STATUS_INCOMPLETE && inside.GetItemCount(Incense) == 1,
        "The genuine accepted quest did not provide the item");
    inside.CastSpell(&inside, 79620, true);
    Require(inside.GetQuestStatus(Vision) == QUEST_STATUS_INCOMPLETE, "Credit succeeded outside the real finale");
    Require(inside.CastSpell(&inside, IncenseSpell, false, inside.GetItemByEntry(Incense)) == SPELL_CAST_OK,
        "The genuine incense did not start the native camera summon");
    Require(second.CastSpell(&second, IncenseSpell, false, second.GetItemByEntry(Incense)) == SPELL_CAST_OK,
        "Another eligible player must be able to start their own camera");
    Tick(inside, 1000);
    Tick(second, 1000);
    auto firstCamera = inside.GetVehicleCreatureBase();
    auto secondCamera = second.GetVehicleCreatureBase();
    Require(firstCamera && secondCamera && firstCamera != secondCamera && !second.canSeeOrDetect(firstCamera) &&
        !inside.canSeeOrDetect(secondCamera), "Parallel visions must have separate private vehicles");
    ObjectGuid secondGuid = secondCamera->GetGUID();
    second.ExitVehicle();
    AcknowledgeVisionExit(second);
    deadmines.RemoveAllObjectsInRemoveList();
    Require(!second.GetVehicle() && !second.HasAura(79587) && !deadmines.GetCreature(secondGuid) &&
        second.GetQuestStatus(Vision) == QUEST_STATUS_INCOMPLETE && inside.GetVehicleBase() == firstCamera,
        "An early exit must remove only the exiting player's camera and leave their quest open");
    MoveGilneasPlayer(second, focus);
    Require(second.CastSpell(&second, IncenseSpell, false, second.GetItemByEntry(Incense)) == SPELL_CAST_OK,
        "An early exit must allow another attempt with the original incense");
    Tick(second, 1000);
    secondCamera = second.GetVehicleCreatureBase();
    Require(secondCamera, "Retry did not board its own new camera");
    secondGuid = secondCamera->GetGUID();
    AbandonSupply(second, Vision);
    AcknowledgeVisionExit(second);
    deadmines.RemoveAllObjectsInRemoveList();
    Require(!second.GetVehicle() && !second.HasAura(79587) && !second.GetItemCount(Incense) && !deadmines.GetCreature(secondGuid) &&
        inside.GetVehicleBase() == firstCamera, "Abandonment must clean up the native ride and only its accepted attempt");
    std::cout << "CASE Vision parallel private rides, voluntary exit, retry and abandonment\n";

    bool boarded = false;
    bool sawEdwin = false;
    bool sawDeath = false;
    bool sawVanessa = false;
    uint32 elapsed = 0;
    for (; elapsed < 185000 && inside.GetQuestStatus(Vision) == QUEST_STATUS_INCOMPLETE; elapsed += 100)
    {
        Tick(inside, 100);
        if (auto camera = inside.GetVehicleCreatureBase())
        {
            if (!boarded)
            {
                boarded = camera->GetEntry() == VisionActor && camera->GetVehicleKit()->GetPassenger(0) == &inside && camera->HasQuestObjectScope();
                Require(boarded && inside.GetQuestStatus(Vision) == QUEST_STATUS_INCOMPLETE, "The native seat must board the owner without granting credit");
                Require(inside.CastSpell(&inside, IncenseSpell, false, inside.GetItemByEntry(Incense)) != SPELL_CAST_OK,
                    "The incense could start a second camera during the scene");
            }
            camera->Update(100);

            std::list<Creature*> actors;
            for (uint32 entry : {42371u, 42697u, 42698u, 42699u, 42700u, 42701u, 42702u, 42703u})
                camera->GetCreatureListWithEntryInGrid(actors, entry, 140.0f);
            for (Creature* actor : actors)
            {
                if (actor->GetQuestObjectScope().lock() != camera->GetQuestObjectScope().lock())
                    continue;
                actor->Update(100);
                if (actor->GetEntry() == 42697)
                {
                    sawEdwin = true;
                    sawDeath |= actor->isDead();
                    if (elapsed % 10000 == 0)
                        std::cout << "CASE Vision combat elapsed=" << elapsed << " Edwin health=" << actor->GetHealth()
                            << '/' << actor->GetMaxHealth() << " victim=" << (actor->getVictim() ? actor->getVictim()->GetEntry() : 0) << '\n';
                }
                sawVanessa |= actor->GetEntry() == 42371;
            }
        }
        else
            for (ObjectGuid const& guid : *inside.GetSummonList(VisionActor))
                if (Creature* camera = deadmines.GetCreature(guid))
                    camera->Update(100);

        deadmines.MoveAllCreaturesInMoveList();
        deadmines.RemoveAllObjectsInRemoveList();
    }
    std::cout << "CASE Vision scene boarded=" << boarded << " Edwin=" << sawEdwin << " death=" << sawDeath
        << " Vanessa=" << sawVanessa << " status=" << inside.GetQuestStatus(Vision) << " elapsed=" << elapsed << std::endl;
    Require(boarded && sawEdwin && sawDeath && sawVanessa && inside.GetQuestStatus(Vision) == QUEST_STATUS_COMPLETE,
        "The vision must end only after the native ride, the actual fight and Vanessa's finale");
    Require(!inside.GetVehicle() && !inside.HasAura(79587), "The final vision must clean up the seat and presentation auras");
}

void CheckVisionTurnIn(LootPlayer& player, Map& westfall)
{
    auto data = sObjectMgr->GetCreatureData(107102);
    Require(data, "The original Sentinel Hill quest receiver is required");
    westfall.LoadGrid(data->posX, data->posY);
    auto marshal = westfall.GetCreature(ObjectGuid::Create<HighGuid::Creature>(0, 234, 107102));
    Require(marshal && marshal->hasInvolvedQuest(Vision), "The actual Marshal must receive the Vision quest");
    Position destination = marshal->GetPosition();
    player.Unit::RemoveFromWorld();
    player.RemoveFromGrid();
    player.ResetMap();
    player.Relocate(destination);
    player.SetMap(&westfall);
    westfall.AddToGrid(static_cast<Player*>(&player), Cell(Trinity::ComputeCellCoord(destination.GetPositionX(), destination.GetPositionY())));
    player.Unit::AddToWorld();
    MoveGilneasPlayer(player, destination);
    Require(player.InSamePhase(marshal) && player.CanInteractWithQuestGiver(marshal), "The Marshal must be available in the original turn-in phase");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = marshal->GetGUID();
    reward.QuestID = Vision;
    reward.ItemChoiceID = 0;
    player.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(player.GetQuestRewardStatus(Vision) && marshal->hasQuest(26322) && player.CanTakeQuest(sQuestDataStore->GetQuestTemplate(26322), false),
        "The native Marshal handoff must reward the Vision and expose Brotherhood");
    std::cout << "CASE Vision Marshal reward and Brotherhood offer\n";
}
#endif

void CheckVision()
{
    Map westfall(0, 300, 0, DIFFICULTY_NONE);
    MapInstanced parent(36, 300);
    InstanceMap deadmines(36, 301, 26320, DIFFICULTY_NORMAL, &parent);
    auto giverData = sObjectMgr->GetCreatureData(107503);
    auto focusData = sObjectMgr->GetGOData(106562);
    Require(giverData && focusData, "The original offer and Deadmines focus must be loaded");
    westfall.LoadGrid(giverData->posX, giverData->posY);
    deadmines.LoadGrid(focusData->posX, focusData->posY);
    auto giver = westfall.GetCreature(ObjectGuid::Create<HighGuid::Creature>(0, 42651, 107503));
    LootPlayer inside(SupplySession(), deadmines, 2632002, Position{focusData->posX, focusData->posY, focusData->posZ, focusData->orientation});
    Prepare(inside);
    auto focus = inside.FindNearestGameObject(Focus, 5.0f);
    Require(giver && focus, "The original offer and focus must be present on their normal maps");
    LootPlayer outside(SupplySession(), westfall, 2632001, giver->GetPosition());
    Prepare(outside);
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = giver->GetGUID();
    accept.QuestID = Vision;
    outside.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(outside.GetQuestStatus(Vision) == QUEST_STATUS_INCOMPLETE && outside.GetItemCount(Incense) == 1,
        "Thoralius did not offer the tool while retaining the active event requirement");
    Require(outside.CastSpell(&outside, IncenseSpell, false, outside.GetItemByEntry(Incense)) != SPELL_CAST_OK,
        "The genuine item could be used outside the dungeon focus");
#ifdef WESTFALL_VISION_SCENE
    CheckVisionScene(inside, deadmines);
    CheckVisionTurnIn(inside, westfall);
#elif !defined(WESTFALL_VISION_OBSERVE)
    Require(!WorldDatabase.Query("SELECT 1 FROM smart_scripts WHERE entryorguid=42651 AND event_type=19 AND action_type=15"),
        "The retired acceptance credit is still attached to Thoralius");
    std::cout << "PASS actual Thoralius acceptance provides the item, stays incomplete and refuses an exterior cast\n";
#else
    // The current vision observation is intentionally separate from the clean admission regression.
    inside.AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(Vision), nullptr);
    Require(inside.GetQuestStatus(Vision) == QUEST_STATUS_INCOMPLETE && inside.GetItemCount(Incense) == 1,
        "The genuine accepted quest did not provide the item");
    auto const result = inside.CastSpell(&inside, IncenseSpell, false, inside.GetItemByEntry(Incense));
    Tick(inside, 6000);
    auto const summons = inside.GetSummonList(VisionActor);
    std::list<Creature*> actors;
    inside.GetCreatureListWithEntryInGrid(actors, VisionActor, 55.0f);
    std::cout << "CASE Vision normal focus cast=" << uint32(result) << " quest=" << inside.GetQuestStatus(Vision)
        << " owned summons=" << summons->size() << " nearby actors=" << actors.size() << std::endl;
    Require(result == SPELL_CAST_OK, "The original item did not admit a cast at the normal dungeon focus");
    Require(inside.GetQuestStatus(Vision) == QUEST_STATUS_COMPLETE, "The original native item and summoned actor did not finish their recorded event");
    std::cout << "OBSERVED the native Deadmines summon completed the quest during the initial item cast\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}, {36, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}, {36, {}}});
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
        CheckVisionCatalog();
        LoadVision();
        CheckVision();
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
