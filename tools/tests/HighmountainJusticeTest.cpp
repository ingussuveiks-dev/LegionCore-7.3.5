#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <set>
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;using int8=int8_t;using uint64=uint64_t;
enum QuestStatus {QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED};
enum {REACT_PASSIVE,UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE=2,UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_IMMUNE_TO_NPC=512,POINT_MOTION_TYPE=1,SPELL_AURA_MOUNTED=78};
enum SpellCastResult {SPELL_CAST_OK,SPELL_FAILED_DONT_REPORT};
#include "QuestStatusData.inc"
struct ObjectGuid {unsigned value=0;bool operator<(ObjectGuid r)const{return value<r.value;}bool operator==(ObjectGuid r)const{return value==r.value;}bool operator!=(ObjectGuid r)const{return value!=r.value;}};
struct Position {float x=0,y=0,z=0,o=0;float GetOrientation()const{return o;}float GetExactDist(Position const* p)const{return std::sqrt((x-p->x)*(x-p->x)+(y-p->y)*(y-p->y)+(z-p->z)*(z-p->z));}};
struct Player;struct Creature;struct ScriptedAI;
struct Unit:Position {ObjectGuid guid;virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}std::set<unsigned> phases={5050,5165,9180};auto const& GetPhases(){return phases;}unsigned entry=0,map=1220;bool alive=true,full=false,targetPhase=true;virtual Unit* GetAnyOwner(){return nullptr;}unsigned GetEntry(){return entry;}bool IsAlive(){return alive;}bool IsFullHealth(){return full;}bool HasPhaseId(unsigned id){return id==5165 && targetPhase;}unsigned GetMapId(){return map;}ObjectGuid GetGUID(){return guid;}float GetDistance(Unit* u){return GetExactDist(u);}};
struct Spline {bool final=true;bool Finalized(){return final;}};
struct Vehicle {Unit* passenger=nullptr;unsigned relocations=0;Unit* GetPassenger(int seat){assert(seat==0);return passenger;}void RelocatePassengers(){++relocations;}};
std::map<ObjectGuid,Creature*> world;unsigned serial=100;
struct Player:Unit {
 QuestStatusData quest;bool alive=true,inWorld=true,teleport=false,flight=false,flying=false,phase=true,visible=true,los=true,combat=false,transport=false,mounted=false,aura=true;
 unsigned credits=0,healCredits=0; bool sourcePhase=true; bool HasPhaseId(unsigned){return sourcePhase;}
 unsigned map=1220,enterCalls=0,teleports=0,saves=0,controls=0;Creature* vehicle=nullptr;Unit* mover=this;Unit* view=nullptr;Spline spline;Spline* movespline=&spline;
 Position exitDest,savedPos;std::vector<int> order;
 explicit Player(unsigned id){guid={id};quest.Status=QUEST_STATUS_INCOMPLETE;}
 Player* ToPlayer()override{return this;}bool IsAlive(){return alive;}bool IsInWorld(){return inWorld;}unsigned GetMapId(){return map;}
 unsigned GetQuestObjectiveData(unsigned,unsigned){return credits;}void KilledMonsterCredit(unsigned id){if(id==100712)++credits;else{assert(id==97498);++healCredits;}}
 int GetQuestStatus(unsigned id){assert(id==40594);return quest.Status;}bool IsBeingTeleported(){return teleport;}bool isInFlight(){return flight;}bool IsFlying(){return flying;}bool isInCombat(){return combat;}bool GetTransport(){return transport;}
 Creature* GetVehicle(){return vehicle;}Unit* GetVehicleBase();Unit* GetUnitBeingMoved(){return mover;}Unit* GetViewpoint(){return view;}
 bool InSamePhase(Unit*){return phase;}bool canSeeOrDetect(Unit*){return visible;}bool IsWithinLOSInMap(Unit*){return los;}
 void SetClientControl(Unit* u,bool control){assert(control==(u==this));mover=u;view=u==this?nullptr:u;++controls;order.push_back(control?3:1);}
 void SetViewpoint(Unit*,bool apply){assert(!apply);view=nullptr;}
 void SendMovementFlagUpdate(bool value){assert(value);}void StopMoving(){}
 void RemoveAurasDueToSpell(unsigned id){assert(id==198214);aura=false;}void RemoveAurasByType(unsigned type){assert(type==SPELL_AURA_MOUNTED);mounted=false;}
 void EnterVehicle(Creature*,int seat,bool triggered){assert(seat==0 && triggered);++enterCalls;}
 void ExitVehicle();void NearTeleportTo(Position const& p){static_cast<Position&>(*this)=p;++teleports;}
 void SaveToDB(){++saves;savedPos=*this;}
 QuestStatusData* getQuestStatus(unsigned){return &quest;}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void AddDelayedEvent(uint64,std::function<void()> callback){callback();}
 void Reaccept(){quest=QuestStatusData{};quest.Status=QUEST_STATUS_INCOMPLETE;}
};
#include "QuestDelayedEvent.inc"
struct Motion {Player* owner=nullptr;unsigned moves=0,id=0;Position destination;void MovePoint(unsigned point,Position const& p,bool generate,float speed){assert(!generate && speed==18);id=point;destination=p;++moves;if(owner){assert(owner->mover!=owner);owner->order.push_back(2);}}};
struct Creature:Unit {
 bool regen=true;unsigned health=100;void setRegeneratingHealth(bool v){regen=v;}void SetHealth(unsigned v){health=v;}unsigned CountPctFromMaxHealth(unsigned v){return v;}void SetPhaseId(std::set<unsigned> p,bool){phases=p;}
 Unit* GetAnyOwner()override{return owner;}Player* owner;ScriptedAI* ai=nullptr;bool alive=true,gone=false,personal=false,fly=false,gravity=false,active=false,hasKit=true;Motion motion;Vehicle kit;
 explicit Creature(Player& p):owner(&p){guid={++serial};world[guid]=this;motion.owner=&p;}
 bool IsAlive(){return alive;}bool IsDespawn(){return gone;}Vehicle* GetVehicleKit(){return hasKit?&kit:nullptr;}Motion* GetMotionMaster(){return &motion;}
 void DespawnOrUnsummon(unsigned=0){gone=true;}void AddPlayerInPersonnalVisibilityList(ObjectGuid g){assert(g==owner->guid);personal=true;}
 void SetReactState(unsigned){}void SetFlag(unsigned,unsigned){}void SetCanFly(bool v){fly=v;}void SetDisableGravity(bool v){gravity=v;}void setActive(bool v){active=v;}
 void NearTeleportTo(Position const& p){static_cast<Position&>(*this)=p;}void StopMoving(){}void SetFacingTo(float o2){o=o2;}
};
Unit* Player::GetVehicleBase(){return vehicle;}
struct ObjectAccessor {static Creature* GetCreature(Player& p,ObjectGuid g){auto i=world.find(g);return p.map!=1220||i==world.end()?nullptr:i->second;}};
struct ScriptedAI {Creature* me;explicit ScriptedAI(Creature* c):me(c){c->ai=this;}virtual ~ScriptedAI()=default;bool UpdateVictim(){return false;}void DoMeleeAttackIfReady(){}virtual void UpdateAI(uint32){}bool charmReplaced=false;virtual void OnCharmed(bool){charmReplaced=true;}virtual void Reset(){}virtual void IsSummonedBy(Unit*){}virtual void PassengerBoarded(Unit*,int8,bool){}virtual void MovementInform(uint32,uint32){}virtual void CalcExitVehiclePos(Position&){};};
void Player::ExitVehicle(){if(!vehicle)return;auto bird=vehicle;bird->kit.passenger=nullptr;bird->ai->OnCharmed(false);assert(!bird->ai->charmReplaced);bird->ai->PassengerBoarded(this,0,false);vehicle=nullptr;exitDest=*bird;bird->ai->CalcExitVehiclePos(exitDest);spline.final=false;order.push_back(4);}
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnUpdate(Player*,uint32){}virtual void OnMapChanged(Player*){}virtual void OnLogout(Player*){};};
struct SpellScript {Unit* caster=nullptr;Unit* target=nullptr;int heal=0;Unit* GetCaster(){return caster;}Unit* GetHitUnit(){return target;}int GetHitHeal(){return heal;}struct Hook{template<class T>void operator+=(T){}} OnCheckCast,AfterCast,BeforeHit,AfterHit;virtual void Register(){};};
#define PrepareSpellScript(name) public:
#define SpellCheckCastFn(fn) &fn
#define SpellCastFn(fn) &fn
#define SpellHitFn(fn) &fn
#include "JusticeRoute.inc"
#include "HighmountainJustice.inc"
void Move(Unit& u,Position const& p){static_cast<Position&>(u)=p;}
struct Flight {
 Player p;Creature bird;npc_highmountain_justice ai;
 explicit Flight(unsigned id):p(id),bird(p),ai(&bird){Move(p,Justice::Return);Move(bird,Justice::Return);ai.IsSummonedBy(&p);}
 ~Flight(){Justice::rides.erase(p.guid);world.erase(bird.guid);}
 void Update(unsigned diff=100){Justice::Update(&p,diff);}
 void Board(){Update();ai.OnCharmed(true);assert(!ai.charmReplaced);p.vehicle=&bird;bird.kit.passenger=&p;Move(p,bird);p.spline.final=false;ai.PassengerBoarded(&p,0,true);}
 void Fly(){Board();Update();assert(bird.motion.moves==0);p.spline.final=true;Update();assert(bird.motion.moves==1);}
 void Arrive(){auto r=Justice::rides.at(p.guid);Move(bird,Justice::Route[r.node]);Move(p,bird);ai.MovementInform(POINT_MOTION_TYPE,uint32(r.node+1));Update();}
 void CompleteExit(){assert(!p.vehicle);Move(p,p.exitDest);p.spline.final=true;Update();}
};
int main()
{
 player_highmountain_justice script;
 {Flight f(1),other(2);assert(f.bird.personal && f.bird.active && f.bird.fly && f.bird.gravity);
  Creature duplicate(f.p);Move(duplicate,Justice::Return);npc_highmountain_justice duplicateAI(&duplicate);duplicateAI.IsSummonedBy(&f.p);assert(duplicate.gone && Justice::rides.at(f.p.guid).bird==f.bird.guid);world.erase(duplicate.guid);
  spell_highmountain_justice spell;spell.caster=&f.p;assert(spell.Check()==SPELL_FAILED_DONT_REPORT);spell.After();assert(f.p.aura);
  f.Update();f.Update();assert(f.p.enterCalls==1 && f.bird.motion.moves==0);
  f.p.vehicle=&f.bird;f.bird.kit.passenger=&f.p;Move(f.p,f.bird);f.p.spline.final=false;f.Update();assert(!f.bird.motion.moves);
  f.p.spline.final=true;f.Update();assert(f.bird.motion.moves==1 && f.p.order[0]==1 && f.p.order[1]==2);
  f.ai.MovementInform(POINT_MOTION_TYPE,1);f.Update();assert(Justice::rides.at(f.p.guid).node==0); // wrong location
  Move(f.bird,Justice::Route[0]);Move(f.p,f.bird);f.ai.MovementInform(99,1);f.Update();assert(Justice::rides.at(f.p.guid).node==0);
  for(size_t i=0;i<Justice::RouteSize;++i)f.Arrive();
  assert(!f.p.vehicle && f.p.mover==&f.p && !f.p.view && f.p.teleports==0);
  assert(f.p.exitDest.GetExactDist(&Justice::Landing)<0.01f);f.Update(1000);assert(Justice::rides.count(f.p.guid) && f.p.teleports==0);
  f.CompleteExit();assert(!Justice::rides.count(f.p.guid) && !f.p.aura && f.bird.gone && f.p.teleports==0);
  assert(Justice::rides.count(other.p.guid) && other.p.aura && !other.bird.gone);
 }
 {Flight f(3);f.Update(10000);assert(!f.bird.motion.moves);f.Update();assert(!Justice::rides.count(f.p.guid) && !f.p.aura && f.p.GetExactDist(&Justice::Return)<0.01f);
  assert(Justice::CanStart(&f.p));}
 {Flight f(4);f.Fly();f.Arrive();f.p.ExitVehicle();assert(f.p.mover==&f.p);f.CompleteExit();f.Update();assert(f.p.teleports==1 && f.p.GetExactDist(&Justice::Return)<0.01f);}
 for(int reason=0;reason<6;++reason){Flight f(10+reason);f.Fly();f.Arrive();
  if(reason==0)f.p.Reaccept();if(reason==1)f.p.alive=false;if(reason==2)f.p.phase=false;if(reason==3)f.p.visible=false;if(reason==4)f.bird.alive=false;
  f.Update(reason==5?300000:100);assert(!f.p.vehicle && f.p.mover==&f.p && f.p.teleports==0);
  f.CompleteExit();assert(f.p.teleports==1 && f.p.GetExactDist(&Justice::Return)<0.01f && !Justice::rides.count(f.p.guid));
 }
 {Flight f(20);f.Fly();f.Arrive();script.OnLogout(&f.p);assert(!f.p.vehicle && f.p.mover==&f.p && f.p.teleports==1 && f.p.saves==1 && !f.p.aura && !Justice::rides.count(f.p.guid));assert(f.p.savedPos.GetExactDist(&Justice::Return)<0.01f);}
 {Flight f(21);f.Fly();f.p.teleport=true;f.Update();assert(!f.p.vehicle && f.p.mover==&f.p && f.p.teleports==0 && !Justice::rides.count(f.p.guid));}
 {Flight f(22);f.Fly();f.p.map=1;script.OnMapChanged(&f.p);assert(f.p.mover==&f.p && !f.p.vehicle && f.p.teleports==0 && !Justice::rides.count(f.p.guid));}
 {Flight f(23);f.Fly();f.p.ExitVehicle();world.erase(f.bird.guid);f.CompleteExit();f.Update();assert(f.p.teleports==1 && !Justice::rides.count(f.p.guid));}
 {Flight f(24);f.Fly();f.Update(300000);f.Update(10000);assert(f.p.teleports==1 && !Justice::rides.count(f.p.guid));} // missing exit callback
 {Flight f(25);f.Fly();f.p.ExitVehicle();f.CompleteExit();Creature other(f.p);npc_highmountain_justice otherAI(&other);f.p.vehicle=&other;f.p.mover=&other;f.Update(10000);assert(f.p.vehicle==&other && f.p.mover==&other && f.p.teleports==0 && !Justice::rides.count(f.p.guid));world.erase(other.guid);}
 {Player p(26);Move(p,Justice::Return);Creature b(p);Move(b,Justice::Return);b.hasKit=false;npc_highmountain_justice ai(&b);ai.IsSummonedBy(&p);assert(b.gone);spell_highmountain_justice spell;spell.caster=&p;spell.After();assert(!p.aura && Justice::CanStart(&p));world.erase(b.guid);}
 for(int reason=0;reason<8;++reason){Player p(30+reason);Move(p,Justice::Return);
  if(reason==0)p.alive=false;if(reason==1)p.quest.Status=QUEST_STATUS_COMPLETE;if(reason==2)p.map=1;if(reason==3)p.x+=20;if(reason==4)p.teleport=true;if(reason==5)p.combat=true;if(reason==6)p.transport=true;if(reason==7)p.flight=true;
  assert(!Justice::CanStart(&p));Creature b(p);Move(b,Justice::Return);npc_highmountain_justice ai(&b);ai.IsSummonedBy(&p);assert(b.gone && !Justice::rides.count(p.guid));world.erase(b.guid);
 }
 {Flight f(100);assert(f.p.credits==0);f.Board();f.Update();assert(f.p.credits==0);f.p.spline.final=true;f.Update();assert(f.p.credits==1 && f.bird.phases.count(5165));f.Update();assert(f.p.credits==1);
  f.p.quest.Status=QUEST_STATUS_COMPLETE;f.p.phases={9180};f.Arrive();assert(f.p.vehicle==&f.bird && f.bird.phases==f.p.phases);}
 {Flight f(101);f.Fly();Creature totem(f.p);totem.entry=97508;Creature ally(f.p);ally.entry=97498;Move(ally,f.p);
  npc_highmountain_justice_wounded ai(&ally);ai.Reset();assert(!ally.regen && ally.health==50);
  spell_highmountain_justice_heal h;h.caster=&totem;h.target=&ally;h.heal=40;h.Before();h.After();assert(f.p.healCredits==1);h.Before();h.After();assert(f.p.healCredits==1);
  Creature next(f.p);next.entry=97506;Move(next,f.p);h.target=&next;h.heal=0;h.Before();h.After();assert(f.p.healCredits==1);h.heal=40;next.full=true;h.Before();h.After();assert(f.p.healCredits==1);
  next.full=false;
  next.targetPhase=false;h.Before();h.After();assert(f.p.healCredits==1);next.targetPhase=true;
  next.entry=1;h.Before();h.After();assert(f.p.healCredits==1);next.entry=97506;
  totem.entry=1;h.Before();h.After();assert(f.p.healCredits==1);totem.entry=97508;
  f.p.quest.Status=QUEST_STATUS_COMPLETE;h.Before();h.After();assert(f.p.healCredits==1);f.p.quest.Status=QUEST_STATUS_INCOMPLETE;
  h.Before();h.After();assert(f.p.healCredits==2);world.erase(totem.guid);world.erase(ally.guid);world.erase(next.guid);}
 {Player p(102);Move(p,Justice::Return);p.sourcePhase=false;assert(!Justice::CanStart(&p));
  Creature other(p);other.targetPhase=false;npc_highmountain_justice_wounded ai(&other);ai.Reset();assert(other.regen && other.health==100);world.erase(other.guid);}
 assert(Justice::rides.empty());
 std::cout<<"PASS: real boarding before departure, exact linear node arrivals, native exit before cleanup, two owners/duplicates, retry, cancellation/death/phase/timeout, logout saved recovery, external transfer and missing actor recovery.\n";
}
