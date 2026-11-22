#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GilneasQuestNative.h"
#include "SmartScriptMgr.h"

namespace
{
constexpr uint32 QuestId = 26333;
constexpr uint32 Prerequisite = 26329;
constexpr uint32 Grenade = 58200;
constexpr uint32 ThrowGrenade = 79751;
constexpr uint32 Tank = 42224;
constexpr uint32 HinklesSpawn = 84584;
constexpr uint32 TankSpawns[] = {84314, 84321, 84323, 84324, 84328, 84329};

struct ArsenalPlacement : Creature
{
    ArsenalPlacement(Map& map, uint32 spawn) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "The actual New Tinkertown placement is missing");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true) && IsInWorld(), "An actual New Tinkertown actor did not load into the map");
    }

    ~ArsenalPlacement()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void LoadNoTanksInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeNoTanks.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "The native grenade quest or prerequisite is absent");
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
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=26333");
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=26333");
    Require(addon && objective && objective->GetRowCount() == 1, "Required grenade objective or quest prerequisite is missing");
    auto quest = quests[QuestId];
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(quest->PrevQuestID);
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    Require(quest->PrevQuestID == Prerequisite && quest->SourceItemId == Grenade && quest->GetObjectives()[0].ObjectID == Tank &&
        quest->GetObjectives()[0].Amount == 5, "The authored grenade quest objective or source item changed");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    auto grenade = sObjectMgr->GetItemTemplate(Grenade);
    Require(grenade && std::any_of(grenade->Effects.begin(), grenade->Effects.end(), [](ItemEffectEntry const* effect)
        { return effect && effect->SpellID == ThrowGrenade && !effect->Charges; }), "The real reusable Techno-Grenade changed");
    auto spell = sSpellMgr->GetSpellInfo(ThrowGrenade);
    Require(spell && spell->GetEffect(EFFECT_0)->Effect == SPELL_EFFECT_DUMMY, "The native grenade effect changed");
    std::cout << "CASE native grenade cast time=" << spell->CalcCastTime() << " missile speed=" << spell->GetMisc()->MiscData.Speed << std::endl;
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(84584,84314,84321,84323,84324,84328,84329)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_addon AS SELECT addon.* FROM legion_world.creature_addon addon JOIN creature spawn ON spawn.guid=addon.guid");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_template_addon AS SELECT * FROM legion_world.creature_template_addon WHERE entry IN(42224,42491)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=26333");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=26333");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 AND entryorguid=42224");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadCreatureTemplateAddons();
    sObjectMgr->LoadCreatureAddons();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSmartScriptMgr->LoadSmartAIFromDB();
}

void AcceptGrenade(LootPlayer& owner, Creature& hinkles)
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
    MoveGilneasPlayer(owner, hinkles.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestID = QuestId;
    accept.QuestGiverGUID = hinkles.GetGUID();
    owner.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(owner.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && owner.GetItemCount(Grenade) == 1,
        "The actual Hinkles did not offer the native grenade quest");
}

Position TankApproach(Creature& tank, float distance = 3.0f)
{
    Position point = tank.GetNearPosition(distance, 0.0f);
    point.SetOrientation(point.GetAngle(tank.GetPositionX(), tank.GetPositionY()));
    return point;
}

#ifdef NO_TANKS_BASELINE
void CheckNoTanks()
{
    sObjectMgr->RemoveCreatureFromGrid(HinklesSpawn, sObjectMgr->GetCreatureData(HinklesSpawn));
    for (uint32 spawn : TankSpawns)
        sObjectMgr->RemoveCreatureFromGrid(spawn, sObjectMgr->GetCreatureData(spawn));
    Map map(0, 620, 0, DIFFICULTY_NONE);
    ArsenalPlacement hinkles(map, HinklesSpawn), tank(map, TankSpawns[0]), secondTank(map, TankSpawns[1]);
    LootPlayer owner(SupplySession(), map, 2633301, hinkles.GetPosition()), former(SupplySession(), map, 2633302, hinkles.GetPosition());
    AcceptGrenade(owner, hinkles);
    AcceptGrenade(former, hinkles);
    AbandonSupply(former, QuestId);
    Require(!former.GetItemCount(Grenade), "The original abandonment retained the real loaned grenade");
    MoveGilneasPlayer(owner, TankApproach(tank));
    Require(tank.GetEntry() == Tank && !owner.IsValidAttackTarget(&tank) && owner.IsWithinLOSInMap(&tank),
        "The original grenade-only tank or line of sight is missing");
    SpellCastResult result = owner.CastSpell(&tank, ThrowGrenade, false);
    Tick(owner, 2000);
    auto credit = owner.GetQuestObjectiveData(QuestId, Tank);
    std::cout << "CASE original itemless grenade cast=" << uint32(result) << " destroyed=" << !tank.IsAlive() << " credit=" << credit << std::endl;
    Require(result == SPELL_CAST_OK && !tank.IsAlive() && credit == 1, "Could not reproduce an itemless native cast destroying a real tank");

    GiveSupply(former, Grenade, 1);
    MoveGilneasPlayer(former, TankApproach(secondTank));
    auto stale = former.CastSpell(&secondTank, ThrowGrenade, false, former.GetItemByEntry(Grenade));
    Tick(former, 2000);
    std::cout << "CASE stale grenade cast=" << uint32(stale) << " destroyed=" << !secondTank.IsAlive() << std::endl;
    Require(stale == SPELL_CAST_OK && !secondTank.IsAlive(), "Could not reproduce a stale item destroying the shared tank after abandonment");
    std::cout << "EXPECTED_BROKEN itemless and stale grenade casts destroyed the original grenade-only tanks\n";
}
#else
struct GrenadeGroup : Group
{
    GrenadeGroup(LootPlayer& first, LootPlayer& second) : members{&first, &second}
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

    ~GrenadeGroup()
    {
        for (auto player : members)
            player->SetGroup(nullptr);
    }

    LootPlayer* const members[2];
};

SpellCastResult UseGrenade(LootPlayer& owner, Creature& target)
{
    return owner.CastSpell(&target, ThrowGrenade, false, owner.GetItemByEntry(Grenade));
}

void CheckNoTanks()
{
    sObjectMgr->RemoveCreatureFromGrid(HinklesSpawn, sObjectMgr->GetCreatureData(HinklesSpawn));
    for (uint32 spawn : TankSpawns)
        sObjectMgr->RemoveCreatureFromGrid(spawn, sObjectMgr->GetCreatureData(spawn));
    Map map(0, 620, 0, DIFFICULTY_NONE);
    ArsenalPlacement hinkles(map, HinklesSpawn);
    std::vector<std::unique_ptr<ArsenalPlacement>> tanks;
    for (uint32 spawn : TankSpawns)
        tanks.push_back(std::make_unique<ArsenalPlacement>(map, spawn));
    LootPlayer owner(SupplySession(), map, 2633301, hinkles.GetPosition()), former(SupplySession(), map, 2633302, hinkles.GetPosition()),
        peer(SupplySession(), map, 2633303, hinkles.GetPosition());
    for (auto player : {&owner, &former, &peer})
        AcceptGrenade(*player, hinkles);
    Require(UseGrenade(owner, hinkles) == SPELL_FAILED_BAD_TARGETS, "The actual item could target the unrelated Hinkles");
    AbandonSupply(former, QuestId);
    Require(!former.GetItemCount(Grenade), "The native abandon packet retained the supplied grenade");
    GiveSupply(former, Grenade, 1);
    auto& first = *tanks.front();
    auto point = TankApproach(first);
    for (auto player : {&owner, &peer, &former})
        MoveGilneasPlayer(*player, point);
    Require(!owner.IsValidAttackTarget(&first), "The regular attack path could kill a grenade-only tank");
    Require(owner.CastSpell(&first, ThrowGrenade, false) == SPELL_FAILED_BAD_TARGETS &&
        UseGrenade(former, first) == SPELL_FAILED_BAD_TARGETS && first.IsAlive(), "The itemless or stale grenade cast destroyed a tank");

    former.DestroyItemCount(Grenade, 1, true);
    AcceptGrenade(former, hinkles);
    MoveGilneasPlayer(former, TankApproach(first, 12.0f));
    Require(UseGrenade(former, first) == SPELL_CAST_OK && first.IsAlive(), "The native delayed grenade cast could not start normally");
    auto grenadeInFlight = former.GetCurrentSpell(CURRENT_GENERIC_SPELL);
    Require(grenadeInFlight, "The authorized grenade has no native cast event");
    Tick(former, 1100);
    Require(grenadeInFlight->getState() == SPELL_STATE_DELAYED && first.IsAlive(), "The grenade did not enter native missile flight");
    AbandonSupply(former, QuestId);
    Require(!former.GetItemCount(Grenade), "Abandonment did not remove the grenade while the missile was in flight");
    AcceptGrenade(former, hinkles);
    MoveGilneasPlayer(former, point);
    Tick(former, 5000);
    Require(first.IsAlive() && !former.GetQuestObjectiveData(QuestId, Tank), "A previous attempt's thrown grenade still hit after reaccepting");

    {
        GrenadeGroup group(owner, peer);
        Require(UseGrenade(owner, first) == SPELL_CAST_OK, "The authorized grenade cast did not start");
        Tick(owner, 5000);
        std::cout << "CASE real destroyed tank credit " << owner.GetQuestObjectiveData(QuestId, Tank) << ','
            << peer.GetQuestObjectiveData(QuestId, Tank) << ',' << former.GetQuestObjectiveData(QuestId, Tank) << std::endl;
        Require(!first.IsAlive() && owner.GetQuestObjectiveData(QuestId, Tank) == 1 && peer.GetQuestObjectiveData(QuestId, Tank) == 1 &&
            !former.GetQuestObjectiveData(QuestId, Tank), "The original linked death and eligible group credit did not run exactly once");
        Require(UseGrenade(owner, first) == SPELL_FAILED_BAD_TARGETS, "An already destroyed tank could be targeted again");
    }
    for (std::size_t index = 1; index < 5; ++index)
    {
        auto& tank = *tanks[index];
        MoveGilneasPlayer(owner, TankApproach(tank));
        double cooldown = owner.GetSpellCooldownDelay(ThrowGrenade);
        if (cooldown > 0.0)
            std::this_thread::sleep_for(std::chrono::milliseconds(uint32(cooldown * 1000) + 60));
        Require(UseGrenade(owner, tank) == SPELL_CAST_OK, "A real valid tank rejected the reusable grenade");
        Tick(owner, 5000);
        Require(!tank.IsAlive() && owner.GetQuestObjectiveData(QuestId, Tank) == int32(index + 1) && owner.GetItemCount(Grenade) == 1,
            "A valid original tank failed to die or supply one native credit");
    }
    auto& extra = *tanks.back();
    MoveGilneasPlayer(owner, TankApproach(extra));
    Require(owner.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE && UseGrenade(owner, extra) == SPELL_FAILED_BAD_TARGETS && extra.IsAlive(),
        "The grenade still destroyed tanks after the required five");

    Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId);
    uint32 choice = 0;
    if (auto package = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (auto entry : *package)
            if (owner.CanSelectQuestPackageItem(entry))
            {
                choice = entry->ItemID;
                break;
            }
    if (!choice)
        if (auto fallback = sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID); fallback && !fallback->empty())
            choice = fallback->front()->ItemID;
    MoveGilneasPlayer(owner, hinkles.GetNearPosition(2.0f, 0.0f));
    Require(choice && owner.CanRewardQuest(quest, choice, false), "The native grenade quest reward is not available");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = QuestId;
    reward.QuestGiverGUID = hinkles.GetGUID();
    reward.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(QuestId) && !owner.GetItemCount(Grenade), "Hinkles did not consume the quest item on reward");
    std::cout << "PASS real grenade scope, five original tanks, delayed abandonment, group credit and Hinkles reward\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "No Tanks requires isolated characters");
        LoadNoTanksInputs();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckNoTanks();
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
