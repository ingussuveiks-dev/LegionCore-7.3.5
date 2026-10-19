// Actual phase-one missile/summon/liquefaction dispatch, using the raid fixture.
#define TRIAL_HELYA_PHASE3_SUPPORT_ONLY
#include "TrialHelyaPhase3Native.cpp"
struct SlimeSpellValueTag{using Type=SpellValue* Spell::*;friend Type Access(SlimeSpellValueTag);};
template struct FixtureAccess<SlimeSpellValueTag,&Spell::m_spellValue>;
static void LoadSlimeConditions()
{
    static bool loaded=false;if(loaded)return;loaded=true;
    auto row=WorldDatabase.Query("SELECT SourceGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=227993");Require(row&&row->GetRowCount()==1,"Native slime condition missing/ambiguous");auto f=row->Fetch();
    auto c=new Condition();c->SourceType=CONDITION_SOURCE_TYPE_SPELL_IMPLICIT_TARGET;c->SourceGroup=f[0].GetUInt32();c->SourceEntry=227993;c->ConditionType=ConditionTypes(f[1].GetInt32());c->ConditionTarget=f[2].GetUInt8();c->ConditionValue1=f[3].GetUInt32();c->ConditionValue2=f[4].GetUInt32();c->NegativeCondition=f[5].GetBool();
    Require(sConditionMgr->isConditionTypeValid(c)&&(sConditionMgr->*Access(ValidateSourceTag{}))(c)&&(sConditionMgr->*Access(PhaseThreeConditionTag{}))(c),"Native slime condition admission failed");
}
static std::vector<AreaTrigger*> SlimePools(InstanceMap& map)
{auto all=HelyaAreas(map);all.erase(std::remove_if(all.begin(),all.end(),[](AreaTrigger* at){return at->GetSpellId()!=227992;}),all.end());return all;}
static void SlimeReset(GarmPlayer& p,InstanceMap& map,Creature& helya,bool retry=true)
{
    ClearCombat(p,helya);PumpHelya(map,2000);
    Require(Actors(map,114553).empty()&&SlimePools(map).empty()&&NativeSpellEvents(helya,230216).empty()&&map.GetInstanceScript()->GetBossState(2)==NOT_STARTED,"Phase-one reset left slimes/pools/delayed missiles");
    for(auto const& ref:map.GetPlayers())Require(!ref.getSource()->HasAura(227998),"Pool aura survived encounter cleanup");
    if(retry){map.PlayerRelocation(&p,helya.GetPositionX(),helya.GetPositionY()-10,helya.GetPositionZ(),0);helya.SetInCombatWith(&p);Require(helya.AI()->GetData(2)==1,"Native slime retry did not enter phase one");helya.RemoveAura(232229);}
}
static std::vector<Position> SlimeBreath(Creature& helya)
{
    helya.InterruptNonMeleeSpells(true);auto old=NativeSpellEvents(helya,230216);helya.CastSpell(&helya,227967,true);std::vector<Position> result;
    for(auto s:NativeSpellEvents(helya,230216))if(std::find(old.begin(),old.end(),s)==old.end()){Require(s->m_targets.HasDst(),"Bilewater missile lost native destination");result.push_back(*s->m_targets.GetDstPos());}
    std::sort(result.begin(),result.end(),[&](Position const& a,Position const& b){return helya.GetExactDist2d(&a)<helya.GetExactDist2d(&b);});return result;
}
static void CheckPhaseOne(GarmPlayer& p,InstanceMap& map,Creature& helya,Group& group)
{
    p.GetOrInitPetStable();LoadSlimeConditions();
    if(p.getRace()==RACE_HUMAN){for(auto id:{227967u,230229u,230216u,228984u,227990u,227993u,227992u,154241u}){auto s=sSpellMgr->GetSpellInfo(id);std::cout<<"CATALOG id="<<id<<" cast="<<s->CalcCastTime(&helya)<<" duration="<<s->GetDuration(map.GetDifficultyID())<<std::endl;for(unsigned i=0;i<8;++i){auto e=s->GetEffect(SpellEffIndex(i),map.GetDifficultyID());if(e->Effect)std::cout<<" EFFECT index="<<i<<" type="<<e->Effect<<" aura="<<e->ApplyAuraName<<" value="<<e->CalcValue()<<" trigger="<<e->TriggerSpell<<" misc="<<e->MiscValue<<" targets="<<e->TargetA.GetTarget()<<','<<e->TargetB.GetTarget()<<" radius="<<e->CalcRadius(&helya)<<std::endl;}}}
    helya.RemoveAura(232229);
    std::vector<std::unique_ptr<GarmPlayer>> others;
    for(unsigned count:{1u,10u,11u,15u,16u,20u,21u,25u,26u,30u}){
        while(others.size()+1<count){others.emplace_back(new GarmPlayer(map,p.getRace()));map.PlayerRelocation(others.back().get(),500,600,3,0);}
        Require(map.GetPlayersCountExceptGMs()==count,"Fixture raid count differs from native map refs");auto destinations=SlimeBreath(helya);unsigned expected=count<=10?2:count<=15?3:count<=20?4:count<=25?5:6;
        Require(destinations.size()==expected,"Bilewater count changed at a native raid-size boundary");std::cout<<"SLIME_COUNT players="<<count<<" bolts="<<destinations.size()<<std::endl;
    }
    while(others.size()+1>26)others.pop_back();others.back()->SetGameMaster(true);Require(SlimeBreath(helya).size()==5&&map.GetPlayersCountExceptGMs()==25,"GM affected native slime raid count");others.back()->SetGameMaster(false);
    while(others.size()+1<30){others.emplace_back(new GarmPlayer(map,p.getRace()));map.PlayerRelocation(others.back().get(),500,600,3,0);}
    SlimeReset(p,map,helya);
    for(float angle:{0.0f,float(M_PI/4),float(M_PI/2),float(M_PI),helya.GetHomePosition().GetOrientation(),float(3*M_PI/2),5.5f,float(2*M_PI-0.01)}){
        helya.SetFacingTo(angle);auto destinations=SlimeBreath(helya);Require(destinations.size()==6,"Geometry fixture lost slime missiles");unsigned shortened=0;
        for(unsigned i=0;i<destinations.size();++i){auto const& pos=destinations[i];float dist=51.0f+11.0f*i,x=helya.GetPositionX()+dist*std::cos(angle),y=helya.GetPositionY()+dist*std::sin(angle);float surface=map.GetHeight(helya.GetPhases(),x,y,MAX_HEIGHT,true,MAX_FALL_DISTANCE);Require(surface>INVALID_HEIGHT,"Native slime geometry has no supporting surface");if(std::fabs(helya.GetExactDist2d(&pos)-dist)>0.02f)++shortened;
#ifndef TRIAL_HELYA_PHASE1_BASELINE
            Require(std::fabs(pos.GetPositionX()-x)<0.02f&&std::fabs(pos.GetPositionY()-y)<0.02f&&std::fabs(pos.GetPositionZ()-surface)<0.02f,"Slime missile shortened or missed its native surface");
#endif
        }
        PumpHelya(map,8000);auto slimes=Actors(map,114553);Require(slimes.size()==6,"Actual Bilewater missile impacts lost slime summons");
        for(auto slime:slimes)Require(slime->IsAlive()&&slime->HasUnitState(UNIT_STATE_CASTING)&&slime->HasAura(154241),"Slime did not start native liquefaction/pose");
        for(auto const& pos:destinations)Require(std::count_if(slimes.begin(),slimes.end(),[&](Creature* slime){return slime->GetExactDist(&pos)<0.1f;})>=1,"Native slime did not spawn at its actual missile destination");
#ifdef TRIAL_HELYA_PHASE1_BASELINE
        if(angle==0)Require(shortened>=2,"Pinned Bilewater edge contraction not reproduced");
#else
        Require(!shortened,"Native Bilewater edge destinations still contract");
#endif
        std::cout<<"SLIME_GEOMETRY angle="<<angle<<" shortened="<<shortened<<" summoned="<<slimes.size()<<std::endl;SlimeReset(p,map,helya);
    }
    others.clear();helya.SetFacingTo(helya.GetHomePosition().GetOrientation());SlimeBreath(helya);PumpHelya(map,7000);auto slimes=Actors(map,114553);Require(slimes.size()==2,"Native two-slime lifecycle fixture missing");auto wounded=slimes[1];
    wounded->SetInCombatWith(&p);auto raw=uint32(std::ceil(0.75*wounded->GetHealth(&p)*wounded->GetHealthMultiplierForTarget(&p)));p.DealDamage(wounded,raw,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(wounded->IsAlive()&&wounded->GetHealthPct()>24&&wounded->GetHealthPct()<26,"Native slime damage did not leave25 percent");
    std::map<unsigned,float> damageByHealth;std::set<ObjectGuid> explosions;for(unsigned t=0;t<17000;t+=10){for(auto s:NativeSpellEvents(helya,227992))if(explosions.insert(s->m_castGuid[0]).second){auto value=s->*Access(SlimeSpellValueTag{});damageByHealth[unsigned(std::lround(value->EffectBasePoints[1]))]=value->EffectBasePoints[0];std::cout<<"SLIME_EXPLOSION bp0="<<value->EffectBasePoints[0]<<" hp="<<value->EffectBasePoints[1]<<std::endl;}PumpHelya(map,10);}
    Require(damageByHealth.size()==2&&damageByHealth[25]>0&&damageByHealth[25]<0.3f*damageByHealth[100]&&damageByHealth[25]>0.2f*damageByHealth[100],"Native slime damage did not reduce liquefaction to roughly25 percent (allowing native variance)");
    auto pools=SlimePools(map);Require(explosions.size()==2&&pools.size()==2,"Native liquefaction lost or duplicated explosions/pools");for(auto s:slimes)Require(s->isDead(false),"Natural liquefaction did not kill its slime");for(auto at:pools){Require(at->GetCasterGUID()==helya.GetGUID(),"Slime pool is not boss-owned");std::cout<<"SLIME_POOL radius="<<at->GetRadius()<<" duration="<<at->GetDuration()<<std::endl;}
    SlimeReset(p,map,helya);SlimeBreath(helya);PumpHelya(map,500);SlimeReset(p,map,helya,false);PumpHelya(map,20000);Require(Actors(map,114553).empty()&&SlimePools(map).empty(),"Pending Bilewater projectile survived wipe");
    std::cout<<"PASS: native phase-one counts/geometry/summons/liquefaction/reset; diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(p.getRace())<<std::endl;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{return RunGarm(argc,argv,CheckPhaseOne);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
