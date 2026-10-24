#include <algorithm>
#include <array>
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
enum { QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,QUEST_DEFAULT_SAVE_TYPE=0,MAX_QUEST_LOG_SIZE=25,
 QUEST_OBJECTIVE_MONSTER=0,QUEST_OBJECTIVE_ITEM=1,QUEST_OBJECTIVE_GAMEOBJECT=2,QUEST_OBJECTIVE_TALKTO=3,
 QUEST_OBJECTIVE_CURRENCY=4,QUEST_OBJECTIVE_LEARNSPELL=5,QUEST_OBJECTIVE_MIN_REPUTATION=6,QUEST_OBJECTIVE_MAX_REPUTATION=7,
 QUEST_OBJECTIVE_MONEY=8,QUEST_OBJECTIVE_PLAYERKILLS=9,QUEST_OBJECTIVE_AREATRIGGER=10,QUEST_OBJECTIVE_PET_TRAINER_DEFEAT=11,
 QUEST_OBJECTIVE_COMPLETE_CRITERIA_TREE=12,QUEST_OBJECTIVE_TASK_IN_ZONE=13,QUEST_OBJECTIVE_HAVE_CURRENCY=14,QUEST_OBJECTIVE_OBTAIN_CURRENCY=15,
 QUEST_OBJECTIVE_FLAG_OPTIONAL=4,QUEST_OBJECTIVE_FLAG_HIDE_ITEM_GAINS=16,QUEST_OBJECTIVE_FLAG_PART_OF_PROGRESS_BAR=64,
 QUEST_SPECIAL_FLAGS_KILL=1,QUEST_SPECIAL_FLAGS_CAST=2,QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT=4,QUEST_SPECIAL_FLAGS_TIMED=8,
 CHAR_REP_CHAR_QUESTSTATUS_OBJECTIVES=1 };
struct Field {
 int64_t value=0;std::string text;
 uint32 GetUInt32() const{return uint32(value);}uint8 GetUInt8() const{return uint8(value);}int8 GetInt8() const{return int8(value);}
 int32 GetInt32() const{return int32(value);}float GetFloat() const{return float(value);}bool GetBool() const{return bool(value);}
 std::string GetString() const{return text;}
};
struct QuestObjective {
 uint32 ID=0,QuestID=0,Flags=0,Flags2=0;uint8 Type=0;int8 StorageIndex=0;int32 ObjectID=0,Amount=1;float TaskStep=0;std::string Description;bool Bugged=false;std::vector<int32> VisualEffects;
 bool IsStoringFlag() const{return false;}
};
struct Quest {
 uint32 identity=41105;std::vector<QuestObjective> Objectives;
 void LoadQuestObjective(Field*);
 uint32 GetQuestId() const{return identity;}auto const& GetObjectives() const{return Objectives;}
 bool IsWeekly() const{return false;}bool IsSeasonal() const{return false;}bool IsRepeatable() const{return false;}bool IsAutoComplete() const{return false;}
 bool HasSpecialFlag(uint32 flags) const{return (flags&QUEST_SPECIAL_FLAGS_KILL)!=0;}bool IsAllowedInRaid(uint32) const{return false;}
};
#include "ObjectiveLoader.inc"
struct QuestObjectivesLocale {std::vector<std::string> Description;};
struct ObjectMgr {static void GetLocaleString(std::vector<std::string> const&,int,std::string&){assert(false);}};
struct Store {Quest const* quest=nullptr;Quest const* GetQuestTemplate(uint32 identity) const{return quest&&quest->identity==identity?quest:nullptr;}QuestObjectivesLocale const* GetQuestObjectivesLocale(uint32) const{return nullptr;}} store;
auto* sQuestDataStore=&store;
struct QuestStatusData {uint32 Status=QUEST_STATUS_INCOMPLETE,Timer=100;std::map<int8,int32> ObjectiveData;};
struct CriteriaTree {};
struct Achievements {bool IsCompletedCriteriaTree(CriteriaTree const*) const{return false;}CriteriaTree const* GetCriteriaTree(uint32) const{return nullptr;}} achievements;
auto* sAchievementMgr=&achievements;
struct PreparedStatement {
 std::array<int64_t,5> values{};
 void setUInt64(unsigned slot,uint64 value){values[slot]=int64_t(value);}void setUInt32(unsigned slot,uint32 value){values[slot]=value;}
 void setInt8(unsigned slot,int8 value){values[slot]=value;}void setInt32(unsigned slot,int32 value){values[slot]=value;}
};
struct Database {PreparedStatement* GetPreparedStatement(uint32 identity){assert(identity==CHAR_REP_CHAR_QUESTSTATUS_OBJECTIVES);return new PreparedStatement;}} CharacterDatabase;
struct Transaction {std::map<int8,int32> rows;void Append(PreparedStatement* statement){rows[int8(statement->values[2])]=int32(statement->values[3]);delete statement;}};
struct Query {
 std::vector<std::array<Field,3>> rows;size_t index=0;
 Field* Fetch(){return rows.at(index).data();}bool NextRow(){return ++index<rows.size();}
};
struct PreparedQueryResult {
 Query* result=nullptr;operator bool() const{return result&&!result->rows.empty();}Query* operator->(){return result;}
};
struct Player {
 QuestStatusData status;bool active=true,raid=false;std::set<uint32> m_RewardedQuests;std::map<uint32,uint32> m_QuestStatusSave;Achievements* m_achievementMgr=&achievements;
 std::map<int8,int32> counters;uint32 updates=0,completions=0;
 struct Guid {std::string ToString() const{return "fixture";}};Guid GetGUID() const{return {};}
 std::string GetName() const{return "fixture";}uint64 GetGUIDLow() const{return 1;}
 struct Session {uint32 GetAccountId() const{return 1;}} session;Session* GetSession(){return &session;}
 QuestStatusData* getQuestStatus(uint32 identity){return active&&identity==41105?&status:nullptr;}
 uint16 FindQuestSlot(uint32) const{return 0;}void SetQuestSlotCounter(uint16,int8 slot,int32 value){counters[slot]=value;}
 void SetQuestSlotState(uint16,uint32){}void RemoveQuestSlotState(uint16,uint32){}void SetSpecialCriteriaComplete(uint16,int8){}
 void SendDirectMessage(void*){assert(false);}
 bool IsQuestWeekRewarded(uint32) const{return false;}bool IsQuestSeasonalRewarded(uint32) const{return false;}bool CanTakeQuest(Quest const*,bool) const{return false;}
 struct Reputation {int32 GetReputation(int32) const{return 0;}} reputation;auto const& GetReputationMgr() const{return reputation;}
 bool HasEnoughMoney(uint64) const{return false;}bool HasSpell(int32) const{return false;}bool HasCurrency(int32,int32) const{return false;}
 struct Group {bool isRaidGroup() const{return true;}} group;Group* GetGroup(){return raid?&group:nullptr;}
 struct Map {uint32 GetDifficultyID() const{return 0;}} map;Map* GetMap(){return &map;}
 int32 GetQuestObjectiveData(uint32,uint32) const;int32 GetQuestObjectiveData(Quest const*,int8) const;
 void SetQuestObjectiveData(Quest const*,QuestObjective const*,int32,bool=false);bool HasQuestObjectiveComplete(Quest const*,QuestObjective const&);bool CanCompleteQuest(uint32);
 void _LoadQuestStatusObjectives(PreparedQueryResult);
 void SendQuestUpdateAddCredit(Quest const*,Guid,QuestObjective const&,uint32){++updates;}
 void CompleteQuest(uint32){status.Status=QUEST_STATUS_COMPLETE;++completions;}void SetQuestStatus(uint32,uint32 value){status.Status=value;}
 void Credit(uint32 real_entry){uint16 addKillCount=1;auto questid=41105u;auto qInfo=store.quest;auto q_status=getQuestStatus(questid);Guid guid;
  if(!qInfo||!q_status)return;
#include "Credit.inc"
 }
 Transaction Save(){Transaction transaction;auto trans=&transaction;auto quest=store.quest;uint64 guid=1;auto saveItr=m_QuestStatusSave.begin();PreparedStatement* stmt=nullptr;
  if(saveItr==m_QuestStatusSave.end())return transaction;
#include "Save.inc"
  return transaction;
 }
};
namespace WorldPackets {namespace Quest {struct QuestUpdateAddCreditSimple {uint32 QuestID=0,ObjectID=0,ObjectiveType=0;void* Write(){return nullptr;}};}}
#include "PlayerObjectives.inc"
std::vector<int64_t> Wire(Quest const* quest){
 struct Packet {struct InfoData {std::vector<QuestObjective> Objectives;} Info;} packet;
 struct Session {int GetSessionDbLocaleIndex() const{return 0;}} session;auto* _session=&session;
#include "QueryObjectives.inc"
 auto& Info=packet.Info;
 struct Sink {std::vector<int64_t> values;Sink& operator<<(uint32 value){values.push_back(value);return *this;}Sink& operator<<(int32 value){values.push_back(value);return *this;}Sink& operator<<(uint8 value){values.push_back(value);return *this;}Sink& operator<<(int8 value){values.push_back(value);return *this;}Sink& operator<<(float value){values.push_back(int64_t(value));return *this;}void WriteString(std::string const&,int){}} _worldPacket;
#include "WriteObjectives.inc"
 return _worldPacket.values;
}
std::vector<std::array<Field,11>> Read(char const* path){
 std::ifstream input(path);std::vector<std::array<Field,11>> rows;std::array<Field,11> row;
 while(input>>row[0].value>>row[1].value>>row[2].value>>row[3].value>>row[4].value>>row[5].value>>row[6].value>>row[7].value>>row[8].value>>row[10].value)rows.push_back(row);
 assert(rows.size()==5);return rows;
}
Quest Load(std::vector<std::array<Field,11>> rows){Quest quest;for(auto& row:rows)quest.LoadQuestObjective(row.data());return quest;}
int main(int argc,char** argv){
 assert(argc==3);auto before=Read(argv[1]);auto after=Read(argv[2]);auto baseline=Load(before);store.quest=&baseline;
 Player alias;alias.Credit(102394);assert(alias.GetQuestObjectiveData(41105,103739)==1);alias.Credit(113105);alias.Credit(103114);assert(alias.completions==1);
 auto fixed=Load(after);store.quest=&fixed;
 std::vector<uint32> expected{281862,286343,286344,281863,282354};std::vector<uint32> actual;
 for(auto const& objective:fixed.Objectives)actual.push_back(objective.ID);
#ifdef BASELINE
 assert(actual!=expected);std::cout<<"PASS: native old loader orders corrected save slots as story order; native optional portal credit aliases return in the old rows and completes after tomb/boss without a return\n";return 0;
#else
 assert(actual==expected);
 auto encoded=Wire(&fixed);assert(encoded.size()==45);for(size_t index=0;index<5;++index){assert(encoded[index*9]==expected[index]);assert(encoded[index*9+2]==fixed.Objectives[index].StorageIndex);}
 std::sort(after.begin(),after.end(),[](auto const& left,auto const& right){return left[0].value<right[0].value;});
 unsigned permutations=0;
 do{auto reordered=Load(after);actual.clear();for(auto const& objective:reordered.Objectives)actual.push_back(objective.ID);assert(actual==expected);++permutations;}
 while(std::next_permutation(after.begin(),after.end(),[](auto const& left,auto const& right){return left[0].value<right[0].value;}));assert(permutations==120);
 for(uint32 optionalMask=0;optionalMask<4;++optionalMask){
  Player player;if(optionalMask&1)player.Credit(97377);if(optionalMask&2)player.Credit(102394);
  assert(player.GetQuestObjectiveData(41105,103739)==0&&!player.CanCompleteQuest(41105));
  player.Credit(113105);assert(!player.CanCompleteQuest(41105));player.Credit(103114);assert(!player.CanCompleteQuest(41105));
  auto saved=player.Save();Query query;for(auto const& record:saved.rows){std::array<Field,3> row;row[0].value=41105;row[1].value=record.first;row[2].value=record.second;query.rows.push_back(row);}
  Player loaded;loaded._LoadQuestStatusObjectives({&query});assert(!loaded.CanCompleteQuest(41105)&&loaded.GetQuestObjectiveData(41105,103739)==0);
  loaded.Credit(103739);assert(loaded.completions==1);auto updates=loaded.updates;loaded.Credit(103739);assert(loaded.completions==1&&loaded.updates==updates);
 }
 for(uint32 missing:{113105u,103114u,103739u}){Player player;for(uint32 credit:{113105u,103114u,103739u})if(credit!=missing)player.Credit(credit);assert(!player.CanCompleteQuest(41105));}
 Player absent;absent.active=false;absent.Credit(103739);assert(absent.status.ObjectiveData.empty());Player raid;raid.raid=true;raid.Credit(103739);assert(raid.status.ObjectiveData.empty());
 Player rewarded;rewarded.m_RewardedQuests.insert(41105);assert(!rewarded.CanCompleteQuest(41105));
 Quest other;other.identity=999;for(auto& row:after){row[1].value=999;other.LoadQuestObjective(row.data());}for(size_t index=0;index<after.size();++index)assert(other.Objectives[index].ID==after[index][0].value);
 Quest blood;auto first=after[0];first[1].value=40729;first[0].value=99;blood.LoadQuestObjective(first.data());first[0].value=281139;blood.LoadQuestObjective(first.data());assert(blood.Objectives.front().ID==281139);
 Quest custom=fixed;first[1].value=41105;for(uint32 identity:{999998u,999999u}){first[0].value=identity;custom.LoadQuestObjective(first.data());}assert(custom.Objectives[5].ID==999998&&custom.Objectives[6].ID==999999);
 std::cout<<"PASS: native41105 loader preserves story order for120 input permutations; actual query/serialization loops retain order and independent slots. Native credit/get/set/completion/save/load keeps optional travel separate, requires all three real goals and retains repeat/status/raid/NewBlood/other-quest behavior. Services are controlled, not live actors, pet/group dispatch or client traversal\n";
#endif
}
