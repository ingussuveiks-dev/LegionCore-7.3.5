#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
#include "FunctionProcessor.inc"
struct ObjectGuid{uint64 high=1,low=1;uint32 GetGUIDLow()const{return uint32(low);}bool operator!=(ObjectGuid const& b)const{return high!=b.high||low!=b.low;}};
struct Map{};
struct WorldObject{Map* map=nullptr;ObjectGuid guid;bool world=true,preDelete=false,alive=true;uint32 phase=1,entry=105540;
 Map* GetMap()const{return map;}ObjectGuid GetGUID()const{return guid;}bool IsInWorld()const{return world;}bool IsPreDelete()const{return preDelete;}bool IsAlive()const{return alive;}uint32 GetEntry()const{return entry;}bool InSamePhase(WorldObject* other)const{return phase==other->phase;}};
struct Player:WorldObject{};
uint32 casts=0,speeches=0,warnings=0;
#define TC_LOG_WARN(...) (++warnings)
enum{TEXT_GENERIC_3=3};
struct Creature:WorldObject{FunctionProcessor functions;Player* nearest=nullptr;
 void AddDelayedEvent(uint64 delay,std::function<void()>&& callback){functions.AddDelayedEvent(delay,std::move(callback));}
 Player* FindNearestPlayer(float range){assert(range==40);return nearest;}
 void CastSpell(Player* target,uint32 spell){assert(target&&spell==209264);++casts;}
};
template<class T>struct HashMapHolder{inline static bool _checkLock=false;inline static std::shared_mutex i_lockVector;inline static uint32 _size=32;inline static std::vector<T*> _objectVector=std::vector<T*>(32,nullptr);
#include "Find.inc"
};
struct ObjectAccessor{
#include "Lookup.inc"
};
#include "CreatureLookup.inc"
struct TextManager{void SendChat(Creature* actor,uint32 group){assert(actor&&group==3);++speeches;}} textManager;
auto sCreatureTextMgr=&textManager;
struct VisualAI{Creature* me;explicit VisualAI(Creature* actor):me(actor){}void Talk(uint32 id){sCreatureTextMgr->SendChat(me,id);}void Schedule(Creature* targ){
#include "Visual.inc"
}};
struct Fixture{Map map,other;Creature parent;Player player;std::unique_ptr<Creature> helper=std::make_unique<Creature>();std::unique_ptr<VisualAI> ai;
 Fixture(){casts=speeches=warnings=0;parent.map=&map;parent.entry=105536;player.map=&map;parent.nearest=&player;helper->map=&map;helper->guid={11,2};HashMapHolder<Creature>::_objectVector.assign(32,nullptr);HashMapHolder<Creature>::_objectVector[2]=helper.get();ai=std::make_unique<VisualAI>(&parent);}
 void Start(){ai->Schedule(helper.get());}void Tick(uint32 ms){parent.functions.Update(ms);}void DropHelper(){HashMapHolder<Creature>::_objectVector[2]=nullptr;helper.reset();}
};
int main(){
#ifdef BASELINE
 Fixture f;f.Start();f.helper->world=false;HashMapHolder<Creature>::_objectVector[2]=nullptr;f.Tick(5000);assert(casts==1&&speeches==1);
 std::cout<<"PASS pinnedc0a7c900: queued5s visual still dereferences/attempts casting on the retired helper retained in memory. Actual freed AI/helper was not dereferenced in baseline. Expected failing lifecycle counterfactual.\n";
#else
 {Fixture f;f.Start();f.Tick(4999);assert(!casts&&!speeches);f.Tick(1);assert(casts==1&&speeches==1&&!warnings);f.Tick(10000);assert(casts==1&&speeches==1);}
 for(int invalid=0;invalid<8;++invalid){Fixture f;f.Start();switch(invalid){case 0:f.DropHelper();break;case 1:f.helper->alive=false;break;case 2:f.helper->world=false;break;case 3:f.helper->preDelete=true;break;case 4:f.helper->map=&f.other;break;case 5:f.helper->phase=2;break;case 6:f.helper->entry=999;break;case 7:f.helper->guid.high=12;break;}f.Tick(5000);assert(!casts&&speeches==1&&warnings==1);f.Tick(5000);assert(warnings==1);}
 {Fixture f;f.Start();f.ai.reset();f.Tick(5000);assert(casts==1&&speeches==1&&!warnings);} // No callback reads the destroyed old AI.
 {Fixture f;f.Start();f.DropHelper();auto replacement=std::make_unique<Creature>();replacement->map=&f.map;replacement->guid={12,2};HashMapHolder<Creature>::_objectVector[2]=replacement.get();f.Tick(5000);assert(!casts&&warnings==1);}
 for(int invalid=0;invalid<4;++invalid){Fixture f;f.Start();if(invalid==0)f.parent.alive=false;if(invalid==1)f.parent.world=false;if(invalid==2)f.parent.entry=999;if(invalid==3)f.parent.preDelete=true;f.Tick(5000);assert(!casts&&!speeches&&!warnings);}
 {Fixture f;f.Start();f.parent.nearest=nullptr;f.Tick(5000);assert(!casts&&speeches==1&&!warnings);}
 {Fixture f;f.Start();f.parent.functions.KillAllFunctions();f.Tick(5000);assert(!casts&&!speeches&&!warnings);}
 {auto parent=std::make_unique<Creature>();auto helper=std::make_unique<Creature>();VisualAI ai(parent.get());ai.Schedule(helper.get());parent.reset();assert(!casts);} // Native member queue destruction drops closures.
 std::cout<<"PASS real FunctionProcessor5s queue + native creature registry/map/predelete lookup, full-GUID/entry/phase/alive/world guards, freed helper/old AI, replacement counter identity, parent retirement/destruction/cancellation, missing player and once-only speech/visual. No progression code exists in this callback;10s transfer/attempt/reset policy remains OPEN.\n";
#endif
}
