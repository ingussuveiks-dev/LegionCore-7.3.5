#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
#include "QuestType.inc"
#include "QuestInfo.inc"
#include "QuestStatus.inc"
#include "QuestObjectiveType.inc"
#include "__QuestFlags.inc"
#include "__QuestSpecialFlags.inc"
#include "QuestObjectiveFlags.inc"
#include "WorldQuestInfo.inc"
constexpr uint16 MAX_QUEST_LOG_SIZE=25,QUEST_STATE_COMPLETE=1;
constexpr uint32 MINUTE=60,EQUIP_ERR_ITEM_NOT_FOUND=1,CHAR_INS_CHARACTER_WORLDQUESTSTATUS=1;
#define TC_LOG_DEBUG(...) ((void)0)
void Check(bool v,char const*why){if(!v)throw std::runtime_error(why);}
namespace GameTime{uint32 now=1000;uint32 GetGameTime(){return now;}}
struct ObjectGuid{using LowType=uint64;};
struct QuestObjective{uint32 Type=QUEST_OBJECTIVE_MONSTER,Flags=0,ObjectID=115845,Amount=10;};
struct Quest{
 uint32 id=44923,Type=QUEST_TYPE_TASK,QuestInfoID=QUEST_INFO_RARE_WORLD_QUEST,flags=0,special=0;
 std::vector<QuestObjective> objectives{{},{QUEST_OBJECTIVE_MONSTER,0,115846,1}};
 uint32 GetQuestId()const{return id;}
 bool IsWorld()const{return QuestInfoID>=QUEST_INFO_WORLD_QUEST&&QuestInfoID<=QUEST_INFO_MAX&&Type==QUEST_TYPE_TASK;}
 bool IsEmissary()const{return QuestInfoID==QUEST_INFO_EMISSARY_QUEST;}
 bool HasFlag(uint32 f)const{return (flags&f)!=0;}bool HasSpecialFlag(uint32 f)const{return (special&f)!=0;}
 bool IsDFQuest()const{return false;}bool IsAutoComplete()const{return Type==QUEST_TYPE_AUTOCOMPLETE;}
 bool IsWeekly()const{return false;}bool IsSeasonal()const{return false;}bool IsRepeatable()const{return false;}
 auto const&GetObjectives()const{return objectives;}
};
struct WorldQuest{void*worldQuest=reinterpret_cast<void*>(1);uint32 ResetTime=2000;};
struct Store{
 Quest quest;WorldQuest active;bool present=true;
 Quest const*GetQuestTemplate(uint32 id){return id==quest.id?&quest:nullptr;}
 WorldQuest const*GetWorldQuest(Quest const*){return present?&active:nullptr;}
} store;
auto sQuestDataStore=&store;
struct Field{uint64 v;uint32 GetUInt32()const{return uint32(v);}};
struct Result{std::vector<std::vector<Field>>rows;size_t index=0;Field*Fetch(){return rows.at(index).data();}bool NextRow(){return ++index<rows.size();}};
using PreparedQueryResult=std::shared_ptr<Result>;
struct CharacterDatabasePreparedStatement{
 uint64 values[4]={};void setUInt64(int i,uint64 v){values[i]=v;}void setUInt32(int i,uint32 v){values[i]=v;}
};
struct Transaction{std::vector<std::unique_ptr<CharacterDatabasePreparedStatement>>rows;void Append(CharacterDatabasePreparedStatement*s){rows.emplace_back(s);}};
using CharacterDatabaseTransaction=std::shared_ptr<Transaction>;
struct Database{CharacterDatabasePreparedStatement*GetPreparedStatement(uint32 id){Check(id==CHAR_INS_CHARACTER_WORLDQUESTSTATUS,"wrong statement");return new CharacterDatabasePreparedStatement;}} CharacterDatabase;
struct DB2{uint32 GetQuestUniqueBitFlag(uint32 id){return id;}}sDB2Manager;
struct Session{uint32 GetAccountId()const{return 77;}};
struct QuestStatusData{QuestStatus Status=QUEST_STATUS_INCOMPLETE;uint32 Timer=1;};
struct Object{};struct Player;
struct Scripts{int completions=0;void OnQuestComplete(Player*,Quest const*){++completions;}}scripts;
auto sScriptMgr=&scripts;
struct Player:Object{
 using WorldQuestStatusMap=std::map<uint32,WorldQuestInfo>;
 WorldQuestStatusMap m_worldquests;std::set<uint32>m_RewardedQuests,bits;
 QuestStatusData state;Session session;bool accepted=true;
 int statusWrites=0,slotWrites=0,enteredReward=0;
 uint64 GetGUIDLow()const{return 0xF1234567ULL;}Session*GetSession(){return &session;}
 bool CanRewardWorldQuest(Quest const*)const;bool WorldQuestCompleted(uint32)const;
 bool CanCompleteQuest(uint32);bool CanRewardQuest(Quest const*,bool);void CompleteQuest(uint32);
 void RewardQuest(Quest const*quest,uint32,Object*,bool){
#include "RewardPrefix.inc"
  ++enteredReward;accepted=false; // Sentinel for entering the destructive remainder, not a reward simulation.
 }
 QuestStatus GetQuestStatus(uint32)const{return state.Status;}
 bool IsQuestDailyRewarded(uint32)const{return false;}bool IsQuestWeekRewarded(uint32)const{return false;}bool IsQuestSeasonalRewarded(uint32)const{return false;}
 bool GetQuestRewardStatus(uint32)const{return false;}uint32 GetItemCount(uint32)const{return 100;}
 void SendEquipError(uint32,void*,void*,uint32){}
 bool HasCurrency(uint32,uint32)const{return true;}bool HasEnoughMoney(uint64)const{return true;}
 bool CanTakeQuest(Quest const*,bool)const{return true;}
 QuestStatusData*getQuestStatus(uint32){return accepted?&state:nullptr;}
 bool HasQuestObjectiveComplete(Quest const*,QuestObjective const&)const{return true;}
 void SetQuestStatus(uint32,QuestStatus s){++statusWrites;state.Status=s;}
 uint16 FindQuestSlot(uint32)const{return 0;}void SetQuestSlotState(uint16,uint16){++slotWrites;}
 void SetQuestCompletedBit(uint32 id,bool b){if(b)bits.insert(id);else bits.erase(id);}
 void _LoadWorldQuestStatus(PreparedQueryResult);void _SaveWorldQuestStatus(CharacterDatabaseTransaction&);void ResetWorldQuest(uint32=0);
};
#include "Admission.inc"
#include "Completed.inc"
#include "CanComplete.inc"
#include "CanReward.inc"
#include "Complete.inc"
#include "Load.inc"
#include "Save.inc"
#include "Reset.inc"
int Run(){
 for(int kind=0;kind<5;++kind){
  store.present=kind!=0;store.active.worldQuest=kind==1?nullptr:reinterpret_cast<void*>(1);
  store.active.ResetTime=kind==2?999:kind==3?1000:2000;
  Player p;if(kind==4)p.m_worldquests[44923]={44923,2000,false};
  Check(!p.CanRewardWorldQuest(&store.quest),"invalid rotation gate");
#ifdef OLD_REWARD
  Check(p.CanCompleteQuest(44923),"baseline completion no longer reproduces");
  p.state.Status=QUEST_STATUS_COMPLETE;Check(p.CanRewardQuest(&store.quest,false),"baseline turn-in no longer reproduces");
  p.RewardQuest(&store.quest,0,&p,true);Check(p.enteredReward==1&&!p.accepted,"baseline direct entry no longer reproduces");
  p.CompleteQuest(44923);Check(p.statusWrites==1&&p.enteredReward==2,"baseline completion entry no longer reproduces");
#else
  // These existing predicates only inspect objective/status eligibility;
  // actual completion and reward must recheck the current rotation.
  Check(p.CanCompleteQuest(44923),"objective predicate changed");
  p.state.Status=QUEST_STATUS_COMPLETE;Check(p.CanRewardQuest(&store.quest,false),"turn-in predicate changed");
  p.state.Status=QUEST_STATUS_INCOMPLETE;
  p.RewardQuest(&store.quest,0,&p,true);p.CompleteQuest(44923);
  Check(p.accepted&&p.enteredReward==0&&!p.statusWrites&&!p.slotWrites,"rejected reward mutated state");
#endif
 }
 store.present=true;store.active={reinterpret_cast<void*>(1),1001};
 {Player p;Check(p.CanRewardWorldQuest(&store.quest)&&p.CanCompleteQuest(44923),"one second before expiry");
  p.CompleteQuest(44923);Check(p.statusWrites==1&&p.slotWrites==1&&p.enteredReward==1,"valid auto task dispatch");}
 {Player p;p.m_worldquests[44923]={44923,1000,false};Check(p.CanRewardWorldQuest(&store.quest),"old personal rotation blocks next rotation");}
 // Save parameters through the native methods, then project the emitted
 // fields into the native SELECT column order. SQL execution is a fixture leaf.
 {Player p;p.m_worldquests[44923]={44923,2000,true};auto tx=std::make_shared<Transaction>();p._SaveWorldQuestStatus(tx);
  Check(tx->rows.size()==1,"missing completion save");auto const&s=*tx->rows[0];
  Check(s.values[0]==0xF1234567ULL&&s.values[1]==44923&&s.values[2]==77&&s.values[3]==2000,"native completion save fields/unsigned GUID");
  std::cout<<"ROW "<<s.values[0]<<' '<<s.values[1]<<' '<<s.values[2]<<' '<<s.values[3]<<'\n';
  Check(!p.m_worldquests.at(44923).needSave,"save dirty flag");
  auto result=std::make_shared<Result>();result->rows={{{s.values[1]},{s.values[0]},{s.values[3]}}};
  Player login;login._LoadWorldQuestStatus(result);
  Check(login.WorldQuestCompleted(44923)&&!login.CanRewardWorldQuest(&store.quest)&&login.bits.count(44923),"login completion recovery");
  GameTime::now=2000;Check(!login.WorldQuestCompleted(44923),"exact personal expiry");login.ResetWorldQuest();Check(login.m_worldquests.empty(),"expired personal cleanup");GameTime::now=1000;}
 {Player p;auto result=std::make_shared<Result>();result->rows={{{99999},{1},{2000}},{{44923},{1},{2000}},{{44923},{1},{3000}}};p._LoadWorldQuestStatus(result);
  Check(p.m_worldquests.size()==1&&p.m_worldquests.at(44923).resetTime==2000,"unknown/duplicate loaded records");
  p.ResetWorldQuest(44923);Check(p.m_worldquests.empty(),"targeted reset");}
 store.quest.QuestInfoID=QUEST_INFO_ACCOUNT;
 {Player p;p.m_worldquests[44923]={44923,2000,true};auto tx=std::make_shared<Transaction>();p._SaveWorldQuestStatus(tx);Check(tx->rows[0]->values[0]==0,"account quest guid convention");}
 store.quest.Type=QUEST_TYPE_NORMAL;store.quest.QuestInfoID=0;store.present=false;
 {Player p;Check(p.CanRewardWorldQuest(&store.quest)&&p.CanCompleteQuest(44923),"ordinary quest blocked");p.CompleteQuest(44923);Check(p.statusWrites==1&&!p.enteredReward,"ordinary quest lost turn-in");}
 store.quest.Type=QUEST_TYPE_TASK;store.quest.QuestInfoID=QUEST_INFO_EMISSARY_QUEST;
 {Player p;Check(!p.CanRewardWorldQuest(&store.quest),"missing emissary rotation accepted");}
 Check(!Player{}.CanRewardWorldQuest(nullptr),"null quest accepted");
#ifdef OLD_REWARD
 std::cout<<"PASS: pinned caddeec5 reproduces completion/turn-in/direct-reward entry for five unavailable/already completed rotation cases.\n";
#else
 std::cout<<"PASS: missing/template-null/expired/exact-expiry/rewarded gates precede completion/direct-reward state writes; valid/ordinary/emissary boundaries and native save/load/reset survive.\n";
#endif
 return 0;
}
int main(){try{return Run();}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
