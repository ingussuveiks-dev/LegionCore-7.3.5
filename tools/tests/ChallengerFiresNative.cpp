#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameObjectPackets.h"
#include "GilneasQuestNative.h"

namespace
{
constexpr uint32 Trial = 29664;
constexpr uint32 Previous = 29420;
constexpr uint32 Unlit = 75008;
constexpr uint32 Lit = 75000;

Quest const* LoadFires()
{
    auto rows = WorldDatabase.Query(
#include "NativeChallengerFires.inc"
    );
    Require(rows && rows->GetRowCount() == 3, "Missing the original challenge and its adjacent quests");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto quest = quests[Trial];
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29664 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 4, "Missing original brazier objectives");
    for (uint32 entry : {56048, 56037, 56038, 56040})
    {
        auto row = objectives->Fetch();
        Require(row[4].GetUInt32() == entry && row[5].GetUInt32() == 1, "An original brazier objective changed");
        quest->LoadQuestObjective(row);
        objectives->NextRow();
    }
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=29664");
    Require(addon && addon->Fetch()[4].GetUInt32() == Previous && addon->Fetch()[5].GetUInt32() == 29421 && quest->SourceItemId == Unlit,
        "The original Li Fei quest route changed");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(Previous);
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sObjectMgr->LoadEquipmentTemplates();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid=244088");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=29664");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=29664");
#ifndef CHALLENGER_FIRES_BASELINE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area "
        "WHERE spell IN (105156,105157,105158,119305,119306,119307)");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
#ifndef CHALLENGER_FIRES_BASELINE
    sSpellMgr->LoadSpellAreas();
#endif
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    return quest;
}

struct TrialBrazier : GameObject
{
    TrialBrazier(Map& map, uint32 spawn, uint32 entry, uint32 spell)
    {
        auto row = WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
            "FROM gameobject WHERE guid=%u AND id=%u AND map=860 AND areaId=5849", spawn, entry);
        Require(bool(row), "The brazier placement is missing");
        auto f = row->Fetch();
        Position point{f[0].GetFloat(), f[1].GetFloat(), f[2].GetFloat(), f[3].GetFloat()};
        G3D::Quat rotation{f[4].GetFloat(), f[5].GetFloat(), f[6].GetFloat(), f[7].GetFloat()};
        m_DBTableGuid = spawn;
        map.LoadGrid(point.GetPositionX(), point.GetPositionY());
        Require(Create(spawn, entry, &map, 1, point, rotation, 255, GO_STATE_READY) && map.AddToMap(static_cast<GameObject*>(this)),
            "Could not load the actual brazier");
        Require(GetGoType() == GAMEOBJECT_TYPE_SPELLCASTER && GetGOInfo()->spellCaster.spell == spell,
            "A real brazier no longer uses its original spell");
    }

    ~TrialBrazier()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

void ClickBrazier(LootPlayer& player, TrialBrazier& brazier)
{
    WorldPacket wire(CMSG_GAME_OBJ_USE);
    wire << brazier.GetGUID();
    WorldPackets::GameObject::GameObjectUse packet(std::move(wire));
    packet.Read();
    player.GetSession()->HandleGameObjectUse(packet);
    Tick(player, 500);
}

struct ChallengerTeacher : Creature
{
    explicit ChallengerTeacher(Map& map) : Creature(false)
    {
        Require(LoadCreatureFromDB(244088, &map, true), "Could not load Master Li Fei");
    }

    ~ChallengerTeacher()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void CheckFires(Quest const* quest, bool baseline)
{
    auto data = sObjectMgr->GetCreatureData(244088);
    Require(data, "Master Li Fei's actual placement is missing");
    sObjectMgr->RemoveCreatureFromGrid(244088, data);
    Map map(860, 637, 0, DIFFICULTY_NONE);
    TrialBrazier source(map, 102395, 209369, 105151), red(map, 102400, 209801, 105156),
        blue(map, 102398, 209802, 105157), violet(map, 102399, 209803, 105158);
    ChallengerTeacher master(map);
    LootPlayer owner(SupplySession(), map, 2966401, master.GetPosition()), outsider(SupplySession(), map, 2966402, source.GetPosition());
    for (auto player : {&owner, &outsider})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_PANDAREN_NEUTRAL);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_MONK);
        player->setFactionForRace(RACE_PANDAREN_NEUTRAL);
        player->InitDisplayIds();
        player->GetReputationMgr().LoadFromDB({});
        player->SetLevel(8);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 10000);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE})
            player->SetFloatValue(field, 1.0f);
    }
    auto accept = [&]
    {
        MoveGilneasPlayer(owner, master.GetPosition());
        WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
        packet.QuestGiverGUID = master.GetGUID();
        packet.QuestID = Trial;
        owner.GetSession()->HandleQuestGiverAcceptQuest(packet);
    };
    accept();
    Require(owner.GetQuestStatus(Trial) == QUEST_STATUS_NONE, "The quest accepted without the preceding trial");
    owner.SetRewardedQuest(Previous);
    accept();
    Require(owner.GetQuestStatus(Trial) == QUEST_STATUS_INCOMPLETE && owner.GetItemCount(Unlit) == 1,
        "Li Fei did not provide the actual unlit torch");

    MoveGilneasPlayer(owner, red.GetPosition());
    ClickBrazier(owner, red);
    uint32 premature = owner.GetQuestObjectiveData(Trial, 56037);
    ClickBrazier(outsider, source);
    uint32 freeTorch = outsider.GetItemCount(Lit);
    std::cout << "CASE premature red=" << premature << " free torch=" << freeTorch << '\n';
    if (baseline)
    {
        Require(premature == 1, "The original unguarded brazier did not record premature credit");
        std::cout << "EXPECTED_BROKEN colored fire credited without a torch\n";
    }
    else
    {
        Require(!premature && !freeTorch, "An unlit torch or unrelated player used a quest brazier");
        ClickBrazier(owner, source);
        Require(!owner.GetItemCount(Lit), "A distant click made a lit torch");
        MoveGilneasPlayer(owner, source.GetPosition());
        owner.DestroyItemCount(Unlit, 1, true);
        ClickBrazier(owner, source);
        Require(!owner.GetItemCount(Lit) && !owner.GetQuestObjectiveData(Trial, 56048), "A first-time attempt lit a missing torch");
        AbandonSupply(owner, Trial);
        accept();
        MoveGilneasPlayer(owner, source.GetPosition());
        ClickBrazier(owner, source);
        ClickBrazier(owner, source);
        Require(!owner.GetItemCount(Unlit) && owner.GetItemCount(Lit) == 1 && owner.GetQuestObjectiveData(Trial, 56048) == 1,
            "The source did not exchange the unlit torch for one lit torch and one receipt");
        MoveGilneasPlayer(owner, blue.GetPosition());
        ClickBrazier(owner, blue);
        Require(owner.HasAura(105157), "The actual blue fire did not enable its visual state");
        owner.DestroyItemCount(Lit, 1, true);
        MoveGilneasPlayer(owner, red.GetPosition());
        ClickBrazier(owner, red);
        Require(!owner.GetQuestObjectiveData(Trial, 56037), "A player with a lost torch lit a colored fire");
        MoveGilneasPlayer(owner, source.GetPosition());
        ClickBrazier(owner, source);
        Require(!owner.GetItemCount(Lit), "A previous objective allowed a new torch without its unlit reagent");
        AbandonSupply(owner, Trial);
        Require(!owner.HasAura(105157), "Abandon left a permanent visual aura from an old attempt");
        accept();
        Require(owner.GetItemCount(Unlit) == 1 && !owner.GetQuestObjectiveData(Trial, 56048),
            "Reacceptance did not restore the torch and clear the prior attempt's receipt");
        MoveGilneasPlayer(owner, source.GetPosition());
        ClickBrazier(owner, source);
        Require(owner.GetItemCount(Lit) == 1, "The actual source could not light the new attempt's torch");
        for (auto brazier : {&blue, &red, &violet})
        {
            MoveGilneasPlayer(owner, brazier->GetPosition());
            ClickBrazier(owner, *brazier);
            ClickBrazier(owner, *brazier);
        }
        Require(owner.GetQuestStatus(Trial) == QUEST_STATUS_COMPLETE && owner.CanRewardQuest(quest, false),
            "The correct three brazier spells did not make the quest rewardable");
        MoveGilneasPlayer(owner, master.GetPosition());
        WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
        reward.QuestID = Trial;
        reward.QuestGiverGUID = master.GetGUID();
        owner.GetSession()->HandleQuestGiverChooseReward(reward);
        Require(owner.GetQuestRewardStatus(Trial) && !owner.GetItemCount(Unlit) && !owner.GetItemCount(Lit),
            "Li Fei's turn-in did not clean up the two torch forms");
        Require(owner.HasAura(105156) && owner.HasAura(105157) && owner.HasAura(105158),
            "Li Fei's turn-in hid the three genuine flames before the next cave quest");
        owner.AddQuest(sQuestDataStore->GetQuestTemplate(29422), &master);
        owner.SetQuestUpdate(29422);
        Tick(owner, 500);
        for (uint32 spell : {105156, 105157, 105158})
            Require(!owner.HasAura(spell), "An old brazier aura remained after the Huo transition");
        for (uint32 spell : {119305, 119306, 119307})
            Require(owner.HasAura(spell), "The next Huo quest did not retain a brazier's visual state");
        std::cout << "PASS native torch exchange, three placed flames, abandon/retry and Master Li Fei turn-in\n";
    }
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{860, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{860, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The brazier check requires isolated characters");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
#ifdef CHALLENGER_FIRES_BASELINE
        CheckFires(LoadFires(), true);
#else
        CheckFires(LoadFires(), false);
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
