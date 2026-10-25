#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <vector>
using uint32 = uint32_t;
using LootState = uint32;
#include "CriteriaTypes.inc"
enum { CLASS_PALADIN=2, QUEST_STATUS_INCOMPLETE=3, QUEST_STATUS_COMPLETE=1, TYPEID_PLAYER=4,
    GO_ACTIVATED=2, GO_JUST_DEACTIVATED=3, GAMEOBJECT_FIELD_FLAGS=17, GO_FLAG_NOT_SELECTABLE=16 };
struct Map { } liveMap, otherMap;
struct Item { };
struct Scenario
{
    uint32 id=1092,step=10;
    uint32 GetScenarioId() const { return id; }
    uint32 GetCurrentStep() const { return step; }
};
struct ScenarioManager
{
    Scenario scenario;
    bool present=true;
    Scenario* GetScenario(uint32 instance) { return present && instance==42 ? &scenario : nullptr; }
} manager;
auto* sScenarioMgr=&manager;
struct Player;
struct GameObject;
struct Unit
{
    virtual ~Unit()=default;
    virtual Player* ToPlayer() { return nullptr; }
    virtual uint32 GetTypeId() const { return 3; }
    uint32 scenes=0,returns=0;
    std::vector<std::function<void()>> pending;
    void CastSpell(Unit* target,uint32 spell,bool triggered=false)
    {
        assert(target==this);
        if(spell==211838){assert(triggered);++scenes;}
        else{assert(spell==211862 && !triggered);++returns;}
    }
    void AddDelayedEvent(uint32 delay,std::function<void()> callback) { assert(delay==12000);pending.push_back(std::move(callback)); }
};
struct Player : Unit
{
    bool world=true,contact=true,alive=true,teleport=false,rewarded=false,owned=false,bankOnly=false,room=true;
    bool phase=true,visible=true,los=true;
    uint32 klass=2,status=3,instance=42,events=0,completed=0,pickup=0,arrival=0,grants=0;
    float distance=1;
    Map* map=&liveMap;
    Item artifact;
    Player* ToPlayer() override { return this; }
    uint32 GetTypeId() const override { return TYPEID_PLAYER; }
    bool IsInWorld() const { return world; }
    bool CanContact() const { return contact; }
    bool IsAlive() const { return alive; }
    bool IsBeingTeleported() const { return teleport; }
    uint32 getClass() const { return klass; }
    Map* GetMap() const { return map; }
    uint32 GetInstanceId() const { return instance; }
    bool InSamePhase(GameObject*) const { return phase; }
    bool canSeeOrDetect(GameObject*) const { return visible; }
    bool IsWithinLOSInMap(GameObject*) const { return los; }
    bool IsWithinDistInMap(GameObject*,float radius) const { return distance<=radius; }
    bool GetQuestRewardStatus(uint32 quest) const { assert(quest==42120);return rewarded; }
    uint32 GetQuestStatus(uint32 quest) const { assert(quest==42120);return status; }
    bool HasItemCount(uint32 item,uint32 count) const { assert(item==128823 && count==1);return owned; }
    Item* GetItemByEntry(uint32 item) { assert(item==128823);return owned && !bankOnly?&artifact:nullptr; }
    void AddItem(uint32 item,uint32 count) { assert(item==128823 && count==1);++grants;owned=room; }
    void UpdateAchievementCriteria(uint32 type,uint32 asset,uint32 amount=0)
    {
        if(type==CRITERIA_TYPE_SCRIPT_EVENT_2){assert(asset==50966 && !amount);++events;manager.scenario.step=11;}
        else{assert(type==CRITERIA_TYPE_COMPLETE_SCENARIO && asset==1092 && amount==1);++completed;}
    }
    void KilledMonsterCredit(uint32 entry) { if(entry==106357)++pickup;else{assert(entry==105892);++arrival;} }
};
struct Creature { uint32 despawns=0;void DespawnOrUnsummon(uint32 delay){assert(delay==1000);++despawns;} } pickupTrigger;
struct GameObjectAI;
struct GameObject
{
    bool world=true,spawned=true,notSelectable=false,actor=true;
    uint32 entry=249824,mapId=1539,instance=42,m_lootState=GO_ACTIVATED;
    Map* map=&liveMap;
    bool IsInWorld() const { return world; }
    bool isSpawned() const { return spawned; }
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instance; }
    Map* GetMap() const { return map; }
    float GetInteractionDistance() const { return 5; }
    bool HasFlag(uint32 field,uint32 flag) const { assert(field==17 && flag==16);return notSelectable; }
    uint32 getLootState() const { return m_lootState; }
    Creature* FindNearestCreature(uint32 entryId,float radius) { assert(entryId==106781 && radius==10);return actor?&pickupTrigger:nullptr; }
    void SetLootState(LootState,Unit*);
    GameObjectAI* AI() const;
};
struct GameObjectAI
{
    explicit GameObjectAI(GameObject* object):go(object) { }
    virtual ~GameObjectAI()=default;
    virtual bool GossipHello(Player*,bool) { return false; }
    virtual void OnStateChanged(uint32,Unit*) { }
protected:
    GameObject* go;
};
struct GameObjectScript;
GameObjectScript* currentScript=nullptr;
struct GameObjectScript
{
    explicit GameObjectScript(char const*) { currentScript=this; }
    virtual ~GameObjectScript()=default;
    virtual bool OnGossipHello(Player*,GameObject*) { return false; }
    virtual GameObjectAI* GetAI(GameObject* object) const { return new GameObjectAI(object); }
    virtual void OnLootStateChanged(GameObject*,uint32,Unit*) { }
};
#include "Admission.inc"
#include "Callback.inc"
struct ScriptManager
{
    void OnGameObjectLootStateChanged(GameObject* object,uint32 state,Unit* target) { assert(currentScript);currentScript->OnLootStateChanged(object,state,target); }
} scriptManager;
auto* sScriptMgr=&scriptManager;
GameObjectAI* GameObject::AI() const { static GameObjectAI passive(nullptr);return &passive; }
#include "SetLootState.inc"
int main()
{
    for(uint32 invalid=0;invalid<32;++invalid)
    {
        manager={};Player player;GameObject weapon;go_art_silverhand script;
        switch(invalid)
        {
            case 0:player.world=false;break;
            case 1:player.contact=false;break;
            case 2:player.alive=false;break;
            case 3:player.teleport=true;break;
            case 4:player.klass=5;break;
            case 5:weapon.world=false;break;
            case 6:weapon.spawned=false;break;
            case 7:weapon.entry=249825;break;
            case 8:weapon.mapId=1220;break;
            case 9:player.map=&otherMap;break;
            case 10:player.instance=43;break;
            case 11:player.phase=false;break;
            case 12:player.visible=false;break;
            case 13:player.los=false;break;
            case 14:player.distance=5.01f;break;
            case 15:weapon.notSelectable=true;break;
            case 16:player.rewarded=true;break;
            case 17:player.status=0;break;
            case 18:player.status=5;break;
            case 19:manager.present=false;break;
            case 20:manager.scenario.id=991;break;
            case 21:manager.scenario.id=1037;break;
            case 22:manager.scenario.step=0;break;
            case 23:manager.scenario.step=9;break;
            case 24:manager.scenario.step=11;break;
            case 25:manager.scenario.step=12;break;
            case 26:player.map=nullptr;weapon.map=nullptr;break;
            case 27:weapon.instance=43;break;
            case 28:weapon.m_lootState=GO_JUST_DEACTIVATED;break;
            case 29:weapon.m_lootState=1;break;
            case 30:manager.scenario.step=11;player.owned=true;player.bankOnly=true;break;
            case 31:manager.scenario.step=12;player.owned=true;break;
        }
        bool ownedBefore=player.owned;
        auto* ai=script.GetAI(&weapon);
        bool denied=script.OnGossipHello(&player,&weapon),reported=ai->GossipHello(&player,false),opened=ai->GossipHello(&player,true);
        delete ai;
        script.OnLootStateChanged(&weapon,GO_ACTIVATED,&player);
#ifdef BASELINE
        assert(!denied && !reported && !opened && player.grants==uint32(!ownedBefore) && player.pickup==1 && player.arrival==1);
#else
        assert(denied==(invalid!=29) && reported==denied && opened==denied);
        assert(player.owned==ownedBefore && !player.grants && !player.events && !player.pickup && !player.arrival && !player.scenes && player.pending.empty());
#endif
    }
    for(uint32 status:{1u,3u})for(bool owned:{false,true})for(bool actor:{false,true})
    {
        manager={};Player player;player.status=status;player.owned=owned;player.distance=5;
        GameObject weapon;weapon.actor=actor;go_art_silverhand script;
        assert(!script.OnGossipHello(&player,&weapon));
        auto* ai=script.GetAI(&weapon);assert(!ai->GossipHello(&player,false) && !ai->GossipHello(&player,true));delete ai;
        weapon.SetLootState(GO_ACTIVATED,&player);
        assert(player.events==1 && player.completed==1 && player.pickup==1 && player.arrival==1 && player.scenes==1 && player.grants==uint32(!owned));
#ifndef BASELINE
        weapon.SetLootState(GO_ACTIVATED,&player);assert(player.pickup==2 && player.scenes==2 && player.grants==uint32(!owned));
#endif
    }
    {
        manager={};Player player;GameObject weapon;go_art_silverhand script;Unit pet,guardian;
        script.OnLootStateChanged(&weapon,GO_ACTIVATED,nullptr);script.OnLootStateChanged(&weapon,GO_ACTIVATED,&pet);script.OnLootStateChanged(&weapon,GO_ACTIVATED,&guardian);
        assert(!player.events && !pet.scenes && !guardian.scenes);
#ifndef BASELINE
        assert(script.OnGossipHello(nullptr,&weapon) && script.OnGossipHello(&player,nullptr));
        script.OnLootStateChanged(nullptr,GO_ACTIVATED,&player);
        player.status=0;weapon.SetLootState(GO_ACTIVATED,&player);assert(!player.events);
        player.status=3;weapon.SetLootState(GO_ACTIVATED,&player);assert(player.events==1);
        manager={};player={};player.room=false;weapon.SetLootState(GO_ACTIVATED,&player);
        assert(!player.owned && player.events==1 && player.arrival==1 && player.pending.size()==1);
        manager={};manager.scenario.step=11;player={};player.owned=true;
        assert(!script.OnGossipHello(&player,&weapon));weapon.SetLootState(GO_ACTIVATED,&player);
        assert(!player.grants && player.scenes==1 && player.pickup==1);
#endif
    }
#ifdef BASELINE
    std::cout<<"BASELINE b7b06b71:32 invalid direct activations still invoke legacy effects; no script/open/AI admission. Controlled inventory/credit sinks, not native quest persistence or live client.\n";
#else
    std::cout<<"PASS:31 invalid script/open/AI predicates and32 invalid/stale activation contexts rejected; eight active/owned/missing-actor paths, carried-copy step11 replay and actual SetLootState dispatch retained. Revocation/retry/null/pet/guardian checked. ReportUse ignores the AI return natively. KNOWN OPEN: eligible activation still grants/credits before real loot, even failed storage, and replays legacy credits; admission only, not loot/equip/return/client repair.\n";
#endif
}
