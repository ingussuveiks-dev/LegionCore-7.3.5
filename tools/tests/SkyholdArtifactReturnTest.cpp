#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int8=int8_t;using int32=int32_t;
#define TC_LOG_DEBUG(...) ((void)0)
#include "ObjectiveEnums.inc"
enum {CLASS_WARRIOR=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,QUEST_DEFAULT_SAVE_TYPE=0,MAX_QUEST_LOG_SIZE=25,
 QUEST_SPECIAL_FLAGS_KILL=1,QUEST_SPECIAL_FLAGS_CAST=2,QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT=4,QUEST_SPECIAL_FLAGS_TIMED=8,
 CONFIG_GROUP_XP_DISTANCE=1};
struct QuestObjective {uint32 ID=0,QuestID=0;uint8 Type=0;int8 StorageIndex=0;int32 ObjectID=0,Amount=1;uint32 Flags=0;float TaskStep=0;bool IsStoringFlag()const{return false;}};
struct Quest {
 uint32 identity=0;std::vector<QuestObjective> objectives;
 uint32 GetQuestId()const{return identity;}auto const& GetObjectives()const{return objectives;}
 bool IsWeekly()const{return false;}bool IsSeasonal()const{return false;}bool IsRepeatable()const{return false;}bool IsAutoComplete()const{return false;}
 bool HasSpecialFlag(uint32 flags)const{return flags&QUEST_SPECIAL_FLAGS_KILL;}bool IsAllowedInRaid(uint32)const{return false;}
};
struct Store {std::map<uint32,Quest> quests;Quest const* GetQuestTemplate(uint32 identity)const{auto found=quests.find(identity);return found==quests.end()?nullptr:&found->second;}} store;
auto* sQuestDataStore=&store;
struct ObjectGuid {std::string ToString()const{return "fixture";}static ObjectGuid Empty;};ObjectGuid ObjectGuid::Empty;
struct Map {bool dungeon=false;uint32 GetDifficultyID()const{return 0;}bool IsDungeon()const{return dungeon;}} liveMap;
struct WorldObject {
 bool world=true;uint32 mapId=1479,instanceId=42;float distance=0;Map* map=&liveMap;
 virtual ~WorldObject()=default;
 bool IsInWorld()const{return world;}bool IsCreature()const{return false;}ObjectGuid GetGUID()const{return {};}
 Map* GetMap()const{return map;}uint32 GetMapId()const{return mapId;}uint32 GetInstanceId()const{return instanceId;}
 float GetDistance(WorldObject const* other)const{return other->distance;}
};
struct Player;
struct GroupReference {Player* player=nullptr;GroupReference* following=nullptr;Player* getSource(){return player;}GroupReference* next(){return following;}};
struct Group {bool raid=false;GroupReference* first=nullptr;bool isRaidGroup()const{return raid;}GroupReference* GetFirstMember(){return first;}};
struct World {float getFloatConfig(uint32 identity)const{assert(identity==CONFIG_GROUP_XP_DISTANCE);return 50;}} world;
auto* sWorld=&world;
struct CriteriaTree {};
struct Achievements {bool IsCompletedCriteriaTree(CriteriaTree const*)const{return false;}CriteriaTree const* GetCriteriaTree(uint32)const{return nullptr;}} achievements;
auto* sAchievementMgr=&achievements;
struct QuestStatusData {uint32 Status=QUEST_STATUS_INCOMPLETE,Timer=100;std::map<int8,int32> ObjectiveData;};
struct Player:WorldObject {
 uint32 activeQuest=39191,klass=CLASS_WARRIOR,updates=0,completions=0;bool active=true,contact=true,teleporting=false,alive=true;
 WorldObject* corpse=nullptr;Group* membership=nullptr;QuestStatusData questState;
 std::set<uint32> items,m_RewardedQuests;std::map<uint32,uint32> m_QuestStatusSave;Achievements* m_achievementMgr=&achievements;
 bool CanContact()const{return contact;}bool IsBeingTeleported()const{return teleporting;}uint32 getClass()const{return klass;}
 bool GetQuestRewardStatus(uint32 identity)const{return m_RewardedQuests.count(identity)!=0;}
 bool HasItemCount(uint32 identity,uint32 count,bool bank)const{assert(count==1&&bank);return items.count(identity)!=0;}
 bool IsAlive()const{return alive;}WorldObject const* GetCorpse()const{return corpse;}bool InInstance()const{return true;}Group* GetGroup(){return membership;}
 bool TalkedToCreature(uint32,ObjectGuid){return false;}
 std::string GetName()const{return "fixture";}
 QuestStatusData* getQuestStatus(uint32 identity){return active&&identity==activeQuest?&questState:nullptr;}
 uint32 GetQuestSlotQuestId(uint8 slot)const{return active&&slot==0?activeQuest:0;}
 uint16 FindQuestSlot(uint32)const{return 0;}void SetQuestSlotCounter(uint16,int8,int32){}
 void SetQuestSlotState(uint16,uint32){}void RemoveQuestSlotState(uint16,uint32){}void SetSpecialCriteriaComplete(uint16,int8){}
 void SendDirectMessage(void*){assert(false);}
 bool IsQuestWeekRewarded(uint32)const{return false;}bool IsQuestSeasonalRewarded(uint32)const{return false;}bool CanTakeQuest(Quest const*,bool)const{return false;}
 struct Reputation {int32 GetReputation(int32)const{return 0;}} reputation;auto const& GetReputationMgr()const{return reputation;}
 bool HasEnoughMoney(uint64)const{return false;}bool HasSpell(int32)const{return false;}bool HasCurrency(int32,int32)const{return false;}
 int32 GetQuestObjectiveData(Quest const*,int8)const;
 void SetQuestObjectiveData(Quest const*,QuestObjective const*,int32,bool=false);bool HasQuestObjectiveComplete(Quest const*,QuestObjective const&);bool CanCompleteQuest(uint32);
 void RewardPlayerAndGroupAtEvent(uint32,WorldObject*);bool IsAtGroupRewardDistance(WorldObject const*)const;
 void KilledMonsterCredit(uint32,ObjectGuid=ObjectGuid::Empty);
 void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,uint32){++updates;}
 void CompleteQuest(uint32){questState.Status=QUEST_STATUS_COMPLETE;++completions;}void SetQuestStatus(uint32,uint32 value){questState.Status=value;}
};
namespace WorldPackets {namespace Quest {struct QuestUpdateAddCreditSimple {uint32 QuestID=0,ObjectID=0,ObjectiveType=0;void* Write(){return nullptr;}};}}
#include "Native.inc"
#include "Policy.inc"
void Player::KilledMonsterCredit(uint32 entry,ObjectGuid guid)
{
 uint32 real_entry=entry;uint16 addKillCount=1;
#include "Credit.inc"
}
uint32 Item(uint32 quest){return quest==39191?128289:quest==40043?128908:128910;}
Player Ready(uint32 identity)
{
 Player player;player.activeQuest=identity;player.items.insert(Item(identity));
 for(auto const& objective:store.quests.at(identity).objectives)
  if(objective.ObjectID!=103739&&!(objective.Flags&QUEST_OBJECTIVE_FLAG_OPTIONAL))player.questState.ObjectiveData[objective.StorageIndex]=objective.Amount;
 return player;
}
int32 ReturnCount(Player const& player){return player.GetQuestObjectiveData(&store.quests.at(player.activeQuest),2);}
void Load(char const* path)
{
 std::ifstream input(path);QuestObjective objective;int type,slot;
 while(input>>objective.ID>>objective.QuestID>>type>>slot>>objective.ObjectID>>objective.Amount>>objective.Flags){objective.Type=uint8(type);objective.StorageIndex=int8(slot);auto& quest=store.quests[objective.QuestID];quest.identity=objective.QuestID;quest.objectives.push_back(objective);}
 assert(store.quests.size()==3);
}
int main(int argc,char** argv)
{
 assert(argc==2);Load(argv[1]);unsigned missingCases=0;
 for(uint32 identity:{39191u,40043u,41105u})
 {
  auto const original=store.quests.at(identity);
  {Player first;first.activeQuest=identity;first.world=false;first.mapId=1220;auto objective=original.objectives.front();first.KilledMonsterCredit(objective.ObjectID);assert(first.GetQuestObjectiveData(&original,objective.StorageIndex)==1);}
  {auto raid=Ready(identity);Group group;group.raid=true;raid.membership=&group;raid.KilledMonsterCredit(103739);assert(ReturnCount(raid)==0);}
  for(auto const& objective:original.objectives)if(objective.ObjectID!=103739&&!(objective.Flags&QUEST_OBJECTIVE_FLAG_OPTIONAL))
  {
   auto player=Ready(identity);player.questState.ObjectiveData[objective.StorageIndex]=0;player.KilledMonsterCredit(103739);++missingCases;
#ifdef BASELINE
   assert(ReturnCount(player)==1);
#else
   assert(ReturnCount(player)==0&&player.updates==0&&player.completions==0);
   player.questState.ObjectiveData[objective.StorageIndex]=objective.Amount;player.KilledMonsterCredit(103739);assert(ReturnCount(player)==1&&player.completions==1);
#endif
  }
  for(unsigned invalid=0;invalid<13;++invalid)
  {
   auto player=Ready(identity);
   switch(invalid){case 0:player.world=false;break;case 1:player.contact=false;break;case 2:player.map=nullptr;break;case 3:player.mapId=1220;break;case 4:player.teleporting=true;break;case 5:player.klass=3;break;case 6:player.items.clear();break;case 7:player.m_RewardedQuests.insert(identity);break;case 8:player.questState.Status=0;break;case 9:player.active=false;break;case 10:player.items={Item(identity)==128289?128908u:128289u};break;case 11:player.questState.Status=5;break;case 12:player.questState.Status=QUEST_STATUS_COMPLETE;break;}
   player.KilledMonsterCredit(103739);
#ifdef BASELINE
   assert(ReturnCount(player)==int32(invalid!=8&&invalid!=9&&invalid!=11&&invalid!=12));
#else
   assert(ReturnCount(player)==0&&player.updates==0&&player.completions==0);
#endif
  }
  auto optionals=std::count_if(original.objectives.begin(),original.objectives.end(),[](QuestObjective const& objective){return objective.Flags&QUEST_OBJECTIVE_FLAG_OPTIONAL;});
  for(unsigned optional=0;optional<(1u<<optionals);++optional)
  {
   auto player=Ready(identity);unsigned index=0;
   for(auto const& objective:original.objectives)if(objective.Flags&QUEST_OBJECTIVE_FLAG_OPTIONAL){if(optional&(1u<<index))player.questState.ObjectiveData[objective.StorageIndex]=objective.Amount;++index;}
   player.KilledMonsterCredit(103739);assert(ReturnCount(player)==1&&player.completions==1);auto updates=player.updates;player.KilledMonsterCredit(103739);assert(player.updates==updates&&player.completions==1);
  }
  auto eligible=Ready(identity),unready=Ready(identity),missing=Ready(identity),dead=Ready(identity),released=Ready(identity),far=Ready(identity),otherMap=Ready(identity),otherInstance=Ready(identity);
  unready.questState.ObjectiveData.clear();missing.items.clear();dead.alive=false;released.alive=false;released.corpse=&released;far.distance=51;otherMap.mapId=1220;otherInstance.instanceId=43;
  GroupReference eighth{&otherInstance},seventh{&otherMap,&eighth},sixth{&far,&seventh},fifth{&released,&sixth},fourth{&dead,&fifth},third{&missing,&fourth},second{&unready,&third},first{&eligible,&second};Group group;group.first=&first;eligible.membership=&group;
  eligible.RewardPlayerAndGroupAtEvent(103739,&eligible);assert(ReturnCount(eligible)==1&&ReturnCount(dead)==1&&ReturnCount(released)==0&&ReturnCount(far)==0&&ReturnCount(otherMap)==0&&ReturnCount(otherInstance)==0);
  liveMap.dungeon=true;eligible.RewardPlayerAndGroupAtEvent(103739,&eligible);assert(ReturnCount(far)==1);liveMap.dungeon=false;
#ifdef BASELINE
  assert(ReturnCount(unready)==1&&ReturnCount(missing)==1);
#else
  assert(ReturnCount(unready)==0&&ReturnCount(missing)==0);
  for(auto changed:{0,1}){auto& quest=store.quests.at(identity);quest=original;quest.objectives.erase(std::remove_if(quest.objectives.begin(),quest.objectives.end(),[changed](QuestObjective const& objective){return changed?objective.ObjectID==103739:objective.StorageIndex==1;}),quest.objectives.end());auto player=Ready(identity);player.KilledMonsterCredit(103739);assert(!player.updates);}
  store.quests[identity]=original;
  {auto& quest=store.quests.at(identity);std::reverse(quest.objectives.begin(),quest.objectives.end());auto player=Ready(identity);player.KilledMonsterCredit(103739);assert(ReturnCount(player)==1);quest=original;}
  {auto& quest=store.quests.at(identity);auto& objective=*std::find_if(quest.objectives.begin(),quest.objectives.end(),[](QuestObjective const& value){return value.ObjectID!=103739&&!(value.Flags&QUEST_OBJECTIVE_FLAG_OPTIONAL);});objective.Amount=2;auto player=Ready(identity);player.questState.ObjectiveData[objective.StorageIndex]=1;player.KilledMonsterCredit(103739);assert(ReturnCount(player)==0);quest=original;}
#endif
 }
 assert(missingCases==6);
 Quest unrelated;unrelated.identity=999;unrelated.objectives={{1,999,0,2,103739,1,2}};store.quests[999]=unrelated;Player other;other.activeQuest=999;other.world=false;other.mapId=1220;other.KilledMonsterCredit(103739);assert(ReturnCount(other)==1);
#ifdef BASELINE
 std::cout<<"BASELINE: actual native quest-credit loop records103739 for all six missing required objectives, missing/wrong artifacts and eligible but unready group members. Flags2 do not gate server counters.\n";
#else
 std::cout<<"PASS: actual native quest-credit loop/get/set/completion plus native group/distance enforce three Skyhold chains per recipient; six missing prerequisites and39 invalid contexts reject, retries and six optional combinations complete once. Dead native-eligible group member retained, released/far/unready/wrong-item rejected; unrelated quest unchanged. No faction filter added; inventory/world services controlled, not live faction/client travel or render.\n";
#endif
}
