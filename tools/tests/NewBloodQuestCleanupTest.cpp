#include <cassert>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>
using uint32=uint32_t;
enum { QUEST_STATUS_NONE=0, DIFFICULTY_NORMAL=1 };
struct ObjectGuid
{
    int id=0, type=0;
    bool IsCreature() const {return type==1;}
    bool IsGameObject() const {return type==2;}
    bool IsConversation() const {return type==3;}
    bool operator==(ObjectGuid const& o) const {return id==o.id&&type==o.type;}
};
#include "Scope.inc"
struct WorldObject
{
    ObjectGuid guid;
    std::weak_ptr<QuestObjectScope> _questObjectScope;
    bool _questObjectBound=false;
    std::set<int> viewers;
    ObjectGuid GetGUID() const {return guid;}
    void AddPlayerInPersonnalVisibilityList(ObjectGuid g){viewers.insert(g.id);}
    void BindQuestObjectScope(std::shared_ptr<QuestObjectScope> const&);
    bool InheritQuestObjectScope(WorldObject const*);
    bool IsQuestObjectExpired() const;
    bool CanTargetQuestObject(WorldObject const*) const;
};
struct Creature: WorldObject
{
    bool removed=false,visible=true;
    void SetVisible(bool value){visible=value;}
    void DespawnOrUnsummon(){assert(IsQuestObjectExpired());removed=true;}
};
struct GameObject: WorldObject {bool removed=false;void Delete(){assert(IsQuestObjectExpired());removed=true;}};
struct Conversation: WorldObject {bool removed=false;void Remove(){assert(IsQuestObjectExpired());removed=true;}};
struct Map
{
    std::map<int,Creature*> creatures;
    std::map<int,GameObject*> objects;
    std::map<int,Conversation*> conversations;
    Creature* GetCreature(ObjectGuid g){return creatures.count(g.id)?creatures.at(g.id):nullptr;}
    GameObject* GetGameObject(ObjectGuid g){return objects.count(g.id)?objects.at(g.id):nullptr;}
};
struct Scenario {uint32 id;uint32 GetScenarioId(){return id;}};
struct ScenarioMgr {std::map<uint32,Scenario> data;Scenario* GetScenario(uint32 id){return data.count(id)?&data.at(id):nullptr;}} scenarios;
auto sScenarioMgr=&scenarios;
struct InstanceSave {uint32 id;uint32 GetInstanceId(){return id;}};
struct InstancePlayerBind {bool perm=false;InstanceSave* save=nullptr;};
struct QuestStatusData {int Status=3;std::shared_ptr<QuestObjectScope> ScriptObjects;};
struct Player
{
    ObjectGuid playerGuid;
    Map map;
    std::map<uint32,QuestStatusData> quests;
    uint32 scenarioId=962,mapId=1502,instanceId=0;
    bool inWorld=true,bound=false;
    InstancePlayerBind binding;
    std::set<uint32> auras{199966,199967,200033,204833,196229,199362,199338,195045,195066,195068,199195,999999};
    unsigned interrupted=0,teleports=0,unbinds=0;
    ObjectGuid GetGUID(){return playerGuid;}
    Map* GetMap(){return &map;}
    bool IsInWorld(){return inWorld;}
    QuestStatusData* getQuestStatus(uint32 q){return quests.count(q)?&quests.at(q):nullptr;}
    uint32 GetScenarioId(){return scenarioId;}
    void SetScenarioId(uint32 id){scenarioId=id;}
    uint32 GetMapId(){return mapId;}
    uint32 GetInstanceId(){return instanceId;}
    void InterruptNonMeleeSpells(bool,uint32 id){assert(id==199966);++interrupted;}
    void RemoveAurasDueToSpell(uint32 id){auras.erase(id);}
    InstancePlayerBind* GetBoundInstance(uint32 requestedMap,int){assert(requestedMap==1522);return bound?&binding:nullptr;}
    void UnbindInstance(uint32,int){bound=false;++unbinds;}
    void TeleportTo(uint32 id,float,float,float,float){assert(id==1502);++teleports;}
    void BindQuestObject(uint32,WorldObject*);
    void CancelQuestObjects(uint32);
    void ResetNewBloodQuest();
};
struct ObjectAccessor
{
    static Conversation* GetConversation(Player& p,ObjectGuid g)
    {return p.map.conversations.count(g.id)?p.map.conversations.at(g.id):nullptr;}
};
#include "ObjectScope.inc"
#include "PlayerScope.inc"
int main()
{
    Player a,b;a.playerGuid={1,0};b.playerGuid={2,0};a.quests[40729]={};b.quests[40729]={};
    Creature root,child,boss,jailer,other,fresh,unrelated;
    root.guid={10,1};child.guid={11,1};boss.guid={12,1};jailer.guid={13,1};other.guid={20,1};fresh.guid={30,1};
    a.BindQuestObject(40729,&root);assert(root.viewers.count(1)&&!root.viewers.count(2));
    assert(child.InheritQuestObjectScope(&root));assert(boss.InheritQuestObjectScope(&child));
    a.BindQuestObject(40729,&jailer);b.BindQuestObject(40729,&other);
    WorldObject playerA,playerB;playerA.guid=a.playerGuid;playerB.guid=b.playerGuid;
    assert(root.CanTargetQuestObject(&boss)&&root.CanTargetQuestObject(&playerA));
    assert(!root.CanTargetQuestObject(&other)&&!root.CanTargetQuestObject(&playerB)&&!root.CanTargetQuestObject(&unrelated));
    assert(unrelated.CanTargetQuestObject(&boss)); // ordinary world targeting unchanged
    GameObject portal;portal.guid={14,2};assert(portal.InheritQuestObjectScope(&child));
    Conversation dialogue;dialogue.guid={15,3};assert(dialogue.InheritQuestObjectScope(&boss));
    a.map.creatures={{12,&boss},{13,&jailer},{20,&other}}; // parent already despawned; child on another map
    a.map.objects={{14,&portal}};a.map.conversations={{15,&dialogue}};
    a.CancelQuestObjects(40729);a.quests.erase(40729);
    assert(boss.removed&&jailer.removed&&portal.removed&&dialogue.removed);
    assert(root.IsQuestObjectExpired()&&child.IsQuestObjectExpired());
    assert(!other.removed&&!other.IsQuestObjectExpired()&&!unrelated.IsQuestObjectExpired());
    a.quests[40729]={};a.BindQuestObject(40729,&fresh);
    assert(!fresh.IsQuestObjectExpired()&&boss.IsQuestObjectExpired());
    assert(!fresh.CanTargetQuestObject(&boss)&&!boss.CanTargetQuestObject(&fresh));
    Creature late;assert(!late.InheritQuestObjectScope(&boss)); // old callbacks cannot produce descendants
    a.CancelQuestObjects(999);assert(!fresh.IsQuestObjectExpired());
    a.CancelQuestObjects(40729);a.CancelQuestObjects(40729);assert(fresh.IsQuestObjectExpired());
    a.quests[40729]={};a.BindQuestObject(40729,&fresh);a.quests.clear();assert(fresh.IsQuestObjectExpired()); // logout/destruction
    a.ResetNewBloodQuest();assert(!a.scenarioId&&!a.teleports&&a.auras==std::set<uint32>{999999});
    scenarios.data[50]={962};InstanceSave save{50};a.binding={false,&save};a.bound=true;a.mapId=1522;a.instanceId=50;
    a.ResetNewBloodQuest();assert(a.teleports==1&&a.unbinds==1&&!a.bound);
    scenarios.data[51]={998};save.id=51;a.instanceId=51;a.bound=true;a.scenarioId=998;
    a.ResetNewBloodQuest();assert(a.teleports==1&&a.unbinds==1&&a.bound&&a.scenarioId==998);
    scenarios.data[51]={962};a.binding.perm=true;a.ResetNewBloodQuest();assert(a.bound&&a.unbinds==1);
    // Remote-map registration racing cancellation cannot corrupt the registry
    // or append live actors after the cancellation snapshot.
    QuestObjectScope concurrent;
    std::thread producer([&]{for(int i=0;i<10000;++i)concurrent.Register({i,1});});
    auto snapshot=concurrent.Cancel();producer.join();
    assert(concurrent.Cancelled&&concurrent.Cancel().empty()&&snapshot.size()<=10000);
    std::cout<<"PASS: production quest-object lifetime and cancellation, descendants after parent loss, same-tick reaccept, player isolation, remote-map expiry, portal/conversation cleanup, scoped aura and intro binding reset, later scenario preserved.\n";
}
