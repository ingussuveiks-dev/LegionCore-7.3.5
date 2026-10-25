#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;
using uint64=uint64_t;
using int8=int8_t;
using Seconds=std::chrono::seconds;
#include "CastResults.inc"
enum { NPC_HIPPOGRYPH_DALARAN_SUM=105886, NPC_HIPPOGRYPH_SCENARIO_SUM=106205 };
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct ObjectGuid
{
    uint32 value=0;
    bool IsEmpty() const { return value==0; }
    explicit operator bool() const { return !IsEmpty(); }
};
struct Player;
struct Creature;
struct TempSummon;
struct Vehicle;
struct Unit
{
    virtual ~Unit()=default;
    virtual Player* ToPlayer() { return nullptr; }
    virtual Creature* ToCreature() { return nullptr; }
    bool IsCreature() { return ToCreature()!=nullptr; }
    bool summoned=false;
    bool isSummon() const { return summoned; }
    Vehicle* m_vehicle=nullptr;
    Unit* GetVehicleBase() const;
    SpellCastResult result=SPELL_CAST_OK;
    uint32 casts=0;
    Unit* lastTarget=nullptr;
    std::function<void()> castHook;
    SpellCastResult CastSpell(Unit* target,uint32 spell,bool triggered) { assert(spell==52391 && triggered);lastTarget=target;++casts;if(castHook)castHook();return result; }
#include "ToTemp.inc"
#include "ToTempConst.inc"
};
struct Player : Unit { Player* ToPlayer() override { return this; } };
struct Vehicle
{
    struct Seat { struct { ObjectGuid Guid; } Passenger; };
    std::map<int8,Seat> Seats;
    Unit* base=nullptr;
    Unit* GetBase() const { return base; }
    bool HasEmptySeat(int8) const;
};
struct BaseAI
{
    virtual ~BaseAI()=default;
    virtual void IsSummonedBy(Unit*)=0;
    uint32 justSummoned=0;
    virtual void JustSummoned(Creature*) { ++justSummoned; }
};
struct Creature : Unit
{
    Creature* ToCreature() override { return this; }
    bool IsAIEnabled=true,m_despawn=false,inWorld=true;
    uint32 entry=0,map=0,queued=0,removed=0,delay=0;
    BaseAI* ai=nullptr;
    Vehicle* kit=nullptr;
    BaseAI* AI() { return ai; }
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return map; }
    bool IsInWorld() const { return inWorld; }
    Vehicle* GetVehicleKit() { return kit; }
    void DespawnOrUnsummon(uint32,Seconds const& forceRespawnTimer=Seconds(0));
    void ForcedDespawn(uint32,Seconds const&) { assert(false); }
};
struct TempSummon : Creature
{
    ObjectGuid m_summonerGUID;
    Unit* GetSummoner() const;
    void InitSummon();
    void UnSummon(uint32 delayMs=0) { if(delayMs){assert(delayMs==1);++queued;delay=delayMs;}else{++removed;} }
};
namespace ObjectAccessor
{
    std::map<uint32,Unit*> objects;
    Unit* GetUnit(TempSummon const&,ObjectGuid identity) { auto found=objects.find(identity.value); return found==objects.end()?nullptr:found->second; }
}
struct ForcedUnsummonDelayEvent
{
    TempSummon& m_owner;
    bool Execute(uint64,uint32);
};
#include "Summoner.inc"
#include "VehicleBase.inc"
#include "EmptySeat.inc"
#include "Despawn.inc"
#include "InitSummon.inc"
#include "Delayed.inc"
struct Script : BaseAI
{
    Creature* me;
    uint32 fly=0;
    explicit Script(Creature* actor):me(actor) { actor->ai=this; }
    void SetFlyMode(bool value) { assert(value);++fly; }
#include "Rider.inc"
#include "Spawn.inc"
};
int main()
{
    uint32 cases=0;
    for(uint32 entry : {105883u,105886u,106203u,106205u,999u})
        for(uint32 map : {1220u,1495u,1u})
            for(bool summoned : {false,true})
                for(uint32 ownerKind=0;ownerKind<4;++ownerKind)
                    for(uint32 seat=0;seat<6;++seat)
                        for(SpellCastResult result : {SPELL_CAST_OK,SPELL_FAILED_ERROR,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_DONT_REPORT})
                        {
                            Player owner,stranger; Unit pet,guardian; Creature npc; Script npcScript(&npc);
                            TempSummon bird; bird.entry=entry;bird.map=map;bird.summoned=summoned;bird.m_summonerGUID={17};
                            Script script(&bird);
                            Vehicle kit;kit.base=&bird;
                            if(seat!=0)bird.kit=&kit;
                            if(seat>=2)kit.Seats.emplace(int8(0),Vehicle::Seat{});
                            if(seat==3)kit.Seats[0].Passenger.Guid={18};
                            if(seat==4){kit.Seats[0].Passenger.Guid={17};owner.m_vehicle=&kit;}
                            if(seat==5)kit.Seats[0].Passenger.Guid={99};
                            Unit* summoner=ownerKind==0?static_cast<Unit*>(&owner):ownerKind==1?&pet:ownerKind==2?&guardian:&npc;
                            ObjectAccessor::objects={{17,summoner},{18,&stranger}};
                            summoner->result=result;warnings=0;
                            bird.InitSummon();
                            assert(summoner->casts==1 && summoner->lastTarget==&bird && script.fly==1 && bird.removed==0);
                            assert(npcScript.justSummoned==uint32(ownerKind==3));
#ifdef BASELINE
                            assert(bird.queued==0 && warnings==0);
#else
                            bool valid=summoned && ownerKind==0 && ((entry==105886 && map==1220)||(entry==106205 && map==1495));
                            bool cleanup=valid && result!=SPELL_CAST_OK && (seat==0 || seat==2);
                            assert(bird.queued==uint32(cleanup) && warnings==uint32(result!=SPELL_CAST_OK));
                            if(cleanup)
                            {
                                assert(bird.delay==1 && bird.m_despawn);
                                bird.InitSummon();assert(bird.queued==1 && summoner->casts==2);
                                ForcedUnsummonDelayEvent event{bird};assert(event.Execute(1,1) && bird.removed==1);
                            }
#endif
                            ++cases;
                        }
#ifndef BASELINE
    for(uint32 entry : {105886u,106205u})
    {
        Player owner,stranger;
        TempSummon bird;bird.entry=entry;bird.map=entry==105886?1220:1495;bird.summoned=true;bird.m_summonerGUID={17};
        Script script(&bird);ObjectAccessor::objects.clear();bird.InitSummon();assert(script.fly==0 && bird.queued==0);
        script.IsSummonedBy(nullptr);assert(script.fly==1 && bird.queued==0);
        ObjectAccessor::objects={{17,&owner}};stranger.result=SPELL_FAILED_ERROR;
        script.IsSummonedBy(&stranger);assert(stranger.casts==1 && bird.queued==0);
        Vehicle kit;kit.base=&bird;kit.Seats.emplace(int8(0),Vehicle::Seat{});bird.kit=&kit;
        owner.result=SPELL_FAILED_ERROR;
        owner.castHook=[&]{owner.m_vehicle=&kit;};
        bird.InitSummon();assert(bird.queued==0);
        owner.m_vehicle=nullptr;owner.castHook={};bird.inWorld=false;
        bird.InitSummon();assert(bird.queued==0 && !bird.m_despawn);
        bird.inWorld=true;bird.InitSummon();assert(bird.queued==1);
    }
#endif
    std::cout<<"PASS: "<<cases<<" extracted summon/result/map/owner/seat cases; ";
#ifdef BASELINE
    std::cout<<"pinned93e6c64c ignores rejected52391 and leaves native summon lifetime unchanged.\n";
#else
    std::cout<<"only rejected original-Player matching unboarded empty-seat summons request1ms native deferred cleanup; occupied/unresolved seat GUID, accepted casts, unknown actors and boarded rider retained. Actual InitSummon, rider/seat helpers, despawn dispatcher and delayed callback compiled; world allocation/queue execution/client remain separate.\n";
#endif
}
