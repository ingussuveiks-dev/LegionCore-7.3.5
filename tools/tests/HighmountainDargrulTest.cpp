#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
using uint8=uint8_t;using int8=int8_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
enum QuestStatus {QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_REWARDED=6};
enum {REACT_PASSIVE=0,POINT_MOTION_TYPE=1,TEMPSUMMON_TIMED_DESPAWN=1,EFFECT_0=0,SPELL_EFFECT_DAMAGE_FROM_MAX_HEALTH_PCT=165};
using SpellEffIndex=int;
#include "QuestStatusData.inc"
struct ObjectGuid {unsigned value=0;bool operator<(ObjectGuid const&r)const{return value<r.value;}bool operator==(ObjectGuid const&r)const{return value==r.value;}};
struct Position {float x=0,y=0,z=0,o=0;float GetExactDist(Position const*p)const{return std::sqrt((x-p->x)*(x-p->x)+(y-p->y)*(y-p->y)+(z-p->z)*(z-p->z));}};
struct Player;struct Creature;struct ScriptedAI;using TempSummon=Creature;
struct Unit:Position {virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}};
struct Vehicle {Unit* passenger=nullptr;Unit* GetPassenger(int seat){assert(!seat);return passenger;}};
struct Motion {unsigned point=0;Position destination;void MovePoint(unsigned id,Position const&p){point=id;destination=p;}};
struct QuestObjective {unsigned ObjectID;};
struct Quest {std::vector<QuestObjective> objectives{{97871},{94610},{97781},{93805}};auto const& GetObjectives()const{return objectives;}};
struct QuestStore {Quest q;Quest const* GetQuestTemplate(unsigned id){assert(id==39027);return &q;}} store;
auto sQuestDataStore=&store;
std::map<ObjectGuid,Creature*> world;std::vector<Creature*> allocated;unsigned serial=100;
struct Player:Unit {
 ObjectGuid guid;QuestStatusData quest;bool alive=true,teleport=false,flight=false,samePhase=true,failExit=false,failBoard=false,failSpawn=false;unsigned map=1220,packets=0,phaseUpdates=0;Creature* vehicle=nullptr;std::map<unsigned,std::vector<ObjectGuid>> summons;std::vector<unsigned> casts;
 struct Event{uint64 delay;std::function<void()> action;};std::vector<Event> events;
 explicit Player(unsigned id):guid{id}{quest.Status=QUEST_STATUS_INCOMPLETE;x=3486.07f;y=4596.64f;z=681.78f;}
 Player* ToPlayer()override{return this;}ObjectGuid GetGUID(){return guid;}bool IsAlive(){return alive;}unsigned GetMapId(){return map;}bool IsBeingTeleported(){return teleport;}bool isInFlight(){return flight;}QuestStatus GetQuestStatus(unsigned q){assert(q==39027);return quest.Status;}
 int GetQuestObjectiveData(unsigned q,unsigned entry){assert(q==39027);return quest.ObjectiveData[entry];}void SetQuestObjectiveData(Quest const*,QuestObjective const* objective,unsigned n){quest.ObjectiveData[objective->ObjectID]=n;}void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,unsigned n){assert(n==1);++packets;}bool CanCompleteQuest(unsigned){return quest.ObjectiveData[94610]&&quest.ObjectiveData[93805];}void CompleteQuest(unsigned){quest.Status=QUEST_STATUS_COMPLETE;}void IncompleteQuest(unsigned){quest.Status=QUEST_STATUS_INCOMPLETE;}
 QuestStatusData* getQuestStatus(unsigned){return &quest;}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);void AddDelayedEvent(uint64 d,std::function<void()> f){events.push_back({d,f});}
 void SetQuestStatus(unsigned id,QuestStatus status){assert(id==39027&&status==QUEST_STATUS_INCOMPLETE);quest.Status=status;++phaseUpdates;}
 void Run(unsigned max){auto work=std::move(events);events.clear();for(auto&e:work){if(e.delay<=max)e.action();else events.push_back(e);}}
 auto GetSummonList(unsigned entry){return &summons[entry];}bool InSamePhase(Creature*){return samePhase;}float GetDistance(Creature*);
 Vehicle* GetVehicle();Creature* GetVehicleBase(){return vehicle;}void ExitVehicle();void CastSpell(Player*,unsigned,bool);void RemoveAurasDueToSpell(unsigned);
 Creature* SummonCreature(unsigned,float,float,float,float,unsigned,unsigned,ObjectGuid);
 void Abandon(){quest=QuestStatusData{};}void Accept(){quest=QuestStatusData{};quest.Status=QUEST_STATUS_INCOMPLETE;}
};
#include "QuestDelayedEvent.inc"
struct Creature:Unit {
 ObjectGuid guid;unsigned entry;Player* owner;ScriptedAI* ai=nullptr;bool alive=true,gone=false,privateVisible=false,walk=false;unsigned delay=0,signals=0;Motion motion;Vehicle kit;
 Creature(unsigned e,Player*p):guid{++serial},entry(e),owner(p){world[guid]=this;if(p)p->summons[e].push_back(guid);}
 Creature* ToCreature()override{return this;}TempSummon* ToTempSummon(){return owner?this:nullptr;}Unit* GetSummoner(){return owner;}ObjectGuid GetGUID(){return guid;}unsigned GetEntry(){return entry;}bool IsAlive(){return alive;}Vehicle* GetVehicleKit(){return entry==97781?&kit:nullptr;}
 void DespawnOrUnsummon(unsigned d=0){delay=d;if(!d){gone=true;if(owner&&owner->vehicle==this){owner->failExit=false;owner->ExitVehicle();}}}void Kill(Creature*c){assert(c==this);alive=false;}
 void AddPlayerInPersonnalVisibilityList(ObjectGuid g){assert(owner&&owner->guid==g);privateVisible=true;}void SetReactState(unsigned){}void SetWalk(bool b){walk=b;}Motion* GetMotionMaster(){return &motion;}ScriptedAI* AI(){return ai;}
};
float Player::GetDistance(Creature*c){return GetExactDist(c);}Vehicle* Player::GetVehicle(){return vehicle?&vehicle->kit:nullptr;}void Player::ExitVehicle(){if(vehicle&&!failExit){vehicle->kit.passenger=nullptr;vehicle=nullptr;}}
struct ObjectAccessor {static Creature* GetCreature(Player&,ObjectGuid g){auto i=world.find(g);return i==world.end()||i->second->gone?nullptr:i->second;}};
struct ScriptedAI {Creature* me;explicit ScriptedAI(Creature*c):me(c){c->ai=this;}virtual ~ScriptedAI()=default;virtual void IsSummonedBy(Unit*){}virtual void MovementInform(uint32,uint32){}virtual void UpdateAI(uint32){}virtual void PassengerBoarded(Unit*,int8,bool){}virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(unsigned,unsigned){++me->signals;}void Talk(unsigned,ObjectGuid){}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual ~PlayerScript()=default;virtual void OnLogin(Player*){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}virtual void OnUpdate(Player*,uint32){}};
struct Hook {void operator+=(int){}};
struct SpellScript {Player* caster=nullptr;Creature* target=nullptr;bool prevented=false;Hook OnEffectHitTarget;virtual ~SpellScript()=default;virtual void Register(){}Unit* GetCaster(){return caster;}Creature* GetHitCreature(){return target;}void PreventHitDefaultEffect(int){prevented=true;}};
#define PrepareSpellScript(x) public:
#define SpellEffectFn(...) 0
#include "HighmountainDargrul.inc"
Creature* Make(unsigned entry,Player*p,Position pos)
{
 auto c=new Creature(entry,p);allocated.push_back(c);static_cast<Position&>(*c)=pos;
 if(entry==97552)new npc_dargrul_escort(c);else if(entry==97781)new npc_dargrul_grasp(c);else new ScriptedAI(c);
 c->AI()->IsSummonedBy(p);return c;
}
Creature* Player::SummonCreature(unsigned e,float px,float py,float pz,float facing,unsigned type,unsigned time,ObjectGuid viewer)
{assert(type==TEMPSUMMON_TIMED_DESPAWN&&time==30000&&viewer==guid);if(failSpawn)return nullptr;auto c=Make(e,this,{px,py,pz,facing});c->privateVisible=true;return c;}
void Player::CastSpell(Player*p,unsigned spell,bool trigger)
{
 assert(p==this&&trigger);casts.push_back(spell);if(failSpawn)return;
 auto c=Make(spell==205051?97552:97781,this,*this);
 if(spell==192936&&!failBoard&&!c->gone){vehicle=c;c->kit.passenger=this;c->AI()->PassengerBoarded(this,0,true);}
}
void Player::RemoveAurasDueToSpell(unsigned spell)
{assert(spell==205051);for(auto g:summons[97552])if(auto c=ObjectAccessor::GetCreature(*this,g))c->DespawnOrUnsummon();}
void Move(Player&p,Position const&pos){static_cast<Position&>(p)=pos;}
void Follow(Player&p,player_dargrul_event& service)
{
 service.OnUpdate(&p,1000);auto c=Dargrul::Existing(&p,97552);assert(c);auto ai=static_cast<npc_dargrul_escort*>(c->AI());
 while(ai->point<7){ai->UpdateAI(1);assert(ai->moving);static_cast<Position&>(*c)=c->motion.destination;Move(p,c->motion.destination);ai->MovementInform(POINT_MOTION_TYPE,c->motion.point);}
 ai->UpdateAI(1);assert(Dargrul::Done(&p,97871)&&c->gone&&p.phaseUpdates==1);
}
int main()
{
 player_dargrul_event service;Player p(1),q(2);
 // No remote escort spawn or credit. Stop between waypoints for the owner.
 Move(p,{3541,4770,714,0});service.OnUpdate(&p,1000);assert(p.casts.empty());Move(p,Dargrul::Trail[0]);service.OnUpdate(&p,1000);
 auto oro=Dargrul::Existing(&p,97552);auto escort=static_cast<npc_dargrul_escort*>(oro->AI());assert(oro->privateVisible&&oro->walk);
 p.y+=25;escort->UpdateAI(1000);assert(!escort->moving&&!Dargrul::Done(&p,97871));Move(p,Dargrul::Trail[0]);
 escort->MovementInform(POINT_MOTION_TYPE,7);assert(!Dargrul::Done(&p,97871));Follow(p,service);assert(!Dargrul::Done(&q,97871));
 // Two complete escorts create independent scene actors. No radius/group credit.
 Follow(q,service);service.OnUpdate(&p,1000);service.OnUpdate(&q,1000);
 auto boss=Dargrul::Existing(&p,94610);auto boss2=Dargrul::Existing(&q,94610);assert(boss&&boss2&&boss!=boss2&&boss->privateVisible&&boss->signals==1);
 assert(!Dargrul::Done(&p,94610)&&!Dargrul::Existing(&p,97781));service.OnUpdate(&p,1000);assert(Dargrul::Existing(&p,94610)==boss);
 p.Run(5500);auto hand=Dargrul::Existing(&p,97781);assert(hand&&p.vehicle==hand&&Dargrul::Done(&p,94610));assert(!Dargrul::Done(&q,94610));
 spell_dargrul_escape escape;escape.caster=&q;escape.target=hand;escape.Escape(0);assert(escape.prevented&&hand->alive&&!Dargrul::Done(&q,97781));
 escape.caster=&p;hand->kit.passenger=nullptr;escape.Escape(0);assert(hand->alive&&!Dargrul::Done(&p,97781));hand->kit.passenger=&p;
 p.alive=false;escape.Escape(0);p.alive=true;p.teleport=true;escape.Escape(0);p.teleport=false;p.map=1;escape.Escape(0);p.map=1220;assert(hand->alive&&!Dargrul::Done(&p,97781));
 p.failExit=true;escape.Escape(0);assert(hand->alive&&!Dargrul::Done(&p,97781));p.failExit=false;
 escape.Escape(0);assert(!p.vehicle&&!hand->alive&&Dargrul::Done(&p,97781)&&!Dargrul::Done(&p,93805)&&p.phaseUpdates==3);auto count=p.packets;escape.Escape(0);assert(p.packets==count);
 // Failed boarding does not confront; orphan hand disappears and another scene can retry.
 q.failBoard=true;q.Run(5500);auto empty=Dargrul::Existing(&q,97781);assert(empty&&!Dargrul::Done(&q,94610));empty->AI()->UpdateAI(3000);assert(empty->gone);
 q.failBoard=false;q.Run(30000);service.OnUpdate(&q,1000);q.Run(5500);assert(q.vehicle&&Dargrul::Done(&q,94610));
 // Disconnect cleanup releases the seat; reconnect can retry escape without replaying credit.
 service.OnLogout(&q);assert(!q.vehicle);service.OnUpdate(&q,1000);assert(q.vehicle);auto retry=q.vehicle;auto before=q.packets;retry->AI()->PassengerBoarded(&q,0,true);assert(q.packets==before);
 // Killing/exiting the hand without the actual escape interaction gives no TALKTO credit.
 q.ExitVehicle();retry->AI()->UpdateAI(3000);assert(retry->gone&&!Dargrul::Done(&q,97781));service.OnUpdate(&q,1000);assert(q.vehicle);
 // Same-tick abandonment/reacceptance expires old NPCs and production delayed actions.
 q.Abandon();q.Accept();auto stale=q.vehicle;stale->AI()->UpdateAI(1);assert(stale->gone&&!q.vehicle);q.Run(30000);assert(!Dargrul::Done(&q,94610));service.OnUpdate(&q,1000);assert(!Dargrul::Existing(&q,94610));
 // An old scene timeout must not unlock a newer personal scene.
 Player r(3);Follow(r,service);service.OnUpdate(&r,1000);auto old=Dargrul::Existing(&r,94610);auto oldTimeout=r.events.back().action;Move(r,{0,0,0,0});service.OnUpdate(&r,1000);assert(old->gone);Move(r,Dargrul::Trail[6]);service.OnUpdate(&r,1000);auto current=Dargrul::Existing(&r,94610);assert(current&&current!=old);auto scenes=r.summons[94610].size();oldTimeout();service.OnUpdate(&r,1000);assert(r.summons[94610].size()==scenes);r.Run(5500);assert(r.vehicle);service.OnMapChanged(&r);assert(!r.vehicle);
 // Missing actors and failed path arrival are retryable, never automatic progress.
 Player f(4);f.failSpawn=true;service.OnUpdate(&f,1000);assert(!Dargrul::Existing(&f,97552));f.failSpawn=false;service.OnUpdate(&f,1000);auto stuck=Dargrul::Existing(&f,97552);stuck->AI()->UpdateAI(1);stuck->AI()->UpdateAI(30000);assert(stuck->gone&&!Dargrul::Done(&f,97871));
 service.OnUpdate(&f,1000);assert(Dargrul::Existing(&f,97552));f.alive=false;service.OnUpdate(&f,1000);assert(!Dargrul::Existing(&f,97552));
 // Repair previously premature exit only; preserve legitimate/rewarded saves and slot data.
 Player save(5);save.quest.ObjectiveData={{97871,1},{94610,1},{93805,1}};save.quest.Status=QUEST_STATUS_COMPLETE;service.OnLogin(&save);assert(save.quest.Status==QUEST_STATUS_INCOMPLETE&&!Dargrul::Done(&save,93805)&&Dargrul::Done(&save,94610));
 save.quest.ObjectiveData[97781]=1;save.quest.ObjectiveData[93805]=1;save.quest.Status=QUEST_STATUS_COMPLETE;service.OnLogin(&save);assert(save.quest.Status==QUEST_STATUS_COMPLETE);save.quest.Status=QUEST_STATUS_REWARDED;save.quest.ObjectiveData[97781]=0;service.OnLogin(&save);assert(Dargrul::Done(&save,93805));
 for(auto c:allocated){delete c->ai;delete c;}
 std::cout<<"PASS: production escort/scene/grasp handlers, two owners, arrival and boarding gates, escape effect/seat validation, failed spawn/path/boarding/exit, reconnect/death/abandon lifetime, and old save reconciliation.\n";
}
