#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

void AddSC_lost_isle();

struct PonySpellConditions
{
    using Type = ConditionContainer ConditionMgr::*;
    friend Type Access(PonySpellConditions);
};
struct PonyClickConditions
{
    using Type = CreatureSpellConditionContainer ConditionMgr::*;
    friend Type Access(PonyClickConditions);
};
struct PonySpellAreas
{
    using Type = SpellAreaMap SpellMgr::*;
    friend Type Access(PonySpellAreas);
};
struct PonyQuestAreas
{
    using Type = SpellAreaForQuestMap SpellMgr::*;
    friend Type Access(PonyQuestAreas);
};
template struct FixtureAccess<PonySpellConditions, &ConditionMgr::ConditionStore>;
template struct FixtureAccess<PonyClickConditions, &ConditionMgr::SpellClickEventConditionStore>;
template struct FixtureAccess<PonySpellAreas, &SpellMgr::mSpellAreaMap>;
template struct FixtureAccess<PonyQuestAreas, &SpellMgr::mSpellAreaForQuestMap>;

namespace
{
constexpr uint32 PonyQuest = 24864;
constexpr uint32 PonyItem = 50602;
constexpr uint32 PonyAura = 71914;
constexpr uint32 ClickSpell = 71917;
constexpr uint32 Hatchling = 38412;
constexpr uint32 HatchlingCredit = 38413;
constexpr uint32 Siren = 38360;

Quest const* LoadPony()
{
    auto row = WorldDatabase.Query(
#include "NativePoolPony.inc"
    );
    Require(bool(row), "Missing Pool Pony quest template");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[PonyQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(PonyQuest, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=24864");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == HatchlingCredit &&
        objective->Fetch()[5].GetUInt32() == 12, "Missing real 12-hatchling objective");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMinRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=24864");
    Require(bool(addon), "Missing original item provision metadata");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    Require(quest->SourceItemId == PonyItem && quest->SourceItemIdCount == 1, "Wrong supplied pony item");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadNPCSpellClickSpells();
    auto rows = WorldDatabase.Query("SELECT SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionTarget,ConditionValue1,"
        "ConditionValue2,ScriptName FROM conditions WHERE (SourceTypeOrReferenceId=17 AND SourceEntry=71914) OR "
        "(SourceTypeOrReferenceId=18 AND SourceEntry=71917 AND SourceGroup IN(38412,44578,44579,44580))");
    Require(rows && rows->GetRowCount() == 6, "Missing the pony and four hatchling conditions");
    do
    {
        auto fields = rows->Fetch();
        auto condition = new Condition;
        condition->SourceType = ConditionSourceType(fields[0].GetUInt32());
        condition->SourceGroup = fields[1].GetUInt32();
        condition->SourceEntry = fields[2].GetUInt32();
        condition->ConditionType = ConditionTypes(fields[3].GetUInt32());
        condition->ConditionTarget = fields[4].GetUInt8();
        condition->ConditionValue1 = fields[5].GetUInt32();
        condition->ConditionValue2 = fields[6].GetUInt32();
        condition->ScriptId = sObjectMgr->GetScriptId(fields[7].GetCString());
        Require((sConditionMgr->*Access(ValidateSourceTag{}))(condition) && sConditionMgr->isConditionTypeValid(condition),
            "Pool Pony condition does not pass the native validator");
        if (condition->SourceType == CONDITION_SOURCE_TYPE_SPELL)
            (sConditionMgr->*Access(PonySpellConditions{}))[condition->SourceType][PonyAura].push_back(condition);
        else
        {
            Require(condition->ScriptId != 0, "The native click has no owner condition binding");
            (sConditionMgr->*Access(PonyClickConditions{}))[condition->SourceGroup][ClickSpell].push_back(condition);
        }
    } while (rows->NextRow());

    auto area = WorldDatabase.Query("SELECT area,quest_start,quest_start_status,quest_end,quest_end_status,aura_spell,racemask,classmask,"
        "active_event,gender,autocast FROM spell_area WHERE spell=71914");
    Require(area && area->GetRowCount() == 1, "Missing scoped native aura cleanup row");
    auto fields = area->Fetch();
    SpellArea policy{};
    policy.spellId = PonyAura;
    policy.areaId = fields[0].GetInt32();
    policy.questStart = fields[1].GetUInt32();
    policy.questStartStatus = fields[2].GetUInt32();
    policy.questEnd = fields[3].GetUInt32();
    policy.questEndStatus = fields[4].GetUInt32();
    policy.auraSpell = fields[5].GetInt32();
    policy.raceMask = fields[6].GetUInt32();
    policy.classmask = fields[7].GetInt32();
    policy.active_event = fields[8].GetUInt32();
    policy.gender = Gender(fields[9].GetUInt8());
    policy.autocast = fields[10].GetBool();
    policy.spellInfo = sSpellMgr->GetSpellInfo(PonyAura);
    auto bound = (sSpellMgr->*Access(PonySpellAreas{})).emplace(PonyAura, policy);
    (sSpellMgr->*Access(PonyQuestAreas{})).emplace(PonyQuest, &bound->second);
    Require(policy.areaId == 4720 && policy.questStart == PonyQuest && policy.questStartStatus == (1u << QUEST_STATUS_INCOMPLETE) &&
        !policy.autocast && policy.spellInfo, "Wrong area, quest status or autocast policy");
    return quest;
}

struct PonyPlayer : LootPlayer
{
    PonyPlayer(Map& map, uint64 guid, Position const& point) : LootPlayer(SupplySession(), map, guid, point)
    {
        GetMap()->GetZoneAndAreaId(m_zoneId, m_areaId, GetPositionX(), GetPositionY(), GetPositionZ());
        Require(m_zoneId == 4720, "The real hatchling is outside the quest zone");
    }
};

struct PonyActor : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(sScriptMgr->GetCreatureAI(this)), "Matching Lost Isles AI could not initialize");
    }
};

SpellCastResult UsePony(PonyPlayer& player)
{
    auto result = player.CastSpell(&player, PonyAura, false, player.GetItemByEntry(PonyItem));
    Tick(player, 6000);
    return result;
}

void CheckPony(Quest const* quest)
{
    auto row = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM creature WHERE id=38412 AND map=648 ORDER BY guid LIMIT 1");
    Require(bool(row), "Missing an original hatchling spawn");
    auto f = row->Fetch();
    Position point{f[0].GetFloat(),f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat()};
    Map map(648, 300, 0, DIFFICULTY_NONE);
    PonyPlayer owner(map, 24864, point), other(map, 24865, point);
    GiveSupply(owner, PonyItem, 1);
    Require(UsePony(owner) != SPELL_CAST_OK && !owner.HasAura(PonyAura), "The item could be used without the quest");
    owner.DestroyItemCount(PonyItem, 1, true);
    for (auto player : {&owner, &other})
    {
        player->AddQuestAndCheckCompletion(quest, nullptr);
        Require(player->GetItemCount(PonyItem) == 1 && !player->HasAura(PonyAura), "Accept did not supply the real item without autocast");
        auto result = UsePony(*player);
        std::cout << "CASE pony cast=" << unsigned(result) << " aura=" << player->HasAura(PonyAura) << std::endl;
        Require(result == SPELL_CAST_OK && player->HasAura(PonyAura), "The native item cast did not activate the aura correctly");
    }

    PonyActor first(&map, Hatchling, 43701, point);
    Require(map.AddToMap(static_cast<Creature*>(&first)), "Could not publish the original hatchling");
    Require(!first.HandleSpellClick(&owner), "Unlured hatchling allowed interaction");
    first.AI()->MoveInLineOfSight_Safe(&owner);
    first.AI()->MoveInLineOfSight_Safe(&other);
    Require(owner.canSeeSpellClickOn(&first) && !other.canSeeSpellClickOn(&first), "The click marker was not projected per owner");
    Require(!first.HandleSpellClick(&other) && !other.GetQuestObjectiveData(PonyQuest, HatchlingCredit), "Player took another player's lure");
    Require(first.HandleSpellClick(&owner) && owner.GetQuestObjectiveData(PonyQuest, HatchlingCredit) == 1 &&
        !first.HandleSpellClick(&owner), "The owner did not collect exactly once");

    PonyActor waiting(&map, 44579, 43702, point);
    Require(map.AddToMap(static_cast<Creature*>(&waiting)), "Could not publish a second hatchling variant");
    waiting.AI()->MoveInLineOfSight_Safe(&owner);
    waiting.AI()->UpdateAI(20000);
    Require(!waiting.HandleSpellClick(&owner), "An expired lure allowed collection");
    waiting.AI()->MoveInLineOfSight_Safe(&owner);
    AbandonSupply(owner, PonyQuest);
    Require(!owner.GetItemCount(PonyItem) && !owner.HasAura(PonyAura) && !waiting.HandleSpellClick(&owner),
        "Abandon left the item, lure aura or interaction active");
    waiting.AI()->MoveInLineOfSight_Safe(&other);
    Require(waiting.HandleSpellClick(&other) && other.GetQuestObjectiveData(PonyQuest, HatchlingCredit) == 1,
        "A stale lure prevented the next eligible player from collecting");

    PonyActor finale(&map, 44580, 43703, point), siren(&map, Siren, 43704, point);
    Require(map.AddToMap(static_cast<Creature*>(&finale)) && map.AddToMap(static_cast<Creature*>(&siren)),
        "Could not publish the protected hatchling and siren");
    finale.AI()->MoveInLineOfSight_Safe(&other);
    Require(!finale.HandleSpellClick(&other), "The active protector failed to block the proximity lure");
    LootPet pet(other, map);
    siren.AI()->JustDied(&pet);
    other.SetQuestObjectiveData(quest, &quest->GetObjectives()[0], 11);
    Require(finale.HandleSpellClick(&other) && other.CanRewardQuest(quest, false) && !other.HasAura(PonyAura),
        "A pet owner's siren lure did not deliver the twelfth objective or clear the aura");
    other.RewardQuest(quest, 0, nullptr, false);
    Require(other.IsQuestRewarded(PonyQuest) && !other.GetItemCount(PonyItem), "Turn-in left the supplied pony item");
    Tick(owner, 500);
    Tick(other, 500);
    std::cout << "PASS native pony cast, actual click owner, duplicate denial, expiry and abandon, siren callback with pet, completion and item turn-in\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{648, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{648, {}}});
        LoadGameObjectModelList(argv[2]);
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_lost_isle();
        sScriptMgr->SwapScriptContext(true);
        auto quest = LoadPony();
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Reward writes require a disposable character schema");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto fixture = CharacterDatabase.Query("SELECT DATABASE(), (SELECT COUNT(*) FROM characters)");
        Require(fixture && fixture->Fetch()[0].GetString() == "fixture_char" && !fixture->Fetch()[1].GetUInt32(), "Reward requires an empty fixture");
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckPony(quest);
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
