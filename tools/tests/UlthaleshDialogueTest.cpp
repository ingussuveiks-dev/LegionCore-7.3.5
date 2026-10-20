#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using SMART_EVENT=uint32;using CriteriaTypes=uint32;
constexpr uint32 SMART_ACTION_TALK=1,SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA=205;
constexpr uint32 SMART_EVENT_LINK=61,SMART_EVENT_TEXT_OVER=52,SMART_EVENT_HEALTH_PCT=2;
constexpr uint32 SMART_TARGET_SELF=1,SMART_TARGET_VICTIM=2;
constexpr uint32 SMART_EVENT_FLAG_NOT_REPEATABLE=1,SMART_EVENT_FLAG_EVENT_NON_COMBAT=64,SMART_EVENT_FLAG_ALLOW_EVENT_IN_COMBAT=32;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) throw std::runtime_error("Invalid production LINK")
struct ObjectGuid{uint32 value=0;inline static constexpr uint32 Empty=0;ObjectGuid(uint32 id=0):value(id){}uint32 GetCounter(){return value;}};
struct Creature;struct Player;
struct Unit{
 virtual ~Unit()=default;virtual Creature* ToCreature(){return nullptr;}virtual Player* ToPlayer(){return nullptr;}
 ObjectGuid GetGUID(){return 1;}bool IsQuestObjectExpired(){return false;}const char* GetName(){return "fixture";}uint32 GetGUIDLow(){return 1;}
};
struct Creature:Unit{
 uint32 entry=100344,hp=1,maxHp=100000;bool combat=true,alive=true;Unit* victim=nullptr;
 Creature* ToCreature()override{return this;}uint32 GetEntry(){return entry;}Unit* getVictim(){return victim;}
 bool isInCombat(){return combat;}uint32 GetMaxHealth(){return maxHp;}float GetHealthPct(){return 100.f*hp/maxHp;}
};
struct Player:Unit{uint32 credits=0;Player* ToPlayer()override{return this;}
 void UpdateAchievementCriteria(CriteriaTypes type,uint32 asset,uint32,uint32,Unit*,bool){assert(type==92&&asset==47989);++credits;}
};
Player* GetScriptCreditPlayer(Unit* u){return u?u->ToPlayer():nullptr;}
using ObjectList=std::list<Unit*>;using ConditionList=int;
struct ConditionSourceInfo{ConditionSourceInfo(Unit*,Unit*){}};
struct Conditions{bool IsObjectMeetingSmartEventConditions(uint32,uint32,uint32,Unit*,Unit*){return true;}
 ConditionList GetConditionsForSmartEvent(uint32,uint32,uint32){return 0;}bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList){return true;}
}conditions;auto sConditionMgr=&conditions;
struct SpellInfo{};struct GameObject{};
struct SmartScriptHolder{
 uint32 event_id=0,link=0,type=0,targetType=0,entryOrGuid=100344,source_type=0;bool active=true,runOnce=false;
 struct{uint32 type=0,event_flags=1,event_phase_mask=0,event_chance=100;
 struct{uint32 min=0,max=0,repeatMin=0,repeatMax=0;}minMaxRepeat;
 struct{uint32 textGroupID=0,creatureEntry=0;}textOver;}event;
 struct{struct{uint32 textGroupID=0,duration=0;}talk;
 struct{uint32 type=0,misc1=0,misc2=0,misc3=0;}achievementCriteria;}action;
 uint32 GetActionType(){return type;}uint32 GetEventType()const{return event.type;}uint32 GetScriptType(){return source_type;}uint32 GetTargetType()const{return targetType;}
};
struct TextMgr{std::vector<std::pair<uint32,uint32>> sent;void SendChat(Creature* c,uint8 group,ObjectGuid){sent.emplace_back(c->entry,group);}}textMgr;
auto sCreatureTextMgr=&textMgr;
bool roll_chance_i(uint32){return true;}
struct SmartScript{
 Creature* me;Player* player;Unit* invoker=nullptr;std::map<uint32,SmartScriptHolder> rows;std::vector<SmartScriptHolder> mEvents;
 void SetRows(std::map<uint32,SmartScriptHolder> const& source){rows=source;for(auto const& row:source)mEvents.push_back(row.second);}
 uint32 mTalkerEntry=0,mLastTextID=0,mTextTimer=0;bool mUseTextTimer=false;ObjectGuid mTextGUID,mLastInvoker;
 uint32 chests=0,guides=0,deaths=0;std::vector<uint32> finale;
 Unit* GetBaseObject(){return me;}Unit* GetLastInvoker(){return invoker;}
 bool IsCreature(Unit* u){return u&&u->ToCreature();}bool IsPlayer(Unit* u){return u&&u->ToPlayer();}bool IsInPhase(uint32){return true;}
 void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
 SmartScriptHolder FindLinkedEvent(uint32 id){return rows.at(id);}
 ObjectList* GetTargets(SmartScriptHolder const& e,Unit*){auto l=new ObjectList();switch(e.GetTargetType()){
#include "DialogueTargets.inc"
 case 18:l->push_back(player);break;
 }return l;}
 void ProcessAction(SmartScriptHolder& e,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const* spell=nullptr,GameObject* gob=nullptr){
#include "DialogueActionAdmission.inc"
 switch(e.type){
#include "DialogueTalk.inc"
#include "DialogueCriteria.inc"
 case 11: return; // Optional cast failure; progression is independent.
 case 21:break;
 case 50:assert(player->credits==1);++chests;finale.push_back(50);break;
 case 12:assert(chests==1);++guides;finale.push_back(12);break;
 case 37:assert(guides==1);++deaths;me->alive=false;finale.push_back(37);break;
 default:throw std::runtime_error("Unexpected fixture action");
 }
#include "DialogueLink.inc"
 void ProcessEvent(SmartScriptHolder& e,Unit* unit,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const* spell=nullptr,GameObject* gob=nullptr){
#include "DialogueEventAdmission.inc"
 switch(e.GetEventType()){
 case SMART_EVENT_LINK:ProcessAction(e,unit,var0,var1,bvar,spell,gob);break;
#include "DialogueHealth.inc"
#include "DialogueTextOver.inc"
 }}
 void ProcessEventsFor(SMART_EVENT e,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const* spell=nullptr,GameObject* gob=nullptr);
 void Tick(uint32 diff){
#include "DialogueTimer.inc"
 }
};
#include "DialogueEvents.inc"
std::map<uint32,SmartScriptHolder> Load(char const* path){
 std::ifstream input(path);assert(input);std::map<uint32,SmartScriptHolder> result;
 uint32 id,link,event,flags,p1,p2,p3,p4,action,a1,a2,target,t1;
 while(input>>id>>link>>event>>flags>>p1>>p2>>p3>>p4>>action>>a1>>a2>>target>>t1){
  SmartScriptHolder row;row.event_id=id;row.link=link;row.type=action;row.targetType=target;
  row.event.type=event;row.event.event_flags=flags;row.event.minMaxRepeat={p1,p2,p3,p4};row.event.textOver={p1,p2};
  row.action.talk={a1,a2};row.action.achievementCriteria={a1,a2,0,0};result[id]=row;
 }assert(result.count(90)&&result.count(13));return result;
}
int main(int argc,char** argv){
 assert(argc==3);auto rows=Load(argv[1]);bool baseline=std::string(argv[2])=="baseline";
 for(uint32 victimKind:{0u,1u,2u,3u})for(bool swap:{false,true}){
  Player player;Creature boss,pet,ally;pet.entry=1860;ally.entry=101254;
  boss.victim=victimKind==0?static_cast<Unit*>(&player):victimKind==1?&pet:victimKind==2?&ally:nullptr;
  SmartScript script{&boss,&player};script.SetRows(rows);textMgr.sent.clear();
  boss.hp=10000;script.ProcessEventsFor(SMART_EVENT_HEALTH_PCT);assert(textMgr.sent.empty());boss.hp=1;
  script.ProcessEventsFor(SMART_EVENT_HEALTH_PCT); // Includes the failed optional cast and independent finale.
  assert(!script.chests&&!player.credits);
  bool blocked=baseline&&(victimKind==1||victimKind==2);
  assert(script.mTalkerEntry==(blocked?boss.victim->ToCreature()->entry:100344));
  if(swap)boss.victim=&player; // A later threat change cannot repair an already captured wrong speaker.
  script.Tick(5000);assert(!player.credits&&!script.chests&&boss.alive);
  script.Tick(1);
  if(blocked){assert(!player.credits&&!script.chests&&boss.alive&&boss.hp==1);continue;}
  assert(player.credits==1&&script.chests==1&&script.guides==1&&script.deaths==1&&!boss.alive);
  assert((script.finale==std::vector<uint32>{50,12,37}));
  assert((textMgr.sent.front()==std::pair<uint32,uint32>{100344,3}));
  script.ProcessEventsFor(SMART_EVENT_HEALTH_PCT);script.ProcessEventsFor(SMART_EVENT_TEXT_OVER,nullptr,3,100344);script.Tick(6000);
  assert(player.credits==1&&script.chests==1&&script.guides==1&&script.deaths==1);
 }
 // Preserve generic TALK: explicit creature targets must still select that creature.
 if(!baseline){
  Player player;Creature boss,pet;pet.entry=1860;boss.victim=&pet;SmartScript script{&boss,&player};script.SetRows(rows);
  for(uint32 id:{1u,4u,11u}){auto row=rows.at(id);script.ProcessAction(row);assert(script.mTalkerEntry==100344);}
  auto other=rows.at(90);other.targetType=2;script.ProcessAction(other);assert(script.mTalkerEntry==1860);
  for(Unit* victim:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(nullptr)}){
   Player participant;Creature actor;actor.victim=victim;actor.hp=100000;SmartScript intro{&actor,&participant};intro.SetRows(rows);
   intro.ProcessEventsFor(SMART_EVENT_TEXT_OVER,nullptr,0,100344);assert(intro.mTalkerEntry==100344&&intro.mLastTextID==1);
   intro.Tick(7001);assert(intro.mTalkerEntry==100344&&intro.mLastTextID==2);intro.Tick(6001);
   actor.hp=1;intro.ProcessEventsFor(SMART_EVENT_HEALTH_PCT);intro.Tick(5001);assert(participant.credits==1&&intro.chests==1&&intro.deaths==1);
  }
 }
 std::cout<<(baseline?"PASS: old VICTIM speaker reproduces protected-1HP pet/creature stall; player/null controls complete\n":"PASS: production TALK, targets, health/TextOver admission, timer, LINK and criteria dispatch complete once after 5s for player/pet/creature/null victims; threat changes and generic TALK preserved\n");
}
