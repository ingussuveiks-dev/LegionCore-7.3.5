#include <cassert>
#include <cstdint>
#include <iostream>
#include <list>
#include <map>
using uint32=uint32_t;
constexpr uint32 SMART_ACTION_CLOSE_GOSSIP=72,SMART_EVENT_LINK=61;
constexpr uint32 CONDITION_QUESTTAKEN=9,CONDITION_QUEST_COMPLETE=28;
enum QuestStatus { QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_FAILED=5,QUEST_STATUS_REWARDED=6 };
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
struct Player;
struct Unit {
 virtual ~Unit()=default; virtual Player* ToPlayer(){return nullptr;}
 bool IsCreature(){return !ToPlayer();}Unit* ToUnit(){return this;}
 Unit* owner=nullptr;bool isPet(){return owner!=nullptr;}Unit* GetOwner(){return owner;}
 struct Threat {Unit* getHostilTarget(){return nullptr;}} threat;
 Threat& getThreatManager(){return threat;}
};
struct Player:Unit {
 struct Talk {int closed=0;void SendCloseGossip(){++closed;}} talk;
 Talk* PlayerTalkClass=&talk;QuestStatus status=QUEST_STATUS_NONE;
 Player* ToPlayer()override{return this;}
 QuestStatus GetQuestStatus(uint32 id){assert(id==41574);return status;}
 bool GetQuestRewardStatus(uint32 id){assert(id==41574);return status==QUEST_STATUS_REWARDED;}
};
bool QuestCondition(Unit* object,uint32 type) {
 bool condMeets=false;uint32 ConditionValue1=41574;
 switch(type){
#include "HunterQuestConditions.inc"
 }return condMeets;
}
using ObjectList=std::list<Unit*>;
struct SmartScriptHolder {
 uint32 event_id=0,link=0,entryOrGuid=100810,type=72,event=61;
 uint32 GetActionType(){return type;}uint32 GetEventType(){return event;}uint32 GetScriptType(){return 0;}
};
struct SmartScript {
 std::map<uint32,SmartScriptHolder> links;ObjectList selected;bool missing=false;int credits=0;
 bool IsPlayer(Unit* p){return p&&p->ToPlayer();}
 ObjectList* GetTargets(SmartScriptHolder&,Unit*){return missing?nullptr:new ObjectList(selected);}
 SmartScriptHolder FindLinkedEvent(uint32 id){return links.at(id);}
 void ProcessEvent(SmartScriptHolder& e,Unit* unit,uint32,uint32,bool,void*,void*){ProcessAction(e,unit);}
 void ProcessAction(SmartScriptHolder& e,Unit* unit=nullptr){
 uint32 var0=0,var1=0;bool bvar=false;void* spell=nullptr;void* gob=nullptr;
 switch(e.GetActionType()){
 case 33:++credits;break; // fixture for unchanged existing credit, not a native credit test
#include "HunterCloseGossip.inc"
 }
#include "HunterLinkTail.inc"
};
int main(){
 Player p;Unit pet;pet.owner=&p;
 for(auto status:{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_FAILED,QUEST_STATUS_REWARDED}){
  p.status=status;bool admitted=QuestCondition(&p,9)||QuestCondition(&p,28);
  assert(admitted==(status==QUEST_STATUS_INCOMPLETE||status==QUEST_STATUS_COMPLETE));
 }
 assert(!QuestCondition(&pet,9)&&!QuestCondition(&pet,28));
 for(uint32 entry:{100810,99986,100171}){
  SmartScript s;s.selected={&p,&pet};SmartScriptHolder root,next;
  root.entryOrGuid=next.entryOrGuid=entry;root.type=33;root.link=1;next.event_id=1;s.links[1]=next;
  int before=p.talk.closed;s.ProcessAction(root,&p);assert(s.credits==1&&p.talk.closed==before+1);
  s.selected={};s.ProcessAction(root);assert(s.credits==2&&p.talk.closed==before+1);
  s.missing=true;s.ProcessAction(root);assert(s.credits==3&&p.talk.closed==before+1);
 }
 std::cout<<"PASS: production gossip close/LINK and active/complete quest conditions; NONE/failed/rewarded and non-player refused, mixed/empty/null targets\n";
}
