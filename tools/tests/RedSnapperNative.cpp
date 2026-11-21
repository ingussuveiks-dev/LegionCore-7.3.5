#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "GameObject.h"
#include "GameTime.h"
#include "TemporarySummon.h"

void AddSC_quest_spell_scripts();

namespace
{
constexpr uint32 QuestId = 9452;
constexpr uint32 Net = 23654;
constexpr uint32 Fish = 23614;
constexpr uint32 Murloc = 17102;
constexpr uint32 CastNet = 29866;
constexpr uint32 School = 181616;

Quest const* LoadQuest()
{
    auto rows = WorldDatabase.Query(
#include "NativeRedSnapperQuest.inc"
    );
    Require(rows && rows->GetRowCount() == 1, "Missing native Red Snapper quest");
    auto quest = new Quest(rows->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    quests[QuestId] = quest;
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[QuestId] = quest;
    auto objectives = WorldDatabase.Query(
        "SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=9452 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 2, "Missing fish or net delivery objective");
    do
        quest->LoadQuestObjective(objectives->Fetch());
    while (objectives->NextRow());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    auto addon = WorldDatabase.Query("SELECT ProvidedItemCount FROM quest_template_addon WHERE ID=9452");
    Require(addon && addon->Fetch()[0].GetUInt8() == 1 && quest->SourceItemId == Net, "Expected one native quest net");
    quest->SourceItemIdCount = addon->Fetch()[0].GetUInt8();
    return quest;
}

void LoadFishingEvidence()
{
    auto& table = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto savedTable = table;
    table += " WHERE Entry=17102 AND Item=23614";
    Require((LootTemplates_Creature.*Access(LoadLootTableTag{}))() == 1, "Missing actual murloc loot row");
    table = savedTable;

    auto rows = WorldDatabase.Query(
        "SELECT SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2 "
        "FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=29866");
    Require(rows && rows->GetRowCount() == 1, "Missing native focus condition");
    auto row = rows->Fetch();
    auto condition = new Condition;
    condition->SourceType = CONDITION_SOURCE_TYPE_SPELL_IMPLICIT_TARGET;
    condition->SourceEntry = CastNet;
    condition->SourceGroup = row[0].GetUInt32();
    condition->ElseGroup = row[1].GetUInt32();
    condition->ConditionType = ConditionTypes(row[2].GetUInt32());
    condition->ConditionTarget = row[3].GetUInt8();
    condition->ConditionValue1 = row[4].GetUInt32();
    condition->ConditionValue2 = row[5].GetUInt32();
    Require(sConditionMgr->isConditionTypeValid(condition) && (sConditionMgr->*Access(ValidateSourceTag{}))(condition) &&
        (sConditionMgr->*Access(AttachConditionTag{}))(condition), "Actual school condition cannot be loaded");
}

struct Focus : GameObject
{
    ~Focus()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        if (FindMap())
            ResetMap();
    }
};

WorldSession* Session()
{
    return new WorldSession(0, "Red Snapper native probe", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
}

void Move(LootPlayer& player, Position const& pos)
{
    player.GetMap()->PlayerRelocation(&player, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
}

bool ThrowNet(LootPlayer& player, Item* net)
{
    auto spell = sSpellMgr->GetSpellInfo(CastNet);
    player.GetGlobalCooldownMgr().CancelGlobalCooldown(spell);
    player.RemoveSpellCooldown(CastNet, false);
    player.CastSpell(&player, CastNet, false, net);
    Tick(player, 5000);
    auto enemy = player.FindNearestCreature(Murloc, 4.0f);
    if (!enemy)
        return false;
    auto summon = enemy->ToTempSummon();
    Require(summon && summon->GetSummonerGUID() == player.GetGUID() && summon->getFaction() == 14,
        "Murloc was not the hostile native summon of the caster");
    summon->UnSummon();
    player.GetMap()->RemoveAllObjectsInRemoveList();
    player.CombatStop(true);
    return true;
}

bool CanLootFish(LootPlayer& player)
{
    for (unsigned attempt = 0; attempt < 40; ++attempt)
    {
        Loot loot;
        loot.FillLoot(Murloc, LootTemplates_Creature, &player, true, true);
        WorldPackets::Loot::LootResponse packet;
        loot.BuildLootResponse(packet, &player, ALL_PERMISSION);
        for (auto const& item : packet.Items)
            if (item.Loot.ItemID == Fish && item.Quantity == 1)
                return true;
    }
    return false;
}

void CheckFishing(Quest const* quest)
{
    auto rows = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM gameobject WHERE guid=7656 AND id=181616 AND map=530");
    Require(rows && rows->GetRowCount() == 1, "Native Azuremyst school spawn missing");
    auto row = rows->Fetch();
    Position const pos{row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
    Map map(530, 300, 9999938, DIFFICULTY_NONE);
    LootPlayer owner(Session(), map, 237, pos), other(Session(), map, 238, pos);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_DRAENEI);
        player->setFactionForRace(RACE_DRAENEI);
    }
    Focus school;
    auto guid = sObjectMgr->GetGenerator<HighGuid::GameObject>()->Generate();
    Require(school.Create(guid, School, &map, 1, pos, G3D::Quat(0, 0, 0, 1), 100, GO_STATE_READY) &&
        school.GetGOInfo()->spellFocus.spellFocusType == 1369 && map.AddToMap(static_cast<GameObject*>(&school)), "Native fishing focus unavailable");
    school.Update(100);
    Require(other.AddItem(Net, 1), "Unable to position a stale net for the counterfactual");
    auto staleNet = other.GetItemByEntry(Net);

#ifdef RED_SNAPPER_BASELINE
    Require(ThrowNet(other, staleNet) || other.GetItemCount(Fish) == 1, "Baseline did not cast the held net without the quest");
    Require(CanLootFish(other), "Baseline did not expose fish to a player without the quest");
    std::cout << "EXPECTED_BROKEN a stale net fished at the school and murloc loot exposed the fish without the quest\n";
#else
    Require(!ThrowNet(other, staleNet) && !other.GetItemCount(Fish) && !CanLootFish(other), "Unaccepted quest still allows net or fish loot");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    Item* net = owner.GetItemByEntry(Net);
    Require(net && owner.GetItemCount(Net) == 1 && owner.HasQuestForItem(Fish) && CanLootFish(owner), "Active quest has no net or eligible murloc loot");
    Require(!ThrowNet(owner, nullptr) && !owner.GetItemCount(Fish), "The spell produced a fishing result without its net");
    Move(owner, {pos.GetPositionX() + 30.0f, pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation()});
    Require(!ThrowNet(owner, net) && !owner.GetItemCount(Fish), "Fishing succeeded outside the school");
    Move(owner, pos);
    bool summoned = false;
    for (unsigned attempt = 0; attempt < 24 && (!summoned || !owner.GetItemCount(Fish)); ++attempt)
        summoned = ThrowNet(owner, net) || summoned;
    Require(summoned && owner.GetItemCount(Fish) && owner.GetItemCount(Net) == 1, "Both native net outcomes did not appear");
    if (auto remaining = 10u - owner.GetItemCount(Fish))
        Require(owner.AddItem(Fish, remaining), "Unable to reach exact quest quota for cleanup");
    Require(owner.GetItemCount(Fish) == 10 && owner.CanRewardQuest(quest, false), "Correct inventory does not permit turn-in");
    Require(!ThrowNet(owner, net) && owner.GetItemCount(Fish) == 10 && !CanLootFish(owner), "A completed quest still yields fish");
    owner.RewardQuest(quest, 0, nullptr, false);
    Require(owner.GetQuestRewardStatus(QuestId) && !owner.GetItemCount(Net) && !owner.GetItemCount(Fish), "Native reward left a net or fish in the bag");
    other.AddQuestAndCheckCompletion(quest, nullptr);
    Require(other.AddItem(Fish, 2), "Unable to exercise abandon with fish");
    Require(other.TakeQuestSourceItem(QuestId, false) && !other.GetItemCount(Net), "Abandon left the quest-start net");
    other.RemoveActiveQuest(QuestId);
    other.SetQuestSlot(0, 0);
    Require(!CanLootFish(other) && !other.HasQuestForItem(Fish), "Abandoned quest still yields murloc fish");
    std::cout << "PASS native Red Snapper focus, both net outcomes, quest loot, quota and reward cleanup\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{530, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{530, {}}});
        Quest const* quest = LoadQuest();
        sObjectMgr->LoadItemTemplates();
        sObjectMgr->LoadItemTemplateAddon();
        sObjectMgr->LoadPageTexts();
        sObjectMgr->LoadGameObjectTemplate();
        LoadFishingEvidence();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_quest_spell_scripts();
        sScriptMgr->SwapScriptContext(true);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
#ifndef RED_SNAPPER_BASELINE
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Reward requires the disposable character schema");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto fixture = CharacterDatabase.Query("SELECT DATABASE(), (SELECT COUNT(*) FROM characters)");
        Require(fixture && fixture->Fetch()[0].GetString() == "fixture_char" && !fixture->Fetch()[1].GetUInt32(), "Reward requires an empty fixture");
#endif
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckFishing(quest);
#ifndef RED_SNAPPER_BASELINE
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
