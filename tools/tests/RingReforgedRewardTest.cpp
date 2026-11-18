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
int main(){Quest q;q.id=40225;unsigned checked=0;
 for(bool msg:{false,true})for(uint32 first:{0u,1u,2u})for(uint32 second:{0u,1u,2u}){
  Player p;p.progress={{97047,first},{97048,second}};auto before=p.progress;bool allowed=p.CanRewardQuest(&q,msg);
#ifdef EXPECT_BASELINE
  assert(allowed);
#else
  assert(allowed==(first>=1&&second>=1));
#endif
  assert(p.progress==before&&!p.errors);++checked;
 }
#ifdef EXPECT_BASELINE
 std::cout<<"COUNTERFACTUAL "<<checked<<" pinned native Ring COMPLETE reward cases ignore both zero-required counters; expected broken baseline\n";
#else
 for(int status:{0,QUEST_STATUS_INCOMPLETE}){Player p;p.status=status;p.progress={{97047,1},{97048,1}};assert(!p.CanRewardQuest(&q,false));++checked;}
 for(bool automatic:{false,true}){Player p;q.automatic=automatic;p.progress={{97047,1},{97048,0}};assert(!p.CanRewardQuest(&q,true));++checked;}q.automatic=false;
 for(int mode=0;mode<4;++mode){Player p;p.progress={{97047,1},{97048,1}};p.daily=mode==0;p.weekly=mode==1;p.seasonal=mode==2;p.rewarded=mode==3;assert(!p.CanRewardQuest(&q,false));++checked;}
 for(int kind:{QUEST_OBJECTIVE_ITEM,QUEST_OBJECTIVE_CURRENCY,QUEST_OBJECTIVE_MONEY}){Player p;p.progress={{97047,1},{97048,1}};q.deliver=kind==QUEST_OBJECTIVE_ITEM;q.objectives={{kind,123,2,0}};
  assert(!p.CanRewardQuest(&q,true));p.items=p.currency=2;p.money=2;assert(p.CanRewardQuest(&q,false));++checked;
 }
 std::cout<<"PASS "<<checked<<" actual extracted native Ring reward cases: both real required counters/thresholds, saved COMPLETE/auto/status/rewarded/daily/weekly/seasonal/storage/currency/money admission and no mutation; controlled services, native sequence checked separately\n";
#endif
}