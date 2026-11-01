// Actual victory/entry, native phases, four waves, motion, dialogue and finale.
// Earlier stage2 admission and combat inputs are controlled; no client/save.
#if !defined(FUZAN_DEFENSE_CRITERIA_BASELINE) && !defined(FUZAN_DEFENSE_CRITERIA_LIVE)
#define FUZAN_DEFENSE_CRITERIA_REPAIR
#endif
#define FUZAN_WAVE_ENTRY_NATIVE_SUPPORT_ONLY
#include "FuZanWaveEntryNative.cpp"
#include "PhaseMgr.h"

static std::string nativeYuLonPhase;
static std::list<Creature*> DefenseActors(Creature& source)
{
    std::list<Creature*> result;
    for(uint32 entry:{109438u,109511u,109933u,110471u})source.GetCreatureListWithEntryInGridAppend(result,entry,300);
    return result;
}
static unsigned LivingDefense(Creature& source)
{
    auto actors=DefenseActors(source);return unsigned(std::count_if(actors.begin(),actors.end(),[](Creature* actor){return actor->IsAlive();}));
}
static void PhaseUpdate(Player& player)
{
    if(player.NeedPhaseRecalculate)player.GetPhaseMgr().Recalculate();
    if(player.NeedPhaseUpdate)player.GetPhaseMgr().Update();
}
struct DefenseFixture
{
    WaveFixture wave;
    uint32 kind;
    std::unique_ptr<BelphiarActor> korithis,corrupted;
    DefenseFixture(uint8 race,uint32 killer,uint32 number):wave(race,killer,number),kind(killer)
    {
        auto& f=wave;
        f.player->GetPhaseMgr().Recalculate();f.player->GetPhaseMgr().Update();
        Require(f.player->HasPhaseId(6905)&&!f.player->HasPhaseId(6906),"Native initial quest phase differs");
        for(auto actor:{f.boss.get(),f.king.get(),f.prior.get(),f.yulon.get()})actor->SetPhaseId({6905},false);
        for(auto actor:Maidens(*f.boss))actor->SetPhaseId({6905},false);
#ifndef FUZAN_DEFENSE_PHASE_BASELINE
        f.yulon->SetPhaseId({6905,6906},false); // Known future single-spawn data projection only.
#endif
        if(f.pet)f.pet->SetPhaseId(f.player->GetPhases(),false);
        if(f.guardian)f.guardian->SetPhaseId(f.player->GetPhases(),false);
        f.Story();f.Source();f.event->Update(10);
        // Run the native Player phase-update slice before the original200ms
        // casts, just as Player::Update does; do not defer it until after them.
        for(uint32 time=0;time<1000;time+=10){f.Pump(10);PhaseUpdate(*f.player);}
        f.Check(true,true);
        Require(f.yulon->HasAura(130966),"Actual native meditation addon missing");
        korithis=std::make_unique<BelphiarActor>(f.map,109821,14670029,Position{885.978f,-2729.15f,192.165f,1.28883f});
        korithis->SetPhaseId({6906},false);
        corrupted=std::make_unique<BelphiarActor>(f.map,109820,14670030,Position{872.483f,-2772.84f,211.671f,1.26685f});
        corrupted->SetPhaseId({6906},false);
        Pump(1000);
        Require(f.player->HasPhaseId(6905)&&f.player->HasPhaseId(6906),"Native stage5 phase notification/definition missing");
    }
    void Pump(uint32 duration)
    {
        auto& f=wave;
        for(uint32 elapsed=0;elapsed<duration;elapsed+=10)
        {
            f.Pump(10);if(f.player)PhaseUpdate(*f.player);
            for(auto actor:{f.yulon.get(),korithis.get(),corrupted.get()})if(actor&&actor->IsInWorld())
            {actor->Unit::Update(10);if(actor->IsAIEnabled)actor->AI()->UpdateAI(10);}
            // Birth casts/splines are native. Wave combat AI is intentionally
            // not ticked: deaths below are actual controlled damage inputs.
            for(auto actor:DefenseActors(*f.yulon))if(actor->IsInWorld())actor->Unit::Update(10);
            f.map.MoveAllCreaturesInMoveList();f.map.RemoveAllObjectsInRemoveList();
        }
    }
    Unit& Killer(Creature& target)
    {
        auto& f=wave;
        if(kind==1&&f.pet){f.map.CreatureRelocation(f.pet.get(),target.GetPositionX(),target.GetPositionY(),target.GetPositionZ(),0);f.pet->SetPhaseId(f.player->GetPhases(),false);return *f.pet;}
        if(kind==2&&f.guardian){f.map.CreatureRelocation(f.guardian.get(),target.GetPositionX(),target.GetPositionY(),target.GetPositionZ(),0);f.guardian->SetPhaseId(f.player->GetPhases(),false);return *f.guardian;}
        if(kind==3||kind==4)
        {
            uint32 entry=kind==3?109350:109352,spell=kind==3?217666:217669;
            auto actors=f.player->GetSummonList(entry);Require(actors->size()==1,"Actual native companion missing before its killing blow");
            auto actor=ObjectAccessor::GetCreature(*f.player,actors->front());
            Require(actor&&actor->IsAlive()&&actor->ToTempSummon()&&actor->ToTempSummon()->GetSummonerGUID()==f.player->GetGUID()&&actor->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL)==spell,"Native companion origin/owner differs");
            // This is the actual controller-created actor; retain its native
            // phase/ownership, unlike the separate pet/Guardian fixture setup.
            f.map.CreatureRelocation(actor,target.GetPositionX(),target.GetPositionY(),target.GetPositionZ(),0);return *actor;
        }
        return *f.player;
    }
    void Kill(Creature& actor,bool repeat)
    {
        Unit& killer=Killer(actor);Require(killer.InSamePhase(&actor),"Native damage input has incompatible phases");
        killer.DealDamage(&actor,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
        Require(!actor.IsAlive(),"Actual native killing blow did not kill target");
        if(repeat)actor.AI()->JustDied(&killer); // Repeated callback must retain original once flags.
    }
    void Finale(bool repeat)
    {
        auto& f=wave;Pump(60000);
        auto bossScript=dynamic_cast<SmartAI*>(korithis->AI())->GetScript();
        Require(f.scenario->GetCurrentStep()==6&&BelphiarCounter(f.scenario,49177)==1&&BelphiarCounter(f.scenario,49351)==1,"Actual native path/criteria missing");
        Require(dynamic_cast<SmartAI*>(f.yulon->AI())->GetScript()->HasRunEvent(10,SMART_EVENT_UPDATE),"Original UPDATE target0 positive control did not run");
#ifdef FUZAN_DEFENSE_PHASE_BASELINE
        Require(!bossScript->HasRunEvent(0,SMART_EVENT_DATA_SET)&&!bossScript->HasRunEvent(2,SMART_EVENT_DATA_SET)&&!f.yulon->InSamePhase(korithis.get()),"Original phase gap not reproduced");
#else
        Require(bossScript->HasRunEvent(0,SMART_EVENT_DATA_SET)&&bossScript->HasRunEvent(2,SMART_EVENT_DATA_SET),"Actual native confrontation replies missing");
        f.map.PlayerRelocation(f.player.get(),886,-2710,192.165f,0);Kill(*korithis,repeat);
        std::list<GameObject*> weapons;korithis->GetGameObjectListWithEntryInGrid(weapons,251605,100);
        std::list<Creature*> spirits;korithis->GetCreatureListWithEntryInGrid(spirits,109432,100);
        Require(!corrupted->IsAlive()&&BelphiarCounter(f.scenario,49179)==1&&f.scenario->GetCurrentStep()==7&&weapons.size()==1&&spirits.size()==1,"Actual native finale receipt/weapon/actors missing");
        for(auto weapon:weapons)weapon->Delete();for(auto spirit:spirits)spirit->DespawnOrUnsummon();f.map.RemoveAllObjectsInRemoveList();
#endif
    }
    ~DefenseFixture()
    {
        for(auto actor:DefenseActors(*wave.yulon))actor->DespawnOrUnsummon();wave.map.RemoveAllObjectsInRemoveList();
    }
};
static void CheckDefense(uint8 race,uint32 killer,bool impsFirst,bool repeat,uint32 number)
{
    DefenseFixture d(race,killer,number);auto& f=d.wave;
    for(unsigned wave=1;wave<=4;++wave)
    {
        auto actors=DefenseActors(*f.yulon);actors.remove_if([](Creature* actor){return !actor->IsAlive();});
        Require(actors.size()==8,"Actual native defense wave cardinality differs");
        actors.sort([=](Creature* a,Creature* b){return impsFirst?a->GetEntry()>b->GetEntry():a->GetEntry()<b->GetEntry();});
        for(auto actor:actors)d.Kill(*actor,repeat);
        d.Pump(100);
#ifdef FUZAN_DEFENSE_CRITERIA_BASELINE
        Require(wave==1&&!LivingDefense(*f.yulon)&&BelphiarCounter(f.scenario,49568)==3&&BelphiarCounter(f.scenario,49620)==5,"Original first-wave loss not reproduced");
        Require(!sConditionMgr->IsObjectMeetingSmartEventConditions(109391,8,SMART_SCRIPT_TYPE_CREATURE,nullptr,f.yulon.get()),"Native outer null-target rejection missing");
        std::cout<<"EXPECTED_BROKEN native first wave credited25 then DATA8 rejected absent target0; race="<<unsigned(race)<<" killer="<<killer<<" order="<<impsFirst<<" repeat="<<repeat<<'\n';return;
#endif
    }
    Require(BelphiarCounter(f.scenario,49568)==12&&BelphiarCounter(f.scenario,49620)==20&&!LivingDefense(*f.yulon),"Native four-wave weighted100/once receipts differ");
    d.Finale(repeat);
#ifdef FUZAN_DEFENSE_PHASE_BASELINE
    std::cout<<"EXPECTED_BROKEN actual weighted100/path/step6 then phase6905-to6906 loses both confrontation replies; race="<<unsigned(race)<<" killer="<<killer<<" order="<<impsFirst<<" repeat="<<repeat<<'\n';
#else
    std::cout<<"CASE actual native defense/finale race="<<unsigned(race)<<" killer="<<killer<<" order="<<impsFirst<<" repeat="<<repeat<<" PASS\n";
#endif
}
static void CheckDefenseReentry(uint8 race,uint32 number)
{
    DefenseFixture d(race,0,number);auto& f=d.wave;
    auto actors=DefenseActors(*f.yulon);actors.remove_if([](Creature* actor){return !actor->IsAlive();});
    Require(actors.size()==8,"Actual first wave missing before reentry");
    d.Kill(*actors.front(),false);auto strong=BelphiarCounter(f.scenario,49568),imps=BelphiarCounter(f.scenario,49620);
    auto receipt=f.player->GetQuestCancellationReceipt(42765);f.Disconnect();d.Pump(1000);f.Connect(entryPosition);
    f.player->GetPhaseMgr().Recalculate();f.player->GetPhaseMgr().Update();d.Pump(7000);f.Check(true,false);
    Require(f.player->GetQuestCancellationReceipt(42765)==receipt&&!receipt->load()&&BelphiarCounter(f.scenario,49568)==strong&&BelphiarCounter(f.scenario,49620)==imps,"Actual partial-wave reconnect lost attempt/progress");
    for(unsigned wave=1;wave<=4;++wave)
    {
        actors=DefenseActors(*f.yulon);actors.remove_if([](Creature* actor){return !actor->IsAlive();});
        Require(actors.size()==(wave==1?7u:8u),"Native reentry changed outstanding wave cardinality");
        for(auto actor:actors)d.Kill(*actor,true);d.Pump(100);
    }
    Require(BelphiarCounter(f.scenario,49568)==12&&BelphiarCounter(f.scenario,49620)==20&&!LivingDefense(*f.yulon),"Reentered native weighted receipts differ");
    d.Finale(true);std::cout<<"CASE actual partial-wave same-GUID Player destruction/recreation race="<<unsigned(race)<<" PASS\n";
}
static void CheckDefenseAbandon(uint8 race,uint32 number)
{
    DefenseFixture d(race,0,number);auto& f=d.wave;
    auto receipt=f.player->GetQuestCancellationReceipt(42765);f.player->SetQuestStatus(42765,QUEST_STATUS_NONE);d.Pump(1000);
    Require(receipt->load()&&!f.player->HasPhaseId(6905)&&!f.player->HasPhaseId(6906)&&!f.player->HasAura(217659)&&!f.player->HasAura(217667),"Native abandon phase/controller cleanup missing");
    auto actors=DefenseActors(*f.yulon);actors.remove_if([](Creature* actor){return !actor->IsAlive();});
    Require(actors.size()==8,"Native environment wave changed before cancellation check");
    for(auto actor:actors)Require(!actor->SelectNearestPlayer(100),"Abandoned phased-out owner remains a native credit selector");
    // A late NPC death remains a real native callback; no eligible Player is
    // selected after phase removal. This does not erase the map's world NPCs.
    actors.front()->Kill(actors.front(),false);d.Pump(100);
    Require(!BelphiarCounter(f.scenario,49568)&&!BelphiarCounter(f.scenario,49620)&&f.scenario->GetCurrentStep()==5,"Native late death advanced abandoned player progress");
    std::cout<<"CASE actual abandon/native phase removal/late NPC death race="<<unsigned(race)<<" PASS\n";
}
#ifndef FUZAN_DEFENSE_NATIVE_SUPPORT_ONLY
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{
        InitializeIntro(argc,argv,true,true,true,true,true,true);HashMapHolder<EventObject>::SetSize(20000);HashMapHolder<GameObject>::SetSize(20000);LoadBelphiarLoot();
        auto rules=WorldDatabase.Query("SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id IN(49567,49568,49620,49177,49351,49179)");
        Require(rules&&!rules->Fetch()[0].GetUInt32(),"Additional native defense/finale criterion rules must be loaded");
        auto phase=WorldDatabase.Query("SELECT PhaseId FROM creature WHERE guid=373396 AND id=109391 AND map=1625");
        Require(phase&&phase->GetRowCount()==1,"Actual native Yu'lon source missing");nativeYuLonPhase=phase->Fetch()[0].GetString();
        Require((nativeYuLonPhase=="6905"||nativeYuLonPhase=="6905 6906")&&sPhaseStore.LookupEntry(6905)&&sPhaseStore.LookupEntry(6906),"Changed/unknown actual phase context");
#ifdef FUZAN_DEFENSE_CRITERIA_LIVE
        Require(nativeYuLonPhase=="6905 6906","Actual live native phase bridge missing");
#endif
        auto& table=LootTemplates_Creature.*Access(LootTableNameTag{});auto original=table;table+=" WHERE Entry IN(109395,109397,109438,109933,109821) ORDER BY Entry,Item";
        auto count=(LootTemplates_Creature.*Access(LoadLootTableTag{}))();table=original;
        Require(count==12&&LootTemplates_Creature.HaveLootFor(109438)&&LootTemplates_Creature.HaveLootFor(109933),"Actual native defense loot context missing");
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());RegisterCreatureAI(npc_fuzan_belphiar);RegisterGameObjectAI(go_fuzan_temple_sanctuary_door);sScriptMgr->SwapScriptContext(true);
        uint32 number=600;
        for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(uint32 killer=0;killer<5;++killer)for(bool order:{false,true})for(bool repeat:{false,true})CheckDefense(race,killer,order,repeat,++number);
#if !defined(FUZAN_DEFENSE_CRITERIA_BASELINE) && !defined(FUZAN_DEFENSE_PHASE_BASELINE)
        for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)}){CheckDefenseReentry(race,++number);CheckDefenseAbandon(race,++number);}
#endif
        CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();
#if defined(FUZAN_DEFENSE_CRITERIA_BASELINE) || defined(FUZAN_DEFENSE_PHASE_BASELINE)
        std::cout<<"EXPECTED_BROKEN40 isolated metadata counterfactuals reproduced; process success is not an ordinary repaired defense/finale pass.\n";
#else
        std::cout<<"PASS44 actual native defense cases; native phases, spawns, scaled one-hit death, Player/pet/Guardian/both actual companion endpoints, exact weighted100, partial-wave reconnect/abandon, path/replies and original finale. Wave combat AI/client pickup/save are separate.\n";
#endif
        return 0;
    }catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
#endif
