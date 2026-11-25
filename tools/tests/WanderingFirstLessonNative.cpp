#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GossipDef.h"
#include "GameObjectPackets.h"
#include "ItemPackets.h"
#include "MiscPackets.h"
#include "SpellPackets.h"
#include "AreaTriggerData.h"
#include "CreatureTextMgr.h"
#include "ScriptedEscortAI.h"
#include "ScriptSystem.h"
#include "SmartAI.h"
#include "SpellMgr.h"

void AddSC_WanderingIsland();

namespace
{
constexpr uint32 Area = 5834;
constexpr uint32 GiverSpawn = 243564;
constexpr uint32 RackSpawn = 102286;
constexpr uint32 RackEntry = 210005;
constexpr uint32 SandyFist = 29406;
constexpr uint32 TrainingTarget = 53714;
constexpr uint32 TargetSpawns[] = {243565, 243566, 243567, 243568, 243569};
constexpr uint32 StifledPride = 29524;
constexpr uint32 TraineeCredit = 54586;
constexpr std::pair<uint32, uint32> Trainees[] = {
    {243610, 54586}, {243660, 54587}, {243608, 65470}, {243645, 65471}
};
constexpr uint32 BurningScroll = 29408;
constexpr uint32 TempleGiverSpawn = 243685;
constexpr uint32 MastersFlame = 80212;
constexpr uint32 FlameCredit = 59591;
constexpr uint32 BurnCredit = 59570;
constexpr uint32 Edict = 210986;
constexpr uint32 EdictSpawn = 102466;
constexpr uint32 AcademyQuest = 29409;
constexpr uint32 AcademyAura = 56267;
constexpr uint32 DiscipleGiverSpawn = 243732;
constexpr uint32 JaominSpawn = 243731;
constexpr uint32 Jaomin = 54611;
constexpr uint32 Aysa = 29410;
constexpr uint32 LorvoSpawn = 243772;
constexpr uint32 MissingDriver = 29419;
constexpr uint32 ImportantItems = 29424;
constexpr uint32 DriverApproach = 6958;
constexpr uint32 DriverSpawn = 243816;
constexpr uint32 Driver = 54855;
constexpr uint32 PrivateDriver = 56503;
constexpr uint32 AysaNext = 29414;
constexpr uint32 AysaNextSpawn = 243862;
constexpr uint32 AysaMeditation = 59642;
constexpr uint32 AysaMeditationSpawn = 243869;
constexpr uint32 AysaMeditationCredit = 54856;
constexpr uint32 AysaMeditationBar = 116421;
constexpr uint32 AysaWave = 59637;
constexpr uint32 AysaMeditationTurnIn = 243875;
constexpr uint32 JiOfTheHuojin = 29522;
constexpr uint32 WayOfTheHuojin = 29417;
constexpr uint32 KindlingTheFire = 29418;
constexpr uint32 JiSpawn = 243824;
constexpr uint32 FeFengAttacker = 54131;
constexpr uint32 FeFengSpawns[] = {243820, 243827, 243835, 243836, 243837, 243844, 243845};
constexpr uint32 Scamp = 54130;
constexpr uint32 StolenSupplies = 72071;
constexpr uint32 SupplyScamps[] = {243766, 243788, 243790, 244114, 244115, 243764};
struct Lesson
{
    uint8 playerClass;
    uint32 quest;
    uint32 mainHand;
    uint32 offHand;
};
constexpr Lesson Lessons[] = {
    {CLASS_MONK, 30027, 77279, 77278}, {CLASS_MAGE, 30033, 76390, 76392},
    {CLASS_HUNTER, 30034, 73211, 0}, {CLASS_PRIEST, 30035, 73207, 76393},
    {CLASS_ROGUE, 30036, 73208, 73212}, {CLASS_SHAMAN, 30037, 76391, 73213},
    {CLASS_WARRIOR, 30038, 73210, 0}
};

void LoadLessonInputs()
{
    auto templates = WorldDatabase.Query(
#include "NativeWanderingLesson.inc"
    );
    Require(templates && templates->GetRowCount() == 25, "Missing a lesson, old introduction or academy quest");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(templates->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (templates->NextRow());
    auto addons = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon "
        "WHERE ID BETWEEN 30033 AND 30045 OR ID IN (30027,29406,29408,29409,29410,29414,29417,29418,29419,29424,29522,29524)");
    Require(addons && addons->GetRowCount() == 25, "Missing the actual lesson prerequisites and class masks");
    do
    {
        auto row = addons->Fetch();
        auto quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        if (quest->PrevQuestID)
            quest->prevQuests.push_back(quest->PrevQuestID);
        auto const next = uint32(std::abs(quest->NextQuestID));
        if (next && next < quests.size() && quests[next])
            quests[next]->prevQuests.push_back(quest->NextQuestID < 0 ? -int32(quest->Id) : int32(quest->Id));
        if (quest->NextQuestIdChain && quest->NextQuestIdChain < quests.size() && quests[quest->NextQuestIdChain])
            quests[quest->NextQuestIdChain]->prevChainQuests.push_back(quest->Id);
    } while (addons->NextRow());
    auto objectives = WorldDatabase.Query(
        "SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives "
        "WHERE QuestID IN (29406,29408,29409,29410,29414,29417,29418,29419,29424,29522,29524,30027) OR QuestID BETWEEN 30033 AND 30038");
    Require(bool(objectives), "Missing the original equipment or training objectives");
    do
    {
        auto row = objectives->Fetch();
        auto quest = quests[row[1].GetUInt32()];
        quest->LoadQuestObjective(row);
        if (row[2].GetUInt8() == QUEST_OBJECTIVE_ITEM)
            quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
        else
            quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    } while (objectives->NextRow());

    auto oldAreaStarts = WorldDatabase.Query("SELECT COUNT(*) FROM area_queststart WHERE id=5834");
    Require(oldAreaStarts && !oldAreaStarts->Fetch()[0].GetUInt32(), "Expected the migrated area to have no automatic introduction");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN "
        "(243564,243565,243566,243567,243568,243569,243610,243660,243608,243645,243685,243731,243732,243772,"
        "243766,243788,243790,244114,244115,243764,243816,243862,243869,243875,"
        "243824,243820,243827,243835,243836,243837,243844,243845)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter "
        "WHERE quest IN (29406,29408,29409,29410,29414,29417,29418,29419,29424,29522,29524,30027) OR quest BETWEEN 30033 AND 30038");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender "
        "WHERE quest IN (29406,29408,29409,29410,29414,29417,29418,29419,29424,29522,29524,30027) OR quest BETWEEN 30033 AND 30045");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts "
        "WHERE source_type=0 AND entryorguid IN (53566,54586,54587,65470,65471)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text "
        "WHERE CreatureID IN (53566,54586,54587,54611,54856,56503,59642,65470,65471)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE areatrigger_scripts AS SELECT * FROM legion_world.areatrigger_scripts WHERE entry=6958");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE script_waypoint AS SELECT * FROM legion_world.script_waypoint WHERE entry=56503");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE spell IN (56267,114610) AND area=5834");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells WHERE npc_entry IN (53566,59591)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE "
        "(SourceTypeOrReferenceId=13 AND SourceEntry IN (114580,114746,115930)) OR "
        "(SourceTypeOrReferenceId=18 AND SourceGroup IN (53566,59591)) OR (SourceTypeOrReferenceId=22 AND SourceEntry=53566)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto& lootTable = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto name = lootTable;
    lootTable += " WHERE Entry=40856";
    auto loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    lootTable = name;
    Require(loaded && LootTemplates_Gameobject.GetLootFor(40856), "The original weapon rack loot is unavailable");
    auto& creatureLootTable = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto const creatureLootName = creatureLootTable;
    creatureLootTable += " WHERE Entry IN (54130,54131)";
    auto const creatureLootLoaded = (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    creatureLootTable = creatureLootName;
    Require(creatureLootLoaded && LootTemplates_Creature.GetLootFor(Scamp) && LootTemplates_Creature.GetLootFor(FeFengAttacker),
        "The scamps' or attackers' original loot is unavailable");
    sQuestDataStore->LoadGameObjectForQuests();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSpellMgr->LoadSpellAreas();
    sObjectMgr->LoadNPCSpellClickSpells();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sConditionMgr->LoadConditions();
    sCreatureTextMgr->LoadCreatureTexts();
    sAreaTriggerDataStore->LoadAreaTriggerScripts();
    sScriptSystemMgr->LoadScriptWaypoints();
}

struct LessonCreature : Creature
{
    LessonCreature(Map& map, uint32 spawn, uint32 entry) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data && data->id == entry && data->mapid == 860, "Missing an original training-grounds spawn");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true) && IsInWorld(), "Could not publish an original training-grounds NPC");
    }

    ~LessonCreature()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void SetStudent(LootPlayer& player, uint8 race, uint8 playerClass)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
    player.SetByteValue(UNIT_FIELD_BYTES_0, 1, playerClass);
    player.setFactionForRace(race);
    player.SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
    player.InitDisplayIds();
    player.GetReputationMgr().LoadFromDB({});
    player.SetLevel(1);
    player.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 400);
}

void Accept(LootPlayer& player, Creature& giver, uint32 quest)
{
    WorldPackets::Quest::QuestGiverAcceptQuest request{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    request.QuestGiverGUID = giver.GetGUID();
    request.QuestID = quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(request);
}

void Reward(LootPlayer& player, Creature& giver, uint32 quest, uint32 choice = 0)
{
    WorldPackets::Quest::QuestGiverChooseReward request{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    request.QuestGiverGUID = giver.GetGUID();
    request.QuestID = quest;
    request.ItemChoiceID = choice;
    player.GetSession()->HandleQuestGiverChooseReward(request);
}

void Equip(LootPlayer& player, uint32 itemId)
{
    Item* item = player.GetItemByEntry(itemId);
    Require(item && !item->IsEquipped(), "Expected a newly looted weapon in the backpack");
    uint16 destination = 0;
    InventoryResult result = player.CanEquipItem(NULL_SLOT, destination, item, !item->IsBag());
    if (result != EQUIP_ERR_OK)
        std::cout << "CASE weapon equip item=" << itemId << " result=" << uint32(result)
                  << " dual-wield=" << player.CanDualWield() << " skill=" << player.GetSkillValue(item->GetSkill()) << '\n';
    WorldPackets::Item::AutoEquipItem request{WorldPacket(CMSG_AUTO_EQUIP_ITEM)};
    request.PackSlot = item->GetBagSlot();
    request.Slot = item->GetSlot();
    request.Inv.Items.push_back({request.PackSlot, request.Slot});
    player.GetSession()->HandleAutoEquipItem(request);
    Require(item->IsEquipped(), "Native equipment packet refused the training weapon");
    Tick(player, 100);
}

void CheckAcademyAura(LootPlayer& player, uint32 lesson)
{
    auto const rules = sSpellMgr->GetSpellAreaMapBounds(AcademyAura);
    auto const rule = std::find_if(rules.first, rules.second, [lesson](auto const& entry) {
        return entry.second.areaId == Area && entry.second.questStart == lesson &&
            entry.second.questEnd == AcademyQuest && entry.second.autocast;
    });
    Require(rule != rules.second && !rule->second.IsFitToRequirements(&player, 5736, Area),
        "Academy aura was missing or eligible before the later quest");

    // Start at the later area's checkpoint to exercise the real aura predicate.
    player.AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(AcademyQuest), nullptr);
    Require(player.GetQuestStatus(AcademyQuest) == QUEST_STATUS_INCOMPLETE, "The academy checkpoint is not active");
    Require(rule->second.IsFitToRequirements(&player, 5736, Area) &&
        !rule->second.IsFitToRequirements(&player, 5736, Area + 1), "The lesson reward did not satisfy the academy aura in its area");
    AbandonSupply(player, AcademyQuest);
    Require(!rule->second.IsFitToRequirements(&player, 5736, Area), "The academy aura still matched after abandoning the later quest");
}

void CheckEquipment(LootPlayer& player, Creature& giver, Lesson const& lesson)
{
    auto row = WorldDatabase.PQuery(
        "SELECT spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE guid=%u AND id=%u AND map=860 AND areaId=5834", RackSpawn, RackEntry);
    Require(bool(row), "Missing the original weapon rack placement");
    auto fields = row->Fetch();
    Placement placement{RackEntry, RackSpawn, fields[0].GetUInt32(),
        {fields[1].GetFloat(), fields[2].GetFloat(), fields[3].GetFloat(), fields[4].GetFloat()},
        {fields[5].GetFloat(), fields[6].GetFloat(), fields[7].GetFloat(), fields[8].GetFloat()}};
    player.GetMap()->LoadGrid(placement.position.GetPositionX(), placement.position.GetPositionY());
    Supply rack(*player.GetMap(), placement);
    SupplyMove(player, placement.position);
    Require(rack.ActivateToQuest(&player), "An eligible student cannot identify the original rack");
    OpenSupply(player, rack);
    Require(player.GetLootGUID() == rack.GetGUID(), "The original rack did not open native loot");
    WorldPackets::Loot::LootResponse view;
    rack.loot.BuildLootResponse(view, &player, ALL_PERMISSION);
    Require(view.Items.size() == (lesson.offHand ? 2u : 1u), "The rack offered the wrong number of training items");
    WorldPackets::Loot::AutoStoreLootItem request{WorldPacket(CMSG_LOOT_ITEM)};
    for (auto const& item : view.Items)
    {
        Require(item.Loot.ItemID == lesson.mainHand || item.Loot.ItemID == lesson.offHand, "The rack offered another lesson's equipment");
        request.Loot.push_back({rack.loot.GetGUID(), item.LootListID});
    }
    player.GetSession()->HandleAutostoreLootItemOpcode(request);
    ReleaseSupply(player, rack);
    Require(player.GetQuestStatus(lesson.quest) == QUEST_STATUS_INCOMPLETE, "Looting alone completed the equipment lesson");
    Equip(player, lesson.mainHand);
    Require(player.GetQuestObjectiveData(lesson.quest, 54139) == 1, "Main-hand equip did not credit its objective");
    if (lesson.offHand)
    {
        Require(player.GetQuestStatus(lesson.quest) == QUEST_STATUS_INCOMPLETE &&
            player.GetQuestObjectiveData(lesson.quest, 57849) == 0, "Main-hand equip also credited the offhand objective");
        Equip(player, lesson.offHand);
        Require(player.GetQuestObjectiveData(lesson.quest, 57849) == 1, "Offhand equip did not credit its objective");
    }
    Require(player.GetQuestStatus(lesson.quest) == QUEST_STATUS_COMPLETE, "Equipping the full set did not complete the lesson");
    SupplyMove(player, giver.GetPosition());
    Reward(player, giver, lesson.quest);
    Require(player.IsQuestRewarded(lesson.quest), "Shang Xi did not record the native lesson reward");
    Require(player.GetQuestStatus(lesson.quest) == QUEST_STATUS_REWARDED, "The lesson reward is not visible to spell_area");
    player.PrepareQuestMenu(giver.GetGUID());
    Require(!player.PlayerTalkClass->GetQuestMenu().HasItem(lesson.quest), "The one-time lesson was offered again");
    Require(player.GetItemByEntry(lesson.mainHand) && (!lesson.offHand || player.GetItemByEntry(lesson.offHand)),
        "Rewarding the lesson removed training equipment");
    if (player.GetQuestStatus(SandyFist) == QUEST_STATUS_NONE)
        Accept(player, giver, SandyFist);
    Require(player.GetQuestStatus(SandyFist) == QUEST_STATUS_INCOMPLETE, "The completed lesson did not admit Sandy Fist");
    CheckAcademyAura(player, lesson.quest);
    std::cout << "PASS native rack, equipment, turn-in, Sandy Fist and academy aura for quest " << lesson.quest << std::endl;
}

struct StudentPet : LootPet
{
    StudentPet(LootPlayer& owner, Map& map) : LootPet(owner, map)
    {
        m_ControlledByPlayer = true;
        setFaction(owner.getFaction());
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
        SetLevel(owner.getLevel());
    }
};

void ClaimPackage(LootPlayer& player, Creature& giver, uint32 questId)
{
    auto quest = sQuestDataStore->GetQuestTemplate(questId);
    auto items = sDB2Manager.GetQuestPackageItems(quest->PackageID);
    Require(items, "The original reward package is unavailable");
    auto item = std::find_if(items->begin(), items->end(), [&](auto reward) {
        return player.CanSelectQuestPackageItem(reward);
    });
    Require(item != items->end(), "No native reward package item is eligible for the student");
    auto choice = (*item)->ItemID;
    auto quantity = (*item)->ItemQuantity;
    auto before = player.GetItemCount(choice);
    Reward(player, giver, questId, choice);
    Require(player.IsQuestRewarded(questId) && player.GetItemCount(choice) == before + quantity,
        "Shang Xi did not record the original reward and grant the selected item");
}

void CheckSandyFist(LootPlayer& player, Creature& giver)
{
    auto& map = *player.GetMap();
    StudentPet pet(player, map);
    auto next = sQuestDataStore->GetQuestTemplate(StifledPride);
    Require(!player.CanTakeQuest(next, false), "Stifled Pride became available before Sandy Fist");
    Reward(player, giver, SandyFist);
    Require(!player.IsQuestRewarded(SandyFist), "Incomplete Sandy Fist could be turned in");
    uint32 credited = 0;
    for (auto spawn : TargetSpawns)
    {
        LessonCreature target(map, spawn, TrainingTarget);
        SupplyMove(player, target.GetPosition());
        map.CreatureRelocation(&pet, target.GetPositionX(), target.GetPositionY(), target.GetPositionZ(), target.GetOrientation());
        Unit& killer = credited % 2 ? static_cast<Unit&>(pet) : static_cast<Unit&>(player);
        if (!killer.IsValidAttackTarget(&target))
            std::cout << "CASE training target " << spawn << " faction=" << target.getFaction()
                << " attacker faction=" << killer.getFaction() << " visible=" << killer.canSeeOrDetect(&target)
                << " reaction=" << killer.GetReactionTo(&target) << std::endl;
        Require(killer.IsValidAttackTarget(&target), "An original training target could not be attacked");
        killer.DealDamage(&target, uint32(target.GetMaxHealth()) * 2);
        Tick(player, 100);
        Require(!target.IsAlive() && player.GetQuestObjectiveData(SandyFist, TrainingTarget) == ++credited,
            "A real training-target death failed to advance exactly once for the student");
    }
    Require(player.GetQuestStatus(SandyFist) == QUEST_STATUS_COMPLETE, "Five training targets did not complete Sandy Fist");
    SupplyMove(player, giver.GetPosition());
    ClaimPackage(player, giver, SandyFist);
    player.PrepareQuestMenu(giver.GetGUID());
    Require(player.PlayerTalkClass->GetQuestMenu().HasItem(StifledPride), "Shang Xi did not offer Stifled Pride");
    Accept(player, giver, StifledPride);
    Require(player.GetQuestStatus(StifledPride) == QUEST_STATUS_INCOMPLETE, "Stifled Pride acceptance failed");
    std::cout << "PASS five placed training targets, player/pet deaths, Sandy Fist package and Stifled Pride acceptance" << std::endl;
}

struct StudentParty : Group
{
    StudentParty(LootPlayer& student, LootPlayer& nearby, LootPlayer& remote) : members{&student, &nearby, &remote}
    {
        this->*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(StifledPride, 0);
        for (auto member : members)
        {
            MemberSlot slot;
            slot.Guid = member->GetGUID();
            slot.Class = member->getClass();
            const_cast<MemberSlotList&>(GetMemberSlots()).push_back(slot);
            member->SetGroup(this, 0);
        }
    }

    ~StudentParty()
    {
        for (auto member : members)
            member->SetGroup(nullptr);
    }

    LootPlayer* members[3];
};

int32 SparringCredit(Player& player)
{
    return player.GetQuestObjectiveData(StifledPride, TraineeCredit);
}

void FinishSpar(LootPlayer& player, Unit& finisher, Creature& trainee)
{
    SupplyMove(player, trainee.GetPosition());
    Require(trainee.IsAIEnabled && player.IsValidAttackTarget(&trainee), "The original trainee is not ready for sparring");
    trainee.AI()->AttackStart(&player);
    finisher.DealDamage(&trainee, uint32(trainee.GetMaxHealth()) * 2);
    trainee.AI()->UpdateAI(100);
    Tick(player, 100);
    Require(trainee.IsAlive() && trainee.getFaction() == 35 && !trainee.isInCombat(), "An original trainee did not surrender safely");
}

void CheckStifledPride(LootPlayer& player, Creature& giver)
{
    auto& map = *player.GetMap();
    LootPlayer nearby(SupplySession(), map, 29521, giver.GetPosition());
    LootPlayer remote(SupplySession(), map, 29522, giver.GetPosition());
    LootPlayer outsider(SupplySession(), map, 29523, giver.GetPosition());
    for (auto other : {&nearby, &remote, &outsider})
    {
        SetStudent(*other, RACE_PANDAREN_NEUTRAL, CLASS_MONK);
        other->SetRewardedQuest(SandyFist);
        Accept(*other, giver, StifledPride);
        Require(other->GetQuestStatus(StifledPride) == QUEST_STATUS_INCOMPLETE, "The sparring witnesses could not accept the quest");
    }
    StudentParty party(player, nearby, remote);
    StudentPet pet(player, map);
    int32 finished = 0;
    for (auto const& [spawn, entry] : Trainees)
    {
        LessonCreature trainee(map, spawn, entry);
        Position pos = trainee.GetPosition();
        SupplyMove(player, pos);
        SupplyMove(nearby, pos);
        SupplyMove(outsider, pos);
        Position remotePosition = pos;
        remotePosition.m_positionX += 200.0f;
        SupplyMove(remote, remotePosition);
        Require(nearby.IsAtGroupRewardDistance(&trainee) && !remote.IsAtGroupRewardDistance(&trainee),
            "Group reward distances did not bracket the trainee");
        map.CreatureRelocation(&pet, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
        TempSummon* guardian = nullptr;
        Unit* finisher = &player;
        if (finished == 1)
            finisher = &pet;
        if (finished == 2)
        {
            guardian = map.SummonCreature(299, pos, sSummonPropertiesStore.LookupEntry(61), 30000, &player);
            Require(dynamic_cast<Guardian*>(guardian) && guardian->GetCharmerOrOwnerPlayerOrPlayerItself() == &player,
                "Could not publish a native guardian owned by the student");
            finisher = guardian;
        }
        player.DealDamage(&trainee, 1);
        Require(SparringCredit(player) == finished, "Damage above the surrender threshold granted early credit");
        FinishSpar(player, *finisher, trainee);
        ++finished;
        Require(SparringCredit(player) == finished && SparringCredit(nearby) == finished &&
            !SparringCredit(remote) && !SparringCredit(outsider), "A surrender failed to credit only the eligible group members");
        finisher->DealDamage(&trainee, uint32(trainee.GetMaxHealth()) * 2);
        Require(trainee.IsAlive() && SparringCredit(player) == finished && SparringCredit(nearby) == finished,
            "A repeated hit on a surrendered trainee awarded progress again");
        if (guardian)
        {
            guardian->UnSummon();
            map.RemoveAllObjectsInRemoveList();
        }
    }
    auto const& [spawn, entry] = Trainees[0];
    LessonCreature trainee(map, spawn, entry);
    SupplyMove(nearby, trainee.GetPosition());
    FinishSpar(player, player, trainee);
    Require(SparringCredit(player) == 5 && SparringCredit(nearby) == 5 &&
        player.GetQuestStatus(StifledPride) == QUEST_STATUS_INCOMPLETE, "Five surrenders did not remain one short");
    trainee.AI()->UpdateAI(14800);
    Require(trainee.getFaction() == 35, "The authored recovery ended too early");
    trainee.AI()->UpdateAI(101);
    Require(trainee.getFaction() == 7, "The trainee did not become available again after the original recovery");
    AbandonSupply(nearby, StifledPride);
    FinishSpar(player, player, trainee);
    Require(SparringCredit(player) == 6 && player.GetQuestStatus(StifledPride) == QUEST_STATUS_COMPLETE &&
        nearby.GetQuestStatus(StifledPride) == QUEST_STATUS_NONE && !SparringCredit(remote) && !SparringCredit(outsider),
        "The sixth surrender must complete only the continuing student");
    SupplyMove(player, giver.GetPosition());
    ClaimPackage(player, giver, StifledPride);
    if (player.GetQuestStatus(BurningScroll) == QUEST_STATUS_NONE)
        Accept(player, giver, BurningScroll);
    Require(player.GetQuestStatus(BurningScroll) == QUEST_STATUS_INCOMPLETE, "Stifled Pride did not admit Burning Scroll");
    std::cout << "PASS all four trainee variants, player/pet/guardian finishes, group distance, repeat protection and Burning Scroll acceptance" << std::endl;
}

void SnatchFlame(LootPlayer& player, Creature& teacher)
{
    // Socketless students do not receive the area's normal movement update.
    player.m_zoneId = teacher.GetZoneId();
    player.m_areaId = teacher.GetAreaId();
    WorldPackets::Spells::SpellClick click{WorldPacket(CMSG_SPELL_CLICK)};
    click.SpellClickUnitGuid = teacher.GetGUID();
    player.GetSession()->HandleSpellClick(click);
    Tick(player, 3000);
}

struct LessonEdict : GameObject
{
    explicit LessonEdict(Map& map)
    {
        auto row = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
            "FROM gameobject WHERE guid=102466 AND id=210986 AND map=860 AND areaId=5834");
        Require(bool(row), "The original Edict of Temperance placement is missing");
        auto fields = row->Fetch();
        Position position{fields[0].GetFloat(), fields[1].GetFloat(), fields[2].GetFloat(), fields[3].GetFloat()};
        G3D::Quat rotation{fields[4].GetFloat(), fields[5].GetFloat(), fields[6].GetFloat(), fields[7].GetFloat()};
        map.LoadGrid(position.GetPositionX(), position.GetPositionY());
        m_DBTableGuid = EdictSpawn;
        Require(Create(EdictSpawn, Edict, &map, 1, position, rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "Could not publish the original edict");
        Update(10000);
    }

    ~LessonEdict()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

void UseEdict(LootPlayer& player, LessonEdict& edict, bool report = false)
{
    player.SetMovedUnit(&player);
    if (report)
    {
        WorldPackets::GameObject::GameObjReportUse request{WorldPacket(CMSG_GAME_OBJ_REPORT_USE)};
        request.Guid = edict.GetGUID();
        player.GetSession()->HandleGameobjectReportUse(request);
    }
    else
    {
        WorldPackets::GameObject::GameObjectUse request{WorldPacket(CMSG_GAME_OBJ_USE)};
        request.Guid = edict.GetGUID();
        player.GetSession()->HandleGameObjectUse(request);
    }
    Tick(player, 200);
}

bool HoldsFlame(LootPlayer& player)
{
    return player.GetItemCount(MastersFlame) == 1 && player.HasAura(114610) &&
        player.GetQuestObjectiveData(BurningScroll, FlameCredit) == 1;
}

void CheckBurningScroll(LootPlayer& player)
{
    auto& map = *player.GetMap();
    LessonCreature teacher(map, TempleGiverSpawn, 53566);
    LessonEdict edict(map);
    auto snatch = sSpellMgr->GetSpellInfo(114746);
    Require(snatch && snatch->GetEffect(EFFECT_2)->TargetA.GetTarget() == TARGET_UNIT_CASTER,
        "The temporary snatch aura must stay on the student");
    SupplyMove(player, teacher.GetPosition());
    SnatchFlame(player, teacher);
    std::cout << "CASE actual temple spellclick: flame inventory=" << player.GetItemCount(MastersFlame)
        << " aura=" << player.HasAura(114610) << " objective=" << player.GetQuestObjectiveData(BurningScroll, FlameCredit)
        << std::endl;
    Require(HoldsFlame(player) && !teacher.HasAura(114746), "The Master's Flame must be credited without locking the shared teacher");
    SnatchFlame(player, teacher);
    Require(HoldsFlame(player), "Repeated clicks did not preserve exactly one flame");
    UseEdict(player, edict);
    Require(HoldsFlame(player) && !player.GetQuestObjectiveData(BurningScroll, BurnCredit), "A remote scroll interaction consumed the flame");

    LootPlayer nearby(SupplySession(), map, 29481, teacher.GetPosition());
    LootPlayer noFlame(SupplySession(), map, 29482, teacher.GetPosition());
    LootPlayer outsider(SupplySession(), map, 29483, teacher.GetPosition());
    for (auto other : {&nearby, &noFlame, &outsider})
    {
        SetStudent(*other, RACE_PANDAREN_NEUTRAL, CLASS_MONK);
        other->SetRewardedQuest(StifledPride);
        Accept(*other, teacher, BurningScroll);
        Require(other->GetQuestStatus(BurningScroll) == QUEST_STATUS_INCOMPLETE, "A scroll witness could not accept the lesson");
    }
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        ItemPosCountVec destination;
        destination.emplace_back((INVENTORY_SLOT_BAG_0 << 8) | slot, 1);
        Require(noFlame.StoreNewItem(destination, 2589, false), "Could not fill the control student's bag");
    }
    SnatchFlame(noFlame, teacher);
    Require(!noFlame.GetItemCount(MastersFlame) && !noFlame.GetQuestObjectiveData(BurningScroll, FlameCredit),
        "A full backpack still granted a flame or credit");
    noFlame.DestroyItem(INVENTORY_SLOT_BAG_0, INVENTORY_SLOT_ITEM_START, false);
    ItemPosCountVec retryDestination;
    Require(noFlame.CanStoreNewItem(NULL_BAG, NULL_SLOT, retryDestination, MastersFlame, 1) == EQUIP_ERR_OK,
        "Freeing the control student's slot did not create room for the flame");
    SnatchFlame(noFlame, teacher);
    Require(HoldsFlame(noFlame), "The flame could not be collected once a bag slot was open");
    AbandonSupply(noFlame, BurningScroll);
    Require(!noFlame.GetItemCount(MastersFlame) && !noFlame.HasAura(114610), "Abandoning the lesson left a flame or aura");
    Accept(noFlame, teacher, BurningScroll);
    Require(noFlame.GetQuestStatus(BurningScroll) == QUEST_STATUS_INCOMPLETE, "The lesson could not be accepted again");
    for (auto other : {&nearby, &outsider})
    {
        SnatchFlame(*other, teacher);
        Require(HoldsFlame(*other), "A second student did not receive their own flame");
    }
    StudentParty group(player, nearby, noFlame);
    for (auto other : {&player, &nearby, &noFlame, &outsider})
        SupplyMove(*other, edict.GetPosition());
    UseEdict(noFlame, edict, true);
    Require(!noFlame.GetQuestObjectiveData(BurningScroll, BurnCredit) && HoldsFlame(player), "A student without a flame could activate the edict");
    UseEdict(player, edict);
    for (auto other : {&player, &nearby})
        Require(!other->GetItemCount(MastersFlame) && !other->HasAura(114610) &&
            other->GetQuestObjectiveData(BurningScroll, BurnCredit) == 1 && other->GetQuestStatus(BurningScroll) == QUEST_STATUS_COMPLETE,
            "An eligible student did not consume the flame and finish the edict");
    Require(HoldsFlame(outsider) && !outsider.GetQuestObjectiveData(BurningScroll, BurnCredit) &&
        !noFlame.GetQuestObjectiveData(BurningScroll, BurnCredit), "The burn affected an unrelated or flame-less player");
    UseEdict(outsider, edict, true);
    Require(outsider.GetQuestStatus(BurningScroll) == QUEST_STATUS_COMPLETE && !outsider.GetItemCount(MastersFlame),
        "A second player could not use the shared edict independently");
    SupplyMove(player, teacher.GetPosition());
    Reward(player, teacher, BurningScroll);
    if (player.GetQuestStatus(AcademyQuest) == QUEST_STATUS_NONE)
        Accept(player, teacher, AcademyQuest);
    Require(player.IsQuestRewarded(BurningScroll) && player.GetQuestStatus(AcademyQuest) == QUEST_STATUS_INCOMPLETE,
        "Burning Scroll did not admit the Disciple's Challenge");
    std::cout << "PASS native temple flame, full bags/retry, abandonment, scoped edict burn and Disciple's Challenge acceptance" << std::endl;
}

void CheckDiscipleChallenge(LootPlayer& player)
{
    auto& map = *player.GetMap();
    LessonCreature giver(map, DiscipleGiverSpawn, 53566);
    LessonCreature boss(map, JaominSpawn, Jaomin);
    LootPlayer nearby(SupplySession(), map, 29491, giver.GetPosition());
    LootPlayer remote(SupplySession(), map, 29492, giver.GetPosition());
    LootPlayer outsider(SupplySession(), map, 29493, giver.GetPosition());
    LootPlayer visitor(SupplySession(), map, 29494, boss.GetPosition());
    SetStudent(visitor, RACE_PANDAREN_NEUTRAL, CLASS_MONK);
    for (auto other : {&nearby, &remote, &outsider})
    {
        SetStudent(*other, RACE_PANDAREN_NEUTRAL, CLASS_MONK);
        other->SetRewardedQuest(BurningScroll);
        Accept(*other, giver, AcademyQuest);
        Require(other->GetQuestStatus(AcademyQuest) == QUEST_STATUS_INCOMPLETE, "A witness could not accept the Disciple's Challenge");
    }
    StudentParty group(player, nearby, remote);
    for (auto other : {&player, &nearby, &outsider})
        SupplyMove(*other, boss.GetPosition());
    Position remotePosition = boss.GetPosition();
    remotePosition.m_positionX += 200.0f;
    SupplyMove(remote, remotePosition);
    Require(!remote.IsAtGroupRewardDistance(&boss), "The distant witness is within the native reward radius");
    StudentPet pet(player, map);
    auto const fullHealth = boss.GetHealth();
    visitor.DealDamage(&boss, uint32(fullHealth) * 2);
    Require(boss.GetHealth() == fullHealth && boss.GetDisplayId() == 39755 && !boss.AI()->CanAIAttack(&visitor),
        "A player without the challenge could start or advance the duel");
    boss.AI()->AttackStart(&player);
    player.DealDamage(&boss, uint32(fullHealth * 3 / 4));
    Require(boss.IsAlive() && boss.GetDisplayId() == 39796 && !player.GetQuestObjectiveData(AcademyQuest, Jaomin),
        "The ordinary damage did not enter the falcon phase without granting victory");
    pet.DealDamage(&boss, uint32(boss.GetMaxHealth()) * 2);
    Tick(player, 100);
    std::cout << "CASE Jaomin pet finish: owner=" << player.GetQuestObjectiveData(AcademyQuest, Jaomin)
        << " party=" << nearby.GetQuestObjectiveData(AcademyQuest, Jaomin)
        << " outsider=" << outsider.GetQuestObjectiveData(AcademyQuest, Jaomin)
        << " remote=" << remote.GetQuestObjectiveData(AcademyQuest, Jaomin) << std::endl;
    Require(!boss.isInCombat(), "Jaomin kept fighting after surrender");
    outsider.DealDamage(&boss, uint32(boss.GetMaxHealth()) * 2);
    Require(outsider.GetQuestStatus(AcademyQuest) == QUEST_STATUS_INCOMPLETE &&
        boss.HasAura(108959), "A hit during surrender altered the cooldown or handed out extra credit");
    boss.AI()->UpdateAI(8999);
    Require(!outsider.IsValidAttackTarget(&boss), "Jaomin reopened the duel before the original recovery time");
    boss.AI()->UpdateAI(2);
    std::cout << "CASE Jaomin after recovery out of combat: attackable="
        << outsider.IsValidAttackTarget(&boss) << std::endl;
    Require(boss.IsAlive() && player.GetQuestStatus(AcademyQuest) == QUEST_STATUS_COMPLETE &&
        nearby.GetQuestStatus(AcademyQuest) == QUEST_STATUS_COMPLETE && outsider.GetQuestStatus(AcademyQuest) == QUEST_STATUS_INCOMPLETE &&
        remote.GetQuestStatus(AcademyQuest) == QUEST_STATUS_INCOMPLETE, "Jaomin did not reward only the actual combatant and nearby group");
    Require(outsider.IsValidAttackTarget(&boss), "Jaomin did not recover without a victim");
    for (uint32 elapsed = 0; elapsed < 1000; elapsed += 100)
        boss.Update(100);
    Require(!boss.IsInEvadeMode(), "Jaomin did not finish returning home");
    boss.AI()->AttackStart(&outsider);
    outsider.DealDamage(&boss, uint32(boss.GetMaxHealth()) * 2);
    Require(outsider.GetQuestStatus(AcademyQuest) == QUEST_STATUS_COMPLETE && boss.IsAlive() &&
        player.GetQuestObjectiveData(AcademyQuest, Jaomin) == 1, "A fresh challenger could not win with one hit after recovery");
    boss.AI()->UpdateAI(9001);
    for (uint32 elapsed = 0; elapsed < 1000; elapsed += 100)
        boss.Update(100);
    SupplyMove(remote, boss.GetPosition());
    TempSummon* guardian = map.SummonCreature(299, boss.GetPosition(), sSummonPropertiesStore.LookupEntry(61), 30000, &remote);
    Require(dynamic_cast<Guardian*>(guardian) && guardian->GetCharmerOrOwnerPlayerOrPlayerItself() == &remote,
        "The next challenger could not publish an owned guardian");
    boss.AI()->AttackStart(&remote);
    Require(boss.AI()->CanAIAttack(guardian), "The recovered challenger did not admit the guardian's owner");
    guardian->DealDamage(&boss, uint32(boss.GetMaxHealth()) * 2);
    Require(remote.GetQuestStatus(AcademyQuest) == QUEST_STATUS_COMPLETE && boss.IsAlive(),
        "An owned guardian could not win the next challenge");
    guardian->UnSummon();
    map.RemoveAllObjectsInRemoveList();
    SupplyMove(player, giver.GetPosition());
    ClaimPackage(player, giver, AcademyQuest);
    if (player.GetQuestStatus(Aysa) == QUEST_STATUS_NONE)
        Accept(player, giver, Aysa);
    Require(player.GetQuestStatus(Aysa) == QUEST_STATUS_COMPLETE || player.GetQuestStatus(Aysa) == QUEST_STATUS_INCOMPLETE,
        "The Disciple's Challenge did not admit Aysa of the Tushui");
    std::cout << "PASS native Jaomin player/pet/guardian finishes, eligible nearby party, recovery and Aysa acceptance" << std::endl;

    LessonCreature lorvo(map, LorvoSpawn, 54943);
    SupplyMove(player, lorvo.GetPosition());
    Reward(player, lorvo, Aysa);
    Require(player.IsQuestRewarded(Aysa), "Aysa of the Tushui could not be turned in to Merchant Lorvo");
    player.PrepareQuestMenu(lorvo.GetGUID());
    Require(player.PlayerTalkClass->GetQuestMenu().HasItem(MissingDriver) &&
        player.PlayerTalkClass->GetQuestMenu().HasItem(ImportantItems), "Lorvo did not offer both follow-up quests");
    Accept(player, lorvo, MissingDriver);
    Accept(player, lorvo, ImportantItems);
    Require(player.GetQuestStatus(MissingDriver) == QUEST_STATUS_INCOMPLETE &&
        player.GetQuestStatus(ImportantItems) == QUEST_STATUS_INCOMPLETE, "Lorvo's two follow-up quests could not be accepted together");
    std::cout << "PASS Aysa turn-in at placed Merchant Lorvo and acceptance of both follow-up quests" << std::endl;
}

void CheckImportantItems(LootPlayer& player)
{
    auto& map = *player.GetMap();
    auto const quest = sQuestDataStore->GetQuestTemplate(ImportantItems);
    uint32 collected = 0;
    for (auto spawn : SupplyScamps)
    {
        LessonCreature scamp(map, spawn, Scamp);
        SupplyMove(player, scamp.GetPosition());
        Require(player.IsValidAttackTarget(&scamp), "An original scamp was unavailable");
        player.DealDamage(&scamp, uint32(scamp.GetMaxHealth()) * 2);
        Tick(player, 100);
        Require(!scamp.IsAlive() && player.GetItemCount(StolenSupplies) == collected, "Scamp death gave an item before it was looted");
        player.GetSession()->LootCorps(scamp.GetGUID());
        // Ordinary corpses keep quest items in the shared container, with a per-player quest list.
        auto& loot = scamp.loot;
        WorldPackets::Loot::LootResponse view;
        loot.BuildLootResponse(view, &player, OWNER_PERMISSION);
        auto supply = std::find_if(view.Items.begin(), view.Items.end(), [](auto const& slot) {
            return slot.Loot.ItemID == StolenSupplies;
        });
        Require(supply != view.Items.end(), "The scamp's native corpse loot did not show the quest supplies");
        WorldPackets::Loot::AutoStoreLootItem request{WorldPacket(CMSG_LOOT_ITEM)};
        request.Loot.push_back({loot.GetGUID(), supply->LootListID});
        player.GetSession()->HandleAutostoreLootItemOpcode(request);
        player.GetSession()->HandleAutostoreLootItemOpcode(request);
        Require(player.GetItemCount(StolenSupplies) == ++collected, "The corpse loot did not grant exactly one supply");
        Require(player.CanRewardQuest(quest, false) == (collected == std::size(SupplyScamps)),
            "Lorvo's reward check did not require all six actual supplies");
        WorldPackets::Loot::LootRelease release{WorldPacket(CMSG_LOOT_RELEASE)};
        release.Unit = scamp.GetGUID();
        player.GetSession()->HandleLootRelease(release);
    }
    LessonCreature lorvo(map, LorvoSpawn, 54943);
    SupplyMove(player, lorvo.GetPosition());
    Reward(player, lorvo, ImportantItems);
    Require(player.IsQuestRewarded(ImportantItems) && !player.GetItemCount(StolenSupplies) &&
        player.GetQuestStatus(MissingDriver) == QUEST_STATUS_INCOMPLETE, "Turning in the supplies affected the parallel driver quest");
    std::cout << "PASS six placed scamps, normal corpse-loot packets and Lorvo turn-in for Items of Utmost Importance" << std::endl;
}

void CrossDriverApproach(LootPlayer& player, bool entering = true)
{
    WorldPackets::Misc::AreaTrigger request{WorldPacket(CMSG_AREA_TRIGGER)};
    request.AreaTriggerID = DriverApproach;
    request.Entered = entering;
    request.FromClient = true;
    player.GetSession()->HandleAreaTrigger(request);
    Tick(player, 100);
}

TempSummon* FindPrivateDriver(LootPlayer& player)
{
    for (ObjectGuid const& guid : *player.GetSummonList(PrivateDriver))
        if (Creature* driver = player.GetMap()->GetCreature(guid))
            if (driver->HasQuestObjectScope() && !driver->IsQuestObjectExpired())
                return driver->ToTempSummon();
    return nullptr;
}

void CheckMissingDriver(LootPlayer& player)
{
    Map& map = *player.GetMap();
    AreaTriggerEntry const* trigger = sAreaTriggerStore.LookupEntry(DriverApproach);
    Require(trigger && trigger->ContinentID == 860 && sScriptSystemMgr->GetPointMoveList(PrivateDriver).size() == 6,
        "The original approach trigger or local Min route was not available");
    Position const approach(trigger->Pos.X, trigger->Pos.Y, trigger->Pos.Z);
    SupplyMove(player, approach);
    CrossDriverApproach(player);
    Require(!FindPrivateDriver(player) && !player.GetQuestObjectiveData(MissingDriver, Driver),
        "Entering the area while the original Min was missing granted progress");

    LessonCreature captured(map, DriverSpawn, Driver);
    LootPlayer visitor(SupplySession(), map, 41200, approach);
    SetStudent(visitor, RACE_PANDAREN_NEUTRAL, CLASS_HUNTER);
    CrossDriverApproach(visitor);
    Require(visitor.GetSummonList(PrivateDriver)->empty(), "A player without the quest summoned Min");

    SupplyMove(player, approach.GetPositionWithOffset({35.0f, 0.0f, 0.0f, 0.0f}));
    CrossDriverApproach(player);
    SupplyMove(player, approach);
    CrossDriverApproach(player, false);
    Require(!FindPrivateDriver(player) && !player.GetQuestObjectiveData(MissingDriver, Driver),
        "An out-of-range packet or leaving the trigger counted as finding Min");
    CrossDriverApproach(player);
    auto driver = FindPrivateDriver(player);
    Require(driver && driver->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == 106205 &&
        driver->GetSummonerGUID() == player.GetGUID() && driver->IsInPersonnalVisibilityList(player.GetGUID()) &&
        !driver->IsInPersonnalVisibilityList(visitor.GetGUID()) && captured.IsAlive() &&
        captured.ShouldHideFor(player.GetGUID()) && !captured.ShouldHideFor(visitor.GetGUID()) &&
        player.GetQuestObjectiveData(MissingDriver, Driver) == 1 && player.GetQuestStatus(MissingDriver) == QUEST_STATUS_COMPLETE,
        "The original Min's native force-cast did not publish his private outro and complete the objective");
    CrossDriverApproach(player);
    Require(FindPrivateDriver(player) == driver && player.GetSummonList(PrivateDriver)->size() == 1 &&
        player.GetQuestObjectiveData(MissingDriver, Driver) == 1, "Repeating the approach created duplicate Min actors or progress");

    ObjectGuid const abandoned = driver->GetGUID();
    AbandonSupply(player, MissingDriver);
    map.RemoveAllObjectsInRemoveList();
    captured.AI()->UpdateAI(1000);
    Require(!map.GetCreature(abandoned) && !captured.ShouldHideFor(player.GetGUID()),
        "Abandoning the quest did not clean up the private Min and restore the original");
    LessonCreature lorvo(map, LorvoSpawn, 54943);
    SupplyMove(player, lorvo.GetPosition());
    Accept(player, lorvo, MissingDriver);
    SupplyMove(player, approach);
    CrossDriverApproach(player);
    driver = FindPrivateDriver(player);
    Require(driver && driver->GetGUID() != abandoned && player.GetQuestObjectiveData(MissingDriver, Driver) == 1,
        "The next accepted attempt could not find Min again");
    ObjectGuid const retry = driver->GetGUID();
    for (uint32 elapsed = 0; elapsed < 8000 && map.GetCreature(retry); elapsed += 100)
    {
        driver->Update(100);
        map.RemoveAllObjectsInRemoveList();
    }
    driver = map.GetCreature(retry) ? map.GetCreature(retry)->ToTempSummon() : nullptr;
    auto escort = driver ? dynamic_cast<npc_escortAI*>(driver->AI()) : nullptr;
    Require(escort && escort->HasEscortState(STATE_ESCORT_ESCORTING) && driver->GetDistance2d(&captured) > 1.0f,
        "Min did not start walking the local route after his native appearance");

    SupplyMove(player, lorvo.GetPosition());
    Reward(player, lorvo, MissingDriver);
    map.RemoveAllObjectsInRemoveList();
    captured.AI()->UpdateAI(1000);
    Require(player.IsQuestRewarded(MissingDriver) && !map.GetCreature(retry) && captured.IsAlive() &&
        !captured.ShouldHideFor(player.GetGUID()) &&
        player.IsQuestRewarded(ImportantItems), "Lorvo did not clean up the private driver and preserve the supplies turn-in");
    LessonCreature aysa(map, AysaNextSpawn, 54567);
    SupplyMove(player, aysa.GetPosition());
    player.PrepareQuestMenu(aysa.GetGUID());
    Require(player.PlayerTalkClass->GetQuestMenu().HasItem(AysaNext), "The next quest was missing from the placed Aysa's menu");
    Accept(player, aysa, AysaNext);
    Require(player.GetQuestStatus(AysaNext) == QUEST_STATUS_INCOMPLETE, "Aysa would not accept the next quest in the chain");
    std::cout << "PASS native Min approach, scoped retry/cleanup, escort start, Lorvo turn-in and Aysa's next offer" << std::endl;
}

void CheckAysaMeditation(LootPlayer& player)
{
    Map& map = *player.GetMap();
    LessonCreature aysa(map, AysaMeditationSpawn, AysaMeditation);
    SupplyMove(player, aysa.GetPosition());
    Position const outside = aysa.GetPositionWithOffset({40.0f, 0.0f, 0.0f, 0.0f});
    LootPlayer late(SupplySession(), map, 41201, outside);
    SetStudent(late, RACE_PANDAREN_NEUTRAL, CLASS_HUNTER);
    late.AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(AysaNext), nullptr);
    LootPlayer visitor(SupplySession(), map, 41202, aysa.GetPosition());
    SetStudent(visitor, RACE_PANDAREN_NEUTRAL, CLASS_HUNTER);
    StudentParty party(player, late, visitor);

    auto ownedActors = [&](uint32 entry) {
        std::list<Creature*> actors;
        aysa.GetCreatureListWithEntryInGrid(actors, entry, 45.0f);
        actors.remove_if([&](Creature* actor) {
            auto summon = actor->ToTempSummon();
            return !summon || summon->GetSummonerGUID() != aysa.GetGUID();
        });
        return actors;
    };
    auto advance = [&](uint32 duration) {
        for (uint32 elapsed = 0; elapsed < duration; elapsed += 100)
        {
            aysa.AI()->UpdateAI(100);
            player.Unit::Update(100);
            late.Unit::Update(100);
            map.RemoveAllObjectsInRemoveList();
        }
    };

    try
    {
        advance(87000);
        Require(!player.GetQuestObjectiveData(AysaNext, AysaMeditationCredit),
            "The first player was credited before finishing the meditation defense");
        SupplyMove(late, aysa.GetPosition());
        advance(4100);
        auto const firstCredit = player.GetQuestObjectiveData(AysaNext, AysaMeditationCredit);
        auto const lateCredit = late.GetQuestObjectiveData(AysaNext, AysaMeditationCredit);
        std::cout << "CASE meditation first=" << firstCredit << " late=" << lateCredit << std::endl;
        Require(firstCredit == 1 && lateCredit == 0 && late.GetQuestStatus(AysaNext) == QUEST_STATUS_INCOMPLETE,
            "The cave granted the full defense to a late arrival after only four seconds");
        Require(!player.HasAura(AysaMeditationBar) && late.HasAura(AysaMeditationBar) &&
            late.GetMaxPower(POWER_ALTERNATE) == 90 && late.GetPower(POWER_ALTERNATE) <= 4 &&
            !visitor.HasAura(AysaMeditationBar),
            "The native power bar did not keep the late arrival's own progress");

        SupplyMove(late, outside);
        advance(1000);
        Require(!late.HasAura(AysaMeditationBar) && ownedActors(AysaWave).empty() &&
            ownedActors(AysaMeditationCredit).empty(), "Leaving the cave left progress or encounter actors running");
        SupplyMove(late, aysa.GetPosition());
        advance(9200);
        auto wave = ownedActors(AysaWave);
        Require(!wave.empty() && late.GetPower(POWER_ALTERNATE) < 10 && late.HasAura(AysaMeditationBar),
            "Reentry did not restart the defense and spawn a real wave");
        wave.front()->DealDamage(&aysa, uint32(aysa.GetMaxHealth()) * 2);
        map.RemoveAllObjectsInRemoveList();
        Require(aysa.IsAlive() && ownedActors(AysaWave).empty() && !late.HasAura(AysaMeditationBar) &&
            !late.GetQuestObjectiveData(AysaNext, AysaMeditationCredit), "A defeat did not reset the temporary waves and player's progress");
        advance(111100);
        Require(late.GetQuestStatus(AysaNext) == QUEST_STATUS_COMPLETE && !late.HasAura(AysaMeditationBar) &&
            ownedActors(AysaWave).empty(), "A retry did not allow the second player to finish the full defense");

        LessonCreature turnIn(map, AysaMeditationTurnIn, 54608);
        SupplyMove(player, turnIn.GetPosition());
        ClaimPackage(player, turnIn, AysaNext);
        if (player.GetQuestStatus(JiOfTheHuojin) == QUEST_STATUS_NONE)
            Accept(player, turnIn, JiOfTheHuojin);
        Require(player.GetQuestStatus(JiOfTheHuojin) != QUEST_STATUS_NONE, "Aysa did not open Ji of the Huojin");
    }
    catch (...)
    {
        for (uint32 entry : {AysaWave, AysaMeditationCredit})
            for (Creature* actor : ownedActors(entry))
            {
                actor->CombatStop(true);
                actor->DespawnOrUnsummon();
            }
        map.RemoveAllObjectsInRemoveList();
        throw;
    }

    std::cout << "PASS native individual bars, late entry, leaving the cave, defeat/retry, Aysa turn-in and Ji offer" << std::endl;
}

void CheckWayOfTheHuojin(LootPlayer& player)
{
    Map& map = *player.GetMap();
    LessonCreature ji(map, JiSpawn, 54568);
    SupplyMove(player, ji.GetPosition());
    Reward(player, ji, JiOfTheHuojin);
    Require(player.IsQuestRewarded(JiOfTheHuojin), "Ji at Wu-Song Village did not accept the introduction");
    if (player.GetQuestStatus(WayOfTheHuojin) == QUEST_STATUS_NONE)
        Accept(player, ji, WayOfTheHuojin);
    Require(player.GetQuestStatus(WayOfTheHuojin) == QUEST_STATUS_INCOMPLETE &&
        !player.CanTakeQuest(sQuestDataStore->GetQuestTemplate(KindlingTheFire), false),
        "Ji did not correctly gate The Way of the Huojin and its successor");

    StudentPet pet(player, map);
    uint32 kills = 0;
    auto defeat = [&](LessonCreature& attacker) {
        SupplyMove(player, attacker.GetPosition());
        Unit& killer = kills % 2 ? static_cast<Unit&>(pet) : static_cast<Unit&>(player);
        if (&killer == &pet)
            map.CreatureRelocation(&pet, attacker.GetPositionX(), attacker.GetPositionY(), attacker.GetPositionZ(), attacker.GetOrientation());
        if (!killer.IsValidAttackTarget(&attacker))
            std::cout << "CASE Fe-Feng spawn=" << attacker.GetDBTableGUIDLow() << " kills=" << kills
                << " alive=" << attacker.IsAlive() << " flags=" << attacker.GetUInt32Value(UNIT_FIELD_FLAGS)
                << " killer-alive=" << killer.IsAlive() << " same-phase=" << killer.InSamePhase(&attacker)
                << " distance=" << killer.GetDistance(&attacker) << std::endl;
        Require(killer.IsValidAttackTarget(&attacker), "An original Fe-Feng attacker was unavailable");
        killer.DealDamage(&attacker, uint32(attacker.GetMaxHealth()) * 2);
        Tick(player, 100);
        Require(!attacker.IsAlive() && player.GetQuestObjectiveData(WayOfTheHuojin, FeFengAttacker) == ++kills &&
            player.CanRewardQuest(sQuestDataStore->GetQuestTemplate(WayOfTheHuojin), false) == (kills == std::size(FeFengSpawns) + 1),
            "The Way of the Huojin did not count exactly eight real player and pet kills");
    };
    for (auto spawn : FeFengSpawns)
    {
        LessonCreature attacker(map, spawn, FeFengAttacker);
        defeat(attacker);
        if (spawn == FeFengSpawns[std::size(FeFengSpawns) - 1])
        {
            attacker.RemoveCorpse(false);
            attacker.SetRespawnTime(0);
            attacker.Update(10000);
            defeat(attacker);
        }
    }

    SupplyMove(player, ji.GetPosition());
    ClaimPackage(player, ji, WayOfTheHuojin);
    if (player.GetQuestStatus(KindlingTheFire) == QUEST_STATUS_NONE)
        Accept(player, ji, KindlingTheFire);
    Require(player.GetQuestStatus(KindlingTheFire) == QUEST_STATUS_INCOMPLETE,
        "Ji did not offer Kindling the Fire after the attacker's real defeat and reward");
    std::cout << "PASS native Ji introduction, player and pet kills on seven placed attackers and one respawn, and Kindling the Fire offer" << std::endl;
}

void CheckFirstLesson()
{
    Map map(860, 300, 0, DIFFICULTY_NONE);
    LessonCreature giver(map, GiverSpawn, 53566);
    uint64 guid = 30000;
    for (Lesson const& route : Lessons)
    {
        uint32 lesson = route.quest;
        LootPlayer student(SupplySession(), map, ++guid, giver.GetPosition());
        SetStudent(student, RACE_PANDAREN_NEUTRAL, route.playerClass);
        student.PrepareAreaQuest(Area);
        for (uint32 introduction = 30039; introduction <= 30045; ++introduction)
            Require(student.GetQuestStatus(introduction) == QUEST_STATUS_NONE, "An obsolete introduction auto-started");
        student.PrepareQuestMenu(giver.GetGUID());
        for (Lesson const& other : Lessons)
            Require(student.PlayerTalkClass->GetQuestMenu().HasItem(other.quest) == (other.playerClass == route.playerClass),
                "Shang Xi's quest menu did not offer only the correct class lesson");
        Accept(student, giver, lesson);
        Require(student.GetQuestStatus(lesson) == QUEST_STATUS_INCOMPLETE &&
            !student.CanTakeQuest(sQuestDataStore->GetQuestTemplate(SandyFist), false),
            "The real acceptance handler failed or skipped the equipment objective");
        AbandonSupply(student, lesson);
        Accept(student, giver, lesson);
        Require(student.GetQuestStatus(lesson) == QUEST_STATUS_INCOMPLETE, "The first lesson could not be accepted again");
        student.InitTalentForLevel();
        student.LearnDefaultSkills();
        student.LearnCustomSpells();
        if (route.playerClass == CLASS_MONK)
            student.AddAura(103985, &student);
        Tick(student, 100);
        if (route.playerClass == CLASS_MONK)
            Require(student.GetPrimarySpecialization() == 269 && student.HasSpell(124146) && student.CanDualWield(),
                "The new monk's native Windwalker specialization did not teach Dual Wield");
        CheckEquipment(student, giver, route);
        if (route.playerClass == CLASS_HUNTER)
        {
            CheckSandyFist(student, giver);
            CheckStifledPride(student, giver);
            CheckBurningScroll(student);
            CheckDiscipleChallenge(student);
            CheckImportantItems(student);
            CheckMissingDriver(student);
            CheckAysaMeditation(student);
            CheckWayOfTheHuojin(student);
        }
    }
    LootPlayer visitor(SupplySession(), map, ++guid, giver.GetPosition());
    SetStudent(visitor, RACE_DRAENEI, CLASS_SHAMAN);
    visitor.PrepareQuestMenu(giver.GetGUID());
    for (auto const& lesson : Lessons)
        Require(!visitor.PlayerTalkClass->GetQuestMenu().HasItem(lesson.quest), "Non-Pandaren could see a starter lesson");

    LootPlayer legacy(SupplySession(), map, ++guid, giver.GetPosition());
    SetStudent(legacy, RACE_PANDAREN_NEUTRAL, CLASS_MONK);
    legacy.AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(30039), nullptr);
    Require(legacy.GetQuestStatus(30039) == QUEST_STATUS_COMPLETE, "The saved introduction cannot be completed");
    Reward(legacy, giver, 30039);
    Require(legacy.GetQuestRewardStatus(30039) && legacy.GetQuestStatus(30027) == QUEST_STATUS_INCOMPLETE,
        "The legacy turn-in did not hand off to the correct weapon lesson");
    std::cout << "PASS seven native class offers, acceptance, abandonment and retry, race restriction, and legacy turn-in\n";
}
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(CrashTrace);
    try
    {
        InitializeNative(argc, argv);
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_WanderingIsland();
        sScriptMgr->SwapScriptContext(true);
        opcodeTable.Initialize();
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{860, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{860, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        std::exception_ptr failure;
        try
        {
            auto schema = CharacterDatabase.Query("SELECT DATABASE()");
            Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The lesson check requires isolated characters");
            LoadLessonInputs();
            for (auto spawn : {GiverSpawn, 243565u, 243566u, 243567u, 243568u, 243569u, 243610u, 243660u,
                243608u, 243645u, TempleGiverSpawn, JaominSpawn, DiscipleGiverSpawn, LorvoSpawn,
                243766u, 243788u, 243790u, 244114u, 244115u, 243764u, DriverSpawn, AysaNextSpawn, AysaMeditationSpawn,
                AysaMeditationTurnIn, JiSpawn, 243820u, 243827u, 243835u, 243836u, 243837u, 243844u, 243845u})
                if (auto data = sObjectMgr->GetCreatureData(spawn))
                    sObjectMgr->RemoveCreatureFromGrid(spawn, data);
            sObjectMgr->LoadPlayerInfo();
            HashMapHolder<Player>::SetSize(32);
            HashMapHolder<GameObject>::SetSize(32);
            GameTime::UpdateGameTimers();
            CheckFirstLesson();
        }
        catch (...)
        {
            failure = std::current_exception();
        }
        CharacterDatabase.Close();
        LoginDatabase.Close();
        HotfixDatabase.Close();
        WorldDatabase.Close();
        if (failure)
            std::rethrow_exception(failure);
        return 0;
    }
    catch (std::exception const& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
