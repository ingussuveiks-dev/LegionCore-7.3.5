#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;
constexpr int QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,SMART_EVENT_OOC_LOS=10;
#include "ConditionValues.inc"
struct Creature;struct CreatureAI;
struct Unit {bool player=false;float distance=0;uint32 GetTypeId(){return player?4:3;}uint32 GetEntry(){return 0;}};
struct Player:Unit {
    bool world=true,alive=true,teleport=false,phase=true,rewarded=false;uint32 map=1498;int quest=QUEST_STATUS_INCOMPLETE;Creature* marker=nullptr;
    Player(){player=true;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}bool IsBeingTeleported(){return teleport;}
    uint32 GetMapId(){return map;}int GetQuestStatus(uint32 id){assert(id==42125);return quest;}
    bool GetQuestRewardStatus(uint32 id){assert(id==42125);return rewarded;}
    Creature* FindNearestCreature(uint32 id,float range){assert(id==99739&&range==30);return distance<=range?marker:nullptr;}
    bool InSamePhase(Creature*){return phase;}
};
struct Creature {
    uint32 db=376351;bool IsAIEnabled=true,combat=false,los=true;CreatureAI* ai=nullptr;
    uint32 GetDBTableGUIDLow(){return db;}CreatureAI* AI(){return ai;}
    bool isInCombat(){return combat;}bool IsWithinDistInMap(Unit* u,float range){return u->distance<=range;}
    bool IsWithinLOSInMap(Unit*){return los;}bool IsHostileTo(Unit*){return false;}
};
bool PlayerCondition(Unit* object){bool condMeets=false;switch(CONDITION_OBJECT_ENTRY){
    #include "PlayerCondition.inc"
}return condMeets;}
struct CreatureAI {
    Creature* me;Player* owner;bool requirePlayer=true,runOnce=false,m_MoveInLineOfSight_locked=false;int step=2,credits=0,waves=0;
    struct Event {struct {struct {uint32 maxDist=30,noHostile=1,cooldownMin=0,cooldownMax=0;} los;} event;} e;
    void RecalcTimer(Event&,uint32,uint32){}
    void ProcessAction(Event&,Unit*){runOnce=true;if(owner->distance<=30){++credits;step=3;}++waves;}
    void MoveInLineOfSight(Unit* unit){
        // Source-22 conditions are admitted before native ProcessAction sets
        // runOnce. The existing stage condition inspects the base actor.
        if(step!=2||runOnce||(requirePlayer&&!PlayerCondition(unit)))return;
        switch(SMART_EVENT_OOC_LOS){
            #include "LosEvent.inc"
        }
    }
    void MoveInLineOfSight_Safe(Unit*);
};
#include "SafeLos.inc"
#include "Arrival.inc"
int main(){
    Player p;Creature marker;CreatureAI ai{&marker,&p};marker.ai=&ai;p.marker=&marker;Unit pet;
    // Baseline: pet reaches the marker first, with its owner >30 yd away.
    // Native LOS consumes the only event and starts waves without credit.
    ai.requirePlayer=false;p.distance=45;ai.MoveInLineOfSight_Safe(&pet);
    assert(ai.runOnce&&ai.credits==0&&ai.waves==1&&ai.step==2);
    p.distance=5;ai.MoveInLineOfSight_Safe(&p);assert(ai.credits==0);
    ai=CreatureAI{&marker,&p};p.distance=45;
    ai.MoveInLineOfSight_Safe(&pet);assert(!ai.runOnce&&!ai.credits&&!ai.waves);
    assert(!DarkWhispersArrival::CheckArrival(&p,1097,2));
    // No new client movement event is supplied after teleporting in place.
    p.distance=5;assert(DarkWhispersArrival::CheckArrival(&p,1097,2));
    assert(ai.step==3&&ai.credits==1&&ai.waves==1&&ai.runOnce);
    for(int i=0;i<20;++i){DarkWhispersArrival::CheckArrival(&p,1097,ai.step);ai.MoveInLineOfSight_Safe(&p);}
    assert(ai.credits==1&&ai.waves==1);
    auto reset=[&]{p=Player{};p.marker=&marker;marker=Creature{};marker.ai=&ai;ai=CreatureAI{&marker,&p};};
    reset();marker.los=false;DarkWhispersArrival::CheckArrival(&p,1097,2);assert(!ai.runOnce);
    marker.los=true;DarkWhispersArrival::CheckArrival(&p,1097,2);assert(ai.credits==1);
    reset();marker.combat=true;DarkWhispersArrival::CheckArrival(&p,1097,2);assert(!ai.runOnce);
    marker.combat=false;DarkWhispersArrival::CheckArrival(&p,1097,2);assert(ai.credits==1);
    for(int branch=0;branch<9;++branch){reset();switch(branch){
        case 0:p.alive=false;break;case 1:p.world=false;break;case 2:p.teleport=true;break;
        case 3:p.map=1220;break;case 4:p.quest=0;break;case 5:p.phase=false;break;
        case 6:marker.db=376353;break;case 7:marker.IsAIEnabled=false;break;case 8:p.rewarded=true;break;
    }assert(!DarkWhispersArrival::CheckArrival(&p,1097,2)&&!ai.runOnce&&!ai.credits);}
    reset();assert(!DarkWhispersArrival::CheckArrival(nullptr,1097,2));
    assert(!DarkWhispersArrival::CheckArrival(&p,998,2));assert(!DarkWhispersArrival::CheckArrival(&p,1097,0));
    p.quest=QUEST_STATUS_COMPLETE;assert(DarkWhispersArrival::CheckArrival(&p,1097,2)&&ai.credits==1);
    std::cout<<"PASS: baseline pet-first consumption reproduced; native Player condition/LOS/safe callback and production arrival retry, stationary teleport, exact-once credit/waves, pending LOS/combat and scoped exclusions\n";
}
