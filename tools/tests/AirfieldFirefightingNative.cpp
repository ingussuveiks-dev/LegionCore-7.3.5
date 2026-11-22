#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "SmartAI.h"
#include "SmartScriptMgr.h"

void AddSC_SmartScripts();

namespace
{
constexpr uint32 FireQuest = 26078;
constexpr uint32 Gear = 56803;
constexpr uint32 GearAura = 78363;
constexpr uint32 Extinguish = 78369;
constexpr uint32 BurningBomber = 42046;
constexpr uint32 Commander = 41853;
constexpr uint32 Airfield = 5115;

void ShadowFireRows(char const* name, char const* filter)
{
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE airfield_%s AS SELECT * FROM %s WHERE %s",
        name, name, filter).c_str());
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE %s AS SELECT * FROM airfield_%s", name, name).c_str());
}

Quest const* LoadFireQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeAirfieldFirefighting.inc"
    );
    Require(bool(row), "Missing Extinguish the Fires");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[FireQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(FireQuest, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=26078");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == BurningBomber && objective->Fetch()[5].GetUInt32() == 6,
        "Expected six actual airfield extinguishing credits");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=26078");
    Require(bool(addon), "Missing Extinguish the Fires addon");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    Require(quest->SourceItemId == Gear && quest->SourceItemIdCount == 1 && !quest->PackageID, "Expected the original one gear and fixed reward");
    Require(sSpellMgr->GetSpellInfo(Extinguish)->CastingReq.RequiredAreasID == 2642, "The native airfield area group changed");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_SmartScripts();
    sScriptMgr->SwapScriptContext(true);

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    ShadowFireRows("smart_scripts", "source_type=0 AND entryorguid=42046");
    ShadowFireRows("creature_questender", "quest=26078");
#ifndef AIRFIELD_FIREFIGHTING_BASELINE
    ShadowFireRows("conditions", "(SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(78363,78369)) OR (SourceTypeOrReferenceId=22 AND SourceEntry=42046)");
    ShadowFireRows("spell_area", "spell=78363");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSmartScriptMgr->LoadSmartAIFromDB();
#ifndef AIRFIELD_FIREFIGHTING_BASELINE
    sConditionMgr->LoadConditions();
    sSpellMgr->LoadSpellAreas();
    Require(sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL, Extinguish).size() == 2,
        "The extinguisher's guard did not load");
#endif
    return quest;
}

struct Fire : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(new SmartAI(this)), "The real fire marker's native SmartAI did not load");
    }
};

struct ConeObserver : NullCreatureAI
{
    using NullCreatureAI::NullCreatureAI;
    void SpellHit(Unit*, SpellInfo const* spell) override { Hit = spell && spell->Id == Extinguish; }
    bool Hit = false;
};

SpellCastResult UseGear(LootPlayer& player, uint32 spell)
{
    auto item = player.GetItemByEntry(Gear);
    Require(item, "The test needs the real carried gear");
    return player.CastSpell(&player, spell, false, item);
}

void CheckFirefighting(Quest const* quest)
{
    auto commander = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=41853 AND guid=81965 AND map=0 AND zoneId=1 AND areaId=5115");
    auto markers = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=42046 AND map=0 AND zoneId=1 AND areaId=5115 ORDER BY guid LIMIT 7");
    Require(commander && markers && markers->GetRowCount() == 7 &&
        WorldDatabase.Query("SELECT 1 FROM creature_queststarter WHERE quest=26078 AND id=41853"),
        "Missing the original commander or airfield locations");
    auto point = [](Field* row) { return Position{row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat()}; };
    Map map(0, 510, 0, DIFFICULTY_NONE);
    Position receiverPoint = point(commander->Fetch());
    Actor receiver(&map, Commander, commander->Fetch()[0].GetUInt32(), receiverPoint);
    Require(map.AddToMap(static_cast<Creature*>(&receiver)), "Could not publish Commander Stonebreaker");
    LootPlayer owner(SupplySession(), map, 26078, receiverPoint), other(SupplySession(), map, 26079, receiverPoint);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_DWARF);
        player->setFactionForRace(RACE_DWARF);
        player->InitDisplayIds();
        player->SetLevel(12);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }

    GiveSupply(other, Gear, 1);
    owner.AddQuestAndCheckCompletion(quest, &receiver);
    Require(owner.GetItemCount(Gear) == 1 && !owner.CanRewardQuest(quest, false), "Acceptance should only provide the gear");
    for (uint32 index = 0; index < 7; ++index, markers->NextRow())
    {
        auto row = markers->Fetch();
        Position location = point(row);
        Position facing{location.GetPositionX() - 4.0f, location.GetPositionY(), location.GetPositionZ(), 0.0f};
        SupplyMove(owner, facing);
        SupplyMove(other, facing);
        owner.GetZoneAndAreaId(owner.m_zoneId, owner.m_areaId);
        other.GetZoneAndAreaId(other.m_zoneId, other.m_areaId);
        Require(owner.GetAreaId() == Airfield && other.GetAreaId() == Airfield, "The cast is not inside the real native area group");
        Fire fire(&map, BurningBomber, row[0].GetUInt32(), location);
        Require(map.AddToMap(static_cast<Creature*>(&fire)) && fire.GetAreaId() == Airfield, "Could not publish the intended actual fire");
        fire.GetZoneAndAreaId(fire.m_zoneId, fire.m_areaId);
        if (index == 0)
        {
#ifdef AIRFIELD_FIREFIGHTING_BASELINE
            SpellCastResult auraResult = UseGear(other, GearAura);
            Tick(other, 1800);
            SpellCastResult coneResult = UseGear(other, Extinguish);
            std::cout << "CASE baseline gear=" << auraResult << " cone=" << coneResult << " area=" << other.GetAreaId()
                << " zone=" << other.GetZoneId() << '\n';
            Require(auraResult == SPELL_CAST_OK && coneResult == SPELL_CAST_OK,
                "Expected the original gear and extinguisher to work without the quest");
            Tick(other, 1900);
            fire.AI()->UpdateAI(600);
            Require(other.HasAura(GearAura) && !fire.IsVisible() && other.GetQuestStatus(FireQuest) == QUEST_STATUS_NONE,
                "Expected a nonparticipant to retain the gear aura and remove a shared fire");
            std::cout << "EXPECTED_BROKEN a nonparticipant consumed a shared airfield fire and retained the gear aura\n";
            return;
#endif
            Require(UseGear(other, Extinguish) != SPELL_CAST_OK && UseGear(other, GearAura) != SPELL_CAST_OK && fire.IsVisible(),
                "Gear without the quest still affected the world");
            SpellCastResult gearResult = UseGear(owner, GearAura);
            Tick(owner, 1800);
            if (gearResult != SPELL_CAST_OK)
                std::cout << "CASE accepted gear result=" << gearResult << " status=" << owner.GetQuestStatus(FireQuest) << '\n';
            Require(gearResult == SPELL_CAST_OK && owner.HasAura(GearAura), "The real accepted gear aura could not be applied");
            Position facingAway{facing.GetPositionX(), facing.GetPositionY(), facing.GetPositionZ(), float(M_PI)};
            SupplyMove(owner, facingAway);
            Require(UseGear(owner, Extinguish) == SPELL_CAST_OK, "The cone should still cast into an empty direction");
            Tick(owner, 1900);
            Require(!owner.GetQuestObjectiveData(FireQuest, BurningBomber), "A fire behind the caster was extinguished");
            SupplyMove(owner, facing);
            Actor unrelated(&map, Commander, 65501, location);
            Require(map.AddToMap(static_cast<Creature*>(&unrelated)), "Could not observe an unrelated target inside the same cone");
            auto observer = new ConeObserver(&unrelated);
            Require(unrelated.AIM_Initialize(observer), "Could not install the target observer");
            Require(UseGear(owner, Extinguish) == SPELL_CAST_OK, "The native cone failed on the correctly faced target");
            Tick(owner, 1800);
            Require(!observer->Hit && owner.GetQuestObjectiveData(FireQuest, BurningBomber) == 1,
                "The cone hit an unrelated NPC or missed the actual fire");
        }
        if (index == 6)
        {
            Require(owner.GetQuestStatus(FireQuest) == QUEST_STATUS_COMPLETE && !owner.HasAura(GearAura) &&
                owner.GetItemCount(Gear) == 1 && UseGear(owner, Extinguish) != SPELL_CAST_OK,
                "Six fires must clear the aura, retain the gear for turn-in and stop further extinguishing");
            break;
        }
        SpellCastResult result = UseGear(owner, Extinguish);
        if (result != SPELL_CAST_OK)
            std::cout << "CASE extinguisher result=" << result << " fire=" << index << " zone=" << owner.GetZoneId() << '\n';
        Require(result == SPELL_CAST_OK, "The airfield item's native cast failed");
        Tick(owner, 1800);
        Require(owner.GetQuestObjectiveData(FireQuest, BurningBomber) == index + 1 &&
            !other.GetQuestObjectiveData(FireQuest, BurningBomber), "The real cone missed its intended target or credited a bystander");
        fire.AI()->UpdateAI(600);
        Require(!fire.IsVisible(), "The credited fire did not follow its native delayed removal");
    }

    Require(owner.CanRewardQuest(quest, false), "Six fires did not permit the real turn-in");
    SupplyMove(owner, receiverPoint);
    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = receiver.GetGUID();
    packet.QuestID = FireQuest;
    packet.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(packet);
    Require(owner.GetQuestRewardStatus(FireQuest) && !owner.GetItemCount(Gear) && !owner.HasAura(GearAura),
        "The native reward did not remove the firefighting gear and aura");

    other.DestroyItemCount(Gear, 1, true);
    other.AddQuestAndCheckCompletion(quest, &receiver);
    Require(UseGear(other, GearAura) == SPELL_CAST_OK, "The second character could not use the quest gear");
    Tick(other, 1800);
    Require(other.HasAura(GearAura), "The second character did not receive the gear aura");
    auto remoteFire = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=42046 AND map=0 AND zoneId=40 ORDER BY guid LIMIT 1");
    Require(bool(remoteFire), "The reused marker no longer has an out-of-area placement");
    SupplyMove(other, point(remoteFire->Fetch()));
    other.GetZoneAndAreaId(other.m_zoneId, other.m_areaId);
    other.UpdateAreaDependentAuras(other.GetCurrentAreaID());
    Require(other.GetCurrentAreaID() != Airfield && !other.HasAura(GearAura) &&
        UseGear(other, Extinguish) == SPELL_FAILED_INCORRECT_AREA, "The native airfield limit or aura removal on departure was lost");
    SupplyMove(other, receiverPoint);
    other.GetZoneAndAreaId(other.m_zoneId, other.m_areaId);
    Require(UseGear(other, GearAura) == SPELL_CAST_OK, "The airfield gear should work again after a legitimate return");
    Tick(other, 1800);
    Require(other.HasAura(GearAura), "Returning with an active quest did not restore manual gear use");
    AbandonSupply(other, FireQuest);
    Require(!other.GetItemCount(Gear) && !other.HasAura(GearAura), "Abandon left the gear or indefinite aura");
    std::cout << "PASS native cone at six real fires, one credit per fire, item and aura guards, reward and abandon cleanup\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The fire test requires the isolated character schema");
        auto quest = LoadFireQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckFirefighting(quest);
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
