#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"

#include "MotionMaster.h"
#include "SpellPackets.h"
#include "TemporarySummon.h"
#include "Vehicle.h"

void AddSC_tirisfal_glades();

namespace
{
constexpr uint32 Recruitment = 26800;
constexpr uint32 Darnell = 49337;
constexpr uint32 Corpse = 49340;
constexpr uint32 SummonDarnell = 91938;
constexpr uint32 RideDarnell = 46598;
constexpr uint32 Spawns[] = {134112, 134118, 134126, 134129, 134203, 134213};

Quest const* LoadRecruitment()
{
    auto rows = WorldDatabase.Query(
#include "NativeRecruitment.inc"
    );
    Require(rows && rows->GetRowCount() == 1, "Missing the original Recruitment quest");
    auto quest = new Quest(rows->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Recruitment] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Recruitment, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=26800");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == Corpse &&
        objective->Fetch()[5].GetUInt32() == 6, "The native six-body objective differs");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(134109,134112,134118,134126,134129,134203,134213,134218)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells WHERE npc_entry=49340");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=49340");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE ScriptName='spell_summon_darnel_q26800'");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE spell=91938");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadNPCSpellClickSpells();
    sConditionMgr->LoadConditions();
    sScriptDataStore->LoadSpellScriptNames();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_tirisfal_glades();
    sScriptMgr->SwapScriptContext(true);
    sScriptDataStore->ValidateSpellScripts();
    sSpellMgr->LoadSpellAreas();
    return quest;
}

struct PlacedActor : Creature
{
    PlacedActor(Map& map, uint32 guid) : Creature(false)
    {
        auto spawn = sObjectMgr->GetCreatureData(guid);
        Require(spawn, "Missing the original Deathknell spawn");
        map.LoadGrid(spawn->posX, spawn->posY);
        Require(LoadCreatureFromDB(guid, &map, true) && IsInWorld() && GetAreaId() == 154 && AI(),
            "Could not load original Deathknell actor");
    }

    ~PlacedActor()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        if (FindMap())
            ResetMap();
    }
};

WorldSession* RecruitmentSession()
{
    return new WorldSession(0, "Recruitment native probe", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
}

void Click(LootPlayer& player, Creature& corpse)
{
    WorldPackets::Spells::SpellClick packet{WorldPacket(CMSG_SPELL_CLICK)};
    packet.SpellClickUnitGuid = corpse.GetGUID();
    player.GetSession()->HandleSpellClick(packet);
}

void MoveTo(LootPlayer& player, Creature& actor)
{
    Position pos = actor.GetNearPosition(1.0f, 0.0f);
    player.GetMap()->PlayerRelocation(&player, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
}

Creature& CallDarnell(LootPlayer& player)
{
    auto result = player.CastSpell(&player, SummonDarnell, true);
    Require(result == SPELL_CAST_OK, Trinity::StringFormat("Original Darnell summon failed with %u in zone %u area %u",
        unsigned(result), player.GetZoneId(), player.GetAreaId()).c_str());
    for (auto const& guid : *player.GetSummonList(Darnell))
        if (Creature* darnell = ObjectAccessor::GetCreature(player, guid))
            if (darnell->IsAlive() && !darnell->IsDespawn())
            {
                Require(darnell->GetVehicleKit() && darnell->GetVehicleKit()->GetVehicleInfo()->ID == 1392 &&
                    darnell->GetVehicleKit()->GetAvailableSeatCount() == 6, "Expected six native cargo seats");
                return *darnell;
            }
    throw std::runtime_error("Original summon is missing from the owner roster");
}

TempSummon* FindCopy(LootPlayer& player, Creature& source)
{
    std::list<Creature*> copies;
    player.GetCreatureListWithEntryInGrid(copies, Corpse, 12.0f);
    for (Creature* actor : copies)
        if (auto copy = actor->ToTempSummon())
            if (!copy->IsDespawn() && copy->GetSummonerGUID() == source.GetGUID())
                return copy;
    return nullptr;
}

void CheckRecruitment(Quest const* quest)
{
    for (uint32 id : {134109u, 134112u, 134118u, 134126u, 134129u, 134203u, 134213u, 134218u})
        if (auto spawn = sObjectMgr->GetCreatureData(id))
            sObjectMgr->RemoveCreatureFromGrid(id, spawn);

    Map map(0, 300, 0, DIFFICULTY_NONE);
    PlacedActor first(map, Spawns[0]);
    LootPlayer owner(RecruitmentSession(), map, 2680001, first.GetNearPosition(1.0f, 0.0f));
    LootPlayer other(RecruitmentSession(), map, 2680002, first.GetNearPosition(1.0f, 0.0f));
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_UNDEAD_PLAYER);
        player->setFactionForRace(RACE_UNDEAD_PLAYER);
        player->UpdateZone(player->GetZoneId(), player->GetAreaId());
        player->AddQuestAndCheckCompletion(quest, nullptr);
        // Hold the normal area update so each player starts the click test
        // without a carrier, including when acceptance already cast the aura.
        player->RemoveAurasDueToSpell(SummonDarnell);
        GuidList summons = *player->GetSummonList(Darnell);
        for (ObjectGuid const& guid : summons)
            if (Creature* darnell = ObjectAccessor::GetCreature(*player, guid))
            {
                darnell->DespawnOrUnsummon(1);
                darnell->Unit::Update(150);
            }
        map.RemoveAllObjectsInRemoveList();
        Require(player->GetSummonList(Darnell)->empty(), "The fixture retained a carrier before the missing-carrier test");
    }
    auto credit = [&] { return owner.GetQuestObjectiveData(quest, quest->GetObjectives()[0].StorageIndex); };
    auto areas = sSpellMgr->GetSpellAreaMapBounds(SummonDarnell);
    Require(areas.first != areas.second, "Missing the original Deathknell summon aura row");
    Click(owner, first);
    Require(!credit() && !first.IsDespawn(), "No owned Darnell should leave the body available");
#ifdef RECRUITMENT_BASELINE
    Require(!areas.first->second.IsFitToRequirements(&owner, 6454, 154) &&
        owner.CastSpell(&owner, SummonDarnell, true) != SPELL_CAST_OK, "Original aura unexpectedly admits an active owner");
    // Model the carrier being present again with its native summon properties
    // so the unrelated area-admission failure cannot hide the click lock.
    Require(map.SummonCreature(Darnell, owner.GetPosition(), sSummonPropertiesStore.LookupEntry(2919),
        0, &owner, ObjectGuid::Empty, SummonDarnell), "Could not publish the native Darnell vehicle");
    Click(owner, first);
    Require(!credit() && !FindCopy(owner, first), "Original missing-carrier click did not lock the shared body");
    std::cout << "EXPECTED_BROKEN Recruitment area aura rejects active owners and a click without Darnell locks the corpse\n";
#else
    Creature& darnell = CallDarnell(owner);
    Require(areas.first->second.IsFitToRequirements(&owner, 6454, 154) &&
        !areas.first->second.IsFitToRequirements(&owner, 6454, 153) && darnell.HasQuestObjectScope() &&
        darnell.GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE,
        "Darnell did not follow the active owner in the corrected area scope");
    Require(!FindCopy(owner, first) && other.GetSummonList(Darnell)->empty(), "The fixture started with an unintended pickup or second carrier");
    Click(other, first);
    Require(!credit() && !FindCopy(owner, first), "A second player used the owner's Darnell");
    Click(owner, first);
    auto failed = FindCopy(owner, first);
    Require(failed && !credit() && !first.IsDespawn(), "The initial spell request already awarded credit");
    darnell.RemoveAurasDueToSpell(RideDarnell);
    failed->Update(3100);
    Require(!credit() && !first.IsDespawn(), "Aborted boarding consumed the source or granted credit");
    map.RemoveAllObjectsInRemoveList();

    auto collect = [&](Creature& source, int32 expected)
    {
        MoveTo(owner, source);
        map.CreatureRelocation(&darnell, owner.GetPositionX(), owner.GetPositionY(), owner.GetPositionZ(), owner.GetOrientation());
        map.MoveAllCreaturesInMoveList();
        Click(owner, source);
        auto copy = FindCopy(owner, source);
        Require(copy && copy->HasQuestObjectScope() && copy->GetQuestObjectScope().lock() == darnell.GetQuestObjectScope().lock() &&
            credit() == expected - 1 && !source.IsDespawn(), "Pickup issued credit before boarding or lacked accepted scope");
        Click(owner, source);
        Require(FindCopy(owner, source) == copy && credit() == expected - 1, "Repeated click created another pickup");
        copy->Unit::Update(50);
        Require(copy->GetVehicle() == darnell.GetVehicleKit() && credit() == expected && source.IsDespawn(),
            "Real boarding did not confirm exactly one body");
    };
    collect(first, 1);
    std::vector<std::unique_ptr<PlacedActor>> bodies;
    for (uint32 index = 1; index < std::size(Spawns); ++index)
    {
        bodies.emplace_back(std::make_unique<PlacedActor>(map, Spawns[index]));
        collect(*bodies.back(), index + 1);
    }
    Require(!other.GetQuestObjectiveData(quest, quest->GetObjectives()[0].StorageIndex) &&
        owner.GetQuestStatus(Recruitment) == QUEST_STATUS_COMPLETE && areas.first->second.IsFitToRequirements(&owner, 6454, 154) &&
        darnell.GetVehicleKit()->GetAvailableSeatCount() == 0,
        "Six owner-only pickups did not complete the objective or reserve all native seats");
    PlacedActor saltain(map, 134109);
    saltain.AI()->OnQuestReward(&owner, quest);
    Require(darnell.GetVehicleKit()->GetAvailableSeatCount() == 6, "Saltain did not unload all six seats");
    owner.RemoveAurasDueToSpell(SummonDarnell);
    Require(darnell.IsDespawn(), "Removing the Recruitment aura left the scoped Darnell alive");
    darnell.Unit::Update(150);
    map.RemoveAllObjectsInRemoveList();

    PlacedActor retryBody(map, 134218);
    MoveTo(other, retryBody);
    auto& originalCarrier = CallDarnell(other);
    auto originalCarrierGuid = originalCarrier.GetGUID();
    Click(other, retryBody);
    auto cancelledCopy = FindCopy(other, retryBody);
    Require(cancelledCopy && !retryBody.IsDespawn(), "Could not prepare abandonment during boarding");
    auto cancelledCopyGuid = cancelledCopy->GetGUID();
    auto slot = other.FindQuestSlot(Recruitment);
    other.RemoveActiveQuest(Recruitment);
    other.SetQuestSlot(slot, 0);
    other.RemoveAurasDueToSpell(SummonDarnell);
    map.RemoveAllObjectsInRemoveList();
    Require(!retryBody.IsDespawn() && !areas.first->second.IsFitToRequirements(&other, 6454, 154) &&
        !ObjectAccessor::GetCreature(other, originalCarrierGuid) && !ObjectAccessor::GetCreature(other, cancelledCopyGuid),
        "Abandoning left actors or consumed the original source");
    other.AddQuestAndCheckCompletion(quest, nullptr);
    auto& freshCarrier = CallDarnell(other);
    Click(other, retryBody);
    auto freshCopy = FindCopy(other, retryBody);
    Require(freshCopy, "New acceptance could not collect the body cancelled by an earlier attempt");
    freshCopy->Unit::Update(50);
    Require(freshCopy->GetVehicle() == freshCarrier.GetVehicleKit() &&
        other.GetQuestObjectiveData(quest, quest->GetObjectives()[0].StorageIndex) == 1,
        "A fresh accepted attempt did not receive one confirmed body");
    other.RemoveActiveQuest(Recruitment);
    map.RemoveAllObjectsInRemoveList();
    std::cout << "PASS Recruitment retry, failed and real boarding, six private cargo seats, turn-in cleanup, aura removal, abandon and retake\n";
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
        auto login = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
        Require(login.substr(login.find_last_of(';') + 1) == "fixture_auth", "Expected a disposable realm");
        LoginDatabase.SetConnectionInfo(login, 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Expected disposable characters");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto fixture = CharacterDatabase.Query("SELECT COUNT(*) FROM characters");
        Require(fixture && !fixture->Fetch()[0].GetUInt32(), "Expected an empty character fixture");
        Quest const* quest = LoadRecruitment();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckRecruitment(quest);
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
