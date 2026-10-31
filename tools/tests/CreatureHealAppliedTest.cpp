#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
struct Unit;struct Creature;
struct CreatureAI
{
    virtual ~CreatureAI()=default;
    virtual void HealReceived(Unit*,uint32&){}
    virtual void HealDone(Unit*,uint32&){}
#include "Default.inc"
};
struct InstanceScript {bool CheckDamageManager()const{return false;}void UpdateDamageManager(uint64,int32,bool){assert(false);}};
struct Unit
{
    virtual ~Unit()=default;
    bool IsAIEnabled=false;
    int32 health=100,maximum=1000,modifications=0;
    CreatureAI* healerAI=nullptr;
    virtual Creature* ToCreature(){return nullptr;}
    virtual CreatureAI* GetAI(){return healerAI;}
    InstanceScript* GetInstanceScript(){return nullptr;}
    uint64 GetGUID()const{return 1;}
    // Controlled health service. Existing native ModifyHealth/scaling suites run separately.
    int32 ModifyHealth(int32 amount,Unit*){++modifications;int32 gain=std::min(maximum-health,amount);health+=gain;return gain;}
    int32 DealHeal(Unit*,uint32);
};
struct Creature:Unit
{
    bool m_AI_locked=false;
    CreatureAI* i_AI=nullptr;CreatureAI* i_disabledAI=nullptr;
    Creature* ToCreature()override{return this;}
    CreatureAI* AI(){return i_AI;}
    CreatureAI* GetAI()override{return AI();}
    void NotifyHealApplied(Unit*,uint32,int32);
    bool AIM_Destroy();bool AIM_Initialize(CreatureAI*);
};
#define ASSERT(condition,...) assert(condition)
#define TC_LOG_DEBUG(...) ((void)0)
#include "Destroy.inc"
#include "Initialize.inc"
#include "Notify.inc"
#include "DealHeal.inc"
struct Observer:CreatureAI
{
    Creature* actor;Unit* expectedHealer;int32 calls=0;uint32 amount=0;int32 gain=0;
    std::function<void()> callback;
    Observer(Creature* c,Unit* u):actor(c),expectedHealer(u){}
    void OnHealApplied(Unit* healer,uint32 finalAmount,int32 actualGain)override
    {
        assert(healer==expectedHealer&&actor->m_AI_locked);
        ++calls;amount=finalAmount;gain=actualGain;
        assert(!actor->AIM_Destroy()&&!actor->AIM_Initialize(nullptr));
        if(callback)callback();
    }
};
struct ChangeHeal:CreatureAI
{
    uint32 value;explicit ChangeHeal(uint32 v):value(v){}
    void HealDone(Unit*,uint32& amount)override{amount=value;}
};
int main()
{
    unsigned matrices=0;
    for(bool initiallyLocked:{false,true})for(bool enabled:{false,true})for(int pointers=0;pointers<5;++pointers)
    for(uint32 finalAmount:{0u,1u,50u,500u,2000u})for(int32 initialHealth:{100,1000})
    {
        Unit healer;Creature actor;Observer active(&actor,&healer),suspended(&actor,&healer);ChangeHeal adjust(finalAmount);
        healer.IsAIEnabled=true;healer.healerAI=&adjust;
        actor.IsAIEnabled=enabled;actor.m_AI_locked=initiallyLocked;actor.health=initialHealth;
        actor.i_AI=pointers==0||pointers==2?nullptr:&active;
        actor.i_disabledAI=pointers==0||pointers==1?nullptr:pointers==4?&active:&suspended;
        // Native HealReceived requires an AI whenever IsAIEnabled is true.
        if(enabled&&!actor.i_AI)continue;
        int32 expectedGain=std::min<int32>(1000-initialHealth,int32(finalAmount));
        assert(healer.DealHeal(&actor,777)==expectedGain);
        assert(actor.health==initialHealth+expectedGain&&actor.modifications==int(finalAmount!=0));
        assert(actor.m_AI_locked==initiallyLocked);
#ifdef BASELINE
        assert(!active.calls&&!suspended.calls);
#else
        assert(active.calls==int(finalAmount&&(actor.i_AI==&active||actor.i_disabledAI==&active)));
        assert(suspended.calls==int(finalAmount&&actor.i_disabledAI==&suspended));
        if(active.calls)assert(active.amount==finalAmount&&active.gain==expectedGain);
        if(suspended.calls)assert(suspended.amount==finalAmount&&suspended.gain==expectedGain);
#endif
        ++matrices;
    }
#ifndef BASELINE
    {Unit healer;Creature actor;std::vector<int> order;
     struct Receiver:Observer{std::vector<int>& order;Receiver(Creature* c,Unit* h,std::vector<int>& o):Observer(c,h),order(o){}
      void HealReceived(Unit*,uint32& incoming)override{order.push_back(1);assert(incoming==777);incoming=50;}
     } receiver(&actor,&healer,order);
     struct Healer:CreatureAI{std::vector<int>& order;explicit Healer(std::vector<int>& o):order(o){}
      void HealDone(Unit*,uint32& amount)override{order.push_back(2);assert(amount==50);amount=30;}
     } source(order);
     actor.i_AI=&receiver;actor.IsAIEnabled=true;healer.healerAI=&source;healer.IsAIEnabled=true;
     receiver.callback=[&]{order.push_back(3);assert(actor.health==130&&receiver.amount==30&&receiver.gain==30);};
     assert(healer.DealHeal(&actor,777)==30&&(order==std::vector<int>{1,2,3}));}
    {Unit healer;Creature actor;Observer active(&actor,&healer),suspended(&actor,&healer);actor.i_AI=&active;actor.i_disabledAI=&suspended;bool nested=false;
     active.callback=[&]{assert(actor.health==200);if(!nested){nested=true;actor.NotifyHealApplied(&healer,5,0);}assert(actor.m_AI_locked);};
     assert(healer.DealHeal(&actor,100)==100);assert(active.calls==2&&suspended.calls==2&&!actor.m_AI_locked);}
    for(bool prior:{false,true}){Unit healer;Creature actor;Observer active(&actor,&healer);actor.i_AI=&active;actor.m_AI_locked=prior;
     active.callback=[] {throw std::runtime_error("controlled callback");};bool caught=false;
     try{healer.DealHeal(&actor,100);}catch(std::runtime_error const&){caught=true;}
     assert(caught&&actor.health==200&&actor.modifications==1&&actor.m_AI_locked==prior);}
    {Unit healer;Creature actor;Observer active(&actor,&healer);actor.i_AI=&active;
     actor.NotifyHealApplied(&healer,0,0);actor.NotifyHealApplied(&healer,1,-1);assert(!active.calls&&!actor.m_AI_locked);
     actor.health=1100;assert(healer.DealHeal(&actor,100)==-100&&!active.calls&&actor.health==1000);}
    {Unit healer,victim;assert(healer.DealHeal(&victim,100)==100&&victim.health==200);}
    {Unit healer;Creature actor;CreatureAI empty;actor.i_AI=&empty;assert(healer.DealHeal(&actor,100)==100&&!actor.m_AI_locked);}
    std::cout<<"PASS "<<matrices<<" final-heal matrices plus nested/exception/default/player/negative cases: native DealHeal prefix, notification, AIM_Destroy and Initialize lock guard. Final amount/health observed after HealDone and one ModifyHealth; positive overheal, cancelled/zero, enabled/current/suspended/alias/missing AI, preserved lock. Controlled health/instance services; no client proof.\n";
#else
    std::cout<<"PASS COUNTERFACTUAL pinned5a4a13649: "<<matrices<<" native DealHeal contexts apply health but provide no post-heal callback, including valid overheal. Not an ordinary repaired run.\n";
#endif
}
