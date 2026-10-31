#include <cmath>
#include <limits>
#include "NativeHost.inc"

// Exact native normalization/current+suspended policy prefix and the actual
// target-relative health getter. Remaining combat/health services are controlled.
uint32 Unit::Deal(Creature* victim,uint32 damage)
{
    DamageEffectType damagetype=0;
    if(victim->IsAIEnabled&&victim->AI())victim->AI()->DamageTaken(this,damage,damagetype);
    uint64 deathHealth=victim->GetHealth(this);
#include "ScaledHook.inc"
    if(deathHealth<=damage)victim->alive=false;
    return damage;
}
struct Small:CreatureAI{
    Creature*me;bool wait_heal=false;unsigned talk2=0,casts=0;std::vector<uint32>speech;EventMap events;
    explicit Small(Creature*actor):me(actor){}
    void Talk(uint32 id){assert(id==0||id==1);speech.push_back(id);}
    void DoCast(uint32 id){assert(id==203836||id==204624||id==204593);++casts;}
#include "Wounds.inc"
#include "SmallSpeech.inc"
#include "SmallApplied.inc"
#include "SmallLifecycle.inc"
#include "SmallHeal.inc"
#include "SmallDamage.inc"
#include "SmallReset.inc"
};
template<class AI>struct Run:Test{
    AI content;AI&ai;
    explicit Run(uint32 entry):Test(),content(&actor),ai(content){actor.entry=entry;actor.i_AI=&content;}
};
void Combat(Test&t){t.scenario.step=3;t.actor.health=t.actor.maximum=1000;}
template<class T>void NoProgress(T const&t){assert(!t.player.requests&&!t.peer.requests&&t.scenario.step==3&&!t.actor.motion.moves&&!t.ai.talk2&&!t.actor.poses&&!t.actor.kits);}
template<class T>std::vector<uint32>Speech(T const&t){return t.actor.entry==101881?std::vector<uint32>{11,12}:std::vector<uint32>{0,1};}
template<class T>uint32 Lethal(T const&t){return t.actor.entry==101881?2000:1000;}
template<class T>void Hold(T&t){uint32 damage=t.player.Deal(&t.actor,Lethal(t));assert(!damage&&t.ai.wait_heal&&t.actor.health==1000&&t.actor.alive&&t.actor.auraCalls==1&&t.actor.states==1&&t.ai.speech==Speech(t));}
template<class AI>unsigned Check(uint32 entry){unsigned cases=0;
#ifdef WOUND_BASELINE
    unsigned missed=0,overblocked=0;
#endif
    for(unsigned source=0;source<4;++source)for(float scale:{0.015625f,0.25f,1.f,16.f})
    for(unsigned portion:{1u,2u,4u})for(unsigned intensity:{0u,1u,2u,4u,8u})for(unsigned mode=0;mode<6;++mode)
    {
        Run<AI>t(entry);Combat(t);t.actor.health=64000*portion/4;t.actor.maximum=64000;t.actor.scalable=true;t.actor.stat.healthMax=uint64(64000*scale);
        Unit npc;Unit*attacker=source==0?static_cast<Unit*>(&t.player):source==1?&t.pet:source==2?&t.guardian:&npc;
        if(mode==1)t.actor.i_AI=&t.temporary,t.actor.i_disabledAI=&t.ai;
        if(mode==2)t.actor.i_disabledAI=&t.ai;
        if(mode==3)t.actor.i_AI=nullptr,t.actor.i_disabledAI=&t.ai,t.actor.IsAIEnabled=false;
        if(mode==4)t.actor.IsAIEnabled=false;
        if(mode==5)t.actor.i_AI=nullptr,t.actor.IsAIEnabled=false;
        bool rawHook=t.actor.IsAIEnabled&&t.actor.AI()==&t.ai;
        bool halves=entry==101881&&rawHook;
        uint64 health=t.actor.GetHealth(attacker);float multiplier=t.actor.GetHealthMultiplierForTarget(attacker);
        uint32 raw=uint32(std::ceil(health*multiplier*intensity/4.f*(halves?2.f:1.f)));
        uint32 afterRaw=halves?raw/2:raw;
        uint32 normalized=uint32(std::min<double>(afterRaw/multiplier,std::numeric_limits<uint32>::max()));
        bool lethal=normalized>=health;
#ifdef WOUND_BASELINE
        bool held=rawHook&&afterRaw>=t.actor.GetHealth();
        if(mode!=5&&!held&&lethal)++missed;
        if(held&&!lethal)++overblocked;
#else
        bool held=mode!=5&&lethal;
#endif
        auto initial=t.actor.health;uint32 expected=held?0:normalized;
        assert(attacker->Deal(&t.actor,raw)==expected&&t.actor.alive==(expected<health));
        assert(t.ai.wait_heal==held&&t.actor.auraCalls==unsigned(held)&&t.actor.states==unsigned(held));
        assert(t.ai.speech==(held?Speech(t):std::vector<uint32>{}));
        assert(t.actor.health==initial); // Policy never grants or normalizes health.
        if(t.actor.alive){assert(attacker->Deal(&t.actor,raw)==expected);assert(t.actor.auraCalls==unsigned(held)&&t.ai.speech==(held?Speech(t):std::vector<uint32>{}));}
        NoProgress(t);++cases;
    }
#ifdef WOUND_BASELINE
    assert(missed&&overblocked);
    for(unsigned input=0;input<4;++input){Run<AI>t(entry);Combat(t);Hold(t);if(input==0)t.actor.health=399;if(input==3)t.actor.health=1100;t.filter.cancel=input==1;t.Heal(input==2?0:1);t.Tick();bool recovered=input!=0;assert(t.ai.wait_heal==!recovered&&t.actor.removed==unsigned(recovered));NoProgress(t);++cases;}
    std::cout<<"PASS COUNTERFACTUAL pinned57321f47e entry"<<entry<<' '<<cases<<" native target-health/normalization/raw hook/late dispatch/heal pipelines: "<<missed<<" missed lethal and "<<overblocked<<" premature holds; crossing40 missed and cancelled/zero/negative final heals clear the old wound. Expected broken boundaries, controlled combat/health services, not passing gameplay.\n";
#else
    for(unsigned source=0;source<4;++source)for(unsigned mode=0;mode<5;++mode)for(unsigned input=0;input<11;++input)
    {
        Run<AI>t(entry);Combat(t);Hold(t);Unit npc;Unit*healer=source==0?static_cast<Unit*>(&t.player):source==1?&t.pet:source==2?&t.guardian:&npc;
        // All sources use the real final-amount hook, including a controlled
        // healer AI cancellation. No owner/class gate is added to wound healing.
        healer->IsAIEnabled=true;healer->healerAI=&t.filter;
        if(mode==1)t.actor.i_AI=&t.temporary,t.actor.i_disabledAI=&t.ai;
        if(mode==2)t.actor.i_disabledAI=&t.ai;
        if(mode==3)t.actor.i_AI=nullptr,t.actor.i_disabledAI=&t.ai,t.actor.IsAIEnabled=false;
        if(mode==4)t.actor.i_AI=nullptr,t.actor.IsAIEnabled=false;
        uint32 amount=1;if(input==2)t.actor.health=399;if(input==3)t.actor.health=1,amount=398;if(input==6)t.actor.health=1100;
        t.filter.cancel=input==0;t.Heal(input==1?0:amount,healer);
        if(input==7)t.ai.Reset();if(input==8)t.actor.alive=false;if(input==9)t.actor.world=false;if(input==10)t.actor.predelete=true;
        t.Tick();bool recovered=mode!=4&&(input==2||input==4||input==5);
        assert(t.ai.wait_heal==!recovered&&t.actor.removed==unsigned(recovered)&&t.actor.states==(recovered?0u:1u));
        assert(t.actor.auraCalls==1&&t.ai.speech==Speech(t));NoProgress(t);
        // New applied healing can recover a still living wounded content AI;
        // restore controlled availability first, without calling ordinary AI.
        t.actor.alive=t.actor.world=true;t.actor.predelete=false;t.actor.i_AI=&t.ai;t.actor.i_disabledAI=nullptr;t.actor.IsAIEnabled=true;t.filter.cancel=false;
        if(!recovered){t.Heal(1000,healer);t.Tick();assert(!t.ai.wait_heal&&t.actor.removed==1);}
        assert(!t.ai._pendingWoundHeal);t.Tick(10000);assert(t.actor.removed==1);NoProgress(t);++cases;
    }
    for(bool accepted:{false,true})for(unsigned callback=0;callback<3;++callback)
    {
        Run<AI>t(entry);Combat(t);t.actor.auraAccepted=accepted;
        t.actor.onAura=[&]{assert(t.ai.wait_heal);if(callback==1)t.ai.Reset();if(callback==2)t.actor.i_AI=&t.temporary,t.actor.i_disabledAI=&t.ai;assert(t.player.Deal(&t.actor,UINT32_MAX)==0);};
        assert(t.player.Deal(&t.actor,UINT32_MAX)==0&&t.actor.auraCalls==1&&t.ai.wait_heal&&t.actor.health==1000&&t.actor.alive);NoProgress(t);++cases;
    }
    for(bool scalable:{false,true})for(bool present:{false,true})
    {
        Run<AI>t(entry);Combat(t);t.actor.scalable=scalable;t.actor.statPresent=present;t.actor.stat.healthMax=1000;
        assert(t.player.Deal(&t.actor,Lethal(t)-1)==999&&!t.ai.wait_heal);assert(t.player.Deal(&t.actor,Lethal(t))==0&&t.ai.wait_heal);NoProgress(t);++cases;
    }
    for(uint64 tiny:{1u,2u,16u})for(int32 hp:{1,32000,64000})for(uint32 raw:{0u,1u,2u,UINT32_MAX})
    {
        Run<AI>t(entry);Combat(t);t.actor.maximum=64000;t.actor.health=hp;t.actor.scalable=true;t.actor.stat.healthMax=tiny;
        uint64 targetHealth=t.actor.GetHealth(&t.player);assert(targetHealth>=1);
        uint32 modified=entry==101881?raw/2:raw;
        uint32 finalDamage=uint32(std::min<double>(modified/t.actor.GetHealthMultiplierForTarget(&t.player),std::numeric_limits<uint32>::max()));
        bool held=finalDamage>=targetHealth;
        assert(t.player.Deal(&t.actor,raw)==(held?0:finalDamage)&&t.actor.alive&&t.ai.wait_heal==held&&t.actor.health==hp);
        assert(t.actor.auraCalls==unsigned(held)&&t.ai.speech==(held?Speech(t):std::vector<uint32>{}));NoProgress(t);++cases;
    }
    {Run<AI>t(entry);Combat(t);Hold(t);t.Heal(1);t.Tick();assert(!t.ai.wait_heal&&t.actor.removed==1);assert(!t.player.Deal(&t.actor,Lethal(t))&&t.ai.wait_heal&&!t.ai._pendingWoundHeal&&t.actor.auraCalls==2);t.Tick();assert(t.ai.wait_heal&&t.actor.removed==1);t.Heal(1);t.Tick();auto twice=Speech(t);auto once=twice;twice.insert(twice.end(),once.begin(),once.end());assert(!t.ai.wait_heal&&t.actor.removed==2&&t.ai.speech==twice);NoProgress(t);++cases;}
    std::cout<<"PASS entry"<<entry<<' '<<cases<<" native target-relative GetHealth/scale/retained damage modifier/uint32 clamp/current+suspended/death-boundary and final DealHeal/Notify/lifecycle cases under ASAN: Player/pet/guardian/NPC, one-hit and nonlethal/no premature hold, repeated latch/failed aura/reentrant Reset+AI handoff, cancelled/zero/negative/overheal/cross40, missing AI/actor, reset and new wound. No health/progression/item grant; remaining combat/health/cast/movement services controlled, no client proof.\n";
#endif
    return cases;
}
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);unsigned total=Check<Taran>(101881);for(uint32 entry:{101882u,101883u,101885u,102063u})total+=Check<Small>(entry);std::cout<<"PASS all five bound Sheilun companion entries: "<<total<<" native contexts; two production AI classes, original per-role speech/damage modifier.\n";}
