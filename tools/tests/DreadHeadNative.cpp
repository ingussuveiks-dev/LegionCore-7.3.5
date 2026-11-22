#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "CreatureTextMgr.h"
#include "GroupMgr.h"

void AddSC_ashenvale();

namespace
{
constexpr uint32 DreadHead = 13842;
constexpr uint32 Durak = 44414;
constexpr uint32 Persuaded = 44416;
constexpr uint32 Transform = 82969;

Quest const* LoadDreadHead()
{
    auto row = WorldDatabase.Query(
#include "NativeDreadHead.inc"
    );
    Require(bool(row), "Missing Dread Head Redemption template");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[DreadHead] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(DreadHead, quest);
    auto objectives = WorldDatabase.Query(
        "SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=13842 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 2, "Missing native Durak objectives");
    for (uint32 entry : {Persuaded, Durak})
    {
        auto fields = objectives->Fetch();
        Require(fields[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER && fields[4].GetUInt32() == entry &&
            fields[5].GetUInt32() == 1, "The original Durak objective changed");
        quest->LoadQuestObjective(fields);
        objectives->NextRow();
    }
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    sCreatureTextMgr->LoadCreatureTexts();
    sObjectMgr->LoadItemTemplates();
    auto& lootTable = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto originalTable = lootTable;
    lootTable += " WHERE Entry=44414";
    auto loaded = (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    lootTable = originalTable;
    Require(loaded > 0, "Original Durak loot is missing");
    auto spell = sSpellMgr->GetSpellInfo(Transform);
    Require(spell, "The native Durak transform is missing");
    bool matched = false;
    for (uint8 index = 0; index < MAX_SPELL_EFFECTS; ++index)
        if (auto effect = spell->GetEffect(index))
            matched |= effect->ApplyAuraName == SPELL_AURA_TRANSFORM && effect->MiscValue == Persuaded;
    Require(matched, "The native Durak transform changed");
    return quest;
}

struct DurakActor : Actor
{
    using Actor::Actor;

    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        auto ai = sScriptMgr->GetCreatureAI(this);
        Require(ai && AIM_Initialize(ai), "Could not initialize the linked Durak controller");
    }
};

struct DurakPet : LootPet
{
    DurakPet(LootPlayer& owner, Map& map) : LootPet(owner, map) { m_ControlledByPlayer = true; }
};

struct DurakParty : Group
{
    DurakParty(Player& owner, Player& member) : members{&owner, &member}
    {
        this->*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(DreadHead, 0);
        for (auto player : members)
        {
            MemberSlot slot;
            slot.Guid = player->GetGUID();
            slot.Class = player->getClass();
            const_cast<MemberSlotList&>(GetMemberSlots()).push_back(slot);
            player->SetGroup(this, 0);
        }
        sGroupMgr->AddGroup(this);
    }

    ~DurakParty()
    {
        for (auto player : members)
            player->SetGroup(nullptr);
        sGroupMgr->RemoveGroup(this);
    }

    Player* members[2];
};

void FinishDialogue(DurakActor& actor, Player const& owner)
{
    auto ai = actor.AI();
    ai->UpdateAI(5000);
    ai->UpdateAI(6000);
    ai->UpdateAI(5999);
    Require(!actor.HasAura(Transform) && actor.getFaction() == 125, "Durak attacked before the final line");
    ai->UpdateAI(1);
    Require(actor.HasAura(Transform, actor.GetGUID()) && actor.getFaction() == 14 &&
        actor.GetEntry() == Durak && actor.GetLootRecipient() == &owner && actor.getVictim() == &owner,
        "Native reveal/combat/tap transition failed");
    Require(owner.GetQuestObjectiveData(DreadHead, Persuaded) == 1 &&
        !owner.GetQuestObjectiveData(DreadHead, Durak), "Persuasion must be acknowledged before the actual kill");
}

void CheckDreadHead(Quest const* quest)
{
    auto spawn = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=44414 AND map=1 AND zoneId=331");
    Require(spawn && spawn->GetRowCount() == 1, "Original Durak placement missing");
    auto data = spawn->Fetch();
    Position point{data[1].GetFloat(),data[2].GetFloat(),data[3].GetFloat(),data[4].GetFloat()};
    auto definition = sObjectMgr->GetCreatureTemplate(Durak);
    Require(definition && definition->AIName.empty() && definition->ScriptID == sObjectMgr->GetScriptId("npc_ashenvale_durak"), "Durak SQL/AI binding differs");
    Map map(1, 300, 0, DIFFICULTY_NONE);
    LootPlayer owner(SupplySession(), map, 13842, point);
    LootPlayer member(SupplySession(), map, 13843, point);
    LootPlayer visitor(SupplySession(), map, 13844, point);
    for (auto player : {&owner, &member, &visitor})
    {
        player->setFactionForRace(RACE_ORC);
        player->SetLevel(19);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    DurakActor actor(&map, Durak, data[0].GetUInt32(), point);
    Require(map.AddToMap(static_cast<Creature*>(&actor)), "Original Durak could not be published");
    auto ai = actor.AI();
    ai->sGossipSelect(&owner, Durak, 0);
    Require(actor.HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP), "A player without the quest started the encounter");
    for (auto player : {&owner, &member, &visitor})
        player->AddQuestAndCheckCompletion(quest, nullptr);
    ai->sGossipSelect(&owner, Durak, 1);
    Require(actor.HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP), "An unrelated menu option started the encounter");
    ai->sGossipSelect(&owner, Durak, 0);
    ai->UpdateAI(5000);
    AbandonSupply(owner, DreadHead);
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    ai->UpdateAI(12000);
    Require(actor.HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP) && !owner.GetQuestObjectiveData(DreadHead, Persuaded),
        "A canceled dialogue continued into the next acceptance");

    DurakParty party(owner, member);
    ai->sGossipSelect(&owner, Durak, 0);
    ai->sGossipSelect(&visitor, Durak, 0);
    FinishDialogue(actor, owner);
    Require(member.GetQuestObjectiveData(DreadHead, Persuaded) == 1 && !visitor.GetQuestObjectiveData(DreadHead, Persuaded),
        "Persuasion failed to distinguish a nearby party member and a bystander");
    visitor.DealDamage(&actor, uint32(actor.GetMaxHealth()));
    Require(actor.IsAlive(), "A bystander ended the party's attempt");
    DurakPet pet(member, map);
    pet.SetLevel(19);
    pet.DealDamage(&actor, uint32(actor.GetMaxHealth()));
    Tick(owner, 100);
    Tick(member, 100);
    Tick(visitor, 100);
    std::cout << "CASE native death=" << actor.isDead(false) << " owner death credit="
        << owner.GetQuestObjectiveData(DreadHead, Durak) << " member death credit="
        << member.GetQuestObjectiveData(DreadHead, Durak) << " owner status=" << owner.GetQuestStatus(DreadHead)
        << " member status=" << member.GetQuestStatus(DreadHead) << std::endl;
    Require(actor.isDead(false) && owner.CanRewardQuest(quest, false) && member.CanRewardQuest(quest, false) &&
        !visitor.GetQuestObjectiveData(DreadHead, Durak), "Native party pet kill did not complete both eligible players");
    Require(owner.GetQuestObjectiveData(DreadHead, Durak) == 1 && member.GetQuestObjectiveData(DreadHead, Durak) == 1,
        "Durak death was not recorded exactly once");
    std::cout << "PASS real dialogue/reveal, new quest attempt, shared tap, bystander denial and native party pet kill\n";
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
        LoadGameObjectModelList(argv[2]);
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_ashenvale();
        sScriptMgr->SwapScriptContext(true);
        auto quest = LoadDreadHead();
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckDreadHead(quest);
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
