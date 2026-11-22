#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "CharmInfo.h"
#include "ConditionMgr.h"
#include "CreatureTextMgr.h"
#include "GilneasQuestNative.h"
#include "MoveSpline.h"
#include "PetPackets.h"
#include "QuestItemObjectUse.h"
#include "TemporarySummon.h"

void AddSC_gilneas();

namespace
{
using GilneanMastiff::AttackLurker;
using GilneanMastiff::Collar;
using GilneanMastiff::Lurker;
using GilneanMastiff::Mastiff;
using GilneanMastiff::Summon;
constexpr uint32 QuestId = GilneanMastiff::Quest;
constexpr uint32 Bite = 72870;
constexpr uint32 Stealth = 5916;
constexpr uint32 Arsenal = 14159;
constexpr uint32 FollowUp = 14214;

void LoadShadowsInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeFromTheShadows.inc"
    );
    Require(rows && rows->GetRowCount() == 3, "The original Gilneas chain is missing");
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
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN(14204,14214)");
    Require(addon && addon->GetRowCount() == 2, "The collar and follow-up quest data are absent");
    do
    {
        auto row = addon->Fetch();
        auto quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        if (quest->PrevQuestID)
            quest->prevQuests.push_back(quest->PrevQuestID);
    } while (addon->NextRow());
    auto quest = quests[QuestId];
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=14204");
    Require(objective && objective->GetRowCount() == 1, "The real Lurker objective is missing");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    Require(quest->SourceItemId == Collar && quest->PackageID == 575 && quests[FollowUp]->PrevQuestID == QuestId &&
        quest->GetObjectives()[0].ObjectID == Lurker && quest->GetObjectives()[0].Amount == 6, "The original collar, kill goal or reward changed");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    auto collar = sObjectMgr->GetItemTemplate(Collar);
    auto summon = sSpellMgr->GetSpellInfo(Summon)->GetEffect(EFFECT_0);
    auto command = sSpellMgr->GetSpellInfo(AttackLurker);
    Require(collar && std::any_of(collar->Effects.begin(), collar->Effects.end(), [](ItemEffectEntry const* effect)
        { return effect && effect->SpellID == Summon && !effect->Charges; }) && summon->MiscValue == Mastiff && summon->MiscValueB == 2161 &&
        command->GetEffect(EFFECT_0)->Effect == SPELL_EFFECT_JUMP && command->GetEffect(EFFECT_0)->TargetA.GetTarget() == TARGET_UNIT_NEARBY_ENTRY,
        "The original reusable collar or Mastiff leap changed");
    std::cout << "Original command range " << command->GetMaxRange(false) << '\n';

    auto& lootTable = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto original = lootTable;
    lootTable += " WHERE Entry=35463";
    auto loaded = (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    lootTable = original;
    Require(loaded == 8, "The actual Bloodfang corpse loot is absent");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(14204,14214)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=14204");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text WHERE CreatureID=35378");
#ifndef FROM_THE_SHADOWS_BASELINE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=67805");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sCreatureTextMgr->LoadCreatureTexts();
#ifndef FROM_THE_SHADOWS_BASELINE
    sConditionMgr->LoadConditions();
#endif
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_gilneas();
    sScriptMgr->SwapScriptContext(true);
}

TempSummon* FindMastiff(LootPlayer& player)
{
    for (ObjectGuid const& guid : *player.GetSummonList(Mastiff))
        if (Creature* actor = player.GetMap()->GetCreature(guid); actor && actor->IsAlive())
            return actor->ToTempSummon();
    return nullptr;
}

SpellCastResult UseCollar(LootPlayer& player)
{
    SpellCastTargets targets;
    targets.SetCaster(&player);
    return player.CastSpell(targets, sSpellMgr->GetSpellInfo(Summon), nullptr, TRIGGERED_NONE, player.GetItemByEntry(Collar));
}

void AcceptShadows(LootPlayer& player, Creature& lorna)
{
    MoveGilneasPlayer(player, lorna.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = lorna.GetGUID();
    accept.QuestID = QuestId;
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Tick(player, 2000);
    Require(player.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && player.GetItemCount(Collar) == 1 && FindMastiff(player),
        "Native acceptance did not provide the Collar and summoned Mastiff");
}

void CheckFromTheShadows()
{
    auto lornaRow = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE id=35378 AND guid=148484 AND map=654");
    auto lurkerRows = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE id=35463 AND map=654 ORDER BY guid");
    Require(lornaRow && lurkerRows && lurkerRows->GetRowCount() == 12, "The original Lorna or 12 Bloodfang Lurker positions are missing");
    Map map(654, 605, 0, DIFFICULTY_NONE);
    GilneasActor lorna(map, lornaRow->Fetch());
    Require(lorna.AIM_Initialize(), "Lorna's registered quest script did not initialize");
    LootPlayer owner(SupplySession(), map, 1420401, lorna.GetPosition()), observer(SupplySession(), map, 1420402, lorna.GetPosition());
    for (LootPlayer* player : {&owner, &observer})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_WORGEN);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(RACE_WORGEN);
        player->InitDisplayIds();
        player->GetReputationMgr().LoadFromDB({});
        player->SetLevel(8);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE, UNIT_FIELD_MOD_HASTE_REGEN})
            player->SetFloatValue(field, 1.0f);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        player->SetRewardedQuest(Arsenal);
        player->AddAura(Bite, player);
        MoveGilneasPlayer(*player, lorna.GetPosition());
        Require(player->HasAura(Bite) && player->HasPhaseId(171) && player->InSamePhase(&lorna), "The native Worgen Bite phase did not reveal Lorna");
    }

    GiveSupply(observer, Collar, 1);
    auto staleCast = UseCollar(observer);
    Tick(observer, 2000);
#ifndef FROM_THE_SHADOWS_BASELINE
    Pet* nativePet = owner.CreateTamedPetFrom(299);
    Require(nativePet && map.AddToMap(static_cast<Creature*>(nativePet)), "Could not create an ordinary persistent Hunter pet in the private character fixture");
    owner.SetMinion(nativePet, true);
    nativePet->SavePetToDB(PET_SAVE_AS_CURRENT);
    uint32 petNumber = nativePet->GetCharmInfo()->GetPetNumber();
    Require(petNumber && owner.GetPet() == nativePet, "The ordinary Hunter pet was not active before accepting Lorna's quest");
    auto hasOriginalPet = [&owner, petNumber]()
    {
        Pet* pet = owner.GetPet();
        return pet && pet->GetCharmInfo()->GetPetNumber() == petNumber;
    };
#endif
    AcceptShadows(owner, lorna);
    TempSummon* mastiff = FindMastiff(owner);
#ifdef FROM_THE_SHADOWS_BASELINE
    Require(!mastiff->GetScriptId() && staleCast == SPELL_CAST_OK && FindMastiff(observer),
        "The original wrong script binding or stale Collar use no longer reproduces");
    AbandonSupply(owner, QuestId);
    Tick(owner, 2000);
    Require(FindMastiff(owner) == mastiff, "The original abandoned guardian no longer survives its quest");
    FindMastiff(observer)->UnSummon();
    mastiff->UnSummon();
    map.RemoveAllObjectsInRemoveList();
    std::cout << "EXPECTED_BROKEN real 35631 lacks the registered AI, an inactive collar works, and abandonment keeps the Mastiff\n";
#else
    Require(staleCast == SPELL_FAILED_BAD_TARGETS && !FindMastiff(observer) && mastiff->GetScriptId() &&
        mastiff->HasQuestObjectScope() && mastiff->GetOwner() == &owner && owner.GetFirstControlled() == mastiff &&
        !owner.GetPet() && mastiff->GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE && !observer.canSeeOrDetect(mastiff),
        "The real quest guardian did not follow its actual owner in private");
    Require(UseCollar(owner) == SPELL_FAILED_BAD_TARGETS, "A second collar replaced the original live guardian");
    AbandonSupply(owner, QuestId);
    map.RemoveAllObjectsInRemoveList();
    Require(!FindMastiff(owner) && !owner.GetItemCount(Collar) && hasOriginalPet(),
        "Abandonment retained the guardian or Collar, or did not restore the original Hunter pet");

    AcceptShadows(owner, lorna);
    mastiff = FindMastiff(owner);
    ObjectGuid expired = mastiff->GetGUID();
    mastiff->UnSummon();
    map.RemoveAllObjectsInRemoveList();
    Require(!FindMastiff(owner) && hasOriginalPet() && UseCollar(owner) == SPELL_CAST_OK,
        "Losing the Mastiff did not restore its ordinary pet or allow the actual Collar to replace it");
    Tick(owner, 2000);
    mastiff = FindMastiff(owner);
    Require(mastiff && mastiff->GetGUID() != expired && owner.GetFirstControlled() == mastiff,
        "Native Collar use did not return the owner's controllable Mastiff");
    for (uint32 count = 1; count <= 6; ++count)
    {
        GilneasActor lurker(map, lurkerRows->Fetch());
        Require(lurker.AIM_Initialize(), "An actual Lurker failed to initialize its native stealth script");
        Position approach = lurker.GetNearPosition(4.5f, 0.0f);
        MoveGilneasPlayer(owner, approach);
        if (count == 1)
        {
            map.CreatureRelocation(mastiff, approach.GetPositionX(), approach.GetPositionY(), approach.GetPositionZ(), approach.GetOrientation());
            map.MoveAllCreaturesInMoveList();
            std::cout << "Mastiff ready: known=" << mastiff->HasSpell(AttackLurker) << ", distance=" << mastiff->GetDistance(&lurker)
                << ", LOS=" << mastiff->IsWithinLOSInMap(&lurker) << ", same phase=" << mastiff->InSamePhase(&lurker) << '\n';
            Require(lurker.HasAura(Stealth) && mastiff->IsWithinDistInMap(&lurker, 30.0f) && mastiff->IsWithinLOSInMap(&lurker),
                "The actual test alley cannot exercise the hidden target");
            Require(observer.CastSpell(&lurker, AttackLurker, true) != SPELL_CAST_OK && lurker.HasAura(Stealth),
                "A foreign caster can use the real Mastiff's skill");
            WorldPackets::PetPackets::PetAction command{WorldPacket(CMSG_PET_ACTION)};
            command.PetGUID = mastiff->GetGUID();
            command.Action = MAKE_UNIT_ACTION_BUTTON(AttackLurker, ACT_PASSIVE);
            owner.GetSession()->HandlePetAction(command);
            std::cout << "After pet command: stealth=" << lurker.HasAura(Stealth) << ", motion="
                << uint32(mastiff->GetMotionMaster()->GetCurrentMovementGeneratorType()) << ", cast cooldown="
                << mastiff->HasCreatureSpellCooldown(AttackLurker) << ", spline duration=" << mastiff->movespline->Duration() << '\n';
            for (uint32 i = 0; i < 2000; i += 10)
            {
                mastiff->Update(10);
                map.MoveAllCreaturesInMoveList();
            }
            std::cout << "After movement: stealth=" << lurker.HasAura(Stealth) << ", motion="
                << uint32(mastiff->GetMotionMaster()->GetCurrentMovementGeneratorType()) << ", melee range=" << mastiff->IsWithinMeleeRange(&lurker)
                << ", spline elapsed=" << mastiff->movespline->GetTimePassed() << ", victim=" << (mastiff->getVictim() ? mastiff->getVictim()->GetEntry() : 0) << std::endl;
            Require(!lurker.HasAura(Stealth) && mastiff->getVictim() == &lurker, "The genuine Pet Action did not reveal, leap to and attack the actual Lurker");
        }
        Unit* killer = count == 1 ? static_cast<Unit*>(mastiff) : static_cast<Unit*>(&owner);
        Require(killer->DealDamage(&lurker, 1000000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) && lurker.isDead(false),
            "A Bloodfang Lurker did not die to a native strike");
        Tick(owner, 100);
        Require(owner.GetQuestObjectiveData(QuestId, Lurker) == int32(count) && !observer.GetQuestObjectiveData(QuestId, Lurker),
            "A native player or guardian last hit did not reach the proper eligible recipient");
        if (count < 6)
            Require(lurkerRows->NextRow(), "The actual Lurker list ended early");
    }

    MoveGilneasPlayer(owner, lorna.GetPosition());
    auto quest = sQuestDataStore->GetQuestTemplate(QuestId);
    uint32 choice = 0;
    if (auto choices = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (auto item : *choices)
            if (owner.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    if (!choice)
        if (auto fallback = sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID); fallback && !fallback->empty())
            choice = fallback->front()->ItemID;
    Require(choice && owner.CanRewardQuest(quest, choice, false), "Lorna's original class package is unavailable");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = QuestId;
    reward.QuestGiverGUID = lorna.GetGUID();
    reward.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    map.RemoveAllObjectsInRemoveList();
    Require(owner.GetQuestRewardStatus(QuestId) && !owner.GetItemCount(Collar) && !FindMastiff(owner) && hasOriginalPet() && lorna.hasQuest(FollowUp),
        "Lorna did not take the Collar, dismiss the guardian and retain the follow-up");
    owner.RemovePet(owner.GetPet(), PET_SAVE_AS_DELETED);
    map.RemoveAllObjectsInRemoveList();
    std::cout << "PASS actual Lorna accept and reward, scoped Mastiff and original Pet Action, six deaths including guardian credit, abandon and retry\n";
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
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Shadows checks require isolated characters");
        LoadShadowsInputs();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckFromTheShadows();
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
