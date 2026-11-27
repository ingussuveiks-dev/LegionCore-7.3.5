#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"

#include "QuestPackets.h"
#include "ScriptsData.h"
#include "Spell.h"

void AddSC_gilneas();

namespace
{
constexpr uint32 Battle = 24904;
constexpr uint32 Rapier = 50334;
constexpr uint32 Inspire = 71388;

void LoadBattleInputs(bool baseline)
{
    auto row = WorldDatabase.Query(
#include "NativeGilneasRapier.inc"
    );
    Require(row && row->GetRowCount() == 3, "Missing the actual Worgen phase, prerequisite or battle quest");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(row->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (row->NextRow());

    auto battle = quests[Battle];
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=24904");
    Require(bool(addon) && addon->Fetch()[4].GetUInt32() == 24676, "Missing the original battle prerequisite");
    battle->LoadQuestTemplateAddon(addon->Fetch());
    battle->prevQuests.push_back(battle->PrevQuestID);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=24904");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == 38854 &&
        objective->Fetch()[5].GetUInt32() == 1, "The real victory objective is required");
    battle->LoadQuestObjective(objective->Fetch());
    battle->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadEquipmentTemplates();
    sSpellMgr->LoadSpellProcEvents();
    sSpellMgr->LoadSpellProcs();
    auto item = sObjectMgr->GetItemTemplate(Rapier);
    auto spell = sSpellMgr->GetSpellInfo(Inspire);
    Require(item && std::any_of(item->Effects.begin(), item->Effects.end(), [](ItemEffectEntry const* effect)
        { return effect->TriggerType == ITEM_SPELLTRIGGER_ON_USE && effect->SpellID == Inspire; }) &&
        spell && spell->GetEffect(EFFECT_0)->TargetA.GetTarget() == TARGET_UNIT_DEST_AREA_ENTRY &&
        spell->GetEffect(EFFECT_1)->TargetA.GetTarget() == TARGET_UNIT_DEST_AREA_ENTRY, "The actual item and both native area effects are required");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature "
        "WHERE guid IN(148108,148116,148208,149656,149660)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=24904");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area "
        "WHERE (spell=68483 AND area=4714 AND quest_start=14386) OR spell=71388 OR (spell=69485 AND quest_start=24676)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceEntry=71388 AND SourceTypeOrReferenceId=13");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id=71388");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
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
    player.SetRewardedQuest(24676);
    MoveForRapier(player, lorna.GetPosition());

    auto spell = sSpellMgr->GetSpellInfo(Inspire);
    auto here = [&]() { return spell->CheckLocation(map.GetId(), player.GetZoneId(), player.GetAreaId(), &player); };
    Require(here() == SPELL_FAILED_INCORRECT_AREA, "An unaccepted battle must not admit the rapier");
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = lorna.GetGUID();
    accept.QuestID = Battle;
    session->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(Battle) == QUEST_STATUS_INCOMPLETE && player.GetItemByEntry(Rapier), "Lorna did not give the actual quest and rapier");

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
    MoveForRapier(player, lorna.GetPosition());
    session->HandleQuestGiverAcceptQuest(accept);
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
        LoadBattleInputs(baseline);
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
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
