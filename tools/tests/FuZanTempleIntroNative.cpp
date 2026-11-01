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
#include "../../src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_temple_intro.h"

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

static void InitializeIntro(int argc,char** argv,bool withSanctuaryDoor=false,bool withTempleRescue=false)
{
    Require(argc==3,"Private native config/runtime missing");
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
    WorldDatabase.DirectExecute(("INSERT INTO fuzan_native_smart_seed SELECT * FROM smart_scripts WHERE "+sources).c_str());
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts LIKE fuzan_native_smart_seed");
    WorldDatabase.DirectExecute("INSERT INTO smart_scripts SELECT * FROM fuzan_native_smart_seed");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE fuzan_native_condition_seed LIKE conditions");
    std::string conditionSources=withSanctuaryDoor?"SourceEntry IN(109413,251591)":"SourceEntry=109413";
    if(withTempleRescue)conditionSources=withSanctuaryDoor?
        "SourceEntry IN(109413,251591,109393,109394,109395,109397,109355,109610)":
        "SourceEntry IN(109413,109393,109394,109395,109397,109355,109610)";
    WorldDatabase.DirectExecute(("INSERT INTO fuzan_native_condition_seed SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND "+conditionSources).c_str());
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions LIKE fuzan_native_condition_seed");
    WorldDatabase.DirectExecute("INSERT INTO conditions SELECT * FROM fuzan_native_condition_seed");
    if(withTempleRescue)
    {
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE fuzan_native_text_seed LIKE creature_text");
        WorldDatabase.DirectExecute("INSERT INTO fuzan_native_text_seed SELECT * FROM creature_text WHERE CreatureID IN(109393,109394,109395,109397,109355,109610)");
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text LIKE fuzan_native_text_seed");
        WorldDatabase.DirectExecute("INSERT INTO creature_text SELECT * FROM fuzan_native_text_seed");
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE fuzan_native_waypoint_seed LIKE waypoints");
        WorldDatabase.DirectExecute("INSERT INTO fuzan_native_waypoint_seed SELECT * FROM waypoints WHERE entry IN(109355,109610)");
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE waypoints LIKE fuzan_native_waypoint_seed");
        WorldDatabase.DirectExecute("INSERT INTO waypoints SELECT * FROM fuzan_native_waypoint_seed");
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
    Require(count&&count->Fetch()[0].GetUInt32()==3u+(withSanctuaryDoor?1u:0u)+(withTempleRescue?32u:0u)&&
        count->Fetch()[1].GetUInt32()==14u+(withTempleRescue?1u:0u),"Actual scoped loader rows changed");
    sDB2Manager.LoadStores(argv[2],LOCALE_enUS);LoadGameTables(argv[2]);
    sSpellMgr->LoadSpellInfoStore();sSpellMgr->LoadSpellRanks();sSpellMgr->LoadTalentSpellInfo();
    sSpellMgr->LoadSkillLineAbilityMap();sSpellMgr->LoadSpellCustomAttr();sSpellMgr->LoadSpellInfoSpellSpecificAndAuraState();
    sObjectMgr->LoadScriptNames();
    auto& names=const_cast<ObjectMgr::ScriptNameContainer&>(sObjectMgr->GetAllScriptNames());
    for(auto name:{"npc_fuzan_temple_intro","spell_fuzan_temple_intro","spell_fuzan_temple_complete"})names.emplace_back(name);
    if(withSanctuaryDoor)names.emplace_back("go_fuzan_temple_sanctuary_door");
    if(withTempleRescue)names.emplace_back("npc_fuzan_belphiar");
    std::sort(names.begin(),names.end());names.erase(std::unique(names.begin(),names.end()),names.end());
    sObjectMgr->LoadInstanceTemplate();sObjectMgr->LoadScenarioData();
    sObjectMgr->LoadCreatureModelInfo();sObjectMgr->LoadWDBCreatureTemplates();sObjectMgr->LoadCreatureTemplates();
    sObjectMgr->LoadCreatureClassLevelStats();sObjectMgr->LoadCreatureScalingData();sObjectMgr->LoadCreatureDifficultyStat();
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
    RegisterCreatureAI(npc_fuzan_temple_intro);RegisterSpellScript(spell_fuzan_temple_intro);RegisterSpellScript(spell_fuzan_temple_complete);
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
    sAchievementMgr->LoadCriteriaList();
    auto rules=WorldDatabase.Query(withSanctuaryDoor?
        "SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id IN(49115,49138,49141)":
        "SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id=49115");
    Require(rules&&!rules->Fetch()[0].GetUInt32(),"Scoped native intro/rescue/door criteria have additional World rules to load");
    (sAchievementMgr->*Access(IntroCriteriaTag{})).assign(MAX_CRITERIA_TREE,nullptr);
    if(withSanctuaryDoor){sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();}
    if(withTempleRescue){sCreatureTextMgr->LoadCreatureTexts();sSmartWaypointMgr->LoadFromDB();}
    sSmartScriptMgr->LoadSmartAIFromDB();sConditionMgr->LoadConditions();sConversationDataStore->LoadConversationData();
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1625,{}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1625,{}}});
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
