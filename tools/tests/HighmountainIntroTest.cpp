#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using QuestStatus=int;
enum {QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,QUEST_OBJECTIVE_MONSTER=0};
struct ObjectGuid {unsigned value=0;bool operator<(ObjectGuid const& rhs)const{return value<rhs.value;}static ObjectGuid const Empty;};
ObjectGuid const ObjectGuid::Empty{};
#include "QuestStatusData.inc"
struct Position {float x=0,y=0,z=0,o=0;};
struct Player;struct Creature;struct ScriptedAI;
struct Unit {Position pos;virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}float GetDistance(Position const& p)const{return std::sqrt((pos.x-p.x)*(pos.x-p.x)+(pos.y-p.y)*(pos.y-p.y)+(pos.z-p.z)*(pos.z-p.z));}float GetDistance(Unit const* u)const{return GetDistance(u->pos);}};
struct QuestObjective {int Type=0,ObjectID=96813,StorageIndex=0;};
struct Quest {unsigned ID=38907;std::vector<QuestObjective> objectives{{}};auto const& GetObjectives()const{return objectives;}};
struct QuestStore {Quest quest;Quest const* GetQuestTemplate(unsigned id){assert(id==38907);return &quest;}} questStore;
auto sQuestDataStore=&questStore;
struct Taxi {unsigned destination=0;unsigned GetTaxiDestination(){return destination;}};
struct Player:Unit {
 unsigned guidValue,map=1220;bool alive=true,teleport=false,flight=false,vehicle=false,visible=true;Taxi m_taxi;
 std::map<unsigned,QuestStatusData> quests;std::map<std::pair<unsigned,unsigned>,int> objectives;
 std::vector<std::function<void()>> events;std::vector<unsigned> credits;unsigned packets=0;
 Player(unsigned value):guidValue(value){}Player* ToPlayer()override{return this;}ObjectGuid GetGUID(){return {guidValue};}
 bool IsAlive(){return alive;}unsigned GetMapId(){return map;}bool IsBeingTeleported(){return teleport;}bool isInFlight(){return flight;}bool GetVehicle(){return vehicle;}bool canSeeOrDetect(Creature*){return visible;}
 void Accept(unsigned id){quests.erase(id);quests[id].Status=3;}
 void Abandon(unsigned id){quests.erase(id);for(auto i=objectives.begin();i!=objectives.end();)if(i->first.first==id)i=objectives.erase(i);else ++i;}
 QuestStatusData* getQuestStatus(unsigned q){auto it=quests.find(q);return it==quests.end()?nullptr:&it->second;}
 int GetQuestStatus(unsigned q){auto s=getQuestStatus(q);return s?s->Status:0;}
 int GetQuestObjectiveData(unsigned q,unsigned object){return objectives[{q,object}];}
 void SetQuestObjectiveData(Quest const* q,QuestObjective const* obj,unsigned count){objectives[{q->ID,obj->ObjectID}]=count;}
 void SendQuestUpdateAddCredit(Quest const* q,ObjectGuid,QuestObjective const& obj,unsigned count){assert(q->ID==38907&&obj.ObjectID==96813&&count==1);++packets;}
 bool CanCompleteQuest(unsigned q){return q==38907?(GetQuestObjectiveData(q,96813)&&GetQuestObjectiveData(q,106244)):GetQuestObjectiveData(q,94965)>=4;}
 void CompleteQuest(unsigned q){quests[q].Status=1;}
 void SetQuestStatus(unsigned q,int status){quests[q].Status=status;}
 void KilledMonsterCredit(unsigned entry){credits.push_back(entry);unsigned q=entry==106244?38907:39272;if(GetQuestStatus(q)!=3)return;++objectives[{q,entry}];if(CanCompleteQuest(q))CompleteQuest(q);}
 void AddDelayedEvent(uint64,std::function<void()> fn){events.push_back(std::move(fn));}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void Next(){assert(!events.empty());auto fn=std::move(events.front());events.erase(events.begin());fn();}
};
#include "QuestDelayedEvent.inc"
std::map<ObjectGuid,Creature*> world;
struct Creature:Unit {unsigned entry;ObjectGuid guid;bool alive=true;ScriptedAI* ai=nullptr;Creature(unsigned e):entry(e),guid{e}{world[guid]=this;}unsigned GetEntry(){return entry;}ObjectGuid GetGUID(){return guid;}bool IsAlive(){return alive;}ScriptedAI* AI(){return ai;}};
struct SpellInfo {unsigned Id;};
struct ScriptedAI {Creature* me;std::vector<unsigned> talks;ScriptedAI(Creature* c):me(c){c->ai=this;}virtual void MoveInLineOfSight(Unit*){}virtual void SpellHit(Unit*,SpellInfo const*){}void Talk(unsigned text,ObjectGuid){talks.push_back(text);}};
struct ObjectAccessor {static Creature* GetCreature(Player&,ObjectGuid guid){auto it=world.find(guid);return it==world.end()?nullptr:it->second;}};
struct PlayerScript {PlayerScript(char const*){}virtual void OnLogin(Player*){}};
#include "HighmountainIntro.inc"
int main(){
 using namespace HighmountainIntro;
 player_highmountain_intro hooks;Player fresh{1};hooks.OnLogin(&fresh);assert(fresh.events.empty());fresh.Accept(38907);WatchFlight(&fresh);assert(fresh.events.size()==1&&fresh.credits.empty());
 // A walk/teleport to the endpoint, or merely knowing taxi node 1719, is not a flight.
 fresh.pos=Landing;fresh.m_taxi.destination=1719;fresh.Next();assert(!fresh.packets);fresh.flight=true;fresh.m_taxi.destination=1;fresh.Next();fresh.flight=false;fresh.Next();assert(!fresh.packets);
 // Observe the actual final taxi leg, then wait until dismounted at its endpoint.
 fresh.Accept(45571);fresh.flight=true;fresh.m_taxi.destination=1719;fresh.Next();assert(!fresh.packets);fresh.flight=false;fresh.m_taxi.destination=0;fresh.Next();assert(fresh.packets==1&&fresh.GetQuestObjectiveData(38907,96813)==1&&!fresh.GetQuestObjectiveData(45571,96813));WatchFlight(&fresh);assert(fresh.events.empty());
 Creature oro{106244};oro.pos=Landing;npc_highmountain_arrival_oro arrival(&oro);fresh.pos.z+=100;arrival.MoveInLineOfSight(&fresh);assert(fresh.GetQuestStatus(38907)==3);fresh.pos=Landing;fresh.flight=true;arrival.MoveInLineOfSight(&fresh);assert(fresh.credits.empty());fresh.flight=false;fresh.visible=false;arrival.MoveInLineOfSight(&fresh);assert(fresh.credits.empty());fresh.visible=true;arrival.MoveInLineOfSight(&fresh);assert(fresh.GetQuestStatus(38907)==1&&fresh.credits.size()==1);arrival.MoveInLineOfSight(&fresh);assert(fresh.credits.size()==1);fresh.Next();assert(arrival.talks==std::vector<unsigned>({0,1}));
 Player other{2};other.Accept(38907);other.pos=Landing;arrival.MoveInLineOfSight(&other);assert(other.GetQuestObjectiveData(38907,106244)==1&&other.GetQuestStatus(38907)==3);other.Abandon(38907);other.Accept(38907);auto talkCount=arrival.talks.size();other.Next();assert(arrival.talks.size()==talkCount);
 Player retry{3};retry.Accept(38907);retry.flight=true;retry.m_taxi.destination=1719;WatchFlight(&retry);retry.Abandon(38907);retry.Accept(38907);retry.flight=false;retry.pos=Landing;retry.Next();assert(!retry.packets&&retry.events.empty());hooks.OnLogin(&retry);retry.Next();assert(!retry.packets);retry.flight=true;retry.Next();retry.alive=false;retry.Next();retry.flight=false;retry.alive=true;retry.Next();assert(!retry.packets);retry.flight=true;retry.Next();retry.flight=false;retry.pos.z+=100;retry.Next();assert(!retry.packets);
 // Login while resuming a taxi still observes the final leg correctly.
 Player resume{4};resume.Accept(38907);resume.flight=true;resume.m_taxi.destination=1719;hooks.OnLogin(&resume);resume.pos=Landing;resume.flight=false;resume.Next();assert(resume.packets==1);
 SpellInfo destroy{195481},wrong{1};Player farmer{5},neighbor{6};farmer.Accept(39272);neighbor.Accept(39272);
 for(unsigned entry=99433;entry<=99436;++entry){
  Creature idol{entry};npc_highmountain_poison_idol ai(&idol);idol.pos={float(entry-99433)*30,0,0,0};farmer.pos=idol.pos;neighbor.pos=idol.pos;
  auto count=farmer.credits.size();ai.SpellHit(&farmer,&wrong);farmer.alive=false;ai.SpellHit(&farmer,&destroy);farmer.alive=true;farmer.pos.z+=10;ai.SpellHit(&farmer,&destroy);farmer.pos=idol.pos;farmer.visible=false;ai.SpellHit(&farmer,&destroy);farmer.visible=true;assert(farmer.credits.size()==count);
  ai.SpellHit(&farmer,&destroy);ai.SpellHit(&farmer,&destroy);assert(farmer.credits.size()==count+2&&farmer.GetQuestObjectiveData(39272,entry)==1);ai.SpellHit(&neighbor,&destroy);assert(neighbor.GetQuestObjectiveData(39272,entry)==1&&idol.alive);
 }
 assert(farmer.GetQuestStatus(39272)==1&&neighbor.GetQuestStatus(39272)==1&&farmer.GetQuestObjectiveData(39272,94965)==4);
 farmer.Abandon(39272);farmer.Accept(39272);Creature first{99433};npc_highmountain_poison_idol again(&first);farmer.pos=first.pos;again.SpellHit(&farmer,&destroy);assert(farmer.GetQuestObjectiveData(39272,94965)==1&&farmer.GetQuestStatus(39272)==3);
 std::cout<<"PASS: production taxi observation, actual landing, no walk/teleport credit, no Broken Shore credit, abandon/death/login recovery; Oro range/phase/flight guards and personal dialogue; four unique native idol spell hits with duplicate and multiplayer protection.\n";
}
