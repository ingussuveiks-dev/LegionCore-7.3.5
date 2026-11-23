#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GilneasQuestNative.h"
#include "NPCPackets.h"
#include "../../src/server/scripts/EasternKingdoms/hillsbrad_basic_botany.h"

namespace
{
using namespace HillsbradBasicBotany;

Quest const* LoadBotanyInputs()
{
    auto row = WorldDatabase.Query(
#include "NativeBrazieBasicBotany.inc"
    );
    Require(bool(row), "Missing Basic Botany template");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[BasicBotany] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(BasicBotany, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=28733");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == IntroduceBotany,
        "Missing original Basic Botany victory objective");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid=131600");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells WHERE npc_entry=49194");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id IN(91649,91687,91748)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=28733");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=28733");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadNPCSpellClickSpells();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_hillsbrad_basic_botany();
    sScriptMgr->SwapScriptContext(true);
    sScriptDataStore->LoadSpellScriptNames();
    sScriptDataStore->ValidateSpellScripts();
    return quest;
}

struct BotanyGiver : Creature
{
    explicit BotanyGiver(::Map& map) : Creature(false)
    {
        CreatureData const* data = sObjectMgr->GetCreatureData(GiverSpawn);
        Require(data, "Brazie's original placement is missing");
        sObjectMgr->RemoveCreatureFromGrid(GiverSpawn, data);
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(GiverSpawn, &map, true) && IsInWorld(), "Brazie could not enter the real map");
    }

    ~BotanyGiver()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

Creature* CurrentBoard(Player& player)
{
    GuidList* boards = player.GetSummonList(Board);
    Require(boards->size() <= 1, "Duplicate personal board");
    return boards->empty() ? nullptr : player.GetMap()->GetCreature(boards->front());
}

std::list<Creature*> Children(Creature& board, uint32 entry)
{
    std::list<Creature*> result;
    board.GetCreatureListWithEntryInGrid(result, entry, 100.0f);
    result.remove_if([&](Creature const* actor)
    {
        auto summon = actor->ToTempSummon();
        return !summon || summon->GetSummonerGUID() != board.GetGUID();
    });
    return result;
}

void Advance(::Map& map, LootPlayer& owner, Creature& board, uint32 diff)
{
    Tick(owner, diff);
    board.Update(diff);
    for (uint32 entry : {SolarPower, Spitter, Zombie})
        for (Creature* actor : Children(board, entry))
            actor->Update(diff);
    map.MoveAllCreaturesInMoveList();
}

void CheckBotany(Quest const* quest)
{
    ::Map map(0, 300, 0, DIFFICULTY_NONE);
    BotanyGiver brazie(map);
    Position start = brazie.GetNearPosition(2.0f, 0.0f);
    LootPlayer owner(SupplySession(), map, 2873301, start), other(SupplySession(), map, 2873302, start);
    for (auto [player, race] : {std::make_pair(&owner, RACE_HUMAN), std::make_pair(&other, RACE_ORC)})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(race);
        player->InitDisplayIds();
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE, UNIT_FIELD_MOD_HASTE_REGEN})
            player->SetFloatValue(field, 1.0f);
    }

    owner.CastSpell(&owner, SummonBoard, true);
    Require(!CurrentBoard(owner), "A player without Basic Botany summoned a board");
    owner.RemoveAurasDueToSpell(SummonBoard);
    owner.AddQuestAndCheckCompletion(quest, &brazie);
    Creature* board = CurrentBoard(owner);
    Require(board && board->GetVehicleKit() && board->HasSpell(PlantSpitter), "Native acceptance did not summon the Solar Power board");
    Require(board->GetVehicleKit()->GetVehicleInfo()->ID == 1382 && board->GetVehicleKit()->GetVehicleInfo()->PowerDisplayID[0] == 166,
        "Matching client Solar Power and camera layout was not used");
    for (unsigned i = 0; i < 40 && owner.GetVehicleBase() != board; ++i)
        Advance(map, owner, *board, 50);
    Advance(map, owner, *board, 1000);
    Require(owner.GetVehicleBase() == board && board->GetPower(POWER_ENERGY) == 0 && !other.canSeeOrDetect(board),
        "Native camera seat, starting Solar Power or private board failed");
    auto plots = Children(*board, EmptySpot);
    auto boardAI = GetBoard(board);
    std::list<Creature*> allPlots;
    owner.GetCreatureListWithEntryInGrid(allPlots, EmptySpot, 100.0f);
    std::cout << "CASE plot count " << plots.size() << " playerside " << allPlots.size() << " controller " << bool(boardAI)
        << " rider " << (boardAI && boardAI->Rider() == &owner) << " teleported " << owner.IsBeingTeleported()
        << " quest " << owner.GetQuestStatus(BasicBotany) << " board XY " << board->GetPositionX() << ' ' << board->GetPositionY()
        << " far floor " << map.GetHeight(-52.0f, 220.0f, 60.0f) << " near floor " << map.GetHeight(-108.0f, 220.0f, 60.0f) << '\n';
    Require(plots.size() == Rows * Columns, "Board did not create all 40 usable positions");
    Require(Children(*board, Zombie).empty(), "Tutorial advanced before the first real defense");
    other.AddQuestAndCheckCompletion(quest, &brazie);
    Creature* second = CurrentBoard(other);
    Require(second && second->HasQuestObjectScope() && !owner.canSeeOrDetect(second), "A simultaneous Horde board was not private");
    for (unsigned i = 0; i < 40 && other.GetVehicleBase() != second; ++i)
        Advance(map, other, *second, 50);
    Advance(map, other, *second, 1000);
    std::cout << "CASE Horde seat " << (other.GetVehicleBase() == second) << " aura " << second->HasAura(VEHICLE_SPELL_RIDE_HARDCODED, other.GetGUID())
        << " plots " << Children(*second, EmptySpot).size() << " owner-sees " << other.canSeeOrDetect(second) << '\n';
    Require(other.GetVehicleBase() == second && Children(*second, EmptySpot).size() == Rows * Columns,
        "Simultaneous Horde camera seat or private field failed");
    std::cout << "CASE private Basic Botany camera seat, energy and 40 plots\n";

    auto cast = [&](float x, float y)
    {
        float z = map.GetHeight(x, y, 60.0f);
        return board->CastSpell(x, y, z, PlantSpitter, false);
    };
    float center = FirstRowY - CenterRow * PlotSpacing;
    Require(cast(FirstColumnX, center) != SPELL_CAST_OK, "Planting bypassed the original Solar Power cost");
    GuidUnorderedSet clicked;
    unsigned planted = 0;
    for (unsigned i = 0; i < 180 && owner.GetVehicleBase() == board; ++i)
    {
        for (Creature* orb : Children(*board, SolarPower))
        {
            if (!clicked.insert(orb->GetGUID()).second)
                continue;
            int32 power = board->GetPower(POWER_ENERGY);
            orb->HandleSpellClick(&other);
            Require(board->GetPower(POWER_ENERGY) == power, "A foreign player collected this personal orb");
            orb->HandleSpellClick(&owner);
            Require(board->GetPower(POWER_ENERGY) == std::min(100, power + 5), "A real solar pickup did not replenish five energy");
            orb->HandleSpellClick(&owner);
            Require(board->GetPower(POWER_ENERGY) == std::min(100, power + 5), "A duplicate click gave solar power twice");
        }
        if (planted < 4 && board->GetPower(POWER_ENERGY) >= 20)
        {
            int32 power = board->GetPower(POWER_ENERGY);
            Require(cast(FirstColumnX, FirstRowY + 25.0f) != SPELL_CAST_OK && board->GetPower(POWER_ENERGY) == power,
                "An out-of-lawn cast consumed or created a plant");
            float x = FirstColumnX + planted * PlotSpacing;
            Require(cast(x, center) == SPELL_CAST_OK, "Native Plant Spitter cast failed on an empty plot");
            Require(board->GetPower(POWER_ENERGY) == power - 20, "The plant did not debit its native energy cost");
            ++planted;
            Require(cast(x, center) != SPELL_CAST_OK, "One lawn square accepted more than one plant");
            GameObject* model = board->FindNearestGameObject(SpitterModel, 100.0f);
            Require(model && model->HasQuestObjectScope() && !other.canSeeOrDetect(model) && model->CanCollideWith(&owner) &&
                !model->CanCollideWith(&other), "A plant model was missing or its collision escaped the private field");
        }
        Advance(map, owner, *board, 1000);
    }
    std::cout << "CASE botany solar pickups " << clicked.size() << " plants " << planted << " quest " << owner.GetQuestStatus(BasicBotany)
        << " credit " << owner.GetQuestObjectiveData(BasicBotany, IntroduceBotany) << '\n';
    Require(!owner.GetVehicle() && owner.GetUnitBeingMoved() == &owner && !owner.HasAura(SummonBoard) &&
        owner.GetQuestStatus(BasicBotany) == QUEST_STATUS_COMPLETE && owner.GetQuestObjectiveData(BasicBotany, IntroduceBotany) == 1,
        "Actual defense did not complete and return control");
    Require(!other.GetQuestObjectiveData(BasicBotany, IntroduceBotany), "Another player received Basic Botany progress");
    map.RemoveAllObjectsInRemoveList();
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = BasicBotany;
    reward.QuestGiverGUID = brazie.GetGUID();
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(BasicBotany), "Native turn-in at Brazie failed");

    ObjectGuid secondGuid = second->GetGUID();
    other.ExitVehicle();
    for (unsigned i = 0; i < 30 && map.GetCreature(secondGuid); ++i)
    {
        if (Creature* previous = map.GetCreature(secondGuid); previous && previous->GetVehicleKit())
            Advance(map, other, *previous, 200);
        map.RemoveAllObjectsInRemoveList();
    }
    Require(!map.GetCreature(secondGuid) && !other.GetVehicle() && other.GetQuestStatus(BasicBotany) == QUEST_STATUS_INCOMPLETE,
        "Leaving the board should clean up while keeping an unfinished quest");
    WorldPackets::NPC::Hello greeting{WorldPacket(CMSG_TALK_TO_GOSSIP)};
    greeting.Unit = brazie.GetGUID();
    other.GetSession()->HandleGossipHelloOpcode(greeting);
    auto const& menu = other.PlayerTalkClass->GetGossipMenu();
    Require(menu.GetItem(0) && menu.GetItem(0)->Action == Retry, "Brazie did not give the same active quest a retry option");
    WorldPackets::NPC::GossipSelectOption retry{WorldPacket(CMSG_GOSSIP_SELECT_OPTION)};
    retry.GossipUnit = brazie.GetGUID();
    retry.GossipID = menu.GetMenuId();
    other.GetSession()->HandleGossipSelectOption(retry);
    Creature* restarted = CurrentBoard(other);
    Require(restarted && restarted->GetGUID() != secondGuid && !other.GetQuestObjectiveData(BasicBotany, IntroduceBotany),
        "The retry did not create a new board without inherited credit");
    auto restartedGuid = restarted->GetGUID();
    AbandonSupply(other, BasicBotany);
    map.RemoveAllObjectsInRemoveList();
    Require(!map.GetCreature(restartedGuid) && !other.GetVehicle(), "Cancellation left a personal board or its passenger");
    std::cout << "PASS Basic Botany native acceptance, simultaneous factions, private camera, power, defense, reward, retry and cancellation\n";
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
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Botany checks require isolated characters");
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}});
        auto quest = LoadBotanyInputs();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckBotany(quest);
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
