#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameObjectPackets.h"
#include "QuestItemObjectUse.h"

namespace
{
using namespace LiberationDay;

constexpr uint32 FlankTheForsaken = 24677;
constexpr uint32 Slavedriver = 37701;
constexpr uint32 MinePhase = 16384;

Position ReadPosition(Field* row)
{
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

struct MineActor : Actor
{
    MineActor(Map& map, uint32 entry, uint32 spawn, Position const& position, uint32 phase = MinePhase)
        : Actor(&map, entry, spawn, position)
    {
        m_DBTableGuid = spawn;
        SetPhaseMask(phase, false);
        map.LoadGrid(position.GetPositionX(), position.GetPositionY());
        Require(map.AddToMap(static_cast<Creature*>(this)), "Could not publish an original Emberstone actor");
    }

    MineActor(Map& map, Field* row) : MineActor(map, row[1].GetUInt32(), row[0].GetUInt32(), ReadPosition(row + 2), row[6].GetUInt32()) { }
};

struct BallAndChain : GameObject
{
    BallAndChain(Map& map, Field* row)
    {
        m_DBTableGuid = row[0].GetUInt32();
        Position const position = ReadPosition(row + 1);
        G3D::Quat const rotation(row[5].GetFloat(), row[6].GetFloat(), row[7].GetFloat(), row[8].GetFloat());
        map.LoadGrid(position.GetPositionX(), position.GetPositionY());
        Require(Create(row[0].GetUInt32(), Chain, &map, MinePhase, position, rotation, 255, GO_STATE_READY), "Could not create an original Ball and Chain");
        auto lock = sLockStore.LookupEntry(GetGOInfo()->GetLockId());
        Require(lock && lock->ID == 1851 && lock->Type[0] == LOCK_KEY_ITEM && lock->Index[0] == Key &&
            GetGoType() == GAMEOBJECT_TYPE_GOOBER, "The original native item lock or GO type has changed");
        Require(map.AddToMap(static_cast<GameObject*>(this)), "Could not publish an original Ball and Chain");
        Update(10000);
        if (getLootState() != GO_READY)
        {
            std::cout << "Chain native initial loot state=" << uint32(getLootState()) << " go state=" << uint32(GetGoState())
                << " spawned=" << isSpawned() << " map=" << GetMapId() << " zone=" << GetZoneId() << '\n';
            map.RemoveFromMap(static_cast<GameObject*>(this), false);
            ResetMap();
            Require(false, "The original Ball and Chain did not reach its ready state");
        }
    }

    ~BallAndChain()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

void LoadLiberationInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeLiberationDay.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "The actual Liberation Day and Flank the Forsaken are missing");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new ::Quest(rows->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (rows->NextRow());
    auto quest = quests[LiberationDay::Quest];
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=24575");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_GAMEOBJECT &&
        objective->Fetch()[4].GetUInt32() == Chain && objective->Fetch()[5].GetUInt32() == 5, "Wrong actual chain objective");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=24575");
    Require(addon && addon->Fetch()[4].GetUInt32() == FlankTheForsaken && !quest->SourceItemId &&
        quest->ItemDrop[1] == Key && quest->ItemDropQuantity[1] == 5, "The real prerequisite or key source fields changed");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(quest->PrevQuestID);
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_CAST);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    auto item = sObjectMgr->GetItemTemplate(Key);
    Require(item && std::any_of(item->Effects.begin(), item->Effects.end(), [](ItemEffectEntry const* row)
        { return row && row->SpellID == OpenChain && row->Charges == -1; }), "Wrong native one-use key metadata");
    Require(sBroadcastTextStore.LookupEntry(37238) && sBroadcastTextStore.LookupEntry(37239), "Original freed villager lines missing");
    auto load = [](LootStore& store)
    {
        auto& table = store.*Access(LootTableNameTag{});
        auto original = table;
        table += " WHERE Entry=37701 AND Item=49881";
        auto count = (store.*Access(LoadLootTableTag{}))();
        table = original;
        return count;
    };
#ifndef LIBERATION_DAY_BASELINE
    Require(load(LootTemplates_Creature) == 1 && load(LootTemplates_Pickpocketing) == 1, "The real corpse and pickpocket key records are missing");
#else
    auto missing = WorldDatabase.Query("SELECT COUNT(*) FROM pickpocketing_loot_template WHERE Item=49881");
    Require(missing && !missing->Fetch()[0].GetUInt32(), "Baseline must use the original pickpocket table without the quest key");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=24575");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=24575");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
}

void AcceptLiberation(LootPlayer& player, Creature& lorna)
{
    SupplyMove(player, lorna.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestID = LiberationDay::Quest;
    accept.QuestGiverGUID = lorna.GetGUID();
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
}

void ClickChain(LootPlayer& player, GameObject const& chain)
{
    SupplyMove(player, chain.GetPosition());
    WorldPacket wire(CMSG_GAME_OBJ_USE);
    wire << chain.GetGUID();
    WorldPackets::GameObject::GameObjectUse packet(std::move(wire));
    packet.Read();
    player.GetSession()->HandleGameObjectUse(packet);
}

bool StoreKey(LootPlayer& player, Loot& loot, bool carry)
{
    WorldPackets::Loot::LootResponse response;
    loot.BuildLootResponse(response, &player, ALL_PERMISSION);
    for (auto const& item : response.Items)
    {
        if (item.Loot.ItemID != Key)
            continue;
        Require(item.Quantity == 1, "The actual slavedriver advertised more than one key");
        if (carry)
        {
            uint32 before = player.GetItemCount(Key);
            player.StoreLootItem(item.LootListID - 1, &loot);
            Require(player.GetItemCount(Key) == before + 1, "The native key slot did not store the key");
        }
        return true;
    }
    return false;
}

#ifndef LIBERATION_DAY_BASELINE
void LootMineKeys(Map& map, LootPlayer& owner, LootPlayer& observer)
{
    auto mobs = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature "
        "WHERE id=37701 AND map=654 AND zoneId=4714 ORDER BY guid");
    Require(mobs && mobs->GetRowCount() == 19, "The 19 actual slavedrivers are missing");
    SupplyMove(owner, ReadPosition(mobs->Fetch() + 2));
    LootPet pet(owner, map);
    pet.SetPhaseMask(MinePhase, false);
    pet.SetOwnerGUID(ObjectGuid::Empty);
    owner.SetMinion(&pet, true);
    bool pocket = false, petKey = false;
    do
    {
        MineActor mob(map, mobs->Fetch());
        SupplyMove(owner, mob.GetPosition());
        if (!pocket)
        {
            Loot loot;
            loot.FillLoot(Slavedriver, LootTemplates_Pickpocketing, &owner, true, false, &mob);
            Require(!StoreKey(observer, loot, false), "An inactive player saw a pickpocket quest key");
            pocket = StoreKey(owner, loot, false);
        }
        Unit& attacker = petKey ? static_cast<Unit&>(owner) : static_cast<Unit&>(pet);
        Require(attacker.DealDamage(&mob, 1000000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) && mob.isDead(false),
            "The actual slavedriver did not die from a player or pet final blow");
        Require(!StoreKey(observer, mob.loot, false), "An inactive observer saw a corpse quest key");
        if (StoreKey(owner, mob.loot, true) && &attacker == &pet)
            petKey = true;
    } while ((!petKey || !pocket || owner.GetItemCount(Key) < 5) && mobs->NextRow());
    owner.SetMinion(&pet, false);
    Require(pocket && petKey && owner.GetItemCount(Key) == 5, "The authentic corpse, pickpocket or actual quest inventory receipt failed");
}
#endif

void CheckLiberation()
{
    auto giver = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE id=37783 AND guid=149656 AND map=654");
    auto chains = WorldDatabase.Query("SELECT g.guid,g.position_x,g.position_y,g.position_z,g.orientation,g.rotation0,g.rotation1,g.rotation2,g.rotation3,"
        "c.guid,c.id,c.position_x,c.position_y,c.position_z,c.orientation,c.phaseMask FROM gameobject g LEFT JOIN creature c ON c.map=g.map AND "
        "c.phaseMask=g.phaseMask AND c.id=37694 AND POW(g.position_x-c.position_x,2)+POW(g.position_y-c.position_y,2)+POW(g.position_z-c.position_z,2)<2.25 "
        "WHERE g.id=201775 AND g.map=654 AND g.zoneId=4714 AND g.phaseMask=16384 ORDER BY g.guid");
    Require(giver && chains && chains->GetRowCount() == 18, "The original Lorna or mine chains are missing");
    Map map(654, 587, 0, DIFFICULTY_NONE);
    MineActor lorna(map, giver->Fetch());
    LootPlayer owner(SupplySession(), map, 2457501, lorna.GetPosition()), companion(SupplySession(), map, 2457502, lorna.GetPosition()),
        observer(SupplySession(), map, 2457503, lorna.GetPosition()), remote(SupplySession(), map, 2457504, lorna.GetPosition());
    for (auto player : {&owner, &companion, &observer, &remote})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_WORGEN);
        player->setFactionForRace(RACE_WORGEN);
        player->InitDisplayIds();
        player->SetLevel(15);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        player->SetPhaseMask(MinePhase, false);
    }
    AcceptLiberation(owner, lorna);
    Require(owner.GetQuestStatus(LiberationDay::Quest) == QUEST_STATUS_NONE, "Lorna bypassed Flank the Forsaken");
    owner.SetRewardedQuest(FlankTheForsaken);
    AcceptLiberation(owner, lorna);
    Require(owner.GetQuestStatus(LiberationDay::Quest) == QUEST_STATUS_INCOMPLETE && !owner.GetItemCount(Key),
        "Lorna did not offer the task without a pregranted Slaver's Key");

#ifdef LIBERATION_DAY_BASELINE
    auto first = chains->Fetch();
    Require(!first[9].IsNull(), "The original first chain lacks a captive");
    BallAndChain chain(map, first);
    MineActor captive(map, first + 9);
    ClickChain(owner, chain);
    Require(owner.GetQuestObjectiveData(LiberationDay::Quest, Chain) == 1 && !owner.GetItemCount(Key) && !captive.IsDespawn(),
        "Original keyless credit and unchanged prisoner were not reproduced");
    std::cout << "EXPECTED_BROKEN an original keyless object packet awarded progress without freeing the captive\n";
#else
    {
        auto first = chains->Fetch();
        BallAndChain chain(map, first);
        MineActor captive(map, first + 9);
        ClickChain(owner, chain);
        Require(!chain.ActivateToQuest(&owner) && !owner.GetQuestObjectiveData(LiberationDay::Quest, Chain) && !captive.IsDespawn(),
            "A keyless click freed a prisoner or advanced an objective");
    }
    LootMineKeys(map, owner, observer);
    companion.SetRewardedQuest(FlankTheForsaken);
    AcceptLiberation(companion, lorna);
    remote.SetRewardedQuest(FlankTheForsaken);
    AcceptLiberation(remote, lorna);
    Require(companion.GetQuestStatus(LiberationDay::Quest) == QUEST_STATUS_INCOMPLETE && remote.GetQuestStatus(LiberationDay::Quest) == QUEST_STATUS_INCOMPLETE,
        "The nearby or distant member did not start the actual quest");
    GiveSupply(companion, Key, 1);
    GiveSupply(observer, Key, 1);
    Group group;
    group.*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(2457501, 0);
    for (auto member : {&owner, &companion, &observer, &remote})
    {
        Group::MemberSlot slot;
        slot.Guid = member->GetGUID();
        slot.Class = member->getClass();
        const_cast<Group::MemberSlotList&>(group.GetMemberSlots()).push_back(slot);
        member->SetGroup(&group, 0);
    }

    uint32 paired = 0, empty = 0, freed = 0;
    do
    {
        auto row = chains->Fetch();
        BallAndChain chain(map, row);
        if (row[9].IsNull())
        {
            ++empty;
            SupplyMove(owner, chain.GetPosition());
            Require(!chain.ActivateToQuest(&owner) && owner.CastSpell(&chain, OpenChain, false, owner.GetItemByEntry(Key)) != SPELL_CAST_OK,
                "A chain with no native captive admitted the key");
            continue;
        }
        ++paired;
        MineActor captive(map, row + 9);
        if (freed == 5)
        {
            Require(!chain.ActivateToQuest(&owner), "A completed attempt could use an additional chain");
            continue;
        }
        SupplyMove(companion, chain.GetPosition());
        SupplyMove(observer, chain.GetPosition());
        Require(chain.ActivateToQuest(&owner) && !chain.ActivateToQuest(&observer), "The actual marker did not follow the accepted quest and key");
        ClickChain(owner, chain);
        ClickChain(companion, chain);
        Tick(owner, 5500);
        Tick(companion, 5500);
        ++freed;
        Require(captive.IsDespawn() && chain.GetGoState() == GO_STATE_ACTIVE && !chain.ActivateToQuest(&companion) &&
            owner.GetItemCount(Key) == 5 - freed && companion.GetItemCount(Key) == 1 && observer.GetItemCount(Key) == 1 &&
            owner.GetQuestObjectiveData(LiberationDay::Quest, Chain) == freed && companion.GetQuestObjectiveData(LiberationDay::Quest, Chain) == freed &&
            !observer.GetQuestObjectiveData(LiberationDay::Quest, Chain) && !remote.GetQuestObjectiveData(LiberationDay::Quest, Chain),
            "One real shared opening did not free its captive and consume only one key");
    } while (chains->NextRow());
    for (auto member : {&owner, &companion, &observer, &remote})
        member->SetGroup(nullptr);
    Require(paired == 14 && empty == 4 && freed == 5 && owner.GetQuestStatus(LiberationDay::Quest) == QUEST_STATUS_COMPLETE,
        "Wrong actual chain-pair count or completed objective");
    SupplyMove(owner, lorna.GetPosition());
    GiveSupply(owner, Key, 1);
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = lorna.GetGUID();
    reward.QuestID = LiberationDay::Quest;
    reward.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    AbandonSupply(companion, LiberationDay::Quest);
    Require(owner.GetQuestRewardStatus(LiberationDay::Quest) && !owner.GetItemCount(Key) && !companion.GetItemCount(Key),
        "Reward or abandonment did not remove the spare keys");
    std::cout << "PASS 14 genuine captive pairs, four unpaired chains, native death and pickpocket loot, five group rescues and both cleanup paths\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{654, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{654, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Liberation Day probes require isolated characters");
        LoadLiberationInputs();
        sWorld->setFloatConfig(CONFIG_GROUP_XP_DISTANCE, 100.0f);
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckLiberation();
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
