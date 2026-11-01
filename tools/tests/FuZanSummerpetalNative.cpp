// Real native kill, manual TempSummon, SmartScript text timers, target selection,
// criteria, conditions and follow. No character save or simulated credit.
#define FUZAN_BELPHIAR_NATIVE_SUPPORT_ONLY
#include "FuZanBelphiarNative.cpp"

struct SummerpetalInstance : InstanceScript
{
    using InstanceScript::InstanceScript;
    Player* recipient=nullptr;uint32 cancelStep=UINT32_MAX;bool invoked=false;
    void onScenarionNextStep(uint32 step) override
    {
        if(step!=cancelStep||!recipient||invoked)return;
        invoked=true;recipient->SetQuestStatus(42765,QUEST_STATUS_NONE);
        recipient->SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);
    }
};

static void SummerpetalCase(uint8 race,uint32 kind,uint32 mode,uint32 number)
{
    MapInstanced parent(1625,300);InstanceMap map(1625,300,9999600+number,DIFFICULTY_N_SCENARIO,&parent);
    auto instance=new SummerpetalInstance(&map);map.*Access(IntroInstanceTag{})=instance;
    Scenario* scenario=sScenarioMgr->AddScenario(&map,1137);scenario->SetCurrentStep(2);
    auto session=new WorldSession(0,"Fu Zan native Summerpetal fixture",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
    {
        LootPlayer player(session,map,8000+number,bossPosition);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);
        player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);player.SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,42765);
        if(mode==16||mode==17){instance->recipient=&player;instance->cancelStep=mode==16?3:4;}
        std::unique_ptr<LootPet> pet;std::unique_ptr<BelphiarGuardian> guardian;
        if(kind==1){pet=std::make_unique<LootPet>(player,map);pet->setFaction(player.getFaction());pet->SetLevel(player.getLevel());}
        if(kind==2)guardian=std::make_unique<BelphiarGuardian>(player,map);
        Unit& killer=kind==1?static_cast<Unit&>(*pet):kind==2?static_cast<Unit&>(*guardian):static_cast<Unit&>(player);
        BelphiarActor boss(map,109397,373405,bossPosition);
        if(mode==13)killer.Kill(&boss,false); // Actual instakill path bypasses damage hooks.
        else killer.DealDamage(&boss,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
        Require(!boss.IsAlive()&&scenario->GetCurrentStep()==3&&BelphiarCounter(scenario,49140)==1&&Maidens(boss).size()==1,"Actual native victory/summon missing");
        auto maiden=Maidens(boss).front();auto smart=dynamic_cast<SmartAI*>(maiden->AI());Require(smart,"Native maiden missing");
#ifndef FUZAN_SUMMERPETAL_BASELINE
        Require(dynamic_cast<npc_fuzan_summerpetal*>(maiden->AI()),"Repaired maiden factory missing");
#endif
        std::unique_ptr<BelphiarActor> king;
        auto addKing=[&](){king=std::make_unique<BelphiarActor>(map,109610,373384,Position{826.786f,-2549.49f,183.12f,1.2733f});};
        if(mode!=7)addKing();
        auto pump=[&](uint32 duration){for(uint32 elapsed=0;elapsed<duration;elapsed+=10){
            player.Unit::Update(10);if(king){king->Unit::Update(10);king->AI()->UpdateAI(10);}
            for(auto actor:Maidens(boss)){actor->Unit::Update(10);if(actor->IsInWorld()&&actor->IsAIEnabled)actor->AI()->UpdateAI(10);}
            map.MoveAllCreaturesInMoveList();map.RemoveAllObjectsInRemoveList();
        }};
        WorldSession* peerSession=nullptr;std::unique_ptr<LootPlayer> peer;
        if(mode==15){king->AI()->UpdateAI(10);player.Relocate(king->GetPosition());king->AI()->MoveInLineOfSight_Safe(&player);
            Require(dynamic_cast<SmartAI*>(king->AI())->GetScript()->HasRunEvent(1,SMART_EVENT_OOC_LOS),"Original early King LOS counterfactual missing");player.Relocate(bossPosition);}
        if(mode==10){Require(maiden->AIM_Destroy()&&maiden->AIM_Initialize(),"Actual maiden AI recreation failed");smart=dynamic_cast<SmartAI*>(maiden->AI());}
        if(mode==11)player.CancelQuestObjects(42765); // Native cancellation also invalidates a remote/unloaded actor's receipt.
        if(mode==12){king->AI()->UpdateAI(10);auto home=player.GetPosition();player.Relocate(king->GetPosition());king->AI()->MoveInLineOfSight_Safe(&player);
            Require(!dynamic_cast<SmartAI*>(king->AI())->GetScript()->HasRunEvent(1,SMART_EVENT_OOC_LOS),"King native early LOS consumed departure before actual stage4");player.Relocate(home);}
        if(mode==2)pump(8000);
        if(mode==1||mode==2)player.Relocate(doorPosition);
        if(mode==3){peerSession=new WorldSession(0,"Fu Zan nearer bystander",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
            peer=std::make_unique<LootPlayer>(peerSession,map,10000+number,maiden->GetPosition());}
        if(mode==4)player.setDeathState(CORPSE);
        if(mode==5||mode==6){player.SetQuestStatus(42765,QUEST_STATUS_NONE);if(mode==6)player.SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);}
        if(mode==8)for(uint32 i=0;i<3;++i)maiden->AI()->Reset();
        if(mode==9)maiden->AI()->UpdateAI(60000);
        pump(16000);
#ifdef FUZAN_SUMMERPETAL_BASELINE
        Require(mode==0||mode==1||mode==15,"Counterfactual only original present/distant/early-King recipients");
        Require(BelphiarCounter(scenario,49356)==(mode?0u:1u),"Original recipient counterfactual differs");
        player.Relocate(bossPosition);pump(30000);
        Require(BelphiarCounter(scenario,49356)==(mode?0u:1u),"Original returned recipient unexpectedly recovered");
        std::cout<<(mode==15?"EXPECTED_BROKEN original early King LOS overwrites pending maiden text; zero49356 and native maiden cleanup":mode==1?"EXPECTED_BROKEN original native dialogue consumed without49356 and never recovers":"PASS original native present-recipient control")<<'\n';
#else
        if((mode>=1&&mode<=6)||mode==11||mode==16){Require(!BelphiarCounter(scenario,49356)&&scenario->GetCurrentStep()==3,"Absent/dead/bystander/cancelled owner incorrectly advanced");}
        if(mode==5||mode==6||mode==11||mode==16||mode==17){Require(Maidens(boss).empty(),"Cancelled attempt retained its own maiden");
            if(mode==16||mode==17)Require(instance->invoked&&BelphiarCounter(scenario,49356)==(mode==17?1u:0u)&&
                !dynamic_cast<SmartAI*>(king->AI())->GetScript()->HasRunEvent(0,SMART_EVENT_DATA_SET),"Reentrant cancellation replayed owned handoff or lost actual pre-cancel credit");}
        else{
            if(mode==1||mode==2)player.Relocate(bossPosition);
            if(mode==3){peer.reset();peerSession->SetPlayer(nullptr);}
            if(mode==4){player.setDeathState(ALIVE);player.SetHealth(player.GetMaxHealth());}
            if(mode==7){Require(BelphiarCounter(scenario,49356)==1&&smart->GetFollowGuid().IsEmpty(),"Missing King replayed/advanced follow");addKing();}
            pump(17000);
            Require(BelphiarCounter(scenario,49356)==1&&scenario->GetCurrentStep()==4,"Actual native owner story did not recover exactly once");
            Require(king&&dynamic_cast<SmartAI*>(king->AI())->GetScript()->HasRunEvent(0,SMART_EVENT_DATA_SET),"Native King acknowledgement missing");
            Require(smart->GetFollowGuid()==player.GetGUID(),"Original follow selector did not resolve actual owner");
            if(mode==12){player.Relocate(king->GetPosition());king->AI()->MoveInLineOfSight_Safe(&player);
                Require(dynamic_cast<SmartAI*>(king->AI())->GetScript()->HasRunEvent(1,SMART_EVENT_OOC_LOS),"King native stage4 LOS admission missing");player.Relocate(bossPosition);}
            boss.AI()->JustDied(&killer);pump(2000);
            Require(BelphiarCounter(scenario,49140)==1&&BelphiarCounter(scenario,49356)==1&&Maidens(boss).size()==1,"Repeated native notification replayed progression/summon");
            // Original6s King speech +10s farewell + native5s invisible despawn stage.
            if(mode==14){player.Relocate(king->GetPosition());king->AI()->MoveInLineOfSight_Safe(&player);pump(23000);
                Require(dynamic_cast<SmartAI*>(king->AI())->GetScript()->HasRunEvent(2,SMART_EVENT_TEXT_OVER),"Native King speech-to-path transition missing");
                if(!Maidens(boss).empty())std::cout<<"OBSERVATION farewell maiden="<<Maidens(boss).size()<<" distance="<<maiden->GetExactDist(king.get())
                    <<" data7="<<smart->GetScript()->HasRunEvent(7,SMART_EVENT_DATA_SET)<<" text8="<<smart->GetScript()->HasRunEvent(8,SMART_EVENT_TEXT_OVER)
                    <<" kingLink5="<<dynamic_cast<SmartAI*>(king->AI())->GetScript()->HasRunEvent(5,SMART_EVENT_LINK)<<" counter="<<BelphiarCounter(scenario,49356)<<std::endl;
                Require(Maidens(boss).empty()&&BelphiarCounter(scenario,49356)==1,"Native King data/farewell did not clean up maiden exactly once");}
        }
#endif
        peer.reset();if(peerSession)peerSession->SetPlayer(nullptr);
        for(auto actor:Maidens(boss))actor->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();
        std::cout<<"CASE Summerpetal race="<<unsigned(race)<<" killer="<<kind<<" mode="<<mode<<" PASS\n";
    }
    session->SetPlayer(nullptr);sScenarioMgr->RemoveScenario(map.GetInstanceId());
}
static void SummerpetalReconnect(uint8 race,bool cancelled,uint32 number)
{
    MapInstanced parent(1625,300);InstanceMap map(1625,300,9999400+number,DIFFICULTY_N_SCENARIO,&parent);
    map.*Access(IntroInstanceTag{})=new InstanceScript(&map);
    auto scenario=sScenarioMgr->AddScenario(&map,1137);scenario->SetCurrentStep(2);
    auto session=new WorldSession(0,"Fu Zan native Player lifetime fixture",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
    {
        std::unique_ptr<LootPlayer> player;
        auto connect=[&](){player=std::make_unique<LootPlayer>(session,map,12000+number,bossPosition);
            player->SetByteValue(UNIT_FIELD_BYTES_0,0,race);player->setFactionForRace(race);player->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
            player->SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);player->SetQuestSlot(0,42765);};
        connect();BelphiarActor boss(map,109397,373405,bossPosition);
        player->Kill(&boss,false);Require(scenario->GetCurrentStep()==3&&Maidens(boss).size()==1,"Reconnect native victory missing");
        auto maiden=Maidens(boss).front();auto smart=dynamic_cast<SmartAI*>(maiden->AI());
        auto receipt=player->GetQuestCancellationReceipt(42765);
        auto pump=[&](uint32 duration){for(uint32 elapsed=0;elapsed<duration;elapsed+=10){
            if(player)player->Unit::Update(10);
            for(auto actor:Maidens(boss)){actor->Unit::Update(10);if(actor->IsInWorld()&&actor->IsAIEnabled)actor->AI()->UpdateAI(10);}
            map.MoveAllCreaturesInMoveList();map.RemoveAllObjectsInRemoveList();}};
        pump(8000);
        if(cancelled)player->CancelQuestObjects(42765);
        player.reset();session->SetPlayer(nullptr); // Actual Player/QuestStatusData/scope destruction, no save.
        Require(receipt->load(std::memory_order_acquire)==cancelled,"Native destruction changed cancellation identity");
        pump(16000);Require(!BelphiarCounter(scenario,49356),"Missing native Player received story progression");
        connect();BelphiarActor king(map,109610,373384,{826.786f,-2549.49f,183.12f,1.2733f});pump(17000);
        if(cancelled)Require(!BelphiarCounter(scenario,49356)&&Maidens(boss).empty(),"Cancelled old Player advanced new quest attempt");
        else Require(BelphiarCounter(scenario,49356)==1&&smart->GetFollowGuid()==player->GetGUID(),"Same-GUID native Player reentry lost pending owned story");
        for(auto actor:Maidens(boss))actor->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();player.reset();
        std::cout<<"CASE native Player destruction/recreation race="<<unsigned(race)<<" cancelled="<<cancelled<<" PASS\n";
    }
    session->SetPlayer(nullptr);sScenarioMgr->RemoveScenario(map.GetInstanceId());
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{
        InitializeIntro(argc,argv,true,true,true);LoadBelphiarLoot();HashMapHolder<GameObject>::SetSize(20000);
        auto info=sObjectMgr->GetCreatureTemplate(109397);Require(info&&info->ScriptID==sObjectMgr->GetScriptId("npc_fuzan_belphiar"),"Actual parent binding missing");
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());RegisterCreatureAI(npc_fuzan_belphiar);RegisterGameObjectAI(go_fuzan_temple_sanctuary_door);sScriptMgr->SwapScriptContext(true);
        uint32 number=0;
        for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(uint32 kind=0;kind<3;++kind)
#ifdef FUZAN_SUMMERPETAL_BASELINE
            for(uint32 mode:{0u,1u,15u})SummerpetalCase(race,kind,mode,++number);
#else
            for(uint32 mode=0;mode<18;++mode)if(mode!=15)SummerpetalCase(race,kind,mode,++number);
#endif
#ifndef FUZAN_SUMMERPETAL_BASELINE
        for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(bool cancelled:{false,true})SummerpetalReconnect(race,cancelled,++number);
#endif
        CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();
        std::cout<<"PASS native Summerpetal matrix; read-only pools and private metadata only; no client traversal claim\n";return 0;
    }catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
