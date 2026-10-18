#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
using uint8=uint8_t; using uint16=uint16_t; using uint32=uint32_t; using uint64=uint64_t; using int32=int32_t;
#include "GreyEnums.inc"
constexpr int GO_READY=0, GO_JUST_DEACTIVATED=3;
#define TC_LOG_DEBUG(...) ((void)0)
struct ObjectGuid {};
struct QuestObjective {uint32 Type; int32 StorageIndex,ObjectID,Amount; uint32 Flags; float TaskStep;};
struct Quest {bool raid=false;std::vector<QuestObjective> objectives;
 const auto& GetObjectives()const{return objectives;} bool HasSpecialFlag(uint32 f)const{return (f&QUEST_SPECIAL_FLAGS_KILL)!=0;}
 bool IsAllowedInRaid(int)const{return raid;} bool IsWeekly()const{return false;}bool IsSeasonal()const{return false;}
 bool IsRepeatable()const{return true;}bool IsAutoComplete()const{return false;}};
struct Rotation {int64_t ResetTime=200;};
struct Store {Quest* quest=nullptr;Rotation* rotation=nullptr;Quest const* GetQuestTemplate(uint32 q){return q==44737?quest:nullptr;}
 Rotation* GetWorldQuest(Quest const*){return rotation;}} store;
auto sQuestDataStore=&store;
namespace GameTime {int64_t now=100;int64_t GetGameTime(){return now;}}
struct Group {bool raid=false;bool isRaidGroup(){return raid;}};
struct Map {int GetDifficultyID(){return 0;}};
struct QuestStatusData {int Status=QUEST_STATUS_INCOMPLETE;uint32 Timer=0;};
struct Rep {uint32 GetReputation(uint32){return 0;}};
struct Player {
 bool alive=true;Group* group=nullptr;Map map;QuestStatusData status;std::map<int,int> counts;
 std::map<uint32,int> m_RewardedQuests;int completionCalls=0;
 bool IsAlive(){return alive;}Group* GetGroup(){return group;}Map* GetMap(){return &map;}
 int GetQuestStatus(uint32){return status.Status;}QuestStatusData* getQuestStatus(uint32){return &status;}
 int GetQuestObjectiveData(Quest const*,int slot){return counts[slot];}
 void SetQuestObjectiveData(Quest const*,QuestObjective const* o,int count){counts[o->StorageIndex]=count;}
 void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,int){}
 void CompleteQuest(uint32){status.Status=QUEST_STATUS_COMPLETE;++completionCalls;}
 void SetQuestStatus(uint32,int s){status.Status=s;}
 bool IsQuestWeekRewarded(uint32){return false;}bool IsQuestSeasonalRewarded(uint32){return false;}
 bool CanTakeQuest(Quest const*,bool){return false;}Rep& GetReputationMgr(){static Rep r;return r;}
 bool HasEnoughMoney(uint64){return false;}bool HasSpell(int){return false;}bool HasCurrency(int,int){return false;}
 bool HasQuestObjectiveComplete(Quest const*,QuestObjective const&);bool CanCompleteQuest(uint32);
 void KilledMonsterCredit(uint32 real_entry){uint16 addKillCount=1;ObjectGuid guid;
  for(uint32 questid:{44737u}) {
#include "GreyKillObjective.inc"
};
#include "GreyCompletion.inc"
struct GameObject {uint32 map=1220,area=8297;bool phase=true,near=true,spawned=true;int loot=GO_READY,deactivations=0;
 uint32 GetMapId(){return map;}uint32 GetAreaId(){return area;}bool InSamePhase(Player*){return phase;}
 float GetInteractionDistance(){return 5;}bool IsWithinDistInMap(Player*,float){return near;}
 bool isSpawned(){return spawned;}int getLootState(){return loot;}
 void SetLootState(int state){loot=state;++deactivations;}};
struct GameObjectScript {explicit GameObjectScript(char const*){}virtual bool OnGossipHello(Player*,GameObject*)=0;};
#include "GreySupplies.inc"
int main(){
 Quest q;Rotation r;store.quest=&q;store.rotation=&r;
 q.objectives={{15,-1,0,1,0,0},{0,1,115025,17,92,6},{0,2,115031,20,92,5},
 {0,3,115026,50,92,2},{0,0,115739,20,92,5},{0,4,115751,4,92,25}};
 go_grey_shoals_supplies script;
 for(int failure=0;failure<12;++failure){
  Player p;GameObject go;Group g;store.quest=&q;store.rotation=&r;r.ResetTime=200;
  switch(failure){case 0:store.rotation=nullptr;break;case 1:r.ResetTime=100;break;
   case 2:r.ResetTime=99;break;case 3:store.quest=nullptr;break;case 4:p.alive=false;break;
   case 5:go.phase=false;break;case 6:p.status.Status=QUEST_STATUS_NONE;break;
   case 7:p.status.Status=QUEST_STATUS_COMPLETE;break;case 8:g.raid=true;p.group=&g;break;
   case 9:go.near=false;break;case 10:go.spawned=false;break;case 11:go.loot=1;break;}
  assert(script.OnGossipHello(&p,&go));assert(p.counts.empty()&&go.deactivations==0);
 }
 store.quest=&q;store.rotation=&r;r.ResetTime=200;
 Player p;GameObject go;
 assert(script.OnGossipHello(&p,&go));assert(p.counts[0]==1&&go.deactivations==1);
 assert(script.OnGossipHello(&p,&go));assert(p.counts[0]==1&&go.deactivations==1);
 go.loot=GO_READY;assert(script.OnGossipHello(&p,&go));assert(p.counts[0]==2);
 // Expiry and a new rotation: stale incomplete state cannot consume the object.
 go.loot=GO_READY;r.ResetTime=100;assert(script.OnGossipHello(&p,&go));assert(p.counts[0]==2&&go.loot==GO_READY);
 r.ResetTime=300;assert(script.OnGossipHello(&p,&go));assert(p.counts[0]==3);
 go.map=1;assert(!script.OnGossipHello(&p,&go));go.map=1220;go.area=1;assert(!script.OnGossipHello(&p,&go));
 Player mixed;mixed.KilledMonsterCredit(115751);for(int i=0;i<10;++i)mixed.KilledMonsterCredit(115025);
 for(int i=0;i<2;++i)mixed.KilledMonsterCredit(115031);mixed.KilledMonsterCredit(115026); // 97%
 assert(!mixed.CanCompleteQuest(44737));mixed.KilledMonsterCredit(115739); // 102%
 assert(mixed.status.Status==QUEST_STATUS_COMPLETE&&mixed.completionCalls==1);
 auto counts=mixed.counts;mixed.KilledMonsterCredit(115739);assert(mixed.counts==counts);
 Player noBoss;for(int i=0;i<20;++i)noBoss.KilledMonsterCredit(115739);
 assert(noBoss.status.Status==QUEST_STATUS_COMPLETE&&noBoss.counts[4]==0);
 Player onlyBoss;for(int i=0;i<4;++i)onlyBoss.KilledMonsterCredit(115751);
 assert(onlyBoss.status.Status==QUEST_STATUS_COMPLETE);
 Player unknown;unknown.KilledMonsterCredit(999);assert(unknown.counts.empty());
 Player raider;Group raid;raid.raid=true;raider.group=&raid;raider.KilledMonsterCredit(115751);assert(raider.counts.empty());
 std::cout<<"PASS: production supply admission, no foreign-spell fallthrough, repeat/expiry/new rotation, native mixed/optional/boss-only progress and completion gating\n";
}
