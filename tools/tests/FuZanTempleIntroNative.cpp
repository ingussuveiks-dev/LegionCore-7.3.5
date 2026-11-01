// Actual native Player, SmartAI factory, Spell dispatch, Conversation payload/
// visibility clock and SendEvent criteria. Earlier quest entry is seeded in
// memory. World/Hotfix/Login connections are read-only; no character session.
#define MASTER_LOOT_NATIVE_SUPPORT_ONLY
#include "MasterLootNative.cpp"
#include "InstanceScript.h"
#include "InstanceSaveMgr.h"
#include "MapManager.h"
#include "MapInstanced.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "ConversationData.h"
#include "SmartAI.h"
#include "CreatureTextMgr.h"
#include "TemporarySummon.h"
#include "EventObjectData.h"
#include "GossipData.h"
#include "../../src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_temple_intro.h"
#include "../../src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_belphiar.h"
#include "../../src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_summerpetal.h"
#include "../../src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_wave_entry.h"
#ifdef FUZAN_BREWING_NATIVE
#include "../../src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_brewing.h"
#endif

struct IntroInstanceTag { using Type=InstanceScript* InstanceMap::*; friend Type Access(IntroInstanceTag); };
template struct FixtureAccess<IntroInstanceTag,&InstanceMap::i_data>;
struct IntroDbGuidTag { using Type=uint64 Creature::*; friend Type Access(IntroDbGuidTag); };
template struct FixtureAccess<IntroDbGuidTag,&Creature::m_DBTableGuid>;
struct IntroBindingsTag { using Type=SpellScriptsContainer ScriptDataStoreMgr::*; friend Type Access(IntroBindingsTag); };
template struct FixtureAccess<IntroBindingsTag,&ScriptDataStoreMgr::_spellScriptsStore>;
struct IntroCriteriaTag { using Type=AchievementCriteriaDataVector AchievementGlobalMgr::*; friend Type Access(IntroCriteriaTag); };
template struct FixtureAccess<IntroCriteriaTag,&AchievementGlobalMgr::m_criteriaDataVector>;
struct IntroStoryTag { using Type=ObjectGuid FuZanTempleIntro::Actor::*; friend Type Access(IntroStoryTag); };
template struct FixtureAccess<IntroStoryTag,&FuZanTempleIntro::Actor::_story>;
struct IntroClockTag { using Type=std::map<ObjectGuid,uint32> Conversation::*; friend Type Access(IntroClockTag); };
template struct FixtureAccess<IntroClockTag,&Conversation::playing>;

static void InitializeIntro(int argc,char** argv,bool withSanctuaryDoor=false,bool withTempleRescue=false,bool withSummerpetal=false,bool withTransition=false,bool withWaveRepair=false,bool withDefense=false,bool withPickup=false)
{
    Require(argc==3,"Private native config/runtime missing");
    Require(!withSummerpetal||withTempleRescue,"Summerpetal requires native rescue loader context");
    Require(!withTransition||withSummerpetal,"EventObject transition requires the complete actual actor context");
    Require(!withWaveRepair||withTransition,"Owned wave entry requires the complete actual transition context");
    Require(!withDefense||withWaveRepair,"Native defense requires the repaired real entry context");
    Require(!withPickup||withDefense,"Native pickup requires the complete actual defense context");
    std::string error;Require(sConfigMgr->LoadInitial(argv[1],{},error),"Private config failed");
    sLog->Initialize(nullptr);MySQL::Library_Init();
    WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo",""),1,1);
    CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo",""),1,1);
    Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements(),"World pool failed");
    Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Hotfix pool failed");
    Require(!LoginDatabase.Open()&&LoginDatabase.PrepareStatements(),"Login pool failed");
    Require(!CharacterDatabase.Open()&&CharacterDatabase.PrepareStatements(),"Character schema preparation failed");
    // Only native loader input is narrowed, with actual rows copied into
    // connection-local tables before making the sessions read-only.
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE fuzan_native_smart_seed LIKE smart_scripts");
    std::string sources="(entryorguid=109413 AND source_type IN(0,9))";
    if(withSanctuaryDoor)sources+=" OR (entryorguid IN(251591,-138378) AND source_type=1)";
    if(withTempleRescue)sources+=" OR (entryorguid IN(109393,109394,109395,109397,109355,109610) AND source_type=0)";
    if(withTransition)sources+=" OR (entryorguid=103 AND source_type=13) OR (entryorguid=109465 AND source_type=0)";
    if(withDefense)sources+=" OR (source_type=0 AND entryorguid IN(109391,109438,109511,109933,110471,109821,109515,109820))";
    if(withPickup)sources+=" OR (source_type=1 AND entryorguid=251605) OR (source_type=0 AND entryorguid=109440)";
    WorldDatabase.DirectExecute(("INSERT INTO fuzan_native_smart_seed SELECT * FROM smart_scripts WHERE "+sources).c_str());
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts LIKE fuzan_native_smart_seed");
    WorldDatabase.DirectExecute("INSERT INTO smart_scripts SELECT * FROM fuzan_native_smart_seed");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE fuzan_native_condition_seed LIKE conditions");
    std::string conditionSources=withSanctuaryDoor?"SourceEntry IN(109413,251591)":"SourceEntry=109413";
    if(withTempleRescue)conditionSources=withSanctuaryDoor?
        "SourceEntry IN(109413,251591,109393,109394,109395,109397,109355,109610)":
        "SourceEntry IN(109413,109393,109394,109395,109397,109355,109610)";
    if(withTransition)conditionSources="("+conditionSources+" OR SourceEntry=103)";
    if(withDefense)conditionSources="("+conditionSources+" OR SourceEntry IN(109391,109438,109511,109933,110471,109821,109515,109820))";
    if(withPickup)conditionSources="("+conditionSources+" OR SourceEntry IN(251605,109440))";
    WorldDatabase.DirectExecute(("INSERT INTO fuzan_native_condition_seed SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND "+conditionSources).c_str());
    if(withDefense)WorldDatabase.DirectExecute("INSERT INTO fuzan_native_condition_seed SELECT * FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=8262");
    if(withPickup)WorldDatabase.DirectExecute("INSERT INTO fuzan_native_condition_seed SELECT * FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup=251605) OR (SourceTypeOrReferenceId=17 AND SourceEntry IN(217813,224657))");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions LIKE fuzan_native_condition_seed");
    WorldDatabase.DirectExecute("INSERT INTO conditions SELECT * FROM fuzan_native_condition_seed");
    uint32 summerConditions=0;
    bool summerProjected=false;
    if(withTempleRescue)
    {
        Require(withTempleRescue,"Summerpetal requires the complete actual rescue loader context");
        auto rows=WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE SourceEntry IN(109355,109610)");
        Require(bool(rows),"Summerpetal source count missing");
#if defined(FUZAN_SUMMERPETAL_BASELINE) || defined(FUZAN_BELPHIAR_BASELINE)
        Require(rows->Fetch()[0].GetUInt32()==0||rows->Fetch()[0].GetUInt32()==3,"Counterfactual refuses other source conditions");
        if(rows->Fetch()[0].GetUInt32())
        {
            auto native=WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE (SourceEntry=109355 AND SourceGroup IN(6,7) AND ScriptName='condition_fuzan_summerpetal_continuation') OR (SourceEntry=109610 AND SourceGroup=2 AND ConditionTypeOrReference=44 AND ConditionValue1=1137 AND ConditionValue2=4 AND ScriptName='')");
            Require(native&&native->Fetch()[0].GetUInt32()==3,"Counterfactual refuses unknown actor conditions");
            WorldDatabase.DirectExecute("DELETE FROM conditions WHERE SourceEntry IN(109355,109610)");
        }
#elif defined(FUZAN_SUMMERPETAL_LIVE)
        Require(rows->Fetch()[0].GetUInt32()==3,"Actual live Summerpetal/King conditions missing");
        summerConditions=3;
#else
        Require(rows->Fetch()[0].GetUInt32()==0||rows->Fetch()[0].GetUInt32()==3,"Private projection refuses custom/partial actor conditions");
        summerConditions=rows->Fetch()[0].GetUInt32();
        if(withSummerpetal&&!summerConditions)
        {
            WorldDatabase.DirectExecute("INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(22,6,109355,0,0,31,1,3,109355,0,0,0,'condition_fuzan_summerpetal_continuation','Summerpetal actual acknowledged handoff'),(22,7,109355,0,0,31,1,3,109355,0,0,0,'condition_fuzan_summerpetal_continuation','Summerpetal actual owned follow'),(22,2,109610,0,0,44,1,1137,4,0,0,0,'','Monkey King native stage4 path admission')");
            summerConditions=3;
            summerProjected=true;
        }
#endif
    }
    if(withTempleRescue)
    {
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE fuzan_native_text_seed LIKE creature_text");
        WorldDatabase.DirectExecute("INSERT INTO fuzan_native_text_seed SELECT * FROM creature_text WHERE CreatureID IN(109393,109394,109395,109397,109355,109610)");
        if(withDefense)WorldDatabase.DirectExecute("INSERT INTO fuzan_native_text_seed SELECT * FROM creature_text WHERE CreatureID IN(109391,109821,109933)");
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text LIKE fuzan_native_text_seed");
        WorldDatabase.DirectExecute("INSERT INTO creature_text SELECT * FROM fuzan_native_text_seed");
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE fuzan_native_waypoint_seed LIKE waypoints");
        WorldDatabase.DirectExecute("INSERT INTO fuzan_native_waypoint_seed SELECT * FROM waypoints WHERE entry IN(109355,109610)");
        if(withDefense)WorldDatabase.DirectExecute("INSERT INTO fuzan_native_waypoint_seed SELECT * FROM waypoints WHERE entry=109391");
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE waypoints LIKE fuzan_native_waypoint_seed");
        WorldDatabase.DirectExecute("INSERT INTO waypoints SELECT * FROM fuzan_native_waypoint_seed");
    }
    if(withDefense)
    {
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE fuzan_native_phase_seed LIKE phase_definitions");
        WorldDatabase.DirectExecute("INSERT INTO fuzan_native_phase_seed SELECT * FROM phase_definitions WHERE zoneId=8262");
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE phase_definitions LIKE fuzan_native_phase_seed");
        WorldDatabase.DirectExecute("INSERT INTO phase_definitions SELECT * FROM fuzan_native_phase_seed");
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE fuzan_native_addon_seed LIKE creature_template_addon");
        WorldDatabase.DirectExecute("INSERT INTO fuzan_native_addon_seed SELECT * FROM creature_template_addon WHERE entry IN(109391,109438,109511,109933,110471,109821,109515,109820)");
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_template_addon LIKE fuzan_native_addon_seed");
        WorldDatabase.DirectExecute("INSERT INTO creature_template_addon SELECT * FROM fuzan_native_addon_seed");
#if defined(FUZAN_DEFENSE_CRITERIA_REPAIR) || defined(FUZAN_DEFENSE_CRITERIA_BASELINE) || defined(FUZAN_DEFENSE_CRITERIA_LIVE)
        // SetData has an outer null-invoker check. Timed UPDATE dispatch goes
        // directly through ProcessEvent's native base-object fallback, so its
        // original group11/target0 remains unchanged and is a positive control.
        auto criteria=WorldDatabase.Query("SELECT COUNT(*),SUM(SourceGroup=9 AND ConditionTarget IN(0,1)),SUM(SourceGroup=11 AND ConditionTarget=0),SUM(SourceGroup=9 AND ConditionTarget=1) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=109391 AND SourceGroup IN(9,11) AND ConditionTypeOrReference=52 AND ConditionValue1=49567");
        Require(criteria&&criteria->Fetch()[0].GetUInt32()==2&&criteria->Fetch()[1].GetUInt32()==1&&criteria->Fetch()[2].GetUInt32()==1,"Defense criteria projection refuses changed context");
#ifdef FUZAN_DEFENSE_CRITERIA_LIVE
        Require(criteria->Fetch()[3].GetUInt32()==1,"Actual native defense actor-target condition missing");
#elif defined(FUZAN_DEFENSE_CRITERIA_BASELINE)
        WorldDatabase.DirectExecute("UPDATE conditions SET ConditionTarget=0 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=109391 AND SourceGroup=9 AND ConditionTypeOrReference=52 AND ConditionValue1=49567");
#else
        WorldDatabase.DirectExecute("UPDATE conditions SET ConditionTarget=1 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=109391 AND SourceGroup=9 AND ConditionTypeOrReference=52 AND ConditionValue1=49567");
#endif
#endif
    }
    if(withPickup)
    {
        for(auto pair:{std::make_pair("spell_scene","MiscValue=1342"),std::make_pair("spell_scene_event","MiscValue=1342"),std::make_pair("spell_linked_spell","spell_trigger=-217814"),std::make_pair("gossip_menu","MenuID=19932"),std::make_pair("gossip_menu_option","MenuID=19932"),std::make_pair("npc_text","ID=29616")})
        {
            std::string table=pair.first,seed="fuzan_native_pickup_"+table;
            WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE "+seed+" LIKE "+table).c_str());
            WorldDatabase.DirectExecute(("INSERT INTO "+seed+" SELECT * FROM "+table+" WHERE "+pair.second).c_str());
            WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE "+table+" LIKE "+seed).c_str());
            WorldDatabase.DirectExecute(("INSERT INTO "+table+" SELECT * FROM "+seed).c_str());
        }
    }
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");CharacterDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"World not read-only");
    ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"Hotfix not read-only");
    ro=LoginDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"Login not read-only");
    ro=CharacterDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"Character not read-only");
    auto count=WorldDatabase.Query("SELECT (SELECT COUNT(*) FROM smart_scripts),(SELECT COUNT(*) FROM conditions)");
    Require(count&&count->Fetch()[0].GetUInt32()==3u+(withSanctuaryDoor?1u:0u)+(withTempleRescue?32u:0u)+(withTransition?6u:0u)+(withDefense?54u:0u)+(withPickup?9u:0u)&&
        count->Fetch()[1].GetUInt32()==14u+(withTempleRescue?1u:0u)+summerConditions+(withDefense?27u:0u)+(withPickup?103u:0u),"Actual scoped loader rows changed");
    sDB2Manager.LoadStores(argv[2],LOCALE_enUS);LoadGameTables(argv[2]);
    sSpellMgr->LoadSpellInfoStore();sSpellMgr->LoadSpellRanks();sSpellMgr->LoadTalentSpellInfo();
    sSpellMgr->LoadSkillLineAbilityMap();sSpellMgr->LoadSpellCustomAttr();sSpellMgr->LoadSpellInfoSpellSpecificAndAuraState();
    sObjectMgr->LoadScriptNames();
    if(withPickup){sSpellMgr->LoadSpellScene();sSpellMgr->LoadSpellLinked();sObjectMgr->LoadNPCText();sGossipDataStore->LoadGossipMenu();sGossipDataStore->LoadGossipMenuItems();}
    auto& names=const_cast<ObjectMgr::ScriptNameContainer&>(sObjectMgr->GetAllScriptNames());
#ifdef FUZAN_BREWING_NATIVE
    for(auto name:{"npc_fuzan_brewing","npc_fuzan_brewing_questgiver","go_fuzan_brewing_fire","go_fuzan_brewing_ingredient","spell_fuzan_brewing_summon","spell_fuzan_brewing_ingredient","spell_fuzan_brewing_delivery","spell_fuzan_brewing_receipt","aura_fuzan_brewing_controller"})names.emplace_back(name);
#endif
    for(auto name:{"npc_fuzan_temple_intro","spell_fuzan_temple_intro","spell_fuzan_temple_complete"})names.emplace_back(name);
    if(withSanctuaryDoor)names.emplace_back("go_fuzan_temple_sanctuary_door");
    if(withTempleRescue)names.emplace_back("npc_fuzan_belphiar");
    if(withTempleRescue){names.emplace_back("npc_fuzan_summerpetal");names.emplace_back("condition_fuzan_summerpetal_continuation");}
    if(withTransition)for(auto name:{"eventobject_fuzan_wave_entry","spell_fuzan_wave_attempt","aura_fuzan_wave_controller","spell_fuzan_wave_conversation","spell_fuzan_wave_retrieval"})names.emplace_back(name);
    std::sort(names.begin(),names.end());names.erase(std::unique(names.begin(),names.end()),names.end());
    sObjectMgr->LoadInstanceTemplate();sObjectMgr->LoadScenarioData();
    if(withDefense)sObjectMgr->LoadPhaseDefinitions();
    sObjectMgr->LoadCreatureModelInfo();sObjectMgr->LoadWDBCreatureTemplates();sObjectMgr->LoadCreatureTemplates();
    sObjectMgr->LoadCreatureClassLevelStats();sObjectMgr->LoadCreatureScalingData();sObjectMgr->LoadCreatureDifficultyStat();
    if(withDefense)sObjectMgr->LoadCreatureTemplateAddons();
    if(withTempleRescue)
    {
        auto maiden=const_cast<CreatureTemplate*>(sObjectMgr->GetCreatureTemplate(109355));Require(maiden,"Actual Summerpetal template missing");
        auto id=sObjectMgr->GetScriptId("npc_fuzan_summerpetal");
#if defined(FUZAN_SUMMERPETAL_BASELINE) || defined(FUZAN_BELPHIAR_BASELINE)
        Require(!maiden->ScriptID||maiden->ScriptID==id,"Counterfactual refuses custom maiden binding");maiden->ScriptID=0;
#elif defined(FUZAN_SUMMERPETAL_LIVE)
        Require(maiden->ScriptID==id&&summerConditions==3,"Actual live maiden binding missing");
#else
        if(summerProjected){Require(!maiden->ScriptID&&summerConditions==3,"Pending maiden refuses custom/partial binding");maiden->ScriptID=id;}
        Require(maiden->ScriptID==(summerConditions?id:0u),"Actual maiden binding/condition cohort differs");
#endif
    }
    // Pending code-coupled delta, private memory only. Live mode requires the
    // real native loaders to supply all three future registrations.
    auto actor=const_cast<CreatureTemplate*>(sObjectMgr->GetCreatureTemplate(109413));Require(actor,"Native intro actor missing");
    uint32 nativeScript=sObjectMgr->GetScriptId("npc_fuzan_temple_intro");
#ifdef FUZAN_INTRO_BASELINE
    Require(!actor->ScriptID||actor->ScriptID==nativeScript,"Counterfactual refuses custom creature script");actor->ScriptID=0;
#elif defined(FUZAN_INTRO_LIVE)
    Require(actor->ScriptID==nativeScript,"Live intro creature binding missing");
#else
    Require(!actor->ScriptID,"Custom intro creature binding must not be overwritten");actor->ScriptID=nativeScript;
#endif
    sScriptDataStore->LoadSpellScriptNames();auto& bindings=sScriptDataStore->*Access(IntroBindingsTag{});
#ifdef FUZAN_BREWING_NATIVE
    auto bindBrewing=[&](uint32 spell,char const* name){
        auto range=bindings.equal_range(spell);auto id=sObjectMgr->GetScriptId(name);
#ifdef FUZAN_BREWING_LIVE
        Require(std::distance(range.first,range.second)==1 && range.first->second.first==id,"Actual live brewing spell registration differs");
#else
        Require(range.first==range.second,"Private brewing fixture refuses custom/partial binding");bindings.emplace(spell,std::make_pair(id,true));
#endif
    };
    for(auto pair:{std::make_pair(217304u,"spell_fuzan_brewing_summon"),{217302u,"aura_fuzan_brewing_controller"},{217244u,"spell_fuzan_brewing_receipt"}})
        bindBrewing(pair.first,pair.second);
    for(uint32 id:{217213u,217216u,217219u,217224u})
        bindBrewing(id,"spell_fuzan_brewing_ingredient");
    for(uint32 id:{217251u,217282u,217290u,217296u,217248u,217280u,217287u,217295u})
        bindBrewing(id,"spell_fuzan_brewing_delivery");
    for(auto pair:{std::make_pair(109224u,"npc_fuzan_brewing"),{109187u,"npc_fuzan_brewing_questgiver"}})
    {
        auto value=const_cast<CreatureTemplate*>(sObjectMgr->GetCreatureTemplate(pair.first));auto id=sObjectMgr->GetScriptId(pair.second);
        Require(value && value->AIName.empty(),"Brewing fixture refuses custom creature AI");
#ifdef FUZAN_BREWING_LIVE
        Require(value->ScriptID==id,"Actual live brewing creature registration differs");
#else
        Require(!value->ScriptID,"Private brewing fixture refuses custom/partial creature binding");value->ScriptID=id;
#endif
    }
#endif
    uint32 waveBindings=0;
    std::vector<std::pair<uint32,std::string>> waveNames;
    if(withTransition)
    {
        for(uint32 spell:{218883u,218882u,217659u,217667u,217666u,217669u})waveNames.emplace_back(spell,"spell_fuzan_wave_attempt");
        for(uint32 spell:{217659u,217667u,217817u})waveNames.emplace_back(spell,"aura_fuzan_wave_controller");
        waveNames.emplace_back(217817,"spell_fuzan_wave_conversation");
        waveNames.emplace_back(217814,"spell_fuzan_wave_retrieval");
        for(auto const& pair:waveNames)
        {
            auto range=bindings.equal_range(pair.first);uint32 id=sObjectMgr->GetScriptId(pair.second.c_str());
            auto count=std::count_if(range.first,range.second,[=](auto const& row){return row.second.first==id;});
            Require(count<=1,"Duplicate wave-entry script binding");waveBindings+=uint32(count);
        }
        for(uint32 spell:{217814u,217817u,218883u,218882u,217659u,217667u,217666u,217669u})
        {
            auto range=bindings.equal_range(spell);
            for(auto it=range.first;it!=range.second;++it)
                Require(std::any_of(waveNames.begin(),waveNames.end(),[&](auto const& pair){return pair.first==spell&&sObjectMgr->GetScriptId(pair.second.c_str())==it->second.first;}),"Unknown wave-entry spell script must be loaded/investigated");
        }
        Require(waveBindings==0||waveBindings==11,"Partial wave-entry binding cohort");
#if defined(FUZAN_WAVE_ENTRY_BASELINE)
        for(uint32 spell:{217814u,217817u,218883u,218882u,217659u,217667u,217666u,217669u})bindings.erase(spell);
#elif defined(FUZAN_WAVE_ENTRY_LIVE)
        Require(withWaveRepair&&waveBindings==11,"Actual live wave-entry spell bindings missing");
#else
        if(withWaveRepair&&!waveBindings)
            for(auto const& pair:waveNames)bindings.emplace(pair.first,std::make_pair(sObjectMgr->GetScriptId(pair.second.c_str()),true));
#endif
    }
    for(auto pair:{std::make_pair(217775u,"spell_fuzan_temple_intro"),std::make_pair(217774u,"spell_fuzan_temple_complete")})
    {
        uint32 id=sObjectMgr->GetScriptId(pair.second);auto range=bindings.equal_range(pair.first);
        auto count=std::count_if(range.first,range.second,[=](auto const& row){return row.second.first==id;});
#ifdef FUZAN_INTRO_BASELINE
        Require(count<=1&&std::distance(range.first,range.second)==count,"Counterfactual refuses other spell bindings");
        bindings.erase(range.first,range.second);
#elif defined(FUZAN_INTRO_LIVE)
        Require(count==1,"Actual live spell binding missing");
#else
        Require(!count&&range.first==range.second,"Custom native spell binding must not be overwritten");bindings.emplace(pair.first,std::make_pair(id,true));
#endif
    }
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
#ifdef FUZAN_BREWING_NATIVE
    RegisterCreatureAI(npc_fuzan_brewing);RegisterCreatureAI(npc_fuzan_brewing_questgiver);
    RegisterGameObjectAI(go_fuzan_brewing_fire);RegisterGameObjectAI(go_fuzan_brewing_ingredient);
    RegisterSpellScript(spell_fuzan_brewing_summon);RegisterSpellScript(spell_fuzan_brewing_ingredient);
    RegisterSpellScript(spell_fuzan_brewing_delivery);RegisterSpellScript(spell_fuzan_brewing_receipt);RegisterAuraScript(aura_fuzan_brewing_controller);
    new map_fuzan_brewing();new player_fuzan_brewing();
#endif
    RegisterCreatureAI(npc_fuzan_temple_intro);RegisterSpellScript(spell_fuzan_temple_intro);RegisterSpellScript(spell_fuzan_temple_complete);
    if(summerConditions){RegisterCreatureAI(npc_fuzan_summerpetal);new condition_fuzan_summerpetal_continuation();}
    if(withWaveRepair){new eventobject_fuzan_wave_entry();RegisterSpellScript(spell_fuzan_wave_attempt);RegisterAuraScript(aura_fuzan_wave_controller);RegisterSpellScript(spell_fuzan_wave_conversation);RegisterSpellScript(spell_fuzan_wave_retrieval);}
    sScriptMgr->SwapScriptContext(true);AIRegistry::Initialize();UnitAI::FillAISpellInfo();sWorld->LoadConfigSettings();opcodeTable.Initialize();
    HashMapHolder<Player>::SetSize(32);HashMapHolder<Creature>::SetSize(20000);HashMapHolder<Pet>::SetSize(20000);HashMapHolder<Conversation>::SetSize(20000);
    auto row=WorldDatabase.Query(
#include "NativeFuZanQuest.inc"
    );Require(bool(row),"Real quest42765 missing");
    auto quest=new Quest(row->Fetch());Require(quest->GetQuestId()==42765,"Native quest shape changed");
    auto& quests=sQuestDataStore->*Access(QuestVectorTag{});quests.resize(45089);sQuestDataStore->*Access(QuestMaxTag{})=45089;
    quests[42765]=quest;const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[42765]=quest;
    auto objectives=WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=42765 ORDER BY StorageIndex");
    Require(bool(objectives),"Native quest objectives missing");do{quest->LoadQuestObjective(objectives->Fetch());}while(objectives->NextRow());
    Require(quest->GetObjectives().size()==3,"Native quest objectives changed");
    if(withPickup)
    {
        for(auto const& objective:quest->GetObjectives())Require(objective.Type==QUEST_OBJECTIVE_MONSTER,"Pickup objective loader normalization needs review");
        // QuestDataStoreMgr::LoadQuests derives these for MONSTER objectives.
        // Loading only the raw Quest constructor omits native credit dispatch.
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_CAST);
    }
    sAchievementMgr->LoadCriteriaList();
    auto rules=WorldDatabase.Query(withSanctuaryDoor?
        "SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id IN(49115,49138,49141)":
        "SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id=49115");
    Require(rules&&!rules->Fetch()[0].GetUInt32(),"Scoped native intro/rescue/door criteria have additional World rules to load");
    (sAchievementMgr->*Access(IntroCriteriaTag{})).assign(MAX_CRITERIA_TREE,nullptr);
    if(withSanctuaryDoor){sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();}
    if(withTempleRescue){sCreatureTextMgr->LoadCreatureTexts();sSmartWaypointMgr->LoadFromDB();}
    if(withTransition)
    {
        sEventObjectDataStore->LoadEventObjectTemplates(); // No global spawn/clone maintenance.
        auto event=const_cast<EventObjectTemplate*>(sEventObjectDataStore->GetEventObjectTemplate(103));Require(event,"Actual EO103 template missing");
        uint32 original=sObjectMgr->GetScriptId("SmartEventObject"),future=sObjectMgr->GetScriptId("eventobject_fuzan_wave_entry");
        Require(event->ScriptID==(waveBindings?future:original),"Wave-entry template/spell cohort differs or is custom");
#if defined(FUZAN_WAVE_ENTRY_BASELINE)
        event->ScriptID=original;
#elif defined(FUZAN_WAVE_ENTRY_LIVE)
        Require(withWaveRepair&&event->ScriptID==future,"Actual live wave-entry event binding missing");
#else
        if(withWaveRepair)event->ScriptID=future;
#endif
    }
    if(withPickup)
    {
        sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();
        auto& table=LootTemplates_Gameobject.*Access(LootTableNameTag{});auto original=table;
        table+=" WHERE Entry=251605 ORDER BY Entry,Item";auto loaded=(LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();table=original;
        Require(loaded==1&&LootTemplates_Gameobject.HaveLootFor(251605),"Actual native Fu Zan loot context missing");
    }
    sSmartScriptMgr->LoadSmartAIFromDB();sConditionMgr->LoadConditions();sConversationDataStore->LoadConversationData();
    if(withPickup)
    {
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1625,{}},{1514,{}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1625,{}},{1514,{}}});
    }
    else
    {
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1625,{}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1625,{}}});
    }
}

static void AdvanceIntro(LootPlayer& player,::Actor& source,uint32 duration)
{
    for(uint32 elapsed=0;elapsed<duration;elapsed+=10)
    {
        player.Unit::Update(10);source.Unit::Update(10);source.AI()->UpdateAI(10);
        auto ai=dynamic_cast<FuZanTempleIntro::Actor*>(source.AI());Require(ai,"Native actor factory changed");
        auto guid=ai->*Access(IntroStoryTag{});
        if(auto story=ObjectAccessor::GetConversation(source,guid))story->Update(10);
        source.GetMap()->MoveAllCreaturesInMoveList();source.GetMap()->RemoveAllObjectsInRemoveList();
    }
}

struct IntroObservedInstance : InstanceScript
{
    using InstanceScript::InstanceScript;
    Player* nested = nullptr;
    bool reentered = false;
    SpellCastResult nestedResult = SPELL_CAST_OK;
    void onScenarionNextStep(uint32 step) override
    {
        if (step == 1 && nested && !reentered)
        {
            reentered = true;
            nestedResult = nested->CastSpell(nested, 217774, false);
        }
    }
};

static void CheckLegacyIntro(uint8 race,uint32 number)
{
    MapInstanced parent(1625,300);InstanceMap map(1625,300,9999900+number,DIFFICULTY_N_SCENARIO,&parent);
    map.*Access(IntroInstanceTag{})=new InstanceScript(&map);
    Scenario* progress=sScenarioMgr->AddScenario(&map,1137);Require(progress,"Native baseline scenario missing");
    auto session=new WorldSession(0,"Fu Zan native old-data counterfactual",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
    {
        LootPlayer player(session,map,950+number,{951.f,-2521.f,180.474f,0.f});
        player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
        auto model=sChrRacesStore.LookupEntry(race);Require(model,"Native baseline race model missing");
        player.SetNativeDisplayId(model->MaleDisplayID);player.SetDisplayId(model->MaleDisplayID);player.SetObjectScale(1.f);
        player.SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,42765);
        ::Actor source(&map,109413,1400+number,{953.719f,-2521.6f,180.474f,4.36382f});source.*Access(IntroDbGuidTag{})=373392;
        Require(map.AddToMap(static_cast<Creature*>(&source))&&source.AIM_Initialize(),"Native baseline source failed");
        Require(dynamic_cast<SmartAI*>(source.AI())&&!dynamic_cast<FuZanTempleIntro::Actor*>(source.AI()),"Counterfactual did not retain original SmartAI");
        uint32 before=uint32(objectCountInWorld[uint8(HighGuid::Conversation)]);
        // Native InitTimer arms OOC_LOS after its initial 1..16ms window.
        // Real map updates precede the subsequent player visibility visit.
        source.AI()->UpdateAI(20);
        Require(sConditionMgr->IsObjectMeetingSmartEventConditions(109413,0,0,&player,&source),"Native baseline admission predicates failed");
        Require(source.IsWithinDistInMap(&player,16.f)&&source.IsWithinLOSInMap(&player),"Native baseline source range/LOS failed");
        Require(!source.isInCombat()&&!source.IsHostileTo(&player),"Native baseline friendly OOC gate failed");
        source.AI()->MoveInLineOfSight_Safe(&player);
        auto legacy=static_cast<SmartAI*>(source.AI())->GetScript();
        std::unique_ptr<ObjectList> nearby(legacy->GetWorldObjectsInDist(50.f));
        Require(std::find(nearby->begin(),nearby->end(),&player)!=nearby->end(),"Native baseline grid search cannot find original player");
        Require(legacy->HasRunEvent(0,SMART_EVENT_OOC_LOS),"Native baseline original LOS did not run");
        for(uint32 elapsed=0;elapsed<31000;elapsed+=10)
        {
            player.Unit::Update(10);source.Unit::Update(10);source.AI()->UpdateAI(10);
            map.MoveAllCreaturesInMoveList();map.RemoveAllObjectsInRemoveList();
        }
        Require(uint32(objectCountInWorld[uint8(HighGuid::Conversation)])==before,"Missing guide unexpectedly produced a native conversation");
        Require(FuZanTempleIntro::Receipt(progress)&&progress->GetCurrentStep()==1,"Expected original independent30s progression not reproduced");
        std::cout<<"EXPECTED_BROKEN native original3-row SmartAI race="<<unsigned(race)<<" missing109353 rejects actual217775/2770 creation but native30s Script9 grants52411 anyway\n";
    }
    session->SetPlayer(nullptr);sScenarioMgr->RemoveScenario(map.GetInstanceId());
}

static void CheckIntro(uint8 race,uint32 number,uint32 mode)
{
    MapInstanced parent(1625,300);InstanceMap map(1625,300,9999800+number,DIFFICULTY_N_SCENARIO,&parent);
    auto instance=new IntroObservedInstance(&map);map.*Access(IntroInstanceTag{})=instance;
    Scenario* progress=sScenarioMgr->AddScenario(&map,1137);Require(progress&&progress->GetCurrentStep()==0,"Native scenario1137 construction failed");
    auto session=new WorldSession(0,"Fu Zan native fixture",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
    {
        LootPlayer player(session,map,900+number,{951.0f,-2521.0f,180.474f,0.f});
        player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
        auto model=sChrRacesStore.LookupEntry(race);Require(model,"Race model missing");
        player.SetNativeDisplayId(model->MaleDisplayID);player.SetDisplayId(model->MaleDisplayID);player.SetObjectScale(1.0f);
        player.SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,42765);
        if(mode==6)player.SetQuestStatus(42765,QUEST_STATUS_COMPLETE);
        std::unique_ptr<LootPlayer> peer;
        std::unique_ptr<ProbeParty> party;
        WorldSession* peerSession=nullptr;
        if(mode==7)
        {
            peerSession=new WorldSession(0,"Fu Zan native group fixture",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
            peer=std::make_unique<LootPlayer>(peerSession,map,1600+number,player.GetPosition());
            peer->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);peer->SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);peer->SetQuestSlot(0,42765);
            party=std::make_unique<ProbeParty>(player,*peer);
        }
        std::unique_ptr<::Actor> guide;
        auto addGuide=[&] {
            guide=std::make_unique<::Actor>(&map,109353,1200+number,Position{960.93f,-2490.3f,180.576f,0.f});
            (*guide).*Access(IntroDbGuidTag{})=373394;
            Require(map.AddToMap(static_cast<Creature*>(guide.get())),"Native guide map admission failed");
        };
        if(mode!=9)addGuide();
        ::Actor source(&map,109413,1300+number,{953.719f,-2521.6f,180.474f,4.36382f});source.*Access(IntroDbGuidTag{})=373392;
        Require(map.AddToMap(static_cast<Creature*>(&source))&&source.AIM_Initialize(),"Native source/factory initialization failed");
        auto ai=dynamic_cast<FuZanTempleIntro::Actor*>(source.AI());Require(ai,"Actual native script binding did not construct intro AI");
        if(guide){source.AI()->MoveInLineOfSight_Safe(guide.get());Require((ai->*Access(IntroStoryTag{})).IsEmpty(),"NPC consumed actual player intro");}
        source.AI()->MoveInLineOfSight_Safe(&player);AdvanceIntro(player,source,190);
        Require((ai->*Access(IntroStoryTag{})).IsEmpty()&&!FuZanTempleIntro::Receipt(progress),"Intro started before native200ms delay");
        AdvanceIntro(player,source,100);
        if(mode==9)
        {
            Require((ai->*Access(IntroStoryTag{})).IsEmpty(),"Missing guide admitted a native story");
            Require(player.HasAura(217775),"Native70s dummy aura was changed by missing-story handling");
            AdvanceIntro(player,source,130000);
            Require(!FuZanTempleIntro::Receipt(progress)&&!source.isActiveObject(),"Failed creation bypassed progress/finite active cleanup");
            addGuide();source.AI()->MoveInLineOfSight_Safe(&player);AdvanceIntro(player,source,300);
        }
        auto guid=ai->*Access(IntroStoryTag{});auto story=dynamic_cast<FuZanTempleIntro::Story*>(ObjectAccessor::GetConversation(source,guid));
        Require(story&&story->GetCasterGUID()==player.GetGUID()&&story->GetUInt32Value(CONVERSATION_FIELD_LAST_LINE_END_TIME)==35000,
            "Actual native cast did not create owned2770/default35s payload");
        Require(story->GetLines().size()==9&&story->GetCreature(0)&&story->GetCreature(2),"Native2770 payload/actors incomplete");
        Require(story->GetLine(8)->id==5765&&story->GetLine(8)->textId==27700,"Native terminal line layout differs");
        if(mode==10)
        {
            auto old=guid;story->Remove();map.RemoveAllObjectsInRemoveList();AdvanceIntro(player,source,1400);
            guid=ai->*Access(IntroStoryTag{});story=dynamic_cast<FuZanTempleIntro::Story*>(ObjectAccessor::GetConversation(source,guid));
            Require(story&&guid!=old&&!FuZanTempleIntro::Receipt(progress),"Interrupted story did not replay without progress");
        }
        if(mode==1||mode==2||mode==8)
        {
            auto old=guid;
            if(mode==1){player.SetQuestStatus(42765,QUEST_STATUS_NONE);player.SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);}
            else if(mode==2)ai->Reset();
            else{player.SetPhaseMask(2,false);ai->UpdateAI(10);player.SetPhaseMask(1,false);}
            source.AI()->MoveInLineOfSight_Safe(&player);AdvanceIntro(player,source,300);
            guid=ai->*Access(IntroStoryTag{});story=dynamic_cast<FuZanTempleIntro::Story*>(ObjectAccessor::GetConversation(source,guid));
            Require(story&&guid!=old&&!ObjectAccessor::GetConversation(source,old)&&!FuZanTempleIntro::Receipt(progress),
                "Native abandon/retake/reset/phase recovery retained a stale intro or skipped playback");
        }
        Require(player.CastSpell(&player,217774,false)!=SPELL_CAST_OK&&!FuZanTempleIntro::Receipt(progress),"Unissued early native completion admitted");
        auto& clocks=story->*Access(IntroClockTag{});clocks.clear(); // Explicit missing-viewer counterfactual.
        AdvanceIntro(player,source,30000);Require(!FuZanTempleIntro::Receipt(progress),"Bare30s timer still advanced the intro");
        if(peer)
        {
            Require(story->IsInPersonnalVisibilityList(player.GetGUID())&&story->IsInPersonnalVisibilityList(peer->GetGUID()),"Native group visibility lost");
            Require(!story->CanNeverSee2(peer.get()),"Native group viewer visibility denied");clocks[peer->GetGUID()]=getMSTime()-35001;
            story->Update(10);Require(!FuZanTempleIntro::Receipt(progress),"Other group member armed original owner's playback");
        }
        Require(!story->CanNeverSee2(&player),"Native original-player playback visibility denied");
        Require(!story->Played(),"A new native viewer clock was treated as completed");
        if(mode==3)player.Unit::setDeathState(JUST_DIED);
        Position start=player.GetPosition();if(mode==4)player.Relocate(source.GetPositionX()+150.f,source.GetPositionY(),source.GetPositionZ(),0.f);
        if(mode==5)instance->nested=&player;
        clocks[player.GetGUID()]=getMSTime()-35001; // Controlled native clock, not actual client playback.
        story->Update(10);AdvanceIntro(player,source,100);
        if(mode==3||mode==4)
        {
            Require(!FuZanTempleIntro::Receipt(progress),"Dead/distant player received completion");
            if(mode==3){player.Unit::setDeathState(ALIVE);player.SetHealth(player.GetMaxHealth());}
            else player.Relocate(start);
            AdvanceIntro(player,source,1100);
        }
        Require(FuZanTempleIntro::Receipt(progress)&&progress->GetCurrentStep()==1,"Native completed playback/217774 did not acknowledge52411");
        if(mode==5)Require(instance->reentered&&instance->nestedResult!=SPELL_CAST_OK,"Native reentrant completion was not rejected");
        Require(!ObjectAccessor::GetConversation(source,guid),"Completed owned conversation not retired");
        auto counter=progress->GetAchievementMgr().GetCriteriaProgressMap()->at(49115).Counter;
        source.AI()->MoveInLineOfSight_Safe(&player);AdvanceIntro(player,source,1000);
        Require(progress->GetAchievementMgr().GetCriteriaProgressMap()->at(49115).Counter==counter,"Native completion duplicated");
        ai->Reset();map.RemoveAllObjectsInRemoveList();
        party.reset();peer.reset();if(peerSession)peerSession->SetPlayer(nullptr);
        std::cout<<"CASE native Fu Zan intro race="<<unsigned(race)<<" mode="<<mode<<" actual source14conditions/cast/2770payload/ownerclock/217774/native52411/cleanup PASS\n";
    }
    session->SetPlayer(nullptr);sScenarioMgr->RemoveScenario(map.GetInstanceId());
}

#ifndef FUZAN_TEMPLE_NATIVE_SUPPORT_ONLY
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try
    {
        InitializeIntro(argc,argv);uint32 number=0;
#ifdef FUZAN_INTRO_BASELINE
        for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})CheckLegacyIntro(race,++number);
#elif defined(FUZAN_INTRO_FAULTS)
        for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(uint32 mode=9;mode<11;++mode)CheckIntro(race,++number,mode);
#else
        for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(uint32 mode=0;mode<9;++mode)CheckIntro(race,++number,mode);
#endif
        CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();
#ifdef FUZAN_INTRO_BASELINE
        std::cout<<"EXPECTED_BROKEN2 actual original-data/native default conversation+independent timer counterfactuals; no old whole-binary/client claim.\n";
#elif defined(FUZAN_INTRO_FAULTS)
        std::cout<<"PASS4 explicit fault injections: absent guide/finite exhaustion/reentry and interrupted story/replay; expected diagnostics retained. No ordinary clean-run claim.\n";
#else
        std::cout<<"PASS18 actual native intro cases, full payload/ordinary cast/real visibility method/native SendEvent; abandon/retake/reset/phase/dead/range/complete quest/group/reentrant branches. Clock advanced in memory; no actual client, saved character or whole-chain claim.\n";
#endif
        return 0;
    }
    catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
#endif
