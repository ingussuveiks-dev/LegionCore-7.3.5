#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
using uint8=uint8_t;
using uint16=uint16_t;
using uint32=uint32_t;
using uint64=uint64_t;
struct GridCoord {uint32 x_coord,y_coord;GridCoord(int x,int y):x_coord(uint32(x)),y_coord(uint32(y)){};};
#include "GridCoordinates.inc"
#include "GrifPositions.inc"
struct GridInfo
{
    uint16 i_unloadActiveLockCount=0;
    uint8 i_unloadReferenceLock=0,i_unloadExplicitLock=0;
    bool getUnloadLock() const;
    void incUnloadActiveLock();
    void decUnloadActiveLock();
};
struct NGrid
{
    GridInfo i_GridInfo;
    void incUnloadActiveLock();
    void decUnloadActiveLock();
};
#include "GridLocks.inc"
struct Diagnostic {std::string message;uint32 guid,entry,currentX,currentY,spawnX,spawnY;};
std::vector<Diagnostic> diagnostics;
void LogError(char const*,char const* message,uint32 guid,uint32 entry,uint32 currentX,uint32 currentY,uint32 homeX,uint32 homeY){diagnostics.push_back({message,guid,entry,currentX,currentY,homeX,homeY});}
#define TC_LOG_ERROR LogError
struct Position
{
    float x=0,y=0,z=0,o=0;
    float GetPositionX() const{return x;}
    float GetPositionY() const{return y;}
    float GetPositionZ() const{return z;}
    float GetOrientation() const{return o;}
};
struct Map;
struct Creature;
struct WorldObject:Position
{
    bool m_isActive=false,inWorld=true;
    Map* map=nullptr;
    virtual ~WorldObject()=default;
    virtual Creature* ToCreature(){return nullptr;}
    bool IsPlayer() const{return false;}
    bool IsCreature(){return ToCreature()!=nullptr;}
    bool IsDynObject() const{return !IsCreatureKind;}
    bool IsGameObject() const{return false;}
    bool IsCreatureKind=false;
    WorldObject* ToDynObject(){return this;}
    WorldObject* ToGameObject(){return this;}
    bool IsInWorld() const{return inWorld;}
    Map* FindMap(){return map;}
    void setActive(bool);
};
struct CreatureData {float posX=spawnX,posY=spawnY,posZ=419.365f,orientation=0,spawndist=0;};
struct ObjectManager
{
    CreatureData data;
    CreatureData const* GetCreatureData(uint32 guid){return guid==11078574?&data:nullptr;}
};
ObjectManager manager;
ObjectManager* sObjectMgr=&manager;
struct Creature:WorldObject
{
    uint32 guid=900901632,entry=106570,m_DBTableGuid=11078574;
    bool pet=false,summon=false;
    Position home;
    Creature(){IsCreatureKind=true;x=spawnX;y=spawnY;}
    Creature* ToCreature() override{return this;}
    bool isPet() const{return pet;}
    bool isSummon() const{return summon;}
    uint32 GetGUIDLow() const{return guid;}
    uint32 GetEntry() const{return entry;}
    uint32 GetDBTableGUIDLow() const{return m_DBTableGuid;}
    uint32 GetMapId() const{return 1579;}
    Position const& GetHomePosition() const{return home;}
    void GetRespawnPosition(float&,float&,float&,float* =nullptr,float* =nullptr) const;
};
#include "RespawnPosition.inc"
struct Session
{
    bool hasPlayer=true;
    uint32 logoutCount=0;
    std::function<void()> onLogout;
    bool GetPlayer(){return hasPlayer;}
    void LogoutPlayer(bool save){assert(save);++logoutCount;if(onLogout)onLogout();hasPlayer=false;}
    void SetMap(Map*){}
};
using WorldSessionPtr=std::shared_ptr<Session>;
struct PendingFunctions {uint32 updated=0;std::function<void()> pending;void Update(uint32 diff){updated+=diff;if(pending)pending();}};
struct Pool {uint32 waited=0;void wait(){++waited;}};
struct Map
{
    using ActiveNonPlayers=std::set<WorldObject*>;
    using GridContainerType=std::map<std::size_t,NGrid>;
    using SessionMap=std::map<uint32,WorldSessionPtr>;
    ActiveNonPlayers m_activeNonPlayers;
    GridContainerType i_loadedGrids;
    SessionMap m_sessions;
    PendingFunctions m_Functions;
    Pool* threadPool=nullptr;
    bool b_isMapUnload=false;
    std::vector<uint32> _creaturesToMove,_gameObjectsToMove,_dynamicObjectsToMove,_areaTriggersToMove;
    std::vector<WorldObject*> objects;
    std::vector<uint16> locksAtDelete;
    static std::size_t Key(uint32 x,uint32 y){return std::size_t(x)*64+y;}
    NGrid* getNGrid(uint32 x,uint32 y){auto found=i_loadedGrids.find(Key(x,y));return found==i_loadedGrids.end()?nullptr:&found->second;}
#include "ActiveHelpers.inc"
    template<class T>void AddToActive(T* object){AddToActiveHelper(object);}
    template<class T>void RemoveFromActive(T* object){RemoveFromActiveHelper(object);}
    void AddToActive(Creature*);
    void RemoveFromActive(Creature*);
    void UnloadAll();
    bool UnloadGrid(GridContainerType::iterator grid,bool force)
    {
        if(!force && grid->second.i_GridInfo.getUnloadLock())return false;
        for(WorldObject* object:objects)
        {
            auto position=Trinity::ComputeGridCoord(object->x,object->y);
            if(Key(position.x_coord,position.y_coord)!=grid->first)continue;
            if(Creature* creature=object->ToCreature()){if(creature->m_isActive)RemoveFromActive(creature);}else RemoveFromActiveHelper(object);
            object->inWorld=false;
        }
        locksAtDelete.push_back(grid->second.i_GridInfo.i_unloadActiveLockCount);
        i_loadedGrids.erase(grid);return true;
    }
    void Attach(WorldObject& object){object.map=this;objects.push_back(&object);}
    void CreateGrid(uint32 x,uint32 y){i_loadedGrids.try_emplace(Key(x,y));}
};
#include "SetActive.inc"
#include "MapActive.inc"
struct Fixture
{
    Map map;
    Creature grif;
    Fixture(){diagnostics.clear();map.CreateGrid(36,36);map.CreateGrid(37,36);map.Attach(grif);grif.setActive(true);grif.x=destinationX;grif.y=destinationY;}
};
int main()
{
    auto spawn=Trinity::ComputeGridCoord(spawnX,spawnY),destination=Trinity::ComputeGridCoord(destinationX,destinationY);
    assert(spawn.x_coord==36 && spawn.y_coord==36 && destination.x_coord==37 && destination.y_coord==36);
#ifdef BASELINE
    Fixture fixture;assert(fixture.map.getNGrid(36,36)->i_GridInfo.i_unloadActiveLockCount==1);
    fixture.map.UnloadAll();assert(diagnostics.size()==1 && diagnostics[0].message.find("removed")!=std::string::npos);
    assert(diagnostics[0].currentX==36 && diagnostics[0].spawnX==37 && fixture.map.locksAtDelete[0]==1);
    std::cout<<"PASS pinned0e238cb1: native UnloadAll removes spawn36 while Grif remains active in37; actual native RemoveFromActive emits reported error, old coordinate labels reversed; spawn lock was still1 at forced deletion\n";
#else
    {
        Fixture fixture;WorldObject dynamic;fixture.map.Attach(dynamic);dynamic.x=destinationX;dynamic.y=destinationY;dynamic.setActive(true);
        Pool pool;fixture.map.threadPool=&pool;auto session=std::make_shared<Session>();fixture.map.m_sessions[1]=session;
        fixture.map._creaturesToMove={1};fixture.map._gameObjectsToMove={1};fixture.map._dynamicObjectsToMove={1};fixture.map._areaTriggersToMove={1};
        fixture.map.UnloadAll();assert(diagnostics.empty() && fixture.map.m_activeNonPlayers.empty() && fixture.map.i_loadedGrids.empty());
        assert(!fixture.grif.m_isActive && !dynamic.m_isActive && fixture.map.locksAtDelete==std::vector<uint16>({0,0}));
        assert(session->logoutCount==1 && pool.waited==1 && fixture.map.m_Functions.updated==120000);
        assert(fixture.map._creaturesToMove.empty() && fixture.map._gameObjectsToMove.empty() && fixture.map._dynamicObjectsToMove.empty() && fixture.map._areaTriggersToMove.empty());
        fixture.map.UnloadAll();assert(diagnostics.empty());
    }
    {
        Fixture fixture;Creature other;other.guid=2;fixture.map.Attach(other);other.setActive(true);
        fixture.map.AddToActive(&fixture.grif);assert(fixture.map.getNGrid(36,36)->i_GridInfo.i_unloadActiveLockCount==2);
        fixture.grif.setActive(false);fixture.map.RemoveFromActive(&fixture.grif);assert(fixture.map.getNGrid(36,36)->i_GridInfo.i_unloadActiveLockCount==1);
        other.setActive(false);assert(!fixture.map.getNGrid(36,36)->i_GridInfo.getUnloadLock());
        assert(fixture.map.UnloadGrid(fixture.map.i_loadedGrids.begin(),false));fixture.map.UnloadAll();assert(diagnostics.empty());
    }
    for(bool pet:{false,true})
    {
        Map map;Creature actor;actor.pet=pet;actor.m_DBTableGuid=pet?11078574:0;map.Attach(actor);actor.setActive(true);
        assert(diagnostics.empty() && map.m_activeNonPlayers.size()==1);map.UnloadAll();assert(!actor.m_isActive && map.m_activeNonPlayers.empty());
    }
    {
        Fixture fixture;fixture.map.i_loadedGrids.erase(Map::Key(36,36));fixture.grif.setActive(false);
        assert(diagnostics.size()==1 && diagnostics[0].message.find("removed")!=std::string::npos && diagnostics[0].currentX==37 && diagnostics[0].spawnX==36);
        diagnostics.clear();fixture.grif.setActive(true);assert(diagnostics.size()==1 && diagnostics[0].message.find("added")!=std::string::npos && diagnostics[0].currentX==37 && diagnostics[0].spawnX==36);
        diagnostics.clear();fixture.map.CreateGrid(36,36);fixture.map.RemoveFromActive(&fixture.grif);fixture.grif.setActive(false);
    }
    {
        Fixture fixture;fixture.grif.inWorld=false;fixture.map.UnloadAll();assert(diagnostics.empty() && fixture.map.m_activeNonPlayers.empty());
    }
    {
        Map map;Creature late;map.CreateGrid(36,36);map.CreateGrid(37,36);map.Attach(late);
        map.m_Functions.pending=[&]{late.setActive(true);late.x=destinationX;late.y=destinationY;};map.UnloadAll();assert(diagnostics.empty() && !late.m_isActive && map.locksAtDelete==std::vector<uint16>({0,0}));
    }
    std::cout<<"PASS native Map add/remove/UnloadAll, WorldObject setActive, Creature respawn coordinates and GridInfo/NGrid locks: release while spawn grid exists, exactly-once membership, ordinary live locks, pet/runtime/non-creature/already-out-of-world/deferred actor/session/thread-pool cleanup, repeated teardown. Genuine missing grids still diagnose current37/spawn36. Grid deletion/actors controlled; no real client unload traversal proof\n";
#endif
}
