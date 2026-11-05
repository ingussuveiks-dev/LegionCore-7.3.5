#include <cassert>
#include <cstdint>
#include <iostream>
#include <atomic>
#include <mutex>
#include <memory>
#include <vector>
#include <map>
using uint32=uint32_t;
struct ObjectGuid {uint64_t high=0,low=0;bool operator==(ObjectGuid other)const{return high==other.high&&low==other.low;}bool operator!=(ObjectGuid other)const{return !(*this==other);}bool operator<(ObjectGuid other)const{return high<other.high||(high==other.high&&low<other.low);}};
#include "QuestScope.inc"
struct WorldObject {
    uint32 map=1607;bool world=true,preDelete=false,expired=false,range=true,phase=true,phaseId=true,target=true;
    uint32 GetMapId()const{return map;}bool IsInWorld()const{return world;}bool IsPreDelete()const{return preDelete;}
    bool IsQuestObjectExpired()const{return expired;}float GetVisibilityRange()const{return 100;}
    bool IsWithinDistInMap(WorldObject*,float)const{return range;}
    bool InSamePhase(WorldObject*)const{return phase;}bool InSamePhaseId(WorldObject*)const{return phaseId;}
    bool CanTargetQuestObject(WorldObject*)const{return target;}
};
struct Creature:WorldObject {bool alive=true;bool IsAlive()const{return alive;}uint32 entry=105660;ObjectGuid guid{9,11};uint32 GetEntry()const{return entry;}ObjectGuid GetGUID()const{return guid;}};
using Unit=Creature;static uint32 clockMs=0;static uint32 getMSTime(){return clockMs;}
struct ObjectAccessor {
    inline static Creature* source=nullptr;
    static Unit* GetUnit(WorldObject const& s,ObjectGuid guid){return GetCreature(s,guid);}
    // The actual native vector accessor indexes the low word; the scoped
    // resolver must itself reject a different high word at both entry points.
    static Creature* GetCreature(WorldObject const& searcher,ObjectGuid guid){return source&&uint32(source->guid.low)==uint32(guid.low)&&source->map==searcher.map?source:nullptr;}
};
struct Conversation:WorldObject {
    bool published=false,removed=false,updated=false;Creature* scopeSource=nullptr;
    uint32 _createdAt=0;std::map<ObjectGuid,uint32> playing;ObjectGuid caster{9,11};ObjectGuid GetCasterGUID()const{return caster;}uint32 GetDuration()const{return 7800;}
    virtual ~Conversation()=default;
    virtual Creature* FindCreatureActor(uint32,uint32,WorldObject*)=0;
    virtual void AddToWorld(){published=true;}
    virtual void Update(uint32){updated=true;}
    void Remove(){removed=true;}
    bool InheritQuestObjectScope(Creature* actor){if(actor->expired)return false;scopeSource=actor;return true;}
};
#include "DevourerOpeningActor.inc"
int main(){
    Creature source;WorldObject caster;ObjectAccessor::source=&source;uint32 cases=0;
    for(uint32 variant=0;variant<20;++variant){
        source=Creature{};caster=WorldObject{};ObjectAccessor::source=&source;
        ObjectGuid exact=source.guid;uint32 index=0,entry=105660;WorldObject* searcher=&caster;bool expected=false;
        switch(variant){
        case 0:expected=true;break;
        case 1:++exact.low;break;case 2:++exact.high;break;case 3:ObjectAccessor::source=nullptr;break;
        case 4:index=1;break;case 5:entry=108214;break;case 6:source.entry=108214;break;
        case 7:caster.map=1500;break;case 8:searcher=nullptr;break;case 9:source.world=false;break;
        case 10:source.preDelete=true;break;case 11:source.expired=true;break;case 12:caster.expired=true;break;
        case 13:caster.range=false;break;case 14:caster.phase=false;break;case 15:caster.phaseId=false;break;
        case 16:source.target=false;break;case 17:caster.target=false;break;case 19:source.alive=false;break;case 18:exact.low^=uint64_t(1)<<40;break;
        }
        OpeningConversation bound(exact,{});Conversation& base=bound;
        assert(base.FindCreatureActor(index,entry,searcher)==(expected?&source:nullptr));++cases;
    }
    source=Creature{};ObjectAccessor::source=&source;
    {OpeningConversation bound(source.guid,{});bound.AddToWorld();assert(bound.published&&bound.scopeSource==&source);++cases;}
    {OpeningConversation bound(source.guid,{});source.expired=true;bound.AddToWorld();assert(!bound.published&&!bound.scopeSource);++cases;}
    {OpeningConversation bound(source.guid,{});ObjectAccessor::source=nullptr;bound.AddToWorld();assert(!bound.published&&!bound.scopeSource);++cases;}
    for(uint32 variant=0;variant<5;++variant){source=Creature{};ObjectAccessor::source=&source;auto guid=source.guid;
        if(variant==0)++guid.high;if(variant==1)source.entry=105542;if(variant==2)source.world=false;if(variant==3)source.preDelete=true;
        if(variant==4)guid.low^=uint64_t(1)<<40;
        OpeningConversation bound(guid,{});bound.AddToWorld();assert(!bound.published&&!bound.scopeSource);++cases;
    }
    source=Creature{};ObjectAccessor::source=&source;
    {source.alive=false;OpeningConversation bound(source.guid,{});bound.AddToWorld();assert(!bound.published);++cases;source.alive=true;}
    {auto attempt=std::make_shared<QuestObjectScope>();OpeningConversation live(source.guid,attempt->CancellationReceipt());live.AddToWorld();assert(live.published);++cases;live.Update(1);assert(live.updated&&!live.removed);++cases;attempt->Cancel();live.Update(0);assert(live.removed);++cases;OpeningConversation cancelled(source.guid,attempt->CancellationReceipt());cancelled.AddToWorld();assert(!cancelled.published&&!cancelled.scopeSource);++cases;}
    for(uint32 mode=0;mode<5;++mode){source=Creature{};ObjectAccessor::source=&source;clockMs=7801;OpeningConversation value(source.guid,{});if(mode==0)value.playing[{8,12}]=clockMs-1;if(mode==1)value.playing[value.caster]=clockMs-1;if(mode==2)value.caster.high=10;if(mode==3)ObjectAccessor::source=nullptr;if(mode==4)value.playing[{8,12}]=0;value.Update(0);assert(value.removed==(mode>=2));if(mode==0){clockMs+=7801;value.Update(0);assert(value.removed);}++cases;}
    std::cout<<"PASS "<<cases<<" ASAN complete exact live1942 subtype/publication with actual cancellation receipt: fullGUID/source/index/entry/map/world/alive/phase/range/reciprocal scope and cancelled receipt; native separate\n";
}
