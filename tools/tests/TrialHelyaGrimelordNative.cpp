// Native Grimelord AI, spells and aura lifecycle after earned raid admission.
#define TRIAL_HELYA_PHASE3_SUPPORT_ONLY
#include "TrialHelyaPhase3Native.cpp"
struct GrimValueTag{using Type=SpellValue* Spell::*;friend Type Access(GrimValueTag);};
template struct FixtureAccess<GrimValueTag,&Spell::m_spellValue>;
struct GrimWatch
{
    InstanceMap& map;ObjectGuid grim;uint32 elapsed=0;
    std::map<uint32,std::set<ObjectGuid>> casts,targets,originals,damaged;
    std::map<uint32,std::set<ObjectGuid>> unitTargets;
    std::map<uint32,std::map<ObjectGuid,std::set<ObjectGuid>>> perCastTargets;
    std::map<uint32,std::vector<float>> values;
    std::map<uint32,std::vector<uint32>> times;
    std::set<ObjectGuid> centeredNovas;
    GrimWatch(InstanceMap& m,Creature& g):map(m),grim(g.GetGUID()){}
    void Observe()
    {
        std::vector<Unit*> casters;if(auto g=map.GetCreature(grim))casters.push_back(g);
        for(auto const& ref:map.GetPlayers())casters.push_back(ref.getSource());
        for(auto caster:casters)for(auto id:{228838u,232408u,228397u,228519u,228390u})for(auto s:NativeSpellEvents(*caster,id)){
            if(casts[id].insert(s->m_castGuid[0]).second){values[id].push_back((s->*Access(GrimValueTag{}))->EffectBasePoints[0]);times[id].push_back(elapsed);unitTargets[id].insert(s->m_targets.GetUnitTargetGUID());if(auto original=s->GetOriginalCaster())originals[id].insert(original->GetGUID());}
            unsigned effect=id==228519?1:0;for(auto const& target:*s->GetUniqueTargetInfo())if(target->effectMask&(1<<effect)){targets[id].insert(target->targetGUID);perCastTargets[id][s->m_castGuid[0]].insert(target->targetGUID);if(target->processed&&target->damage>0)damaged[id].insert(target->targetGUID);}
            if(id==228390&&s->m_targets.HasDst()&&caster->GetExactDist(s->m_targets.GetDstPos())<0.05f)centeredNovas.insert(s->m_castGuid[0]);
        }
    }
    void Advance(uint32 ms,bool ai=false)
    {
        Observe();for(unsigned t=0;t<ms;t+=10){if(ai)PumpHelya(map,10);else if(auto g=map.GetCreature(grim))g->Unit::Update(10);Observe();for(auto const& ref:map.GetPlayers()){auto p=ref.getSource();if(p->IsAlive())p->SetHealth(p->GetMaxHealth());p->Unit::Update(10);Observe();}elapsed+=10;}
    }
};
static Creature* GrimBegin(GarmPlayer& p,InstanceMap& map,Creature& helya)
{
    ClearCombat(p,helya);PumpHelya(map,12000);if(helya.isInCombat()){ClearCombat(p,helya);PumpHelya(map,12000);}
    Require(map.GetInstanceScript()->GetBossState(2)==NOT_STARTED&&!helya.HasUnitState(UNIT_STATE_EVADE),"Grimelord retry did not finish actual Helya reset");
    for(auto const& ref:map.GetPlayers()){ref.getSource()->RemoveAura(193367);ref.getSource()->RemoveAura(228519);ref.getSource()->SetGameMaster(false);}
    map.PlayerRelocation(&p,510,670,2,0);helya.SetInCombatWith(&p);
    for(unsigned i=0;i<16&&helya.AI()->GetData(2)==1;++i)p.DealDamage(&helya,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
    Require(helya.AI()->GetData(2)==2,"Grimelord bootstrap missed phase two");
    PumpHelya(map,7000);Require(helya.AI()->GetData(2)==2,"Grimelord bootstrap lost settled phase two");
    map.PlayerRelocation(&p,458,640,7.5f,0);auto grim=helya.SummonCreature(114709,458,639,7.5f,0,TEMPSUMMON_MANUAL_DESPAWN);Require(grim&&grim->AI(),"Native Grimelord summon missing");auto guid=grim->GetGUID();PumpHelya(map,20);Require(map.GetCreature(guid)==grim&&grim->IsInWorld(),"Grimelord left map during native bootstrap");std::cout<<"GRIM_BEGIN guid="<<guid.GetCounter()<<" combat="<<grim->isInCombat()<<" phase="<<helya.AI()->GetData(2)<<std::endl;return grim;
}
static void GrimAbilities(GarmPlayer& p,GarmPlayer& closePlayer,GarmPlayer& distantPlayer,LootPet& pet,InstanceMap& map,Creature& helya)
{
    auto grim=GrimBegin(p,map,helya);map.PlayerRelocation(&closePlayer,500,640,7.5f,0);map.PlayerRelocation(&distantPlayer,520,640,7.5f,0);
    pet.SetMaxHealth(p.GetMaxHealth());pet.SetHealth(p.GetMaxHealth());grim->AddThreat(&p,100000);grim->AddThreat(&closePlayer,1);grim->AddThreat(&distantPlayer,1);grim->AddThreat(&pet,1);
    GrimWatch rotation(map,*grim);rotation.Advance(70000,true);
    std::cout<<"GRIM_ROTATION slam=";for(auto t:rotation.times[228519])std::cout<<t<<',';std::cout<<" nova=";for(auto t:rotation.times[228390])std::cout<<t<<',';std::cout<<" rot=";for(auto t:rotation.times[228838])std::cout<<t<<',';std::cout<<" slamVictim="<<(rotation.unitTargets[228519]==std::set<ObjectGuid>{p.GetGUID()})<<" centeredNovas="<<rotation.centeredNovas.size()<<std::endl;
    Require(rotation.casts[228519].size()>=3&&rotation.casts[228390].size()>=2,"Native Grimelord Slam/Nova did not repeat");
    Require(rotation.unitTargets[228519]==std::set<ObjectGuid>{p.GetGUID()}&&rotation.centeredNovas.size()==rotation.casts[228390].size(),"Native AI chose wrong Slam victim/Nova destination");
    unsigned firstSlam=map.IsLfrRaid()?14000:12000,firstNova=map.IsLfrRaid()||map.IsNormalRaid()?17000:14000,novaRepeat=24300;
#ifdef TRIAL_HELYA_GRIMELORD_BASELINE
    firstSlam=14000;firstNova=16000;novaRepeat=25000;
#endif
    Require(rotation.times[228519].front()+40>=firstSlam&&rotation.times[228519].front()<=firstSlam+100,"Native initial Slam timer wrong");
    Require(rotation.times[228390].front()+40>=firstNova&&rotation.times[228390].front()<=firstNova+100,"Native initial Nova timer wrong");
    for(unsigned i=1;i<rotation.times[228390].size();++i){auto delta=rotation.times[228390][i]-rotation.times[228390][i-1];Require(delta>=novaRepeat&&delta<=novaRepeat+2000,"Native Nova repeat timer wrong");}
#ifndef TRIAL_HELYA_GRIMELORD_BASELINE
    Require(rotation.casts[228838].size()>=3&&!pet.HasAura(193367),"Native full-raid Rot selector did not repeat or selected Pet");
    for(auto const& cast:rotation.perCastTargets[228838]){Require(cast.second.size()==2,"Native Rot selector lost its two-Player cap");for(auto guid:cast.second)Require(guid.IsPlayer(),"Native full-raid selector admitted a non-Player");}
    unsigned repeat=map.IsLfrRaid()||map.IsNormalRaid()?15800:12200;auto const& times=rotation.times[228838];for(unsigned i=1;i<times.size();++i)Require(times[i]-times[i-1]>=repeat&&times[i]-times[i-1]<=repeat+5000,"Native repeated Rot used wrong difficulty interval");
#endif
    grim=GrimBegin(p,map,helya);map.PlayerRelocation(&p,470,640,7.5f,0);map.PlayerRelocation(&closePlayer,474,640,7.5f,0);map.PlayerRelocation(&distantPlayer,480,640,7.5f,0);
    GrimWatch slam(map,*grim);grim->CastSpell(&p,228519,false);slam.Advance(2000);
    Require(slam.targets[228519]==std::set<ObjectGuid>{p.GetGUID(),closePlayer.GetGUID()}&&p.HasAura(228519)&&closePlayer.HasAura(228519)&&!distantPlayer.HasAura(228519),"Native Slam missed tank-centered radius/debuff or hit distant Player");
    Require(slam.damaged[228519]==slam.targets[228519],"Native Slam selected Players without damaging them");
    Require(p.GetAura(228519)->GetEffect(EFFECT_2)->GetAmount()==400,"Native Slam lost400-percent vulnerability");
    for(auto player:{&p,&closePlayer}){player->GetAura(228519)->SetDuration(1);player->Unit::Update(30);player->Unit::Update(30);Require(!player->HasAura(228519),"Slam vulnerability did not expire");}
    auto pos=grim->GetPosition();map.PlayerRelocation(&p,pos.GetPositionX()+10,pos.GetPositionY(),pos.GetPositionZ(),0);map.PlayerRelocation(&closePlayer,pos.GetPositionX()+14,pos.GetPositionY(),pos.GetPositionZ(),0);map.PlayerRelocation(&distantPlayer,pos.GetPositionX()+25,pos.GetPositionY(),pos.GetPositionZ(),0);
    GrimWatch nova(map,*grim);grim->CastSpell(grim,228390,false);nova.Advance(3500);Require(nova.targets[228390]==std::set<ObjectGuid>{p.GetGUID(),closePlayer.GetGUID()}&&nova.damaged[228390]==nova.targets[228390],"Native fifteen-yard Nova missed nearby or hit distant Player");
    grim->CastSpell(grim,228390,false);Require(grim->HasUnitState(UNIT_STATE_CASTING),"Nova interrupt probe did not start an actual cast");GrimWatch interrupted(map,*grim);interrupted.Advance(500);grim->InterruptNonMeleeSpells(true);interrupted.Advance(3500);Require(interrupted.damaged[228390].empty(),"Interrupted Nova still dispatched damage");
    std::cout<<"GRIM_GEOMETRY slamTargets="<<slam.targets[228519].size()<<" novaTargets="<<nova.targets[228390].size()<<" interruptedTargets="<<interrupted.targets[228390].size()<<std::endl;
}
static void GrimelordCase(GarmPlayer& p,InstanceMap& map,Creature& helya,Group& group)
{
    p.GetOrInitPetStable();GarmPlayer closePlayer(map,p.getRace()),distantPlayer(map,p.getRace());
    if(p.getRace()==RACE_HUMAN)for(auto id:{193367u,228397u,228519u,228390u,228838u,232408u,158534u}){
        auto s=sSpellMgr->GetSpellInfo(id);Require(s,"Grimelord native spell missing");
        std::cout<<"GRIM_CATALOG diff="<<uint32(map.GetDifficultyID())<<" spell="<<id<<" duration="<<s->GetDuration(map.GetDifficultyID())<<" stacks="<<s->GetAuraOptions(map.GetDifficultyID())->CumulativeAura<<" cap="<<unsigned(s->TargetRestrictions.MaxAffectedTargets)<<" cast="<<s->CalcCastTime(&helya)<<std::endl;
        for(unsigned i=0;i<4;++i){auto e=s->GetEffect(i,map.GetDifficultyID());if(e->Effect)std::cout<<" EFFECT index="<<i<<" type="<<e->Effect<<" aura="<<e->ApplyAuraName<<" target="<<e->TargetA.GetTarget()<<','<<e->TargetB.GetTarget()<<" radius="<<e->CalcRadius(&helya)<<" base="<<e->BasePoints<<" period="<<e->ApplyAuraPeriod<<std::endl;}
    }
    {
        LootPet pet(p,map);pet.setFaction(p.getFaction());pet.SetMaxHealth(p.GetMaxHealth());pet.SetHealth(p.GetMaxHealth());
        auto grim=GrimBegin(p,map,helya);closePlayer.SetGameMaster(true);distantPlayer.SetGameMaster(true);
        Require(map.GetCreature(grim->GetGUID())==grim,"Native Grimelord lost before threat setup");
        grim->getThreatManager().clearReferences();grim->AddThreat(&p,100000);grim->AddThreat(&pet,1);
        Require(grim->getThreatManager().getThreatList().size()==2,"Native Grimelord threat inputs missing");GrimWatch watch(map,*grim);
        for(unsigned t=0;t<11000&&!p.HasAura(193367)&&!pet.HasAura(193367);t+=10)watch.Advance(10,true);
        std::cout<<"GRIM_FIRST time="<<watch.elapsed<<" player="<<p.HasAura(193367)<<" pet="<<pet.HasAura(193367)<<" stacks="<<(p.GetAura(193367)?p.GetAura(193367)->GetStackAmount():0)<<std::endl;
#ifdef TRIAL_HELYA_GRIMELORD_BASELINE
        Require(p.HasAura(193367)&&pet.HasAura(193367)&&p.GetAura(193367)->GetStackAmount()==1,"Pinned raw-threat/single-stack Rot changed");
#else
        Require(p.HasAura(193367)&&!pet.HasAura(193367)&&p.GetAura(193367)->GetStackAmount()==3&&!closePlayer.HasAura(193367)&&!distantPlayer.HasAura(193367),"Native selector lost Player admission/three initial stacks");
        Require(watch.elapsed>=6980&&watch.elapsed<=7040&&watch.casts[228838].size()==1,"Native initial Rot timer/selector not executed");
#endif
        pet.RemoveAura(193367);p.RemoveAura(193367);ClearCombat(p,helya);PumpHelya(map,2000);
    }
    auto grim=GrimBegin(p,map,helya);auto grimGuid=grim->GetGUID();
    map.PlayerRelocation(&p,500,640,7.5f,0);map.PlayerRelocation(&distantPlayer,510,640,7.5f,0);
    LootPet pet(p,map);pet.setFaction(p.getFaction());pet.Relocate(502,640,7.5f,0);
    auto ally=helya.SummonCreature(WORLD_TRIGGER,503,640,7.5f,0,TEMPSUMMON_MANUAL_DESPAWN);Require(ally,"Native Rot ally probe missing");ally->setFaction(p.getFaction());
    Require(grim->AddAura(193367,&p,nullptr,3),"Native three-stack aura probe failed");GrimWatch rot(map,*grim);
    float perStack=sSpellMgr->GetSpellInfo(193367)->GetEffect(EFFECT_1,map.GetDifficultyID())->BasePoints;
    for(unsigned step=1;step<=3;++step){
        map.PlayerRelocation(&closePlayer,504,640,7.5f,0);closePlayer.RemoveAura(193367);rot.Advance(3100);
        auto aura=p.GetAura(193367);std::cout<<"ROT_PERIOD step="<<step<<" stacks="<<(aura?aura->GetStackAmount():0)<<" nearby="<<closePlayer.HasAura(193367)<<" tickCasts="<<rot.casts[232408].size()<<" spreadCasts="<<rot.casts[228397].size()<<std::endl;
#ifndef TRIAL_HELYA_GRIMELORD_BASELINE
        Require((step==3?!aura:(aura&&aura->GetStackAmount()==3-step)),"Native Rot failed three-to-two-to-one-to-zero decay");
        if(aura)Require(aura->GetEffect(EFFECT_1)->GetAmount()==perStack*(3-step),"Native healing penalty did not fall with stacks");
        bool spreads=map.IsMythicRaid()||step==3;Require(closePlayer.HasAura(193367)==spreads,"Fetid spread crossed Normal/Mythic period gate");
        if(spreads)Require(closePlayer.GetAura(193367)->GetCasterGUID()==grimGuid&&closePlayer.GetAura(193367)->GetStackAmount()==3,"Secondary Rot lost original caster/initial stacks");
        Require(!distantPlayer.HasAura(193367)&&!pet.HasAura(193367)&&!ally->HasAura(193367),"Rot spread exceeded five yards or infected pet/NPC");
#endif
        closePlayer.RemoveAura(193367);map.PlayerRelocation(&closePlayer,520,640,7.5f,0);
    }
#ifdef TRIAL_HELYA_GRIMELORD_BASELINE
    Require(rot.casts[232408].empty()&&!rot.casts[228397].empty(),"Pinned missing carrier damage/legacy periodic spread not reproduced");
#else
    Require(rot.casts[232408].size()==3&&rot.casts[228397].size()==(map.IsMythicRaid()?3:1),"Native Rot lost/duplicated periodic damage/spread");
    Require(rot.targets[232408]==std::set<ObjectGuid>{p.GetGUID()}&&rot.targets[228397]==std::set<ObjectGuid>{closePlayer.GetGUID()},"Native Rot effects hit wrong actual targets");
    Require(rot.originals[232408]==std::set<ObjectGuid>{grimGuid}&&rot.originals[228397]==rot.originals[232408],"Native Rot damage lost original caster");
    auto values=rot.values[232408];Require(values.size()==3&&std::fabs(values[0]-3*values[2])<1&&std::fabs(values[1]-2*values[2])<1,"Native carrier damage did not scale3/2/1 with runtime base points");
#endif
    p.RemoveAura(193367);
    for(auto mode:{AURA_REMOVE_BY_DEFAULT,AURA_REMOVE_BY_CANCEL,AURA_REMOVE_BY_DEATH}){grim->AddAura(193367,&p,nullptr,3);GrimWatch removed(map,*grim);p.GetAura(193367)->Remove(mode);removed.Advance(3200);Require(removed.casts[232408].empty()&&removed.casts[228397].empty(),"Script/cancel/death cleanup created Rot effects");}
    p.RemoveAura(193367);Require(p.AddAura(193367,&p,nullptr,3),"Legacy self-owned Rot reset probe failed");ClearCombat(p,helya);
#ifdef TRIAL_HELYA_GRIMELORD_BASELINE
    std::cout<<"REFERENCE_ROT_RESET active="<<p.HasAura(193367)<<std::endl;p.RemoveAura(193367);
#else
    Require(!p.HasAura(193367),"Native Helya reset left Rot");
#endif
    PumpHelya(map,2000);GrimAbilities(p,closePlayer,distantPlayer,pet,map,helya);grim=GrimBegin(p,map,helya);grim->AddAura(193367,&p,nullptr,3);
    for(auto tentacle:Actors(map,114900))p.Kill(tentacle,false);PumpHelya(map,7000);Require(helya.AI()->GetData(2)==3&&grim->IsAlive()&&p.HasAura(193367),"Phase three lost existing Grimelord/Rot");
    p.RemoveAura(193367);Require(p.AddAura(193367,&p,nullptr,3),"Legacy self-owned Rot victory probe failed");
    helya.ResetPlayerDamageReq();helya.LowerPlayerDamageReq(1);helya.SetLootRecipient(&p);for(unsigned i=0;i<16&&helya.IsAlive();++i)p.DealDamage(&helya,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
    PumpHelya(map,100);Tick(p,2000);
    Require(helya.isDead(false)&&p.GetQuestObjectiveData(44868,55482)==1,"Native Grimelord victory lost Helya credit");Checkpoint(map,group,7);
#ifdef TRIAL_HELYA_GRIMELORD_BASELINE
    std::cout<<"REFERENCE_ROT_VICTORY active="<<p.HasAura(193367)<<std::endl;p.RemoveAura(193367);
#else
    Require(!p.HasAura(193367),"Native victory left Rot");
#endif
    std::cout<<"PASS: native Grimelord Rot selection/stacks/damage/spread/cleanup/phase-three/victory; diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(p.getRace())<<std::endl;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{return RunGarm(argc,argv,GrimelordCase);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
