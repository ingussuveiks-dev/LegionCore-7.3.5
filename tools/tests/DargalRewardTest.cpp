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
int main(){Quest q;unsigned checked=0;
 for(bool msg:{false,true})for(uint32 spirits:{0u,8u,9u,10u})for(uint32 boss:{0u,1u,2u}){
  Player p;p.progress={{108815,spirits},{110635,boss}};auto before=p.progress;bool allowed=p.CanRewardQuest(&q,msg);
#ifdef EXPECT_BASELINE
  assert(allowed); // Pinned old COMPLETE log bypasses both actual counters.
#else
  assert(allowed==(spirits>=9&&boss>=1));
#endif
  assert(!p.errors&&p.progress==before);++checked;
 }
#ifdef EXPECT_BASELINE
 std::cout<<"COUNTERFACTUAL "<<checked<<" pinned native COMPLETE reward cases ignore missing repaired required counters; expected broken baseline, not successful ordinary progression\n";return 0;
#else
 for(uint32 id:{40225u,40938u,42770u,42772u,40518u}){Player p;q.id=id;assert(p.CanRewardQuest(&q,false));++checked;}q.id=42771;
 for(int mode=0;mode<7;++mode){Player p;p.progress={{108815,9},{110635,1}};
  if(mode==0)p.status=QUEST_STATUS_INCOMPLETE;if(mode==1)p.status=0;if(mode==2)p.daily=true;if(mode==3)p.weekly=true;if(mode==4)p.seasonal=true;if(mode==5)p.rewarded=true;
  if(mode==6){p.status=0;q.df=true;assert(p.CanRewardQuest(&q,false));q.df=false;}else assert(!p.CanRewardQuest(&q,true));++checked;
 }
 for(bool automatic:{false,true}){Player p;q.automatic=automatic;p.progress={{108815,9},{110635,0}};assert(!p.CanRewardQuest(&q,true));++checked;}q.automatic=false;
 for(int kind:{QUEST_OBJECTIVE_ITEM,QUEST_OBJECTIVE_CURRENCY,QUEST_OBJECTIVE_MONEY}){Player p;p.progress={{108815,9},{110635,1}};q.deliver=kind==QUEST_OBJECTIVE_ITEM;q.objectives={{kind,123,2,0}};
  assert(!p.CanRewardQuest(&q,true));p.items=p.currency=2;p.money=2;assert(p.CanRewardQuest(&q,false));++checked;
 }q.objectives.clear();q.deliver=false;
 std::cout<<"PASS "<<checked<<" actual native CanRewardQuest cases: legacy COMPLETE/area-independent real9+1 admission, no counter mutation, repeated/auto/DF/daily/weekly/seasonal/reward/item/currency/money and unrelated quests; controlled storage/status endpoints, not native login/client reward execution\n";
#endif
}