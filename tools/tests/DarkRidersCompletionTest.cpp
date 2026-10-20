#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using int8=int8_t;
struct QuestObjective{uint32 ObjectID;int8 StorageIndex;};
struct Quest{std::vector<QuestObjective> objectives{{103473,0},{100813,3}};auto const& GetObjectives()const{return objectives;}};
struct QuestStore{Quest q;Quest const* GetQuestTemplate(uint32 id){return id==40623?&q:nullptr;}} questStore;auto sQuestDataStore=&questStore;
constexpr int CLASS_WARLOCK=9,QUEST_STATUS_INCOMPLETE=3;
#define TC_LOG_INFO(...) ((void)0)
struct Player {
 bool world=true,teleport=false,rewarded=false,owned=true;int cls=9,status=3,claim=0,complete=0,credits=0;
 bool IsInWorld(){return world;}bool IsBeingTeleported(){return teleport;}int getClass(){return cls;}
 bool GetQuestRewardStatus(uint32 id){assert(id==40623);return rewarded;}
 int GetQuestStatus(uint32 id){assert(id==40623);return status;}
 int32 GetQuestObjectiveData(uint32,uint32)const;
 int32 GetQuestObjectiveData(Quest const*,int8 index)const{return index==0?claim:complete;}
 bool HasItemCount(uint32 id,int count,bool bank){assert(id==128942&&count==1&&bank);return owned;}
 void KilledMonsterCredit(uint32 id){assert(id==100813);++credits;complete=1;status=1;}
};
#include "ObjectiveLookup.inc"
#include "Completion.inc"
int main(){
 Player p;DarkRidersCompletion::Recover(&p);assert(!p.credits); // bought item alone
 p.claim=1;DarkRidersCompletion::Recover(&p);assert(p.credits==1&&p.complete==1);
 DarkRidersCompletion::Recover(&p);assert(p.credits==1);
 for(int gate=0;gate<8;++gate){Player q;q.claim=1;
  switch(gate){case 0:q.world=false;break;case 1:q.teleport=true;break;case 2:q.rewarded=true;break;
   case 3:q.cls=6;break;case 4:q.status=0;break;case 5:q.claim=0;break;case 6:q.complete=1;break;case 7:q.owned=false;break;}
  DarkRidersCompletion::Recover(&q);assert(!q.credits);
 }
 DarkRidersCompletion::Recover(nullptr);
 std::cout<<"PASS: actual completion repair requires persisted final claim and owned artifact; shop-only, wrong class/quest, completed, teleport and repeated calls excluded\n";
}
