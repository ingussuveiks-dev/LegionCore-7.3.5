#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"

#include "GossipData.h"
#include "GossipDef.h"
#include "CreatureTextMgr.h"
#include "NPCPackets.h"
#include "QuestPackets.h"
#include "ScriptsData.h"
#include "Spell.h"

void AddSC_gilneas();

namespace
{
constexpr uint32 Battle = 24904;
constexpr uint32 PushThemOut = 24676;
constexpr uint32 BattlePhase = 69485;
constexpr uint32 KrennanMenu = 11061;
constexpr uint32 Rapier = 50334;
constexpr uint32 Inspire = 71388;

struct BattlePet : LootPet
{
    BattlePet(LootPlayer& owner, Map& map) : LootPet(owner, map)
    {
        m_ControlledByPlayer = true;
    }
};

void LoadBattleInputs(bool baseline, bool opening)
{
    auto row = WorldDatabase.Query(
#include "NativeGilneasRapier.inc"
    );
    Require(row && row->GetRowCount() == 4, "Missing the actual Worgen phase, prerequisite or battle quest");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(row->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (row->NextRow());

    auto addons = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN(24676,24904)");
    Require(addons && addons->GetRowCount() == 2, "Missing the original battle prerequisites");
    do
    {
        auto fields = addons->Fetch();
        auto quest = quests[fields[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(fields);
        quest->prevQuests.push_back(quest->PrevQuestID);
    } while (addons->NextRow());
    Require(quests[PushThemOut]->PrevQuestID == 24575 && quests[Battle]->PrevQuestID == PushThemOut,
        "The original Liberation Day and Push Them Out requirements must remain in order");
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN(24676,24904)");
    Require(objectives && objectives->GetRowCount() == 4, "The original combat and victory objectives are required");
    do
    {
        auto fields = objectives->Fetch();
        auto quest = quests[fields[1].GetUInt32()];
        quest->LoadQuestObjective(fields);
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    } while (objectives->NextRow());

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadEquipmentTemplates();
    sSpellMgr->LoadSpellProcEvents();
    sSpellMgr->LoadSpellPrcoCheck();
    sSpellMgr->LoadSpellProcs();
    auto item = sObjectMgr->GetItemTemplate(Rapier);
    auto spell = sSpellMgr->GetSpellInfo(Inspire);
    Require(item && std::any_of(item->Effects.begin(), item->Effects.end(), [](ItemEffectEntry const* effect)
        { return effect->TriggerType == ITEM_SPELLTRIGGER_ON_USE && effect->SpellID == Inspire; }) &&
        spell && spell->GetEffect(EFFECT_0)->TargetA.GetTarget() == TARGET_UNIT_DEST_AREA_ENTRY &&
        spell->GetEffect(EFFECT_1)->TargetA.GetTarget() == TARGET_UNIT_DEST_AREA_ENTRY, "The actual item and both native area effects are required");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature "
        "WHERE guid IN(148102,148103,148108,148114,148116,148208,148209,148213,148225,148228,148229,149656,149660,149706)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(24676,24904)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest IN(24676,24904)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_text AS SELECT * FROM legion_world.npc_text WHERE ID IN(15377)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu AS SELECT * FROM legion_world.gossip_menu WHERE MenuID=11061");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu_option AS SELECT * FROM legion_world.gossip_menu_option WHERE MenuID=11061");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area "
        "WHERE (spell=68483 AND area=4714 AND quest_start=14386) OR spell=71388 OR (spell=69485 AND quest_start=24676)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceEntry=71388 AND SourceTypeOrReferenceId=13");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id=71388");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_loot_template AS SELECT * FROM legion_world.creature_loot_template "
        "WHERE Entry IN(37685,37686,37692)");
    if (opening)
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text "
            "WHERE (CreatureID=38553 AND GroupID=0) OR (CreatureID=38218 AND GroupID BETWEEN 30 AND 36)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    Require((LootTemplates_Creature.*Access(LoadLootTableTag{}))() > 0, "Missing the prerequisite enemies' original loot data");
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sObjectMgr->LoadNPCText();
    sGossipDataStore->LoadGossipMenu();
    sGossipDataStore->LoadGossipMenuItems();
    if (opening)
        sCreatureTextMgr->LoadCreatureTexts();
    sSpellMgr->LoadSpellAreas();
    if (!baseline)
    {
        sConditionMgr->LoadConditions();
        sScriptDataStore->LoadSpellScriptNames();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_gilneas();
        sScriptMgr->SwapScriptContext(true);
        sScriptDataStore->ValidateSpellScripts();
    }
}

Creature& BattleActor(Map& map, uint32 entry, uint64 guid)
{
    auto spawn = sObjectMgr->GetCreatureData(guid);
    Require(spawn && spawn->id == entry, "The actual battle actor placement is required");
    map.LoadGrid(spawn->posX, spawn->posY);
    auto actor = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(654, entry, guid));
    Require(actor && actor->IsAlive(), "The actual battle actor did not enter the map");
    return *actor;
}

void MoveForRapier(LootPlayer& player, Position const& position)
{
    auto map = player.GetMap();
    map->PlayerRelocation(&player, position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(), position.GetOrientation());
    map->GetZoneAndAreaId(player.m_zoneId, player.m_areaId, position.GetPositionX(), position.GetPositionY(), position.GetPositionZ());
    player.UpdateZoneDependentAuras(player.GetZoneId());
    player.UpdateAreaDependentAuras(player.GetAreaId());
    player.GetPhaseMgr().Recalculate();
    player.GetPhaseMgr().Update();
}

void UseRapier(LootPlayer& player, Position const& target)
{
    auto item = player.GetItemByEntry(Rapier);
    Require(item && item->IsInWorld(), "Lorna's genuine starting item is required");
    SpellCastTargets targets;
    targets.SetDst(target);
    int32 misc[2] = {};
    player.CastItemUseSpell(item, targets, misc, {});
    Tick(player, 600);
}

void AcceptAtLorna(LootPlayer& player, Creature const& lorna, uint32 quest)
{
    MoveForRapier(player, lorna.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = lorna.GetGUID();
    accept.QuestID = quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
}

void FinishPushThemOut(LootPlayer& player, Creature const& lorna)
{
    AcceptAtLorna(player, lorna, Battle);
    Require(player.GetQuestStatus(Battle) == QUEST_STATUS_NONE && !player.GetItemByEntry(Rapier),
        "The Battle cannot start before Push Them Out is rewarded");
    AcceptAtLorna(player, lorna, PushThemOut);
    Require(player.GetQuestStatus(PushThemOut) == QUEST_STATUS_INCOMPLETE && !player.HasAura(BattlePhase),
        "Lorna should admit Push Them Out without activating the battle phase");

    auto& map = *player.GetMap();
    auto defeat = [&](uint32 entry, uint64 guid, bool petFinalBlow)
    {
        auto& enemy = BattleActor(map, entry, guid);
        MoveForRapier(player, enemy.GetPosition());
        Require(player.InSamePhase(&enemy) && player.IsValidAttackTarget(&enemy), "The real prerequisite enemy must be reachable");
        if (petFinalBlow)
        {
            BattlePet pet(player, map);
            pet.SetPhaseMask(player.GetPhaseMask(), false);
            pet.DealDamage(&enemy, uint32(enemy.GetMaxHealth()), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
        }
        else
            player.DealDamage(&enemy, uint32(enemy.GetMaxHealth()), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
        Tick(player, 200);
        Require(enemy.isDead(false), "A real prerequisite enemy did not die");
    };
    defeat(37692, 148209, false);
    defeat(37692, 148213, true);
    defeat(37692, 148228, false);
    defeat(37692, 148229, true);
    defeat(37686, 148225, false);
    Require(player.GetQuestStatus(PushThemOut) == QUEST_STATUS_INCOMPLETE,
        "Four infantry and Cornell must still require Valnov");
    defeat(37685, 149706, true);
    Require(player.GetQuestStatus(PushThemOut) == QUEST_STATUS_COMPLETE && !player.HasAura(BattlePhase),
        "All six real deaths must complete Push Them Out before its reward applies the next phase");

    MoveForRapier(player, lorna.GetPosition());
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = lorna.GetGUID();
    reward.QuestID = PushThemOut;
    reward.ItemChoiceID = 0;
    player.GetSession()->HandleQuestGiverChooseReward(reward);
    MoveForRapier(player, lorna.GetPosition());
    Require(player.GetQuestRewardStatus(PushThemOut) && player.HasAura(BattlePhase),
        "Lorna's real reward must open the native battle phase");
    std::cout << "PASS Push Them Out admits six real player or pet kills, rewards at Lorna and opens the battle phase\n";
}

void ReportBattleAdmission(LootPlayer& player, Creature& krennan)
{
    MoveForRapier(player, krennan.GetPosition());
    Require(player.GetAreaId() == 4842 && player.InSamePhase(&krennan) && player.IsWithinLOSInMap(&krennan),
        "Krennan's original Livery actor must be visible after the prerequisite");
    auto& map = *player.GetMap();
    auto& liam = BattleActor(map, 38218, 148114);
    auto& almyra = BattleActor(map, 38466, 148102);
    auto& myriam = BattleActor(map, 38465, 148103);
    auto bounds = sGossipDataStore->GetGossipMenuItemsMapBounds(KrennanMenu);
    Require(std::distance(bounds.first, bounds.second) == 1 && bounds.first->second.OptionBroadcastTextID == 38484,
        "The original Krennan option is required in the catalog");

    WorldPackets::NPC::Hello hello{WorldPacket(CMSG_TALK_TO_GOSSIP)};
    hello.Unit = krennan.GetGUID();
    player.GetSession()->HandleGossipHelloOpcode(hello);
    auto& menu = player.PlayerTalkClass->GetGossipMenu();
    std::cout << "OBSERVED Krennan menu=" << menu.GetMenuId() << " choices=" << menu.GetMenuItemCount()
        << " Liam distance=" << krennan.GetDistance(&liam) << " Liam phase=" << player.InSamePhase(&liam)
        << " Almyra phase=" << player.InSamePhase(&almyra) << " Myriam phase=" << player.InSamePhase(&myriam) << '\n';
}

void CheckOpening()
{
    constexpr uint32 Liam = 38218;
    constexpr uint32 Speech = 1;
    constexpr uint32 March = 2;
    constexpr uint32 Assault = 3;
    constexpr uint32 FirstWaveCleared = 4;
    Map map(654, 838, 0, DIFFICULTY_NONE);
    auto& lorna = BattleActor(map, 37783, 149656);
    auto& krennan = BattleActor(map, 38553, 149660);
    auto first = new WorldSession(0, "Native Gilneas opening", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    auto second = new WorldSession(0, "Native Gilneas opening visitor", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    LootPlayer owner(first, map, 2490406, lorna.GetPosition());
    LootPlayer visitor(second, map, 2490407, lorna.GetPosition());
    for (auto player : {&owner, &visitor})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_WORGEN);
        player->setFactionForRace(RACE_WORGEN);
        player->InitDisplayIds();
        player->SetLevel(15);
        player->SetRewardedQuest(14386);
        player->SetRewardedQuest(PushThemOut);
    }

    auto speak = [&](LootPlayer& player)
    {
        MoveForRapier(player, krennan.GetPosition());
        WorldPackets::NPC::Hello hello{WorldPacket(CMSG_TALK_TO_GOSSIP)};
        hello.Unit = krennan.GetGUID();
        player.GetSession()->HandleGossipHelloOpcode(hello);
        auto const& menu = player.PlayerTalkClass->GetGossipMenu();
        return menu.GetMenuId() == KrennanMenu && menu.GetMenuItemAction(0) == Battle;
    };
    auto select = [&](LootPlayer& player, uint32 id, uint32 index)
    {
        WorldPackets::NPC::GossipSelectOption selection{WorldPacket(CMSG_GOSSIP_SELECT_OPTION)};
        selection.GossipUnit = krennan.GetGUID();
        selection.GossipID = id;
        selection.GossipIndex = index;
        player.GetSession()->HandleGossipSelectOption(selection);
    };
    auto rootFor = [&](LootPlayer& player) -> Creature*
    {
        auto const& roots = *player.GetSummonList(Liam);
        Require(roots.size() <= 1, "More than one opening was created for the same accepted quest");
        return roots.empty() ? nullptr : map.GetCreature(roots.front());
    };
    auto abandon = [&](LootPlayer& player)
    {
        WorldPackets::Quest::QuestLogRemoveQuest request{WorldPacket(CMSG_QUEST_LOG_REMOVE_QUEST)};
        request.Entry = player.FindQuestSlot(Battle);
        player.GetSession()->HandleQuestLogRemoveQuest(request);
        map.RemoveAllObjectsInRemoveList();
    };

    Require(krennan.GetScriptName() == "npc_gilneas_battle_krennan", "The real opening script is not bound in the isolated fixture");
    Require(!speak(owner), "Krennan offered the battle before acceptance");
    select(owner, KrennanMenu, 0);
    Require(!rootFor(owner), "A forged early option created a battle");
    AcceptAtLorna(owner, lorna, Battle);
    AcceptAtLorna(visitor, lorna, Battle);
    Require(speak(owner), "The accepted Worgen did not see Krennan's genuine menu");
    select(owner, KrennanMenu + 1, 0);
    select(owner, KrennanMenu, 1);
    Require(!rootFor(owner), "A mismatched menu or option created the opening");
    select(owner, KrennanMenu, 0);
    auto liam = rootFor(owner);
    Require(liam && liam->AI()->GetData(Battle) == Speech && liam->HasQuestObjectScope() &&
        liam->IsInPersonnalVisibilityList(owner.GetGUID()) && !liam->IsInPersonnalVisibilityList(visitor.GetGUID()),
        "The real selection must publish the owner's Liam only");
    ObjectGuid liamGuid = liam->GetGUID();
    select(owner, KrennanMenu, 0);
    Require(rootFor(owner) == liam && !speak(owner), "The opening must consume repeat selections while the attempt is active");

    Require(speak(visitor), "A separate accepted owner could not open their own attempt");
    select(visitor, KrennanMenu, 0);
    auto otherLiam = rootFor(visitor);
    Require(otherLiam && otherLiam->GetQuestObjectScope().lock() != liam->GetQuestObjectScope().lock() &&
        !otherLiam->IsInPersonnalVisibilityList(owner.GetGUID()), "Concurrent attempts share visibility or scope");
    ObjectGuid otherGuid = otherLiam->GetGUID();
    abandon(visitor);
    Require(!map.GetCreature(otherGuid) && map.GetCreature(liamGuid), "Abandoning one attempt affected the other owner");
    AcceptAtLorna(visitor, lorna, Battle);

    auto lift = [&](bool follow, bool updateTroops = true)
    {
        constexpr uint32 tick = 100;
        Require(map.GetCreature(liamGuid) == liam, "The active Liam disappeared during the opening");
        if (follow && owner.GetExactDist(liam) > 12.0f)
            MoveForRapier(owner, liam->GetPosition());
        Tick(owner, tick);
        liam->Update(tick);
        for (uint32 entry : {38465u, 38466u, 38221u, 38210u})
        {
            if (!updateTroops && (entry == 38221 || entry == 38210))
                continue;
            GuidList children = *liam->GetSummonList(entry);
            for (ObjectGuid guid : children)
                if (auto child = map.GetCreature(guid))
                    child->Update(tick);
        }
        map.MoveAllCreaturesInMoveList();
        map.RemoveAllObjectsInRemoveList();
    };
    for (uint32 step = 0; step < 20; ++step)
        lift(false);
    Require(liam->HasAura(72069) && liam->AI()->GetData(Battle) == Speech && liam->GetSummonList(38210)->empty(),
        "The speech phase must retain the native aura and cannot spawn wave enemies");
    for (uint32 step = 0; step < 1650 && liam->AI()->GetData(Battle) < Assault; ++step)
        lift(true);
    Require(liam->AI()->GetData(Battle) == Assault && liam->GetPositionY() > 1368.0f && liam->GetSummonList(38210)->size() == 10,
        "The first wave must wait for all three actual bridge arrivals before summoning ten enemies");
    std::cout << "PASS Krennan selection, private attachments, duplicate/wrong menu rejection, speech and three spline arrivals\n";

    GuidList hostiles = *liam->GetSummonList(38210);
    auto enemy = map.GetCreature(hostiles.front());
    Require(enemy && enemy->IsAlive(), "Missing actual first-wave hostile");
    MoveForRapier(visitor, enemy->GetPosition());
    visitor.DealDamage(enemy, uint32(enemy->GetMaxHealth()) * 2);
    Require(enemy->IsAlive(), "A different attempt damaged the private crossbowman");

    auto defeat = [&](ObjectGuid guid, bool petFinalBlow)
    {
        auto target = map.GetCreature(guid);
        Require(target && target->IsAlive(), "First-wave hostile disappeared before combat");
        MoveForRapier(owner, target->GetPosition());
        if (petFinalBlow)
        {
            BattlePet pet(owner, map);
            pet.SetPhaseMask(owner.GetPhaseMask(), false);
            pet.SetPhaseId(owner.GetPhases(), false);
            pet.DealDamage(target, uint32(target->GetMaxHealth()) * 2);
        }
        else
            owner.DealDamage(target, uint32(target->GetMaxHealth()) * 2);
        Require(target->isDead(false), "A scoped owner or pet's actual final hit was rejected");
    };
    auto last = hostiles.back();
    hostiles.pop_back();
    bool petFinalBlow = false;
    for (ObjectGuid guid : hostiles)
    {
        defeat(guid, petFinalBlow);
        petFinalBlow = !petFinalBlow;
    }
    for (uint32 step = 0; step < 850; ++step)
        lift(true, false);
    Require(liam->AI()->GetData(Battle) == Assault && map.GetCreature(last) && map.GetCreature(last)->IsAlive(),
        "Route arrival alone must not clear the first wave");
    defeat(last, true);
    for (uint32 step = 0; step < 850 && liam->AI()->GetData(Battle) < FirstWaveCleared; ++step)
        lift(true);
    std::cout << "CASE opening stage=" << liam->AI()->GetData(Battle) << " quest=" << owner.GetQuestStatus(Battle)
        << " victory=" << owner.GetQuestObjectiveData(Battle, 38854) << " Liam=" << liam->GetPositionX() << ',' << liam->GetPositionY();
    for (uint32 entry : {38465u, 38466u})
        for (ObjectGuid guid : *liam->GetSummonList(entry))
            if (Creature* leader = map.GetCreature(guid))
                std::cout << " leader " << entry << '=' << leader->GetPositionX() << ',' << leader->GetPositionY();
    std::cout << '\n';
    Require(liam->AI()->GetData(Battle) == FirstWaveCleared && owner.GetQuestStatus(Battle) == QUEST_STATUS_INCOMPLETE &&
        owner.GetQuestObjectiveData(Battle, 38854) == 0, "The first wave must wait for hostile deaths and the march, with no finale credit");
    abandon(owner);
    Require(!map.GetCreature(liamGuid), "The scoped opening was not removed on abandon");
    AcceptAtLorna(owner, lorna, Battle);
    Require(speak(owner), "An abandoned battle could not be started again after genuine reacceptance");
    select(owner, KrennanMenu, 0);
    Require(rootFor(owner) && rootFor(owner)->GetGUID() != liamGuid && rootFor(owner)->AI()->GetData(Battle) == Speech,
        "The retry reused an earlier attempt");
    abandon(owner);
    abandon(visitor);
    std::cout << "PASS isolated first-wave owner/pet kills, foreign damage rejection, no victory credit, abandon and retry\n";
}

void CheckRapier(bool baseline)
{
    Map map(654, 832, 0, DIFFICULTY_NONE);
    auto& lorna = BattleActor(map, 37783, 149656);
    auto& militia = BattleActor(map, 38221, 148116);
    auto& cityMilitia = BattleActor(map, 38221, 148108);
    auto& unrelated = BattleActor(map, 38553, 149660);
    auto& enemy = BattleActor(map, 38210, 148208);
    auto session = new WorldSession(0, "Native Gilneas rapier", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    LootPlayer player(session, map, 2490401, lorna.GetPosition());
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_WORGEN);
    player.setFactionForRace(RACE_WORGEN);
    player.InitDisplayIds();
    player.SetLevel(15);
    player.SetRewardedQuest(14386);
    player.SetRewardedQuest(24575);
    MoveForRapier(player, lorna.GetPosition());

    auto spell = sSpellMgr->GetSpellInfo(Inspire);
    auto here = [&]() { return spell->CheckLocation(map.GetId(), player.GetZoneId(), player.GetAreaId(), &player); };
    Require(here() == SPELL_FAILED_INCORRECT_AREA, "An unaccepted battle must not admit the rapier");
    FinishPushThemOut(player, lorna);
    AcceptAtLorna(player, lorna, Battle);
    Require(player.GetQuestStatus(Battle) == QUEST_STATUS_INCOMPLETE && player.GetItemByEntry(Rapier), "Lorna did not give the actual quest and rapier");
    ReportBattleAdmission(player, unrelated);

    MoveForRapier(player, militia.GetPosition());
    if (baseline)
    {
        Require(player.GetAreaId() == 4842 && here() == SPELL_FAILED_INCORRECT_AREA,
            "Old rapier admission no longer reproduces the contradictory status gate");
        UseRapier(player, militia.GetPosition());
        Require(!militia.HasAura(Inspire), "The old item unexpectedly inspires a Livery ally");
        std::cout << "EXPECTED_BROKEN the accepted quest's real rapier fails at Livery Outpost\n";
        return;
    }

    auto position = militia.GetPosition();
    auto cityPosition = cityMilitia.GetPosition();
    auto cityPhase = cityMilitia.GetPhaseMask();
    auto enemyPhase = enemy.GetPhaseMask();
    map.CreatureRelocation(&unrelated, position.GetPositionX(), position.GetPositionY() + 1.0f, position.GetPositionZ(), 0.f);
    map.CreatureRelocation(&enemy, position.GetPositionX(), position.GetPositionY() + 2.0f, position.GetPositionZ(), 0.f);
    map.CreatureRelocation(&cityMilitia, position.GetPositionX(), position.GetPositionY() + 3.0f, position.GetPositionZ(), 0.f);
    enemy.SetPhaseMask(player.GetPhaseMask(), false);
    cityMilitia.SetPhaseMask(enemyPhase, false);
    map.MoveAllCreaturesInMoveList();
    Require(player.GetAreaId() == 4842 && here() == SPELL_CAST_OK && player.InSamePhase(&unrelated) &&
        player.InSamePhase(&enemy) && !player.InSamePhase(&cityMilitia), "The test must place the comparison actors in the cast area");
    UseRapier(player, position);
    std::cout << "CASE Livery auras ally0=" << (militia.GetAuraEffect(Inspire, EFFECT_0, player.GetGUID()) != nullptr)
        << " ally1=" << (militia.GetAuraEffect(Inspire, EFFECT_1, player.GetGUID()) != nullptr)
        << " enemy=" << enemy.HasAura(Inspire) << " bystander=" << unrelated.HasAura(Inspire)
        << " otherPhase=" << cityMilitia.HasAura(Inspire) << " caster=" << player.HasAura(Inspire) << '\n';
    Require(militia.GetAuraEffect(Inspire, EFFECT_0, player.GetGUID()) && militia.GetAuraEffect(Inspire, EFFECT_1, player.GetGUID()) &&
        !enemy.HasAura(Inspire) && !unrelated.HasAura(Inspire) && !cityMilitia.HasAura(Inspire) && !player.HasAura(Inspire),
        "Both actual aura effects must reach the battle ally alone");

    map.CreatureRelocation(&cityMilitia, cityPosition.GetPositionX(), cityPosition.GetPositionY(), cityPosition.GetPositionZ(), cityPosition.GetOrientation());
    map.MoveAllCreaturesInMoveList();
    cityMilitia.SetPhaseMask(cityPhase, false);
    MoveForRapier(player, cityPosition);
    Require(player.GetZoneId() == 4755 && here() == SPELL_CAST_OK, "The active rapier should remain usable inside the city");
    player.RemoveSpellCooldown(Inspire);
    player.GetGlobalCooldownMgr().CancelGlobalCooldown(spell);
    UseRapier(player, cityPosition);
    Require(cityMilitia.GetAuraEffect(Inspire, EFFECT_0, player.GetGUID()) &&
        cityMilitia.GetAuraEffect(Inspire, EFFECT_1, player.GetGUID()) && !player.GetQuestObjectiveData(Battle, 38854),
        "The actual city cast must inspire the militia without awarding victory");
    MoveForRapier(player, Position(-2041.39f, 975.445f, 70.1487f));
    Require(here() == SPELL_FAILED_INCORRECT_AREA, "The active rapier escaped the battle route");
    MoveForRapier(player, militia.GetPosition());
    player.SetQuestStatus(Battle, QUEST_STATUS_COMPLETE);
    Require(here() == SPELL_FAILED_INCORRECT_AREA, "Completed battles should not admit a new cast");
    player.SetQuestStatus(Battle, QUEST_STATUS_INCOMPLETE);
    WorldPackets::Quest::QuestLogRemoveQuest abandon{WorldPacket(CMSG_QUEST_LOG_REMOVE_QUEST)};
    abandon.Entry = player.FindQuestSlot(Battle);
    session->HandleQuestLogRemoveQuest(abandon);
    Require(here() == SPELL_FAILED_INCORRECT_AREA && !player.GetItemByEntry(Rapier),
        "Abandoning the battle should clear the item and close casting");
    AcceptAtLorna(player, lorna, Battle);
    Require(player.GetQuestStatus(Battle) == QUEST_STATUS_INCOMPLETE && player.GetItemByEntry(Rapier) && here() == SPELL_CAST_OK,
        "The legitimate retry must receive a rapier that can be used normally");
    std::cout << "PASS original rapier effects at Livery and in the city, eligibility and phase gates, complete, abandon and retry\n";
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
        auto login = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(login.substr(login.find_last_of(';') + 1) == "fixture_auth" &&
            characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Private Gilneas fixture required");
        LoginDatabase.SetConnectionInfo(login, 1, 1);
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto baseline = sConfigMgr->GetBoolDefault("NativeGilneasRapier.Baseline", false);
        auto opening = sConfigMgr->GetBoolDefault("NativeGilneasRapier.Opening", false);
        LoadBattleInputs(baseline, opening);
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<Pet>::SetSize(32);
        GameTime::UpdateGameTimers();
        if (opening)
            CheckOpening();
        else
            CheckRapier(baseline);
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
