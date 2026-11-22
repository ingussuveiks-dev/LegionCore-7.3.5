#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "SmartAI.h"
#include "SmartScriptMgr.h"
#include "TemporarySummon.h"
#include "CreatureTextMgr.h"

void AddSC_SmartScripts();

namespace
{
constexpr uint32 GuideQuest = 13988;
constexpr uint32 Totem = 46782;
constexpr uint32 SummonEagle = 65751;
constexpr uint32 Eagle = 34615;
constexpr uint32 Spirit = 34623;
constexpr uint32 SummitCredit = 34631;
constexpr uint32 Jerrik = 34626;

void ShadowGuideRows(char const* name, char const* filter)
{
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE tonga_%s AS SELECT * FROM %s WHERE %s",
        name, name, filter).c_str());
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE %s AS SELECT * FROM tonga_%s", name, name).c_str());
}

Quest const* LoadGuideQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeTongaGuide.inc"
    );
    Require(bool(row), "Missing A Growing Problem");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[GuideQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(GuideQuest, quest);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=13988 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 2 && objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objectives->Fetch()[4].GetUInt32() == SummitCredit && objectives->Fetch()[5].GetUInt32() == 1,
        "Expected the original summit credit");
    quest->LoadQuestObjective(objectives->Fetch());
    objectives->NextRow();
    Require(objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM && objectives->Fetch()[4].GetUInt32() == Totem &&
        objectives->Fetch()[5].GetUInt32() == 1, "Expected return of the quest totem");
    quest->LoadQuestObjective(objectives->Fetch());
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=13988");
    Require(bool(addon), "Missing A Growing Problem quest addon");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST | QUEST_SPECIAL_FLAGS_DELIVER);
    Require(quest->SourceItemId == Totem && quest->SourceItemIdCount == 1, "Expected one provided totem");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_SmartScripts();
    sScriptMgr->SwapScriptContext(true);

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    ShadowGuideRows("smart_scripts", "(source_type=1 AND entryorguid=195146) OR (source_type=0 AND entryorguid=34623)");
#ifdef TONGA_GUIDE_BASELINE
    // The original position cast dereferences the nonexistent creature on
    // this GO. Isolate the other native fault, the shared credit radius.
    WorldDatabase.DirectExecute("DELETE FROM smart_scripts WHERE source_type=1 AND entryorguid=195146 AND id=1 AND action_param1=65763");
#endif
    ShadowGuideRows("creature_questender", "quest=13988");
    ShadowGuideRows("conditions", "(SourceTypeOrReferenceId=22 AND SourceEntry=195146) OR (SourceTypeOrReferenceId=17 AND SourceEntry=65751)");
#ifndef TONGA_GUIDE_BASELINE
    ShadowGuideRows("spell_target_position", "ID=65763");
    ShadowGuideRows("creature_text", "CreatureID=34623");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sConditionMgr->LoadConditions();
#ifndef TONGA_GUIDE_BASELINE
    sSpellMgr->LoadSpellTargetPositions();
    sCreatureTextMgr->LoadCreatureTexts();
    Require(sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL, SummonEagle).size() >= 2,
        "The totem's active quest and carried item guards did not load");
#endif
    return quest;
}

struct SummitFocus : GameObject
{
    SummitFocus(Map& map, Field* row)
    {
        m_DBTableGuid = row[0].GetUInt32();
        Position point{row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat()};
        G3D::Quat rotation(row[5].GetFloat(), row[6].GetFloat(), row[7].GetFloat(), row[8].GetFloat());
        Require(Create(m_DBTableGuid, 195146, &map, row[9].GetUInt32(), point, rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)) && AI(), "The actual Thunderhead Summit focus could not load its AI");
    }

    ~SummitFocus()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

TempSummon* FindGuide(LootPlayer& player, uint32 entry)
{
    for (ObjectGuid const& guid : *player.GetSummonList(entry))
        if (Creature* creature = ObjectAccessor::GetCreature(player, guid))
            if (!creature->IsDespawn())
                return creature->ToTempSummon();
    return nullptr;
}

SpellCastResult UseTotem(LootPlayer& player)
{
    auto item = player.GetItemByEntry(Totem);
    Require(item, "Expected the real quest item");
    return player.CastSpell(&player, SummonEagle, false, item);
}

void CheckSummit(Quest const* quest)
{
    auto summit = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,phaseMask "
        "FROM gameobject WHERE id=195146 AND guid=32485 AND map=1 AND zoneId=17");
    auto jerrik = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=34626 AND guid=87500 AND map=1 AND zoneId=17");
    Require(summit && jerrik && WorldDatabase.Query("SELECT 1 FROM creature c JOIN creature_queststarter q ON q.id=c.id "
        "WHERE c.id=3448 AND q.quest=13988 AND c.map=1 AND c.zoneId=17"),
        "Missing Tonga's offer, the summit focus or Jerrik");
    Map map(1, 500, 0, DIFFICULTY_NONE);
    SummitFocus focus(map, summit->Fetch());
    auto jr = jerrik->Fetch();
    Position receiverPoint{jr[1].GetFloat(), jr[2].GetFloat(), jr[3].GetFloat(), jr[4].GetFloat()};
    Actor receiver(&map, Jerrik, jr[0].GetUInt32(), receiverPoint);
    Require(map.AddToMap(static_cast<Creature*>(&receiver)), "Could not publish the original Jerrik");
    LootPlayer owner(SupplySession(), map, 13988, receiverPoint), guest(SupplySession(), map, 13989, receiverPoint);
    for (auto player : {&owner, &guest})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_TAUREN);
        player->setFactionForRace(RACE_TAUREN);
        player->InitDisplayIds();
        player->SetLevel(15);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    guest.AddQuestAndCheckCompletion(quest, nullptr);
    SupplyMove(guest, focus.GetPosition());
    focus.AI()->UpdateAI(1300);
    Require(!guest.CanRewardQuest(quest, false), "Standing at the banner without a companion granted completion");
#ifndef TONGA_GUIDE_BASELINE
    Require(UseTotem(owner) == SPELL_CAST_OK, "The native cast did not start");
    Tick(owner, 500);
    AbandonSupply(owner, GuideQuest);
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    Tick(owner, 1800);
    Require(!FindGuide(owner, Eagle), "An interrupted quest attempt summoned an eagle for a later acceptance");
#endif
    Require(UseTotem(owner) == SPELL_CAST_OK, "The native totem rejected an accepted quest");
    Tick(owner, 1800);
    TempSummon* eagle = FindGuide(owner, Eagle);
    Require(eagle && eagle->GetSummoner() == &owner && eagle->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == SummonEagle,
        "The item did not summon Tonga's actual companion");
#ifndef TONGA_GUIDE_BASELINE
    Require(eagle->GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE &&
        eagle->GetReactState() == REACT_PASSIVE && eagle->HasQuestObjectScope() && !eagle->CanTargetQuestObject(&guest),
        "The real companion must follow its owner privately and stay out of combat");
#endif
    focus.AI()->UpdateAI(1400);
    Require(!owner.CanRewardQuest(quest, false), "Jerrik alone triggered the summit");
    SupplyMove(owner, focus.GetPosition());
    eagle->NearTeleportTo(focus.GetPosition());
    map.MoveAllCreaturesInMoveList();
    Require(eagle->IsWithinDistInMap(&focus, 10.0f), "The eagle never reached the actual focus");
    focus.AI()->UpdateAI(1500);
#ifdef TONGA_GUIDE_BASELINE
    Require(owner.CanRewardQuest(quest, false) && guest.CanRewardQuest(quest, false),
        "Expected the old shared focus to credit a stranger without their own companion");
    std::cout << "EXPECTED_BROKEN one eagle credited an unrelated adventurer at Thunderhead Summit\n";
    return;
#endif
    Require(owner.CanRewardQuest(quest, false) && !guest.CanRewardQuest(quest, false) && owner.GetItemCount(Totem) == 1,
        "Summit credit must belong to the eagle's owner and retain the totem for handoff");
    TempSummon* spirit = FindGuide(owner, Spirit);
    Require(spirit && spirit->HasQuestObjectScope() && spirit->CanTargetQuestObject(&owner) &&
        !spirit->CanTargetQuestObject(&guest), "Tonga's Spirit is missing or visible to the wrong owner");
    Require(std::abs(spirit->GetHomePosition().GetPositionX() + 1480.68f) < 0.1f &&
        std::abs(spirit->GetHomePosition().GetPositionY() + 3294.02f) < 0.1f,
        "The spirit did not use the authored destination preserved from the focus");
    focus.AI()->UpdateAI(1800);
    Require(owner.GetQuestObjectiveData(GuideQuest, SummitCredit) == 1 && FindGuide(owner, Spirit) == spirit,
        "A further update duplicated summit credit or the presentation");

    SupplyMove(owner, receiverPoint);
    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = receiver.GetGUID();
    packet.QuestID = GuideQuest;
    packet.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(packet);
    map.RemoveAllObjectsInRemoveList();
    Require(owner.GetQuestRewardStatus(GuideQuest) && !owner.GetItemCount(Totem) && !FindGuide(owner, Spirit),
        "Native turn-in did not remove the totem and presentation");

    Require(UseTotem(guest) == SPELL_CAST_OK, "The next player could not summon at the summit");
    Tick(guest, 2000);
    TempSummon* secondEagle = FindGuide(guest, Eagle);
    Require(secondEagle && secondEagle->HasQuestObjectScope(), "A second player could not summon their own scoped eagle");
    focus.AI()->UpdateAI(2000);
    Require(guest.CanRewardQuest(quest, false) && FindGuide(guest, Spirit),
        "A prior player's scene consumed the shared summit for the next player");
    AbandonSupply(guest, GuideQuest);
    map.RemoveAllObjectsInRemoveList();
    Require(!guest.GetItemCount(Totem) && !FindGuide(guest, Eagle) && !FindGuide(guest, Spirit),
        "Abandon left a companion, spirit or totem");
    GiveSupply(guest, Totem, 1);
    Require(UseTotem(guest) != SPELL_CAST_OK && !FindGuide(guest, Eagle), "A leftover totem could summon without the quest");
    std::cout << "PASS actual totem, original summit, private owner and spirit, second attempt and native inventory cleanup\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The summit test requires the isolated character schema");
        auto quest = LoadGuideQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckSummit(quest);
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
