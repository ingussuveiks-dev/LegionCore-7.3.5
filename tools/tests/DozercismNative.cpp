#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "CreatureTextMgr.h"
#include "Dozercism.h"
#include "SmartAI.h"

struct DbSpawnTag { using Type = uint64 Creature::*; friend Type Access(DbSpawnTag); };
template struct FixtureAccess<DbSpawnTag, &Creature::m_DBTableGuid>;

namespace
{
constexpr uint32 Clubnik = 35088;
constexpr uint32 Prerequisite = 14209;

struct PlacedActor : Actor
{
    PlacedActor(Map& map, uint32 entry, Field* fields) :
        Actor(&map, entry, fields[0].GetUInt32(), {fields[1].GetFloat(), fields[2].GetFloat(), fields[3].GetFloat(), fields[4].GetFloat()})
    {
        this->*Access(DbSpawnTag{}) = fields[0].GetUInt64();
    }

    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(GetEntry() == Dozercism::Dozer ? static_cast<CreatureAI*>(new SmartAI(this)) : new NullCreatureAI(this)),
            "Placed quest actor AI could not load");
    }
};

struct RitualParty : Group
{
    ~RitualParty()
    {
        if (Registered)
            sGroupMgr->RemoveGroup(this);
    }

    bool Registered = false;
};

void Shadow(char const* table, char const* filter)
{
    std::string name(table);
    WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE scoped_" + name + " AS SELECT * FROM " + name + " WHERE " + filter).c_str());
    WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE " + name + " AS SELECT * FROM scoped_" + name).c_str());
}

std::pair<Quest const*, Quest const*> LoadQuests()
{
    auto templates = WorldDatabase.Query(
#include "NativeDozercism.inc"
    );
    Require(templates && templates->GetRowCount() == 2, "Missing Dozercism or Gunk in the Trunk");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(templates->Fetch());
        uint32 id = quest->GetQuestId();
        quests[id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(id, quest);
    } while (templates->NextRow());
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN(14209,14423) ORDER BY QuestID,StorageIndex");
    Require(objectives && objectives->GetRowCount() == 3, "Missing the original two quests' three objectives");
    do
    {
        auto fields = objectives->Fetch();
        Quest* quest = const_cast<Quest*>(quests[fields[1].GetUInt32()]);
        quest->LoadQuestObjective(fields);
    } while (objectives->NextRow());
    auto addons = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN(14209,14423) ORDER BY ID");
    Require(addons && addons->GetRowCount() == 2 && addons->Fetch()[4].GetInt32() == 14202 && addons->Fetch()[5].GetUInt32() == Dozercism::Quest,
        "The native prerequisite sequence changed");
    const_cast<Quest*>(quests[Prerequisite])->LoadQuestTemplateAddon(addons->Fetch());
    addons->NextRow();
    Require(addons->Fetch()[4].GetInt32() == Prerequisite, "Dozercism lost its predecessor");
    const_cast<Quest*>(quests[Dozercism::Quest])->LoadQuestTemplateAddon(addons->Fetch());
    const_cast<Quest*>(quests[Prerequisite])->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    const_cast<Quest*>(quests[Dozercism::Quest])->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST | QUEST_SPECIAL_FLAGS_DELIVER);
    Quest const* quest = quests[Dozercism::Quest];
    Require(quest->SourceItemId == Dozercism::Flaregun && quest->SourceItemIdCount == 1 && quest->PackageID == 1935 &&
        quest->GetObjectives()[0].ObjectID == Dozercism::Dozer && quest->GetObjectives()[0].Amount == 1 &&
        quest->GetObjectives()[1].ObjectID == Dozercism::Flaregun && quest->GetObjectives()[1].Amount == 1,
        "The real kill, carried flaregun, or reward package changed");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_SmartScripts();
    sScriptMgr->SwapScriptContext(true);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    Shadow("creature_questender", "quest IN(14209,14423)");
    Shadow("creature_template_addon", "entry IN(35526,36552)");
    Shadow("creature_text", "CreatureID IN(35526,36552)");
    Shadow("event_scripts", "id=22429");
    Shadow("npc_spellclick_spells", "npc_entry=35526");
    Shadow("smart_scripts", "source_type=0 AND entryorguid IN(35526,36552)");
#ifdef DOZERCISM_BASELINE
    WorldDatabase.DirectExecute("UPDATE smart_scripts SET target_type=21,target_param1=40 WHERE entryorguid=35526 AND source_type=0 AND id=1 AND action_type=49");
    Shadow("conditions", "SourceTypeOrReferenceId=18 AND SourceGroup=35526");
#else
    Shadow("conditions", "(SourceTypeOrReferenceId IN(13,17) AND SourceEntry=68007) OR (SourceTypeOrReferenceId=18 AND SourceGroup=35526)");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sObjectMgr->LoadCreatureTemplateAddons();
    sCreatureTextMgr->LoadCreatureTexts();
    sScriptDataStore->LoadEventScripts();
    sObjectMgr->LoadNPCSpellClickSpells();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sConditionMgr->LoadConditions();
    return {quest, quests[Prerequisite]};
}

Creature* FindRitual(LootPlayer& player, uint32 entry)
{
    for (auto const& guid : *player.GetSummonList(entry))
        if (Creature* actor = player.GetMap()->GetCreature(guid); actor && !actor->IsDespawn() && !actor->IsQuestObjectExpired())
            return actor;
    return nullptr;
}

SpellCastResult Fire(LootPlayer& player, bool withItem = true)
{
    Item* gun = withItem ? player.GetItemByEntry(Dozercism::Flaregun) : nullptr;
    Require(!withItem || gun, "The actual flaregun was not carried");
    SpellCastTargets targets;
    targets.SetCaster(&player);
    return player.CastSpell(targets, sSpellMgr->GetSpellInfo(Dozercism::Flare), nullptr, TRIGGERED_NONE, gun);
}

void Advance(Map& map, std::initializer_list<LootPlayer*> players, uint32 milliseconds)
{
    for (uint32 elapsed = 0; elapsed < milliseconds; elapsed += 100)
    {
        for (LootPlayer* player : players)
        {
            Tick(*player, 100);
            for (uint32 entry : {Dozercism::Priest, Dozercism::Dozer})
                if (Creature* actor = FindRitual(*player, entry))
                    actor->Update(100);
        }
        map.MoveAllCreaturesInMoveList();
        map.RemoveAllObjectsInRemoveList();
    }
}

void CheckRitual(Quest const* quest, Quest const* prerequisites)
{
    auto placement = WorldDatabase.Query("SELECT c.guid,c.position_x,c.position_y,c.position_z,c.orientation FROM creature c "
        "JOIN creature_queststarter s ON s.id=c.id AND s.quest=14423 JOIN creature_questender e ON e.id=c.id AND e.quest=14423 "
        "WHERE c.guid=250497 AND c.id=35088 AND c.map=1 AND c.zoneId=16");
    auto anchorData = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE guid=260725 AND id=35526 AND map=1 AND zoneId=16");
    Require(placement && anchorData, "Missing the original Custer or neutral dozer placement");
    Map map(1, 500, 0, DIFFICULTY_NONE);
    RitualParty party;
    party.*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(Dozercism::Quest, 0);
    PlacedActor clubnik(map, Clubnik, placement->Fetch()), anchor(map, Dozercism::Dozer, anchorData->Fetch());
    Position firing = anchor.GetNearPosition(5.0f, 0.0f);
    map.LoadGrid(firing.GetPositionX(), firing.GetPositionY());
    Require(map.AddToMap(static_cast<Creature*>(&clubnik)) && map.AddToMap(static_cast<Creature*>(&anchor)), "Placed actors did not load");
    LootPlayer owner(SupplySession(), map, 144230, firing), second(SupplySession(), map, 144231, firing),
        earlier(SupplySession(), map, 144232, firing), member(SupplySession(), map, 144233, firing), stale(SupplySession(), map, 144234, firing);
    for (LootPlayer* player : {&owner, &second, &earlier, &member, &stale})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_GOBLIN);
        player->setFactionForRace(RACE_GOBLIN);
        player->InitDisplayIds();
        player->SetLevel(16);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        player->m_zoneId = 16;
        player->m_areaId = 4830;
    }
    earlier.AddQuestAndCheckCompletion(prerequisites, &clubnik);
    Require(anchor.getFaction() == 1933 && anchor.HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK),
        "The preceding quest needs the neutral spell-click anchor");
    GiveSupply(owner, Dozercism::Flaregun, 1);
#ifdef DOZERCISM_BASELINE
    SpellCastResult baseline = Fire(owner);
    std::cout << "Baseline inactive flare result=" << unsigned(baseline) << '\n';
    Require(baseline == SPELL_CAST_OK, "Old flare did not admit an inactive quest user");
    Tick(owner, 1000);
    Creature* priest = owner.FindNearestCreature(Dozercism::Priest, 45.0f);
    Require(priest && earlier.canSeeOrDetect(priest, true), "Old flare did not publish a public priest");
    for (uint32 elapsed = 0; elapsed < 28000; elapsed += 100)
        priest->Update(100);
    Require(anchor.getFaction() == 14, "Old priest did not turn the preceding quest's actual dozer hostile");
    std::cout << "EXPECTED_BROKEN inactive flare, public priest and hostile shared predecessor dozer\n";
    priest->DespawnOrUnsummon();
    map.RemoveAllObjectsInRemoveList();
#else
    Require(Fire(owner) != SPELL_CAST_OK, "A retained flaregun worked without the quest");
    Require(!FindRitual(owner, Dozercism::Priest), "An inactive flare published an actor");
    owner.DestroyItemCount(Dozercism::Flaregun, 1, true);
    for (LootPlayer* player : {&owner, &second, &member, &stale})
        player->AddQuestAndCheckCompletion(quest, &clubnik);
    for (LootPlayer* player : {&owner, &member, &stale})
    {
        Group::MemberSlot slot;
        slot.Guid = player->GetGUID();
        slot.Class = player->getClass();
        const_cast<Group::MemberSlotList&>(party.GetMemberSlots()).push_back(slot);
        player->SetGroup(&party, 0);
    }
    sGroupMgr->AddGroup(&party);
    party.Registered = true;
    Require(Fire(owner, false) != SPELL_CAST_OK, "An itemless spell bypassed the flaregun");
    owner.KilledMonsterCredit(Dozercism::Dozer, anchor.GetGUID());
    Require(!owner.GetQuestObjectiveData(Dozercism::Quest, Dozercism::Dozer), "The shared anchor was accepted as an encounter kill");
    Position nowhereNear = firing;
    nowhereNear.m_positionX += 35.0f;
    SupplyMove(owner, nowhereNear);
    Require(Fire(owner) != SPELL_CAST_OK, "The flare summoned a priest away from the real bulldozer");
    SupplyMove(owner, firing);

    SpellCastResult ownerFire = Fire(owner);
    SpellCastResult secondFire = Fire(second);
    std::cout << "Active flare results=" << unsigned(ownerFire) << ',' << unsigned(secondFire) << '\n';
    Require(ownerFire == SPELL_CAST_OK && secondFire == SPELL_CAST_OK, "The two owners could not fire their original flareguns");
    Advance(map, {&owner, &second}, 1000);
    Creature* priest = FindRitual(owner, Dozercism::Priest);
    Creature* secondPriest = FindRitual(second, Dozercism::Priest);
    Require(priest && secondPriest && priest != secondPriest && priest->GetQuestObjectScope().lock()->Owner == owner.GetGUID() &&
        owner.canSeeOrDetect(priest, true) && !second.canSeeOrDetect(priest, true) && !earlier.canSeeOrDetect(priest, true),
        "The real event did not publish independent private priests");
    Require(!FindRitual(owner, Dozercism::Dozer) && Fire(owner) != SPELL_CAST_OK, "The fight started before the text chain or a repeat flare was admitted");
    Advance(map, {&owner, &second}, 27000);
    Creature* dozer = FindRitual(owner, Dozercism::Dozer);
    Creature* secondDozer = FindRitual(second, Dozercism::Dozer);
    Require(dozer && secondDozer && dozer != secondDozer && dozer->getFaction() == 14 && dozer->getVictim() == &owner &&
        secondDozer->getVictim() == &second, "Each native priest must activate only its actual owner's dozer");
    Require(!dozer->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK) && !owner.canSeeOrDetect(&anchor, true) &&
        earlier.canSeeOrDetect(&anchor, true) && anchor.getFaction() == 1933 && anchor.getVictim() == nullptr,
        "The fight left a duplicate chassis for its owner or changed the preceding player's anchor");
    Require(anchor.HandleSpellClick(&earlier) && !anchor.HandleSpellClick(&owner) && !dozer->HandleSpellClick(&earlier),
        "The native prior-quest spell-click transferred to a ritual actor or disappeared from the neutral anchor");
    Require(!owner.GetQuestObjectiveData(Dozercism::Quest, Dozercism::Dozer) && owner.GetItemCount(Dozercism::Flaregun) == 1,
        "Starting the encounter granted kill credit or spent the return item");
    uint32 damage = dozer->GetMaxHealth();
    dozer->AI()->DamageTaken(&second, damage, DIRECT_DAMAGE);
    Require(!damage, "An unrelated quest owner could damage another player's dozer");
    {
        LootPet pet(owner, map);
        pet.SetLevel(owner.getLevel());
        pet.SetEffectiveLevel(owner.GetEffectiveLevel());
        pet.SetOwnerGUID(ObjectGuid::Empty); // Pet::LoadPetFromDB publishes the creator before SetMinion establishes ownership.
        owner.SetMinion(&pet, true);
        Require(pet.IsControlledByPlayer(), "The native fixture pet was not registered to its player");
        Require(pet.DealDamage(dozer, 1000000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) && dozer->isDead(false),
            "A genuine scaled pet hit could not kill the owner's encounter");
    }
    AbandonSupply(stale, Dozercism::Quest);
    stale.AddQuestAndCheckCompletion(quest, &clubnik);
    Tick(stale, 150);
    Tick(owner, 150);
    Tick(member, 150);
    std::cout << "Ritual death=" << dozer->isDead(false) << " present=" << (map.GetCreature(dozer->GetGUID()) == dozer) << '\n';
    for (LootPlayer* player : {&owner, &member, &stale, &second})
        std::cout << "Ritual avatar=" << player->GetGUIDLow() << " status=" << unsigned(player->GetQuestStatus(Dozercism::Quest))
            << " kill=" << player->GetQuestObjectiveData(Dozercism::Quest, Dozercism::Dozer)
            << " carried=" << player->GetItemCount(Dozercism::Flaregun) << " groupRange=" << player->IsAtGroupRewardDistance(dozer) << '\n';
    Require(dozer->isDead(false) && owner.GetQuestStatus(Dozercism::Quest) == QUEST_STATUS_COMPLETE &&
        member.GetQuestStatus(Dozercism::Quest) == QUEST_STATUS_COMPLETE && !stale.GetQuestObjectiveData(Dozercism::Quest, Dozercism::Dozer) &&
        !second.GetQuestObjectiveData(Dozercism::Quest, Dozercism::Dozer), "Native pet death credited the wrong attempt or missed an eligible party member");
    Advance(map, {&owner, &second}, 17000);
    Require(!FindRitual(owner, Dozercism::Priest) && secondDozer->IsAlive(), "The real death did not finish only its own priest's outro");

    AbandonSupply(second, Dozercism::Quest);
    map.RemoveAllObjectsInRemoveList();
    Require(!second.GetItemCount(Dozercism::Flaregun) && !FindRitual(second, Dozercism::Priest) && !FindRitual(second, Dozercism::Dozer) &&
        second.canSeeOrDetect(&anchor, true), "Abandonment left a quest item, ritual actors or hid the shared actor");
    second.AddQuestAndCheckCompletion(quest, &clubnik);
    Require(Fire(second) == SPELL_CAST_OK, "A newly accepted attempt could not retry");
    Advance(map, {&second}, 200);
    AbandonSupply(second, Dozercism::Quest);
    map.RemoveAllObjectsInRemoveList();

    uint32 choice = 0;
    if (auto entries = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (auto reward : *entries)
            if (owner.CanSelectQuestPackageItem(reward))
            {
                choice = reward->ItemID;
                break;
            }
    Require(choice && owner.CanRewardQuest(quest, choice, false), "The native 1935 class reward was unavailable");
    SupplyMove(owner, clubnik.GetPosition());
    Require(clubnik.hasInvolvedQuest(Dozercism::Quest) && owner.CanInteractWithQuestGiver(&clubnik), "Custer was not a reachable native receiver");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = clubnik.GetGUID();
    reward.QuestID = Dozercism::Quest;
    reward.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    map.RemoveAllObjectsInRemoveList();
    std::cout << "Ritual reward status=" << owner.IsQuestRewarded(Dozercism::Quest) << " item=" << choice
        << " earned=" << owner.GetItemCount(choice) << " gun=" << owner.GetItemCount(Dozercism::Flaregun)
        << " visibleAnchor=" << owner.canSeeOrDetect(&anchor, true) << '\n';
    Require(owner.IsQuestRewarded(Dozercism::Quest) && !owner.GetItemCount(Dozercism::Flaregun) && owner.GetItemCount(choice) == 1 &&
        !FindRitual(owner, Dozercism::Dozer) && owner.canSeeOrDetect(&anchor, true), "Native reward did not return the flaregun, give the actual package or clean up the fight");
    std::cout << "PASS 14423 native flare, authored speech/DATA, independent private fights, preceding quest anchor, pet/group credit, retry and reward\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Dozercism requires the disposable character schema");
        auto [quest, predecessor] = LoadQuests();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckRitual(quest, predecessor);
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
