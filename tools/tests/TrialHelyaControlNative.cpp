#define TRIAL_HELYA_PHASE3_SUPPORT_ONLY
#include "TrialHelyaPhase3Native.cpp"
struct ControlWatch
{
    InstanceMap& map;GarmPlayer& player;Creature& boss;uint32 elapsed=0;
    std::map<uint32,std::set<ObjectGuid>> casts,targets,damaged;
    ControlWatch(InstanceMap& m,GarmPlayer& p,Creature& h):map(m),player(p),boss(h){}
    void Observe(){for(Unit* caster:{static_cast<Unit*>(&player),static_cast<Unit*>(&boss)})for(auto id:{228854u,231863u,232350u})for(auto s:NativeSpellEvents(*caster,id)){
        casts[id].insert(s->m_castGuid[0]);for(auto const& t:*s->GetUniqueTargetInfo())if(t->effectMask&1){targets[id].insert(t->targetGUID);if(t->processed&&t->damage>0)damaged[id].insert(t->targetGUID);}
    }}
    // Freeze boss scheduling; tick the actual Player aura/spell dispatchers.
    // Do not buffer this Player's health: threshold tests depend on it.
    void Advance(uint32 ms){Observe();for(unsigned t=0;t<ms;t+=10){player.Unit::Update(10);Observe();elapsed+=10;}}
};
static void ControlBegin(GarmPlayer& p,GarmPlayer& subject,InstanceMap& map,Creature& helya)
{
    subject.RemoveAura(232350);subject.RemoveAura(232450);Restart(p,map,helya);EnterPhaseThree(p,map,helya);
    map.PlayerRelocation(&subject,helya.GetPositionX()+8,helya.GetPositionY(),helya.GetPositionZ(),0);subject.SetHealth(subject.GetMaxHealth());
    helya.CastSpell(&subject,232450,true);Require(subject.HasAura(232450),"Native Axion absorb missing");subject.GetAura(232450)->SetDuration(1);
    Tick(subject,30);Require(subject.HasAura(232350)==map.IsMythicRaid(),"Actual absorb expiry crossed the Mythic gate");
}
static void ControlCase(GarmPlayer& p,InstanceMap& map,Creature& helya,Group& group)
{
    LoadPhaseThreeConditions();sSpellMgr->LoadSpellPrcoCheck();GarmPlayer subject(map,p.getRace());auto faction=subject.getFaction();
    p.GetOrInitPetStable();LootPet pet(p,map);pet.setFaction(p.getFaction());pet.SetMaxHealth(p.GetMaxHealth());pet.SetHealth(p.GetMaxHealth());
    ControlBegin(p,subject,map,helya);
    if(!map.IsMythicRaid()){Require(subject.GetCharmerGUID().IsEmpty()&&!subject.HasAura(232358),"Non-Mythic absorb charmed/transformed Player");ClearCombat(p,helya);std::cout<<"PASS: native Corrupted difficulty gate diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(p.getRace())<<std::endl;return;}
    Require(subject.GetCharmerGUID()==helya.GetGUID()&&subject.HasAura(232358)&&subject.HasAura(231862)&&subject.getFaction()==helya.getFaction(),"Native Corrupted charm/transform/eruption/faction missing");
    std::cout<<"CONTROL_DATA threshold="<<subject.GetAuraEffect(232350,EFFECT_0)->GetAmount()<<" duration="<<subject.GetAura(232350)->GetMaxDuration()<<std::endl;
    Require(!subject.IsControlledByPlayer(),"Native NPC charm did not exercise the old loot-admission branch");
#ifdef TRIAL_HELYA_CONTROL_BASELINE
    // Original run without this isolation crashes in actual Taint damage at
    // Creature::hasLootRecipient. Keep that trace; isolate only to inspect the
    // independent aura bugs using the same pinned original libraries.
    subject.RemoveAura(228054);
#else
    for(Unit* attacker:{static_cast<Unit*>(&helya),static_cast<Unit*>(&p),static_cast<Unit*>(&pet)}){
        auto before=subject.GetHealth();attacker->DealDamage(&subject,100,nullptr,SPELL_DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
        Require(subject.IsAlive()&&subject.GetHealth()<before&&subject.HasAura(232350),"Actual NPC/Player damage to charmed Player failed");
    }
#endif
    helya.SetPower(POWER_ALTERNATE,10);ControlWatch casting(map,subject,helya);casting.Advance(8500);
    std::cout<<"CONTROL_CAST infusion="<<casting.casts[228854].size()<<" burst="<<casting.casts[231863].size()<<" energy="<<helya.GetPower(POWER_ALTERNATE)<<" casting="<<subject.HasUnitState(UNIT_STATE_CASTING)<<std::endl;
    Require(casting.casts[231863].size()==1&&helya.GetPower(POWER_ALTERNATE)==40,"Controlled Player did not complete one native Infusion");
    for(unsigned pct:{25u,24u,10u}){
        ControlBegin(p,subject,map,helya);subject.RemoveAura(228054);subject.SetHealth(subject.CountPctFromMaxHealth(pct));helya.SetPower(POWER_ALTERNATE,10);ControlWatch threshold(map,subject,helya);threshold.Advance(1500);
        std::cout<<"CONTROL_THRESHOLD pct="<<pct<<" aura="<<subject.HasAura(232350)<<" transform="<<subject.HasAura(232358)<<" eruption="<<subject.HasAura(231862)<<" charmer="<<subject.GetCharmerGUID().GetCounter()<<" casting="<<subject.HasUnitState(UNIT_STATE_CASTING)<<std::endl;
        threshold.Advance(8000);std::cout<<"CONTROL_AFTER pct="<<pct<<" infusions="<<threshold.casts[228854].size()<<" bursts="<<threshold.casts[231863].size()<<" energy="<<helya.GetPower(POWER_ALTERNATE)<<std::endl;
        subject.RemoveAura(232350);Require(subject.getFaction()==faction&&subject.GetCharmerGUID().IsEmpty(),"Native removal did not restore Player faction/charmer");
    }
    ClearCombat(p,helya);std::cout<<"PASS: native Corrupted observation diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(p.getRace())<<std::endl;
}
int main(int argc,char** argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);try{return RunGarm(argc,argv,ControlCase);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
