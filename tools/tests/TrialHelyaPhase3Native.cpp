// Native phase-three abilities/edge destinations after the earned raid transition.
#define TRIAL_HELYA_SUPPORT_ONLY
#include "TrialHelyaNative.cpp"
#include "Spell.h"
struct PhaseThreeConditionTag{using Type=bool(ConditionMgr::*)(Condition*);friend Type Access(PhaseThreeConditionTag);};
template struct FixtureAccess<PhaseThreeConditionTag,&ConditionMgr::addToSpellImplicitTargetConditions>;
static void LoadPhaseThreeConditions()
{
    static bool loaded=false;if(loaded)return;loaded=true;
    auto rows=WorldDatabase.Query("SELECT SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry IN(227903,228056,228057,228061,228062,228063,228121,228565,228566,232418,232449,232450,232452)");
    Require(bool(rows),"Native phase-three spell conditions missing");unsigned count=0;
    do{auto f=rows->Fetch();auto c=new Condition();c->SourceType=ConditionSourceType(f[0].GetInt32());c->SourceGroup=f[1].GetUInt32();c->SourceEntry=f[2].GetUInt32();c->SourceId=f[3].GetInt32();c->ElseGroup=f[4].GetUInt32();c->ConditionType=ConditionTypes(f[5].GetInt32());c->ConditionTarget=f[6].GetUInt8();c->ConditionValue1=f[7].GetUInt32();c->ConditionValue2=f[8].GetUInt32();c->ConditionValue3=f[9].GetUInt32();c->NegativeCondition=f[10].GetBool();c->ErrorTextId=f[11].GetUInt32();Require(sConditionMgr->isConditionTypeValid(c)&&(sConditionMgr->*Access(ValidateSourceTag{}))(c)&&(sConditionMgr->*Access(PhaseThreeConditionTag{}))(c),"Native phase-three condition admission failed");++count;}while(rows->NextRow());Require(count==1,"Inspect newly added phase-three conditions");
}
static uint32 LethalInput(GarmPlayer& p,Creature& helya)
{return uint32(std::ceil(1.5*helya.GetHealth(&p)*helya.GetHealthMultiplierForTarget(&p)));}
static void EnterPhaseThree(GarmPlayer& p,InstanceMap& map,Creature& helya)
{
    p.DealDamage(&helya,LethalInput(p,helya),nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
    Require(helya.AI()->GetData(2)==2,"Phase-three bootstrap missed real phase two");
    for(auto tentacle:Actors(map,114900))p.Kill(tentacle,false);
    PumpHelya(map,7000);Require(helya.AI()->GetData(2)==3,"Native phase-three entry failed");
}
static std::set<ObjectGuid> OrbBurst(GarmPlayer& p,InstanceMap& map,Creature& helya,uint32 spell,uint32 entry,uint32 marker)
{
    Require(Actors(map,entry).empty(),"Orb burst must start without previous actors");
    helya.CastSpell(&helya,spell,true);PumpHelya(map,3000);auto orbs=Actors(map,entry);
    unsigned expected=map.GetDifficultyID()==DIFFICULTY_LFR_RAID||map.GetDifficultyID()==DIFFICULTY_NORMAL_RAID?2:3;
    Require(orbs.size()==expected,"Native isolated orb cast lost difficulty count");
    std::set<ObjectGuid> targets;for(auto orb:orbs){targets.insert(orb->GetTargetGUID());auto target=ObjectAccessor::GetPlayer(*orb,orb->GetTargetGUID());Require(target&&target->HasAura(marker),"Native orb target/marker missing");
#ifndef TRIAL_HELYA_PHASE3_BASELINE
        Require(target->HasAura(marker,orb->GetGUID()),"Distinct orb did not retain its own native target marker");
#endif
    }
    std::cout<<"BURST entry="<<entry<<" actors="<<orbs.size()<<" distinctTargets="<<targets.size()<<std::endl;
#ifdef TRIAL_HELYA_PHASE3_BASELINE
    Require(targets.size()==1,"Pinned shared delayed target changed");
#else
    Require(targets.size()==expected,"Different native orbs follow the same final loop target");
#endif
    auto first=orbs.front();auto start=first->GetPosition();auto target=ObjectAccessor::GetPlayer(*first,first->GetTargetGUID());map.PlayerRelocation(target,target->GetPositionX()+8,target->GetPositionY(),target->GetPositionZ(),target->GetOrientation());PumpHelya(map,3000);
    Require(first->IsInWorld()&&first->IsAlive(),"Native orb expired before chase check");std::cout<<"CHASE entry="<<entry<<" moved="<<first->GetExactDist(&start)<<" distance="<<first->GetDistance(target)<<std::endl;
    Require(first->GetExactDist(&start)>0.05f,"Native orb did not move towards its moving target");
    std::set<ObjectGuid> trail;
    if(entry==114566)for(auto at:HelyaAreas(map))if(at->GetSpellId()==228121){
#ifndef TRIAL_HELYA_PHASE3_BASELINE
        Require(at->GetCasterGUID()==helya.GetGUID(),"Duplicate orb-owned Decay escaped Helya's Fury/cleanup ownership");
#endif
        if(at->GetCasterGUID()==helya.GetGUID())trail.insert(at->GetGUID());
    }
    return trail;
}
static void Restart(GarmPlayer& p,InstanceMap& map,Creature& helya)
{
    ClearCombat(p,helya);PumpHelya(map,12000);
#ifdef TRIAL_HELYA_PHASE3_BASELINE
    // Old pending orbs can re-enter combat while the first reset drains.
    // Use another actual evade, never patch the phase/threat/EVADE flags.
    if(helya.AI()->GetData(2)!=0||helya.isInCombat()){ClearCombat(p,helya);PumpHelya(map,12000);}
#endif
    map.PlayerRelocation(&p,524,680,5,0);helya.SetInCombatWith(&p);Require(helya.AI()->GetData(2)==1,"Native Helya retry did not enter phase one");
}
struct HelyaEventListTag{using Type=EventList EventProcessor::*;friend Type Access(HelyaEventListTag);};
template struct FixtureAccess<HelyaEventListTag,&EventProcessor::m_events>;
struct HelyaEventQueueTag{using Type=EventList EventProcessor::*;friend Type Access(HelyaEventQueueTag);};
template struct FixtureAccess<HelyaEventQueueTag,&EventProcessor::m_events_queue>;
struct HelyaEventSpellTag{using Type=Spell* SpellEvent::*;friend Type Access(HelyaEventSpellTag);};
template struct FixtureAccess<HelyaEventSpellTag,&SpellEvent::m_Spell>;
static std::vector<Spell*> NativeSpellEvents(Creature& caster,uint32 id)
{
    std::vector<Spell*> result;
    for(auto member:{Access(HelyaEventListTag{}),Access(HelyaEventQueueTag{})})for(auto const& pair:caster.m_Events.*member)if(auto event=dynamic_cast<SpellEvent*>(pair.second)){auto spell=event->*Access(HelyaEventSpellTag{});if(spell->GetSpellInfo()->Id==id)result.push_back(spell);}
    return result;
}
static void CheckBreathGeometry(InstanceMap& map,Creature& helya)
{
    auto diff=map.GetDifficultyID();unsigned expected=diff==DIFFICULTY_MYTHIC_RAID?5:diff==DIFFICULTY_HEROIC_RAID?3:0;
    for(float angle:{0.0f,float(M_PI/4),float(M_PI/2),float(M_PI),helya.GetHomePosition().GetOrientation(),float(3*M_PI/2),5.5f,float(2*M_PI-0.01)})
    {
        helya.InterruptNonMeleeSpells(true);helya.SetFacingTo(angle);
        auto old=NativeSpellEvents(helya,232418);helya.CastSpell(&helya,228565,true);
        std::vector<Position> destinations;
        for(auto spell:NativeSpellEvents(helya,232418))if(std::find(old.begin(),old.end(),spell)==old.end()){
            Require(spell->m_targets.HasDst(),"Geometry probe lost actual missile destination");destinations.push_back(*spell->m_targets.GetDstPos());
        }
        Require(destinations.size()==expected,"Breath geometry changed difficulty-specific missile count");
        std::sort(destinations.begin(),destinations.end(),[&](Position const& a,Position const& b){return helya.GetExactDist2d(&a)<helya.GetExactDist2d(&b);});
        unsigned collapsed=0,elevated=0;std::set<ObjectGuid> impacts;
        for(unsigned i=0;i<destinations.size();++i){auto const& pos=destinations[i];float dist=43.0f+13.0f*i;
            float x=helya.GetPositionX()+dist*std::cos(angle),y=helya.GetPositionY()+dist*std::sin(angle);
            // Query real extracted terrain + VMap, without the walking helper's
            // six-yard slope restriction. The test never writes a destination.
            float surface=map.GetHeight(helya.GetPhases(),x,y,MAX_HEIGHT,true,MAX_FALL_DISTANCE);
            Require(std::isfinite(surface)&&surface>INVALID_HEIGHT,"Geometry fixture lacks a native supporting surface");
            if(helya.GetExactDist2d(&pos)<1.0f)++collapsed;if(surface>helya.GetPositionZ()+6)++elevated;
#ifndef TRIAL_HELYA_PHASE3_BASELINE
            Require(std::fabs(pos.GetPositionX()-x)<0.01f&&std::fabs(pos.GetPositionY()-y)<0.01f,"Axion destination slid backwards or returned to Helya");
            Require(std::fabs(pos.GetPositionZ()-surface)<0.01f,"Axion destination missed the actual top supporting surface");
#endif
            std::cout<<"GEOMETRY_DEST angle="<<angle<<" distance="<<dist<<" actual="<<pos.GetPositionX()<<','<<pos.GetPositionY()<<','<<pos.GetPositionZ()<<" surface="<<surface<<std::endl;
        }
        for(unsigned t=0;t<8000;t+=10){for(auto spell:NativeSpellEvents(helya,232449))impacts.insert(spell->m_castGuid[0]);PumpHelya(map,10);}
        Require(impacts.size()==expected,"Native missile did not execute its ground impact at the edge destination");
#ifdef TRIAL_HELYA_PHASE3_BASELINE
        if(angle==0&&expected)Require(collapsed>=2,"Pinned off-platform destination collapse no longer reproduced");
#else
        Require(collapsed==0,"Native edge missiles still land on Helya");
#endif
        std::cout<<"GEOMETRY angle="<<angle<<" bolts="<<destinations.size()<<" collapsed="<<collapsed<<" elevated="<<elevated<<" impacts="<<impacts.size()<<std::endl;
        for(auto const& ref:map.GetPlayers())if(auto aura=ref.getSource()->GetAura(232450))aura->Remove(AURA_REMOVE_BY_ENEMY_SPELL);
    }
    helya.InterruptNonMeleeSpells(true);helya.SetFacingTo(helya.GetHomePosition().GetOrientation());
}
static void CheckBreath(GarmPlayer& p,std::vector<std::unique_ptr<GarmPlayer>>& others,InstanceMap& map,Creature& helya)
{
    auto& other=*others.front();auto& soaker=*others[1];
    auto diff=map.GetDifficultyID();
    auto breath=sSpellMgr->GetSpellInfo(228565)->GetEffect(EFFECT_1,diff);auto damage=sSpellMgr->GetSpellInfo(228566)->GetEffect(EFFECT_0,diff);auto missile=sSpellMgr->GetSpellInfo(232418)->GetEffect(EFFECT_0,diff);auto absorb=sSpellMgr->GetSpellInfo(232450)->GetEffect(EFFECT_1,diff);
    Require(breath->Effect==SPELL_EFFECT_TRIGGER_SPELL&&breath->TriggerSpell==228566&&damage->Effect==SPELL_EFFECT_SCHOOL_DAMAGE&&damage->TargetA.GetTarget()==104&&missile->TriggerSpell==232449&&absorb->ApplyAuraName==SPELL_AURA_SCHOOL_HEAL_ABSORB,"Native breath/axion DB2 contract changed");
    p.RemoveAura(232488);other.RemoveAura(232488);helya.InterruptNonMeleeSpells(true);helya.SetFacingTo(helya.GetHomePosition().GetOrientation());
    float dx=10*std::cos(helya.GetOrientation()),dy=10*std::sin(helya.GetOrientation());
    map.PlayerRelocation(&p,helya.GetPositionX()+dx,helya.GetPositionY()+dy,helya.GetPositionZ(),0);map.PlayerRelocation(&other,helya.GetPositionX()-dx,helya.GetPositionY()-dy,helya.GetPositionZ(),0);
    auto soak=helya.GetNearPosition(43.0f,0.0f);map.PlayerRelocation(&soaker,soak.GetPositionX(),soak.GetPositionY(),soak.GetPositionZ(),0);map.PlayerRelocation(others[2].get(),helya.GetPositionX()-2*dx,helya.GetPositionY()-2*dy,helya.GetPositionZ(),0);
    for(auto const& ref:map.GetPlayers())if(auto aura=ref.getSource()->GetAura(232450))aura->Remove(AURA_REMOVE_BY_ENEMY_SPELL);
    std::map<uint32,std::set<ObjectGuid>> impacts;
    auto observe=[&](){for(auto id:{232449u,232452u,232450u})for(auto s:NativeSpellEvents(helya,id))impacts[id].insert(s->m_castGuid[0]);};
    auto advance=[&](uint32 ms){observe();for(unsigned t=0;t<ms;t+=10){PumpHelya(map,10);observe();}};
    auto old=NativeSpellEvents(helya,232418);helya.CastSpell(&helya,228565,true);auto events=NativeSpellEvents(helya,232418);unsigned bolts=0;for(auto s:events)if(std::find(old.begin(),old.end(),s)==old.end()){++bolts;Require(s->m_targets.HasDst(),"Axion missile has no native destination");std::cout<<"AXION x="<<s->m_targets.GetDstPos()->GetPositionX()<<" y="<<s->m_targets.GetDstPos()->GetPositionY()<<std::endl;}
    advance(500);std::cout<<"BREATH front="<<p.HasAura(232488)<<" behind="<<other.HasAura(232488)<<" bolts="<<bolts<<" aiTarget="<<UnitAI::AISpellInfo[228565].target<<std::endl;
    Require(p.HasAura(232488)&&!other.HasAura(232488),"Native Corrupted Breath hit behind or missed front player");
    Require(bolts==(diff==DIFFICULTY_MYTHIC_RAID?5:diff==DIFFICULTY_HEROIC_RAID?3:0),"Native breath lost difficulty-specific axion count");
    advance(8000);helya.InterruptNonMeleeSpells(true);std::cout<<"IMPACTS axions="<<impacts[232449].size()<<" fallback="<<impacts[232452].size()<<" absorbCasts="<<impacts[232450].size()<<" soaked="<<soaker.HasAura(232450)<<std::endl;
    Require(impacts[232449].size()==bolts&&impacts[232452].size()==(bolts?bolts-1:0)&&soaker.HasAura(232450)==bool(bolts),"Native axion missile/soak/missed raid effect chain failed");
    for(auto mode:{AURA_REMOVE_BY_ENEMY_SPELL,AURA_REMOVE_BY_DEFAULT,AURA_REMOVE_BY_EXPIRE})
    {
        other.RemoveAura(232350);helya.CastSpell(&other,232450,true);Require(other.HasAura(232450),"Native Corrupted Axion heal absorb did not apply");
        if(mode==AURA_REMOVE_BY_ENEMY_SPELL)p.HealBySpell(&other,sSpellMgr->GetSpellInfo(2061),1000000000);
        else if(mode==AURA_REMOVE_BY_EXPIRE){other.GetAura(232450)->SetDuration(1);Tick(other,30);}
        else other.GetAura(232450)->Remove(mode);
        PumpHelya(map,100);Require(!other.HasAura(232450),"Native healing/expiry/removal did not clear Axion absorb");std::cout<<"ABSORB mode="<<uint32(mode)<<" corrupted="<<other.HasAura(232350)<<" extra="<<other.HasAura(232358)<<" charmer="<<other.GetCharmerGUID().GetCounter()<<std::endl;
#ifdef TRIAL_HELYA_PHASE3_BASELINE
        bool expected=diff==DIFFICULTY_MYTHIC_RAID&&mode!=AURA_REMOVE_BY_ENEMY_SPELL;
#else
        bool expected=diff==DIFFICULTY_MYTHIC_RAID&&mode==AURA_REMOVE_BY_EXPIRE;
#endif
        Require(other.HasAura(232350)==expected,"Axion mind control crossed heal/script-removal/expiry or difficulty gate");
        Require(other.HasAura(232358)==expected&&other.GetCharmerGUID()==(expected?helya.GetGUID():ObjectGuid::Empty),"Actual Axion transform/charm state disagrees with control aura");
    }
    other.RemoveAura(232350);Require(!other.HasAura(232358)&&other.GetCharmerGUID().IsEmpty(),"Removing Corrupted left native transform or charmer");helya.CastSpell(&other,232450,true);
}
static void CheckPhaseThree(GarmPlayer& p,InstanceMap& map,Creature& helya,Group& group)
{
    p.GetOrInitPetStable();LoadPhaseThreeConditions();
    // Additional native players occupy distinct positions to expose shared
    // delayed-target state. Admission/positioning is fixture setup.
    std::vector<std::unique_ptr<GarmPlayer>> others;
    for(unsigned i=0;i<3;++i){others.emplace_back(new GarmPlayer(map,p.getRace()));map.PlayerRelocation(others.back().get(),500.0f+float(i)*8,660.0f,3.0f,0);}
    map.PlayerRelocation(&p,524,680,5,0);
    OrbBurst(p,map,helya,227903,114523,229119);Restart(p,map,helya);EnterPhaseThree(p,map,helya);auto trail=OrbBurst(p,map,helya,228056,114566,230267);Require(!trail.empty(),"Native corrosion produced no boss-owned Decay");PumpHelya(map,7000);for(auto guid:trail)Require(map.GetAreaTrigger(guid),"Decay vanished with its short-lived orb before Fury");
    std::set<ObjectGuid> orbs,areas;std::map<uint32,unsigned> spells;unsigned breaths=0;bool hatred=false;auto previous=helya.GetPower(POWER_ENERGY);
    int firstWave=-1;bool washed=false;
    for(unsigned sec=0;sec<100;++sec)
    {
        PumpHelya(map,1000);
        for(auto c:Actors(map,114566))orbs.insert(c->GetGUID());
        for(auto at:HelyaAreas(map))if(areas.insert(at->GetGUID()).second){++spells[at->GetSpellId()];if(at->GetSpellId()==228027&&firstWave<0)firstWave=int(sec);}
        if(firstWave>=0&&int(sec)>=firstWave+5){washed=true;for(auto guid:trail)if(map.GetAreaTrigger(guid))washed=false;}
        auto energy=helya.GetPower(POWER_ENERGY);if(energy<previous)++breaths;previous=energy;hatred|=p.HasAura(232488);
    }
    Require(p.IsAlive()&&helya.AI()->GetData(2)==3,"Phase-three ability probe lost encounter");
    Require(breaths>=2&&hatred&&spells[228027]>=1&&spells[228121]>0&&orbs.size()>=4&&washed,"Native breath/debuff/Fury/Decay/orb repetition or wave cleaning missing");
    std::cout<<"ABILITIES breaths="<<breaths<<" orbs="<<orbs.size()<<" decay="<<spells[228121]<<" waves="<<spells[228027]<<std::endl;
    // Freeze further automatic energy gains and drain existing missiles before
    // isolated native breath requests, without changing phase or firing AI hooks.
    helya.RemoveAura(232230);PumpHelya(map,8000);CheckBreathGeometry(map,helya);CheckBreath(p,others,map,helya);
    // Interrupt an actually cast orb request before its two-second summon.
    helya.InterruptNonMeleeSpells(true);helya.CastSpell(&helya,228056,true);PumpHelya(map,500);ClearCombat(p,helya);PumpHelya(map,3000);
    std::cout<<"DEBUFF_RESET hatred="<<p.HasAura(232488)<<" absorb="<<others.front()->HasAura(232450)<<" corrupted="<<others.front()->HasAura(232350)<<std::endl;
#ifdef TRIAL_HELYA_PHASE3_BASELINE
    Require(p.HasAura(232488),"Pinned stale Dark Hatred reset behavior changed");
    for(auto c:Actors(map,114566))c->DespawnOrUnsummon();PumpHelya(map,100);
#else
    Require(!p.HasAura(232488)&&!others.front()->HasAura(232450)&&!others.front()->HasAura(232350)&&!others.front()->HasAura(232358)&&others.front()->GetCharmerGUID().IsEmpty(),"Native evade retained breath debuffs or mind control");
    Require(!helya.isInCombat()&&map.GetInstanceScript()->GetBossState(2)==NOT_STARTED,"Stale aura boundary requires an actually reset boss");
    auto stale=helya.AddAura(232450,others.front().get());Require(stale,"Native stale-absorb application failed");stale->SetDuration(1);Tick(*others.front(),30);PumpHelya(map,100);Require(!others.front()->HasAura(232350)&&!helya.isInCombat(),"Expired stale absorb restarted control/combat after reset");
#endif
#ifdef TRIAL_HELYA_PHASE3_BASELINE
    std::cout<<"REFERENCE_LATE_ORBS count="<<Actors(map,114566).size()<<std::endl;
#else
    Require(Actors(map,114566).empty()&&HelyaAreas(map,true).empty(),"Delayed corrosion orbs/effects remained after native evade");
#endif
    Restart(p,map,helya);EnterPhaseThree(p,map,helya);helya.CastSpell(&p,232488,true);helya.CastSpell(others.front().get(),232450,true);Require(p.HasAura(232488)&&others.front()->HasAura(232450),"Victory cleanup probe lacks real breath debuffs");
    helya.ResetPlayerDamageReq();helya.LowerPlayerDamageReq(1);helya.SetLootRecipient(&p);p.DealDamage(&helya,LethalInput(p,helya),nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Tick(p,2000);PumpHelya(map,15000);
    Require(helya.isDead(false)&&map.GetInstanceScript()->GetBossState(2)==DONE&&p.GetQuestStatus(44868)==QUEST_STATUS_COMPLETE&&p.GetQuestObjectiveData(44868,55482)==1,"Native phase-three victory lost quest credit");Checkpoint(map,group,7);
#ifdef TRIAL_HELYA_PHASE3_BASELINE
    Require(p.HasAura(232488),"Pinned stale Dark Hatred victory behavior changed");
#else
    Require(!p.HasAura(232488)&&!others.front()->HasAura(232450)&&!others.front()->HasAura(232350)&&!others.front()->HasAura(232358)&&others.front()->GetCharmerGUID().IsEmpty()&&HelyaAreas(map,true).empty(),"Victory left breath debuffs, mind control or combat area effects");
#endif
    std::cout<<"PASS: native phase-three targets/chase/Decay/breath/geometry/axion/absorb/reset/victory; diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(p.getRace())<<std::endl;
    others.clear();
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{return RunGarm(argc,argv,CheckPhaseThree);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
