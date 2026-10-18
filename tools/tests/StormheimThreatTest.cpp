#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <list>
#include <map>
using uint32=uint32_t;
enum QuestStatus {QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_REWARDED=6};
struct Creature {uint32 area=7803;bool alive=true,visible=true,los=true;float distance=4;uint32 GetAreaId(){return area;}bool IsAlive(){return alive;}int GetGUID(){return 92539;}};
struct Group{bool raid=false;bool isRaidGroup(){return raid;}};
struct Map{int GetDifficultyID(){return 0;}};
struct Quest{bool IsAllowedInRaid(int){return false;}};
struct QuestStore{Quest q;Quest* GetQuestTemplate(uint32 id){assert(id==44771);return &q;}} questStore;auto sQuestDataStore=&questStore;
struct Player {
 bool aura=false,alive=true,castWorks=true;uint32 map=1220,zone=7541;int credits=0,casts=0;std::map<uint32,QuestStatus> quests;
 bool IsAlive(){return alive;}uint32 GetMapId(){return map;}uint32 GetZoneId(){return zone;}
 QuestStatus GetQuestStatus(uint32 q){return quests[q];}bool HasAura(uint32 id){assert(id==229539);return aura;}
 bool HasAuraEffect(uint32 id,uint32 effect){assert(effect==0);return HasAura(id);}
 void RemoveAurasDueToSpell(uint32 id){assert(id==229539);aura=false;}
 bool canSeeOrDetect(Creature* c){return c->visible;}bool IsWithinDistInMap(Creature* c,float d){return c->distance<=d;}
 bool IsWithinLOSInMap(Creature* c){return c->los;}
 bool CastSpell(Player* p,uint32 id,bool triggered){assert(p==this&&id==229554&&triggered);++casts;return castWorks;}
 void KilledMonsterCredit(uint32 id){assert(id==114962&&!aura);++credits;quests[44771]=QUEST_STATUS_COMPLETE;}
 Group* group=nullptr;Group* GetGroup(){return group;}Map* GetMap(){static Map m;return &m;}
 int GetPosition(){return 0;}Player* ToPlayer(){return this;}Player* ToUnit(){return this;}bool IsCreature(){return false;}
 bool isPet(){return false;}Player* GetOwner(){return nullptr;}struct Threat{Player* getHostilTarget(){return nullptr;}};
 Threat& getThreatManager(){static Threat t;return t;}
};
using Unit=Player;
std::list<Creature*> world;
void GetCreatureListWithEntryInGrid(std::list<Creature*>& out,Player*,uint32 entry,float distance){assert(entry==92539&&distance==5);out=world;}
enum class HighGuid{Conversation};struct Generator{int Generate(){return 1;}};
struct ObjectMgr{template<HighGuid> Generator* GetGenerator(){static Generator g;return &g;}} objectMgr;auto sObjectMgr=&objectMgr;
struct Conversation {static inline int calls=0;static inline bool works=true;
 bool CreateConversation(int,uint32 id,Player*,void*,int){assert(id==3931);++calls;return works;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnUpdate(Player*,uint32){}virtual void OnLogin(Player*){}virtual void OnMapChanged(Player*){}};
#include "ThreatProduction.inc"
constexpr int CONDITION_AURA=1,CONDITION_QUESTREWARDED=8,CONDITION_QUESTTAKEN=9;
bool NativeCondition(Player* object,int type,uint32 ConditionValue1,uint32 ConditionValue2=0,bool negative=false){bool condMeets=false;switch(type){
#include "ThreatAuraCondition.inc"
#include "ThreatRewardCondition.inc"
#include "ThreatActiveCondition.inc"
}return negative?!condMeets:condMeets;}
struct ConditionSourceInfo{Player* player;};
struct Condition{uint32 ElseGroup=0,ConditionType=9,ConditionValue1=44771,ReferenceId=0;bool isLoaded(){return true;}bool Meets(ConditionSourceInfo& s){return NativeCondition(s.player,ConditionType,ConditionValue1);}};
using ConditionList=std::list<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;
struct ConditionMgr{ConditionReferenceContainer ConditionReferenceStore;bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;};
#define TC_LOG_DEBUG(...) ((void)0)
#include "ThreatConditionList.inc"
template<std::size_t N>
#include "ThreatLogic.inc"
bool Acquire(Player& p){return NativeCondition(&p,8,44720)&&NativeCondition(&p,9,44771)&&NativeCondition(&p,1,229539,0,true);}
int main(){
 player_stormheim_threat hook;Creature havi,duplicate;world={&havi,&duplicate};
 Player p;p.quests[44720]=QUEST_STATUS_REWARDED;p.quests[44771]=QUEST_STATUS_INCOMPLETE;
 // Actual source gates: no reward, no active quest, completed and held drink.
 for(auto previous:{QUEST_STATUS_NONE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED})for(auto current:{QUEST_STATUS_NONE,QUEST_STATUS_COMPLETE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_REWARDED})for(bool held:{false,true}){
  p.quests[44720]=previous;p.quests[44771]=current;p.aura=held;
  assert(Acquire(p)==(previous==QUEST_STATUS_REWARDED&&current==QUEST_STATUS_INCOMPLETE&&!held));
 }
 p=Player{};p.quests[44720]=QUEST_STATUS_REWARDED;p.quests[44771]=QUEST_STATUS_INCOMPLETE;
 // Narration elapsed, interrupted fill and standing near Havi alone do nothing.
 for(int n=0;n<100;++n)hook.OnUpdate(&p,1000);assert(!p.credits&&!p.casts);
 assert(Acquire(p));p.aura=true; // adapter: successful native18s open-lock/GO spell completion
 assert(!Acquire(p));Player bystander;bystander.quests=p.quests;
 Group raid;raid.raid=true;p.group=&raid;hook.OnUpdate(&p,1);assert(p.aura&&!p.credits&&!p.casts);p.group=nullptr;
 // No delivery to dead/hidden/foreign-stage/out-of-range/occluded Havi; stationary retry.
 world={&havi};havi.distance=6;hook.OnUpdate(&p,1);assert(p.aura&&!p.credits);
 havi.distance=4;havi.los=false;hook.OnUpdate(&p,1);assert(p.aura&&!p.credits);
 havi.los=true;havi.visible=false;hook.OnUpdate(&p,1);assert(p.aura&&!p.credits);
 havi.visible=true;havi.area=7550;hook.OnUpdate(&p,1);assert(p.aura&&!p.credits);
 havi.area=7803;havi.alive=false;hook.OnUpdate(&p,1);assert(p.aura&&!p.credits);
 havi.alive=true;world={&havi,&duplicate};p.castWorks=false;Conversation::works=false;
 hook.OnUpdate(&p,1);assert(p.credits==1&&p.casts==1&&!p.aura&&Conversation::calls==1);
 for(int n=0;n<100;++n)hook.OnUpdate(&p,1);hook.OnLogin(&p);assert(p.credits==1&&Conversation::calls==1);
 hook.OnUpdate(&bystander,1);assert(!bystander.credits); // no group/nearby reward adapter invoked
 // Two independent carriers, abandonment/reaccept, stale login and transfer/death cleanup.
 for(int reason=0;reason<6;++reason){Player r;r.aura=true;r.quests[44771]=QUEST_STATUS_INCOMPLETE;
  if(reason==0)r.quests[44771]=QUEST_STATUS_NONE;if(reason==1)r.quests[44771]=QUEST_STATUS_COMPLETE;
  if(reason==2)r.map=0;if(reason==3)r.zone=7502;if(reason==4)r.alive=false;if(reason==5)r.quests[44771]=QUEST_STATUS_REWARDED;
  hook.OnMapChanged(&r);assert(!r.aura&&!r.credits);
  r=Player{};r.quests[44720]=QUEST_STATUS_REWARDED;r.quests[44771]=QUEST_STATUS_INCOMPLETE;
  assert(Acquire(r));r.aura=true;hook.OnLogin(&r);assert(r.credits==1&&!r.aura);
 }
 // The native packed PlayerCondition negates aura slot0, not its quest requirements.
 std::array<bool,4> states{false,true,true,true};assert(PlayerConditionLogic(65536,states));
 states={true,true,true,true};assert(!PlayerConditionLogic(65536,states));
 // Native ElseGroup admission reproduces the old AND and proves the OR repair.
 Condition threat,unrelated;unrelated.ConditionValue1=42004;ConditionList phase{&threat,&unrelated};ConditionMgr manager;
 p=Player{};p.quests[44771]=QUEST_STATUS_INCOMPLETE;ConditionSourceInfo source{&p};
 assert(!manager.IsObjectMeetToConditionList(source,phase));unrelated.ElseGroup=1;assert(manager.IsObjectMeetToConditionList(source,phase));
 p.quests[44771]=QUEST_STATUS_NONE;assert(!manager.IsObjectMeetToConditionList(source,phase));p.quests[42004]=QUEST_STATUS_INCOMPLETE;assert(manager.IsObjectMeetToConditionList(source,phase));
 std::cout<<"PASS: extracted production delivery/lifecycle callbacks and native acquisition conditions/packed logic; no timer/no-drink credit, personal delivery, duplicate Havi, stationary retry, failed optional casts/conversation, abandonment, login, map, death and independent carriers. World/credit persistence/cast engine are adapters.\n";
}
