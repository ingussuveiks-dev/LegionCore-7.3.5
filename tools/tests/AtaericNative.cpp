#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "CreatureTextMgr.h"
#include "TemporarySummon.h"

void AddSC_silverpine_forest();

namespace
{
constexpr uint32 QuestId = 27518;
constexpr uint32 Spawn = 254632;
constexpr uint32 Fiend = 45766;
constexpr uint32 Credit = 45769;

Quest const* LoadAtaericInputs()
{
    auto result = WorldDatabase.Query(
#include "NativeAtaeric.inc"
    );
    Require(result && result->GetRowCount() == 1, "Missing original Chapter III quest");
    auto quest = new Quest(result->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[QuestId] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(QuestId, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=27518");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == Credit && objective->Fetch()[5].GetInt32() == 1, "Unexpected native shield objective");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadPlayerInfo();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    sSpellMgr->LoadSpellProcEvents();
    sSpellMgr->LoadSpellProcs();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid=254632");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text WHERE CreatureID=2120");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id=87249");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sCreatureTextMgr->LoadCreatureTexts();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_silverpine_forest();
    sScriptMgr->SwapScriptContext(true);
    sScriptDataStore->LoadSpellScriptNames();
    sScriptDataStore->ValidateSpellScripts();
    auto effect = sSpellMgr->GetSpellInfo(87249)->GetEffect(EFFECT_1);
    Require(effect->Effect == SPELL_EFFECT_ENERGIZE_PCT && effect->MiscValue == POWER_ENERGY && effect->BasePoints == 5,
        "Build 26972 expulsion power effect changed");
    return quest;
}

struct AtaericActor : Creature
{
    explicit AtaericActor(Map& map) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(Spawn);
        Require(data && data->id == 2120, "Missing original Ataeric placement");
        sObjectMgr->RemoveCreatureFromGrid(Spawn, data);
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(Spawn, &map, true) && IsInWorld() && AI(), "Could not load the original archmage");
    }
    ~AtaericActor()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        ResetMap();
    }
};

WorldSession* MakeSession()
{
    return new WorldSession(0, "Ataeric native probe", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
}

TempSummon* CurrentFiend(AtaericActor& boss)
{
    std::list<Creature*> nearby;
    boss.GetAliveCreatureListWithEntryInGrid(nearby, Fiend, 50.0f);
    for (auto creature : nearby)
        if (auto actor = creature->ToTempSummon(); actor && actor->GetSummonerGUID() == boss.GetGUID())
            return actor;
    return nullptr;
}

TempSummon& SpawnFiend(AtaericActor& boss, LootPlayer& player)
{
    for (unsigned i = 0; i < 150; ++i)
    {
        if (auto fiend = CurrentFiend(boss))
            return *fiend;
        boss.Update(50);
        Tick(player, 50);
    }
    throw std::runtime_error("Native 87253 did not publish an Arcane Fiend");
}

void KillFiend(AtaericActor& boss, LootPlayer& player, Unit& killer)
{
    auto& fiend = SpawnFiend(boss, player);
    if (!fiend.IsWithinDistInMap(&boss, 15.0f) || !fiend.IsWithinLOSInMap(&boss))
    {
        fiend.NearTeleportTo(boss.GetNearPosition(2.0f, 0));
        boss.GetMap()->MoveAllCreaturesInMoveList();
    }
    fiend.LowerPlayerDamageReq(uint32(fiend.GetMaxHealth()));
    killer.Kill(&fiend, false);
    Require(fiend.isDead(false), "The real Arcane Fiend did not die");
    fiend.DespawnOrUnsummon();
    boss.GetMap()->RemoveAllObjectsInRemoveList();
}

void CheckAtaeric(Quest const* quest)
{
    Map map(0, 300, 0, DIFFICULTY_NONE);
    AtaericActor boss(map);
    Position start = boss.GetNearPosition(3.0f, 0);
    LootPlayer owner(MakeSession(), map, 2751801, start), member(MakeSession(), map, 2751802, start),
        visitor(MakeSession(), map, 2751803, start);
    for (auto player : {&owner, &member, &visitor})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_UNDEAD_PLAYER);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(RACE_UNDEAD_PLAYER);
        player->InitDisplayIds();
        player->SetLevel(19);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE, UNIT_FIELD_MOD_HASTE_REGEN})
            player->SetFloatValue(field, 1.0f);
    }
    boss.AI()->MoveInLineOfSight_Safe(&visitor);
    boss.Update(4500);
    Require(boss.HasAura(87251) && !CurrentFiend(boss), "A player without Chapter III started the shield fight");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    boss.AI()->MoveInLineOfSight_Safe(&owner);

    for (unsigned i = 0; i < 150; ++i)
        boss.Update(1000);
    Require(boss.IsAlive() && boss.GetPower(POWER_ENERGY) == 0 && !owner.GetQuestObjectiveData(QuestId, Credit),
        "Time or passive regeneration broke the shield");

    // An actual death outside the shield's range has no progress.
    auto& distant = SpawnFiend(boss, owner);
    Position outside = boss.GetPosition();
    outside.m_positionX += 26.f;
    distant.NearTeleportTo(outside);
    map.MoveAllCreaturesInMoveList();
    Require(!distant.IsWithinDistInMap(&boss, 15.0f), "Fixture did not move the distant fiend out of range");
    owner.Kill(&distant, false);
    Require(boss.GetPower(POWER_ENERGY) == 0, "A distant fiend overloaded the shield");
    distant.DespawnOrUnsummon();
    map.RemoveAllObjectsInRemoveList();

    KillFiend(boss, owner, owner);
    Require(boss.GetPower(POWER_ENERGY) == 5 && boss.IsAlive(), "A real nearby fiend did not apply the native five percent");
    auto receipt = owner.GetQuestCancellationReceipt(QuestId);
    auto slot = owner.FindQuestSlot(QuestId);
    owner.RemoveActiveQuest(QuestId);
    owner.SetQuestSlot(slot, 0);
    Require(receipt && receipt->load(), "Native quest abandon did not cancel the accepted receipt");
    for (unsigned i = 0; i < 8; ++i)
    {
        Tick(owner, 100);
        boss.Update(100);
        map.RemoveAllObjectsInRemoveList();
    }
    std::cout << "Ataeric after abandon energy=" << boss.GetPower(POWER_ENERGY)
        << " fiend=" << (CurrentFiend(boss) != nullptr) << " shield=" << boss.HasAura(87251) << '\n';
    Require(boss.GetPower(POWER_ENERGY) == 0 && !CurrentFiend(boss) && boss.HasAura(87251),
        "Abandonment left a charged shield or an owned fiend");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    member.AddQuestAndCheckCompletion(quest, nullptr);
    visitor.AddQuestAndCheckCompletion(quest, nullptr);

    LootPet pet(owner, map);
    pet.setFaction(owner.getFaction());
    Group group;
    group.*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(27518, 0);
    for (auto player : {&owner, &member})
    {
        Group::MemberSlot memberSlot;
        memberSlot.Guid = player->GetGUID();
        memberSlot.Class = player->getClass();
        const_cast<Group::MemberSlotList&>(group.GetMemberSlots()).push_back(memberSlot);
        player->SetGroup(&group, 0);
    }
    sGroupMgr->AddGroup(&group);
    boss.AI()->MoveInLineOfSight_Safe(&owner);
    KillFiend(boss, owner, visitor);
    Require(boss.GetPower(POWER_ENERGY) == 0, "An unrelated player advanced the accepted encounter");
    for (unsigned i = 1; i <= 20; ++i)
    {
        Unit& killer = i % 3 == 0 ? static_cast<Unit&>(pet) : i % 3 == 1 ? static_cast<Unit&>(owner) : static_cast<Unit&>(member);
        KillFiend(boss, owner, killer);
        if (i != 20)
            Require(boss.IsAlive() && boss.HasAura(87251) && boss.GetPower(POWER_ENERGY) == int32(i * 5) &&
                !owner.GetQuestObjectiveData(QuestId, Credit), "The shield ended before twenty native fiend deaths");
    }
    owner.SetGroup(nullptr);
    member.SetGroup(nullptr);
    sGroupMgr->RemoveGroup(&group);
    Require(!boss.IsAlive() && !boss.HasAura(87251) && owner.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE &&
        member.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE && visitor.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE &&
        !CurrentFiend(boss), "Overload did not finish the shield and reward only its eligible group");
    std::cout << "PASS real Ataeric placement, idle timer, distant death, owner/player/pet fiends, group credit and abandon/retry\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}});
        auto quest = LoadAtaericInputs();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckAtaeric(quest);
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
