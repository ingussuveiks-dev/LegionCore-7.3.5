#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <vector>
using uint8=uint8_t;
using uint32=uint32_t;
using uint64=uint64_t;
using FunctionList=std::multimap<uint64,std::function<void()>>;
#include "FunctionClass.inc"
#include "FunctionMethods.inc"
enum {TYPEID_PLAYER=4,UNIT_STATE_STUNNED=1,CRITERIA_TYPE_SCRIPT_EVENT_2=92,UNIT_FIELD_FLAGS=0,
    UNIT_FLAG_NOT_SELECTABLE=1,UNIT_FLAG_IMMUNE_TO_PC=2,UNIT_FLAG_IMMUNE_TO_NPC=4,UNIT_FLAG_NOT_ATTACKABLE_1=8};
struct Unit
{
    uint32 type=0;
    uint32 GetTypeId() const{return type;}
};
struct Player
{
    std::vector<uint32> credits;
    void UpdateAchievementCriteria(uint32 type,uint32 event){assert(type==92);credits.push_back(event);}
};
struct Creature;
struct CreatureAI
{
    virtual ~CreatureAI()=default;
    virtual void UpdateLifecycle(uint32){}
    virtual void MoveInLineOfSight(Unit*){}
    virtual void SummonedCreatureDies(Creature*,Unit*){}
    virtual void UpdateAI(uint32){}
    virtual void Reset(){}
    virtual void AttackStart(Creature*){}
    virtual void Talk(uint32){}
};
struct AddAI : CreatureAI
{
    uint32 attacks=0,talks=0;
    Creature* target=nullptr;
    void AttackStart(Creature* actor) override{++attacks;target=actor;}
    void Talk(uint32 line) override{assert(line==0);++talks;}
};
struct Creature
{
    FunctionProcessor functions;
    CreatureAI* i_AI=nullptr;
    CreatureAI* i_disabledAI=nullptr;
    bool m_AI_locked=false,IsAIEnabled=true,stunned=false,visible=false,aura=false,missingDefender=false;
    bool victim=false,failSpawn=false;
    uint32 flags=31,entry=106777;
    float positionX=0,positionY=0,positionZ=0,orientation=0;
    Creature* defender=nullptr;
    Player* nearby=nullptr;
    std::vector<Creature*> lights;
    std::vector<std::unique_ptr<Creature>> adds;
    std::vector<std::unique_ptr<AddAI>> addAIs;
    std::function<void()> onSummon;
    std::vector<uint32> casts,talks;
    CreatureAI* AI(){return i_AI;}
    bool AIM_Destroy();
    Player* FindNearestPlayer(float radius){assert(radius==100);return nearby;}
    bool HasAura(uint32 spell) const{assert(spell==211502);return aura;}
    void SetVisible(bool value){visible=value;}
    bool HasUnitState(uint32 state) const{assert(state==UNIT_STATE_STUNNED);return stunned;}
    Creature* FindNearestCreature(uint32 identity,float radius,bool alive)
    {assert(identity==106774 && radius==70 && alive);return missingDefender?nullptr:defender;}
    Creature* SummonCreature(uint32 identity,float spawnX,float spawnY,float spawnZ,float facing)
    {
        if(onSummon)onSummon();
        if(failSpawn)return nullptr;
        auto add=std::make_unique<Creature>();
        auto ai=std::make_unique<AddAI>();
        add->entry=identity;add->positionX=spawnX;add->positionY=spawnY;add->positionZ=spawnZ;add->orientation=facing;
        add->i_AI=ai.get();
        Creature* result=add.get();
        adds.push_back(std::move(add));addAIs.push_back(std::move(ai));
        return result;
    }
    float GetPositionX() const{return positionX;}
    float GetPositionY() const{return positionY;}
    float GetPositionZ() const{return positionZ;}
    float GetOrientation() const{return orientation;}
    void RemoveFlag(uint32 field,uint32 mask){assert(field==UNIT_FIELD_FLAGS);flags&=~mask;}
    void AddDelayedEvent(uint64 delay,std::function<void()>&& callback){functions.AddDelayedEvent(delay,std::move(callback));}
    void Lifecycle(uint32 diff)
    {
#include "LifecycleBridge.inc"
    }
    void Run(uint32 diff)
    {
        Lifecycle(diff);
        functions.Update(diff);
        if(i_AI)i_AI->UpdateAI(diff);
    }
};
uint32 diagnostics=0;
#define TC_LOG_DEBUG(...) ++diagnostics
#define ASSERT(value,...) assert(value)
#include "DestroyAI.inc"
struct ScriptedAI : CreatureAI
{
    Creature* me;
    explicit ScriptedAI(Creature* actor):me(actor){}
    void DoCast(uint32 spell){assert(spell==215258);me->casts.push_back(spell);}
    void Talk(uint32 line) override{assert(line==0);me->talks.push_back(line);}
    bool UpdateVictim() const{return me->victim;}
};
void GetCreatureListWithEntryInGrid(std::list<Creature*>& result,Creature* actor,uint32 entry,float radius)
{assert(entry==106094 && radius==30);result.assign(actor->lights.begin(),actor->lights.end());}
int irand(int minimum,int maximum){assert(minimum==-3 || minimum==-2);assert(maximum==-minimum);return 0;}
#include "WaveActor.inc"
using WaveAI=npc_travar_lastAI;
struct Fixture
{
    Creature actor,defender,lit,unlit;
    Player player;
    Unit visitor;
    WaveAI ai{&actor};
    Fixture()
    {
        actor.i_AI=&ai;actor.defender=&defender;actor.nearby=&player;
        lit.aura=true;actor.lights={&lit,&unlit};visitor.type=TYPEID_PLAYER;
    }
};
void CheckFirstWave(Creature const& actor,Creature* defender)
{
    assert(actor.adds.size()==3 && actor.addAIs.size()==3);
    assert(actor.adds[0]->entry==106580 && actor.adds[1]->entry==106578 && actor.adds[2]->entry==106578);
    assert(actor.adds[0]->positionX==1911.84f && actor.adds[0]->positionY==2334.61f && actor.adds[0]->positionZ==73.39f);
    for(auto const& ai:actor.addAIs)assert(ai->attacks==1 && ai->target==defender);
}
int main()
{
    {
        Fixture fixture;
        Unit pet,guardian;
        fixture.ai.MoveInLineOfSight(&pet);fixture.ai.MoveInLineOfSight(&guardian);
        assert(!fixture.ai._introDone && !fixture.ai.startCriteria && fixture.actor.adds.empty());
        fixture.ai.MoveInLineOfSight(&fixture.visitor);fixture.ai.MoveInLineOfSight(&fixture.visitor);
        assert(fixture.player.credits==std::vector<uint32>{50915});
        assert(fixture.actor.casts==std::vector<uint32>{215258} && fixture.actor.talks==std::vector<uint32>{0});
        assert(fixture.lit.visible && !fixture.unlit.visible);
        fixture.actor.stunned=true;fixture.actor.victim=true;
        fixture.actor.Run(4999);assert(fixture.actor.adds.empty());
        fixture.actor.Run(1);CheckFirstWave(fixture.actor,&fixture.defender);
        fixture.actor.Run(5000);assert(fixture.actor.adds.size()==3);
        assert(fixture.player.credits==std::vector<uint32>{50915});
        for(uint32 deaths=0;deaths<3;++deaths)fixture.ai.SummonedCreatureDies(nullptr,&pet);
        assert(fixture.actor.adds.size()==6 && fixture.ai.count_died==3);
        for(uint32 deaths=0;deaths<3;++deaths)fixture.ai.SummonedCreatureDies(nullptr,&guardian);
        assert(fixture.actor.adds.size()==7 && fixture.actor.adds.back()->entry==106719);
        assert(fixture.actor.adds.back()->flags==16 && fixture.actor.addAIs.back()->talks==1);
    }
    for(bool failSpawn:{false,true})
    {
        Fixture fixture;
        fixture.actor.failSpawn=failSpawn;fixture.actor.missingDefender=!failSpawn;
        fixture.ai.MoveInLineOfSight(&fixture.visitor);fixture.actor.Run(5000);
        assert(fixture.actor.adds.empty());
        fixture.actor.failSpawn=false;fixture.actor.missingDefender=false;fixture.actor.Run(5000);
        assert(fixture.actor.adds.empty());
    }
    {
        Fixture fixture;
        fixture.ai.MoveInLineOfSight(&fixture.visitor);fixture.ai.Reset();fixture.actor.Run(5000);
        CheckFirstWave(fixture.actor,&fixture.defender);
        fixture.ai.MoveInLineOfSight(&fixture.visitor);fixture.actor.Run(5000);
        assert(fixture.actor.adds.size()==3);
    }
    {
        Fixture fixture;
        fixture.ai.MoveInLineOfSight(&fixture.visitor);
        fixture.actor.nearby=nullptr;fixture.actor.Run(5000);CheckFirstWave(fixture.actor,&fixture.defender);
    }
    alignas(WaveAI) unsigned char storage[sizeof(WaveAI)];
    {
        Creature actor,defender;
        actor.defender=&defender;
        Unit visitor;visitor.type=TYPEID_PLAYER;
        WaveAI* original=new(storage) WaveAI(&actor);actor.i_AI=original;
        original->MoveInLineOfSight(&visitor);
        original->~WaveAI();actor.i_AI=nullptr;
        WaveAI* replacement=new(storage) WaveAI(&actor);actor.i_AI=replacement;
        actor.Run(5000);
#ifdef BASELINE
        CheckFirstWave(actor,&defender);
        assert(!replacement->_introDone && !replacement->startCriteria);
        replacement->~WaveAI();actor.i_AI=nullptr;
        std::cout<<"PASS baseline: actual Unit-owned five-second closure summons through a transparently replaced AI without its introduction; no freed-storage dereference or live-client claim\n";
        return 0;
#else
        assert(actor.adds.empty() && !replacement->_introDone && !replacement->startCriteria);
        replacement->MoveInLineOfSight(&visitor);actor.Run(4999);assert(actor.adds.empty());
        actor.Run(1);CheckFirstWave(actor,&defender);
        replacement->~WaveAI();actor.i_AI=nullptr;
#endif
    }
#ifndef BASELINE
    {
        Creature actor,defender;
        Unit visitor;visitor.type=TYPEID_PLAYER;actor.defender=&defender;
        uint32 unrelated=0;
        actor.functions.AddDelayedEvent(5000,[&unrelated]{++unrelated;});
        actor.i_AI=new WaveAI(&actor);actor.i_AI->MoveInLineOfSight(&visitor);
        assert(actor.AIM_Destroy() && !actor.i_AI && !actor.IsAIEnabled);
        actor.Run(5000);assert(actor.adds.empty() && unrelated==1);
    }
    for(uint32 mode=0;mode<4;++mode)
    {
        Fixture fixture;
        CreatureAI replacement;
        fixture.ai.MoveInLineOfSight(&fixture.visitor);
        if(mode){fixture.actor.i_disabledAI=&fixture.ai;fixture.actor.i_AI=mode==1?&replacement:mode==2?&fixture.ai:nullptr;}
        fixture.actor.onSummon=[&fixture]{assert(fixture.actor.m_AI_locked);fixture.actor.missingDefender=true;};
        fixture.actor.Lifecycle(4999);assert(fixture.actor.adds.empty());
        fixture.actor.Lifecycle(1);CheckFirstWave(fixture.actor,&fixture.defender);
        assert(!fixture.actor.m_AI_locked && fixture.ai._waveFunctions.Size()==0 && fixture.ai._waveFunctions.SizeQueue()==0);
    }
    {
        Fixture fixture;
        fixture.ai.MoveInLineOfSight(&fixture.visitor);
        fixture.actor.onSummon=[&fixture]{assert(!fixture.actor.AIM_Destroy());};
        uint32 before=diagnostics;
        fixture.actor.Lifecycle(5000);CheckFirstWave(fixture.actor,&fixture.defender);
        assert(diagnostics==before+3 && fixture.actor.i_AI==&fixture.ai && !fixture.actor.m_AI_locked);
    }
    std::cout<<"PASS: whole production last Travard AI/native FunctionProcessor and locked primary/disabled lifecycle, 4999+1ms, one queued wave, destruction/replacement/fresh retry, Reset preservation, summon failure/missing defender, original three-wave/flags/attacks and no added credit semantics; world services controlled, not client traversal\n";
#endif
}
