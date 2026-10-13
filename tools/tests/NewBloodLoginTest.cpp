#include <cassert>
#include <cstdint>
#include <iostream>
using uint32 = uint32_t;
constexpr int CLASS_WARLOCK=9, QUEST_STATUS_INCOMPLETE=3;
#define TC_LOG_INFO(...) ((void)0)
struct Scenario { int id; int GetScenarioId(){return id;} };
struct Manager { Scenario* scenario=nullptr; Scenario* GetScenario(uint32){return scenario;} } manager;
auto sScenarioMgr=&manager;
struct Player {
    int map=1522, cls=9, status=3, selector=0; bool rewarded=false, relocated=false;
    uint32 GetMapId(){return map;} int getClass(){return cls;}
    int GetQuestStatus(int){return status;} bool IsQuestRewarded(int){return rewarded;}
    int GetInstanceId(){return 1;} int GetScenarioId(){return selector;}
    void SetScenarioId(int id){selector=id;}
    void Relocate(float x,float y,float z,float o){assert(x==2959.64f && y==1126.32f && z==212.0f && o==6.27446f);relocated=true;}
    void restore(){
#include "Restore.inc"
    }
};
bool allowed(Player* player, uint32 mapid){int introQuest=40518;
#include "Access.inc"
return true;
}
int main(){
    Player p; p.restore(); assert(p.selector==962 && p.relocated && allowed(&p,1522));
    Scenario live{962};manager.scenario=&live;p=Player{};p.restore();assert(p.selector==962 && !p.relocated);
    live.id=998;p=Player{};p.restore();assert(!p.selector && !p.relocated && !allowed(&p,1522));
    manager.scenario=nullptr;
    for(int status:{0,1,6}){p=Player{};p.status=status;p.restore();assert(!p.selector && !p.relocated && !allowed(&p,1522));}
    p=Player{};p.rewarded=true;p.restore();assert(!p.selector && !p.relocated);
    p=Player{};p.cls=1;p.restore();assert(!p.selector && !p.relocated);
    for(int map:{1,1502,1460}){p=Player{};p.map=map;p.restore();assert(!p.selector && !p.relocated);}
    p=Player{};assert(allowed(&p,1460));p.status=0;assert(!allowed(&p,1460));
    p=Player{};p.selector=998;assert(!allowed(&p,1522));p.selector=962;assert(!allowed(&p,1500));
    std::cout << "PASS: fresh prison restore, live instance position, quest/class/map gates, later scenario and Broken Shore isolation\n";
}
