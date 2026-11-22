#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GilneasQuestNative.h"
#include "SmartScriptMgr.h"
#include <thread>

namespace
{
constexpr uint32 Mission = 26651;
constexpr uint32 Prisoners = 26646;
constexpr uint32 Kit = 60385;
constexpr uint32 EquipKit = 82587;
constexpr uint32 PlantCharge = 82585;
constexpr uint32 Hut = 43589;
constexpr uint32 Tower = 43590;
constexpr uint32 GiverSpawn = 343676;
constexpr uint32 ReceiverSpawn = 260620;
constexpr uint32 HutSpawn = 39071;
constexpr uint32 TowerSpawn = 39103;
constexpr uint32 FocusEntry = 301307;
constexpr uint32 HutFocusSpawn = 106469;
constexpr uint32 TowerFocusSpawn = 106470;

Quest const* LoadMissionInputs()
{
    auto templates = WorldDatabase.Query(
#include "NativeRedridgeSeaforium.inc"
    );
    Require(templates && templates->GetRowCount() == 2, "The original Redridge mission chain is missing");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(templates->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (templates->NextRow());
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=26651 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 2, "Missing the native charge objectives");
    auto quest = quests[Mission];
    quest->LoadQuestObjective(objectives->Fetch());
    Require(objectives->NextRow(), "Missing the second charge objective");
    quest->LoadQuestObjective(objectives->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=26651");
    Require(addon && addon->Fetch()[4].GetUInt32() == Prisoners, "Prisoners of War no longer precedes the field kit");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(Prisoners);
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    Require(quest->SourceItemId == Kit && quest->GetObjectives()[0].ObjectID == Hut && quest->GetObjectives()[1].ObjectID == Tower,
        "The field kit or original charge targets changed");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sObjectMgr->LoadPlayerInfo();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    auto item = sObjectMgr->GetItemTemplate(Kit);
    auto bar = sOverrideSpellDataStore.LookupEntry(279);
    auto aura = sSpellMgr->GetSpellInfo(EquipKit)->GetEffect(EFFECT_1);
    auto charge = sSpellMgr->GetSpellInfo(PlantCharge)->GetEffect(EFFECT_0);
    Require(item && std::any_of(item->Effects.begin(), item->Effects.end(), [](ItemEffectEntry const* effect)
        { return effect && effect->SpellID == EquipKit && !effect->Charges; }) && bar &&
        std::find(std::begin(bar->Spells), std::end(bar->Spells), PlantCharge) != std::end(bar->Spells) &&
        aura && aura->ApplyAuraName == SPELL_AURA_OVERRIDE_SPELLS && aura->MiscValue == 279 &&
        charge && charge->TargetA.GetTarget() == TARGET_UNIT_NEARBY_ENTRY &&
        sSpellMgr->GetSpellInfo(PlantCharge)->CastingReq.RequiresSpellFocus == 1676, "The native field-kit bar, selector or focus changed");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions "
        "WHERE SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(82585,82587)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE spell=82587");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts "
        "WHERE source_type=0 AND entryorguid IN(43589,43590)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature "
        "WHERE guid IN(39071,39103,343676,260620) AND map=0 AND zoneId=44");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=26651");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=26651");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sConditionMgr->LoadConditions();
    sSpellMgr->LoadSpellAreas();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sObjectMgr->LoadEquipmentTemplates();
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    return quest;
}

struct MissionActor : Creature
{
    MissionActor(Map& map, uint32 spawn) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "A native Blackrock mission placement is missing");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true) && IsInWorld(), "Could not publish the original mission actor");
    }

    ~MissionActor()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

struct MissionFocus : GameObject
{
    MissionFocus(Map& map, uint32 spawn)
    {
        auto result = WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
            "FROM gameobject WHERE guid=%u AND id=301307 AND map=0 AND zoneId=44 AND areaId=997 AND phaseMask=1", spawn);
        Require(bool(result), "An original Blackrock spell focus is missing");
        auto row = result->Fetch();
        Position position(row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat());
        G3D::Quat rotation(row[4].GetFloat(), row[5].GetFloat(), row[6].GetFloat(), row[7].GetFloat());
        m_DBTableGuid = spawn;
        Require(Create(spawn, FocusEntry, &map, 1, position, rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "The genuine Seaforium spell focus could not be published");
    }

    ~MissionFocus()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

SpellCastResult CastMissionSpell(LootPlayer& player, uint32 spellId, Item* item = nullptr)
{
    GameTime::UpdateGameTimers();
    auto info = sSpellMgr->GetSpellInfo(spellId);
    auto delay = std::max<double>(player.GetSpellCooldownDelay(spellId) * 1000.0, info->Cooldowns.StartRecoveryTime);
    if (delay > 0.0)
    {
        Require(delay < 10000.0, "The field-kit spell gained a long cooldown");
        std::this_thread::sleep_for(std::chrono::milliseconds(uint32(delay) + 60));
        GameTime::UpdateGameTimers();
    }
    auto result = player.CastSpell(&player, spellId, false, item);
    Tick(player, info->CalcCastTime() + 1000);
    return result;
}

SpellCastResult UseKit(LootPlayer& player)
{
    return CastMissionSpell(player, EquipKit, player.GetItemByEntry(Kit));
}

SpellCastResult Plant(LootPlayer& player)
{
    return CastMissionSpell(player, PlantCharge);
}

void CheckMission(Quest const* quest, bool baseline)
{
    for (uint32 spawn : {GiverSpawn, ReceiverSpawn, HutSpawn, TowerSpawn})
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "The production spawn was omitted by the loader");
        sObjectMgr->RemoveCreatureFromGrid(spawn, data);
    }
    Map map(0, 626, 0, DIFFICULTY_NONE);
    MissionActor giver(map, GiverSpawn), receiver(map, ReceiverSpawn), hut(map, HutSpawn), tower(map, TowerSpawn);
    MissionFocus hutFocus(map, HutFocusSpawn), towerFocus(map, TowerFocusSpawn);
    Require(hutFocus.IsWithinDistInMap(&hut, 10.0f) && towerFocus.IsWithinDistInMap(&tower, 10.0f),
        "A native focus and its exact charge target are no longer paired");
    LootPlayer owner(SupplySession(), map, 2665101, giver.GetPosition()), outsider(SupplySession(), map, 2665102, giver.GetPosition());
    for (auto player : {&owner, &outsider})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_HUMAN);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_WARRIOR);
        player->setFactionForRace(RACE_HUMAN);
        player->InitDisplayIds();
        player->GetReputationMgr().LoadFromDB({});
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE})
            player->SetFloatValue(field, 1.0f);
    }
    auto accept = [&]
    {
        MoveGilneasPlayer(owner, giver.GetPosition());
        WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
        packet.QuestGiverGUID = giver.GetGUID();
        packet.QuestID = Mission;
        owner.GetSession()->HandleQuestGiverAcceptQuest(packet);
    };
    accept();
    Require(owner.GetQuestStatus(Mission) == QUEST_STATUS_NONE, "Native acceptance skipped Prisoners of War");
    owner.SetRewardedQuest(Prisoners);
    accept();
    Require(owner.GetQuestStatus(Mission) == QUEST_STATUS_INCOMPLETE && owner.GetItemCount(Kit) == 1,
        "The eligible player did not receive the real Bravo Company kit");
    MoveGilneasPlayer(outsider, hut.GetPosition());
    GiveSupply(outsider, Kit, 1);
    auto unauthorizedKit = UseKit(outsider);
    MoveGilneasPlayer(owner, hut.GetPosition());
    auto prematureCharge = Plant(owner);
    std::cout << "CASE outside kit=" << unsigned(unauthorizedKit) << " aura=" << outsider.HasAura(EquipKit)
        << " direct charge=" << unsigned(prematureCharge) << " hut=" << owner.GetQuestObjectiveData(Mission, Hut)
        << " area=" << owner.GetAreaId() << " visible=" << owner.canSeeOrDetect(&hut) << std::endl;
    if (baseline)
    {
        Require(unauthorizedKit == SPELL_CAST_OK && outsider.HasAura(EquipKit) &&
            prematureCharge == SPELL_CAST_OK && owner.GetQuestObjectiveData(Mission, Hut) == 1,
            "Could not reproduce equipping the stale kit and bypassing the original bar");
        std::cout << "EXPECTED_BROKEN a stale kit equips without a quest and the direct charge gives credit without equipping\n";
        return;
    }
    Require(unauthorizedKit != SPELL_CAST_OK && !outsider.HasAura(EquipKit) && prematureCharge != SPELL_CAST_OK &&
        !owner.GetQuestObjectiveData(Mission, Hut), "The kit or its charge bypassed the actual active attempt");
    Require(UseKit(owner) == SPELL_CAST_OK && owner.HasAura(EquipKit) && owner.HasSpell(PlantCharge),
        "The native kit did not expose the charge on its action bar");
    MoveGilneasPlayer(owner, hut.GetNearPosition(28.0f, 2.2f));
    auto away = Plant(owner);
    std::cout << "CASE away charge=" << unsigned(away) << " aura=" << owner.HasAura(EquipKit)
        << " zone=" << owner.GetZoneId() << " area=" << owner.GetAreaId()
        << " focus distance=" << owner.GetDistance(&hutFocus) << std::endl;
    Require(away == SPELL_FAILED_REQUIRES_SPELL_FOCUS &&
        !owner.GetQuestObjectiveData(Mission, Hut) && !owner.GetQuestObjectiveData(Mission, Tower),
        "A charge away from both real spell foci was accepted");
    MoveGilneasPlayer(owner, hut.GetPosition());
    owner.DestroyItemCount(Kit, 1, true);
    Require(Plant(owner) != SPELL_CAST_OK && !owner.GetQuestObjectiveData(Mission, Hut), "A remaining aura accepted a missing field kit");
    GiveSupply(owner, Kit, 1);
    Require(Plant(owner) == SPELL_CAST_OK && owner.GetQuestObjectiveData(Mission, Hut) == 1 &&
        !owner.GetQuestObjectiveData(Mission, Tower), "The native nearby-entry cast did not credit exactly the munitions hut");
    Require(Plant(owner) == SPELL_CAST_OK && owner.GetQuestObjectiveData(Mission, Hut) == 1 &&
        !owner.GetQuestObjectiveData(Mission, Tower), "Repeating the same target advanced another objective");
    AbandonSupply(owner, Mission);
    Require(!owner.HasAura(EquipKit) && !owner.HasSpell(PlantCharge) && !owner.GetItemCount(Kit),
        "Abandonment retained the kit, aura or temporary charge spell");

    accept();
    Require(owner.GetQuestStatus(Mission) == QUEST_STATUS_INCOMPLETE && owner.GetItemCount(Kit) == 1 &&
        !owner.GetQuestObjectiveData(Mission, Hut), "A fresh attempt did not get its own kit and uncredited objectives");
    MoveGilneasPlayer(owner, tower.GetPosition());
    Require(UseKit(owner) == SPELL_CAST_OK && owner.HasAura(EquipKit), "Reacceptance did not restore the original field kit");
    Require(Plant(owner) == SPELL_CAST_OK && owner.GetQuestObjectiveData(Mission, Tower) == 1 &&
        !owner.GetQuestObjectiveData(Mission, Hut), "The genuine tower target did not credit on its own");
    MoveGilneasPlayer(owner, hut.GetPosition());
    Require(Plant(owner) == SPELL_CAST_OK && owner.GetQuestStatus(Mission) == QUEST_STATUS_COMPLETE && !owner.HasAura(EquipKit),
        "The second structure did not complete and clean up the native charge state");
    MoveGilneasPlayer(owner, receiver.GetPosition());
    Require(owner.CanRewardQuest(quest, 0, false), "Keeshan rejected the completed charge objectives");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = Mission;
    reward.QuestGiverGUID = receiver.GetGUID();
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(Mission) && !owner.GetItemCount(Kit), "The actual receiver did not award the quest and remove its kit");
    std::cout << "PASS the genuine kit override, both world structures, quest admission, abandonment and reward\n";
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
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The Redridge check requires isolated characters");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
#ifdef REDRIDGE_SEAFORIUM_BASELINE
        CheckMission(LoadMissionInputs(), true);
#else
        CheckMission(LoadMissionInputs(), false);
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
