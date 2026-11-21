#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "CreatureTextMgr.h"
#include "ScriptSystem.h"
#include "ScriptedEscortAI.h"

void AddSC_lost_isle();

namespace
{
constexpr uint32 MinerQuest = 14021;
constexpr uint32 Foreman = 35769;
constexpr uint32 Miner = 35813;
constexpr uint32 Cart = 35814;
constexpr uint32 Credit = 35816;
constexpr uint32 Ore = 195622;

Quest const* LoadMinerQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeMinerTroubles.inc"
    );
    Require(bool(row), "Missing Miner Troubles template");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[MinerQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(MinerQuest, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=14021");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == Credit &&
        objective->Fetch()[5].GetUInt32() == 1, "Original escort objective missing");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sScriptSystemMgr->LoadScriptWaypoints();
    sCreatureTextMgr->LoadCreatureTexts();
    auto const& path = sScriptSystemMgr->GetPointMoveList(Miner);
    Require(path.size() == 26 && path.back().uiPointId == 26, "Original 26-waypoint route missing");
    auto summonsMiner = sSpellMgr->GetSpellInfo(68059);
    auto summonsCart = sSpellMgr->GetSpellInfo(68064);
    Require(summonsMiner && summonsCart && summonsMiner->GetEffect(EFFECT_0)->MiscValueB == 2261 &&
        summonsCart->GetEffect(EFFECT_0)->MiscValueB == 2262, "Original Legion summon properties changed");
    return quest;
}

struct ForemanActor : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(sScriptMgr->GetCreatureAI(this)), "Linked foreman AI could not initialize");
    }
};

struct TestOre : GameObject
{
    ~TestOre()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

Creature* FindMiner(Map& map, LootPlayer& owner)
{
    auto guids = owner.GetSummonList(Miner);
    Require(guids->size() <= 1, "The owner has duplicate miners");
    return guids->empty() ? nullptr : map.GetCreature(guids->front());
}

void CheckMiner(Quest const* quest)
{
    auto row = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM creature WHERE id=35769 AND map=648 ORDER BY guid LIMIT 1");
    Require(bool(row), "Original Foreman not on Lost Isles");
    auto fields = row->Fetch();
    Position point{fields[0].GetFloat(),fields[1].GetFloat(),fields[2].GetFloat(),fields[3].GetFloat()};
    Map map(648, 300, 0, DIFFICULTY_NONE);
    LootPlayer first(SupplySession(), map, 14021, point), second(SupplySession(), map, 14022, point);
    ForemanActor foreman(&map, Foreman, 44951, point);
    Require(map.AddToMap(static_cast<Creature*>(&foreman)), "Could not publish original foreman");
    foreman.AI()->OnStartQuest(&first, quest);
    Require(!FindMiner(map, first), "An unaccepted quest got a miner");
    for (auto player : {&first, &second})
    {
        player->AddQuestAndCheckCompletion(quest, nullptr);
        foreman.AI()->OnStartQuest(player, quest);
        foreman.AI()->OnStartQuest(player, quest);
    }
    auto a = FindMiner(map, first);
    auto b = FindMiner(map, second);
    Require(a && b && a != b && a->IsInPersonnalVisibilityList(first.GetGUID()) &&
        !a->IsInPersonnalVisibilityList(second.GetGUID()) && b->IsInPersonnalVisibilityList(second.GetGUID()) &&
        !b->IsInPersonnalVisibilityList(first.GetGUID()), "The shared foreman did not create separate personal actors");

    for (auto miner : {a, b})
    {
        for (unsigned i = 0; i < 500 && miner->GetSummonList(Cart)->empty(); ++i)
            miner->Unit::Update(10);
        auto carts = miner->GetSummonList(Cart);
        Require(carts->size() == 1, "Native cart spell did not produce exactly one cart");
        auto cart = map.GetCreature(carts->front());
        auto scope = miner->GetQuestObjectScope().lock();
        Require(cart && scope && cart->GetQuestObjectScope().lock() == scope, "Cart did not inherit the actual quest scope");
    }

    auto rock = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM gameobject WHERE guid=101227 AND id=195622 AND map=648");
    Require(bool(rock), "Original ore beside waypoint 10 is missing");
    auto pos = rock->Fetch();
    Position orePoint{pos[0].GetFloat(),pos[1].GetFloat(),pos[2].GetFloat(),pos[3].GetFloat()};
    TestOre ore;
    Require(ore.Create(101227, Ore, &map, 1, orePoint, G3D::Quat(0,0,0,1), 255, GO_STATE_READY) &&
        map.AddToMap(static_cast<GameObject*>(&ore)),
        "Could not publish the native ore fixture");
    ore.Update(10);
    auto oreState = ore.getLootState();
    Require(ore.isSpawned() && oreState != GO_JUST_DEACTIVATED, "Native ore is unavailable before mining");
    SupplyMove(first, orePoint);
    SupplyMove(second, orePoint);
    for (auto miner : {a, b})
    {
        map.CreatureRelocation(miner, orePoint.GetPositionX(), orePoint.GetPositionY(), orePoint.GetPositionZ(), orePoint.GetOrientation());
        map.MoveAllCreaturesInMoveList();
        auto escort = dynamic_cast<npc_escortAI*>(miner->AI());
        Require(escort && escort->HasEscortState(STATE_ESCORT_ESCORTING), "Miner did not start the original escort");
        escort->WaypointReached(10);
        Require(escort->HasEscortState(STATE_ESCORT_PAUSED), "Miner could not find the shared native ore");
        escort->MovementInform(POINT_MOTION_TYPE, 1000);
        escort->UpdateAI(10000);
        Require(!escort->HasEscortState(STATE_ESCORT_PAUSED), "Miner stayed paused after its real ore event");
        Require(ore.isSpawned() && ore.getLootState() == oreState, "One miner's mining animation removed the world ore");
    }

    ObjectGuid originalA = a->GetGUID();
    AbandonSupply(first, MinerQuest);
    map.RemoveAllObjectsInRemoveList();
    Require(!map.GetCreature(originalA) && !b->IsQuestObjectExpired(), "Abandon did not cancel only the owner's miner and cart");
    SupplyMove(first, point);
    first.AddQuestAndCheckCompletion(quest, nullptr);
    foreman.AI()->OnStartQuest(&first, quest);
    a = FindMiner(map, first);
    Require(a && a->GetGUID() != originalA, "A fresh quest attempt could not start a new miner");
    dynamic_cast<npc_escortAI*>(b->AI())->WaypointReached(23);
    Require(second.CanRewardQuest(quest, false) && !first.GetQuestObjectiveData(MinerQuest, Credit),
        "The original arrival stage did not credit just its own player");
    ObjectGuid finalA = a->GetGUID();
    ObjectGuid finalB = b->GetGUID();
    a->AI()->JustDied(nullptr);
    b->AI()->JustDied(nullptr);
    map.RemoveAllObjectsInRemoveList();
    Require(!map.GetCreature(finalA) && !map.GetCreature(finalB) && first.GetQuestStatus(MinerQuest) == QUEST_STATUS_FAILED &&
        second.CanRewardQuest(quest, false), "Miner death left a stale escort or revoked credit from a completed escort");
    AbandonSupply(first, MinerQuest);
    AbandonSupply(second, MinerQuest);
    map.RemoveAllObjectsInRemoveList();
    std::cout << "PASS native shared foreman, two private miners/carts, shared ore retained, abandon/retry, death and scoped waypoint credit\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{648, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{648, {}}});
        LoadGameObjectModelList(argv[2]);
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_lost_isle();
        sScriptMgr->SwapScriptContext(true);
        auto quest = LoadMinerQuest();
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckMiner(quest);
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
