#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
using uint32=uint32_t;
using uint8=uint8_t;
using uint64=uint64_t;
struct ObjectGuid {
    uint64 value=0;
    static ObjectGuid Empty;
    bool IsEmpty() const {return !value;}
    bool IsPlayer() const {return value==1;}
    bool IsPet() const {return value==3;}
    uint64 GetCounter() const {return value;}
    explicit operator bool() const {return value!=0;}
    bool operator==(ObjectGuid other) const {return value==other.value;}
    bool operator!=(ObjectGuid other) const {return !(*this==other);}
    bool operator<(ObjectGuid other) const {return value<other.value;}
    void Clear(){value=0;}
};
ObjectGuid ObjectGuid::Empty;
using GuidUnorderedSet=std::set<ObjectGuid>;
enum class HighGuid { GameObject };
enum { GO_STATE_READY=1,MAX_GAMEOBJECT_SLOT=4,SPELL_ATTR0_DISABLED_WHILE_ACTIVE=1,SPELL_ATTR1_IS_FISHING=2 };
struct Position {
    float positionX,positionY,positionZ,orientation;
    Position():positionX(0),positionY(0),positionZ(0),orientation(0){}
    Position(float coordinateX,float coordinateY,float coordinateZ,float facing):positionX(coordinateX),positionY(coordinateY),positionZ(coordinateZ),orientation(facing){}
    float GetPositionX() const{return positionX;}float GetPositionY() const{return positionY;}float GetPositionZ() const{return positionZ;}
    void SimplePosXYRelocationByAngle(Position&,float,float){assert(false);}
};
namespace G3D {struct Quat {Quat(float,float,float,float){}};}
uint32 errors=0,fatals=0,created=0,destroyed=0,staticCreated=0,staticDestroyed=0,cleanups=0;
bool templateAvailable=true,creationAvailable=true,scopeAvailable=true,staticTemplate=false;
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_FATAL(...) (++fatals)
#define ASSERT(value) assert(value)
struct WorldObject;struct Unit;struct GameObject;struct Map;
std::set<GameObject*> allocations;
std::map<uint64,Unit*> units;
struct Transport {
    std::set<WorldObject*> passengers;
    uint32 removed=0;
    void RemovePassenger(WorldObject* passenger);
};
struct WorldObject {
    uint32 kind=0;
    bool world=true;
    Map* map=nullptr;
    ObjectGuid identity{5};
    Transport* transport=nullptr;
    std::set<uint32> m_phaseId{5737},m_phaseBit{4},_terrainSwaps{1},_worldMapAreaSwaps{2};
    GuidUnorderedSet _visibilityPlayerList,_hideForGuid;
    virtual ~WorldObject()=default;
    virtual Unit* ToUnit(){return nullptr;}
    bool IsInWorld() const{return world;}
    bool IsPlayer() const{return kind==1;}
    bool IsCreature() const{return kind>=2 && kind<=4;}
    Map* GetMap() const{return map;}
    ObjectGuid GetGUID() const{return identity;}
    uint32 GetPhaseMask() const{return 4;}
    float GetObjectSize()const{return 1;}
    void GetPosition(float& x,float& y,float& z,float& o){x=1868.87f;y=2198.75f;z=37.3797f;o=0.141143f;}
    void GetNearPoint2D(Position&,float,float){assert(false);}
    Position GetFirstCollisionPosition(float,float){return {1868.87f,2198.75f,37.3797f,0.141143f};}
    std::set<uint32> const& GetPhases() const{return m_phaseId;}
    Transport* GetTransport() const{return transport;}
    void SetTratsport(Transport* value){if(value){transport=value;value->passengers.insert(this);}}
    virtual void RemoveFromWorld(){world=false;}
    virtual void CleanupsBeforeDelete(bool=true);
    GameObject* SummonGameObject(uint32,Position,float,float,float,float,float,uint32,ObjectGuid={},GuidUnorderedSet* = nullptr,bool=true);
    GameObject* SummonGameObject(uint32,float,float,float,float,float,float,float,float,uint32,ObjectGuid={},GuidUnorderedSet* = nullptr,bool=true);
};
void Transport::RemovePassenger(WorldObject* passenger){if(passengers.erase(passenger)){passenger->transport=nullptr;++removed;}}
#include "WorldCleanup.inc"
struct SpellInfo {bool HasAttribute(uint32) const{return false;}};
struct SpellMgr {SpellInfo info;SpellInfo const* GetSpellInfo(uint32){return &info;}} spellManager;
auto* sSpellMgr=&spellManager;
struct Unit:WorldObject {
    std::list<GameObject*> m_gameObj;
    std::array<ObjectGuid,MAX_GAMEOBJECT_SLOT> m_ObjectSlot;
    Unit* ToUnit() override{return this;}
    Unit* ToPlayer(){return this;}
    void AddSpellAndCategoryCooldowns(SpellInfo const*,uint32,void*,bool){assert(false);}
    void RemoveAurasDueToSpell(uint32){assert(false);}
    void SendCooldownEvent(SpellInfo const*){assert(false);}
    void AddGameObject(GameObject*);
    void RemoveGameObject(GameObject*,bool);
    void RemoveAllGameObjects();
};
struct GameObjectTemplate {uint32 entry=247690;uint32 GetLinkedGameObjectEntry() const{return 0;}} objectTemplate;
struct Generator {uint32 identity=100;uint32 Generate(){return ++identity;}} generator;
struct ObjectMgr {
    GameObjectTemplate const* GetGameObjectTemplate(uint32 entry){assert(entry==objectTemplate.entry);return templateAvailable?&objectTemplate:nullptr;}
    bool IsStaticTransport(uint32){return staticTemplate;}
    template<HighGuid> Generator* GetGenerator(){return &generator;}
} objectManager;
auto* sObjectMgr=&objectManager;
struct GameObject:WorldObject {
    ObjectGuid owner,summoner;
    uint32 lifetime=0,m_respawnTime=0,m_respawnDelayTime=0;
    bool m_spawnedByDefault=true,m_uint32Values=false,admissionAttempted=false,deleted=false;
    GameObject(){world=false;++created;allocations.insert(this);}
    ~GameObject() override {assert(!world&&owner.IsEmpty()&&!transport);if(admissionAttempted)assert(m_phaseId.empty()&&m_phaseBit.empty()&&_terrainSwaps.empty()&&_worldMapAreaSwaps.empty()&&_visibilityPlayerList.empty()&&_hideForGuid.empty());allocations.erase(this);++destroyed;}
    bool Create(uint32 low,uint32 entry,Map* target,uint32 phase,Position position,G3D::Quat,uint32 animation,uint32 state){
        assert(entry==objectTemplate.entry&&(phase==4||phase==1));assert(animation==100&&state==GO_STATE_READY);
        assert(position.positionX==1868.87f&&position.positionY==2198.75f&&position.positionZ==37.3797f&&position.orientation==0.141143f);
        identity={low};map=target;m_uint32Values=creationAvailable;return creationAvailable;
    }
    bool InheritQuestObjectScope(WorldObject*){return scopeAvailable;}
    void SetPhaseId(std::set<uint32> const& phases,bool){m_phaseId=phases;}
    void SetSummonerGUID(ObjectGuid value){summoner=value;}
    void SetRespawnTime(uint32 value){lifetime=m_respawnTime=m_respawnDelayTime=value;}
    bool isSpawned()const;
    void SetSpawnedByDefault(bool value){m_spawnedByDefault=value;}
    void AddPlayerInPersonnalVisibilityList(ObjectGuid viewer){_visibilityPlayerList.insert(viewer);}
    void AddPlayersInPersonnalVisibilityList(GuidUnorderedSet const& viewers){_visibilityPlayerList.insert(viewers.begin(),viewers.end());}
    ObjectGuid GetOwnerGUID() const{return owner;}
    void SetOwnerGUID(ObjectGuid value){owner=value;}
    uint32 GetSpellId() const{return 0;}
    uint32 GetGUIDLow() const{return uint32(identity.value);}
    GameObjectTemplate const* GetGOInfo() const{return &objectTemplate;}
    void Delete(){deleted=true;}
    void RemoveFromOwner();
    void CleanupsBeforeDelete(bool=true) override;
};
struct SummonedTransport:GameObject {
    std::set<WorldObject*> _passengers;
    uint32 staticUnloads=0;
    void UnloadStaticPassengers(){++staticUnloads;}
    void RemovePassenger(WorldObject* passenger){assert(_passengers.erase(passenger)==1);}
    void CleanupsBeforeDelete(bool=true) override;
};
#define Transport SummonedTransport
#include "TransportCleanup.inc"
#undef Transport
struct StaticTransport:SummonedTransport {
    WorldObject firstPassenger,secondPassenger;
    StaticTransport(){++staticCreated;_passengers.insert(&firstPassenger);_passengers.insert(&secondPassenger);}
    ~StaticTransport()override{if(admissionAttempted)assert(_passengers.empty()&&staticUnloads==1);++staticDestroyed;}
};
struct ObjectAccessor {static Unit* GetUnit(WorldObject const& object,ObjectGuid identity){auto found=units.find(identity.value);return found!=units.end()&&found->second->GetMap()==object.GetMap()?found->second:nullptr;}};
#include "Ownership.inc"
#include "GameObjectCleanup.inc"
struct Map {
    bool admission=true;
    uint32 attempts=0;
    ObjectGuid observedOwner;
    uint32 observedViewers=0;
    uint32 GetId() const{return 1539;}
    bool AddToMap(GameObject* object){++attempts;object->admissionAttempted=true;observedOwner=object->owner;observedViewers=uint32(object->_visibilityPlayerList.size());if(!admission)return false;object->world=true;return true;}
    GameObject* SummonGameObject(uint32,Position,float,float,float,float,float,uint32);
    GameObject* SummonGameObject(uint32,float,float,float,float,float,float,float,float,uint32,ObjectGuid={},GuidUnorderedSet* = nullptr,bool=true);
};
#include "WorldSummon.inc"
#include "MapSummon.inc"
GameObject* Summon(WorldObject& source,bool creator,uint32 lifetime,GuidUnorderedSet* viewers){return source.SummonGameObject(247690,Position(1868.87f,2198.75f,37.3797f,0.141143f),0.141143f,0,0,0,0,lifetime,{77},viewers,creator);}
void Dispose(GameObject* object){if(object){object->CleanupsBeforeDelete();delete object;}}
void Reset(){assert(allocations.empty());errors=fatals=created=destroyed=staticCreated=staticDestroyed=0;templateAvailable=creationAvailable=scopeAvailable=true;staticTemplate=false;units.clear();}
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);
    bool mapOnly=argc==2&&std::string(argv[1])=="map";
    if(!mapOnly)for(uint32 kind:{0u,1u,2u,3u,4u})for(bool creator:{false,true})for(uint32 lifetime:{0u,3600u})for(bool attachment:{false,true})for(bool special:{false,true})for(bool admitted:{false,true}){
        Reset();Map map;map.admission=admitted;Unit unit;WorldObject other,sentinel;Transport transport;
        unit.kind=kind;unit.map=&map;unit.identity={kind?kind:5};units[unit.identity.value]=&unit;
        other.map=&map;WorldObject& source=kind?static_cast<WorldObject&>(unit):other;
        transport.passengers.insert(&sentinel);if(attachment)source.transport=&transport;
        staticTemplate=special;GuidUnorderedSet viewers{{88},{99}};
        auto* result=Summon(source,creator,lifetime,&viewers);
        assert((result!=nullptr)==admitted);
        bool owned=kind&&creator&&(kind==1||!lifetime);
        assert(map.attempts==1&&fatals==0&&errors==uint32(!admitted));
        assert(map.observedOwner==ObjectGuid{owned?unit.identity.value:0}&&map.observedViewers==3);
        if(admitted){assert(result->world&&result->lifetime==lifetime&&result->summoner==source.identity);assert(result->owner==ObjectGuid{owned?unit.identity.value:0});assert(result->_visibilityPlayerList==GuidUnorderedSet({{77},{88},{99}}));assert(unit.m_gameObj.size()==uint32(owned));assert(result->m_spawnedByDefault==owned);}
        else assert(unit.m_gameObj.empty()&&allocations.empty());
        Dispose(result);assert(allocations.empty()&&unit.m_gameObj.empty()&&created==destroyed&&staticCreated==staticDestroyed);
        assert(transport.passengers==std::set<WorldObject*>{&sentinel});assert(transport.removed==uint32(attachment));assert(source.world&&source.m_phaseId==std::set<uint32>{5737});
    }
    if(!mapOnly){
        Reset();Map map;map.admission=false;Unit source;source.kind=1;source.map=&map;units[source.identity.value]=&source;
        auto* unrelated=new GameObject;unrelated->map=&map;unrelated->identity={1000};unrelated->m_uint32Values=true;
        source.AddGameObject(unrelated);source.m_ObjectSlot[0]=unrelated->identity;
        assert(!Summon(source,true,3600,nullptr)&&source.m_gameObj.size()==1&&source.m_gameObj.front()==unrelated&&source.m_ObjectSlot[0]==unrelated->identity);
        Dispose(unrelated);assert(source.m_gameObj.empty()&&source.m_ObjectSlot[0].IsEmpty()&&allocations.empty());
    }
    for(bool special:{false,true})for(bool admitted:{false,true}){
        Reset();Map map;map.admission=admitted;staticTemplate=special;GuidUnorderedSet viewers{{88}};
        auto* result=map.SummonGameObject(247690,1868.87f,2198.75f,37.3797f,0.141143f,0,0,0,0,3600,{77},&viewers,false);
        assert((result!=nullptr)==admitted&&errors==uint32(!admitted));
        if(result)assert(result->world&&!result->m_spawnedByDefault&&result->owner.IsEmpty()&&result->_visibilityPlayerList==GuidUnorderedSet({{77},{88}}));
        Dispose(result);assert(allocations.empty()&&created==destroyed&&staticCreated==staticDestroyed&&fatals==0);
    }
    for(bool admitted:{false,true}){
        Reset();Map map;map.admission=admitted;
        auto* result=map.SummonGameObject(247690,Position(1868.87f,2198.75f,37.3797f,0.141143f),0.141143f,0,0,0,0,3600);
        assert((result!=nullptr)==admitted&&map.attempts==1&&!map.observedViewers&&map.observedOwner.IsEmpty());
        Dispose(result);assert(allocations.empty()&&created==destroyed);
    }
    for(uint32 failure=0;failure<4;++failure){
        Reset();Map map;Unit source;source.kind=1;source.map=&map;units[source.identity.value]=&source;
        if(failure==0)source.world=false;if(failure==1)templateAvailable=false;if(failure==2)creationAvailable=false;if(failure==3)scopeAvailable=false;
        assert(!Summon(source,true,3600,nullptr)&&map.attempts==0&&source.m_gameObj.empty()&&allocations.empty());
    }
    Reset();Map map;map.admission=false;Unit source;source.kind=1;source.map=&map;units[source.identity.value]=&source;
    assert(!Summon(source,true,3600,nullptr)&&source.m_gameObj.empty());map.admission=true;
    auto* result=Summon(source,true,3600,nullptr);assert(result&&result->world&&source.m_gameObj.size()==1);Dispose(result);
    assert(allocations.empty()&&source.m_gameObj.empty()&&created==destroyed&&fatals==0);
    std::cout<<"PASS: extracted WorldObject/Map summon overloads report controlled insertion failure as null and clean native owner/transport/phase references; success, personal visibility, static transports, owner policies and later retry remain intact. Actual grid admission/rendering are not simulated.\n";
    return 0;
}
