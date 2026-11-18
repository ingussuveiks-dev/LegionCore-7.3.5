#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;
constexpr int QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,QUEST_SPECIAL_FLAGS_DELIVER=128,QUEST_OBJECTIVE_ITEM=1,QUEST_OBJECTIVE_CURRENCY=4,QUEST_OBJECTIVE_MONEY=8,QUEST_OBJECTIVE_FLAG_OPTIONAL=4,EQUIP_ERR_ITEM_NOT_FOUND=1;
struct QuestObjective{int Type=0;uint32 ObjectID=0;int Amount=0,Flags=0;};
struct Quest{uint32 id=42771;bool df=false,automatic=false,deliver=false;std::vector<QuestObjective> objectives;uint32 GetQuestId()const{return id;}bool IsDFQuest()const{return df;}bool IsAutoComplete()const{return automatic;}bool HasSpecialFlag(int)const{return deliver;}auto const& GetObjectives()const{return objectives;}};
struct Player{
 int status=QUEST_STATUS_COMPLETE;bool daily=false,weekly=false,seasonal=false,rewarded=false;uint32 items=0,currency=0;uint64 money=0;unsigned errors=0;std::map<uint32,uint32> progress;
 uint32 GetQuestObjectiveData(uint32,uint32 object){return progress[object];}int GetQuestStatus(uint32){return status;}
 bool IsQuestDailyRewarded(uint32){return daily;}bool IsQuestWeekRewarded(uint32){return weekly;}bool IsQuestSeasonalRewarded(uint32){return seasonal;}bool GetQuestRewardStatus(uint32){return rewarded;}
 uint32 GetItemCount(uint32){return items;}void SendEquipError(int,void*,void*,uint32){++errors;}bool HasCurrency(uint32,int amount){return currency>=uint32(amount);}bool HasEnoughMoney(uint64 amount){return money>=amount;}
 bool CanRewardQuest(Quest const*,bool);
};
#include "CanReward.inc"
int main(){Quest q;q.id=40938;unsigned checked=0;
 for(bool msg:{false,true})for(unsigned mask=0;mask<243;++mask){
  unsigned value=mask;Player p;bool expected=true;for(uint32 marker:{102466u,102465u,102652u,102653u,102654u}){auto count=value%3;value/=3;p.progress[marker]=count;expected&=count>=1;}auto before=p.progress;
#ifdef EXPECT_BASELINE
  assert(p.CanRewardQuest(&q,msg));
#else
  assert(p.CanRewardQuest(&q,msg)==expected);
#endif
  assert(p.progress==before&&!p.errors);++checked;
 }
#ifdef EXPECT_BASELINE
 std::cout<<"COUNTERFACTUAL "<<checked<<" pinned native40938 COMPLETE reward cases ignore all missing required milestones; expected broken baseline\n";
#else
 for(int mode=0;mode<6;++mode){Player p;for(uint32 c:{102466u,102465u,102652u,102653u,102654u})p.progress[c]=1;
  if(mode==0)p.status=QUEST_STATUS_INCOMPLETE;if(mode==1)p.status=0;if(mode==2)p.daily=true;if(mode==3)p.weekly=true;if(mode==4)p.seasonal=true;if(mode==5)p.rewarded=true;assert(!p.CanRewardQuest(&q,true));++checked;
 }
 for(bool automatic:{false,true}){Player p;q.automatic=automatic;p.progress={{102466,1},{102465,1},{102652,1},{102653,0},{102654,1}};assert(!p.CanRewardQuest(&q,true));++checked;}q.automatic=false;
 for(int kind:{QUEST_OBJECTIVE_ITEM,QUEST_OBJECTIVE_CURRENCY,QUEST_OBJECTIVE_MONEY}){Player p;for(uint32 c:{102466u,102465u,102652u,102653u,102654u})p.progress[c]=1;q.deliver=kind==QUEST_OBJECTIVE_ITEM;q.objectives={{kind,123,2,0}};assert(!p.CanRewardQuest(&q,true));p.items=p.currency=2;p.money=2;assert(p.CanRewardQuest(&q,false));++checked;}
 std::cout<<"PASS "<<checked<<" extracted actual native Priest CanRewardQuest cases: all5 real milestones/thresholds, legacy COMPLETE, no mutation, auto/status/rewarded/daily/weekly/seasonal/inventory/currency/money admission; controlled endpoints, not client reward execution\n";
#endif
}
