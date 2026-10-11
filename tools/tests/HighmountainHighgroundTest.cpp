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
using SpellEffIndex=int;using SpellCastResult=int;enum{SPELL_CAST_OK=0,SPELL_FAILED_DONT_REPORT=1};
#include "QuestStatusData.inc"
struct ObjectGuid {unsigned value=0;bool operator<(ObjectGuid const&r)const{return value<r.value;}bool operator==(ObjectGuid const&r)const{return value==r.value;}};
struct Position {float x=0,y=0,z=0,o=0;float GetExactDist(Position const*p)const{return std::sqrt((x-p->x)*(x-p->x)+(y-p->y)*(y-p->y)+(z-p->z)*(z-p->z));}};
struct Player;struct Creature;struct ScriptedAI;using TempSummon=Creature;
struct Unit:Position {virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}};
struct Vehicle {Unit* passenger=nullptr;Unit* GetPassenger(int seat){assert(!seat);return passenger;}};
struct Motion {unsigned follows=0,clears=0;void MoveFollow(Player*,float,float){++follows;}void Clear(){++clears;}unsigned point=0;Position destination;void MovePoint(unsigned id,Position const&p){point=id;destination=p;}};
struct QuestObjective {unsigned ObjectID;};
struct Quest {std::vector<QuestObjective> objectives{{94547},{95078}};auto const& GetObjectives()const{return objectives;}};
struct QuestStore {Quest q;Quest const* GetQuestTemplate(unsigned id){assert(id==38909);return &q;}} store;
auto sQuestDataStore=&store;
std::map<ObjectGuid,Creature*> world;std::vector<Creature*> allocated;unsigned serial=100;
struct Player:Unit {
 ObjectGuid guid;QuestStatusData quest;bool alive=true,teleport=false,flight=false,flying=false,visible=true,samePhase=true,failExit=false,failBoard=false,failSpawn=false;unsigned zone=7503,map=1220,packets=0,phaseUpdates=0;Creature* vehicle=nullptr;std::map<unsigned,std::vector<ObjectGuid>> summons;std::vector<unsigned> casts;
 struct Event{uint64 delay;std::function<void()> action;};std::vector<Event> events;
 explicit Player(unsigned id):guid{id}{quest.Status=QUEST_STATUS_INCOMPLETE;x=3486.07f;y=4596.64f;z=681.78f;}
 Player* ToPlayer()override{return this;}ObjectGuid GetGUID(){return guid;}bool IsAlive(){return alive;}unsigned GetMapId(){return map;}bool IsBeingTeleported(){return teleport;}bool isInFlight(){return flight;}bool IsFlying(){return flying;}unsigned GetZoneId(){return zone;}bool canSeeOrDetect(Creature*){return visible;}QuestStatus GetQuestStatus(unsigned q){assert(q==38909);return quest.Status;}
 int GetQuestObjectiveData(unsigned q,unsigned entry){assert(q==38909);return quest.ObjectiveData[entry];}void SetQuestObjectiveData(Quest const*,QuestObjective const* objective,unsigned n){quest.ObjectiveData[objective->ObjectID]=n;}void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,unsigned n){assert(n==1);++packets;}bool CanCompleteQuest(unsigned){return quest.ObjectiveData[94547]&&quest.ObjectiveData[95078];}void CompleteQuest(unsigned){quest.Status=QUEST_STATUS_COMPLETE;}void IncompleteQuest(unsigned){quest.Status=QUEST_STATUS_INCOMPLETE;}
 QuestStatusData* getQuestStatus(unsigned){return &quest;}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);void AddDelayedEvent(uint64 d,std::function<void()> f){events.push_back({d,f});}
 void SetQuestStatus(unsigned id,QuestStatus status){assert(id==38909&&status==QUEST_STATUS_INCOMPLETE);quest.Status=status;++phaseUpdates;}
 void Run(unsigned max){auto work=std::move(events);events.clear();for(auto&e:work){if(e.delay<=max)e.action();else events.push_back(e);}}
 auto GetSummonList(unsigned entry){return &summons[entry];}bool InSamePhase(Creature*){return samePhase;}float GetDistance(Creature*);
 Vehicle* GetVehicle();Creature* GetVehicleBase(){return vehicle;}void ExitVehicle();void CastSpell(Player*,unsigned,bool);void RemoveAurasDueToSpell(unsigned);
 Creature* SummonCreature(unsigned,float,float,float,float,unsigned,unsigned,ObjectGuid);
 void Abandon(){quest=QuestStatusData{};}void Accept(){quest=QuestStatusData{};quest.Status=QUEST_STATUS_INCOMPLETE;}
};
#include "QuestDelayedEvent.inc"
struct Creature:Unit {
 ObjectGuid guid;unsigned entry;Player* owner;ScriptedAI* ai=nullptr;Creature* bridge=nullptr;Creature* mayla=nullptr;bool phase=true;bool alive=true,gone=false,privateVisible=false,walk=false;unsigned delay=0,signals=0;Motion motion;Vehicle kit;
 Creature(unsigned e,Player*p):guid{++serial},entry(e),owner(p){world[guid]=this;if(p)p->summons[e].push_back(guid);}
 bool InSamePhase(Creature*){return phase;}Creature* FindNearestCreature(unsigned e,float range,bool living){assert(living);auto c=e==94547?bridge:mayla;return c&&GetExactDist(c)<=range?c:nullptr;}Creature* ToCreature()override{return this;}TempSummon* ToTempSummon(){return owner?this:nullptr;}Unit* GetSummoner(){return owner;}ObjectGuid GetGUID(){return guid;}unsigned GetEntry(){return entry;}bool IsAlive(){return alive;}Vehicle* GetVehicleKit(){return entry==97781?&kit:nullptr;}
 void DespawnOrUnsummon(unsigned d=0){delay=d;if(!d){gone=true;if(owner&&owner->vehicle==this){owner->failExit=false;owner->ExitVehicle();}}}void Kill(Creature*c){assert(c==this);alive=false;}
 void AddPlayerInPersonnalVisibilityList(ObjectGuid g){assert(owner&&owner->guid==g);privateVisible=true;}void SetReactState(unsigned){}void SetWalk(bool b){walk=b;}Motion* GetMotionMaster(){return &motion;}ScriptedAI* AI(){return ai;}
};
float Player::GetDistance(Creature*c){return GetExactDist(c);}Vehicle* Player::GetVehicle(){return vehicle?&vehicle->kit:nullptr;}void Player::ExitVehicle(){if(vehicle&&!failExit){vehicle->kit.passenger=nullptr;vehicle=nullptr;}}
struct ObjectAccessor {static Creature* GetCreature(Player&,ObjectGuid g){auto i=world.find(g);return i==world.end()||i->second->gone?nullptr:i->second;}};
struct ScriptedAI {Creature* me;explicit ScriptedAI(Creature*c):me(c){c->ai=this;}virtual ~ScriptedAI()=default;virtual void IsSummonedBy(Unit*){}virtual void MovementInform(uint32,uint32){}virtual void UpdateAI(uint32){}virtual void PassengerBoarded(Unit*,int8,bool){}virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(unsigned,unsigned){++me->signals;}void Talk(unsigned,ObjectGuid){}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual ~PlayerScript()=default;virtual void OnLogin(Player*){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}virtual void OnUpdate(Player*,uint32){}};
struct Hook {void operator+=(int){}};
struct SpellScript {Player* caster=nullptr;Creature* target=nullptr;bool prevented=false;Hook OnEffectHitTarget,OnCheckCast;virtual ~SpellScript()=default;virtual void Register(){}Unit* GetCaster(){return caster;}Creature* GetHitCreature(){return target;}void PreventHitDefaultEffect(int){prevented=true;}};
#define PrepareSpellScript(x) public:
#define SpellCheckCastFn(...) 0
#include "HighmountainHighground.inc"
Creature* Make(Player*p)
{
 auto c=new Creature(95078,p);allocated.push_back(c);static_cast<Position&>(*c)=*p;new npc_highground_jale(c);c->AI()->IsSummonedBy(p);return c;
}
void Player::CastSpell(Player*p,unsigned spell,bool triggered)
{
 assert(p==this&&spell==188583&&triggered);spell_highground_jale gate;gate.caster=this;if(gate.Check()!=SPELL_CAST_OK)return;casts.push_back(spell);if(!failSpawn)Make(this);
}
void Move(Unit&u,Position const&pos){static_cast<Position&>(u)=pos;}
int main()
{
 player_highground_jale service;Player p(1),q(2);Creature bridge(94547,nullptr),mayla(93826,nullptr);Move(bridge,{4020.24f,4568.14f,662.637f,0});Move(mayla,HighGround::Route[2]);
 service.OnUpdate(&p,1000);auto j=HighGround::Existing(&p);assert(j&&j->privateVisible&&j->motion.follows==1);auto ai=static_cast<npc_highground_jale*>(j->AI());j->bridge=&bridge;j->mayla=&mayla;
 p.CastSpell(&p,188583,true);assert(p.summons[95078].size()==1);ai->UpdateAI(1000);assert(!HighGround::Done(&p,94547)&&!HighGround::Done(&p,95078));
 // Neither arriving alone nor another player's Jale can complete this escort.
 Move(p,bridge);ai->UpdateAI(1000);assert(j->gone&&!HighGround::Done(&p,94547));service.OnUpdate(&p,1000);j=HighGround::Existing(&p);ai=static_cast<npc_highground_jale*>(j->AI());j->bridge=&bridge;j->mayla=&mayla;
 j->phase=false;ai->UpdateAI(1000);assert(!HighGround::Done(&p,94547));j->phase=true;ai->UpdateAI(1000);assert(HighGround::Done(&p,94547)&&!HighGround::Done(&p,95078)&&ai->moving&&p.phaseUpdates==1);assert(!HighGround::Done(&q,94547));
 auto packets=p.packets;ai->MovementInform(POINT_MOTION_TYPE,3);assert(p.packets==packets&&!ai->arrived);
 // Jale pauses a leg when the owner stops following; no arrival callback, no credit.
 Move(*j,{p.x+35,p.y,p.z,0});ai->UpdateAI(1000);assert(!ai->moving);Move(p,*j);ai->UpdateAI(1000);assert(ai->moving);
 while(!ai->arrived){Move(*j,j->motion.destination);Move(p,*j);ai->MovementInform(POINT_MOTION_TYPE,j->motion.point);if(!ai->arrived)ai->UpdateAI(1000);}
 p.y+=20;ai->UpdateAI(1000);assert(!HighGround::Done(&p,95078));Move(p,mayla);p.visible=false;ai->UpdateAI(1000);assert(!HighGround::Done(&p,95078));p.visible=true;ai->UpdateAI(1000);assert(HighGround::Done(&p,95078)&&p.quest.Status==QUEST_STATUS_COMPLETE&&j->gone);assert(!HighGround::Done(&q,95078));service.OnUpdate(&p,1000);assert(!HighGround::Existing(&p));
 // Native hearthstone/flight arrival can resume at Mayla, but still requires an
 // actual companion movement/arrival rather than credit from spawn or proximity.
 Move(q,mayla);service.OnUpdate(&q,1000);auto other=HighGround::Existing(&q);other->mayla=&mayla;auto otherAi=static_cast<npc_highground_jale*>(other->AI());otherAi->UpdateAI(1000);assert(HighGround::Done(&q,94547)&&!HighGround::Done(&q,95078)&&otherAi->point==2);service.OnLogout(&q);assert(other->gone);service.OnUpdate(&q,1000);other=HighGround::Existing(&q);other->mayla=&mayla;otherAi=static_cast<npc_highground_jale*>(other->AI());otherAi->UpdateAI(1000);otherAi->MovementInform(POINT_MOTION_TYPE,3);otherAi->UpdateAI(1000);assert(q.quest.Status==QUEST_STATUS_COMPLETE);
 // Same-tick abandonment/reacceptance invalidates the old summon and guarded work.
 Player r(3);service.OnUpdate(&r,1000);auto old=HighGround::Existing(&r);r.Abandon();r.Accept();old->AI()->UpdateAI(1);assert(old->gone);service.OnUpdate(&r,1000);assert(HighGround::Existing(&r)&&HighGround::Existing(&r)!=old);assert(!r.packets);
 r.flying=true;service.OnUpdate(&r,1000);assert(!HighGround::Existing(&r));r.flying=false;r.zone=1;service.OnUpdate(&r,1000);assert(!HighGround::Existing(&r));r.zone=7731;service.OnUpdate(&r,1000);assert(HighGround::Existing(&r));service.OnMapChanged(&r);assert(!HighGround::Existing(&r));
 r.failSpawn=true;service.OnUpdate(&r,1000);assert(!HighGround::Existing(&r));r.failSpawn=false;service.OnUpdate(&r,1000);auto retry=HighGround::Existing(&r);assert(retry);r.alive=false;service.OnUpdate(&r,1000);assert(retry->gone);
 Player stuck(4);Move(stuck,bridge);service.OnUpdate(&stuck,1000);auto lost=HighGround::Existing(&stuck);lost->bridge=&bridge;lost->AI()->UpdateAI(1);lost->AI()->UpdateAI(120000);assert(lost->gone&&!HighGround::Done(&stuck,95078));service.OnUpdate(&stuck,1000);assert(HighGround::Existing(&stuck));
 Player save(5);save.quest.ObjectiveData[95078]=1;save.quest.Status=QUEST_STATUS_COMPLETE;service.OnLogin(&save);assert(!HighGround::Done(&save,95078)&&save.quest.Status==QUEST_STATUS_INCOMPLETE);save.quest.ObjectiveData={{94547,1},{95078,1}};save.quest.Status=QUEST_STATUS_COMPLETE;service.OnLogin(&save);assert(save.quest.Status==QUEST_STATUS_COMPLETE);save.quest.Status=QUEST_STATUS_REWARDED;save.quest.ObjectiveData[94547]=0;service.OnLogin(&save);assert(HighGround::Done(&save,95078));
 for(auto c:allocated){delete c->ai;delete c;}std::cout<<"PASS: extracted Jale summon/escort/arrival/recovery handlers, owner isolation, phase and distance checks, ordered credit, failed path/summon, landing/reconnect/death/map/abandon cleanup and legacy save repair.\n";
}
