#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GilneasQuestNative.h"
#include "NPCPackets.h"
#include "../../src/server/scripts/EasternKingdoms/hillsbrad_basic_botany.h"

namespace
{
using namespace HillsbradBasicBotany;

#include "NativeBrazieStage.inc"

Quest const* LoadBotanyInputs()
{
    auto row = WorldDatabase.Query(
#include "NativeBrazieBasicBotany.inc"
    );
    Require(row && row->GetRowCount() == 2, "Missing a Brazie quest template");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(row->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (row->NextRow());
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN(28733,28617)");
    Require(objective && objective->GetRowCount() == 2, "Missing original Brazie victory objectives");
    do
    {
        auto data = objective->Fetch();
        auto* quest = quests[data[1].GetUInt32()];
        Require(data[4].GetUInt32() == (quest->GetQuestId() == BasicBotany ? IntroduceBotany : FlowerWave), "Original credit changed");
        quest->LoadQuestObjective(data);
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    } while (objective->NextRow());
    auto predecessor = WorldDatabase.Query("SELECT PrevQuestID FROM quest_template_addon WHERE ID=28617");
    Require(predecessor && predecessor->Fetch()[0].GetInt32() == BasicBotany, "Flower Power must follow Basic Botany");
    quests[FlowerPower]->prevQuests.push_back(BasicBotany);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid=131600");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells WHERE npc_entry IN(49194,49604)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id IN(91646,91649,91687,91748,92434,92440,92441)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(28733,28617)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest IN(28733,28617)");
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
    return quests[BasicBotany];
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
    for (uint32 entry : {SolarPower, Spitter, Sunflower, Freezya, FreezyaSeeds, Zombie, Ghoul})
        for (Creature* actor : Children(board, entry))
            actor->Update(diff);
    map.MoveAllCreaturesInMoveList();
}

void CheckFlower(Quest const* quest)
{
    ::Map map(0, 300, 0, DIFFICULTY_NONE);
    BotanyGiver brazie(map);
    LootPlayer owner(SupplySession(), map, 2861701, brazie.GetNearPosition(2.0f, 0.0f));
    LootPlayer visitor(SupplySession(), map, 2861702, brazie.GetNearPosition(2.0f, 0.0f));
    for (auto* player : {&owner, &visitor})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_ORC);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(RACE_ORC);
        player->InitDisplayIds();
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE, UNIT_FIELD_MOD_HASTE_REGEN})
            player->SetFloatValue(field, 1.0f);
    }

    Require(!owner.CanTakeQuest(quest, false), "Flower Power was available before Basic Botany's reward");
    owner.SetRewardedQuest(BasicBotany);
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = brazie.GetGUID();
    accept.QuestID = FlowerPower;
    owner.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Creature* board = CurrentBoard(owner);
    Require(board && board->HasSpell(PlantSunflower) && board->HasSpell(PlantSpitter) && !board->HasSpell(PlantFreezya),
        "Flower Power did not open with only the earned plants");
    for (unsigned i = 0; i < 40 && owner.GetVehicleBase() != board; ++i)
        Advance(map, owner, *board, 50);
    Advance(map, owner, *board, 1000);
    Require(owner.GetVehicleBase() == board && Children(*board, EmptySpot).size() == Rows * Columns, "Flower Power did not seat the player at the shared layout");

    auto cast = [&](uint32 spell, uint8 row, uint8 column)
    {
        float x = FirstColumnX + column * PlotSpacing;
        float y = FirstRowY - row * PlotSpacing;
        return board->CastSpell(x, y, map.GetHeight(x, y, 60.0f), spell, false);
    };
    GuidUnorderedSet clicked;
    unsigned sunflowers = 0;
    bool bonusSun = false, seeds = false, slowed = false, ghoulSeen = false;
    bool sunflowerModel = false, freezyaModel = false;
    std::array<bool, Rows> attackRows{};
    for (unsigned i = 0; i < 230 && owner.GetVehicleBase() == board; ++i)
    {
        for (Creature* orb : Children(*board, SolarPower))
        {
            if (!clicked.insert(orb->GetGUID()).second)
                continue;
            if (std::abs(orb->GetPositionY() - FirstRowY) < 0.5f)
                bonusSun = true;
            orb->HandleSpellClick(&owner);
        }
        if (sunflowers < 3 && board->GetPower(POWER_ENERGY) >= 10)
        {
            auto energy = board->GetPower(POWER_ENERGY);
            Require(cast(PlantSunflower, 0, uint8(sunflowers)) == SPELL_CAST_OK && board->GetPower(POWER_ENERGY) == energy - 10,
                "Native Sunflower did not consume its ten Solar Power");
            ++sunflowers;
        }
        auto filled = [&](uint8 row, uint8 column, uint32 entry)
        {
            for (Creature* plant : Children(*board, entry))
                if (plant->IsAlive() && std::abs(plant->GetPositionX() - (FirstColumnX + column * PlotSpacing)) < 0.5f &&
                    std::abs(plant->GetPositionY() - (FirstRowY - row * PlotSpacing)) < 0.5f)
                    return true;
            return false;
        };
        for (uint8 column : {0, 1})
            for (uint8 row : {2, 1, 3})
                if (board->GetPower(POWER_ENERGY) >= 20 && !filled(row, column, Spitter))
                    cast(PlantSpitter, row, column);
        for (uint8 row : {2, 1, 3})
            if (seeds && board->GetPower(POWER_ENERGY) >= 20 && !filled(row, 2, Freezya))
                cast(PlantFreezya, row, 2);
        auto verifyModel = [&](uint32 entry, bool& checked)
        {
            if (checked)
                return;
            if (GameObject* model = board->FindNearestGameObject(entry, 100.0f))
            {
                Require(model->HasQuestObjectScope() && model->CanCollideWith(&owner) && !model->CanCollideWith(&visitor) &&
                    !visitor.canSeeOrDetect(model), "A Flower Power plant escaped its owner's visible and solid board");
                checked = true;
            }
        };
        verifyModel(SunflowerModel, sunflowerModel);
        verifyModel(FreezyaModel, freezyaModel);
        for (Creature* bag : Children(*board, FreezyaSeeds))
        {
            if (seeds)
                break;
            Require(!board->HasSpell(PlantFreezya), "Freezya appeared before its real seed pickup");
            bag->HandleSpellClick(&visitor);
            Require(!board->HasSpell(PlantFreezya), "A visitor unlocked Freezya for the owner");
            bag->HandleSpellClick(&owner);
            Require(board->HasSpell(PlantFreezya), "The native seed click did not unlock Freezya");
            seeds = true;
        }
        for (uint32 entry : {Zombie, Ghoul})
            for (Creature* enemy : Children(*board, entry))
            {
                uint8 row = uint8(std::lround((FirstRowY - enemy->GetPositionY()) / PlotSpacing));
                Require(row > 0 && row < Rows - 1, "Flower Power attackers entered a safe outer lane");
                attackRows[row] = true;
                ghoulSeen = ghoulSeen || entry == Ghoul;
                slowed = slowed || enemy->HasAura(FreezyaBlast);
            }
        Advance(map, owner, *board, 1000);
    }
    std::cout << "CASE flower solar " << clicked.size() << " bonus " << bonusSun << " seeds " << seeds << " ghoul " << ghoulSeen
        << " slow " << slowed << " credit " << owner.GetQuestObjectiveData(FlowerPower, FlowerWave) << '\n';
    Require(bonusSun && seeds && ghoulSeen && slowed && sunflowerModel && freezyaModel && attackRows[1] && attackRows[2] && attackRows[3],
        "Flower Power did not exercise all lanes, Sunflower bonus, genuine seeds and the native slow");
    Require(owner.GetQuestStatus(FlowerPower) == QUEST_STATUS_COMPLETE && !owner.GetVehicle() &&
        owner.GetQuestObjectiveData(FlowerPower, FlowerWave) == 1 && !visitor.GetQuestObjectiveData(FlowerPower, FlowerWave),
        "Flower Power did not finish on real enemy deaths for the actual owner");
    map.RemoveAllObjectsInRemoveList();
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = FlowerPower;
    reward.QuestGiverGUID = brazie.GetGUID();
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(FlowerPower), "Brazie did not accept Flower Power after the real fight");
    std::cout << "PASS Flower Power native prerequisite, 3 lanes, Sunflower production, seed unlock, Freezya slow, clear and reward\n";
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
        if (CheckFlowerStage)
            CheckFlower(sQuestDataStore->GetQuestTemplate(FlowerPower));
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
