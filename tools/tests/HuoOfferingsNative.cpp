#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GilneasQuestNative.h"
#include "ScriptsData.h"

void AddSC_WanderingIsland();

namespace
{
constexpr uint32 FireQuest = 29422;
constexpr uint32 Trial = 29421;
constexpr uint32 Offerings = 72583;
constexpr uint32 FanFlames = 102522;
constexpr uint32 HuoCredit = 54787;
constexpr uint32 TeacherSpawn = 244088;
constexpr uint32 HuoSpawn = 244099;

Quest const* LoadFireQuest(bool baseline)
{
    auto rows = WorldDatabase.Query(
#include "NativeHuoOfferings.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "Missing Huo and its actual prerequisite");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto quest = quests[FireQuest];
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29422");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == HuoCredit &&
        objective->Fetch()[5].GetUInt32() == 1 && quest->SourceItemId == Offerings, "The actual Huo offering objective changed");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=29422");
    Require(addon && addon->Fetch()[4].GetUInt32() == Trial && addon->Fetch()[5].GetUInt32() == 29423, "The original Huo quest route changed");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(Trial);
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadEquipmentTemplates();
    auto item = sObjectMgr->GetItemTemplate(Offerings);
    Require(item && std::any_of(item->Effects.begin(), item->Effects.end(), [](ItemEffectEntry const* e)
        { return e && e->SpellID == FanFlames && e->CoolDownMSec == 15000; }), "Huo's actual 15 second item cooldown changed");
    auto cast = sSpellMgr->GetSpellInfo(FanFlames);
    Require(cast && cast->GetMaxDuration() == 12000 && cast->GetEffect(EFFECT_0)->ApplyAuraPeriod == 750,
        "The client Huo sequence is not the 12 second offering aura");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(244088,244099)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=29422");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=29422");
    if (!baseline)
    {
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id=102522");
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions "
            "WHERE SourceTypeOrReferenceId=13 AND SourceEntry IN(102522,109090,109105)");
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE spell=102522");
    }
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    if (!baseline)
    {
        sScriptDataStore->LoadSpellScriptNames();
        sConditionMgr->LoadConditions();
        sSpellMgr->LoadSpellAreas();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_WanderingIsland();
        sScriptMgr->SwapScriptContext(true);
    }
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    return quest;
}

struct OfferingActor : Creature
{
    OfferingActor(Map& map, uint32 spawn) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "The actual offering actor placement is missing");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true), "Could not load the actual offering actor");
    }

    ~OfferingActor()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void CheckOfferings(Quest const* quest, bool baseline)
{
    for (uint32 spawn : {TeacherSpawn, HuoSpawn})
        sObjectMgr->RemoveCreatureFromGrid(spawn, sObjectMgr->GetCreatureData(spawn));
    Map map(860, 642, 0, DIFFICULTY_NONE);
    OfferingActor master(map, TeacherSpawn), huo(map, HuoSpawn);
    LootPlayer owner(SupplySession(), map, 2942201, master.GetPosition());
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_PANDAREN_NEUTRAL);
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_MONK);
    owner.setFactionForRace(RACE_PANDAREN_NEUTRAL);
    owner.InitDisplayIds();
    owner.GetReputationMgr().LoadFromDB({});
    owner.SetLevel(8);
    owner.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 10000);
    for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE})
        owner.SetFloatValue(field, 1.0f);
    auto accept = [&]
    {
        MoveGilneasPlayer(owner, master.GetPosition());
        WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
        packet.QuestGiverGUID = master.GetGUID();
        packet.QuestID = FireQuest;
        owner.GetSession()->HandleQuestGiverAcceptQuest(packet);
    };
    auto use = [&] { return owner.CastSpell(&owner, FanFlames, false, owner.GetItemByEntry(Offerings)); };
    owner.SetRewardedQuest(Trial);
    accept();
    Require(owner.GetItemCount(Offerings) == 1, "Master Li Fei did not provide Huo's Offerings");
    auto remote = use();
    Tick(owner, 14000);
    if (baseline)
    {
        Require(remote == SPELL_CAST_OK && owner.GetQuestStatus(FireQuest) == QUEST_STATUS_COMPLETE,
            "The original offering cast did not award remote credit as expected");
        std::cout << "EXPECTED_BROKEN the offering aura completed beside Li Fei without Huo\n";
        return;
    }
    Require(remote != SPELL_CAST_OK && owner.GetQuestStatus(FireQuest) == QUEST_STATUS_INCOMPLETE,
        "The offering completed without its real nearby spirit");
    MoveGilneasPlayer(owner, huo.GetPosition());
    Require(owner.CastSpell(&owner, FanFlames, false) != SPELL_CAST_OK, "The offering spell could run without the item cast");
    Require(use() == SPELL_CAST_OK, "The actual item could not start beside Huo");
    Tick(owner, 4000);
    auto aura = owner.GetAura(FanFlames);
    Require(aura && !aura->CanBeSaved() && !owner.GetQuestObjectiveData(FireQuest, HuoCredit),
        "Offering credited early or could persist across logout");
    AbandonSupply(owner, FireQuest);
    accept();
    Tick(owner, 12000);
    Require(!owner.HasAura(FanFlames) && !owner.GetQuestObjectiveData(FireQuest, HuoCredit),
        "A canceled offering finished in the next quest attempt");

    owner.RemoveSpellCooldown(FanFlames, true);
    MoveGilneasPlayer(owner, huo.GetPosition());
    Require(use() == SPELL_CAST_OK, "The new offering attempt could not start");
    Tick(owner, 3000);
    MoveGilneasPlayer(owner, master.GetPosition());
    Tick(owner, 12000);
    Require(!owner.GetQuestObjectiveData(FireQuest, HuoCredit), "Leaving Huo still awarded the former offering");

    owner.RemoveSpellCooldown(FanFlames, true);
    MoveGilneasPlayer(owner, huo.GetPosition());
    Require(use() == SPELL_CAST_OK, "The offering could not restart beside Huo");
    Tick(owner, 13000);
    Require(owner.CanRewardQuest(quest, false) && owner.GetQuestObjectiveData(FireQuest, HuoCredit) == 1,
        "The completed offering did not award one native spirit objective");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = FireQuest;
    reward.QuestGiverGUID = huo.GetGUID();
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(FireQuest) && !owner.GetItemCount(Offerings), "The genuine Huo turn-in did not clean up the offerings");
    std::cout << "PASS genuine Huo, twelve second offering, cancellation, movement, retry and native turn-in\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The offering check requires isolated characters");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
#ifdef HUO_OFFERINGS_BASELINE
        CheckOfferings(LoadFireQuest(true), true);
#else
        CheckOfferings(LoadFireQuest(false), false);
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
