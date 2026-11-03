#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;
struct ObjectGuid {uint64_t high=0,low=0;bool operator==(ObjectGuid other)const{return high==other.high&&low==other.low;}};
struct WorldObject {
    uint32 map=1620;bool world=true,preDelete=false,expired=false,range=true,phase=true,phaseId=true,target=true;
    uint32 GetMapId()const{return map;}bool IsInWorld()const{return world;}bool IsPreDelete()const{return preDelete;}
    bool IsQuestObjectExpired()const{return expired;}float GetVisibilityRange()const{return 100;}
    bool IsWithinDistInMap(WorldObject*,float)const{return range;}
    bool InSamePhase(WorldObject*)const{return phase;}bool InSamePhaseId(WorldObject*)const{return phaseId;}
    bool CanTargetQuestObject(WorldObject*)const{return target;}
};
struct Creature:WorldObject {uint32 entry=108218;ObjectGuid guid{9,11};uint32 GetEntry()const{return entry;}};
struct ObjectAccessor {
    inline static Creature* source=nullptr;
    static Creature* GetCreature(WorldObject const& searcher,ObjectGuid guid){return source&&source->guid==guid&&source->map==searcher.map?source:nullptr;}
};
struct Conversation:WorldObject {
    bool published=false;Creature* scopeSource=nullptr;
    virtual ~Conversation()=default;
    virtual Creature* FindCreatureActor(uint32,uint32,WorldObject*)=0;
    virtual void AddToWorld(){published=true;}
    bool InheritQuestObjectScope(Creature* actor){if(actor->expired)return false;scopeSource=actor;return true;}
};
#include "AltheaActor.inc"
int main(){
    Creature source;WorldObject caster;ObjectAccessor::source=&source;uint32 cases=0;
    for(uint32 variant=0;variant<18;++variant){
        source=Creature{};caster=WorldObject{};ObjectAccessor::source=&source;
        ObjectGuid exact=source.guid;uint32 index=0,entry=108218;WorldObject* searcher=&caster;bool expected=false;
        switch(variant){
        case 0:expected=true;break;
        case 1:++exact.low;break;case 2:++exact.high;break;case 3:ObjectAccessor::source=nullptr;break;
        case 4:index=1;break;case 5:entry=108214;break;case 6:source.entry=108214;break;
        case 7:caster.map=1500;break;case 8:searcher=nullptr;break;case 9:source.world=false;break;
        case 10:source.preDelete=true;break;case 11:source.expired=true;break;case 12:caster.expired=true;break;
        case 13:caster.range=false;break;case 14:caster.phase=false;break;case 15:caster.phaseId=false;break;
        case 16:source.target=false;break;case 17:caster.target=false;break;
        }
        AltheaDeathConversation bound(exact);Conversation& base=bound;
        assert(base.FindCreatureActor(index,entry,searcher)==(expected?&source:nullptr));++cases;
    }
    source=Creature{};ObjectAccessor::source=&source;
    {AltheaDeathConversation bound(source.guid);bound.AddToWorld();assert(bound.published&&bound.scopeSource==&source);++cases;}
    {AltheaDeathConversation bound(source.guid);source.expired=true;bound.AddToWorld();assert(!bound.published&&!bound.scopeSource);++cases;}
    {AltheaDeathConversation bound(source.guid);ObjectAccessor::source=nullptr;bound.AddToWorld();assert(!bound.published&&!bound.scopeSource);++cases;}
    std::cout<<"PASS "<<cases<<" ASAN actual full scoped Althea resolver/AddToWorld contexts: full GUID, slot/entry/map, source lifetime, range/phase/phase-ID, reciprocal quest scope and source cancellation before publication; controlled service boundaries, no client claim\n";
}
