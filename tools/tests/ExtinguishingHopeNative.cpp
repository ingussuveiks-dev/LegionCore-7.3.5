#define MASTER_LOOT_NATIVE_SUPPORT_ONLY
#include "MasterLootNative.cpp"
#include "GameTime.h"
#include "SmartAI.h"

struct NorthshireConditionsTag
{
    using Type = ConditionContainer ConditionMgr::*;
    friend Type Access(NorthshireConditionsTag);
};
template struct FixtureAccess<NorthshireConditionsTag, &ConditionMgr::ConditionStore>;
struct NorthshireEventsTag
{
    using Type = SmartEventConditionContainer ConditionMgr::*;
    friend Type Access(NorthshireEventsTag);
};
template struct FixtureAccess<NorthshireEventsTag, &ConditionMgr::SmartEventConditionStore>;
struct NorthshireSmartTag
{
    using Type = SmartAIEventMap (SmartAIMgr::*)[SMART_SCRIPT_TYPE_MAX];
    friend Type Access(NorthshireSmartTag);
};
template struct FixtureAccess<NorthshireSmartTag, &SmartAIMgr::mEventMap>;
struct NorthshireSpellAreaTag
{
    using Type = SpellAreaMap SpellMgr::*;
    friend Type Access(NorthshireSpellAreaTag);
};
template struct FixtureAccess<NorthshireSpellAreaTag, &SpellMgr::mSpellAreaMap>;
struct NorthshireQuestAreaTag
{
    using Type = SpellAreaForQuestMap SpellMgr::*;
    friend Type Access(NorthshireQuestAreaTag);
};
template struct FixtureAccess<NorthshireQuestAreaTag, &SpellMgr::mSpellAreaForQuestMap>;

namespace
{
constexpr uint32 QuestId = 26391;
constexpr uint32 Extinguisher = 58362;
constexpr uint32 Controller = 80199;
constexpr uint32 Spray = 80208;
constexpr uint32 FireEntry = 42940;
constexpr uint32 Milly = 9296;
constexpr uint32 Prerequisites[] = {28817, 28818, 28819, 28820, 28821, 28822, 28823, 29083, 31145};

Quest const* LoadQuest()
{
    auto rows = WorldDatabase.Query(
#include "NativeExtinguishingHopeQuest.inc"
    );
    Require(rows && rows->GetRowCount() == 10, "Missing Extinguishing Hope or class prerequisite");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->Id] = quest;
    } while (rows->NextRow());
    auto quest = quests[QuestId];
    auto objectives = WorldDatabase.Query(
        "SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=26391");
    Require(objectives && objectives->GetRowCount() == 1 && quest->SourceItemId == Extinguisher, "Unexpected quest or source item");
    quest->LoadQuestObjective(objectives->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    auto addon = WorldDatabase.Query("SELECT ProvidedItemCount FROM quest_template_addon WHERE ID=26391");
    Require(addon && addon->Fetch()[0].GetUInt8() == 1 && quest->Objectives.front().ObjectID == FireEntry &&
        quest->Objectives.front().Amount == 8, "Expected one bucket and eight fires");
    quest->SourceItemIdCount = addon->Fetch()[0].GetUInt8();
    return quest;
}

void LoadVineyardConditions()
{
    auto rows = WorldDatabase.Query("SELECT spell,area,quest_start,quest_start_status,quest_end,quest_end_status,aura_spell,racemask,gender,autocast,classmask,active_event FROM spell_area WHERE spell=80199");
    Require(rows && rows->GetRowCount() == 1, "Missing periodic controller's area requirement");
    auto row = rows->Fetch();
    SpellArea area{};
    area.spellId = row[0].GetUInt32();
    area.areaId = row[1].GetInt32();
    area.questStart = row[2].GetUInt32();
    area.questStartStatus = row[3].GetUInt32();
    area.questEnd = row[4].GetUInt32();
    area.questEndStatus = row[5].GetUInt32();
    area.auraSpell = row[6].GetInt32();
    area.raceMask = row[7].GetUInt32();
    area.gender = Gender(row[8].GetUInt8());
    area.autocast = row[9].GetBool();
    area.classmask = row[10].GetInt32();
    area.active_event = row[11].GetUInt32();
    area.spellInfo = sSpellMgr->GetSpellInfo(Controller);
    Require(area.areaId == 59 && !area.autocast && area.spellInfo && area.spellInfo->Effects[EFFECT_0]->TriggerSpell == Spray,
        "The native area or periodic trigger changed");
    auto loaded = (sSpellMgr->*Access(NorthshireSpellAreaTag{})).emplace(Controller, area);
    if (area.questStart)
        (sSpellMgr->*Access(NorthshireQuestAreaTag{})).emplace(area.questStart, &loaded->second);

    auto conditions = WorldDatabase.Query(
        "SELECT SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition "
        "FROM conditions WHERE (SourceTypeOrReferenceId=22 AND SourceEntry=42940) OR (SourceTypeOrReferenceId=19 AND SourceEntry=26391) "
        "OR (SourceTypeOrReferenceId IN (13,17) AND SourceEntry IN (80199,80208))");
#ifdef EXTINGUISHING_HOPE_BASELINE
    Require(!conditions && area.questStart == 0, "Expected original admissions");
#else
    Require(conditions && conditions->GetRowCount() == 15 && area.questStart == QuestId &&
        area.questStartStatus == 1 << QUEST_STATUS_INCOMPLETE, "Expected guarded spell, fire and admission data");
    do
    {
        auto fields = conditions->Fetch();
        auto condition = new Condition;
        condition->SourceType = ConditionSourceType(fields[0].GetUInt32());
        condition->SourceGroup = fields[1].GetUInt32();
        condition->SourceEntry = fields[2].GetUInt32();
        condition->SourceId = fields[3].GetUInt32();
        condition->ElseGroup = fields[4].GetUInt32();
        condition->ConditionType = ConditionTypes(fields[5].GetUInt32());
        condition->ConditionTarget = fields[6].GetUInt8();
        condition->ConditionValue1 = fields[7].GetUInt32();
        condition->ConditionValue2 = fields[8].GetUInt32();
        condition->ConditionValue3 = fields[9].GetUInt32();
        condition->NegativeCondition = fields[10].GetBool();
        Require(sConditionMgr->isConditionTypeValid(condition) && (sConditionMgr->*Access(ValidateSourceTag{}))(condition),
            "One of the actual native conditions is invalid");
        if (condition->SourceType == CONDITION_SOURCE_TYPE_SPELL_IMPLICIT_TARGET)
            Require((sConditionMgr->*Access(AttachConditionTag{}))(condition), "Native cone target filter was rejected");
        else if (condition->SourceType == CONDITION_SOURCE_TYPE_SMART_EVENT)
            (sConditionMgr->*Access(NorthshireEventsTag{}))[{condition->SourceEntry, condition->SourceId}][condition->SourceGroup].push_back(condition);
        else
            (sConditionMgr->*Access(NorthshireConditionsTag{}))[condition->SourceType][condition->SourceEntry].push_back(condition);
    } while (conditions->NextRow());
#endif
}

void LoadFireEvents()
{
    auto rows = WorldDatabase.Query(
        "SELECT id,link,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,"
        "action_type,action_param1,action_param2,target_type FROM smart_scripts WHERE source_type=0 AND entryorguid=42940 ORDER BY id");
    Require(rows && rows->GetRowCount() == 5, "Fire SmartAI graph does not have its five rows");
    auto& store = sSmartScriptMgr->*Access(NorthshireSmartTag{});
    do
    {
        auto row = rows->Fetch();
        SmartScriptHolder holder;
        holder.entryOrGuid = FireEntry;
        holder.event_id = row[0].GetUInt32();
        holder.link = row[1].GetUInt32();
        holder.event.type = SMART_EVENT(row[2].GetUInt32());
        holder.event.event_phase_mask = row[3].GetUInt32();
        holder.event.event_chance = row[4].GetUInt32();
        holder.event.event_flags = row[5].GetUInt32();
        holder.event.spellHit = {row[6].GetUInt32(), row[7].GetUInt32(), row[8].GetUInt32(), row[9].GetUInt32()};
        holder.action.type = SMART_ACTION(row[10].GetUInt32());
        holder.action.cast.spell = row[11].GetUInt32();
        holder.action.cast.flags = row[12].GetUInt32();
        holder.target = {};
        holder.target.type = SMARTAI_TARGETS(row[13].GetUInt32());
        store[SMART_SCRIPT_TYPE_CREATURE][FireEntry].push_back(holder);
    } while (rows->NextRow());
}

struct Fire : Actor
{
    Fire(Map& map, uint32 guid, Position const& pos) : Actor(&map, FireEntry, guid, pos) { }

    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(new SmartAI(this)), "Native fire SmartAI initialization failed");
    }
};

WorldSession* Session()
{
    return new WorldSession(0, "Extinguishing Hope native probe", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
}

void FaceFire(LootPlayer& player, Position const& fire, float angle = 0.0f)
{
    player.GetMap()->PlayerRelocation(&player, fire.GetPositionX() - 2 * std::cos(angle),
        fire.GetPositionY() - 2 * std::sin(angle), fire.GetPositionZ() + 0.5f, angle);
    player.m_zoneId = 6170;
    player.m_areaId = 59;
}

SpellCastResult UseBucket(LootPlayer& player)
{
    player.GetGlobalCooldownMgr().CancelGlobalCooldown(sSpellMgr->GetSpellInfo(Controller));
    player.RemoveSpellCooldown(Controller, false);
    return player.CastSpell(&player, Controller, false, player.GetItemByEntry(Extinguisher));
}

void CheckVineyard(Quest const* quest)
{
    auto placements = WorldDatabase.Query(
        "SELECT guid,position_x,position_y,position_z FROM creature WHERE id=42940 AND map=0 AND areaId=59 AND spawntimesecs=120 ORDER BY guid");
    Require(placements && placements->GetRowCount() == 12, "Expected twelve native world fires");
    auto row = placements->Fetch();
    Position pos{row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat(), 0};
    Map map(0, 300, 9999905, DIFFICULTY_NONE);
    LootPlayer player(Session(), map, 438, pos), stranger(Session(), map, 439, pos);
    for (auto who : {&player, &stranger})
    {
        FaceFire(*who, pos);
        who->SetLevel(4);
        who->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, sObjectMgr->GetXPForLevel(4));
    }
    Require(stranger.AddItem(Extinguisher, 1), "Unable to position a retained bucket");
    auto staleCheck = std::make_unique<Fire>(map, 1000, pos);
    Require(map.AddToMap(static_cast<Creature*>(staleCheck.get())), "Native vineyard fire not in grid");

#ifdef EXTINGUISHING_HOPE_BASELINE
    Require(player.CanTakeQuest(quest, false), "Old admission unexpectedly requires its prerequisite");
    Require(UseBucket(stranger) == SPELL_CAST_OK, "Baseline did not apply the retained bucket without the quest");
    Tick(stranger, 1250);
    Require(staleCheck->isDead() && !stranger.GetQuestObjectiveData(QuestId, FireEntry), "Baseline did not waste a shared fire without the quest");
    std::cout << "EXPECTED_BROKEN admission lacked its prerequisite and a retained bucket consumed a shared fire without the quest\n";
#else
    Require(!player.CanTakeQuest(quest, false), "Milly offers the quest before its prerequisite");
    auto& rewarded = const_cast<RewardedQuestSet&>(player.getRewardedQuests());
    for (uint32 prior : Prerequisites)
    {
        rewarded.insert(prior);
        Require(player.CanTakeQuest(quest, false), "A legitimate class prerequisite cannot unlock Milly's quest");
        rewarded.erase(prior);
    }
    rewarded.insert(28817);
    Require(UseBucket(stranger) != SPELL_CAST_OK &&
        stranger.CastSpell(&stranger, Spray, false) != SPELL_CAST_OK, "A retained bucket or direct spray is usable without the quest");
    staleCheck->AI()->SpellHit(&stranger, sSpellMgr->GetSpellInfo(Spray));
    Require(staleCheck->IsAlive(), "A direct invalid hit still consumed a fire");
    staleCheck.reset();
    stranger.DestroyItemCount(Extinguisher, 1, true);

    LootPlayer partner(Session(), map, 440, pos);
    FaceFire(partner, pos);
    partner.SetLevel(4);
    partner.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, sObjectMgr->GetXPForLevel(4));
    partner.SetRewardedQuest(28817);
    partner.AddQuestAndCheckCompletion(quest, nullptr);
    ProbeParty party(player, partner);
    player.AddQuestAndCheckCompletion(quest, nullptr);
    Require(player.GetItemCount(Extinguisher) == 1, "Acceptance should provide a usable extinguisher");
    player.m_zoneId = 12;
    player.m_areaId = 9;
    Require(UseBucket(player) != SPELL_CAST_OK, "The bucket is usable outside the vineyard area");
    FaceFire(player, pos);
    do
    {
        row = placements->Fetch();
        Position site{row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat(), 0};
        auto count = player.GetQuestObjectiveData(QuestId, FireEntry);
        if (count == 8)
            break;
        FaceFire(player, site);
        FaceFire(partner, site);
        FaceFire(stranger, site);
        Fire fire(map, 1001 + count, site);
        Require(map.AddToMap(static_cast<Creature*>(&fire)), "A real fire placement failed");
        // Try a clear side of each real spawn. A wall or vine between the two
        // positions can make a particular approach fail normal spell LOS.
        for (float angle : {0.0f, 1.5708f, 3.14159f, 4.71239f})
        {
            FaceFire(player, site, angle);
            if (!player.IsWithinLOSInMap(&fire))
                continue;
            Require(UseBucket(player) == SPELL_CAST_OK, "Native carried bucket did not start its periodic effect");
            Tick(player, 1250);
            if (fire.isDead())
                break;
            player.RemoveAurasDueToSpell(Controller);
        }
        std::cout << "CASE before=" << count << " caster=" << player.GetQuestObjectiveData(QuestId, FireEntry)
            << " group=" << partner.GetQuestObjectiveData(QuestId, FireEntry) << " bystander=" << stranger.GetQuestObjectiveData(QuestId, FireEntry)
            << " fire_dead=" << fire.isDead() << " aura=" << player.HasAura(Controller) << std::endl;
        Require(fire.isDead() && player.GetQuestObjectiveData(QuestId, FireEntry) == count + 1 &&
            partner.GetQuestObjectiveData(QuestId, FireEntry) == count + 1 &&
            !stranger.GetQuestObjectiveData(QuestId, FireEntry), "Periodic cone target or linked credit/death is incorrect");
        if (count + 1 < 8)
            player.RemoveAurasDueToSpell(Controller);
    } while (placements->NextRow());
    Require(player.GetQuestObjectiveData(QuestId, FireEntry) == 8 && player.CanRewardQuest(quest, false) &&
        !player.HasAura(Controller) && player.GetItemCount(Extinguisher) == 1 && UseBucket(player) != SPELL_CAST_OK,
        "Eight valid fires did not finish the quest and stop the controller");
    Actor milly(&map, Milly, 3000, player.GetPosition());
    Require(map.AddToMap(static_cast<Creature*>(&milly)), "Milly template not instantiated");
    player.RewardQuest(quest, 0, &milly, false);
    Require(player.GetQuestRewardStatus(QuestId) && !player.GetItemCount(Extinguisher) && !player.HasAura(Controller),
        "The native turn-in left the extinguisher or aura");
    stranger.SetRewardedQuest(28817);
    Require(stranger.CanTakeQuest(quest, false), "The abandonment case cannot accept through native admission");
    stranger.AddQuestAndCheckCompletion(quest, nullptr);
    Require(stranger.GetItemCount(Extinguisher) == 1 && UseBucket(stranger) == SPELL_CAST_OK, "Abandonment bucket did not start its cast");
    Tick(stranger, 500);
    Require(stranger.HasAura(Controller) && stranger.TakeQuestSourceItem(QuestId, false), "Abandonment requires an active controller and carried item");
    stranger.RemoveActiveQuest(QuestId);
    stranger.SetQuestSlot(0, 0);
    Tick(stranger, 1200);
    Require(!stranger.GetItemCount(Extinguisher) && !stranger.HasAura(Controller) && UseBucket(stranger) != SPELL_CAST_OK,
        "Abandonment left the item or a running periodic controller");
    Require(stranger.CanTakeQuest(quest, false), "Abandonment blocked a legitimate retry");
    stranger.AddQuestAndCheckCompletion(quest, nullptr);
    Require(stranger.GetItemCount(Extinguisher) == 1 && UseBucket(stranger) == SPELL_CAST_OK, "Fresh acceptance did not recover the bucket");
    std::cout << "PASS native prerequisite variants, bucket aura and cone, eight distinct fire hits, eligible group, bystander, reward and retry\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}});
        auto quest = LoadQuest();
        sObjectMgr->LoadItemTemplates();
        sObjectMgr->LoadItemTemplateAddon();
        sObjectMgr->LoadPlayerInfo();
        LoadVineyardConditions();
        LoadFireEvents();
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
#ifndef EXTINGUISHING_HOPE_BASELINE
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Reward requires a disposable character schema");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto fixture = CharacterDatabase.Query("SELECT DATABASE(), (SELECT COUNT(*) FROM characters)");
        Require(fixture && fixture->Fetch()[0].GetString() == "fixture_char" && !fixture->Fetch()[1].GetUInt32(), "Reward requires an empty fixture");
#endif
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckVineyard(quest);
#ifndef EXTINGUISHING_HOPE_BASELINE
        CharacterDatabase.Close();
#endif
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
