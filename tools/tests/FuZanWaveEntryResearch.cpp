// Expected-broken research and native controls against the pinned current EO103.
// No repair, client traversal or full quest certification is claimed.
#define FUZAN_BELPHIAR_NATIVE_SUPPORT_ONLY
#include "FuZanBelphiarNative.cpp"
void AddSC_SmartScripts();
static std::map<uint32,uint32> launches,launchTargets,finished;
class zzzz_fuzan_transition_launch_probe : public SpellScript
{
    PrepareSpellScript(zzzz_fuzan_transition_launch_probe);
    void Launch(SpellEffIndex){++launches[GetSpellInfo()->Id];}
    void Target(SpellEffIndex){++launchTargets[GetSpellInfo()->Id];}
    void Finished(){++finished[GetSpellInfo()->Id];}
    void Register() override
    {
        OnEffectLaunch+=SpellEffectFn(zzzz_fuzan_transition_launch_probe::Launch,EFFECT_0,SPELL_EFFECT_TRIGGER_SPELL_4);
        OnEffectLaunchTarget+=SpellEffectFn(zzzz_fuzan_transition_launch_probe::Target,EFFECT_0,SPELL_EFFECT_TRIGGER_SPELL_4);
        AfterCast+=SpellCastFn(zzzz_fuzan_transition_launch_probe::Finished);
    }
};
struct TransitionEvent : EventObject
{
    ~TransitionEvent(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<EventObject*>(this),false);ResetMap();}
};
static void CheckTransition(uint32 step,uint32 number,bool cancelBeforeCasts=false)
{
    MapInstanced parent(1625,300);InstanceMap map(1625,300,9999200+number,DIFFICULTY_N_SCENARIO,&parent);
    map.*Access(IntroInstanceTag{})=new InstanceScript(&map);auto scenario=sScenarioMgr->AddScenario(&map,1137);scenario->SetCurrentStep(step);
    Position const location{898.03f,-2582.66f,181.311f,3.93323f};
    auto session=new WorldSession(0,"Native Fu Zan EventObject boundary",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
    {
        LootPlayer player(session,map,14000+number,location);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
        player.GetZoneAndAreaId(player.m_zoneId,player.m_areaId);
        Require(player.m_zoneId==8262&&player.m_areaId==8262,"Native map terrain must supply actual8262 cached spell location");
        player.SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,42765);
        BelphiarActor prior(map,109465,373425,{896.955f,-2585.63f,181.381f,0.329788f});
        TransitionEvent event;Require(event.Create(20000+number,&map,1,103,location.GetPositionX(),location.GetPositionY(),location.GetPositionZ(),location.GetOrientation(),5,0,0),"Actual EO creation failed");
        event.m_DBTableGuid=400106;
        map.AddToGrid(static_cast<EventObject*>(&event),Cell(Trinity::ComputeCellCoord(location.GetPositionX(),location.GetPositionY())));
        event.AddToWorld(); // The native grid-loader path; AddToMap<EventObject> has no core instantiation.
        auto pump=[&](uint32 duration){for(uint32 elapsed=0;elapsed<duration;elapsed+=10){player.Unit::Update(10);prior.Unit::Update(10);prior.AI()->UpdateAI(10);event.Update(10);map.MoveAllCreaturesInMoveList();map.RemoveAllObjectsInRemoveList();}};
        event.Update(10);
        std::cout<<"OBSERVATION step="<<step<<" afterNativeEO step="<<scenario->GetCurrentStep()<<" criterion49176="<<BelphiarCounter(scenario,49176)
            <<" activated="<<event.ActivatedForPlayer(&player)<<" data109465="<<dynamic_cast<SmartAI*>(prior.AI())->GetScript()->HasRunEvent(0,SMART_EVENT_DATA_SET)<<std::endl;
        if(cancelBeforeCasts){player.SetQuestStatus(42765,QUEST_STATUS_NONE);player.SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);}
        pump(1000);
        std::cout<<"OBSERVATION afterNativeDelayedCasts cancelled="<<cancelBeforeCasts<<" aura217817="<<player.HasAura(217817)<<" aura217659="<<player.HasAura(217659)<<" aura217667="<<player.HasAura(217667)<<std::endl;
        if(cancelBeforeCasts){Require(player.HasAura(217659)&&player.HasAura(217667),"Original delayed-cast attempt counterfactual differs");
            std::cout<<"EXPECTED_BROKEN original200ms pending casts apply both infinite companion controllers after explicit abandon/retake\n";}
        if(step==3){Require(!BelphiarCounter(scenario,49176)&&event.ActivatedForPlayer(&player),"Early native source admission observation differs");scenario->SetCurrentStep(4);pump(1000);
            Require(scenario->GetCurrentStep()==4&&!BelphiarCounter(scenario,49176),"Stationary native trigger unexpectedly retried after stage transition");
            std::cout<<"EXPECTED_BROKEN early EO consumes player entry and sends native linked effects before49176; standing inside after stage4 cannot recover\n";}
        else Require(scenario->GetCurrentStep()==5&&BelphiarCounter(scenario,49176)==1,"Native active-stage control missing");
        player.RemoveAllAuras();std::list<Creature*> children;player.GetCreatureListWithEntryInGrid(children,109350,100);player.GetCreatureListWithEntryInGrid(children,109352,100);
        for(auto actor:children)if(actor->ToTempSummon())actor->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();
    }
    session->SetPlayer(nullptr);sScenarioMgr->RemoveScenario(map.GetInstanceId());
}
static void CheckCompanionCasts()
{
    uint32 number=0;
    for(uint32 spell:{217817u,218883u,218882u,217659u,217667u})
    {
        MapInstanced parent(1625,300);InstanceMap map(1625,300,9999100+(++number),DIFFICULTY_N_SCENARIO,&parent);
        map.*Access(IntroInstanceTag{})=new InstanceScript(&map);auto scenario=sScenarioMgr->AddScenario(&map,1137);scenario->SetCurrentStep(5);
        auto session=new WorldSession(0,"Native separate transition cast control",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
        {
            LootPlayer player(session,map,16000+number,{898.03f,-2582.66f,181.311f,3.93323f});player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
            player.GetZoneAndAreaId(player.m_zoneId,player.m_areaId);
            Require(player.m_zoneId==8262&&player.m_areaId==8262,"Native map spell location differs");
            player.SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,42765);
            TriggerCastData data;data.triggerFlags=TRIGGERED_NONE;
            Spell check(&player,sSpellMgr->GetSpellInfo(spell),data);SpellCastTargets targets;targets.SetCaster(&player);targets.SetUnitTarget(&player);check.InitExplicitTargets(targets);
            auto raw=check.CheckCast(true);
            auto result=player.CastSpell(&player,spell,false);
            for(uint32 elapsed=0;elapsed<1000;elapsed+=10)player.Unit::Update(10);
            std::cout<<"OBSERVATION singleFreshCast="<<spell<<" result="<<uint32(result)<<" rawCheck="<<uint32(raw)<<" nativeOK="<<uint32(SPELL_CAST_OK)
                <<" aura217817="<<player.HasAura(217817)<<" aura217659="<<player.HasAura(217659)<<" aura217667="<<player.HasAura(217667)
                <<" observedLaunch="<<launches[spell]<<" launchTarget="<<launchTargets[spell]<<" afterCast="<<finished[spell]<<std::endl;
            player.RemoveAllAuras();map.RemoveAllObjectsInRemoveList();
        }
        session->SetPlayer(nullptr);sScenarioMgr->RemoveScenario(map.GetInstanceId());
    }
}
static void CheckRealTransition(uint32 cancelAt,uint32 number)
{
    MapInstanced parent(1625,300);InstanceMap map(1625,300,9999000+number,DIFFICULTY_N_SCENARIO,&parent);
    map.*Access(IntroInstanceTag{})=new InstanceScript(&map);auto scenario=sScenarioMgr->AddScenario(&map,1137);scenario->SetCurrentStep(2);
    auto session=new WorldSession(0,"Native real victory to wave-entry continuation",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
    {
        LootPlayer player(session,map,18000+number,bossPosition);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
        player.GetZoneAndAreaId(player.m_zoneId,player.m_areaId);Require(player.m_zoneId==8262&&player.m_areaId==8262,"Real cached spell zone missing");
        player.SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,42765);
        BelphiarActor boss(map,109397,373405,bossPosition);
        BelphiarActor king(map,109610,373384,{826.786f,-2549.49f,183.12f,1.2733f});
        player.Kill(&boss,false);
        auto pump=[&](uint32 duration){for(uint32 elapsed=0;elapsed<duration;elapsed+=10){
            player.Unit::Update(10);king.Unit::Update(10);king.AI()->UpdateAI(10);
            for(auto actor:Maidens(boss)){actor->Unit::Update(10);if(actor->IsInWorld()&&actor->IsAIEnabled)actor->AI()->UpdateAI(10);}
            map.MoveAllCreaturesInMoveList();map.RemoveAllObjectsInRemoveList();}};
        pump(16000);Require(scenario->GetCurrentStep()==4&&BelphiarCounter(scenario,49140)==1&&BelphiarCounter(scenario,49356)==1,"Actual victory/story receipts missing before wave-entry");
        Position const location{898.03f,-2582.66f,181.311f,3.93323f};
        map.PlayerRelocation(&player,location.GetPositionX(),location.GetPositionY(),location.GetPositionZ(),location.GetOrientation());
        BelphiarActor prior(map,109465,373425,{896.955f,-2585.63f,181.381f,0.329788f});
        TransitionEvent event;Require(event.Create(22000+number,&map,1,103,location.GetPositionX(),location.GetPositionY(),location.GetPositionZ(),location.GetOrientation(),5,0,0),"Real continuation EO creation missing");
        event.m_DBTableGuid=400106;map.AddToGrid(static_cast<EventObject*>(&event),Cell(Trinity::ComputeCellCoord(location.GetPositionX(),location.GetPositionY())));event.AddToWorld();
        event.Update(10);Require(scenario->GetCurrentStep()==5&&BelphiarCounter(scenario,49176)==1,"Actual wave-entry receipt missing");
        auto receipt=player.GetQuestCancellationReceipt(42765);
        if(cancelAt==200)pump(200);
        if(cancelAt==1000)pump(1000);
        if(cancelAt!=UINT32_MAX)std::cout<<"OBSERVATION BEFORE_CANCEL at="<<cancelAt<<" aura217817="<<player.HasAura(217817)
            <<" aura217659="<<player.HasAura(217659)<<" aura217667="<<player.HasAura(217667)<<std::endl;
        if(cancelAt!=UINT32_MAX){player.SetQuestStatus(42765,QUEST_STATUS_NONE);player.SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);Require(receipt->load(),"Explicit native attempt was not cancelled");}
        pump(1000);
        std::cout<<"OBSERVATION REAL native49140/49356/49176 cancelAt="<<cancelAt<<" aura217817="<<player.HasAura(217817)<<" aura217659="<<player.HasAura(217659)<<" aura217667="<<player.HasAura(217667)<<std::endl;
        Require(player.HasAura(217659)&&player.HasAura(217667),"Real native controller observation differs");
        if(cancelAt!=UINT32_MAX)std::cout<<"EXPECTED_BROKEN actual native victory/story/entry then abandon-retake retains or creates both infinite controllers; no seeded49176/49356\n";
        player.RemoveAllAuras();for(auto actor:Maidens(boss))actor->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();
    }
    session->SetPlayer(nullptr);sScenarioMgr->RemoveScenario(map.GetInstanceId());
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{InitializeIntro(argc,argv,true,true,true,true);HashMapHolder<EventObject>::SetSize(20000);
        auto& names=const_cast<ObjectMgr::ScriptNameContainer&>(sObjectMgr->GetAllScriptNames());
        std::string observer="zzzz_fuzan_transition_launch_probe";Require(names.back()<observer,"Appending observer must not shift actual existing script IDs");names.push_back(observer);
        auto& bindings=sScriptDataStore->*Access(IntroBindingsTag{});
        for(uint32 id:{218883u,218882u})bindings.emplace(id,std::make_pair(sObjectMgr->GetScriptId(observer.c_str()),true));
        auto nativeEvent=sEventObjectDataStore->GetEventObjectTemplate(103);
        Require(nativeEvent&&nativeEvent->ScriptID==sObjectMgr->GetScriptId("SmartEventObject"),"Research requires the original actual EO103 binding");
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());AddSC_SmartScripts();RegisterSpellScript(zzzz_fuzan_transition_launch_probe);
        RegisterCreatureAI(npc_fuzan_belphiar);RegisterGameObjectAI(go_fuzan_temple_sanctuary_door);sScriptMgr->SwapScriptContext(true);
        CheckTransition(3,1);CheckTransition(4,2);CheckTransition(4,3,true);CheckCompanionCasts();
        LoadBelphiarLoot();for(uint32 number=0;number<4;++number)CheckRealTransition(number==0?UINT32_MAX:number==1?0:number==2?200:1000,number+1);
        CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();return 0;
    }catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
