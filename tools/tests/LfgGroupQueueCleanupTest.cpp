#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <string>
using uint32=uint32_t;
int errorLogs=0,debugLogs=0;
#define TC_LOG_ERROR(...) (++errorLogs)
#define TC_LOG_DEBUG(...) (++debugLogs)
struct ObjectGuid
{
    int id=0;
    bool IsEmpty()const{return !id;}
    bool operator<(ObjectGuid b)const{return id<b.id;}
    bool operator==(ObjectGuid b)const{return id==b.id;}
    std::string ToString()const{return std::to_string(id);}
};
struct LFGMgr
{
    std::map<ObjectGuid,std::set<uint32>> GroupDungeons;
    void RemoveFromGroupQueue(ObjectGuid,uint32);
};
#include "Cleanup.inc"
int main()
{
    LFGMgr manager;ObjectGuid party{1},other{2};
    manager.GroupDungeons[party]={1045,908};manager.GroupDungeons[other]={1045};
    auto initial=manager.GroupDungeons;
    manager.RemoveFromGroupQueue({},1045);assert(manager.GroupDungeons==initial);
    manager.RemoveFromGroupQueue({99},1045);assert(manager.GroupDungeons==initial);
    manager.RemoveFromGroupQueue(party,777);assert(manager.GroupDungeons==initial);
    assert(!errorLogs&&!debugLogs);
    manager.RemoveFromGroupQueue(party,1045);assert(manager.GroupDungeons.at(party)==std::set<uint32>{908});
    assert(manager.GroupDungeons.at(other)==std::set<uint32>{1045}&&debugLogs==1&&!errorLogs);
    manager.RemoveFromGroupQueue(party,1045);assert(debugLogs==1&&!errorLogs);
    manager.RemoveFromGroupQueue(party,908);assert(!manager.GroupDungeons.count(party)&&debugLogs==2&&!errorLogs);
    manager.RemoveFromGroupQueue(party,908);assert(debugLogs==2&&!errorLogs);
    manager.GroupDungeons[party]={};manager.RemoveFromGroupQueue(party,1045);assert(!manager.GroupDungeons.count(party));
    std::cout<<"PASS: native group-queue cleanup handles solo/absent/repeated/empty queues, preserves other queues/groups, and emits no false ERROR.\n";
}
