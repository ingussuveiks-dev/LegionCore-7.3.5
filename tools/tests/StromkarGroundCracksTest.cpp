// Production projection policy/controller with controlled map/allocation sinks.
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <list>
#include <map>
using uint32=uint32_t; using int32=int32_t; using uint8=uint8_t;
#pragma pack(push,1)
#include "GroundTypes.inc"
#pragma pack(pop)
#include "GroundData.inc"
struct ObjectGuid { uint64_t id=0; void Clear(){id=0;} bool operator==(ObjectGuid b)const{return id==b.id;} bool operator<(ObjectGuid b)const{return id<b.id;} static const ObjectGuid Empty; };
const ObjectGuid ObjectGuid::Empty{};
struct Position { float x=1882.18f,y=2163.35f,z=35.9247f,o=2.00205f; };
struct Unit{}; struct Creature; struct AreaTrigger;
struct Map {std::map<ObjectGuid,AreaTrigger*> areas; AreaTrigger* GetAreaTrigger(ObjectGuid g){auto i=areas.find(g);return i==areas.end()?nullptr:i->second;}} world;
struct Scenario {uint32 id=1037,step=7;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}} worldScenario;
struct Manager {bool missing=false;Scenario*GetScenario(uint32){return missing?nullptr:&worldScenario;}} manager;auto*sScenarioMgr=&manager;
struct Creature:Position {bool live=true,in=true;uint32 entry=103398,db=370991,map=1539;ObjectGuid guid{42};bool IsInWorld(){return in;}bool IsAlive(){return live;}uint32 GetEntry(){return entry;}uint32 GetDBTableGUIDLow(){return db;}uint32 GetMapId(){return map;}uint32 GetInstanceId(){return 1;}Map*GetMap(){return &world;}ObjectGuid GetGUID(){return guid;}Position GetPosition(){return *this;}};
template<class T>struct Store {T row{};bool missing=false;T const*LookupEntry(uint32 id){assert(id==10000);return missing?nullptr:&row;}};
Store<DecalPropertiesEntry>sDecalPropertiesStore;Store<TextureBlendSetEntry>sTextureBlendSetStore;
enum class HighGuid{AreaTrigger};
struct Generator {uint64_t id=100;uint64_t Generate(){return ++id;}} generator;
struct Objects {template<HighGuid>Generator*GetGenerator(){return &generator;}} objectManager;auto*sObjectMgr=&objectManager;
bool failCreate=false;uint32 creates=0,deletes=0,warnings=0;
enum {AREATRIGGER_FIELD_DURATION,AREATRIGGER_FIELD_TIME_TO_TARGET_SCALE};
#define TC_LOG_WARN(...) (++warnings)
struct AreaTrigger {ObjectGuid guid,caster;uint32 custom=0;bool world=true,removed=false,m_isUpdate=true,_on_despawn=false;Position pos;int32 _duration=-1;uint32 fields[3]{};~AreaTrigger(){++deletes;}
 int32 GetDuration(){return _duration;}void SetUInt32Value(uint32 field,uint32 value){fields[field]=value;}
 void NativeDurationFields(){
#include "NativeDurationFields.inc"
 }
 void NativeExpiration(uint32 p_time){
#include "NativeExpiration.inc"
 }
 ObjectGuid GetGUID(){return guid;}ObjectGuid GetCasterGUID(){return caster;}uint32 GetCustomEntry(){return custom;}bool IsInWorld(){return world;}
 bool CreateAreaTrigger(uint64_t id,uint32 trigger,Creature*source,void*spell,Position const&p,Position const&move,void*,ObjectGuid target,uint32 entry){
  assert(!trigger&&!spell&&!target.id&&entry==900001);assert(!std::memcmp(&p,&move,sizeof p));++creates;if(failCreate)return false;guid={id};caster=source->guid;custom=entry;pos=p;::world.areas[guid]=this;return true;
 }
 void Remove(bool=false){removed=true;world=false;}
};
struct AreaTriggerAI {explicit AreaTriggerAI(AreaTrigger*){}virtual~AreaTriggerAI()=default;virtual void CalculateDuration(int32&){}virtual bool CreateUnitList(std::list<Unit*>&){return false;}};
#include "GroundController.inc"
void Data(){
 sDecalPropertiesStore={};sTextureBlendSetStore={};
 sDecalPropertiesStore.row={10000,0,.5f,.5f,0,4,4,1,.5f,.5f,75,0,10000,10000,0,0,0};
 auto&t=sTextureBlendSetStore.row;t.ID=10000;t.TextureFileDataID[0]=797909;t.TextureFileDataID[1]=t.TextureFileDataID[2]=1002607;
 for(int i=0;i<3;++i)t.TextureScaleU[i]=t.TextureScaleV[i]=1;
 for(float&v:t.ModX)v=4;t.SwizzleGreen=1;t.SwizzleBlue=2;t.SwizzleAlpha=3;
}
void Reset(){for(auto const&pair:world.areas)delete pair.second;world.areas.clear();worldScenario={};manager={};Data();failCreate=false;creates=deletes=warnings=0;}
int main(){
 static_assert(sizeof(DecalPropertiesEntry)==62&&sizeof(TextureBlendSetEntry)==84);
 Reset();assert(StromkarGroundCrackData::Supported(&sDecalPropertiesStore.row,&sTextureBlendSetStore.row));
 // Every byte belongs to a checked packed field, including all texture/UV channels.
 for(size_t i=0;i<62+84;++i){Data();auto*p=i<62?reinterpret_cast<uint8*>(&sDecalPropertiesStore.row)+i:reinterpret_cast<uint8*>(&sTextureBlendSetStore.row)+i-62;*p^=1;assert(!StromkarGroundCrackData::Supported(&sDecalPropertiesStore.row,&sTextureBlendSetStore.row));}
 assert(!StromkarGroundCrackData::Supported(nullptr,&sTextureBlendSetStore.row));
 for(uint32 bad=0;bad<10;++bad){Reset();Creature c;StromkarGroundCracks::State s;if(bad==0)c.in=false;if(bad==1)c.live=false;if(bad==2)c.map=1;if(bad==3)c.entry=1;if(bad==4)c.db=1;if(bad==5)worldScenario.id=1;if(bad==6)worldScenario.step=8;if(bad==7)manager.missing=true;if(bad==8)sDecalPropertiesStore.missing=true;if(bad==9)sTextureBlendSetStore.missing=true;StromkarGroundCracks::Update(&c,0,2000,s);assert(!creates&&!s.area.id);}
 Reset();{Creature c;StromkarGroundCracks::State s;StromkarGroundCracks::Update(&c,0,2000,s);auto*a=world.GetAreaTrigger(s.area);assert(a&&creates==1&&!std::memcmp(&a->pos,&c,sizeof(Position)));StromkarGroundCracks::Update(&c,500,1500,s);assert(creates==1);StromkarGroundCracks::Update(&c,1500,0,s);assert(a->removed&&!s.area.id);StromkarGroundCracks::Clear(&c,s);assert(creates==1);}
 Reset();{Creature c;StromkarGroundCracks::State s;failCreate=true;StromkarGroundCracks::Update(&c,0,2000,s);assert(creates==1&&deletes==1&&warnings==1);StromkarGroundCracks::Update(&c,499,1501,s);assert(creates==1);StromkarGroundCracks::Update(&c,1,1500,s);assert(creates==2&&warnings==1);failCreate=false;StromkarGroundCracks::Update(&c,500,1000,s);assert(creates==3&&s.area.id);StromkarGroundCracks::Update(&c,1000,0,s);assert(world.GetAreaTrigger({generator.id})->removed);}
 for(uint32 bad=0;bad<3;++bad){Reset();Creature c;StromkarGroundCracks::State s;StromkarGroundCracks::Update(&c,0,2000,s);auto*a=world.GetAreaTrigger(s.area);if(bad==0)a->caster={99};if(bad==1)a->custom=1;if(bad==2)worldScenario.step=8;StromkarGroundCracks::Clear(&c,s);assert(a->removed==(bad==2)&&!s.area.id);}
 Reset();{Creature c;StromkarGroundCracks::State s;StromkarGroundCracks::Update(&c,0,2000,s);auto*a=world.GetAreaTrigger(s.area);worldScenario.step=8;StromkarGroundCracks::Update(&c,1,1999,s);assert(a->removed);}
 {AreaTrigger area;at_stromkar_ground_cracks ai(&area);ai.CalculateDuration(area._duration);assert(area._duration==2000);area.NativeDurationFields();assert(area.fields[0]==2000&&area.fields[1]==2000);area.NativeExpiration(1999);assert(area._duration==1&&!area.removed);area.NativeExpiration(1);assert(area.removed&&!area.m_isUpdate);Unit unrelated;std::list<Unit*> units{&unrelated};assert(ai.CreateUnitList(units)&&units.empty());}
 Reset();std::cout<<"PASS original packed62/84-byte records:146 one-byte corruption refusals,10 scope/data refusals, same real pose, one create, bounded retry/cleanup, own-object isolation, two-second duration and no affected units. Map/allocation endpoints controlled; rendering remains a client check.\n";
}
