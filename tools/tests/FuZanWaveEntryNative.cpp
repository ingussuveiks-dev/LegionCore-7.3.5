// Actual victory/story/entry, native spell tickets, conversation, auras and
// owned minions. Earlier admission is controlled; no character save/client.
#define FUZAN_BELPHIAR_NATIVE_SUPPORT_ONLY
#include "FuZanBelphiarNative.cpp"

struct WaveLoadScriptsTag{using Type=void(Spell::*)();friend Type Access(WaveLoadScriptsTag);};
template struct FixtureAccess<WaveLoadScriptsTag,&Spell::LoadScripts>;
struct WavePendingTag{using Type=uint32 FuZanWaveEntry::Instance::*;friend Type Access(WavePendingTag);};
template struct FixtureAccess<WavePendingTag,&FuZanWaveEntry::Instance::_pending>;

struct WaveInstance : FuZanWaveEntry::Instance
{
    using FuZanWaveEntry::Instance::Instance;
    Player* cancelOwner=nullptr;bool cancelOnEntry=false,invoked=false;
    void onScenarionNextStep(uint32 step) override
    {
        if(step==5 && cancelOnEntry && cancelOwner && !invoked)
        {invoked=true;cancelOwner->SetQuestStatus(42765,QUEST_STATUS_NONE);cancelOwner->SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);}
    }
};
struct WaveEvent : EventObject
{
    ~WaveEvent(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<EventObject*>(this),false);ResetMap();}
};
static Position const entryPosition{898.03f,-2582.66f,181.311f,3.93323f};
static uint64 sourceGuid=40000;

struct WaveFixture
{
    MapInstanced parent;
    InstanceMap map;
    WaveInstance* state;
    Scenario* scenario;
    WorldSession* session;
    uint64 playerGuid;
    uint8 race;
    std::unique_ptr<LootPlayer> player;
    std::unique_ptr<LootPet> pet;
    std::unique_ptr<BelphiarGuardian> guardian;
    std::unique_ptr<BelphiarActor> boss,king,prior,yulon;
    std::unique_ptr<WaveEvent> event;
    ObjectGuid maiden;

    WaveFixture(uint8 playerRace,uint32 kind,uint32 number,bool receiver=true,bool instakill=false) : parent(1625,300),
        map(1625,300,9998000+number,DIFFICULTY_N_SCENARIO,&parent),playerGuid(20000+number),race(playerRace)
    {
        state=new WaveInstance(&map);map.*Access(IntroInstanceTag{})=state;
        scenario=sScenarioMgr->AddScenario(&map,1137);scenario->SetCurrentStep(2);
        session=new WorldSession(0,"Native Fu Zan wave-entry regression",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
        Connect(bossPosition);
        if(kind==1){pet=std::make_unique<LootPet>(*player,map);pet->setFaction(player->getFaction());pet->SetLevel(player->getLevel());}
        if(kind==2)guardian=std::make_unique<BelphiarGuardian>(*player,map);
        boss=std::make_unique<BelphiarActor>(map,109397,373405,bossPosition);
        king=std::make_unique<BelphiarActor>(map,109610,373384,Position{826.786f,-2549.49f,183.12f,1.2733f});
        prior=std::make_unique<BelphiarActor>(map,109465,373425,Position{896.955f,-2585.63f,181.381f,0.329788f});
        if(receiver)AddReceiver();
        Unit& killer=kind==1?static_cast<Unit&>(*pet):kind==2?static_cast<Unit&>(*guardian):static_cast<Unit&>(*player);
        if(instakill)killer.Kill(boss.get(),false);
        else killer.DealDamage(boss.get(),UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
        Require(!boss->IsAlive()&&scenario->GetCurrentStep()==3&&BelphiarCounter(scenario,49140)==1&&Maidens(*boss).size()==1,"Actual wave prerequisite victory missing");
        maiden=Maidens(*boss).front()->GetGUID();
    }
    void Connect(Position const& position)
    {
        player=std::make_unique<LootPlayer>(session,map,playerGuid,position);
        player->SetByteValue(UNIT_FIELD_BYTES_0,0,race);player->setFactionForRace(race);player->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
        player->GetZoneAndAreaId(player->m_zoneId,player->m_areaId);
        Require(player->m_zoneId==8262&&player->m_areaId==8262,"Native map cached location missing");
        player->SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);player->SetQuestSlot(0,42765);
        state->OnPlayerEnter(player.get()); // Actual production callback, before any later quest operation.
        state->cancelOwner=player.get();
    }
    void AddReceiver(){yulon=std::make_unique<BelphiarActor>(map,109391,373396,Position{940.823f,-2561.81f,180.15f,4.34808f});}
    void Story()
    {
        Pump(16000);Require(scenario->GetCurrentStep()==4&&BelphiarCounter(scenario,49356)==1,"Actual three-timer maiden receipt missing");
        map.PlayerRelocation(player.get(),entryPosition.GetPositionX(),entryPosition.GetPositionY(),entryPosition.GetPositionZ(),entryPosition.GetOrientation());
    }
    void Source()
    {
        event=std::make_unique<WaveEvent>();
        Require(event->Create(++sourceGuid,&map,1,103,entryPosition.GetPositionX(),entryPosition.GetPositionY(),entryPosition.GetPositionZ(),entryPosition.GetOrientation(),5,0,0),"Native event creation failed");
        event->m_DBTableGuid=400106;
        map.AddToGrid(static_cast<EventObject*>(event.get()),Cell(Trinity::ComputeCellCoord(entryPosition.GetPositionX(),entryPosition.GetPositionY())));
        event->AddToWorld();
    }
    void Pump(uint32 duration)
    {
        for(uint32 elapsed=0;elapsed<duration;elapsed+=10)
        {
            if(player)player->Unit::Update(10);
            for(auto actor:{king.get(),prior.get()})if(actor && actor->IsInWorld())
            {actor->Unit::Update(10);if(actor->IsAIEnabled)actor->AI()->UpdateAI(10);}
            if(boss)if(auto actor=ObjectAccessor::GetCreature(*boss,maiden))
            {actor->Unit::Update(10);if(actor->IsInWorld()&&actor->IsAIEnabled)actor->AI()->UpdateAI(10);}
            state->Update(10);
            if(event&&event->IsInWorld())event->Update(10);
            map.MoveAllCreaturesInMoveList();map.RemoveAllObjectsInRemoveList();
        }
    }
    void Cancel(){player->SetQuestStatus(42765,QUEST_STATUS_NONE);player->SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);}
    void Disconnect()
    {
        pet.reset();guardian.reset();state->OnPlayerLeave(player.get());player.reset();session->SetPlayer(nullptr);state->cancelOwner=nullptr;Pump(1000);
    }
    void RetrievalEffects()
    {
        // Component endpoint: execute only actual217814 effects4/5 and their
        // real target script dispatch. Full pickup/other effects are separate.
        TriggerCastData data;data.triggerFlags=TRIGGERED_NONE;
        Spell spell(player.get(),sSpellMgr->GetSpellInfo(217814),data);SpellCastTargets targets;targets.SetCaster(player.get());targets.SetUnitTarget(player.get());spell.InitExplicitTargets(targets);
        (spell.*Access(WaveLoadScriptsTag{}))();
        spell.HandleEffects(player.get(),nullptr,nullptr,EFFECT_4,SPELL_EFFECT_HANDLE_HIT_TARGET);
        spell.HandleEffects(player.get(),nullptr,nullptr,EFFECT_5,SPELL_EFFECT_HANDLE_HIT_TARGET);
    }
    void Check(bool companions,bool scene)
    {
        Require(player->HasAura(217659)==companions&&player->HasAura(217667)==companions,"Native controller state differs");
        Require(player->GetSummonList(109350)->size()==(companions?1u:0u)&&player->GetSummonList(109352)->size()==(companions?1u:0u),"Actual owned minion count differs");
        Require(player->HasAura(217817)==scene,"Actual own scene aura state differs");
        Require(BelphiarCounter(scenario,49140)==1&&BelphiarCounter(scenario,49356)==1&&BelphiarCounter(scenario,49176)==1,"Actual earned receipts changed/replayed");
        Require(!(state->*Access(WavePendingTag{})),"Native cast ACK still pending beyond regression horizon");
    }
    ~WaveFixture()
    {
        event.reset();if(player)player->RemoveAllAuras();
        if(boss)for(auto actor:Maidens(*boss))actor->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();
        yulon.reset();prior.reset();king.reset();boss.reset();pet.reset();guardian.reset();player.reset();session->SetPlayer(nullptr);
        sScenarioMgr->RemoveScenario(map.GetInstanceId());
    }
};

static void CheckWave(uint8 race,uint32 kind,uint32 mode,uint32 number)
{
    WaveFixture f(race,kind,number,mode!=4,mode==17);f.Story();f.Source();
    WorldSession* peerSession=nullptr;std::unique_ptr<LootPlayer> peer;
    if(mode==5)f.player->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);
    if(mode==6)f.player->setDeathState(CORPSE);
    if(mode==7)f.player->SetPhaseMask(2,false);
    if(mode==8)f.player->SetQuestStatus(42765,QUEST_STATUS_COMPLETE);
    if(mode==12)f.state->cancelOnEntry=true;
    if(mode==14){f.player->m_zoneId=0;f.player->m_areaId=0;} // Explicit incomplete-location fixture; retain native failed cast checks.
    if(mode==16)
    {
        f.map.PlayerRelocation(f.player.get(),bossPosition.GetPositionX(),bossPosition.GetPositionY(),bossPosition.GetPositionZ(),bossPosition.GetOrientation());
        peerSession=new WorldSession(0,"Forced non-admitted wave-entry bystander",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
        peer=std::make_unique<LootPlayer>(peerSession,f.map,400000+number,entryPosition);peer->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
        peer->SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);peer->SetQuestSlot(0,42765);
        f.event->Update(10);
        Require(!f.event->ActivatedForPlayer(peer.get())&&!BelphiarCounter(f.scenario,49176),"Forced eligible bystander replaced the actual solo winner");
        f.map.PlayerRelocation(f.player.get(),entryPosition.GetPositionX(),entryPosition.GetPositionY(),entryPosition.GetPositionZ(),entryPosition.GetOrientation());
    }
    if(mode==19)
    {
        f.Cancel();f.event->Update(10);f.Pump(7000);
        Require(!f.event->ActivatedForPlayer(f.player.get())&&!BelphiarCounter(f.scenario,49176)&&!f.player->HasAura(217659)&&!f.player->HasAura(217667),"Abandoned native victory admitted a new attempt");
        Require(!f.player->GetQuestCancellationReceipt(42765)->load(),"Old cancelled attempt poisoned the fresh task scope");
        std::cout<<"CASE wave-entry race="<<unsigned(race)<<" killer="<<kind<<" mode="<<mode<<" PASS\n";return;
    }
    if(mode==10)Require(f.player->CastSpell(f.player.get(),217659,false)!=SPELL_CAST_OK&&!f.player->HasAura(217659),"Unissued native child cast escaped its attempt ticket");
    f.event->Update(10);
    if(mode>=4&&mode<=7)
    {
        Require(!f.event->ActivatedForPlayer(f.player.get())&&f.scenario->GetCurrentStep()==4&&!BelphiarCounter(f.scenario,49176),"Rejected entry consumed native area/progression");
        if(mode==4)f.AddReceiver();
        if(mode==5)f.player->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
        if(mode==6){f.player->setDeathState(ALIVE);f.player->SetHealth(f.player->GetMaxHealth());}
        if(mode==7)f.player->SetPhaseMask(1,false);
        f.event->Update(10);
    }
    Require(f.scenario->GetCurrentStep()==5&&BelphiarCounter(f.scenario,49176)==1,"Native admitted entry missing");
    if(mode==1)f.Cancel();
    if(mode==2){f.Pump(200);f.Cancel();}
    if(mode==3){f.Pump(1000);f.Cancel();}
    if(mode==9)for(uint32 i=0;i<20;++i)f.event->Update(10);
    if(mode==13){f.event.reset();f.Pump(1000);f.Source();f.event->Update(10);}
    if(mode==14){f.Pump(1000);f.player->GetZoneAndAreaId(f.player->m_zoneId,f.player->m_areaId);}
    if(mode==15){f.yulon.reset();f.Pump(1000);f.AddReceiver();}
    if(mode==18)f.state->Update(60000);
    if(mode==11){f.Pump(1000);f.RetrievalEffects();}
    f.Pump(7000);
    bool cancelled=(mode>=1&&mode<=3)||mode==12;
    f.Check(!cancelled&&mode!=11,!cancelled);
    if(mode==12)Require(f.state->invoked,"Actual reentrant stage5 callback not exercised");
    if(peer){Require(!peer->HasAura(217659)&&!peer->HasAura(217667),"Native private controllers leaked to a bystander");peer.reset();peerSession->SetPlayer(nullptr);}
    std::cout<<"CASE wave-entry race="<<unsigned(race)<<" killer="<<kind<<" mode="<<mode<<" PASS\n";
}
static void CheckReconnect(uint8 race,uint32 mode,uint32 number)
{
    WaveFixture f(race,0,number);
    bool cancelBeforeEntry=mode==1;
    if(cancelBeforeEntry)f.Pump(8000);else{f.Story();f.Source();f.event->Update(10);}
    if(mode>=2){f.Pump(1000);f.Check(true,true);}
    if(mode==3){f.RetrievalEffects();f.Pump(1000);f.Check(false,true);}
    auto receipt=f.player->GetQuestCancellationReceipt(42765);f.Disconnect();
    f.Connect(cancelBeforeEntry?bossPosition:entryPosition);
    Require(f.player->GetQuestCancellationReceipt(42765)==receipt,"Actual OnPlayerEnter did not adopt the canonical attempt before input");
    if(cancelBeforeEntry)
    {
        f.Cancel();Require(receipt->load(),"Immediate post-entry abandonment missed original actors");f.Pump(17000);
        Require(!BelphiarCounter(f.scenario,49356)&&!BelphiarCounter(f.scenario,49176),"Reconnected old maiden advanced a retaken task");
    }
    else{f.Pump(7000);f.Check(mode!=3,mode==0);}
    std::cout<<"CASE wave-entry real Player destruction/recreation race="<<unsigned(race)<<" mode="<<mode<<" PASS\n";
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{
        InitializeIntro(argc,argv,true,true,true,true,true);HashMapHolder<EventObject>::SetSize(20000);LoadBelphiarLoot();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());RegisterCreatureAI(npc_fuzan_belphiar);RegisterGameObjectAI(go_fuzan_temple_sanctuary_door);sScriptMgr->SwapScriptContext(true);
        uint32 number=0;
        for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(uint32 kind=0;kind<3;++kind)for(uint32 mode=0;mode<20;++mode)CheckWave(race,kind,mode,++number);
        for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(uint32 mode=0;mode<4;++mode)CheckReconnect(race,mode,++number);
        CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();
        std::cout<<"PASS128 actual native wave-entry cases; original victory/story/entry receipts, exact effects/owned actor cardinality, cancellation/reentry/receiver admission and actual retrieval-effect endpoint. No saved character/full client pickup or chain claim.\n";return 0;
    }catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
