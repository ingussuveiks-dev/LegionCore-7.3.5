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
#include <set>
#include <vector>
using uint8=uint8_t;
using uint32=uint32_t;
using uint64=uint64_t;
using FunctionList=std::multimap<uint64,std::function<void()>>;
#include "FunctionClass.inc"
#include "FunctionMethods.inc"
enum {TYPEID_PLAYER=4,UNIT_STATE_STUNNED=1,CRITERIA_TYPE_SCRIPT_EVENT_2=92,UNIT_FIELD_FLAGS=0,
    UNIT_FLAG_NOT_SELECTABLE=1,UNIT_FLAG_IMMUNE_TO_PC=2,UNIT_FLAG_IMMUNE_TO_NPC=4,UNIT_FLAG_NOT_ATTACKABLE_1=8,
    CLASS_PALADIN=2,QUEST_STATUS_INCOMPLETE=3};
struct Player;
struct Creature;
struct TempSummon;
struct ObjectGuid
{
    uint64 low=0,high=0;
    bool operator<(ObjectGuid const& other) const{return high<other.high || (high==other.high && low<other.low);}
    bool operator!=(ObjectGuid const& other) const{return high!=other.high || low!=other.low;}
};
using GuidSet=std::set<ObjectGuid>;
uint64 nextGuid=100;
struct Map {} liveMap,otherMap;
struct Unit
{
    virtual ~Unit()=default;
    virtual Player* ToPlayer(){return nullptr;}
    virtual bool IsCreature() const{return false;}
    virtual Creature* ToCreature(){return nullptr;}
    bool IsAIEnabled=true;
    uint32 type=0;
    uint32 GetTypeId() const{return type;}
};
struct Player : Unit
{
    Player(){type=TYPEID_PLAYER;}
    Player* ToPlayer() override{return this;}
    bool world=true,preDelete=false,contact=true,alive=true,gm=false,teleport=false,phase=true,visible=true,los=true,rewarded=false;
    uint32 klass=2,status=3,instance=42,faction=0;
    float distance=1;
    Map* map=&liveMap;
    bool IsInWorld() const{return world;}
    bool IsPreDelete() const{return preDelete;}
    bool CanContact() const{return contact;}
    bool IsAlive() const{return alive;}
    bool isGameMaster() const{return gm;}
    bool IsBeingTeleported() const{return teleport;}
    uint32 getClass() const{return klass;}
    Map* GetMap() const{return map;}
    uint32 GetInstanceId() const{return instance;}
    bool InSamePhase(Creature*) const{return phase;}
    bool canSeeOrDetect(Creature*) const{return visible;}
    bool IsWithinLOSInMap(Creature*) const{return los;}
    bool IsWithinDistInMap(Creature*,float radius) const{return distance<=radius;}
    bool GetQuestRewardStatus(uint32 quest) const{assert(quest==42120);return rewarded;}
    uint32 GetQuestStatus(uint32 quest) const{assert(quest==42120);return status;}
    std::vector<uint32> credits;
    void UpdateAchievementCriteria(uint32 type,uint32 event){assert(type==92);credits.push_back(event);}
};
struct CreatureAI
{
    virtual ~CreatureAI()=default;
    virtual void UpdateLifecycle(uint32){}
    virtual void MoveInLineOfSight(Unit*){}
    virtual void SummonedCreatureDies(Creature*,Unit*){}
    virtual void JustSummoned(Creature*){}
    virtual void IsSummonedBy(Unit*){}
    virtual void UpdateAI(uint32){}
    virtual void Reset(){}
    virtual void AttackStart(Creature*){}
    virtual void Talk(uint32){}
};
struct AddAI : CreatureAI
{
    uint32 attacks=0,talks=0;
    Creature* target=nullptr;
    Creature* self=nullptr;
    std::function<void(Creature*)> onSummonedBy;
    void AttackStart(Creature* actor) override{++attacks;target=actor;}
    void Talk(uint32 line) override{assert(line==0);++talks;}
    void IsSummonedBy(Unit*) override{if(onSummonedBy)onSummonedBy(self);}
};
struct Creature : Unit
{
    Creature(){guid={nextGuid++,1};}
    ObjectGuid guid;
    ObjectGuid GetGUID() const{return guid;}
    bool IsCreature() const override{return true;}
    Creature* ToCreature() override{return this;}
    virtual TempSummon* ToTempSummon(){return nullptr;}
    FunctionProcessor functions;
    CreatureAI* i_AI=nullptr;
    CreatureAI* i_disabledAI=nullptr;
    bool m_AI_locked=false,stunned=false,visible=false,aura=false,missingDefender=false;
    bool victim=false,failSpawn=false;
    uint32 flags=31,entry=106777;
    bool world=true,preDelete=false,alive=true;
    uint32 mapId=1539,instance=42;
    Map* map=&liveMap;
    float positionX=0,positionY=0,positionZ=0,orientation=0;
    Creature* defender=nullptr;
    Player* nearby=nullptr;
    std::vector<Creature*> lights;
    std::vector<std::unique_ptr<Creature>> adds;
    std::vector<std::unique_ptr<AddAI>> addAIs;
    std::function<void()> onSummon;
    std::function<void()> onCast,onTalk;
    std::function<void(Creature*)> onSummonedBy;
    std::vector<uint32> casts,talks;
    CreatureAI* AI(){return i_AI;}
    bool IsInWorld() const{return world;}
    bool IsPreDelete() const{return preDelete;}
    bool IsAlive() const{return alive;}
    uint32 GetEntry() const{return entry;}
    uint32 GetMapId() const{return mapId;}
    uint32 GetInstanceId() const{return instance;}
    Map* GetMap() const{return map;}
    bool AIM_Destroy();
    Player* FindNearestPlayer(float radius){assert(radius==100);return nearby;}
    bool HasAura(uint32 spell) const{assert(spell==211502);return aura;}
    void SetVisible(bool value){visible=value;}
    bool HasUnitState(uint32 state) const{assert(state==UNIT_STATE_STUNNED);return stunned;}
    Creature* FindNearestCreature(uint32 identity,float radius,bool alive)
    {assert(identity==106774 && radius==70 && alive);return missingDefender?nullptr:defender;}
    Creature* SummonCreature(uint32 identity,float spawnX,float spawnY,float spawnZ,float facing);
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
struct TempSummon : Creature
{
    Creature* owner=nullptr;
    ObjectGuid summonerGuid;
    Unit* GetSummoner(){return owner;}
    ObjectGuid GetSummonerGUID() const{return summonerGuid;}
    TempSummon* ToTempSummon() override{return this;}
    void InitSummon();
};
#include "InitSummon.inc"
void RouteDeath(Creature* creature,Unit* killer)
{
#include "DeathBridge.inc"
}
void KillSummon(Creature* creature,Unit* killer)
{
    creature->alive=false;
    RouteDeath(creature,killer);
}
Creature* Creature::SummonCreature(uint32 identity,float spawnX,float spawnY,float spawnZ,float facing)
{
    if(onSummon)onSummon();
    if(failSpawn)return nullptr;
    auto add=std::make_unique<TempSummon>();
    auto ai=std::make_unique<AddAI>();
    add->owner=this;add->summonerGuid=GetGUID();add->map=map;add->instance=instance;
    add->entry=identity;add->positionX=spawnX;add->positionY=spawnY;add->positionZ=spawnZ;add->orientation=facing;
    add->i_AI=ai.get();ai->self=add.get();ai->onSummonedBy=onSummonedBy;
    TempSummon* result=add.get();
    adds.push_back(std::move(add));addAIs.push_back(std::move(ai));
    result->InitSummon();
    return result;
}
uint32 diagnostics=0;
#define TC_LOG_DEBUG(...) ++diagnostics
#define ASSERT(value,...) assert(value)
#include "DestroyAI.inc"
struct ScriptedAI : CreatureAI
{
    Creature* me;
    explicit ScriptedAI(Creature* actor):me(actor){}
    void DoCast(uint32 spell){assert(spell==215258);me->casts.push_back(spell);if(me->onCast)me->onCast();}
    void Talk(uint32 line) override{assert(line==0);me->talks.push_back(line);if(me->onTalk)me->onTalk();}
    bool UpdateVictim() const{return me->victim;}
};
void GetCreatureListWithEntryInGrid(std::list<Creature*>& result,Creature* actor,uint32 entry,float radius)
{assert(entry==106094 && radius==30);result.assign(actor->lights.begin(),actor->lights.end());}
int irand(int minimum,int maximum){assert(minimum==-3 || minimum==-2);assert(maximum==-minimum);return 0;}
struct Scenario
{
    uint32 id=1092,step=9;
    uint32 GetScenarioId() const{return id;}
    uint32 GetCurrentStep() const{return step;}
};
struct ScenarioManager
{
    bool present=true;
    Scenario scenario;
    Scenario* GetScenario(uint32 instance){return present && instance==42?&scenario:nullptr;}
} manager;
auto* sScenarioMgr=&manager;
#include "FinalEntryPolicy.inc"
#include "WaveActor.inc"
using WaveAI=npc_travar_lastAI;
struct Fixture
{
    Creature actor,defender,lit,unlit;
    Player player;
    Player visitor;
    WaveAI ai{&actor};
    Fixture()
    {
        actor.i_AI=&ai;actor.defender=&defender;actor.nearby=&player;
        lit.aura=true;actor.lights={&lit,&unlit};visitor.type=TYPEID_PLAYER;
        manager=ScenarioManager{};
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
#ifdef DEATH_BASELINE
    {
        Fixture fixture;fixture.ai.MoveInLineOfSight(&fixture.visitor);fixture.actor.stunned=true;
        Unit pet;
        for(uint32 duplicate=0;duplicate<6;++duplicate)fixture.ai.SummonedCreatureDies(nullptr,&pet);
        assert(fixture.ai.count_died==6 && fixture.actor.adds.size()==4 && fixture.actor.adds.back()->entry==106719);
        std::cout<<"PASS pinned1f73b44c: six null death notifications advance both regular-wave thresholds and create Mordoth before scheduled first wave\n";
    }
    {
        Creature actor,defender;Player visitor;actor.defender=&defender;manager=ScenarioManager{};
        auto* original=new WaveAI(&actor);actor.i_AI=original;original->MoveInLineOfSight(&visitor);actor.stunned=true;actor.Lifecycle(5000);
        delete original;actor.i_AI=nullptr;
        auto* replacement=new WaveAI(&actor);actor.i_AI=replacement;replacement->MoveInLineOfSight(&visitor);
        for(uint32 identity=0;identity<6;++identity)KillSummon(actor.adds[identity].get(),nullptr);
        assert(replacement->count_died==6 && replacement->_waveFunctions.SizeQueue()==1);
        assert(actor.adds.size()==7 && actor.adds.back()->entry==106719);
        delete replacement;actor.i_AI=nullptr;
        std::cout<<"PASS pinned1f73b44c: real old-AI summons notify recreated current owner AI through native death bridge, reaching wave2/Mordoth before its queued first wave\n";
    }
    return 0;
#else
#ifdef ADMISSION_BASELINE
    {
        Fixture fixture;fixture.visitor.klass=5;fixture.visitor.status=0;
        manager.scenario.id=991;manager.scenario.step=0;
        fixture.ai.MoveInLineOfSight(&fixture.visitor);
        assert(fixture.ai._introDone && fixture.ai.startCriteria && fixture.player.credits==std::vector<uint32>{50915});
        fixture.actor.Run(5000);
        CheckFirstWave(fixture.actor,&fixture.defender);
        std::cout<<"PASS pinned67b2f259: inactive Priest in wrong scenario starts last Travard ritual/lights/wave and credits unchecked nearby Player50915\n";
        return 0;
    }
#else
#ifndef BASELINE
    {
        Fixture fixture;Unit pet,guardian,npc;
        assert(!SilverHandFinalEntry::CanStart(nullptr,&fixture.actor));
        assert(!SilverHandFinalEntry::CanStart(&fixture.visitor,nullptr));
        fixture.ai.MoveInLineOfSight(nullptr);fixture.ai.MoveInLineOfSight(&pet);fixture.ai.MoveInLineOfSight(&guardian);fixture.ai.MoveInLineOfSight(&npc);
        assert(!fixture.ai._introDone && fixture.actor.casts.empty());
    }
    for(uint32 invalid=0;invalid<28;++invalid)
    {
        Fixture fixture;
        switch(invalid)
        {
            case 0:fixture.visitor.world=false;break;
            case 1:fixture.visitor.preDelete=true;break;
            case 2:fixture.visitor.contact=false;break;
            case 3:fixture.visitor.alive=false;break;
            case 4:fixture.visitor.gm=true;break;
            case 5:fixture.visitor.teleport=true;break;
            case 6:fixture.visitor.klass=1;break;
            case 7:fixture.visitor.map=nullptr;break;
            case 8:fixture.visitor.map=&otherMap;break;
            case 9:fixture.visitor.instance=43;break;
            case 10:fixture.visitor.phase=false;break;
            case 11:fixture.visitor.visible=false;break;
            case 12:fixture.visitor.los=false;break;
            case 13:fixture.visitor.distance=100.01f;break;
            case 14:fixture.visitor.rewarded=true;break;
            case 15:fixture.visitor.status=0;break;
            case 16:fixture.visitor.status=1;break;
            case 17:fixture.actor.world=false;break;
            case 18:fixture.actor.preDelete=true;break;
            case 19:fixture.actor.alive=false;break;
            case 20:fixture.actor.entry=106676;break;
            case 21:fixture.actor.mapId=1220;break;
            case 22:fixture.actor.map=nullptr;break;
            case 23:fixture.actor.instance=43;break;
            case 24:manager.present=false;break;
            case 25:manager.scenario.id=991;break;
            case 26:manager.scenario.id=1037;break;
            case 27:manager.scenario.step=10;break;
        }
        fixture.ai.MoveInLineOfSight(&fixture.visitor);fixture.actor.Run(5000);
        assert(!fixture.ai._introDone && !fixture.ai.startCriteria && fixture.actor.casts.empty() && fixture.actor.talks.empty());
        assert(fixture.player.credits.empty() && !fixture.lit.visible && fixture.actor.adds.empty());
        fixture.visitor.world=true;fixture.visitor.preDelete=false;fixture.visitor.contact=true;fixture.visitor.alive=true;
        fixture.visitor.gm=false;fixture.visitor.teleport=false;fixture.visitor.klass=2;fixture.visitor.map=&liveMap;
        fixture.visitor.instance=42;fixture.visitor.phase=true;fixture.visitor.visible=true;fixture.visitor.los=true;
        fixture.visitor.distance=100;fixture.visitor.rewarded=false;fixture.visitor.status=3;
        fixture.actor.world=true;fixture.actor.preDelete=false;fixture.actor.alive=true;fixture.actor.entry=106777;
        fixture.actor.mapId=1539;fixture.actor.map=&liveMap;fixture.actor.instance=42;manager=ScenarioManager{};
        fixture.ai.MoveInLineOfSight(&fixture.visitor);fixture.actor.stunned=true;
        fixture.actor.Run(4999);assert(fixture.actor.adds.empty());
        fixture.actor.Run(1);CheckFirstWave(fixture.actor,&fixture.defender);
        assert(fixture.player.credits.empty() && fixture.visitor.credits.empty());
    }
    for(uint32 step=0;step<12;++step)
    {
        Fixture fixture;manager.scenario.step=step;
        fixture.ai.MoveInLineOfSight(&fixture.visitor);fixture.actor.stunned=true;fixture.actor.Run(5000);
        assert(fixture.ai._introDone==(step==9) && fixture.actor.adds.size()==(step==9?3u:0u));
    }
    for(uint32 faction:{0u,1u})
    {
        Fixture fixture;fixture.visitor.faction=faction;
        fixture.actor.onCast=[&fixture]{fixture.ai.MoveInLineOfSight(&fixture.visitor);};
        fixture.actor.onTalk=fixture.actor.onCast;
        fixture.ai.MoveInLineOfSight(&fixture.visitor);fixture.ai.MoveInLineOfSight(&fixture.visitor);
        assert(fixture.actor.casts==std::vector<uint32>{215258} && fixture.actor.talks==std::vector<uint32>{0});
        fixture.actor.stunned=true;fixture.actor.Run(5000);CheckFirstWave(fixture.actor,&fixture.defender);
        assert(fixture.player.credits.empty() && fixture.visitor.credits.empty());
    }
    std::cout<<"PASS final entry: actual triggering Player/28 invalid contexts/all12 steps/both factions/100yd/retry and cast-speech reentry; no nearest substitute or50915; original own5000ms wave retained. Accepted-wave progress/re-entry/client remain separate\n";
    for(uint32 killerKind=0;killerKind<5;++killerKind)
    {
        Fixture fixture;Player player;Creature pet,guardian,ally;
        std::vector<Unit*> killers{&player,&pet,&guardian,&ally,nullptr};
        Unit* killer=killers[killerKind];
        fixture.ai.MoveInLineOfSight(&fixture.visitor);fixture.actor.stunned=true;
        fixture.ai.SummonedCreatureDies(nullptr,killer);
        Creature unrelated;unrelated.alive=false;fixture.ai.SummonedCreatureDies(&unrelated,killer);
        assert(fixture.ai.count_died==0 && fixture.ai._waveSummons.empty());
        fixture.actor.Lifecycle(5000);assert(fixture.ai._waveSummons.size()==3);
        Creature* first=fixture.actor.adds[0].get();
        RouteDeath(first,killer);assert(fixture.ai.count_died==0 && fixture.ai._waveSummons.size()==3);
        Creature collision;collision.guid=first->guid;++collision.guid.high;collision.alive=false;
        fixture.ai.SummonedCreatureDies(&collision,killer);assert(fixture.ai.count_died==0);
        KillSummon(first,killer);fixture.ai.JustSummoned(first);
        for(uint32 repeat=0;repeat<300;++repeat)RouteDeath(first,killer);
        assert(fixture.ai.count_died==1 && fixture.ai._waveSummons.size()==2 && fixture.actor.adds.size()==3);
        KillSummon(fixture.actor.adds[1].get(),killer);KillSummon(fixture.actor.adds[2].get(),killer);
        assert(fixture.ai.count_died==3 && fixture.ai._waveSummons.size()==3 && fixture.actor.adds.size()==6);
        fixture.ai.Reset();RouteDeath(first,killer);
        for(uint32 identity=3;identity<6;++identity)KillSummon(fixture.actor.adds[identity].get(),killer);
        assert(fixture.ai.count_died==6 && fixture.ai._waveSummons.empty() && fixture.actor.adds.size()==7);
        assert(fixture.actor.adds.back()->entry==106719);
        KillSummon(fixture.actor.adds.back().get(),killer);
        for(uint32 repeat=0;repeat<300;++repeat)fixture.ai.SummonedCreatureDies(nullptr,killer);
        assert(fixture.ai.count_died==6 && fixture.actor.adds.size()==7);
        assert(fixture.player.credits.empty() && fixture.visitor.credits.empty());
    }
    for(uint32 invalid=0;invalid<9;++invalid)
    {
        Fixture fixture;fixture.ai.MoveInLineOfSight(&fixture.visitor);fixture.actor.stunned=true;
        TempSummon candidate;candidate.entry=106580;candidate.owner=&fixture.actor;candidate.summonerGuid=fixture.actor.GetGUID();
        switch(invalid)
        {
            case 0:fixture.ai._introDone=false;break;
            case 1:candidate.alive=false;break;
            case 2:candidate.entry=106719;break;
            case 3:candidate.entry=106774;break;
            case 4:++candidate.summonerGuid.high;break;
            case 5:candidate.map=&otherMap;break;
            case 6:candidate.instance=43;break;
            case 7:fixture.actor.map=nullptr;break;
            case 8:candidate.map=nullptr;break;
        }
        fixture.ai.JustSummoned(&candidate);assert(fixture.ai._waveSummons.empty());
        KillSummon(&candidate,nullptr);assert(fixture.ai.count_died==0);
    }
    {
        Fixture fixture;fixture.ai.MoveInLineOfSight(&fixture.visitor);
        fixture.ai.JustSummoned(nullptr);
        Creature ordinary;ordinary.entry=106580;fixture.ai.JustSummoned(&ordinary);
        assert(fixture.ai._waveSummons.empty());
    }
    for(uint32 invalid=0;invalid<3;++invalid)
    {
        Fixture fixture;fixture.ai.MoveInLineOfSight(&fixture.visitor);fixture.actor.stunned=true;fixture.actor.Lifecycle(5000);
        Creature* first=fixture.actor.adds[0].get();first->alive=false;
        if(invalid==0)fixture.actor.world=false;
        if(invalid==1)fixture.actor.preDelete=true;
        if(invalid==2)fixture.actor.alive=false;
        RouteDeath(first,nullptr);assert(fixture.ai.count_died==0 && fixture.ai._waveSummons.size()==3);
        fixture.actor.world=true;fixture.actor.preDelete=false;fixture.actor.alive=true;
        RouteDeath(first,nullptr);assert(fixture.ai.count_died==1 && fixture.ai._waveSummons.size()==2);
    }
    {
        Fixture fixture;Creature guardian;
        fixture.actor.onSummonedBy=[&fixture,&guardian](Creature* summon){
            if(summon->GetEntry()==106719)return;
            assert(fixture.ai._waveSummons.count(summon->GetGUID())==1 && fixture.actor.m_AI_locked);
            KillSummon(summon,&guardian);RouteDeath(summon,&guardian);
        };
        fixture.ai.MoveInLineOfSight(&fixture.visitor);fixture.actor.stunned=true;fixture.actor.Lifecycle(5000);
        assert(fixture.ai.count_died==6 && fixture.ai._waveSummons.empty() && fixture.actor.adds.size()==7);
        assert(fixture.actor.adds.back()->entry==106719 && fixture.player.credits.empty());
    }
    {
        Creature actor,defender;Player visitor;actor.defender=&defender;manager=ScenarioManager{};
        auto* original=new WaveAI(&actor);actor.i_AI=original;original->MoveInLineOfSight(&visitor);actor.stunned=true;actor.Lifecycle(5000);
        delete original;actor.i_AI=nullptr;
        auto* replacement=new WaveAI(&actor);actor.i_AI=replacement;replacement->MoveInLineOfSight(&visitor);
        for(uint32 identity=0;identity<3;++identity)KillSummon(actor.adds[identity].get(),nullptr);
        assert(replacement->count_died==0 && replacement->_waveSummons.empty() && actor.adds.size()==3);
        actor.Lifecycle(5000);
        for(uint32 identity=0;identity<3;++identity)RouteDeath(actor.adds[identity].get(),nullptr);
        assert(replacement->count_died==0 && replacement->_waveSummons.size()==3);
        for(uint32 identity=3;identity<6;++identity)KillSummon(actor.adds[identity].get(),nullptr);
        assert(replacement->count_died==3 && replacement->_waveSummons.size()==3 && actor.adds.size()==9);
        delete replacement;actor.i_AI=nullptr;
    }
    std::cout<<"PASS wave deaths: native TempSummon owner-before-IsSummonedBy/death-owner bridge, full GUID registration, live/null/untracked/wrong owner-map-entry rejects, duplicate/dead-hook rearm/wrap prevention, actual three+three thresholds, Player/pet/guardian/ally/null killer labels, Reset/replacement and one-hit-before-return reentry; no Player credit added\n";
#endif
    {
        Fixture fixture;
        Unit pet,guardian;
        fixture.ai.MoveInLineOfSight(&pet);fixture.ai.MoveInLineOfSight(&guardian);
        assert(!fixture.ai._introDone && !fixture.ai.startCriteria && fixture.actor.adds.empty());
        fixture.ai.MoveInLineOfSight(&fixture.visitor);fixture.ai.MoveInLineOfSight(&fixture.visitor);
#ifdef BASELINE
        assert(fixture.player.credits==std::vector<uint32>{50915});
#else
        assert(fixture.player.credits.empty());
#endif
        assert(fixture.actor.casts==std::vector<uint32>{215258} && fixture.actor.talks==std::vector<uint32>{0});
        assert(fixture.lit.visible && !fixture.unlit.visible);
        fixture.actor.stunned=true;fixture.actor.victim=true;
        fixture.actor.Run(4999);assert(fixture.actor.adds.empty());
        fixture.actor.Run(1);CheckFirstWave(fixture.actor,&fixture.defender);
        fixture.actor.Run(5000);assert(fixture.actor.adds.size()==3);
#ifdef BASELINE
        assert(fixture.player.credits==std::vector<uint32>{50915});
#else
        assert(fixture.player.credits.empty());
#endif
        for(uint32 deaths=0;deaths<3;++deaths)KillSummon(fixture.actor.adds[deaths].get(),&pet);
        assert(fixture.actor.adds.size()==6 && fixture.ai.count_died==3);
        for(uint32 deaths=3;deaths<6;++deaths)KillSummon(fixture.actor.adds[deaths].get(),&guardian);
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
        Player visitor;
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
        Player visitor;actor.defender=&defender;
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
        if(mode==3)fixture.actor.IsAIEnabled=false;
        fixture.actor.onSummon=[&fixture]{assert(fixture.actor.m_AI_locked);fixture.actor.missingDefender=true;};
        fixture.actor.Lifecycle(4999);assert(fixture.actor.adds.empty());
        fixture.actor.Lifecycle(1);CheckFirstWave(fixture.actor,&fixture.defender);
        assert(!fixture.actor.m_AI_locked && fixture.ai._waveFunctions.Size()==0 && fixture.ai._waveFunctions.SizeQueue()==0);
        assert(fixture.ai._waveSummons.size()==3);
        fixture.actor.i_AI=&fixture.ai;fixture.actor.i_disabledAI=nullptr;fixture.actor.IsAIEnabled=true;
        fixture.actor.missingDefender=false;
        fixture.actor.onSummon={};
        for(uint32 identity=0;identity<3;++identity)KillSummon(fixture.actor.adds[identity].get(),nullptr);
        assert(fixture.ai.count_died==3 && fixture.actor.adds.size()==6);
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
#endif
#endif
}
