#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>
using uint8=uint8_t; using int8=int8_t; using uint32=uint32_t; using int32=int32_t;
enum {CLASS_MONK=10,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,UNIT_FIELD_NPC_FLAGS=1,
 UNIT_FIELD_FLAGS=2,UNIT_FLAG_IMMUNE_TO_PC=4,UNIT_FLAG_IMMUNE_TO_NPC=8,REACT_PASSIVE=0,
 POINT_MOTION_TYPE=8,EFFECT_MOTION_TYPE=16,CHAT_MSG_ADDON=0,LANG_ADDON=0,TEXT_RANGE_PERSONAL=5,
 EMOTE_ONESHOT_USE_STANDING=3};
struct ObjectGuid {uint32 n=0; static ObjectGuid Empty;void Clear(){n=0;}uint32 GetCounter()const{return n;}
 bool operator==(ObjectGuid b)const{return n==b.n;}bool operator!=(ObjectGuid b)const{return n!=b.n;}
 bool operator<(ObjectGuid b)const{return n<b.n;}};ObjectGuid ObjectGuid::Empty;
struct Position {float x=0,y=0,z=0,o=0;float GetOrientation()const{return o;}};
struct QuestObjectScope {ObjectGuid Owner;bool Cancelled=false;};
struct QuestStatusData {std::shared_ptr<QuestObjectScope> ScriptObjects;};
struct Quest {uint32 id=40633;uint32 GetQuestId()const{return id;}};
struct Creature;struct TempSummon;struct Player;struct AI;struct Map;
std::map<ObjectGuid,Creature*> actors;std::map<ObjectGuid,Player*> players;
std::vector<std::unique_ptr<TempSummon>> summons;
std::vector<std::tuple<uint32,uint32,uint32>> speech;
uint32 warningCount=0;bool allowBoard=true,propertiesAvailable=true;uint32 failEntry=0;
#define TC_LOG_WARN(...) (++warningCount)
struct Session {bool loading=false,logout=false;bool PlayerLoading(){return loading;}bool PlayerLogout(){return logout;}};
struct Unit {virtual ~Unit()=default;virtual Creature* ToCreature(){return nullptr;}virtual Player* ToPlayer(){return nullptr;}
 Creature* vehicle=nullptr;bool IsOnVehicle(Creature* b){return vehicle==b;}void ExitVehicle(){vehicle=nullptr;}
 void EnterVehicle(Creature*,int8);};
struct Motion {uint32 id=0;Position dest;void MovePoint(uint32 i,Position const&p,bool path){assert(path);id=i;dest=p;}
 void MoveLand(uint32 i,Position const&p){id=i;dest=p;}
 void MoveTakeoff(uint32 i,float x,float y,float z){id=i;dest={x,y,z,0};}};
struct WorldObject:Unit {ObjectGuid guid;Map* map=nullptr;bool world=true,alive=true;Position pos;uint32 mask=1;std::set<uint32> phases{5910};
 bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}uint32 GetMapId()const;
 bool IsInMap(WorldObject const*b)const{return map==b->map&&world&&b->world;}
 bool IsWithinDistInMap(WorldObject const*b,float d)const{return IsInMap(b)&&GetExactDist(&b->pos)<d;}
 float GetExactDist(Position const*p)const{return std::sqrt((pos.x-p->x)*(pos.x-p->x)+(pos.y-p->y)*(pos.y-p->y)+(pos.z-p->z)*(pos.z-p->z));}
 ObjectGuid GetGUID()const{return guid;}Position const& GetPosition()const{return pos;}uint32 GetGUIDLow()const{return guid.n;}
 uint32 GetPhaseMask()const{return mask;}std::set<uint32> const& GetPhases()const{return phases;}
 void SetPhaseMask(uint32 m,bool){mask=m;}void SetPhaseId(std::set<uint32>const&p,bool){phases=p;}
};
struct Creature:WorldObject {uint32 entry=0,db=0;AI* ai=nullptr;Motion motion;std::weak_ptr<QuestObjectScope> scope;
 std::set<ObjectGuid> hidden;bool kit=false;Creature* ToCreature()override{return this;}
 virtual TempSummon* ToTempSummon(){return nullptr;}AI* AI()const{return ai;}
 uint32 GetDBTableGUIDLow()const{return db;}std::weak_ptr<QuestObjectScope> GetQuestObjectScope()const{return scope;}
 void SetUInt32Value(uint32,uint32){}void SetReactState(uint32){}void SetFlag(uint32,uint32){}void setActive(bool){}
 void SetFacingToObject(Creature*){}void SetFacingTo(float){}void SetWalk(bool){}void SetCanFly(bool){}
 void HandleEmoteCommand(uint32){}Motion* GetMotionMaster(){return &motion;}Map* GetMap(){return map;}
 void* GetVehicleKit(){return kit?this:nullptr;}void AddToHideList(ObjectGuid g){hidden.insert(g);}
 void RemoveFromHideList(ObjectGuid g){hidden.erase(g);}void UpdateObjectVisibility(){}
 void GetCreatureListWithEntryInGrid(std::list<Creature*>& list,uint32 e,float r){for(auto&a:actors)if(a.second->entry==e&&IsWithinDistInMap(a.second,r))list.push_back(a.second);}
 void DespawnOrUnsummon();
};
struct Player:WorldObject {uint32 cls=CLASS_MONK,status=QUEST_STATUS_INCOMPLETE,optional=0;bool contact=true,teleport=false,rewarded=false,visible=true;
 QuestStatusData quest;Session session;uint32 packets=0;
 Player* ToPlayer()override{return this;}bool CanContact(){return contact;}bool HasPendingTeleport(){return teleport;}
 uint32 getClass(){return cls;}bool GetQuestRewardStatus(uint32 q){assert(q==40633);return rewarded;}
 uint32 GetQuestStatus(uint32 q){assert(q==40633);return status;}QuestStatusData* getQuestStatus(uint32 q){assert(q==40633);return status?&quest:nullptr;}
 uint32 GetQuestObjectiveData(uint32 q,uint32 e){assert(q==40633&&e==102531);return optional;}
 void BindQuestObject(uint32 q,Creature*c){assert(q==40633);if(!quest.ScriptObjects){quest.ScriptObjects=std::make_shared<QuestObjectScope>();quest.ScriptObjects->Owner=guid;}c->scope=quest.ScriptObjects;}
 Session* GetSession(){return &session;}bool canSeeOrDetect(WorldObject const*){return visible;}
 void SendDirectMessage(void const*){++packets;}
};
struct TempSummon:Creature {Creature* summoner=nullptr;TempSummon* ToTempSummon()override{return this;}
 Unit* GetSummoner(){return summoner;}};
struct Map {uint32 id=1514;TempSummon* SummonCreature(uint32,Position const&,void const*,uint32,Unit*,ObjectGuid,uint32,int32,ObjectGuid);};
uint32 WorldObject::GetMapId()const{return map?map->id:0;}
struct AI {Creature* me;explicit AI(Creature*c):me(c){c->ai=this;}virtual ~AI()=default;
 virtual void sQuestAccept(Player*,Quest const*){}virtual void SummonedCreatureDespawn(Creature*){}
 virtual void MovementInform(uint32,uint32){}virtual void PassengerBoarded(Unit*,int8,bool){}virtual void UpdateAI(uint32){}
};
struct SmartAI:AI {using AI::AI;uint32 accepted=0,updates=0;
 void sQuestAccept(Player*,Quest const*)override{++accepted;}void UpdateAI(uint32)override{++updates;}};
struct ScriptedAI:AI{using AI::AI;};
struct Properties {void const* LookupEntry(uint32 id){assert(id==3545);return propertiesAvailable?this:nullptr;}} sSummonPropertiesStore;
struct Texts {void SendChat(Creature*c,uint8 group,ObjectGuid o,int,int,int range){assert(range==TEXT_RANGE_PERSONAL);speech.emplace_back(c->entry,group,o.n);}} textMgr;
Texts* sCreatureTextMgr=&textMgr;
namespace ObjectAccessor {
Player* FindPlayer(ObjectGuid g){auto f=players.find(g);return f==players.end()?nullptr:f->second;}
Creature* GetCreature(Creature const& src,ObjectGuid g){auto f=actors.find(g);return f!=actors.end()&&src.map==f->second->map&&f->second->world?f->second:nullptr;}
}
#include "Adventure.inc"
TempSummon* Map::SummonCreature(uint32 e,Position const&p,void const*prop,uint32 duration,Unit* unit,ObjectGuid target,uint32 spell,int32 vehicle,ObjectGuid viewer){
 assert(prop&&duration&&target==ObjectGuid::Empty&&vehicle==0&&players.count(viewer));
 if(e==failEntry)return nullptr;assert(e==100475||e==100481||e==100972);
 auto c=std::make_unique<TempSummon>();c->guid={uint32(100+summons.size())};c->entry=e;c->pos=p;c->map=this;c->summoner=unit->ToCreature();c->scope=c->summoner->scope;
 assert(e!=100972||(spell==198964&&duration==40000));c->kit=e==100972;
 if(e==100475)new LiliAdventure::AdventureAI(c.get());else if(e==100972)new LiliAdventure::CraneAI(c.get());else new SmartAI(c.get());
 auto ptr=c.get();actors[c->guid]=ptr;summons.push_back(std::move(c));return ptr;
}
void Creature::DespawnOrUnsummon(){if(!world)return;world=false;ExitVehicle();if(auto t=ToTempSummon())if(t->summoner&&t->summoner->AI())t->summoner->AI()->SummonedCreatureDespawn(this);}
void Unit::EnterVehicle(Creature*b,int8 seat){if(!allowBoard)return;vehicle=b;b->AI()->PassengerBoarded(this,seat,true);}
struct Fixture {Map map;Creature root,dad;Player p,q;LiliAdventure::AdventureAI ai;
 Fixture():ai(&root){actors.clear();players.clear();speech.clear();warningCount=0;allowBoard=true;failEntry=0;propertiesAvailable=true;
 root.guid={1};root.db=372111;root.entry=100475;root.pos={741.679f,3667.43f,143.347f,0};root.map=&map;
 dad.guid={2};dad.db=340237;dad.entry=100481;dad.pos={736.434f,3662.43f,143.397f,0};dad.map=&map;
 p.guid={10};q.guid={11};p.pos=q.pos=root.pos;p.map=q.map=&map;actors[root.guid]=&root;actors[dad.guid]=&dad;players[p.guid]=&p;players[q.guid]=&q;}
 ~Fixture(){for(auto&c:summons)delete c->ai;summons.clear();}
 LiliAdventure::AdventureAI* scene(Player&player){auto g=ai.scenes.at(player.guid);return dynamic_cast<LiliAdventure::AdventureAI*>(actors.at(g)->AI());}
};
void ticks(AI& ai,uint32 ms){for(uint32 t=0;t<ms;t+=100)if(ai.me->world)ai.UpdateAI(100);}
void arrive(Creature*c,uint32 type){auto id=c->motion.id;c->pos=c->motion.dest;c->AI()->MovementInform(type,id);}
void outside(LiliAdventure::AdventureAI& a){ticks(a,40000);assert(a.moveId==1);arrive(a.me,POINT_MOTION_TYPE);assert(a.moveId==2);arrive(a.me,POINT_MOTION_TYPE);ticks(a,1200);assert(a.step==7);}
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);uint32 cases=0;
 for(bool second:{false,true}){Fixture f;assert(f.ai.Start(&f.p));auto a=f.scene(f.p);size_t n=summons.size();assert(f.ai.Start(&f.p)&&summons.size()==n);
  LiliAdventure::AdventureAI* b=nullptr;if(second){assert(f.ai.Start(&f.q));b=f.scene(f.q);assert(a->owner!=b->owner);}
  f.p.phases={5906};ticks(*a,100);assert(a->me->phases==f.p.phases);outside(*a);
  auto bird=actors.at(a->crane);a->CraneBoarded(bird->guid);assert(!a->boarded);
  f.p.EnterVehicle(bird,0);assert(!f.p.IsOnVehicle(bird)&&!a->boarded);
  if(b){b->Lili()->EnterVehicle(bird,0);assert(!b->Lili()->IsOnVehicle(bird)&&!a->boarded);}
  bird->AI()->MovementInform(EFFECT_MOTION_TYPE,3);assert(!a->landing); // not actually landed
  arrive(bird,EFFECT_MOTION_TYPE);assert(a->landing);ticks(*a,18000);assert(a->boarded&&a->me->IsOnVehicle(bird));
  ticks(*a,6000);assert(a->step==11);bird->AI()->MovementInform(EFFECT_MOTION_TYPE,4);assert(!a->stopped); // interrupted takeoff
  arrive(bird,EFFECT_MOTION_TYPE);assert(a->stopped&&!a->me->world&&!bird->world&&!f.root.hidden.count(f.p.guid)&&f.ai.scenes.count(f.p.guid)==0);
  if(b){assert(!b->stopped&&f.root.hidden.count(f.q.guid));b->Finish();}
  std::vector<std::pair<uint32,uint32>> expected={{100475,0},{100481,0},{100481,1},{100475,1},{100481,2},{100475,2},{100475,3},{100475,4},{100475,5}};
  assert(speech.size()==expected.size());for(size_t i=0;i<speech.size();++i){auto&line=speech[i];assert(std::get<2>(line)==10&&std::get<0>(line)==expected[i].first&&std::get<1>(line)==expected[i].second);}assert(warningCount==0);++cases;
 }
 for(int mode=0;mode<13;++mode){Fixture f;assert(f.ai.Start(&f.p));auto a=f.scene(f.p);auto old=a->me->guid;
  if(mode==0){f.p.status=0;ticks(*a,100);} // abandonment
  if(mode==1){f.p.alive=false;ticks(*a,100);}
  if(mode==2){f.p.world=false;ticks(*a,100);}
  if(mode==3){f.p.quest.ScriptObjects=std::make_shared<QuestObjectScope>();ticks(*a,100);}
  if(mode==4){a->me->DespawnOrUnsummon();} // root expiry callback
  if(mode==5){outside(*a);ticks(*a,16000);} // no landing receipt
  if(mode==6){outside(*a);auto bird=actors.at(a->crane);arrive(bird,EFFECT_MOTION_TYPE);allowBoard=false;ticks(*a,23000);}
  if(mode==7){ticks(*a,40000);ticks(*a,31000);} // movement failure
  if(mode==8){actors.at(a->father)->DespawnOrUnsummon();}
  if(mode==9){outside(*a);actors.at(a->crane)->DespawnOrUnsummon();}
  if(mode==10){f.map.id=1;ticks(*a,100);}
  if(mode==11){f.p.quest.ScriptObjects->Cancelled=true;ticks(*a,100);}
  if(mode==12){a->me->alive=false;ticks(*a,100);}
  if(!(a->stopped&&!f.root.hidden.count(f.p.guid)&&!f.dad.hidden.count(f.p.guid)))std::cerr<<"Failed cleanup mode "<<mode<<" stopped="<<a->stopped<<" root="<<f.root.hidden.size()<<" dad="<<f.dad.hidden.size()<<"\n";assert(a->stopped&&!f.root.hidden.count(f.p.guid)&&!f.dad.hidden.count(f.p.guid));
  f.p.status=QUEST_STATUS_INCOMPLETE;f.p.alive=f.p.world=true;f.map.id=1514;f.p.quest.ScriptObjects.reset();allowBoard=true;
  assert(f.ai.Start(&f.p));auto fresh=f.scene(f.p);assert(fresh->me->guid!=old);f.ai.Release(f.p.guid,old,f.dad.guid);
  assert(f.root.hidden.count(f.p.guid)&&f.ai.scenes.at(f.p.guid)==fresh->me->guid);fresh->Finish();++cases;
 }
 for(uint32 e:{100475u,100481u}){Fixture f;failEntry=e;assert(!f.ai.Start(&f.p));assert(f.root.hidden.empty()&&f.dad.hidden.empty());++cases;}
 {Fixture f;assert(f.ai.Start(&f.p));auto a=f.scene(f.p);failEntry=100972;ticks(*a,40000);arrive(a->me,POINT_MOTION_TYPE);arrive(a->me,POINT_MOTION_TYPE);ticks(*a,1200);assert(a->stopped);++cases;}
 for(int mode=0;mode<8;++mode){Fixture f;if(mode==0)f.p.cls=1;if(mode==1)f.p.status=0;if(mode==2)f.p.rewarded=true;
  if(mode==3)f.p.optional=1;if(mode==4)f.root.db=1;if(mode==5)propertiesAvailable=false;if(mode==6)f.dad.db=1;if(mode==7)f.p.pos.x+=100;
  assert(!f.ai.Start(&f.p)&&summons.empty()&&f.root.hidden.empty());++cases;}
 {Fixture f;Quest other;other.id=40569;f.ai.sQuestAccept(&f.p,&other);assert(f.ai.accepted==1);f.ai.UpdateAI(100);assert(f.ai.updates==1);++cases;}
 std::cout<<"PASS "<<cases<<" production Li Li scene cases: dialogue, real movement/seat receipts, two owners, cleanup/retry, stale generation, failed summons, abandonment/death/logout/map/attempt, preserved SmartAI fallback. Map/path/vehicle services controlled; client rendering remains pending.\n";
}
