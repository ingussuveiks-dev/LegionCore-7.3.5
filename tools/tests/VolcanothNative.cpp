#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "ConditionMgr.h"
#include "CreatureTextMgr.h"
#include "GilneasQuestNative.h"
#include "QuestItemObjectUse.h"
#include "SmartScriptMgr.h"
#include "Vehicle.h"

void AddSC_lost_isle();

namespace
{
constexpr uint32 QuestId = Volcanoth::Quest;
constexpr uint32 Preparations = 24954;
constexpr uint32 OldFriends = 25023;
constexpr uint32 Credit = 38868;
constexpr uint32 Boss = 38855;
constexpr uint32 BossSpawn = 242274;
constexpr uint32 BomberSpawn = 242281;
constexpr uint32 CorpseSpawn = 3300110;

struct CombatGroup : Group
{
    CombatGroup(LootPlayer& first, LootPlayer& second) : members{&first, &second}
    {
        this->*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(first.GetGUID().GetCounter(), 0);
        for (LootPlayer* player : members)
        {
            Group::MemberSlot slot;
            slot.Guid = player->GetGUID();
            slot.Class = player->getClass();
            const_cast<Group::MemberSlotList&>(GetMemberSlots()).push_back(slot);
            player->SetGroup(this, 0);
        }
    }

    ~CombatGroup()
    {
        for (LootPlayer* player : members)
            player->SetGroup(nullptr);
    }

    LootPlayer* const members[2];
};

void LoadVolcanothInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeVolcanoth.inc"
    );
    Require(rows && rows->GetRowCount() == 5, "The original Lost Isles quests are absent");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (rows->NextRow());

    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN(24958,25023)");
    Require(addon && addon->GetRowCount() == 2, "Preparations and Sassy's follow-up are absent");
    do
    {
        auto row = addon->Fetch();
        auto quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        quest->prevQuests.push_back(quest->PrevQuestID);
    } while (addon->NextRow());

    auto quest = quests[QuestId];
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=24958");
    Require(objective && objective->GetRowCount() == 1, "The actual Volcanoth objective is absent");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    Require(quest->PrevQuestID == Preparations && quest->SourceItemId == Volcanoth::Bootzooka &&
        quest->GetObjectives()[0].ObjectID == Credit && quest->GetObjectives()[0].Amount == 1 &&
        quests[OldFriends]->PrevQuestID == QuestId && sObjectMgr->GetCreatureTemplate(Boss)->KillCredit[0] == Credit,
        "The authored reward, objective or native kill credit changed");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    auto item = sObjectMgr->GetItemTemplate(Volcanoth::Bootzooka);
    auto fire = sSpellMgr->GetSpellInfo(Volcanoth::Fire);
    Require(item && std::any_of(item->Effects.begin(), item->Effects.end(), [](ItemEffectEntry const* effect)
        { return effect && effect->SpellID == Volcanoth::Fire && !effect->Charges; }) &&
        fire->GetEffect(EFFECT_0)->Effect == SPELL_EFFECT_SCHOOL_DAMAGE && fire->GetEffect(EFFECT_0)->BasePoints == 524,
        "The original reusable Bootzooka's native damage spell changed");
    std::cout << "Bootzooka range " << fire->GetMaxRange(false) << ", missile speed " << fire->GetMisc()->MiscData.Speed << '\n';

    auto& lootTable = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto originalLootQuery = lootTable;
    lootTable += " WHERE Entry=38845";
    auto loadedLoot = (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    lootTable = originalLootQuery;
    Require(loadedLoot == 22, "The original Child corpse loot is not loaded");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(242215,242211,242274,242281,3300110)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_addon AS SELECT * FROM legion_world.creature_addon WHERE guid IN(242215,242211,242274,242281,3300110)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_template_addon AS SELECT * FROM legion_world.creature_template_addon WHERE entry IN(38120,38845,38855,38928,38929)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells WHERE npc_entry=38929");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE vehicle_template_accessory AS SELECT * FROM legion_world.vehicle_template_accessory WHERE EntryOrAura=38929");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text WHERE CreatureID IN(38120,38855,38928)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(24958,25023)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=24958");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE area=4720 AND spell IN(72676,73065)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_target_position AS SELECT * FROM legion_world.spell_target_position WHERE id IN(73135,74103)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 AND entryorguid IN(38855,-242274)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadCreatureTemplateAddons();
    sObjectMgr->LoadCreatureAddons();
    sObjectMgr->LoadNPCSpellClickSpells();
    sObjectMgr->LoadVehicleTemplateAccessories();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sCreatureTextMgr->LoadCreatureTexts();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sSpellMgr->LoadSpellAreas();
    sSpellMgr->LoadSpellTargetPositions();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_lost_isle();
    sScriptMgr->SwapScriptContext(true);
}

struct DatabaseActor : Creature
{
    DatabaseActor(Map& map, uint32 spawn) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "The actual database placement is missing");
        map.LoadGrid(data->posX, data->posY);
        if (!LoadCreatureFromDB(spawn, &map, true) || !IsInWorld())
            throw std::runtime_error("The native creature placement " + std::to_string(spawn) + " did not load into the map");
    }

    ~DatabaseActor()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void AcceptVolcanoth(LootPlayer& player, Creature& hobart)
{
    MoveGilneasPlayer(player, hobart.GetPosition());
#ifdef VOLCANOTH_BASELINE
    player.SetPhaseMask(8192, false); // Permit the downstream wrong-target counterfactual after observing the missing aura.
#endif
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = hobart.GetGUID();
    accept.QuestID = QuestId;
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && player.GetItemCount(Volcanoth::Bootzooka) == 1,
        "The actual Hobart did not offer the quest and give its reusable Bootzooka");
}

SpellCastResult FireBootzooka(LootPlayer& player, Creature& target)
{
    return player.CastSpell(&target, Volcanoth::Fire, false, player.GetItemByEntry(Volcanoth::Bootzooka));
}

void CheckVolcanoth()
{
    for (uint32 spawn : {242215u, 242211u, BossSpawn, BomberSpawn, CorpseSpawn})
        if (auto data = sObjectMgr->GetCreatureData(spawn))
            sObjectMgr->RemoveCreatureFromGrid(spawn, data);

    Map map(648, 606, 0, DIFFICULTY_NONE);
    DatabaseActor hobart(map, 242215), child(map, 242211), boss(map, BossSpawn), bomber(map, BomberSpawn);
    LootPlayer owner(SupplySession(), map, 2495801, hobart.GetPosition()), peer(SupplySession(), map, 2495802, hobart.GetPosition()),
        bystander(SupplySession(), map, 2495803, hobart.GetPosition());
    for (LootPlayer* player : {&owner, &peer, &bystander})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_GOBLIN);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(RACE_GOBLIN);
        player->InitDisplayIds();
        player->GetReputationMgr().LoadFromDB({});
        player->SetLevel(10);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE, UNIT_FIELD_MOD_HASTE_REGEN})
            player->SetFloatValue(field, 1.0f);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        player->SetRewardedQuest(24929);
        player->SetRewardedQuest(Preparations);
        MoveGilneasPlayer(*player, hobart.GetPosition());
#ifdef VOLCANOTH_BASELINE
        Require(!player->HasAura(72676) && !player->InSamePhase(&hobart), "The original inverted end mask no longer hides the actual Hobart");
#else
        Require(player->HasAura(72676) && player->HasPhaseId(182) && player->InSamePhase(&hobart) && !player->InSamePhase(&bomber),
            "The native pre-kill phase or hidden bomber changed");
#endif
    }

    AcceptVolcanoth(owner, hobart);
    Position childApproach = child.GetNearPosition(3.0f, 0.0f);
    childApproach.SetOrientation(childApproach.GetAngle(child.GetPositionX(), child.GetPositionY()));
    MoveGilneasPlayer(owner, childApproach);
#ifdef VOLCANOTH_BASELINE
    owner.SetPhaseMask(8192, false);
#endif
    Require(owner.IsWithinLOSInMap(&child) && owner.InSamePhase(&child), "The original wrong target is unavailable to the fixture");
    auto health = child.GetHealth();
    auto wrong = FireBootzooka(owner, child);
    Tick(owner, 6000);
    std::cout << "Wrong target cast " << uint32(wrong) << ", real child health " << health << " -> " << child.GetHealth()
        << ", original boss phase " << boss.GetPhaseMask() << std::endl;
#ifdef VOLCANOTH_BASELINE
    Require(boss.GetPhaseMask() == 24576 && !sObjectMgr->GetCreatureData(CorpseSpawn) && wrong == SPELL_CAST_OK && child.GetHealth() < health,
        "Original shared-phase boss or the weapon damaging the wrong actor no longer reproduces");
    std::cout << "EXPECTED_BROKEN inverted phase mask hides Hobart, Bootzooka damages an unrelated actor and the living Volcanoth is in both phases\n";
#else
    Require(boss.GetPhaseMask() == 8192 && wrong == SPELL_FAILED_BAD_TARGETS && child.GetHealth() == health,
        "Bootzooka damaged the wrong child or the living boss remains visible after victory");
    DatabaseActor corpse(map, CorpseSpawn);
    Require(corpse.GetPhaseMask() == 16384 && corpse.HasAura(29266) && corpse.HasAura(65734) && corpse.HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE),
        "The actual separate post-kill corpse auras or non-attackable flags are absent");

    AcceptVolcanoth(peer, hobart);
    AcceptVolcanoth(bystander, hobart);
    Position approach = boss.GetNearPosition(5.0f, 0.0f);
    approach.SetOrientation(approach.GetAngle(boss.GetPositionX(), boss.GetPositionY()));
    for (auto player : {&owner, &peer, &bystander})
        MoveGilneasPlayer(*player, approach);
    Require(owner.InSamePhase(&boss) && owner.IsWithinLOSInMap(&boss) && !owner.InSamePhase(&corpse), "The intended target or its guarded phase is unavailable");
    Require(owner.CastSpell(&boss, Volcanoth::Fire, true) != SPELL_CAST_OK, "An itemless raw spell can fire the Bootzooka");
    boss.SetMaxHealth(10000);
    boss.SetHealth(10000);
    Require(FireBootzooka(owner, boss) == SPELL_CAST_OK, "The actual Bootzooka did not fire on Volcanoth");
    Tick(owner, 6000);
    Require(boss.GetHealth() < 10000 && boss.IsAlive() && !owner.GetQuestObjectiveData(QuestId, Credit) && owner.GetItemCount(Volcanoth::Bootzooka),
        "The original item did not damage its real target without premature credit");

    CombatGroup group(owner, peer);
    LootPet pet(owner, map);
    pet.SetLevel(owner.getLevel());
    pet.SetEffectiveLevel(owner.GetEffectiveLevel());
    pet.SetPhaseMask(owner.GetPhaseMask(), false);
    pet.SetPhaseId(owner.GetPhases(), false);
    pet.SetOwnerGUID(ObjectGuid::Empty);
    owner.SetMinion(&pet, true);
    Require(pet.DealDamage(&boss, 1000000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) && boss.isDead(false),
        "Volcanoth did not die to the actual Hunter pet's final blow");
    for (auto player : {&owner, &peer, &bystander})
        Tick(*player, 100);
    owner.SetMinion(&pet, false);
    Require(owner.GetQuestObjectiveData(QuestId, Credit) == 1 && peer.GetQuestObjectiveData(QuestId, Credit) == 1 &&
        !bystander.GetQuestObjectiveData(QuestId, Credit), "Native boss death did not credit the eligible owner/group only");

    MoveGilneasPlayer(owner, bomber.GetPosition());
    Require(owner.HasAura(73065) && owner.HasPhaseId(183) && owner.InSamePhase(&corpse) && owner.InSamePhase(&bomber) &&
        !owner.InSamePhase(&boss) && !bystander.InSamePhase(&bomber), "The native post-kill phase did not expose the corpse and cave handoff");
    Vehicle* kit = bomber.GetVehicleKit();
    Require(kit, "The actual cave bomber did not instantiate its native vehicle kit");
    for (unsigned i = 0; i < 200 && !kit->GetPassenger(0); ++i)
    {
        bomber.Update(10);
        if (Creature* accessory = bomber.FindNearestCreature(38928, 10.0f, true))
            accessory->Unit::Update(10);
    }
    Unit* passenger = kit->GetPassenger(0);
    std::cout << "Sassy entry in seat zero " << (passenger ? passenger->GetEntry() : 0)
        << ", quest receiver " << (passenger && passenger->ToCreature()->hasInvolvedQuest(QuestId))
        << ", accessible " << (passenger && owner.IsWithinLOSInMap(passenger)) << std::endl;
    Require(passenger && passenger->GetEntry() == 38928 && passenger->ToCreature()->hasInvolvedQuest(QuestId) &&
        passenger->ToCreature()->hasQuest(OldFriends) && owner.IsWithinLOSInMap(passenger), "The original seated Sassy is unavailable for handoff");
    Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId);
    uint32 choice = 0;
    if (auto items = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (auto item : *items)
            if (owner.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    if (!choice)
        if (auto items = sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID); items && !items->empty())
            choice = items->front()->ItemID;
    Require(choice && owner.CanRewardQuest(quest, choice, false), "The native reward choice is unavailable");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = QuestId;
    reward.QuestGiverGUID = passenger->GetGUID();
    reward.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(QuestId) && !owner.GetItemCount(Volcanoth::Bootzooka), "The actual Sassy turn-in did not take the Bootzooka");
    Require(owner.m_homebindMapId == 648 && std::fabs(owner.m_homebindX - 1176.66f) < 0.01f &&
        std::fabs(owner.m_homebindY - 1090.84f) < 0.01f && std::fabs(owner.m_homebindZ - 118.859f) < 0.01f,
        "The native reward spell did not set the actual cave home bind");
    AbandonSupply(bystander, QuestId);
    Require(!bystander.GetItemCount(Volcanoth::Bootzooka), "Abandonment retained the Bootzooka");
    std::cout << "PASS actual Bootzooka damage and target rejection, pet and group boss kill, native corpse and seated Sassy with reward and cleanup\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{648, {}}, {659, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{648, {}}, {659, {}}});
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Volcanoth checks require isolated characters");
        LoadVolcanothInputs();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckVolcanoth();
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
