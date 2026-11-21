#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "CreatureTextMgr.h"
#include "KyleLunch.h"

void AddSC_mulgore();

struct KyleConditionTag { using Type = bool (ConditionMgr::*)(Condition*); friend Type Access(KyleConditionTag); };
template struct FixtureAccess<KyleConditionTag, &ConditionMgr::addToSpellImplicitTargetConditions>;

namespace
{
void LoadKyleData()
{
    auto rows = WorldDatabase.Query(
#include "NativeKyleQuest.inc"
    );
    Require(bool(rows) && rows->GetRowCount() == 1, "Missing actual Kyle quest");
    auto quest = new Quest(rows->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(KyleLunch::Quest + 1);
    quests[KyleLunch::Quest] = quest;
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->Id] = quest;
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=11129");
    Require(bool(objectives) && objectives->GetRowCount() == 1, "Missing original Kyle objective");
    quest->LoadQuestObjective(objectives->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);

    auto conditions = WorldDatabase.Query("SELECT SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,"
        "ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ScriptName FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=42222");
    Require(bool(conditions) && conditions->GetRowCount() == 1, "Missing native Kyle target condition");
    auto row = conditions->Fetch();
    auto condition = new Condition;
    condition->SourceType = CONDITION_SOURCE_TYPE_SPELL_IMPLICIT_TARGET;
    condition->SourceGroup = row[0].GetUInt32();
    condition->SourceEntry = KyleLunch::Spell;
    condition->SourceId = row[1].GetUInt32();
    condition->ElseGroup = row[2].GetUInt32();
    condition->ConditionType = ConditionTypes(row[3].GetInt32());
    condition->ConditionTarget = row[4].GetUInt8();
    condition->ConditionValue1 = row[5].GetUInt32();
    condition->ConditionValue2 = row[6].GetUInt32();
    condition->ConditionValue3 = row[7].GetUInt32();
    condition->NegativeCondition = row[8].GetBool();
    Require(row[9].GetString().empty() && sConditionMgr->isConditionTypeValid(condition) &&
        (sConditionMgr->*Access(KyleConditionTag{}))(condition), "Kyle native target condition did not load");
}

struct StoryCreature : Creature
{
    ~StoryCreature()
    {
        if (IsInWorld())
        {
            CombatStop(true);
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        }
        if (FindMap())
            ResetMap();
    }

    void Spawn(Map& map, uint32 entry, Position const& pos)
    {
        auto guid = sObjectMgr->GetGenerator<HighGuid::Creature>()->Generate();
        Require(Create(guid, &map, 1, entry, 0, HORDE, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation()), "Missing Kyle fixture actor");
        SetHomePosition(pos);
        Require(map.AddToMap(static_cast<Creature*>(this)) && map.GetCreature(GetGUID()) == this, "Kyle fixture actor could not enter the grid");
    }
};

void Accept(LootPlayer& player)
{
    player.SetQuestStatus(KyleLunch::Quest, QUEST_STATUS_INCOMPLETE);
    player.SetQuestSlot(0, KyleLunch::Quest);
}

Item* AddChicken(LootPlayer& player)
{
    ItemPosCountVec dest;
    Require(player.CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, KyleLunch::Item, 1) == EQUIP_ERR_OK, "Cannot store actual quest item");
    Item* item = player.StoreNewItem(dest, KyleLunch::Item, true);
    Require(item && player.GetItemCount(KyleLunch::Item) == 1, "Missing actual chicken in the test bag");
    return item;
}

void UseChicken(LootPlayer& player, Item* item)
{
    player.CastSpell(&player, KyleLunch::Spell, false, item);
    Tick(player, 500);
}

void CheckFeeding(WorldSession* first, WorldSession* second)
{
    auto rows = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM creature WHERE guid=94002 AND id=23616 AND map=1");
    Require(bool(rows), "Missing actual Bloodhoof Village Kyle spawn");
    auto row = rows->Fetch();
    Position spot(row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat());
    Map map(1, 300, 9999992, DIFFICULTY_NONE);
    // The actual caster-summon offset places the plate several yards from Kyle.
    Position castSpot(spot.GetPositionX() + 5, spot.GetPositionY(), spot.GetPositionZ(), 3.14f);
    LootPlayer owner(first, map, 101, castSpot);
    LootPlayer other(second, map, 102, castSpot);
    for (LootPlayer* player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_TAUREN);
        player->setFactionForRace(RACE_TAUREN);
    }
    StoryCreature plainstrider;
    plainstrider.Spawn(map, 2956, castSpot);
    Require(!owner.HasQuestForItem(KyleLunch::Item), "An uninvolved player qualifies for the quest chicken loot");
    auto chicken = AddChicken(owner);
    UseChicken(owner, chicken);
    Require(owner.GetItemCount(KyleLunch::Item) == 1 && !owner.GetGameObject(KyleLunch::Spell), "A player without the quest could spend the chicken");
    Accept(owner);
    UseChicken(owner, chicken);
    Require(owner.GetItemCount(KyleLunch::Item) == 1 && !owner.GetGameObject(KyleLunch::Spell), "A nearby unrelated creature allowed the throw");

    StoryCreature kyle;
    kyle.Spawn(map, KyleLunch::Frenzied, spot);
    Accept(other);
    auto otherChicken = AddChicken(other);
    UseChicken(owner, chicken);
    auto food = owner.GetGameObject(KyleLunch::Spell);
    Require(food && food->GetEntry() == KyleLunch::Food && food->GetOwnerGUID() == owner.GetGUID() && food->HasQuestObjectScope() &&
        food->CanCollideWith(&owner) && !food->CanCollideWith(&other) && owner.canSeeOrDetect(food) && !other.canSeeOrDetect(food) &&
        owner.GetItemCount(KyleLunch::Item) == 0, "The real throw did not publish and consume one owner-bound chicken");
    UseChicken(other, otherChicken);
    Require(other.GetItemCount(KyleLunch::Item) == 1 && !other.GetGameObject(KyleLunch::Spell), "A second throw consumed the other player's chicken while Kyle was occupied");

    kyle.AI()->UpdateAI(5000);
    kyle.AI()->UpdateAI(12000);
    map.RemoveAllObjectsInRemoveList();
    Require(!owner.GetGameObject(KyleLunch::Spell) && owner.GetQuestObjectiveData(KyleLunch::Quest, KyleLunch::Frenzied) == 0,
        "An unfinished movement could claim credit or leave its food");

    UseChicken(owner, AddChicken(owner));
    Require(owner.GetGameObject(KyleLunch::Spell), "Kyle did not recover after movement timeout");
    owner.RemoveActiveQuest(KyleLunch::Quest);
    owner.SetQuestSlot(0, 0);
    Accept(owner);
    kyle.AI()->UpdateAI(5000);
    map.RemoveAllObjectsInRemoveList();
    Require(!owner.GetGameObject(KyleLunch::Spell) && owner.GetQuestObjectiveData(KyleLunch::Quest, KyleLunch::Frenzied) == 0,
        "Old food carried progress into a new accepted attempt");

    UseChicken(owner, AddChicken(owner));
    food = owner.GetGameObject(KyleLunch::Spell);
    Require(food && kyle.GetExactDist(food) > 2.5f, "Feeding fixture did not require real movement");
    for (uint32 elapsed = 0; elapsed < 21000 && owner.GetQuestStatus(KyleLunch::Quest) != QUEST_STATUS_COMPLETE; elapsed += 100)
    {
        kyle.Update(100);
        map.MoveAllCreaturesInMoveList();
        map.RemoveAllObjectsInRemoveList();
    }
    Require(owner.GetQuestStatus(KyleLunch::Quest) == QUEST_STATUS_COMPLETE && owner.GetQuestObjectiveData(KyleLunch::Quest, KyleLunch::Frenzied) == 1 &&
        other.GetQuestObjectiveData(KyleLunch::Quest, KyleLunch::Frenzied) == 0 && kyle.GetEntry() == KyleLunch::Friendly && !owner.GetGameObject(KyleLunch::Spell),
        "Real arrival and feeding did not complete exactly once for the owner");
    kyle.AI()->UpdateAI(30000);
    Require(kyle.GetEntry() == KyleLunch::Frenzied, "Kyle did not return from the friendly dance");
    Require(owner.CanRewardQuest(sQuestDataStore->GetQuestTemplate(KyleLunch::Quest), false) && !owner.HasQuestForItem(KyleLunch::Item),
        "The completed quest is not rewardable or still admits food loot");
    std::cout << "PASS native Kyle use, target rejection and item charges, personal food, timeout, abandon, actual feeding and single owner credit\n";
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
        LoadKyleData();
        sObjectMgr->LoadItemTemplates();
        sObjectMgr->LoadItemTemplateAddon();
        sObjectMgr->LoadPageTexts();
        sObjectMgr->LoadGameObjectTemplate();
        sCreatureTextMgr->LoadCreatureTexts();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_mulgore();
        sScriptMgr->SwapScriptContext(true);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        auto first = new WorldSession(0, std::string("Kyle owner"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
        auto other = new WorldSession(0, std::string("Kyle bystander"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
        CheckFeeding(first, other);
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
