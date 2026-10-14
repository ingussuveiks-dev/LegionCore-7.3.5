#include <cassert>
#include <cstdint>
#include <iostream>
using uint32 = uint32_t;
constexpr int CLASS_WARLOCK=9, QUEST_STATUS_INCOMPLETE=3;
constexpr int PLAYER_LOGIN_QUERY_LOADQUESTSTATUS=0;
#define TC_LOG_INFO(...) ((void)0)
struct Scenario { int id; int GetScenarioId(){return id;} };
struct Manager { Scenario* scenario=nullptr; Scenario* GetScenario(uint32){return scenario;} } manager;
auto sScenarioMgr=&manager;
struct Player {
    int map=1522, cls=9, status=3, selector=0, loads=0; bool rewarded=false, relocated=false;
    uint32 tome=0, bankTome=0, unrelated=7;
    bool itemsLoaded=false;
    uint32 GetItemCount(uint32 item, bool bank){assert(item==132512 && bank && itemsLoaded);return tome+bankTome;}
    void DestroyItemCount(uint32 item,uint32 count,bool update){assert(item==132512 && !update && count==tome+bankTome && itemsLoaded);tome=bankTome=0;}
    struct Holder {int status=3;int GetPreparedResult(int){return status;}} holder;
    void _LoadQuestStatus(int savedStatus){++loads;status=savedStatus;}
    bool early(){uint32 mapId=map,instanceId=1;
#include "EarlyRestore.inc"
        return newBloodQuestStatusLoaded;
    }
    uint32 GetMapId(){return map;} int getClass(){return cls;}
    int GetQuestStatus(int){return status;} bool IsQuestRewarded(int){return rewarded;}
    int GetInstanceId(){return 1;} int GetScenarioId(){return selector;}
    void SetScenarioId(int id){selector=id;}
    void Relocate(float x,float y,float z,float o){assert(x==2959.64f && y==1126.32f && z==212.0f && o==6.27446f);relocated=true;}
    void restore(uint32 savedTome=1,uint32 savedBankTome=1){
#include "Restore.inc"
        // The saved inventory becomes available only after the restart decision.
        itemsLoaded=true;tome=savedTome;bankTome=savedBankTome;
#include "InventoryRestore.inc"
    }
};
bool allowed(Player* player, uint32 mapid){int introQuest=40518;
#include "Access.inc"
return true;
}
int main(){
    // Actual login order: status/selector before CreateMap, then the constructor
    // consumes the selector, then the post-add access check sees the live scenario.
    Player resumed;resumed.status=0;assert(resumed.early());
    assert(resumed.loads==1 && resumed.selector==962 && resumed.status==3);
    resumed.restore();Scenario created{962};manager.scenario=&created;
    resumed.selector=0;assert(allowed(&resumed,1522));
    manager.scenario=nullptr;
    Player p; p.restore(); assert(p.selector==962 && p.relocated && allowed(&p,1522));
    assert(!p.tome && !p.bankTome && p.unrelated==7);
    p=Player{};p.restore(0,0);assert(p.relocated && !p.tome && !p.bankTome && p.unrelated==7);
    Scenario live{962};manager.scenario=&live;p=Player{};p.restore();assert(p.selector==962 && !p.relocated && p.tome==1 && p.bankTome==1);
    live.id=998;p=Player{};p.restore();assert(!p.selector && !p.relocated && !allowed(&p,1522) && p.tome==1 && p.bankTome==1);
    p.selector=962;assert(!allowed(&p,1522)); // stale selector cannot override a later live scenario
    manager.scenario=nullptr;
    for(int status:{0,1,6}){p=Player{};p.holder.status=status;assert(p.early());assert(p.loads==1 && !p.selector);p.restore();assert(!p.selector && !p.relocated && !allowed(&p,1522) && p.tome==1);}
    p=Player{};p.rewarded=true;p.restore();assert(!p.selector && !p.relocated && p.tome==1);
    p=Player{};p.cls=1;p.restore();assert(!p.selector && !p.relocated && p.tome==1);
    assert(!p.early() && !p.loads);
    for(int map:{1,1502,1460}){p=Player{};p.map=map;p.restore();assert(!p.selector && !p.relocated && p.tome==1);assert(!p.early() && !p.loads);}
    p=Player{};assert(allowed(&p,1460));p.status=0;assert(!allowed(&p,1460));
    p=Player{};p.selector=998;assert(!allowed(&p,1522));p.selector=962;assert(!allowed(&p,1500));
    std::cout << "PASS: fresh prison restore removes stale inventory/bank tome after loading; live instance, other items, quest/class/map gates, later scenario and Broken Shore preserved\n";
}
