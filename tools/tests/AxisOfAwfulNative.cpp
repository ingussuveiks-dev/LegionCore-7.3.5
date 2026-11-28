#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "CreatureTextMgr.h"
#include "SmartScriptMgr.h"
#include "SpellPackets.h"

struct RepeatTextTag
{
    using Type = CreatureTextRepeatIds (CreatureTextMgr::*)(Creature*, uint8);
    friend Type Access(RepeatTextTag);
};
template struct FixtureAccess<RepeatTextTag, &CreatureTextMgr::GetRepeatGroup>;

namespace
{
constexpr uint32 Axis = 26868;
constexpr uint32 MosshideTag = 44266;
constexpr uint32 PlantKit = 60502;
constexpr uint32 Pheromone = 60503;
constexpr uint32 PlantAura = 82788;
constexpr uint32 TagSpell = 82799;

bool Spoke(Creature& actor, uint8 group)
{
    return !(sCreatureTextMgr->*Access(RepeatTextTag{}))(&actor, group).empty();
}

Quest const* LoadAxisInputs()
{
    auto row = WorldDatabase.Query(
#include "NativeAxisOfAwful.inc"
    );
    Require(row && row->GetRowCount() == 1, "Missing Axis of Awful");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Axis] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Axis, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=26868");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == MosshideTag &&
        objective->Fetch()[5].GetInt32() == 1, "Missing native Mosshide tagged objective");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    sSpellMgr->LoadSpellProcEvents();
    sSpellMgr->LoadSpellProcs();

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(96525,96526,96528,96530,96531)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text WHERE CreatureID IN(44262,44263,44264)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE entryorguid IN(44262,44263,4426200,4426300)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions "
        "WHERE (SourceTypeOrReferenceId IN(13,17) AND SourceEntry=82799) OR (SourceTypeOrReferenceId=22 AND SourceEntry IN(44262,44263))");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sCreatureTextMgr->LoadCreatureTexts();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sConditionMgr->LoadConditions();
    return quest;
}

struct MeetingActor : Creature
{
    MeetingActor(Map& map, uint32 guid) : Creature(false)
    {
        auto spawn = sObjectMgr->GetCreatureData(guid);
        Require(spawn, "Missing the original meeting actor");
        map.LoadGrid(spawn->posX, spawn->posY);
        Require(LoadCreatureFromDB(guid, &map, true) && IsInWorld() && AI(), "Could not load original meeting actor");
    }
    ~MeetingActor()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        ResetMap();
    }
};

void UseMeetingItem(LootPlayer& player, uint32 itemId, uint32 spell, Unit& target)
{
    auto info = sSpellMgr->GetSpellInfo(spell);
    for (unsigned pulse = 0; pulse < 120 && (player.GetGlobalCooldownMgr().HasGlobalCooldown(info) || player.HasSpellCooldown(spell)); ++pulse)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        GameTime::UpdateGameTimers();
        Tick(player, 50);
    }
    Require(!player.GetGlobalCooldownMgr().HasGlobalCooldown(info) && !player.HasSpellCooldown(spell),
        "The item test did not get past the native cooldown");
    Item* item = player.GetItemByEntry(itemId);
    Require(item, "Missing the actual cache item");
    WorldPackets::Spells::ItemUse use{WorldPacket(CMSG_USE_ITEM)};
    use.itemGUID = item->GetGUID();
    use.bagIndex = item->GetBagSlot();
    use.slot = item->GetSlot();
    use.Cast.SpellID = spell;
    use.Cast.Target.Flags = TARGET_FLAG_UNIT;
    use.Cast.Target.Unit = target.GetGUID();
    player.GetSession()->HandleUseItemOpcode(use);
    Tick(player, 3000);
}

void CheckAxis(Quest const* quest)
{
    for (uint32 guid : {96525,96526,96528,96530,96531})
        if (auto spawn = sObjectMgr->GetCreatureData(guid))
            sObjectMgr->RemoveCreatureFromGrid(guid, spawn);

    Map map(0, 300, 0, DIFFICULTY_NONE);
    MeetingActor mosshide(map,96526), leader(map,96528), second(map,96530), third(map,96531), kobold(map,96525);
    Position start = mosshide.GetNearPosition(4.0f, 0);
    auto session = [] { return new WorldSession(0, "Axis native probe", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {}); };
    LootPlayer owner(session(), map, 2686801, start), visitor(session(), map, 2686802, start);
    for (auto player : {&owner, &visitor})
    {
        player->InitDisplayIds();
        player->SetLevel(16);
        player->UpdateZone(38, 556);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE, UNIT_FIELD_MOD_HASTE_REGEN})
            player->SetFloatValue(field, 1.0f);

        for (uint32 id : {PlantKit, Pheromone})
        {
            ItemPosCountVec destination;
            Require(player->CanStoreNewItem(NULL_BAG, NULL_SLOT, destination, id, 1) == EQUIP_ERR_OK &&
                player->StoreNewItem(destination, id, true), "Could not seed the two real cache items");
        }
    }

    owner.AddQuestAndCheckCompletion(quest, nullptr);
    UseMeetingItem(owner, Pheromone, TagSpell, mosshide);
    Require(!owner.GetQuestObjectiveData(Axis, MosshideTag) && !mosshide.HasAura(TagSpell) && owner.HasItemCount(Pheromone),
        "An undisguised use started the meeting or consumed the pheromone");
    UseMeetingItem(visitor, PlantKit, PlantAura, visitor);
    Require(visitor.HasAura(PlantAura), "Native Clever Plant Disguise did not apply");
    UseMeetingItem(visitor, Pheromone, TagSpell, mosshide);
    Require(!mosshide.HasAura(TagSpell), "An inactive quest could tag Mosshide");
    UseMeetingItem(owner, PlantKit, PlantAura, owner);
    Require(owner.HasAura(PlantAura), "Owner could not apply the native disguise");
    UseMeetingItem(owner, Pheromone, TagSpell, mosshide);
    Require(owner.GetQuestObjectiveData(Axis, MosshideTag) == 1 && owner.GetQuestStatus(Axis) == QUEST_STATUS_COMPLETE &&
        visitor.GetQuestStatus(Axis) == QUEST_STATUS_NONE && Spoke(leader, 1) && !Spoke(leader, 0),
        "A real disguised tag did not grant immediate credit and the opening sniff");

    auto tickMeeting = [&](unsigned duration)
    {
        for (unsigned elapsed = 0; elapsed < duration; elapsed += 50)
        {
            mosshide.Update(50);
            leader.Update(50);
            second.Update(50);
            third.Update(50);
            kobold.Update(50);
            map.MoveAllCreaturesInMoveList();
        }
    };
    tickMeeting(15000);
    Require(Spoke(mosshide, 0) && Spoke(mosshide, 2) && Spoke(third, 2) && Spoke(second, 3) &&
        !Spoke(leader, 0) && !leader.getVictim() && mosshide.IsAlive(),
        "The two replies and gnoll dialogue did not precede the leader's roar");
    tickMeeting(2300);
    Require(Spoke(leader, 0) && !Spoke(second, 0) && !Spoke(third, 0) && mosshide.IsAlive() && !leader.getVictim(),
        "The original leader did not roar exactly before the attack");
    tickMeeting(2400);
    Require(leader.getVictim() == &mosshide && mosshide.IsAlive() && Spoke(mosshide, 3),
        "The leader did not use its native delayed attack against Mosshide");
    tickMeeting(1500);
    Require(second.getVictim() == &mosshide && third.getVictim() == &mosshide,
        "The supporting Bluegill did not join the original attack");
    tickMeeting(3500);
    Require(!mosshide.IsAlive() && Spoke(mosshide, 1) && Spoke(kobold, 0) && owner.GetQuestObjectiveData(Axis, MosshideTag) == 1,
        "The native scripted fight did not finish with its original dialogue and single credit");
    std::cout << "PASS Axis actual item packets, active/disguise admission, immediate credit, lead dialogue, timed attacks and original endpoint\n";
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
        auto login = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
        Require(login.substr(login.find_last_of(';') + 1) == "fixture_auth", "Expected a disposable realm");
        LoginDatabase.SetConnectionInfo(login, 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0,{}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0,{}}});
        auto quest = LoadAxisInputs();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckAxis(quest);
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
