#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "AnowynCrystal.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GameTime.h"
#include <thread>

void AddSC_ghostlands();

namespace
{
Quest const* LoadQuest()
{
    auto rows = WorldDatabase.Query(
#include "NativeAnowynQuest.inc"
    );
    Require(bool(rows) && rows->GetRowCount() == 1, "Missing Deactivate An'owyn quest");
    auto quest = new Quest(rows->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    quests[AnowynCrystal::Quest] = quest;
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size() - 1);
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->Id] = quest;
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=9169");
    Require(bool(objectives) && objectives->GetRowCount() == 1, "Missing original Moon Crystal objective");
    quest->LoadQuestObjective(objectives->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_CAST);
    return quest;
}

Position Placement(uint32 guid)
{
    auto rows = WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation FROM gameobject WHERE guid=%u AND map=530 AND id=181359", guid);
    Require(bool(rows), "Missing original Moon Crystal position");
    auto row = rows->Fetch();
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

struct MoonCrystal : GameObject
{
    ~MoonCrystal()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        if (FindMap())
            ResetMap();
    }

    void Spawn(Map& map, uint32 dbGuid, Position const& pos, uint32 phase)
    {
        auto guid = sObjectMgr->GetGenerator<HighGuid::GameObject>()->Generate();
        m_DBTableGuid = dbGuid;
        Require(Create(guid, AnowynCrystal::Entry, &map, phase, pos, G3D::Quat(0, 0, 0, 1), 100, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)) && map.GetGameObject(GetGUID()) == this && GetDBTableGUIDLow() == dbGuid,
            "Moon Crystal did not enter the grid with its original spawn identity");
        SetRespawnDelayTime(0);
        Update(10000);
    }
};

void Accept(LootPlayer& player)
{
    player.SetQuestStatus(AnowynCrystal::Quest, QUEST_STATUS_INCOMPLETE);
    player.SetQuestSlot(0, AnowynCrystal::Quest);
}

Item* AddOrb(LootPlayer& player)
{
    ItemPosCountVec dest;
    Require(player.CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, AnowynCrystal::Orb, 1) == EQUIP_ERR_OK, "Could not store the original orb");
    Item* orb = player.StoreNewItem(dest, AnowynCrystal::Orb, true);
    Require(orb && player.GetItemCount(AnowynCrystal::Orb) == 1 && orb->GetSpellCharges(0) == -1, "Orb has incorrect starting charges");
    return orb;
}

void Cast(LootPlayer& player, GameObject& crystal, Item* item)
{
    player.CastSpell(&crystal, AnowynCrystal::Opening, false, item);
    Tick(player, 5000);
}

void CheckCrystal(Quest const* quest, WorldSession* first, WorldSession* other)
{
    GameTime::UpdateGameTimers();
    Position pos = Placement(12008), copyPos = Placement(25167306);
    Map map(530, 300, 9999960, DIFFICULTY_NONE);
    LootPlayer owner(first, map, 100, pos), second(other, map, 101, copyPos);
    for (LootPlayer* player : {&owner, &second})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_BLOODELF);
        player->setFactionForRace(RACE_BLOODELF);
    }
    MoonCrystal crystal, copy;
    crystal.Spawn(map, 12008, pos, 1);
    copy.Spawn(map, 25167306, copyPos, 65535);
    Require(crystal.GetGOInfo()->GetLockId() == AnowynCrystal::Lock && crystal.GetGOInfo()->goober.questID == AnowynCrystal::Quest,
        "The original Lock.db2 and quest links changed");

#ifdef ANOWYN_UNGUARDED_BASELINE
    Accept(owner);
    Require(crystal.ActivateToQuest(&owner), "The original objective was not active");
    crystal.Use(&owner);
    Require(owner.CanRewardQuest(quest, false) && !owner.GetItemCount(AnowynCrystal::Orb), "Baseline did not reproduce credit from a bare click");
    Accept(second);
    Require(copy.ActivateToQuest(&second), "Baseline did not reproduce the false quest marker on the distant copy");
    Cast(second, copy, AddOrb(second));
    Require(second.CanRewardQuest(quest, false) && !second.GetItemCount(AnowynCrystal::Orb), "Baseline did not reproduce successful use at the distant copy");
    std::cout << "EXPECTED_BROKEN bare use credited without the orb and the distant copy advertised and accepted the quest cast\n";
#else
    Require(!crystal.ActivateToQuest(&owner) && !owner.HasQuestForItem(AnowynCrystal::Orb), "A player without the quest sees an active objective or orb loot");
    auto orb = AddOrb(owner);
    Cast(owner, crystal, orb);
    Require(owner.GetItemCount(AnowynCrystal::Orb) == 1 && !crystal.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE),
        "A player without the quest could spend the orb");
    Accept(owner);
    Accept(second);
    Require(second.HasQuestForItem(AnowynCrystal::Orb) && crystal.ActivateToQuest(&owner) && second.canSeeOrDetect(&copy) && !copy.ActivateToQuest(&second),
        "The real goal marker or orb loot is missing, or the visible copy advertises a quest objective");
    auto otherOrb = AddOrb(second);
    copy.Use(&second);
    Cast(second, copy, otherOrb);
    Require(second.GetItemCount(AnowynCrystal::Orb) == 1 && !second.GetQuestObjectiveData(AnowynCrystal::Quest, AnowynCrystal::Entry) &&
        !copy.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE), "Use on the distant copy consumed the orb or gave credit");
    crystal.Use(&owner);
    crystal.AI()->GossipHello(&owner, false);
    Cast(owner, crystal, nullptr);
    Require(owner.GetItemCount(AnowynCrystal::Orb) == 1 && !owner.GetQuestObjectiveData(AnowynCrystal::Quest, AnowynCrystal::Entry) &&
        !crystal.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE), "Raw or itemless interaction deactivated the crystal");
    map.PlayerRelocation(&second, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
    Cast(owner, crystal, orb);
    Require(owner.GetQuestObjectiveData(AnowynCrystal::Quest, AnowynCrystal::Entry) == 1 && owner.CanRewardQuest(quest, false) &&
        !owner.GetItemCount(AnowynCrystal::Orb) && !crystal.ActivateToQuest(&owner) && crystal.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE),
        "Native opening did not consume the orb, give credit, clear the marker and animate the crystal");
    Cast(second, crystal, otherOrb);
    Require(second.GetItemCount(AnowynCrystal::Orb) == 1 && !second.GetQuestObjectiveData(AnowynCrystal::Quest, AnowynCrystal::Entry),
        "The shared animation consumed another player's orb or awarded unintended credit");
    Require(second.TakeQuestSourceItem(AnowynCrystal::Quest, false) && !second.GetItemCount(AnowynCrystal::Orb), "Abandon did not clean the unspent quest orb");
    second.RemoveActiveQuest(AnowynCrystal::Quest);
    second.SetQuestSlot(0, 0);
    Require(!crystal.ActivateToQuest(&second) && !second.HasQuestForItem(AnowynCrystal::Orb), "Abandon did not clear the marker and loot eligibility");
    Accept(second);
    Require(second.HasQuestForItem(AnowynCrystal::Orb), "Reaccept cannot recover the orb from its source");
    std::this_thread::sleep_for(std::chrono::seconds(16));
    GameTime::UpdateGameTimers();
    for (unsigned tick = 0; tick < 3; ++tick)
        crystal.Update(10000);
    Require(crystal.isSpawned() && !crystal.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE), "The shared crystal did not return after its actual animation timer");
    Cast(second, crystal, AddOrb(second));
    Require(second.CanRewardQuest(quest, false) && !second.GetItemCount(AnowynCrystal::Orb), "The restored crystal could not serve a second player");
    std::cout << "PASS native An'owyn POI and spare model, actual orb use, raw-use rejection, shared animation, abandon and reaccept\n";
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
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
#ifndef ANOWYN_UNGUARDED_BASELINE
        AddSC_ghostlands();
#endif
        sScriptMgr->SwapScriptContext(true);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        auto first = new WorldSession(0, std::string("Anowyn owner"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
        auto other = new WorldSession(0, std::string("Anowyn bystander"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
        CheckCrystal(quest, first, other);
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
