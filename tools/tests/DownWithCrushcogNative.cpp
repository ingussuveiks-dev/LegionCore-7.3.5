#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GilneasQuestNative.h"
#include "SmartScriptMgr.h"
#include "GossipData.h"
#include "GossipDef.h"
#include "NPCPackets.h"
#include "CreatureTextMgr.h"
#include "ScriptMgr.h"
#include "ScriptsData.h"
#include "Vehicle.h"

void AddSC_dun_morogh();

struct AssaultTextTag
{
    using Type = CreatureTextRepeatIds (CreatureTextMgr::*)(Creature*, uint8);
    friend Type Access(AssaultTextTag);
};
template struct FixtureAccess<AssaultTextTag, &CreatureTextMgr::GetRepeatGroup>;

namespace
{
constexpr uint32 QuestId = 26364;
constexpr uint32 Prerequisite = 26342;
constexpr uint32 Device = 58253;
constexpr uint32 GroundTarget = 80101;
constexpr uint32 Tinker = 42849;
constexpr uint32 DefeatedCredit = 42860;
constexpr uint32 JarviSpawn = 84470;
constexpr uint32 TinkerSpawn = 84169;
constexpr uint32 StonegrindSpawn = 84288;
constexpr uint32 RazloSpawn = 84395;
constexpr std::array<uint32, 4> GuardianSpawns{84392, 84393, 84394, 84407};

struct IceflowPlacement : Creature
{
    IceflowPlacement(Map& map, uint32 spawn) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "The actual Iceflow Lake placement is missing");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true) && IsInWorld(), "An actual Iceflow Lake actor did not load into the map");
    }

    ~IceflowPlacement()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void LoadDownWithCrushcogInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeDownWithCrushcog.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "The native Device quest or prerequisite is absent");
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
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=26364");
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=26364 ORDER BY StorageIndex");
    Require(addon && objective && objective->GetRowCount() == 2, "Required assault objectives or quest prerequisite is missing");
    auto quest = quests[QuestId];
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(quest->PrevQuestID);
    do { quest->LoadQuestObjective(objective->Fetch()); } while (objective->NextRow());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    Require(quest->PrevQuestID == Prerequisite && quest->SourceItemId == Device && quest->GetObjectives()[0].ObjectID == DefeatedCredit &&
        quest->GetObjectives()[0].Amount == 1 && quest->GetObjectives()[1].ObjectID == Tinker && quest->GetObjectives()[1].Amount == 1, "The authored Device quest objective or source item changed");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    auto device = sObjectMgr->GetItemTemplate(Device);
    Require(device && std::any_of(device->Effects.begin(), device->Effects.end(), [](ItemEffectEntry const* effect)
        { return effect && effect->SpellID == GroundTarget && !effect->Charges; }), "The real reusable Device changed");
    auto spell = sSpellMgr->GetSpellInfo(GroundTarget);
    Require(spell && spell->GetEffect(EFFECT_0)->Effect == SPELL_EFFECT_SUMMON && spell->GetEffect(EFFECT_0)->MiscValue == 42929, "The native Device effect changed");
    std::cout << "CASE native Device cast time=" << spell->CalcCastTime() << " missile speed=" << spell->GetMisc()->MiscData.Speed << std::endl;
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(84470,84169,84288,84395,84392,84393,84394,84407)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_addon AS SELECT addon.* FROM legion_world.creature_addon addon JOIN creature spawn ON spawn.guid=addon.guid");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_template_addon AS SELECT * FROM legion_world.creature_template_addon WHERE entry IN(42353,42849,42852,42839,42294)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=26364");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=26364");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 AND entryorguid IN (42849,42839,42852)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu AS SELECT * FROM legion_world.gossip_menu WHERE MenuID=11662");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu_option AS SELECT * FROM legion_world.gossip_menu_option WHERE MenuID=11662");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_text AS SELECT * FROM legion_world.npc_text WHERE ID=16305");
#ifndef DOWN_WITH_CRUSHCOG_BASELINE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells WHERE npc_entry=42839");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE vehicle_template_accessory AS SELECT * FROM legion_world.vehicle_template_accessory WHERE EntryOrAura=42839");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text WHERE CreatureID IN(42849,42852,42839)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id IN(79931,80098,80101,80110,80148,80220)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE (SourceTypeOrReferenceId=15 AND SourceGroup=11662) OR (SourceTypeOrReferenceId=13 AND SourceEntry=79931)");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadCreatureTemplateAddons();
    sObjectMgr->LoadCreatureAddons();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sObjectMgr->LoadNPCText();
    sGossipDataStore->LoadGossipMenu();
    sGossipDataStore->LoadGossipMenuItems();
    sSmartScriptMgr->LoadSmartAIFromDB();
#ifndef DOWN_WITH_CRUSHCOG_BASELINE
    sObjectMgr->LoadNPCSpellClickSpells();
    sObjectMgr->LoadVehicleTemplateAccessories();
    sCreatureTextMgr->LoadCreatureTexts();
    sConditionMgr->LoadConditions();
    sScriptDataStore->LoadSpellScriptNames();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_dun_morogh();
    sScriptMgr->SwapScriptContext(true);
    sScriptDataStore->ValidateSpellScripts();
#endif
}

void AcceptAssault(LootPlayer& owner, Creature& jarvi)
{
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_GNOME);
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_MAGE);
    owner.setFactionForRace(RACE_GNOME);
    owner.InitDisplayIds();
    owner.GetReputationMgr().LoadFromDB({});
    owner.SetLevel(5);
    owner.SetRewardedQuest(Prerequisite);
    owner.SetFloatValue(UNIT_FIELD_MOD_TIME_RATE, 1.0f);
    owner.SetFloatValue(UNIT_FIELD_MOD_CASTING_SPEED, 1.0f);
    owner.SetFloatValue(UNIT_FIELD_MOD_SPELL_HASTE, 1.0f);
    owner.SetFloatValue(UNIT_FIELD_MOD_HASTE, 1.0f);
    owner.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    MoveGilneasPlayer(owner, jarvi.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestID = QuestId;
    accept.QuestGiverGUID = jarvi.GetGUID();
    owner.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(owner.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && owner.GetItemCount(Device) == 1,
        "Jarvi did not offer the native Iceflow Lake assault");
}

void SelectAssault(LootPlayer& player, Creature& tinker)
{
    WorldPacket wire(CMSG_GOSSIP_SELECT_OPTION);
    wire << tinker.GetGUID() << uint32(11662) << uint32(0);
    wire.WriteBits(0, 8);
    wire.FlushBits();
    WorldPackets::NPC::GossipSelectOption packet(std::move(wire));
    packet.Read();
    player.GetSession()->HandleGossipSelectOption(packet);
}

#ifdef DOWN_WITH_CRUSHCOG_BASELINE
void CheckDownWithCrushcog()
{
    for (uint32 spawn : {JarviSpawn, TinkerSpawn, StonegrindSpawn, RazloSpawn})
        sObjectMgr->RemoveCreatureFromGrid(spawn, sObjectMgr->GetCreatureData(spawn));

    Map map(0, 622, 0, DIFFICULTY_NONE);
    IceflowPlacement jarvi(map, JarviSpawn), tinker(map, TinkerSpawn), stonegrind(map, StonegrindSpawn), razlo(map, RazloSpawn);
    LootPlayer owner(SupplySession(), map, 2636401, jarvi.GetPosition());
    AcceptAssault(owner, jarvi);
    MoveGilneasPlayer(owner, tinker.GetNearPosition(2.0f, 0.0f));
    owner.PrepareGossipMenu(&tinker, 11662, true);
    std::cout << "CASE native assault option=" << bool(owner.PlayerTalkClass->GetGossipMenu().GetItem(0))
        << " Razlo vehicle=" << bool(razlo.GetVehicleKit()) << " allied level=" << uint32(stonegrind.getLevel())
        << " enemy level=" << uint32(razlo.getLevel()) << std::endl;
    if (owner.PlayerTalkClass->GetGossipMenu().GetItem(0))
        SelectAssault(owner, tinker);
    Tick(owner, 1500);
    Require(!tinker.isInCombat() && !stonegrind.isInCombat() && !razlo.isInCombat() && !razlo.GetVehicleKit(),
        "The original actors already had the assault and vehicle");
    std::cout << "CASE original battle after selection talk=" << owner.GetQuestObjectiveData(QuestId, Tinker)
        << " kill=" << owner.GetQuestObjectiveData(QuestId, DefeatedCredit) << " combat=" << razlo.isInCombat() << std::endl;

    MoveGilneasPlayer(owner, razlo.GetNearPosition(3.0f, 0.0f));
    razlo.SetHealth(1);
    Require(owner.DealDamage(&razlo, 1, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) && !razlo.IsAlive(),
        "Could not reach the original actual Razlo death dispatch");
    Tick(owner, 1500);
    auto victory = owner.GetQuestObjectiveData(QuestId, DefeatedCredit);
    std::cout << "CASE original real death credit=" << victory << std::endl;
    Require(!victory && owner.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE, "Could not reproduce the missing Razlo death credit");
    std::cout << "EXPECTED_BROKEN the original gossip never started combat and an actual Razlo death gave no victory receipt\n";
}
#else
struct AssaultParty : Group
{
    AssaultParty(LootPlayer& first, LootPlayer& second) : members{&first, &second}
    {
        this->*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(first.GetGUID().GetCounter(), 0);
        for (LootPlayer* player : members)
        {
            MemberSlot slot;
            slot.Guid = player->GetGUID();
            slot.Class = player->getClass();
            const_cast<MemberSlotList&>(GetMemberSlots()).push_back(slot);
            player->SetGroup(this, 0);
        }
    }

    ~AssaultParty()
    {
        for (LootPlayer* player : members)
            player->SetGroup(nullptr);
    }

    std::array<LootPlayer*, 2> const members;
};

void CheckDownWithCrushcog()
{
    for (uint32 spawn : {JarviSpawn, TinkerSpawn, StonegrindSpawn, RazloSpawn})
        sObjectMgr->RemoveCreatureFromGrid(spawn, sObjectMgr->GetCreatureData(spawn));
    for (uint32 spawn : GuardianSpawns)
        sObjectMgr->RemoveCreatureFromGrid(spawn, sObjectMgr->GetCreatureData(spawn));

    Map map(0, 622, 0, DIFFICULTY_NONE);
    IceflowPlacement jarvi(map, JarviSpawn), tinker(map, TinkerSpawn), stonegrind(map, StonegrindSpawn), razlo(map, RazloSpawn);
    std::vector<std::unique_ptr<IceflowPlacement>> guardians;
    for (uint32 spawn : GuardianSpawns)
        guardians.push_back(std::make_unique<IceflowPlacement>(map, spawn));
    LootPlayer owner(SupplySession(), map, 2636401, jarvi.GetPosition()),
        ally(SupplySession(), map, 2636402, jarvi.GetPosition()), outsider(SupplySession(), map, 2636403, jarvi.GetPosition());
    Position const stand = tinker.GetNearPosition(2.0f, 0.0f);
    Position const target = razlo.GetPosition();
    Position const tinkerHome = tinker.GetHomePosition();
    Position const stonegrindHome = stonegrind.GetHomePosition();
    auto advance = [&](uint32 duration)
    {
        for (uint32 elapsed = 0; elapsed < duration; elapsed += 20)
        {
            clockMs += 20;
            owner.Unit::Update(20);
            ally.Unit::Update(20);
            outsider.Unit::Update(20);
            tinker.Update(20);
            stonegrind.Update(20);
            if (razlo.IsAlive())
                razlo.Update(20);
            for (auto const& guardian : guardians)
                if (guardian->IsAlive())
                    guardian->Update(20);
            if (Creature* pilot = razlo.FindNearestCreature(42494, 10.0f, true))
                pilot->Unit::Update(20);
            map.MoveAllCreaturesInMoveList();
            map.RemoveAllObjectsInRemoveList();
        }
    };
    auto select = [&](LootPlayer& player)
    {
        MoveGilneasPlayer(player, tinker.GetNearPosition(2.0f, 0.0f));
        player.PrepareGossipMenu(&tinker, 11662, true);
        Require(player.PlayerTalkClass->GetGossipMenu().GetItem(0), "The native Mekkatorque option was hidden");
        SelectAssault(player, tinker);
    };
    auto use = [&](LootPlayer& player, Position const& destination)
    {
        return player.CastSpell(destination, GroundTarget, false, player.GetItemByEntry(Device));
    };
    for (LootPlayer* player : {&owner, &ally, &outsider})
        AcceptAssault(*player, jarvi);

    auto kit = razlo.GetVehicleKit();
    advance(1000);
    Require(kit && kit->GetPassenger(0) && kit->GetPassenger(0)->GetEntry() == 42494, "Razlo did not load the native vehicle and pilot");
    MoveGilneasPlayer(owner, stand);
    Require(use(owner, target) != SPELL_CAST_OK && !razlo.isInCombat(), "The device started the battle without the Mekkatorque option");
    select(owner);
    Require(owner.GetQuestObjectiveData(QuestId, Tinker) == 1, "The first genuine selection did not credit the Mekkatorque conversation");
    AbandonSupply(owner, QuestId);
    advance(1000);
    Require(!owner.GetItemCount(Device) && !razlo.isInCombat(), "An abandoned encounter retained the device or reached battle");
    AcceptAssault(owner, jarvi);

    AssaultParty group(owner, ally);
    MoveGilneasPlayer(ally, stand);
    select(owner);
    select(ally);
    for (uint32 elapsed = 0; elapsed < 75000 && !razlo.isInCombat(); elapsed += 500)
        advance(500);
    std::cout << "CASE native approach distance=" << tinker.GetDistance(&razlo)
        << " boss combat=" << razlo.isInCombat() << " allied combat=" << stonegrind.isInCombat()
        << " allied distance from Razlo=" << stonegrind.GetDistance(&razlo)
        << " allied home distance from Razlo=" << stonegrind.GetHomePosition().GetExactDist(&target)
        << " friendly=" << stonegrind.IsFriendlyTo(&razlo)
        << " can attack=" << stonegrind.canCreatureAttack(&razlo)
        << " Razlo can attack=" << razlo.canCreatureAttack(&stonegrind) << std::endl;
    Require(razlo.isInCombat() && stonegrind.isInCombat(), "The original approach did not start the fight");

    for (LootPlayer* player : {&owner, &ally, &outsider})
        MoveGilneasPlayer(*player, razlo.GetNearPosition(6.0f, 0.0f));
    Require(use(outsider, target) != SPELL_CAST_OK && use(owner, jarvi.GetPosition()) != SPELL_CAST_OK &&
        owner.CastSpell(target, GroundTarget, false) != SPELL_CAST_OK, "The device accepted an outsider, invalid destination or itemless cast");
    auto health = razlo.GetHealth();
    Require(use(owner, razlo.GetPosition()) == SPELL_CAST_OK, "The genuine orbital device cast did not start");
    advance(700);
    Creature* marker = owner.FindNearestCreature(42929, 25.0f, true);
    Require(marker && marker->HasQuestObjectScope() && owner.canSeeOrDetect(marker) && !outsider.canSeeOrDetect(marker),
        "The orbital device did not publish a marker scoped to its own attempt");
    std::cout << "CASE orbital distance=" << marker->GetDistance(&razlo)
        << " weapon shout=" << !(sCreatureTextMgr->*Access(AssaultTextTag{}))(&tinker, 5).empty()
        << " spell speed=" << sSpellMgr->GetSpellInfo(80098)->GetMisc()->MiscData.Speed << std::endl;
    advance(1700);
    std::cout << "CASE orbital hit on actual Razlo health=" << health << " -> " << razlo.GetHealth() << std::endl;
    Require(razlo.IsAlive() && health > razlo.GetHealth() + razlo.CountPctFromMaxHealth(3),
        "The original marker failed to trigger orbital damage or consumed the reusable device");
    Require(owner.GetItemCount(Device) == 1, "The orbital device lost its reusable charge");
    Require(!outsider.DealDamage(&razlo, 1, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false),
        "A nonparticipant could damage the encounter boss");
    LootPet pet(owner, map);
    pet.setFaction(owner.getFaction());
    razlo.SetHealth(1);
    Require(pet.DealDamage(&razlo, 1, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) && !razlo.IsAlive(),
        "A participant's pet did not kill the real Razlo");
    advance(1200);
    std::cout << "CASE real Razlo death credit=" << owner.GetQuestObjectiveData(QuestId, DefeatedCredit) << ','
        << ally.GetQuestObjectiveData(QuestId, DefeatedCredit) << ',' << outsider.GetQuestObjectiveData(QuestId, DefeatedCredit) << std::endl;
    Require(owner.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE && ally.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE &&
        !outsider.GetQuestObjectiveData(QuestId, DefeatedCredit), "The death did not complete exactly the eligible players' quests");

    MoveGilneasPlayer(owner, jarvi.GetNearPosition(2.0f, 0.0f));
    Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId);
    uint32 choice = 0;
    if (auto items = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (QuestPackageItemEntry const* item : *items)
            if (owner.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    if (!choice)
        if (auto fallback = sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID); fallback && !fallback->empty())
            choice = fallback->front()->ItemID;
    std::cout << "CASE Jarvi package=" << quest->PackageID << " item=" << choice << " interaction="
        << owner.CanInteractWithQuestGiver(&jarvi) << std::endl;
    Require(owner.CanInteractWithQuestGiver(&jarvi) && owner.CanRewardQuest(quest, choice, false), "Jarvi's native reward is not available");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = QuestId;
    reward.QuestGiverGUID = jarvi.GetGUID();
    reward.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(QuestId) && !owner.GetItemCount(Device), "Jarvi did not award the quest and remove the device");
    advance(12000);
    Require(tinker.GetHomePosition().GetExactDist(&tinkerHome) < 0.01f &&
        stonegrind.GetHomePosition().GetExactDist(&stonegrindHome) < 0.01f &&
        !tinker.isInCombat() && !stonegrind.isInCombat(), "The completed assault did not restore the allies' original homes and stop combat");
    std::cout << "PASS native vehicle, gossip, movement, item impact, pet death, group eligibility, abandonment and Jarvi reward\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}});
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The assault probe requires isolated characters");
        LoadDownWithCrushcogInputs();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckDownWithCrushcog();
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
