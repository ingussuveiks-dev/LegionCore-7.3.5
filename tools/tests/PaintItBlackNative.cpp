#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GilneasQuestNative.h"
#include "SmartScriptMgr.h"

namespace
{
constexpr uint32 QuestId = 26342;
constexpr uint32 Prerequisite = 26339;
constexpr uint32 Paintinator = 58203;
constexpr uint32 BlindSentry = 79781;
constexpr uint32 Sentry = 42291;
constexpr uint32 BlindedCredit = 42796;
constexpr uint32 JarviSpawn = 84470;
constexpr uint32 SentrySpawns[] = {84276, 84277, 84282, 84283, 84374, 84375};

struct IceflowPlacement : Creature
{
    IceflowPlacement(Map& map, uint32 spawn) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "The actual Iceflow Lake placement is missing");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true) && IsInWorld(), "An actual Iceflow Lake actor did not load into the map");
    }

    ~IceflowPlacement()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void LoadPaintItBlackInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativePaintItBlack.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "The native Paintinator quest or prerequisite is absent");
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
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=26342");
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=26342");
    Require(addon && objective && objective->GetRowCount() == 1, "Required Paintinator objective or quest prerequisite is missing");
    auto quest = quests[QuestId];
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(quest->PrevQuestID);
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    Require(quest->PrevQuestID == Prerequisite && quest->SourceItemId == Paintinator && quest->GetObjectives()[0].ObjectID == BlindedCredit &&
        quest->GetObjectives()[0].Amount == 5, "The authored Paintinator quest objective or source item changed");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    auto paintinator = sObjectMgr->GetItemTemplate(Paintinator);
    Require(paintinator && std::any_of(paintinator->Effects.begin(), paintinator->Effects.end(), [](ItemEffectEntry const* effect)
        { return effect && effect->SpellID == BlindSentry && !effect->Charges; }), "The real reusable Paintinator changed");
    auto spell = sSpellMgr->GetSpellInfo(BlindSentry);
    Require(spell && spell->GetEffect(EFFECT_0)->Effect == SPELL_EFFECT_DUMMY, "The native Paintinator effect changed");
    std::cout << "CASE native Paintinator cast time=" << spell->CalcCastTime() << " missile speed=" << spell->GetMisc()->MiscData.Speed << std::endl;
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(84470,84276,84277,84282,84283,84374,84375)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_addon AS SELECT addon.* FROM legion_world.creature_addon addon JOIN creature spawn ON spawn.guid=addon.guid");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_template_addon AS SELECT * FROM legion_world.creature_template_addon WHERE entry IN(42291,42353)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=26342");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=26342");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 AND entryorguid=42291");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadCreatureTemplateAddons();
    sObjectMgr->LoadCreatureAddons();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSmartScriptMgr->LoadSmartAIFromDB();
}

void AcceptPaint(LootPlayer& owner, Creature& jarvi)
{
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_GNOME);
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_MAGE);
    owner.setFactionForRace(RACE_GNOME);
    owner.InitDisplayIds();
    owner.GetReputationMgr().LoadFromDB({});
    owner.SetLevel(5);
    owner.SetRewardedQuest(Prerequisite);
    owner.SetFloatValue(UNIT_FIELD_MOD_TIME_RATE, 1.0f);
    owner.SetFloatValue(UNIT_FIELD_MOD_CASTING_SPEED, 1.0f);
    owner.SetFloatValue(UNIT_FIELD_MOD_SPELL_HASTE, 1.0f);
    owner.SetFloatValue(UNIT_FIELD_MOD_HASTE, 1.0f);
    owner.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    MoveGilneasPlayer(owner, jarvi.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestID = QuestId;
    accept.QuestGiverGUID = jarvi.GetGUID();
    owner.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(owner.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && owner.GetItemCount(Paintinator) == 1,
        "The actual Jarvi did not offer the native Paintinator quest");
}

Position SentryApproach(Creature& sentry, float distance = 3.0f)
{
    Position point = sentry.GetNearPosition(distance, 0.0f);
    point.SetOrientation(point.GetAngle(sentry.GetPositionX(), sentry.GetPositionY()));
    return point;
}

#ifdef PAINT_IT_BLACK_BASELINE
void CheckPaintItBlack()
{
    sObjectMgr->RemoveCreatureFromGrid(JarviSpawn, sObjectMgr->GetCreatureData(JarviSpawn));
    for (uint32 spawn : SentrySpawns)
        sObjectMgr->RemoveCreatureFromGrid(spawn, sObjectMgr->GetCreatureData(spawn));
    Map map(0, 621, 0, DIFFICULTY_NONE);
    IceflowPlacement jarvi(map, JarviSpawn), sentry(map, SentrySpawns[0]), second(map, SentrySpawns[1]), tapped(map, SentrySpawns[2]);
    LootPlayer owner(SupplySession(), map, 2634201, jarvi.GetPosition()), former(SupplySession(), map, 2634202, jarvi.GetPosition()),
        attacker(SupplySession(), map, 2634203, jarvi.GetPosition());
    AcceptPaint(owner, jarvi);
    AcceptPaint(former, jarvi);
    AbandonSupply(former, QuestId);
    Require(!former.GetItemCount(Paintinator), "The original abandonment retained the Paintinator");
    MoveGilneasPlayer(owner, SentryApproach(sentry));
    Require(sentry.GetEntry() == Sentry && sentry.GetReactState() != REACT_PASSIVE, "The original sentry is absent or already disabled");
    auto cast = owner.CastSpell(&sentry, BlindSentry, false);
    Tick(owner, 3000);
    auto credit = owner.GetQuestObjectiveData(QuestId, BlindedCredit);
    std::cout << "CASE itemless native spell cast=" << uint32(cast) << " disabled=" << (sentry.GetReactState() == REACT_PASSIVE) << " credit=" << credit << std::endl;
    Require(cast == SPELL_CAST_OK && sentry.GetReactState() == REACT_PASSIVE && credit == 1,
        "Could not reproduce a direct itemless spell disabling the real sentry");

    GiveSupply(former, Paintinator, 1);
    MoveGilneasPlayer(former, SentryApproach(second));
    cast = former.CastSpell(&second, BlindSentry, false, former.GetItemByEntry(Paintinator));
    Tick(former, 3000);
    std::cout << "CASE stale Paintinator cast=" << uint32(cast) << " disabled=" << (second.GetReactState() == REACT_PASSIVE) << std::endl;
    Require(cast == SPELL_CAST_OK && second.GetReactState() == REACT_PASSIVE, "Could not reproduce a stale Paintinator disabling a sentry");

    AcceptPaint(attacker, jarvi);
    MoveGilneasPlayer(attacker, SentryApproach(tapped));
    MoveGilneasPlayer(owner, SentryApproach(tapped));
    attacker.DealDamage(&tapped, 1, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
    Require(tapped.GetLootRecipient() == &attacker && tapped.Attack(&attacker, true), "An actual initial attacker could not claim the native combat tap");
    cast = owner.CastSpell(&tapped, BlindSentry, false, owner.GetItemByEntry(Paintinator));
    Tick(owner, 3000);
    std::cout << "CASE original tap credit=" << attacker.GetQuestObjectiveData(QuestId, BlindedCredit)
        << " painter credit=" << owner.GetQuestObjectiveData(QuestId, BlindedCredit) << " attacking=" << bool(tapped.getVictim()) << std::endl;
    Require(cast == SPELL_CAST_OK && attacker.GetQuestObjectiveData(QuestId, BlindedCredit) == 1 &&
        owner.GetQuestObjectiveData(QuestId, BlindedCredit) == 1 && tapped.getVictim() == &attacker,
        "Could not reproduce diversion to the combat tap and the disabled sentry still attacking");
    std::cout << "EXPECTED_BROKEN direct and stale Paintinator spells shut down the authored sentries\n";
}
#else
struct SentryGroup : Group
{
    SentryGroup(LootPlayer& first, LootPlayer& second) : members{&first, &second}
    {
        this->*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(first.GetGUID().GetCounter(), 0);
        for (auto player : members)
        {
            Group::MemberSlot slot;
            slot.Guid = player->GetGUID();
            slot.Class = player->getClass();
            const_cast<Group::MemberSlotList&>(GetMemberSlots()).push_back(slot);
            player->SetGroup(this, 0);
        }
    }

    ~SentryGroup()
    {
        for (auto player : members)
            player->SetGroup(nullptr);
    }

    LootPlayer* const members[2];
};

SpellCastResult UsePaint(LootPlayer& owner, Creature& target)
{
    return owner.CastSpell(&target, BlindSentry, false, owner.GetItemByEntry(Paintinator));
}

void CheckPaintItBlack()
{
    sObjectMgr->RemoveCreatureFromGrid(JarviSpawn, sObjectMgr->GetCreatureData(JarviSpawn));
    for (uint32 spawn : SentrySpawns)
        sObjectMgr->RemoveCreatureFromGrid(spawn, sObjectMgr->GetCreatureData(spawn));
    Map map(0, 621, 0, DIFFICULTY_NONE);
    IceflowPlacement jarvi(map, JarviSpawn);
    std::vector<std::unique_ptr<IceflowPlacement>> sentries;
    for (uint32 spawn : SentrySpawns)
        sentries.push_back(std::make_unique<IceflowPlacement>(map, spawn));
    LootPlayer owner(SupplySession(), map, 2634201, jarvi.GetPosition()), former(SupplySession(), map, 2634202, jarvi.GetPosition()),
        peer(SupplySession(), map, 2634203, jarvi.GetPosition());
    for (auto player : {&owner, &former, &peer})
        AcceptPaint(*player, jarvi);
    Require(UsePaint(owner, jarvi) == SPELL_FAILED_BAD_TARGETS, "The actual Paintinator targeted the unrelated Jarvi");
    AbandonSupply(former, QuestId);
    Require(!former.GetItemCount(Paintinator), "The native abandon packet retained the supplied Paintinator");
    GiveSupply(former, Paintinator, 1);
    auto& first = *sentries.front();
    auto point = SentryApproach(first);
    for (auto player : {&owner, &peer, &former})
        MoveGilneasPlayer(*player, point);
    Require(owner.CastSpell(&first, BlindSentry, false) == SPELL_FAILED_BAD_TARGETS &&
        UsePaint(former, first) == SPELL_FAILED_BAD_TARGETS && first.GetReactState() != REACT_PASSIVE,
        "An itemless or stale paint spell disabled the shared sentry");
    former.DestroyItemCount(Paintinator, 1, true);
    AcceptPaint(former, jarvi);
    MoveGilneasPlayer(former, point);
    Require(UsePaint(former, first) == SPELL_CAST_OK, "The native 1.5-second Paintinator cast did not start");
    Tick(former, 700);
    AbandonSupply(former, QuestId);
    AcceptPaint(former, jarvi);
    MoveGilneasPlayer(former, point);
    Tick(former, 3000);
    Require(first.GetReactState() != REACT_PASSIVE && !former.GetQuestObjectiveData(QuestId, BlindedCredit),
        "A Paintinator cast completed from a previous accepted attempt");
    auto damage = former.DealDamage(&first, 1, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
    first.Attack(&former, true);
    std::cout << "CASE native tap damage=" << damage << " tap=" << (first.GetLootRecipient() == &former)
        << " attacking=" << (first.getVictim() == &former) << std::endl;
    Require(first.GetLootRecipient() == &former && first.getVictim() == &former, "Could not tag a real sentry using ordinary combat");

    {
        SentryGroup group(owner, peer);
        Require(UsePaint(owner, first) == SPELL_CAST_OK, "The genuine Paintinator cast did not start");
        Tick(owner, 3000);
        std::cout << "CASE real sentry credit " << owner.GetQuestObjectiveData(QuestId, BlindedCredit) << ','
            << peer.GetQuestObjectiveData(QuestId, BlindedCredit) << ',' << former.GetQuestObjectiveData(QuestId, BlindedCredit) << std::endl;
        Require(first.IsAlive() && first.GetReactState() == REACT_PASSIVE && !first.getVictim() &&
            owner.GetQuestObjectiveData(QuestId, BlindedCredit) == 1 && peer.GetQuestObjectiveData(QuestId, BlindedCredit) == 1 &&
            !former.GetQuestObjectiveData(QuestId, BlindedCredit), "Original shutdown or eligible group credit did not run once");
        Require(UsePaint(former, first) == SPELL_FAILED_BAD_TARGETS, "An already disabled sentry could be targeted again");
        first.AI()->UpdateAI(3002);
        Require(!first.IsVisible(), "The original timed sentry shutdown did not hide the disabled robot");
    }
    for (std::size_t index = 1; index < 5; ++index)
    {
        auto& sentry = *sentries[index];
        MoveGilneasPlayer(owner, SentryApproach(sentry));
        double cooldown = owner.GetSpellCooldownDelay(BlindSentry);
        if (cooldown > 0.0)
            std::this_thread::sleep_for(std::chrono::milliseconds(uint32(cooldown * 1000) + 60));
        Require(UsePaint(owner, sentry) == SPELL_CAST_OK, "A live authored sentry rejected the reusable Paintinator");
        Tick(owner, 3000);
        Require(sentry.GetReactState() == REACT_PASSIVE &&
            owner.GetQuestObjectiveData(QuestId, BlindedCredit) == int32(index + 1) && owner.GetItemCount(Paintinator) == 1,
            "The original sentry failed to shut down or grant one credit");
    }
    auto& extra = *sentries.back();
    MoveGilneasPlayer(owner, SentryApproach(extra));
    Require(owner.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE && UsePaint(owner, extra) == SPELL_FAILED_BAD_TARGETS &&
        extra.GetReactState() != REACT_PASSIVE, "The Paintinator still disabled sentries after the required five");

    Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId);
    MoveGilneasPlayer(owner, jarvi.GetNearPosition(2.0f, 0.0f));
    Require(owner.CanRewardQuest(quest, 0, false), "Jarvi's native reward is not available");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = QuestId;
    reward.QuestGiverGUID = jarvi.GetGUID();
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(QuestId) && !owner.GetItemCount(Paintinator), "Jarvi did not remove the Paintinator on reward");
    std::cout << "PASS carried Paintinator, five real sentry shutdowns, original group credit and Jarvi reward\n";
}
#endif
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
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "No Sentrys requires isolated characters");
        LoadPaintItBlackInputs();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckPaintItBlack();
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
