#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <vector>
using uint8=uint8_t;
using uint32=uint32_t;
using uint64=uint64_t;
struct ObjectGuid
{
    uint64 low=0,high=1;
    void Clear(){low=0;high=0;}
    uint32 GetGUIDLow() const{return uint32(low);}
    bool operator==(ObjectGuid const& other) const{return low==other.low && high==other.high;}
    bool operator!=(ObjectGuid const& other) const{return !(*this==other);}
    bool operator<(ObjectGuid const& other) const{return high<other.high || (high==other.high && low<other.low);}
};
using GuidSet=std::set<ObjectGuid>;
enum {CRITERIA_TYPE_SCRIPT_EVENT_2=92,GO_STATE_ACTIVE=0,GO_STATE_READY=1};
#include "DoorConstants.inc"
struct Unit {uint32 kind=0;};
struct Creature {uint32 entry;uint32 GetEntry() const{return entry;}};
struct GameObject;
struct Map
{
    bool unloading=false;
    bool IsMapUnload() const{return unloading;}
    GameObject* GetGameObject(ObjectGuid const& guid);
    void RemoveMaxVisible(GameObject*){assert(false);}
    bool ContainsGameObjectModel(int){assert(false);return false;}
    void RemoveGameObjectModel(int){assert(false);}
};
template<class Object> struct HashMapHolder
{
    inline static std::vector<Object*> objects=std::vector<Object*>(256,nullptr);
    inline static std::vector<Object*>& _objectVector=objects;
    inline static uint32 _size=256;
    inline static bool _checkLock=false;
    struct Lock {void lock_shared(){}void unlock_shared(){}};
    inline static Lock i_lockVector;
#include "NativeFind.inc"
    static void Clear(){objects.assign(256,nullptr);}
};
struct ObjectAccessor
{
#include "LookupWorld.inc"
#include "LookupMap.inc"
    void RemoveObject(GameObject* object);
} accessor;
auto* sObjectAccessor=&accessor;
struct InstanceScript
{
    Map* instance;
    std::vector<uint32> credits;
    uint32 removals=0,objectsRemoved=0,doorsRemoved=0;
    explicit InstanceScript(Map* map):instance(map){}
    virtual ~InstanceScript()=default;
    virtual void OnGameObjectRemove(GameObject* go);
    void OnGameObjectRemoveForScript(GameObject*){++removals;}
    void AddObject(GameObject*,bool add){assert(!add);++objectsRemoved;}
    void AddDoor(GameObject*,bool add){assert(!add);++doorsRemoved;}
    void HandleGameObject(ObjectGuid GUID,bool open,GameObject* go=nullptr);
    void DoUpdateAchievementCriteria(uint32 type,uint32 event){assert(type==92);credits.push_back(event);}
};
struct WorldObject
{
    bool world=true;
    void RemoveFromWorld(){world=false;}
};
struct GameObject : WorldObject
{
    ObjectGuid guid;
    uint32 entry;
    Map* map;
    InstanceScript* m_zoneScript;
    bool preDelete=false,MaxVisible=false;
    int* m_model=nullptr;
    uint32 state=GO_STATE_READY,phase=77,states=0,phases=0;
    std::function<void()> onState;
    GameObject(uint64 identity,uint32 value,Map* location,InstanceScript* script):guid{identity,1},entry(value),map(location),m_zoneScript(script){}
    bool IsInWorld() const{return world;}
    bool IsPreDelete() const{return preDelete;}
    Map* GetMap() const{return map;}
    uint32 GetEntry() const{return entry;}
    ObjectGuid GetGUID() const{return guid;}
    bool GetOwnerGUID() const{return false;}
    InstanceScript* GetBattleground() const{return nullptr;}
    void RemoveFromOwner(){}
    void SetGoState(uint32 value){state=value;++states;if(onState)onState();}
    void SetPhaseMask(uint32 value,bool update){assert(value==1 && update);phase=value;++phases;}
    void RemoveFromWorld();
};
void ObjectAccessor::RemoveObject(GameObject* object){HashMapHolder<GameObject>::objects[object->GetGUID().GetGUIDLow()]=nullptr;}
#define TC_LOG_DEBUG(...) ((void)0)
#include "NativeDoorMethods.inc"
struct Script : InstanceScript
{
    using InstanceScript::InstanceScript;
    std::list<Creature*> trashGUIDconteiner;
#ifdef BASELINE
    std::list<GameObject*> DoorGUIDconteiner;
#else
    GuidSet DoorGUIDconteiner;
#endif
    ObjectGuid tyrosusGUID;
    uint8 alliesFight=0;
#include "DoorMethods.inc"
};
void Add(Script& script,GameObject& object)
{
    script.OnGameObjectCreate(&object);
    HashMapHolder<GameObject>::objects[object.GetGUID().GetGUIDLow()]=&object;
}
int main(int argc,char** argv)
{
#ifdef BASELINE
    Map map;Script script(&map);script.Initialize();
    bool death=argc>1 && argv[1][0]=='d';
    auto* retired=new GameObject(1,death?GO_BOSS_ZERUS_DOOR_2:GO_ZERUS_DOOR,&map,&script);Add(script,*retired);
    retired->RemoveFromWorld();delete retired;
    if(death){Creature boss{NPC_JAILER_ZERUS_BOSS};script.CreatureDies(&boss,nullptr);}
    else script.onScenarionNextStep(DATA_STAGE_3);
    assert(false);
#else
    for(uint32 kind=0;kind<6;++kind)
    {
        Map map;Script script(&map);script.Initialize();
        GameObject first(1,GO_ZERUS_DOOR,&map,&script),bossFirst(2,GO_BOSS_ZERUS_DOOR_1,&map,&script);
        GameObject bossSecond(3,GO_BOSS_ZERUS_DOOR_2,&map,&script),unrelated(4,247318,&map,&script);
        for(GameObject* object:{&first,&bossFirst,&bossSecond,&unrelated}){Add(script,*object);Add(script,*object);}
        assert(script.DoorGUIDconteiner.size()==3);
        for(uint32 stage=0;stage<7;++stage)script.onScenarionNextStep(stage);
        assert(first.states==1 && bossFirst.states==0 && bossSecond.states==0 && unrelated.states==0);
        Unit killer{kind};Creature boss{NPC_JAILER_ZERUS_BOSS};script.CreatureDies(&boss,kind?&killer:nullptr);
        assert(script.credits==std::vector<uint32>{46064} && bossFirst.states==1 && bossSecond.phase==1 && bossSecond.phases==1);
        assert(first.states==1 && unrelated.states==0 && unrelated.phase==77);
        first.RemoveFromWorld();first.RemoveFromWorld();assert(script.DoorGUIDconteiner.size()==2);
        assert(script.objectsRemoved==1 && script.doorsRemoved==1 && script.removals==1);
        script.onScenarionNextStep(DATA_STAGE_3);assert(first.states==1);
        unrelated.RemoveFromWorld();assert(script.objectsRemoved==2 && script.doorsRemoved==2 && script.DoorGUIDconteiner.size()==2);
        Add(script,first);first.world=true;script.onScenarionNextStep(DATA_STAGE_3);assert(first.states==2);
        for(GameObject* object:{&first,&bossFirst,&bossSecond})object->RemoveFromWorld();
        assert(script.DoorGUIDconteiner.empty());HashMapHolder<GameObject>::Clear();
    }
    for(uint32 context=0;context<5;++context)
    {
        Map map,other;Script script(&map);script.Initialize();
        auto* retired=new GameObject(5,GO_ZERUS_DOOR,&map,&script);Add(script,*retired);
        if(context==0)retired->RemoveFromWorld();
        else if(context==1){map.unloading=true;retired->RemoveFromWorld();map.unloading=false;}
        else HashMapHolder<GameObject>::Clear();
        delete retired;
        GameObject replacement(5,GO_ZERUS_DOOR,&map,&script);
        if(context==2)replacement.map=&other;
        if(context==3)replacement.preDelete=true;
        if(context==4)replacement.guid.high=2;
        if(context>=2)HashMapHolder<GameObject>::objects[replacement.GetGUID().GetGUIDLow()]=&replacement;
        script.onScenarionNextStep(DATA_STAGE_3);
        Creature boss{NPC_JAILER_ZERUS_BOSS};script.CreatureDies(&boss,nullptr);
        assert(replacement.states==0 && script.credits==std::vector<uint32>{46064});
        script.Initialize();assert(script.DoorGUIDconteiner.empty());HashMapHolder<GameObject>::Clear();
    }
    {
        Map map;Script script(&map);script.Initialize();
        GameObject first(7,GO_ZERUS_DOOR,&map,&script);
        auto* removed=new GameObject(8,GO_BOSS_ZERUS_DOOR_1,&map,&script);
        Add(script,first);Add(script,*removed);
        first.onState=[&]{first.RemoveFromWorld();removed->RemoveFromWorld();delete removed;};
        script.onScenarionNextStep(DATA_STAGE_3);assert(first.states==1 && script.DoorGUIDconteiner.empty());
        HashMapHolder<GameObject>::Clear();
    }
    {
        Map map;Script script(&map);script.Initialize();
        GameObject first(9,GO_BOSS_ZERUS_DOOR_1,&map,&script);
        auto* removed=new GameObject(10,GO_BOSS_ZERUS_DOOR_2,&map,&script);
        Add(script,first);Add(script,*removed);
        first.onState=[&]{first.RemoveFromWorld();removed->RemoveFromWorld();delete removed;};
        Creature boss{NPC_JAILER_ZERUS_BOSS};script.CreatureDies(&boss,nullptr);
        assert(first.states==1 && script.DoorGUIDconteiner.empty() && script.credits==std::vector<uint32>{46064});
        HashMapHolder<GameObject>::Clear();
    }
    for(uint32 entry:{GO_BOSS_ZERUS_DOOR_1,GO_BOSS_ZERUS_DOOR_2})
    {
        Map map;Script script(&map);script.Initialize();
        auto* retired=new GameObject(6,entry,&map,&script);Add(script,*retired);retired->RemoveFromWorld();delete retired;
        Creature boss{NPC_JAILER_ZERUS_BOSS};script.CreatureDies(&boss,nullptr);
        GameObject restored(6,entry,&map,&script);Add(script,restored);script.CreatureDies(&boss,nullptr);
        assert(entry==GO_BOSS_ZERUS_DOOR_1?restored.states==1:restored.phases==1);
        restored.RemoveFromWorld();HashMapHolder<GameObject>::Clear();
    }
    std::cout<<"PASS extracted Ashbringer instance create/remove/Initialize/stage/death, actual GameObject::RemoveFromWorld and map/fullGUID/predelete lookup/HandleGameObject/base removal: freed three door kinds, skipped unload callback, missing/foreign/predelete/same-low-different-high objects, duplicate create/remove/recreate, unrelated object bookkeeping, all stages and null/player/pet/guardian/NPC killer labels. Original criteria and phase/state actions retained; no late-door progression replay or actual client collision/traversal proof\n";
#endif
}
