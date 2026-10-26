#include <atomic>
#include <array>
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
struct Reference
{
    Player* player;
    Player* getSource() const{return player;}
};
struct Map
{
    std::vector<Reference> players;
    std::vector<Reference> const& GetPlayers() const{return players;}
} liveMap,otherMap;
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
    std::function<void()> onCredit;
    void UpdateAchievementCriteria(uint32 type,uint32 event){assert(type==92);credits.push_back(event);if(onCredit)onCredit();}
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
    uint32 flags=31,entry=106777,failEntry=0,failAddMask=0,addAttempts=0;
    bool world=true,preDelete=false,alive=true;
    uint32 mapId=1539,instance=42;
    Map* map=&liveMap;
    float positionX=0,positionY=0,positionZ=0,orientation=0;
    Creature* defender=nullptr;
    Player* nearby=nullptr;
    std::vector<Creature*> lights;
    std::vector<std::unique_ptr<Creature>> adds;
    std::vector<std::unique_ptr<AddAI>> addAIs;
    std::vector<std::array<float,4>> addRequests;
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
    if(identity==106578)
    {
        addRequests.push_back({spawnX,spawnY,spawnZ,facing});
        uint32 attempt=addAttempts++;
        if(attempt<2 && (failAddMask & (1u<<attempt)))return nullptr;
    }
    if(failSpawn || identity==failEntry)return nullptr;
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
bool distinctJitter=false;
uint32 jitterCalls=0;
int irand(int minimum,int maximum){assert(minimum==-3 || minimum==-2);assert(maximum==-minimum);return distinctJitter?((jitterCalls++/2)%2?maximum:minimum):0;}
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
        distinctJitter=false;jitterCalls=0;
        actor.i_AI=&ai;actor.defender=&defender;actor.nearby=&player;
        lit.aura=true;actor.lights={&lit,&unlit};visitor.type=TYPEID_PLAYER;
        manager=ScenarioManager{};
        liveMap.players={{&player}};
    }
    ~Fixture(){liveMap.players.clear();distinctJitter=false;jitterCalls=0;}
};
void CheckFirstWave(Creature const& actor,Creature* defender)
{
    assert(actor.adds.size()==3 && actor.addAIs.size()==3);
    assert(actor.adds[0]->entry==106580 && actor.adds[1]->entry==106578 && actor.adds[2]->entry==106578);
    assert(actor.adds[0]->positionX==1911.84f && actor.adds[0]->positionY==2334.61f && actor.adds[0]->positionZ==73.39f);
    for(auto const& ai:actor.addAIs)assert(ai->attacks==1 && ai->target==defender);
}
void StartMissingDefender(Fixture& fixture,uint32 wave)
{
    assert(wave>=1 && wave<=3);
    fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.stunned=true;
    if(wave==1)fixture.actor.missingDefender=true;
    fixture.actor.Lifecycle(5000);
    if(wave==1)return;
    KillSummon(fixture.actor.adds[0].get(),&fixture.player);
    KillSummon(fixture.actor.adds[1].get(),&fixture.player);
    if(wave==2)fixture.actor.missingDefender=true;
    KillSummon(fixture.actor.adds[2].get(),&fixture.player);
    if(wave==2)return;
    KillSummon(fixture.actor.adds[3].get(),&fixture.player);
    KillSummon(fixture.actor.adds[4].get(),&fixture.player);
    fixture.actor.missingDefender=true;
    KillSummon(fixture.actor.adds[5].get(),&fixture.player);
}
void StartFailedCreation(Fixture& fixture,uint32 wave)
{
    assert(wave>=1 && wave<=3);
    fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.stunned=true;
    if(wave==1)fixture.actor.failSpawn=true;
    fixture.actor.Lifecycle(5000);
    if(wave==1)return;
    KillSummon(fixture.actor.adds[0].get(),&fixture.player);
    KillSummon(fixture.actor.adds[1].get(),&fixture.player);
    if(wave==2)fixture.actor.failSpawn=true;
    KillSummon(fixture.actor.adds[2].get(),&fixture.player);
    if(wave==2)return;
    KillSummon(fixture.actor.adds[3].get(),&fixture.player);
    KillSummon(fixture.actor.adds[4].get(),&fixture.player);
    fixture.actor.failSpawn=true;
    KillSummon(fixture.actor.adds[5].get(),&fixture.player);
}
void StartPartialWave(Fixture& fixture,uint32 wave,uint32 mask)
{
    assert(wave==1 || wave==2);
    fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.stunned=true;
    if(wave==1){fixture.actor.failAddMask=mask;distinctJitter=true;jitterCalls=0;}
    fixture.actor.Lifecycle(5000);
    if(wave==1)return;
    KillSummon(fixture.actor.adds[0].get(),&fixture.player);
    KillSummon(fixture.actor.adds[1].get(),&fixture.player);
    fixture.actor.failAddMask=mask;fixture.actor.addAttempts=0;fixture.actor.addRequests.clear();
    distinctJitter=true;jitterCalls=0;
    KillSummon(fixture.actor.adds[2].get(),&fixture.player);
}
int main()
{
#ifdef PARTIAL_BASELINE
    for(uint32 wave:{1u,2u})
        for(uint32 mask:{1u,2u,3u})
        {
            Fixture fixture;StartPartialWave(fixture,wave,mask);
            uint32 missing=mask==3?2u:1u,created=wave*3-missing;
            assert(fixture.actor.adds.size()==created && fixture.ai._waveFunctions.SizeQueue()==0);
            for(uint32 identity=(wave-1)*3;identity<created;++identity)KillSummon(fixture.actor.adds[identity].get(),nullptr);
            fixture.actor.Lifecycle(10000);
            assert(fixture.actor.adds.size()==created && fixture.ai.count_died==created && fixture.ai._waveSummons.empty());
        }
    std::cout<<"PASS pinned5c2c3447: either/both null106578 slots in original first/second wave are lost; subsequent valid creator and all extant deaths cannot reach3/6 or advance waves without missing actors\n";
    return 0;
#else
#ifdef CREATION_BASELINE
    for(uint32 wave:{1u,2u,3u})
    {
        Fixture fixture;StartFailedCreation(fixture,wave);
        uint32 originalAdds=(wave-1)*3;
        assert(fixture.actor.adds.size()==originalAdds && fixture.ai._waveFunctions.SizeQueue()==0);
        fixture.actor.failSpawn=false;fixture.actor.Lifecycle(5000);
        assert(fixture.actor.adds.size()==originalAdds && fixture.ai.count_died==originalAdds && fixture.player.credits.empty());
    }
    std::cout<<"PASS pinned7bef0cb3: null first/second leader or Mordoth creation permanently loses its wave despite subsequent valid creation; no queued recovery\n";
    return 0;
#else
#ifdef DEFENDER_BASELINE
    for(uint32 wave:{1u,2u,3u})
    {
        Fixture fixture;StartMissingDefender(fixture,wave);
        uint32 originalAdds=wave==1?0u:wave==2?3u:6u;
        assert(fixture.actor.adds.size()==originalAdds && fixture.ai._waveFunctions.SizeQueue()==0);
        fixture.actor.missingDefender=false;fixture.actor.Run(5000);
        assert(fixture.actor.adds.size()==originalAdds && fixture.ai.count_died==(wave-1)*3);
        assert(fixture.player.credits.empty());
    }
    std::cout<<"PASS pinned1eb802a1: first/second/Mordoth requests are lost if native defender lookup is empty; later valid actor and lifecycle do not recover any wave\n";
    return 0;
#else
#ifdef LATE_WAVE_BASELINE
    for(uint32 threshold:{3u,6u})
    {
        Fixture fixture;Creature guardian;uint32 progress=179;
        fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.stunned=true;fixture.actor.Lifecycle(5000);
        for(uint32 identity=0;identity<threshold-1;++identity)KillSummon(fixture.actor.adds[identity].get(),&guardian);
        fixture.actor.stunned=false;
        fixture.player.onCredit=[&progress](){if(++progress==180)manager.scenario.step=10;};
        fixture.ai.UpdateAI(1000);
        assert(progress==180 && manager.scenario.step==10);
        KillSummon(fixture.actor.adds[threshold-1].get(),&guardian);
        assert(fixture.ai.count_died==threshold && fixture.actor.adds.size()==(threshold==3?6u:7u));
        if(threshold==6)assert(fixture.actor.adds.back()->entry==106719);
    }
    std::cout<<"PASS pinned7974ba11: native owner-death bridge creates wave2 or new attacking Mordoth after final179->180 ritual pulse moves stage label10; progress sink controlled, native weighted tree independently checked\n";
    return 0;
#else
#ifdef CONTEXT_BASELINE
    {
        Fixture fixture;
        uint32 progress=179;
        fixture.player.onCredit=[&progress](){if(++progress==180)manager.scenario.step=10;};
        fixture.ai.MoveInLineOfSight(&fixture.player);
        fixture.actor.Run(1000);
        assert(progress==180 && manager.scenario.step==10 && fixture.actor.adds.empty());
        fixture.actor.Run(4000);
        CheckFirstWave(fixture.actor,&fixture.defender);
        assert(fixture.ai._introDone && fixture.ai.startCriteria && fixture.player.credits.size()==1);
        std::cout<<"PASS pinned f3e7c9e2: accepted5000ms callback spawns first wave after179->180 pulse changes native-stage label to10; controlled progress sink, native weighted threshold checked separately\n";
        return 0;
    }
#else
#ifdef PROGRESS_BASELINE
    {
        Fixture fixture;
        fixture.visitor.klass=5;fixture.visitor.status=0;
        fixture.actor.nearby=&fixture.visitor;
        liveMap.players={{&fixture.visitor},{&fixture.player}};
        fixture.ai._introDone=true;fixture.ai.startCriteria=true;
        fixture.ai.UpdateAI(1000);
        assert(fixture.visitor.credits==std::vector<uint32>{51838} && fixture.player.credits.empty());
        manager.scenario.step=10;fixture.ai.UpdateAI(1000);
        assert(fixture.visitor.credits.size()==2);
        std::cout<<"PASS pinned df5bf3f5: unchecked nearby inactive Priest takes ritual pulse from eligible map participant and keeps receiving pulses after stage9\n";
        return 0;
    }
#else
#if !defined(BASELINE) && !defined(ADMISSION_BASELINE) && !defined(DEATH_BASELINE) && !defined(PROGRESS_BASELINE)
    for(uint32 wave:{1u,2u})
        for(uint32 mask:{1u,2u,3u})
            for(uint32 mode=0;mode<4;++mode)
            {
                Fixture fixture;CreatureAI replacement;Creature recoveredDefender,guardian;
                StartPartialWave(fixture,wave,mask);
                uint32 missing=mask==3?2u:1u,created=wave*3-missing,before=diagnostics;
                auto requested=fixture.actor.addRequests;
                assert(requested.size()==2 && fixture.actor.adds.size()==created && fixture.ai._waveFunctions.SizeQueue()==missing);
                std::vector<std::array<float,4>> missingPositions;
                for(uint32 slot=0;slot<2;++slot)if(mask & (1u<<slot))missingPositions.push_back(requested[slot]);
                for(uint32 identity=(wave-1)*3;identity<created;++identity)KillSummon(fixture.actor.adds[identity].get(),&guardian);
                assert(fixture.ai.count_died==created && fixture.ai._waveSummons.empty());
                fixture.actor.adds[(wave-1)*3]->positionX=9000;fixture.actor.adds[(wave-1)*3]->positionZ=-100;
                fixture.actor.failEntry=106578;
                fixture.actor.onSummon=[&fixture](){assert(fixture.actor.m_AI_locked);};
                if(mode){fixture.actor.i_disabledAI=&fixture.ai;fixture.actor.i_AI=mode==1?&replacement:mode==2?&fixture.ai:nullptr;}
                if(mode==3)fixture.actor.IsAIEnabled=false;
                fixture.actor.victim=true;fixture.actor.Lifecycle(999);
                assert(fixture.actor.adds.size()==created && diagnostics==before);
                fixture.actor.Lifecycle(1);assert(diagnostics==before+missing && fixture.ai._waveFunctions.SizeQueue()==missing);
                fixture.actor.Lifecycle(10000);assert(diagnostics==before+missing*2 && fixture.ai._waveFunctions.SizeQueue()==missing);
                fixture.actor.Lifecycle(0);assert(diagnostics==before+missing*2);
                fixture.actor.missingDefender=true;uint32 attempts=fixture.actor.addAttempts;fixture.actor.Lifecycle(1000);
                assert(fixture.actor.addAttempts==attempts && diagnostics==before+missing*3 && fixture.ai._waveFunctions.SizeQueue()==missing);
                fixture.actor.missingDefender=false;fixture.actor.defender=&recoveredDefender;fixture.actor.failEntry=0;
                fixture.player.alive=false;fixture.player.status=0;fixture.player.phase=false;fixture.player.los=false;fixture.player.faction=mode%2;liveMap.players.clear();
                fixture.ai.Reset();fixture.actor.Lifecycle(999);assert(fixture.actor.adds.size()==created);
                fixture.actor.Lifecycle(1);
                assert(fixture.actor.adds.size()==wave*3 && fixture.ai.count_died==created && fixture.ai._waveSummons.size()==missing);
                for(uint32 identity=created;identity<wave*3;++identity)
                {
                    Creature* add=fixture.actor.adds[identity].get();
                    auto const& pose=missingPositions[identity-created];
                    assert(add->entry==106578 && add->positionX==pose[0] && add->positionY==pose[1] && add->positionZ==pose[2] && add->orientation==pose[3]);
                    assert(fixture.actor.addAIs[identity]->attacks==1 && fixture.actor.addAIs[identity]->target==&recoveredDefender);
                }
                assert(fixture.ai._waveFunctions.Size()==0 && fixture.ai._waveFunctions.SizeQueue()==0 && !fixture.actor.m_AI_locked);
                fixture.actor.Lifecycle(10000);assert(fixture.actor.adds.size()==wave*3 && diagnostics==before+missing*3);
                fixture.actor.i_AI=&fixture.ai;fixture.actor.i_disabledAI=nullptr;fixture.actor.IsAIEnabled=true;fixture.actor.onSummon={};
                for(uint32 identity=created;identity<wave*3;++identity)KillSummon(fixture.actor.adds[identity].get(),&guardian);
                assert(fixture.ai.count_died==wave*3 && fixture.actor.adds.size()==(wave==1?6u:7u));
                assert(fixture.player.credits.empty());
            }
    for(uint32 wave:{1u,2u})
        for(uint32 context=0;context<22;++context)
        {
            Fixture fixture;StartPartialWave(fixture,wave,3);
            uint32 created=wave*3-2,before=diagnostics;
            if(context<12)manager.scenario.step=context;
            else switch(context)
            {
                case 12:fixture.actor.world=false;break;
                case 13:fixture.actor.preDelete=true;break;
                case 14:fixture.actor.alive=false;break;
                case 15:fixture.actor.entry=106676;break;
                case 16:fixture.actor.mapId=1220;break;
                case 17:fixture.actor.map=nullptr;break;
                case 18:fixture.actor.instance=43;break;
                case 19:manager.present=false;break;
                case 20:manager.scenario.id=991;break;
                case 21:manager.scenario.id=1037;break;
            }
            fixture.actor.failEntry=106578;fixture.actor.Lifecycle(1000);
            assert(fixture.actor.adds.size()==created && diagnostics==before+2 && fixture.ai._waveFunctions.SizeQueue()==(context==9?2u:0u));
            fixture.actor.world=true;fixture.actor.preDelete=false;fixture.actor.alive=true;fixture.actor.entry=106777;
            fixture.actor.mapId=1539;fixture.actor.map=&liveMap;fixture.actor.instance=42;manager=ScenarioManager{};
            fixture.actor.failEntry=0;fixture.actor.Lifecycle(1000);
            assert(fixture.actor.adds.size()==(context==9?wave*3:created));
        }
    {
        Fixture fixture;Creature guardian;StartPartialWave(fixture,1,3);
        KillSummon(fixture.actor.adds[0].get(),&guardian);
        fixture.actor.onSummonedBy=[&guardian](Creature* summon){KillSummon(summon,&guardian);};
        fixture.actor.Lifecycle(1000);
        assert(fixture.actor.adds.size()==7 && fixture.ai.count_died==6 && fixture.ai._waveSummons.empty());
        assert(fixture.ai._waveFunctions.SizeQueue()==0);fixture.actor.Lifecycle(10000);assert(fixture.actor.adds.size()==7);
    }
    {
        Creature actor,defender;Player player;manager=ScenarioManager{};actor.defender=&defender;actor.failAddMask=3;
        actor.i_AI=new WaveAI(&actor);actor.i_AI->MoveInLineOfSight(&player);actor.stunned=true;actor.Lifecycle(5000);
        uint32 unrelated=0;actor.functions.AddDelayedEvent(1000,[&unrelated](){++unrelated;});
        assert(actor.AIM_Destroy());actor.i_AI=new WaveAI(&actor);actor.IsAIEnabled=true;actor.Run(1000);
        assert(actor.adds.size()==1 && unrelated==1);assert(actor.AIM_Destroy());
    }
    {
        Fixture fixture;Creature guardian;uint32 recovered=0;StartPartialWave(fixture,1,3);
        KillSummon(fixture.actor.adds[0].get(),&guardian);
        fixture.actor.onSummonedBy=[&fixture,&guardian,&recovered](Creature* summon){
            if(++recovered==2)fixture.actor.failSpawn=true;
            KillSummon(summon,&guardian);
        };
        fixture.actor.Lifecycle(1000);
        assert(recovered==2 && fixture.actor.adds.size()==3 && fixture.ai.count_died==3 && fixture.ai._waveSummons.empty());
        assert(fixture.ai._waveFunctions.SizeQueue()==1);
        fixture.actor.failSpawn=false;fixture.actor.onSummonedBy={};fixture.actor.Lifecycle(1000);
        assert(fixture.actor.adds.size()==6 && fixture.ai.count_died==3 && fixture.ai._waveSummons.size()==3 && fixture.ai._waveFunctions.SizeQueue()==0);
    }
    std::cout<<"PASS missing slots: first/second wave either/both106578 failure preserves original leader/other add, immutable pose and original3/6 owned death counts; repeated/null defender/999+1/no lag catch-up/Reset/shared Player loss/faction, primary-disabled-alias-no-primary,22 contexts, destruction/replacement and one-hit guardian reentry recover only absent slots without item/manual credit or populated-wave replay\n";
    for(uint32 wave:{1u,2u,3u})
        for(uint32 mode=0;mode<4;++mode)
        {
            Fixture fixture;CreatureAI replacement;Creature recoveredDefender;
            StartFailedCreation(fixture,wave);
            uint32 originalAdds=(wave-1)*3,before=diagnostics;
            assert(fixture.actor.adds.size()==originalAdds && fixture.ai._waveFunctions.SizeQueue()==1);
            fixture.actor.onSummon=[&fixture](){assert(fixture.actor.m_AI_locked);};
            if(mode){fixture.actor.i_disabledAI=&fixture.ai;fixture.actor.i_AI=mode==1?&replacement:mode==2?&fixture.ai:nullptr;}
            if(mode==3)fixture.actor.IsAIEnabled=false;
            fixture.actor.victim=true;fixture.actor.Lifecycle(999);
            assert(fixture.actor.adds.size()==originalAdds && diagnostics==before);
            fixture.actor.Lifecycle(1);assert(diagnostics==before+1 && fixture.ai._waveFunctions.SizeQueue()==1);
            fixture.actor.Lifecycle(10000);assert(diagnostics==before+2 && fixture.ai._waveFunctions.SizeQueue()==1);
            fixture.actor.Lifecycle(0);assert(diagnostics==before+2);
            fixture.actor.failSpawn=false;fixture.actor.missingDefender=true;fixture.actor.Lifecycle(1000);
            assert(diagnostics==before+3 && fixture.actor.adds.size()==originalAdds && fixture.ai._waveFunctions.SizeQueue()==1);
            fixture.actor.missingDefender=false;fixture.actor.defender=&recoveredDefender;
            fixture.player.alive=false;fixture.player.phase=false;fixture.player.los=false;fixture.player.status=0;fixture.player.faction=mode%2;liveMap.players.clear();
            fixture.ai.Reset();fixture.actor.Lifecycle(999);assert(fixture.actor.adds.size()==originalAdds);
            fixture.actor.Lifecycle(1);
            uint32 total=wave==3?7u:originalAdds+3;
            assert(fixture.actor.adds.size()==total && fixture.ai.count_died==originalAdds && !fixture.actor.m_AI_locked);
            assert(fixture.ai._waveSummons.size()==(wave==3?0u:3u));
            assert(fixture.ai._waveFunctions.Size()==0 && fixture.ai._waveFunctions.SizeQueue()==0);
            for(uint32 identity=originalAdds;identity<total;++identity)
                assert(fixture.actor.addAIs[identity]->attacks==1 && fixture.actor.addAIs[identity]->target==&recoveredDefender);
            if(wave==1)CheckFirstWave(fixture.actor,&recoveredDefender);
            if(wave==3)assert(fixture.actor.adds.back()->entry==106719 && fixture.actor.adds.back()->flags==16 && fixture.actor.addAIs.back()->talks==1);
            fixture.actor.Lifecycle(10000);assert(fixture.actor.adds.size()==total && diagnostics==before+3);
            assert(fixture.ai._introDone && fixture.ai.startCriteria && fixture.player.credits.empty());
            fixture.actor.i_AI=&fixture.ai;fixture.actor.i_disabledAI=nullptr;fixture.actor.IsAIEnabled=true;
        }
    for(uint32 wave:{1u,2u,3u})
        for(uint32 context=0;context<22;++context)
        {
            Fixture fixture;StartFailedCreation(fixture,wave);
            uint32 originalAdds=(wave-1)*3,before=diagnostics;
            if(context<12)manager.scenario.step=context;
            else switch(context)
            {
                case 12:fixture.actor.world=false;break;
                case 13:fixture.actor.preDelete=true;break;
                case 14:fixture.actor.alive=false;break;
                case 15:fixture.actor.entry=106676;break;
                case 16:fixture.actor.mapId=1220;break;
                case 17:fixture.actor.map=nullptr;break;
                case 18:fixture.actor.instance=43;break;
                case 19:manager.present=false;break;
                case 20:manager.scenario.id=991;break;
                case 21:manager.scenario.id=1037;break;
            }
            fixture.actor.Lifecycle(1000);
            assert(fixture.actor.adds.size()==originalAdds && diagnostics==before+1);
            assert(fixture.ai._waveFunctions.Size()==0 && fixture.ai._waveFunctions.SizeQueue()==(context==9?1u:0u));
            fixture.actor.world=true;fixture.actor.preDelete=false;fixture.actor.alive=true;fixture.actor.entry=106777;
            fixture.actor.mapId=1539;fixture.actor.map=&liveMap;fixture.actor.instance=42;manager=ScenarioManager{};
            fixture.actor.failSpawn=false;fixture.actor.Lifecycle(1000);
            assert(fixture.actor.adds.size()==(context==9?(wave==3?7u:originalAdds+3):originalAdds));
            assert(fixture.ai.count_died==originalAdds && fixture.player.credits.empty());
        }
    {
        Fixture fixture;Creature guardian;
        fixture.actor.onSummonedBy=[&guardian](Creature* summon){KillSummon(summon,&guardian);};
        fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.stunned=true;fixture.actor.Lifecycle(5000);
        assert(fixture.actor.adds.size()==7 && fixture.ai.count_died==6 && fixture.ai._waveSummons.empty());
        assert(fixture.ai._waveFunctions.SizeQueue()==0);fixture.actor.Lifecycle(10000);assert(fixture.actor.adds.size()==7);
    }
    for(uint32 wave:{1u,2u,3u})
    {
        Creature actor,defender;Player player;manager=ScenarioManager{};actor.defender=&defender;
        auto* ai=new WaveAI(&actor);actor.i_AI=ai;
        ai->MoveInLineOfSight(&player);actor.stunned=true;
        if(wave==1)actor.failSpawn=true;
        actor.Lifecycle(5000);
        for(uint32 identity=0;identity<(wave-1)*3;++identity)
        {
            if(identity+1==(wave-1)*3)actor.failSpawn=true;
            KillSummon(actor.adds[identity].get(),nullptr);
        }
        assert(ai->_waveFunctions.SizeQueue()==1);
        uint32 unrelated=0;actor.functions.AddDelayedEvent(1000,[&unrelated](){++unrelated;});
        assert(actor.AIM_Destroy());actor.failSpawn=false;actor.i_AI=new WaveAI(&actor);actor.IsAIEnabled=true;
        actor.Run(1000);assert(actor.adds.size()==(wave-1)*3 && unrelated==1);
        assert(actor.AIM_Destroy());
    }
    std::cout<<"PASS zero-creation retry: all three requests, repeated null/999+1/no catch-up, locked primary-disabled-alias-no-primary, fresh defender, Reset and shared player/faction loss,22 contexts and AI destruction/replacement; nonnull immediate owner/guardian deaths do not retry; missing106578 slots checked separately\n";
    for(uint32 wave:{1u,2u,3u})
        for(uint32 mode=0;mode<4;++mode)
        {
            Fixture fixture;CreatureAI replacement;Creature recoveredDefender;
            StartMissingDefender(fixture,wave);
            uint32 originalAdds=wave==1?0u:wave==2?3u:6u;
            uint32 before=diagnostics;
            assert(fixture.actor.adds.size()==originalAdds && fixture.ai._waveFunctions.SizeQueue()==1);
            fixture.actor.onSummon=[&fixture](){assert(fixture.actor.m_AI_locked);};
            if(mode){fixture.actor.i_disabledAI=&fixture.ai;fixture.actor.i_AI=mode==1?&replacement:mode==2?&fixture.ai:nullptr;}
            if(mode==3)fixture.actor.IsAIEnabled=false;
            fixture.actor.victim=true;
            fixture.actor.Lifecycle(999);assert(fixture.actor.adds.size()==originalAdds && diagnostics==before);
            fixture.actor.Lifecycle(1);assert(fixture.actor.adds.size()==originalAdds && diagnostics==before+1);
            assert(fixture.ai._waveFunctions.Size()==0 && fixture.ai._waveFunctions.SizeQueue()==1);
            fixture.actor.Lifecycle(10000);assert(diagnostics==before+2 && fixture.ai._waveFunctions.SizeQueue()==1);
            fixture.actor.Lifecycle(0);assert(diagnostics==before+2);
            fixture.actor.missingDefender=false;fixture.actor.defender=&recoveredDefender;
            fixture.player.alive=false;fixture.player.phase=false;fixture.player.los=false;fixture.player.status=0;fixture.player.faction=mode%2;liveMap.players.clear();
            fixture.ai.Reset();fixture.actor.Lifecycle(999);assert(fixture.actor.adds.size()==originalAdds);
            fixture.actor.Lifecycle(1);
            uint32 total=wave==1?3u:wave==2?6u:7u;
            assert(fixture.actor.adds.size()==total && !fixture.actor.m_AI_locked);
            assert(fixture.ai.count_died==(wave-1)*3 && fixture.ai._waveSummons.size()==(wave==3?0u:3u));
            assert(fixture.ai._waveFunctions.Size()==0 && fixture.ai._waveFunctions.SizeQueue()==0);
            for(uint32 identity=originalAdds;identity<total;++identity)
                assert(fixture.actor.addAIs[identity]->attacks==1 && fixture.actor.addAIs[identity]->target==&recoveredDefender);
            if(wave==1)CheckFirstWave(fixture.actor,&recoveredDefender);
            if(wave==3)assert(fixture.actor.adds.back()->entry==106719 && fixture.actor.adds.back()->flags==16 && fixture.actor.addAIs.back()->talks==1);
            fixture.actor.Lifecycle(10000);assert(fixture.actor.adds.size()==total && diagnostics==before+2);
            assert(fixture.ai._introDone && fixture.ai.startCriteria && fixture.player.credits.empty());
            fixture.actor.i_AI=&fixture.ai;fixture.actor.i_disabledAI=nullptr;fixture.actor.IsAIEnabled=true;
        }
    {
        Fixture fixture;Creature guardian;uint32 created=0;
        fixture.actor.onSummonedBy=[&fixture,&guardian,&created](Creature* summon){
            if(++created==3)fixture.actor.missingDefender=true;
            KillSummon(summon,&guardian);
        };
        fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.stunned=true;fixture.actor.Lifecycle(5000);
        assert(created==3 && fixture.actor.adds.size()==3 && fixture.ai.count_died==3 && fixture.ai._waveSummons.empty());
        assert(fixture.ai._waveFunctions.SizeQueue()==1);
        fixture.actor.onSummonedBy={};fixture.actor.missingDefender=false;fixture.actor.Lifecycle(1000);
        assert(fixture.actor.adds.size()==6 && fixture.ai.count_died==3 && fixture.ai._waveSummons.size()==3);
    }
    for(uint32 wave:{1u,2u,3u})
        for(uint32 context=0;context<22;++context)
        {
            Fixture fixture;StartMissingDefender(fixture,wave);
            uint32 originalAdds=wave==1?0u:wave==2?3u:6u;
            uint32 before=diagnostics;
            if(context<12)manager.scenario.step=context;
            else switch(context)
            {
                case 12:fixture.actor.world=false;break;
                case 13:fixture.actor.preDelete=true;break;
                case 14:fixture.actor.alive=false;break;
                case 15:fixture.actor.entry=106676;break;
                case 16:fixture.actor.mapId=1220;break;
                case 17:fixture.actor.map=nullptr;break;
                case 18:fixture.actor.instance=43;break;
                case 19:manager.present=false;break;
                case 20:manager.scenario.id=991;break;
                case 21:manager.scenario.id=1037;break;
            }
            fixture.actor.Lifecycle(1000);
            assert(fixture.actor.adds.size()==originalAdds && diagnostics==before+1);
            assert(fixture.ai._waveFunctions.Size()==0 && fixture.ai._waveFunctions.SizeQueue()==(context==9?1u:0u));
            fixture.actor.world=true;fixture.actor.preDelete=false;fixture.actor.alive=true;fixture.actor.entry=106777;
            fixture.actor.mapId=1539;fixture.actor.map=&liveMap;fixture.actor.instance=42;manager=ScenarioManager{};
            fixture.actor.missingDefender=false;fixture.actor.Lifecycle(1000);
            assert(fixture.actor.adds.size()==(context==9?(wave==1?3u:wave==2?6u:7u):originalAdds));
            assert(fixture.ai._introDone && fixture.ai.startCriteria && fixture.ai.count_died==(wave-1)*3);
        }
    {
        Creature actor,defender;Player player;manager=ScenarioManager{};actor.defender=&defender;actor.missingDefender=true;
        auto* ai=new WaveAI(&actor);actor.i_AI=ai;ai->MoveInLineOfSight(&player);actor.Lifecycle(5000);
        assert(ai->_waveFunctions.SizeQueue()==1);
        uint32 unrelated=0;actor.functions.AddDelayedEvent(1000,[&unrelated](){++unrelated;});
        assert(actor.AIM_Destroy());actor.Run(1000);assert(actor.adds.empty() && unrelated==1);
    }
    std::cout<<"PASS missing defender: actual three wave requests recover after repeated empty lookup through one AI-owned1000ms job,999+1/no catch-up/Reset/primary-disabled-alias-no-primary lock, fresh defender pointer and unchanged roster/count/flags/attacks; all12 stages/10 actor-scenario failures cancel stale retry, shared Player loss/faction labels retained, AI destruction drops retry without clearing Unit jobs; partial creation remains separate\n";
    for(uint32 threshold:{3u,6u})
        for(uint32 killerKind=0;killerKind<5;++killerKind)
            for(uint32 context=0;context<22;++context)
            {
                Fixture fixture;Player player;Creature pet,guardian,ally;
                std::vector<Unit*> killers{&player,&pet,&guardian,&ally,nullptr};
                Unit* killer=killers[killerKind];
                fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.stunned=true;fixture.actor.Lifecycle(5000);
                for(uint32 identity=0;identity<threshold-1;++identity)KillSummon(fixture.actor.adds[identity].get(),killer);
                fixture.ai.timer=37;
                Creature* final=fixture.actor.adds[threshold-1].get();
                uint32 originalAdds=threshold==3?3u:6u;
                if(context<12)manager.scenario.step=context;
                else switch(context)
                {
                    case 12:fixture.actor.world=false;break;
                    case 13:fixture.actor.preDelete=true;break;
                    case 14:fixture.actor.alive=false;break;
                    case 15:fixture.actor.entry=106676;break;
                    case 16:fixture.actor.mapId=1220;break;
                    case 17:fixture.actor.map=nullptr;break;
                    case 18:fixture.actor.instance=43;break;
                    case 19:manager.present=false;break;
                    case 20:manager.scenario.id=991;break;
                    case 21:manager.scenario.id=1037;break;
                }
                bool valid=context==9;
                bool deferred=context>=12 && context<=14;
                uint32 before=diagnostics;
                KillSummon(final,killer);
                assert(fixture.ai.count_died==(deferred?threshold-1:threshold));
                assert(fixture.actor.adds.size()==(valid?(threshold==3?6u:7u):originalAdds));
                assert(fixture.ai._waveSummons.size()==(deferred?1u:valid && threshold==3?3u:0u));
                assert(diagnostics==before+(valid || deferred?0u:1u));
                assert(fixture.ai._introDone && fixture.ai.startCriteria && fixture.ai.timer==37);
                assert(fixture.ai._waveFunctions.Size()==0 && fixture.ai._waveFunctions.SizeQueue()==0);
                for(uint32 repeat=0;repeat<30;++repeat)RouteDeath(final,killer);
                assert(diagnostics==before+(valid || deferred?0u:1u));
                fixture.actor.world=true;fixture.actor.preDelete=false;fixture.actor.alive=true;fixture.actor.entry=106777;
                fixture.actor.mapId=1539;fixture.actor.map=&liveMap;fixture.actor.instance=42;manager=ScenarioManager{};
                fixture.ai.Reset();RouteDeath(final,killer);
                for(uint32 repeat=0;repeat<30;++repeat)RouteDeath(final,killer);
                assert(fixture.ai.count_died==threshold && fixture.ai.timer==37);
                assert(fixture.actor.adds.size()==(valid || deferred?(threshold==3?6u:7u):originalAdds));
                assert(fixture.player.credits.empty() && player.credits.empty());
            }
    for(uint32 threshold:{3u,6u})
    {
        Fixture fixture;Creature guardian;uint32 progress=179;
        fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.stunned=true;fixture.actor.Lifecycle(5000);
        for(uint32 identity=0;identity<threshold-1;++identity)KillSummon(fixture.actor.adds[identity].get(),&guardian);
        fixture.actor.stunned=false;
        fixture.player.onCredit=[&progress](){if(++progress==180)manager.scenario.step=10;};
        fixture.ai.UpdateAI(1000);KillSummon(fixture.actor.adds[threshold-1].get(),&guardian);
        assert(progress==180 && manager.scenario.step==10 && fixture.ai.count_died==threshold);
        assert(fixture.actor.adds.size()==(threshold==3?3u:6u) && fixture.ai._waveSummons.empty());
        assert(fixture.player.credits.size()==1);
    }
    for(uint32 faction:{0u,1u})
    {
        Fixture fixture;Creature pet;fixture.player.faction=faction;
        fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.stunned=true;fixture.actor.victim=true;fixture.actor.Lifecycle(5000);
        fixture.player.alive=false;fixture.player.phase=false;fixture.player.los=false;fixture.player.status=0;liveMap.players.clear();
        for(uint32 identity=0;identity<6;++identity)KillSummon(fixture.actor.adds[identity].get(),&pet);
        assert(fixture.ai.count_died==6 && fixture.actor.adds.size()==7 && fixture.actor.adds.back()->entry==106719);
        assert(fixture.player.credits.empty());
    }
    std::cout<<"PASS follow-up waves: both3/6 consumed thresholds across all12 stages/10 invalid actor-scenario contexts and Player/pet/guardian/ally/null killer labels; valid9 retained, stale2/Mordoth dropped once without replay after Reset/context recovery, original early invalid-owner deferral retained, no timer/intro/criteria/credit changes;179->180 transition blocks late combat; same-stage Player loss and both faction labels do not cancel shared waves\n";
    assert(!SilverHandFinalEntry::CanStartWave(nullptr));
    for(uint32 mode=0;mode<4;++mode)
        for(uint32 context=0;context<22;++context)
        {
            Fixture fixture;CreatureAI replacement;
            fixture.ai.MoveInLineOfSight(&fixture.player);
            fixture.ai.timer=37;
            if(mode){fixture.actor.i_disabledAI=&fixture.ai;fixture.actor.i_AI=mode==1?&replacement:mode==2?&fixture.ai:nullptr;}
            if(mode==3)fixture.actor.IsAIEnabled=false;
            fixture.actor.Lifecycle(4999);assert(fixture.actor.adds.empty());
            if(context<12)manager.scenario.step=context;
            else switch(context)
            {
                case 12:fixture.actor.world=false;break;
                case 13:fixture.actor.preDelete=true;break;
                case 14:fixture.actor.alive=false;break;
                case 15:fixture.actor.entry=106676;break;
                case 16:fixture.actor.mapId=1220;break;
                case 17:fixture.actor.map=nullptr;break;
                case 18:fixture.actor.instance=43;break;
                case 19:manager.present=false;break;
                case 20:manager.scenario.id=991;break;
                case 21:manager.scenario.id=1037;break;
            }
            bool valid=context==9;
            uint32 before=diagnostics;
            fixture.actor.Lifecycle(1);
            assert(!fixture.actor.m_AI_locked && fixture.ai.timer==37 && fixture.ai.count_died==0);
            assert(fixture.ai._introDone==valid && fixture.ai.startCriteria==valid);
            assert(fixture.actor.adds.size()==(valid?3u:0u) && fixture.ai._waveSummons.size()==(valid?3u:0u));
            assert(diagnostics==before+(valid?0u:1u));
            assert(fixture.ai._waveFunctions.Size()==0 && fixture.ai._waveFunctions.SizeQueue()==0);
            fixture.actor.Lifecycle(5000);assert(diagnostics==before+(valid?0u:1u));
            fixture.actor.i_AI=&fixture.ai;fixture.actor.i_disabledAI=nullptr;fixture.actor.IsAIEnabled=true;
            fixture.actor.world=true;fixture.actor.preDelete=false;fixture.actor.alive=true;fixture.actor.entry=106777;
            fixture.actor.mapId=1539;fixture.actor.map=&liveMap;fixture.actor.instance=42;manager=ScenarioManager{};
            fixture.ai.Reset();fixture.ai.MoveInLineOfSight(&fixture.player);fixture.ai.MoveInLineOfSight(&fixture.player);
            fixture.actor.Lifecycle(4999);assert(fixture.actor.adds.size()==(valid?3u:0u));
            fixture.actor.Lifecycle(1);CheckFirstWave(fixture.actor,&fixture.defender);
            assert(fixture.actor.casts.size()==(valid?1u:2u) && fixture.actor.talks.size()==(valid?1u:2u));
            assert(fixture.ai._introDone && fixture.ai.startCriteria && fixture.ai.timer==37 && fixture.ai.count_died==0);
            assert(fixture.player.credits.empty());
        }
    {
        Fixture fixture;uint32 progress=179;
        fixture.player.onCredit=[&progress](){if(++progress==180)manager.scenario.step=10;};
        fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.Run(1000);
        assert(progress==180 && manager.scenario.step==10 && fixture.actor.adds.empty());
        fixture.actor.Run(4000);fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.Run(5000);
        assert(fixture.actor.adds.empty() && !fixture.ai._introDone && !fixture.ai.startCriteria);
        assert(fixture.actor.casts.size()==1 && fixture.player.credits.size()==1);
    }
    for(uint32 faction:{0u,1u})
    {
        Fixture fixture;fixture.player.faction=faction;
        fixture.ai.MoveInLineOfSight(&fixture.player);
        fixture.player.alive=false;fixture.player.los=false;fixture.player.phase=false;fixture.player.status=0;
        liveMap.players.clear();fixture.actor.stunned=true;fixture.actor.victim=true;
        fixture.actor.Lifecycle(5000);CheckFirstWave(fixture.actor,&fixture.defender);
        assert(fixture.ai._introDone && fixture.ai.startCriteria && fixture.player.credits.empty());
    }
    std::cout<<"PASS pending first wave: all12 stages/10 invalid actor-scenario contexts across primary/disabled/alias/no-primary locked lifecycle,4999+1ms, single diagnostic/drop, preserved timer/roster/count, Reset/fresh eligible retry and no stage10 restart;179->180 progress transition drops stale wave; same-stage Player death/LOS/quest loss/no references/stun/victim do not cancel existing wave\n";
    for(uint32 invalid=0;invalid<15;++invalid)
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
            case 6:fixture.visitor.klass=5;break;
            case 7:fixture.visitor.map=&otherMap;break;
            case 8:fixture.visitor.instance=43;break;
            case 9:fixture.visitor.phase=false;break;
            case 10:fixture.visitor.visible=false;break;
            case 11:fixture.visitor.los=false;break;
            case 12:fixture.visitor.distance=100.01f;break;
            case 13:fixture.visitor.rewarded=true;break;
            case 14:fixture.visitor.status=0;break;
        }
        fixture.actor.nearby=&fixture.visitor;
        liveMap.players={{nullptr},{&fixture.visitor},{&fixture.player},{&fixture.player}};
        fixture.ai.startCriteria=true;
        fixture.ai.UpdateAI(999);assert(fixture.player.credits.empty());
        fixture.ai.UpdateAI(1);assert(fixture.player.credits==std::vector<uint32>{51838} && fixture.visitor.credits.empty());
        fixture.ai.UpdateAI(0);assert(fixture.player.credits.size()==1);
    }
    for(uint32 step=0;step<12;++step)
    {
        Fixture fixture;fixture.ai.startCriteria=true;manager.scenario.step=step;
        fixture.ai.UpdateAI(1000);
        assert(fixture.player.credits.size()==(step==9?1u:0u));
    }
    for(uint32 invalid=0;invalid<9;++invalid)
    {
        Fixture fixture;fixture.ai.startCriteria=true;
        switch(invalid)
        {
            case 0:fixture.actor.world=false;break;
            case 1:fixture.actor.preDelete=true;break;
            case 2:fixture.actor.alive=false;break;
            case 3:fixture.actor.entry=106676;break;
            case 4:fixture.actor.mapId=1;break;
            case 5:fixture.actor.map=nullptr;break;
            case 6:manager.present=false;break;
            case 7:manager.scenario.id=991;break;
            case 8:manager.scenario.id=1037;break;
        }
        fixture.ai.UpdateAI(1000);assert(fixture.player.credits.empty());
    }
    {
        Fixture fixture;
        fixture.ai.UpdateAI(1000);assert(fixture.player.credits.empty());
        fixture.ai.startCriteria=true;
        liveMap.players.clear();fixture.ai.UpdateAI(5000);assert(fixture.player.credits.empty());
        liveMap.players={{&fixture.player},{&fixture.visitor}};
        fixture.ai.UpdateAI(999);assert(fixture.player.credits.empty());
        fixture.ai.UpdateAI(1);assert(fixture.player.credits.size()==1 && fixture.visitor.credits.empty());
        fixture.ai.UpdateAI(5000);assert(fixture.player.credits.size()==2);
        fixture.actor.stunned=true;fixture.ai.UpdateAI(1000);assert(fixture.player.credits.size()==2);
        fixture.actor.stunned=false;fixture.actor.victim=true;fixture.ai.UpdateAI(1000);assert(fixture.player.credits.size()==2);
        fixture.actor.victim=false;fixture.player.phase=false;fixture.visitor.status=0;
        fixture.ai.UpdateAI(1000);assert(fixture.player.credits.size()==2);
        fixture.player.phase=true;fixture.player.faction=1;
        fixture.ai.UpdateAI(1000);assert(fixture.player.credits.size()==3);
        fixture.player.onCredit=[](){manager.scenario.step=10;};
        fixture.ai.UpdateAI(1000);assert(fixture.player.credits.size()==4);
        fixture.ai.UpdateAI(1000);assert(fixture.player.credits.size()==4);
        assert(fixture.actor.adds.empty() && fixture.actor.casts.empty());
    }
    std::cout<<"PASS scoped ritual pulses: null/15 invalid references bypassed, one actual-map eligible recipient per unchanged interval, nine invalid actors/scenarios/all12 stages, no backfill, stun/victim, phase recovery/faction and stage10 stop; no wave/item/manual group fanout\n";
#endif
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
#ifdef BASELINE
        assert(fixture.actor.adds.empty());
#else
        CheckFirstWave(fixture.actor,&fixture.defender);
#endif
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
    std::cout<<"PASS: whole production last Travard AI/native FunctionProcessor and locked primary/disabled lifecycle, 4999+1ms, one queued wave, destruction/replacement/fresh retry, Reset preservation, summon failure/missing defender, original three-wave/flags/attacks/event IDs; ritual recipient admission checked separately; world services controlled, not client traversal\n";
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
}
