#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
#define ASSERT assert
#define TC_LOG_DEBUG(...)
#include "Status.inc"
enum {INVALIDREASON_QUEST_FAILED_LOW_LEVEL,INVALIDREASON_DONT_HAVE_REQ,INVALIDREASON_QUEST_FAILED_WRONG_RACE};
using PrevQuests=std::vector<int32>;using ExclusiveQuestGroups=std::multimap<int32,uint32>;
struct Quest {uint32 id=0,MinLevel=0,MaxLevel=0;int32 AllowableClasses=0,ExclusiveGroup=0;uint64 AllowableRaces=uint64(-1);PrevQuests prevQuests;uint32 GetQuestId()const{return id;}};
struct QuestStore {std::map<uint32,Quest> rows;ExclusiveQuestGroups mExclusiveQuestGroups;Quest const* GetQuestTemplate(uint32 id)const{auto i=rows.find(id);return i==rows.end()?nullptr:&i->second;}} store;
auto sQuestDataStore=&store;
struct Player;
struct Unit {virtual Player* ToPlayer(){return nullptr;}Unit* ToUnit(){return this;}bool IsCreature()const{return false;}bool isPet()const{return false;}Unit* GetOwner(){return nullptr;}struct Threat{Unit* getHostilTarget(){return nullptr;}} threat;Threat& getThreatManager(){return threat;}};
struct FactionEntry{uint32 id;};struct Factions{FactionEntry row;FactionEntry const* LookupEntry(uint32 id){row.id=id;return &row;}} sFactionStore;
struct Player:Unit {
 uint32 level=106;uint64 races=uint64(1)<<9;uint32 classes=256;
 std::set<uint32> m_RewardedQuests;std::map<uint32,QuestStatus> active;
 std::map<std::pair<uint32,uint32>,uint32> objectives;
 struct Reps{std::map<uint32,uint32> ranks;uint32 GetRank(FactionEntry const* f){return ranks[f->id];}} reps;
 Player* ToPlayer()override{return this;}uint32 getLevel()const{return level;}uint32 getClassMask()const{return classes;}uint64 getRaceMask()const{return races;}
 QuestStatus GetQuestStatus(uint32 id)const{if(m_RewardedQuests.count(id))return QUEST_STATUS_REWARDED;auto i=active.find(id);return i==active.end()?QUEST_STATUS_NONE:i->second;}
 bool GetQuestRewardStatus(uint32 id)const{return m_RewardedQuests.count(id)!=0;}
 uint32 GetQuestObjectiveData(uint32 q,uint32 o)const{auto i=objectives.find({q,o});return i==objectives.end()?0:i->second;}
 Reps& GetReputationMgr(){return reps;}
 template<class... T>void SendCanTakeQuestResponse(T...)const{}
 bool SatisfyQuestLevel(Quest const*,bool);bool SatisfyQuestPreviousQuest(Quest const*,bool);
 bool SatisfyQuestClass(Quest const*,bool)const;bool SatisfyQuestRace(Quest const*,bool);
 bool CoreGates(Quest const* q){return GetQuestStatus(q->id)==QUEST_STATUS_NONE&&SatisfyQuestLevel(q,false)&&SatisfyQuestClass(q,false)&&SatisfyQuestRace(q,false)&&SatisfyQuestPreviousQuest(q,false);}
};
#include "Level.inc"
#include "Previous.inc"
#include "Class.inc"
#include "Race.inc"
enum {CONDITION_REPUTATION_RANK=5,CONDITION_QUESTREWARDED=8,CONDITION_QUESTTAKEN=9,CONDITION_QUEST_NONE=14,CONDITION_QUEST_COMPLETE=28,CONDITION_QUEST_OBJECTIVE_DONE=41};
struct Condition;
struct ConditionSourceInfo {Unit* object;Condition* mLastFailedCondition=nullptr;};
struct ScriptMgr{bool OnConditionCheck(Condition*,ConditionSourceInfo&){return true;}} scripts;
auto sScriptMgr=&scripts;
struct Condition {
 uint32 ElseGroup=0,ConditionType=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ReferenceId=0;
 bool NegativeCondition=false;bool isLoaded()const{return true;}
 bool Meets(ConditionSourceInfo& sourceInfo){Unit* object=sourceInfo.object;bool condMeets=false;switch(ConditionType){
#include "ConditionCases.inc"
 default:assert(false);}
#include "ConditionTail.inc"
 }
};
using ConditionList=std::vector<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;
struct ConditionMgr{ConditionReferenceContainer ConditionReferenceStore;bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;} conditions;
#include "ConditionList.inc"
std::map<std::pair<int,int>,std::vector<Condition>> rows;
bool Meets(Player& p,int source,int entry){auto found=rows.find({source,entry});if(found==rows.end())return true;ConditionList list;for(auto& c:found->second)list.push_back(&c);ConditionSourceInfo info{&p};return conditions.IsObjectMeetToConditionList(info,list);}
int main(int argc,char**argv){
 assert(argc==2);std::ifstream f(argv[1]);std::string type;
 std::vector<std::pair<uint32,int32>> next;
 while(f>>type){
  if(type=="Q"){Quest q;int32 prev,n;f>>q.id>>q.MinLevel>>q.MaxLevel>>q.AllowableClasses>>q.AllowableRaces>>q.ExclusiveGroup>>prev>>n;if(prev)q.prevQuests.push_back(prev);store.rows[q.id]=q;if(n)next.push_back({q.id,n});}
  else if(type=="C"){int source,entry;Condition c;f>>source>>entry>>c.ElseGroup>>c.ConditionType>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.NegativeCondition;rows[{source,entry}].push_back(c);}
  else assert(false);
 }
 for(auto pair:next){auto i=store.rows.find(abs(pair.second));if(i!=store.rows.end())i->second.prevQuests.push_back(pair.second<0?-int32(pair.first):int32(pair.first));}
 for(auto const& pair:store.rows)if(pair.second.ExclusiveGroup)store.mExclusiveQuestGroups.emplace(pair.second.ExclusiveGroup,pair.first);
 Player p;p.m_RewardedQuests={40821,40823,40824};
 auto take=[&](uint32 id){auto q=store.GetQuestTemplate(id);assert(q);return p.CoreGates(q)&&Meets(p,19,id);};
 assert(take(44099)&&!take(42608));p.m_RewardedQuests.insert(44099);assert(take(42608));
 p.active[42608]=QUEST_STATUS_INCOMPLETE;
 assert(Meets(p,23,4));assert(take(41748)&&take(41750));
 p.active[42608]=QUEST_STATUS_COMPLETE;assert(!take(41748)&&!take(41750));
 p.active.erase(42608);p.m_RewardedQuests.insert(42608);
 for(auto id:{42603,41797,42602,42601,42097,41759,39179,39389,39142,40218,41767,42100,42098}){
  assert(take(id));p.m_RewardedQuests.insert(id);
 }
 assert(Meets(p,23,4)&&Meets(p,23,5)&&Meets(p,23,6)&&Meets(p,23,7)&&Meets(p,23,9));
 assert(take(41752)&&take(41753)&&take(41798));
 p.active[41798]=QUEST_STATUS_INCOMPLETE;assert(Meets(p,23,10));
 // Level 110 content remains blocked at 106 even with completed prerequisites.
 assert(!take(39985)&&!take(43341));p.level=110;assert(take(39985));
 assert(!take(43341));for(auto faction:{1828,1859,1883,1900,1948})p.reps.ranks[faction]=4;
 assert(take(43341));p.reps.ranks[1859]=3;assert(!take(43341));
 // Previously chosen zones are not new offers; the other Horde routes remain.
 p.level=106;p.m_RewardedQuests.insert(39718);p.m_RewardedQuests.insert(39731);
 assert(!take(39718)&&!take(39731));assert(take(39733)&&take(39864));assert(!take(39735));
 assert(take(39861)&&take(41220));
 std::cout<<"PASS: native level/race/class/prerequisite gates and native condition grouping across the 101/103 Warlock chain, champion phases, zone follow-ups and level-110 reputation gates\n";
}
