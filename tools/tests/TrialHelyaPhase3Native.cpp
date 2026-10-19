// Native phase-three abilities after the earned Odyn/Guarm/Helya transition.
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
    ClearCombat(p,helya);PumpHelya(map,12000);map.PlayerRelocation(&p,524,680,5,0);helya.SetInCombatWith(&p);Require(helya.AI()->GetData(2)==1,"Native Helya retry did not enter phase one");
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
    // Interrupt an actually cast orb request before its two-second summon.
    helya.CastSpell(&helya,228056,true);PumpHelya(map,500);ClearCombat(p,helya);PumpHelya(map,3000);
#ifdef TRIAL_HELYA_PHASE3_BASELINE
    Require(!Actors(map,114566).empty(),"Pinned post-wipe delayed orb behavior changed");
#else
    Require(Actors(map,114566).empty()&&HelyaAreas(map,true).empty(),"Delayed corrosion orbs/effects remained after native evade");
#endif
    std::cout<<"PASS: native phase-three orb targets/chase/repetition and reset; diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(p.getRace())<<std::endl;
    others.clear();ClearCombat(p,helya);
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{return RunGarm(argc,argv,CheckPhaseThree);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
