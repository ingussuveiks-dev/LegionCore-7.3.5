#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;using int64=int64_t;
enum QuestStatus{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED};
enum{CONDITION_QUESTTAKEN=9,CONDITION_QUEST_OBJECTIVE_DONE=41};
struct Player;
struct Unit{Player* player=nullptr;Unit* owner=nullptr;bool creature=false,pet=false;
 Player* ToPlayer(){return player;}bool IsCreature(){return creature;}bool isPet(){return pet;}Unit* ToUnit(){return this;}Unit* GetOwner(){return owner;}
 Unit& getThreatManager(){return *this;}Unit* getHostilTarget(){return nullptr;}
};
struct Player:Unit{std::map<int,QuestStatus> status;std::map<std::pair<int,int>,int> progress;
 Player(){player=this;}
 QuestStatus GetQuestStatus(int q){return status[q];}uint32 GetQuestObjectiveData(int q,int o){return progress[{q,o}];}
};
struct Condition{int source,group,entry,id,elseGroup,type,ConditionValue1,ConditionValue2,ConditionValue3,negative;
 bool Meets(Unit* object){bool condMeets=false;switch(type){
#include "Taken.inc"
#include "Objective.inc"
 default:assert(false);}
 return negative?!condMeets:condMeets;}
};
using ConditionList=std::vector<Condition*>;using ConditionTypeContainer=std::map<uint32,ConditionList>;using SmartEventConditionContainer=std::map<std::pair<int64,uint32>,ConditionTypeContainer>;
#define TC_LOG_DEBUG(...) ((void)0)
struct ConditionMgr{SmartEventConditionContainer SmartEventConditionStore;ConditionList GetConditionsForSmartEvent(int64,uint32,uint32);};
#include "Lookup.inc"
bool Allowed(ConditionList const& list,Unit* player){assert(!list.empty());for(auto c:list)if(!c->Meets(player))return false;return true;}
int main(int argc,char**argv){assert(argc==2);std::ifstream input(argv[1]);std::string tag;std::vector<Condition> rows;Condition c;
 while(input>>tag>>c.source>>c.group>>c.entry>>c.id>>c.elseGroup>>c.type>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.negative)rows.push_back(c);
 assert(rows.size()==13);ConditionMgr mgr;std::map<int,ConditionList> clicks;
 for(auto& row:rows)if(row.source==22)mgr.SmartEventConditionStore[{row.entry,uint32(row.id)}][row.group].push_back(&row);else clicks[row.group].push_back(&row);
 Player p;
 auto intro=mgr.GetConditionsForSmartEvent(309,0,13);assert(!Allowed(intro,&p));assert(mgr.GetConditionsForSmartEvent(309,0,10).empty());
 p.status[41705]=QUEST_STATUS_INCOMPLETE;assert(Allowed(intro,&p));p.status[41705]=QUEST_STATUS_REWARDED;assert(!Allowed(intro,&p));
 for(int entry:{107988,108081}){auto list=clicks[entry];assert(!Allowed(list,&p));p.status[42630]=QUEST_STATUS_INCOMPLETE;assert(Allowed(list,&p));p.status[42630]=QUEST_STATUS_COMPLETE;assert(!Allowed(list,&p));p.status[42630]=QUEST_STATUS_NONE;}
 for(int entry:{117453,117520}){auto hit=mgr.GetConditionsForSmartEvent(entry,0,0);for(int count=0;count<4;++count){p.progress[{45572,117454}]=count;
  p.status[45572]=QUEST_STATUS_NONE;assert(!Allowed(clicks[entry],&p)&&!Allowed(hit,&p));
  p.status[45572]=QUEST_STATUS_INCOMPLETE;assert(Allowed(clicks[entry],&p)==(count<3)&&Allowed(hit,&p)==(count<3));}}
 auto legion=mgr.GetConditionsForSmartEvent(368,0,13);p.status[45572]=QUEST_STATUS_NONE;assert(!Allowed(legion,&p));p.status[45572]=QUEST_STATUS_INCOMPLETE;assert(Allowed(legion,&p));p.progress[{45572,117473}]=1;assert(!Allowed(legion,&p));
 std::cout<<"PASS: twice-replayed migration, native SmartEvent source/event lookup, actual quest/objective conditions, both launcher gates, both rescues and both conversations\n";
}
