#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <stdexcept>
using uint32=uint32_t;using uint8=uint8_t;using ReactStates=uint32;
constexpr int SMART_ACTION_WP_START=53,SMART_ACTION_TALK=1,SMART_EVENT_LINK=61,SMART_EVENT_TEXT_OVER=52,SMART_ESCORT_TARGETS=1;
enum ChatMsg{CHAT_MSG_MONSTER_SAY=12,CHAT_MSG_MONSTER_YELL=14,CHAT_MSG_MONSTER_EMOTE=16,CHAT_MSG_RAID_BOSS_EMOTE=41};
enum{CONFIG_LISTEN_RANGE_SAY,CONFIG_LISTEN_RANGE_YELL,CONFIG_LISTEN_RANGE_TEXTEMOTE};
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) throw std::runtime_error("Invalid production LINK")
#define CAST_AI(type,ai) (ai)
struct World{float getFloatConfig(int id){return id==CONFIG_LISTEN_RANGE_YELL?300.f:40.f;}}world;
auto sWorld=&world;
struct ObjectGuid{inline static constexpr int Empty=0;int v;ObjectGuid(int value=0):v(value){}};
struct Creature;struct Player;
struct Unit{virtual ~Unit()=default;virtual Creature* ToCreature(){return nullptr;}virtual Player* ToPlayer(){return nullptr;}ObjectGuid GetGUID(){return 1;}};
struct Player:Unit{Player* ToPlayer()override{return this;}};
struct AI{bool running=false;uint32 mEscortQuestID=0;
 bool StartPath(bool run,uint32 id,bool repeat,Unit*){assert(run&&id==106692&&!repeat);running=true;return true;}
 void SetDespawnTime(uint32){}
};
struct Creature:Unit{AI ai;AI* AI(){return &ai;}Creature* ToCreature()override{return this;}uint32 GetEntry(){return 106692;}void SetReactState(ReactStates){} };
struct CreatureTextMgr{
 int openingType=14,firstTransmissions=0;bool firstWhileRunning=false;uint32 now=0,firstAt=99999;
 static float GetRangeForChatType(ChatMsg type);
 void SendChat(Creature* actor,uint8 group,ObjectGuid){if(group==0){++firstTransmissions;firstWhileRunning=actor->ai.running;firstAt=now;}}
}textMgr;
auto sCreatureTextMgr=&textMgr;
#include "ChatRange.inc"
using ObjectList=std::list<Unit*>;
struct SmartScriptHolder{
 uint32 event_id=0,link=0,type=0,event=SMART_EVENT_LINK,entryOrGuid=106692;bool runOnce=false;
 struct {struct{uint32 textGroupID=0,duration=16500;}talk;
 struct{uint32 run=1,pathID=106692,repeat=0,quest=0,despawnTime=0,reactState=0;}wpStart;}action;
 uint32 GetActionType(){return type;}uint32 GetEventType(){return event;}uint32 GetScriptType(){return 0;}
};
struct SmartScript{
 Creature* me;std::map<uint32,SmartScriptHolder> rows;
 uint32 mTalkerEntry=0,mLastTextID=0,mTextTimer=0;bool mUseTextTimer=false,pickedUp=false;
 ObjectGuid mTextGUID;uint32 var0=0,var1=0;bool bvar=false;void* spell=nullptr;void* gob=nullptr;
 bool IsSmart(){return true;}bool IsCreature(Unit* u){return u&&u->ToCreature();}bool IsPlayer(Unit* u){return u&&u->ToPlayer();}
 Unit* GetLastInvoker(){return nullptr;}ObjectList* GetTargets(SmartScriptHolder&,Unit*){return nullptr;}
 void StoreTargetList(ObjectList* p,int){delete p;}
 SmartScriptHolder FindLinkedEvent(uint32 id){return rows.at(id);}
 void ProcessEvent(SmartScriptHolder& e,Unit* unit,uint32,uint32,bool,void*,void*){ProcessAction(e,unit);}
 void ProcessEventsFor(int event,Unit*,uint32 group,uint32 entry){assert(event==SMART_EVENT_TEXT_OVER&&entry==106692&&group==0);pickedUp=true;auto next=rows.at(2);ProcessAction(next);}
 void Tick(uint32 diff){textMgr.now+=diff;
#include "TextTimer.inc"
 }
 void ProcessAction(SmartScriptHolder& e,Unit* unit=nullptr){switch(e.type){
#include "Waypoint.inc"
#include "Talk.inc"
 }
#include "LinkTail.inc"
};
int main(int argc,char** argv){
 assert(argc==3);int type=std::atoi(argv[1]);uint32 duration=std::atoi(argv[2]);assert(type==14&&duration>=16006);
 // Normal chat and sound both call this native range selector. Initial
 // spawn-to-doomweaver distances exceed SAY40; later standing beside the skull
 // cannot recover the already transmitted first packet.
 for(float distance:{41.85f,45.39f,49.13f}){
  assert(distance>CreatureTextMgr::GetRangeForChatType(CHAT_MSG_MONSTER_SAY));
  assert(distance<CreatureTextMgr::GetRangeForChatType(static_cast<ChatMsg>(type)));
 }
 Creature actor;SmartScript script{&actor};
 SmartScriptHolder start;start.type=53;start.link=1;
 SmartScriptHolder first;first.event_id=1;first.type=1;first.action.talk.duration=duration;script.rows[1]=first;
 SmartScriptHolder second=first;second.event_id=2;second.action.talk.textGroupID=1;second.action.talk.duration=7750;script.rows[2]=second;
 script.ProcessAction(start);assert(textMgr.firstTransmissions==1&&textMgr.firstWhileRunning&&textMgr.firstAt==0&&!script.pickedUp);
 script.Tick(16005);assert(!script.pickedUp);script.Tick(duration-16005);assert(!script.pickedUp);
 script.Tick(1);assert(script.pickedUp&&textMgr.now>16006&&script.mLastTextID==1);
 assert(textMgr.firstTransmissions==1);
 std::cout<<"PASS: native WP_START -> TALK LINK sends opening during the first running frame; native text timer delays pickup until full clip; SAY40 loss and scoped YELL delivery reproduced\n";
}
