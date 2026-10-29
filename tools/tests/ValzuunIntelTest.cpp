// Complete production controller and native group event dispatch. World lookup,
// text delivery, map/range predicates and capped quest storage are adapters.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;
constexpr uint32 QUEST_STATUS_INCOMPLETE=3,CHAT_MSG_MONSTER_SAY=12;
constexpr float INTERACTION_DISTANCE=5;
struct ObjectGuid {uint32 id=0;bool operator<(ObjectGuid other)const{return id<other.id;}bool operator==(ObjectGuid other)const{return id==other.id;}static ObjectGuid const Empty;};ObjectGuid const ObjectGuid::Empty{};
struct QuestStatusData {std::shared_ptr<uint8> ScriptLifetime;};
struct Map{} origin,other;
struct Player;
struct WorldObject {bool world=true;ObjectGuid guid;virtual bool IsCreature()const{return false;}virtual ~WorldObject()=default;bool IsInWorld()const{return world;}ObjectGuid GetGUID()const{return guid;}};
struct TalkMenu {uint32 closes=0;void SendCloseGossip(){++closes;}};
struct GroupReference {Player* player=nullptr;GroupReference* following=nullptr;Player* getSource(){return player;}GroupReference* next(){return following;}};
struct Group {std::vector<GroupReference> members;Group(std::initializer_list<Player*> players){for(auto p:players)members.push_back({p,nullptr});for(size_t i=1;i<members.size();++i)members[i-1].following=&members[i];}GroupReference* GetFirstMember(){return members.empty()?nullptr:&members[0];}};
struct Creature:WorldObject {uint32 entry=105333;bool alive=true,combat=false,scoped=false,allowed=true;Map* map=&origin;
 uint32 GetEntry()const{return entry;}bool IsAlive()const{return alive;}bool isInCombat()const{return combat;}Map* GetMap()const{return map;}bool HasQuestObjectScope()const{return scoped;}bool CanTargetQuestObject(Player*)const{return allowed;}bool IsCreature()const override{return true;}
};
struct Player:WorldObject {
 bool alive=true,contact=true,pending=false,phase=true,los=true,rewarded=false,hasQuest=true,groupRange=true,released=false,finish=false;
 float distance=1;Map* map=&origin;uint32 status=3,credits=0;QuestStatusData quest;Group* party=nullptr;TalkMenu menu;TalkMenu* PlayerTalkClass=&menu;std::function<void()> creditHook;
 bool CanContact()const{return contact;}bool IsAlive()const{return alive;}bool HasPendingTeleport()const{return pending;}Map* GetMap()const{return map;}
 bool InSamePhase(Creature*)const{return phase;}bool IsWithinDistInMap(Creature*,float range)const{return distance<=range;}bool IsWithinLOSInMap(Creature*)const{return los;}
 bool GetQuestRewardStatus(uint32 id)const{assert(id==41919);return rewarded;}uint32 GetQuestStatus(uint32 id)const{assert(id==41919);return hasQuest?status:0;}
 QuestStatusData* getQuestStatus(uint32 id){assert(id==41919);return hasQuest?&quest:nullptr;}
 Group* GetGroup(){return party;}bool TalkedToCreature(uint32,ObjectGuid){return false;}bool IsAtGroupRewardDistance(WorldObject*)const{return groupRange;}
 WorldObject* GetCorpse(){return released?this:nullptr;}
 void KilledMonsterCredit(uint32 id,ObjectGuid){assert(id==105333);if(hasQuest&&!rewarded&&status==3&&!credits){++credits;if(finish)status=1;}if(creditHook)creditHook();}
 void RewardPlayerAndGroupAtEvent(uint32,WorldObject*);
};
using Unit=WorldObject;
#include "NativeGroupCredit.inc"
std::map<ObjectGuid,Player*> players;
namespace ObjectAccessor {Player* GetPlayer(Creature&,ObjectGuid guid){auto it=players.find(guid);return it==players.end()||!it->second->world?nullptr:it->second;}}
struct CreatureTextMgr {bool groups[4]={true,true,true,true};static float GetRangeForChatType(uint32){return 25;}bool TextExist(uint32,uint8 group)const{return groups[group];}} textManager;
CreatureTextMgr* sCreatureTextMgr=&textManager;
uint32 errors=0,now=0;struct Line {uint32 time;uint8 group;ObjectGuid player;};
struct SmartAI {Creature* me;std::vector<Line> lines;uint32 delegated=0,resets=0,updates=0;
 explicit SmartAI(Creature* actor):me(actor){}virtual ~SmartAI()=default;virtual void Reset(){++resets;}virtual void UpdateAI(uint32){++updates;}virtual void sGossipSelect(Player*,uint32,uint32){++delegated;}
 void Talk(uint8 group,ObjectGuid player){lines.push_back({now,group,player});}
};
#define TC_LOG_ERROR(...) ++errors
#include "ValzuunIntel.inc"
struct Fixture {Creature actor;Player first,second; npc_valzuun_intel script;
 Fixture():script(&actor){now=0;players.clear();first.guid.id=1;second.guid.id=2;players[first.guid]=&first;players[second.guid]=&second;}
 void Start(Player* player=nullptr){script.sGossipSelect(player?player:&first,19472,0);}
 void Tick(uint32 diff){now+=diff;script.UpdateAI(diff);}
 void Finish(){for(int i=0;i<25;++i)Tick(1000);}
};
int main(){
 {Fixture f;f.first.finish=true;f.Start();assert(f.script.lines.size()==1&&f.first.credits==0&&f.first.menu.closes==1);f.Tick(4999);assert(f.script.lines.size()==1);f.Tick(1);assert(f.script.lines.size()==2);f.Tick(9999);assert(f.script.lines.size()==2);f.Tick(1);f.Tick(8000);assert(f.script.lines.size()==4&&f.first.credits==0);f.Tick(1999);assert(f.first.credits==0);f.first.creditHook=[&]{assert(f.script.flows.empty());};f.Tick(1);assert(f.first.credits==1&&f.first.status==1&&f.script.flows.empty());
  for(size_t i=0;i<4;++i){uint32 expected[]={0,5000,15000,23000};assert(f.script.lines[i].time==expected[i]&&f.script.lines[i].group==i);}f.Start();assert(f.script.lines.size()==4);
 }
 {Fixture f;f.Start();f.Tick(2000);f.Start();assert(f.script.lines.size()==1);f.Start(&f.second);assert(f.script.flows.size()==2);for(int i=0;i<23;++i)f.Tick(1000);assert(f.first.credits==1&&f.second.credits==0);f.Tick(2000);assert(f.second.credits==1&&f.script.flows.empty());assert(f.script.lines.size()==8);f.Start();f.Finish();assert(f.first.credits==1);}
 // Starting or continuing must not use an unrelated player, stale quest lease,
 // inaccessible actor or state lost during a later tick.
 for(unsigned mode=0;mode<16;++mode){Fixture f;f.Start();assert(f.script.flows.size()==1);switch(mode){
 case 0:f.first.hasQuest=false;break;case 1:f.first.status=1;break;case 2:f.first.rewarded=true;break;case 3:f.first.alive=false;break;case 4:f.first.world=false;break;
 case 5:f.first.contact=false;break;case 6:f.first.pending=true;break;case 7:f.first.map=&other;break;case 8:f.first.phase=false;break;case 9:f.first.los=false;break;
 case 10:f.first.distance=26;break;case 11:f.actor.alive=false;break;case 12:f.actor.world=false;break;case 13:f.actor.combat=true;break;
 case 14:f.first.quest.ScriptLifetime=std::make_shared<uint8>();break;case 15:f.actor.scoped=true;f.actor.allowed=false;break;}
 f.Tick(5000);assert(f.first.credits==0&&f.script.flows.empty()&&f.script.lines.size()==1);
 }
 {Fixture f;f.first.distance=6;f.Start();assert(f.script.flows.empty());f.first.distance=1;f.script.sGossipSelect(nullptr,19472,0);assert(f.script.flows.empty());f.Start();f.script.Reset();f.Finish();assert(f.first.credits==0&&f.script.resets==1);f.Start();f.Finish();assert(f.first.credits==1);}
 {Fixture f;f.Start();f.first.hasQuest=false;f.first.quest.ScriptLifetime.reset();f.first.hasQuest=true;f.first.quest.ScriptLifetime=std::make_shared<uint8>();f.Tick(5000);assert(f.script.flows.empty()&&!f.first.credits);f.Start();f.Finish();assert(f.first.credits==1);}
 {Fixture f;f.Start();f.Tick(30000);assert(f.script.lines.size()==2&&f.first.credits==0);f.Tick(10000);f.Tick(8000);f.Tick(2000);assert(f.script.lines.size()==4&&f.first.credits==1);}
 {Fixture f;f.Start();f.Tick(90000);assert(f.script.flows.empty()&&!f.first.credits&&f.script.lines.size()==1);}
 {Fixture f;f.script.sGossipSelect(&f.first,19472,1);f.script.sGossipSelect(&f.first,19474,0);assert(f.script.delegated==2&&f.script.flows.empty()&&!f.first.credits);}
 {Fixture f;textManager.groups[3]=false;uint32 prior=errors;f.Start();assert(f.script.flows.empty()&&f.script.lines.empty()&&!f.first.credits&&errors==prior+1);textManager.groups[3]=true;f.Start();f.Finish();assert(f.first.credits==1);}
 {Fixture f;f.Start();f.Tick(5000);uint32 prior=errors;textManager.groups[2]=false;f.Tick(10000);assert(f.script.flows.empty()&&!f.first.credits&&errors==prior+1);textManager.groups[2]=true;f.Start();f.Finish();assert(f.first.credits==1);}
 // Native party award retains range/world/dead-release/quest eligibility.
 {Fixture f;Player far,offline,inactive,dead;far.guid.id=3;offline.guid.id=4;inactive.guid.id=5;dead.guid.id=6;far.groupRange=false;offline.world=false;inactive.status=0;dead.alive=false;dead.released=true;
  Group party{&f.first,&f.second,&far,&offline,&inactive,&dead};f.first.party=&party;f.first.finish=true;f.second.finish=true;f.Start();f.Finish();assert(f.first.credits==1&&f.second.credits==1&&!far.credits&&!offline.credits&&!inactive.credits&&!dead.credits);
 }
 std::puts("PASS complete production four-line intel at0/5/15/23s, credit25s/native group dispatch; independent/repeated invokers, no fast-forward credit, finite expiry, reset/missing/dead/contact/map/phase/LOS/range/abandon-reaccept leases and preserved SmartAI delegation. World/text/quest storage endpoints are adapters; actual client audio/overlapping public speech remain unverified.");
}
