#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "ConditionMgr.h"
#include "CreatureTextMgr.h"
#include "GilneasQuestNative.h"
#include "ScriptsData.h"
#include "SmartScriptMgr.h"

void AddSC_durotar();

namespace
{
constexpr uint32 QuestId = 25165;
constexpr uint32 Coastline = 25170;
constexpr uint32 Scorpid = 3125;
constexpr uint32 Totem = 39236;
constexpr uint32 ItemId = 52505;
constexpr uint32 PlaceTotem = 73673;
constexpr uint32 Envenom = 73672;
constexpr uint32 BombaySpawn = 80376;
constexpr uint32 ScorpidSpawn = 80388;

struct DurotarPlacement : Creature
{
    DurotarPlacement(Map& map, uint32 spawn) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "An original Durotar spawn is missing");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true) && IsInWorld(), "An original Durotar actor did not load");
    }

    ~DurotarPlacement()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void LoadScorpidInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeScorpidVenom.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "Missing original Bom'bay quests");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (rows->NextRow());

    auto quest = quests[QuestId];
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=25165");
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=25165");
    Require(addon && objective && objective->GetRowCount() == 1, "Missing the actual prerequisite or venom objective");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(quest->PrevQuestID);
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    Require(quest->PrevQuestID == Coastline && quest->SourceItemId == ItemId && quest->GetObjectives()[0].ObjectID == Totem &&
        quest->GetObjectives()[0].Amount == 6, "The original venom objective, prerequisite or provided item changed");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(80376,80388,80404,80324,81614,81262,81606,80391)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_addon AS SELECT * FROM legion_world.creature_addon WHERE guid IN(80376,80388,80404,80324,81614,81262,81606,80391)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_template_addon AS SELECT * FROM legion_world.creature_template_addon WHERE entry IN(3125,39236,10578)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=25165");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=25165");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text WHERE CreatureID=3125");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 AND entryorguid=3125");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=3125");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id IN(73672,73673)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadCreatureTemplateAddons();
    sObjectMgr->LoadCreatureAddons();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sCreatureTextMgr->LoadCreatureTexts();
    sSmartScriptMgr->LoadSmartAIFromDB();
#ifdef SCORPID_VENOM_BASELINE
    sConditionMgr->LoadConditions();
#else
    auto obsolete = WorldDatabase.Query("SELECT COUNT(*) FROM conditions");
    Require(obsolete && obsolete->Fetch()[0].GetUInt64() == 0, "The obsolete proximity-only credit condition is still present");
    sScriptDataStore->LoadSpellScriptNames();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_durotar();
    sScriptMgr->SwapScriptContext(true);
#endif
}

void AcceptAtBombay(LootPlayer& player, Creature& bombay)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_TROLL);
    player.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
    player.setFactionForRace(RACE_TROLL);
    player.InitDisplayIds();
    player.GetReputationMgr().LoadFromDB({});
    player.SetLevel(6);
    player.SetMaxHealth(1000);
    player.SetHealth(1000);
    for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE, UNIT_FIELD_MOD_HASTE_REGEN})
        player.SetFloatValue(field, 1.0f);
    player.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    player.SetRewardedQuest(Coastline);
    MoveGilneasPlayer(player, bombay.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = bombay.GetGUID();
    accept.QuestID = QuestId;
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && player.GetItemCount(ItemId) == 1,
        "The real Bom'bay did not offer the quest with its native totem item");
}

#ifdef SCORPID_VENOM_BASELINE
void CheckScorpidVenom()
{
    for (uint32 spawn : {BombaySpawn, ScorpidSpawn})
        if (auto data = sObjectMgr->GetCreatureData(spawn))
            sObjectMgr->RemoveCreatureFromGrid(spawn, data);
    Map map(1, 609, 0, DIFFICULTY_NONE);
    DurotarPlacement bombay(map, BombaySpawn), scorpid(map, ScorpidSpawn);
    LootPlayer victim(SupplySession(), map, 2516501, bombay.GetPosition()), other(SupplySession(), map, 2516502, bombay.GetPosition());
    for (auto player : {&victim, &other})
    {
        AcceptAtBombay(*player, bombay);
        MoveGilneasPlayer(*player, scorpid.GetNearPosition(2.0f, 0.0f));
    }
    Require(scorpid.GetEntry() == Scorpid && scorpid.AI(), "The original live Scorpid is missing");
    scorpid.AI()->AttackStart(&victim);
    Require(scorpid.getVictim() == &victim, "The original Scorpid could not start combat against the active player");
    auto unprotected = scorpid.CastSpell(&victim, Envenom, true);
    Require(unprotected == SPELL_CAST_OK && !victim.GetQuestObjectiveData(QuestId, Totem),
        "The original locality condition did not reject credit while no totem existed");
    auto result = other.CastSpell(&other, PlaceTotem, false, other.GetItemByEntry(ItemId));
    Tick(other, 1000);
    Require(result == SPELL_CAST_OK && other.FindNearestCreature(Totem, 15.0f, true) &&
        victim.FindNearestCreature(Totem, 15.0f, true), "Could not reproduce the original public extraction totem");
    auto before = victim.GetHealth();
    auto venom = scorpid.CastSpell(&victim, Envenom, true);
    auto credit = victim.GetQuestObjectiveData(QuestId, Totem);
    std::cout << "CASE foreign extraction totem: health " << before << " -> " << victim.GetHealth() << ", credit=" << credit << std::endl;
    Require(venom == SPELL_CAST_OK && credit == 1 && victim.GetHealth() < before && !victim.HasAura(PlaceTotem),
        "The original unrelated totem, false credit or unabsorbed Envenom no longer reproduces");
    std::cout << "EXPECTED_BROKEN actual foreign totem grants a sample to the victim without absorbing the poison\n";
}
#else
struct VenomGroup : Group
{
    VenomGroup(LootPlayer& first, LootPlayer& second) : members{&first, &second}
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

    ~VenomGroup()
    {
        for (auto player : members)
            player->SetGroup(nullptr);
    }

    LootPlayer* const members[2];
};

SpellCastResult UseTotem(LootPlayer& player)
{
    return player.CastSpell(&player, PlaceTotem, false, player.GetItemByEntry(ItemId));
}

void RunEnvenom(Creature& scorpid, Unit& target)
{
    Require(scorpid.CastSpell(&target, Envenom, true) == SPELL_CAST_OK, "Actual Envenom cast failed");
}

void CheckScorpidVenom()
{
    constexpr uint32 spawns[] = {ScorpidSpawn, 80404, 80324, 81614, 81262, 81606, 80391};
    sObjectMgr->RemoveCreatureFromGrid(BombaySpawn, sObjectMgr->GetCreatureData(BombaySpawn));
    for (uint32 spawn : spawns)
        sObjectMgr->RemoveCreatureFromGrid(spawn, sObjectMgr->GetCreatureData(spawn));
    Map map(1, 609, 0, DIFFICULTY_NONE);
    DurotarPlacement bombay(map, BombaySpawn);
    std::vector<std::unique_ptr<DurotarPlacement>> actors;
    for (uint32 spawn : spawns)
        actors.push_back(std::make_unique<DurotarPlacement>(map, spawn));
    auto& first = *actors[0];
    LootPlayer victim(SupplySession(), map, 2516501, bombay.GetPosition()), other(SupplySession(), map, 2516502, bombay.GetPosition()),
        third(SupplySession(), map, 2516503, bombay.GetPosition());
    for (auto player : {&victim, &other, &third})
    {
        AcceptAtBombay(*player, bombay);
        Require(UseTotem(*player) == SPELL_FAILED_BAD_TARGETS, "The totem could be used at Bom'bay without a nearby Clattering Scorpid");
        MoveGilneasPlayer(*player, first.GetNearPosition(2.0f, 0.0f));
    }

    Require(victim.CastSpell(&victim, PlaceTotem, false) == SPELL_FAILED_BAD_TARGETS, "Direct placement without the carried item was allowed");
    first.AI()->AttackStart(&victim);
    RunEnvenom(first, victim);
    Require(!victim.GetQuestObjectiveData(QuestId, Totem), "Envenom awarded a sample before a totem was placed");
    Require(UseTotem(other) == SPELL_CAST_OK, "The original totem item did not deploy for its real owner");
    Tick(other, 100);
    Require(other.GetSummonList(Totem)->size() == 1, "The native summon did not create a single owned totem");
    auto foreignGuid = other.GetSummonList(Totem)->front();
    auto foreign = map.GetCreature(foreignGuid);
    Require(foreign && other.canSeeOrDetect(foreign) && !victim.canSeeOrDetect(foreign), "Foreign totem was not private");
    auto health = victim.GetHealth();
    RunEnvenom(first, victim);
    Require(victim.GetHealth() < health && !victim.GetQuestObjectiveData(QuestId, Totem) && !other.GetQuestObjectiveData(QuestId, Totem),
        "An unrelated player's nearby totem absorbed or credited the venom");

    {
        VenomGroup group(victim, third);
        Require(UseTotem(victim) == SPELL_CAST_OK, "The actual victim could not use the totem on the live Clattering Scorpid");
        Tick(victim, 100);
        health = victim.GetHealth();
        RunEnvenom(first, victim);
        RunEnvenom(first, victim);
        Require(victim.GetHealth() == health && victim.GetQuestObjectiveData(QuestId, Totem) == 1 &&
            third.GetQuestObjectiveData(QuestId, Totem) == 1 && !other.GetQuestObjectiveData(QuestId, Totem),
            "Totem protection, exactly-once sample or native recipient scope changed");
    }

    AbandonSupply(other, QuestId);
    map.RemoveAllObjectsInRemoveList();
    Require(!other.GetItemCount(ItemId) && !map.GetCreature(foreignGuid), "Abandonment retained the totem or the quest item");
    for (std::size_t i = 1; i <= 5; ++i)
    {
        auto& actual = *actors[i];
        MoveGilneasPlayer(victim, actual.GetNearPosition(2.0f, 0.0f));
        // Expiring a totem removes its GUID from the owner's summon list immediately.
        auto activeTotems = *victim.GetSummonList(Totem);
        for (ObjectGuid const& guid : activeTotems)
            if (auto totem = map.GetCreature(guid))
                totem->Update(15000);
        map.RemoveAllObjectsInRemoveList();
        double cooldown = victim.GetSpellCooldownDelay(PlaceTotem);
        Require(cooldown <= 20.0, "The native item cooldown did not resume after the original totem lifetime");
        std::this_thread::sleep_for(std::chrono::milliseconds(uint32(cooldown * 1000) + 75));
        SpellCastResult placement = UseTotem(victim);
        std::cout << "CASE totem deployment for spawn=" << spawns[i] << " result=" << uint32(placement)
            << " cooldown=" << cooldown << " zone=" << victim.GetZoneId() << " line of sight=" << victim.IsWithinLOSInMap(&actual) << std::endl;
        Require(placement == SPELL_CAST_OK, "The reusable totem could not be deployed near the next placed Scorpid");
        Tick(victim, 100);
        auto earlierHealth = victim.GetHealth();
        if (i == 1)
        {
            LootPet pet(victim, map);
            pet.SetLevel(victim.getLevel());
            pet.SetEffectiveLevel(victim.GetEffectiveLevel());
            pet.setFaction(victim.getFaction());
            pet.SetPhaseMask(victim.GetPhaseMask(), false);
            pet.SetPhaseId(victim.GetPhases(), false);
            pet.SetOwnerGUID(ObjectGuid::Empty);
            victim.SetMinion(&pet, true);
            Require(pet.GetCharmerOrOwnerPlayerOrPlayerItself() == &victim, "The actual pet owner is absent from the fixture");
            Require(ObjectAccessor::GetUnit(actual, pet.GetGUID()) == &pet, "The native hit dispatcher cannot resolve the fixture pet");
            actual.AI()->AttackStart(&pet);
            auto petHealth = pet.GetHealth();
            for (unsigned cast = 0; cast < 4 && victim.GetQuestObjectiveData(QuestId, Totem) < i + 1; ++cast)
                RunEnvenom(actual, pet);
            Require(pet.GetHealth() == petHealth, "The actual Hunter pet did not receive its owner's totem protection");
            victim.SetMinion(&pet, false);
        }
        else
            for (unsigned cast = 0; cast < 4 && victim.GetQuestObjectiveData(QuestId, Totem) < i + 1; ++cast)
                RunEnvenom(actual, victim);
        std::cout << "CASE actual Scorpid samples=" << victim.GetQuestObjectiveData(QuestId, Totem) << " after spawn=" << spawns[i] << std::endl;
        Require(victim.GetHealth() == earlierHealth && victim.GetQuestObjectiveData(QuestId, Totem) == int32(i + 1),
            "The native Envenom did not unlock the next sample with its real owner and target");
    }

    Require(victim.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE && victim.GetItemCount(ItemId) == 1 &&
        UseTotem(victim) != SPELL_CAST_OK, "The completed quest admits another use or prematurely removed the reusable item");
    MoveGilneasPlayer(victim, bombay.GetNearPosition(2.0f, 0.0f));
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = QuestId;
    reward.QuestGiverGUID = bombay.GetGUID();
    victim.GetSession()->HandleQuestGiverChooseReward(reward);
    map.RemoveAllObjectsInRemoveList();
    Require(victim.GetQuestRewardStatus(QuestId) && !victim.GetItemCount(ItemId) && victim.GetSummonList(Totem)->empty(),
        "Bom'bay did not remove the quest item and its scoped totems at reward");
    std::cout << "PASS actual Bom'bay, reusable item, own totem, Envenom and pet/group credit, six placed Scorpids and cleanup\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1, {}}});
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Scorpid Venom requires isolated characters");
        LoadScorpidInputs();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<Pet>::SetSize(256);
        GameTime::UpdateGameTimers();
        CheckScorpidVenom();
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
