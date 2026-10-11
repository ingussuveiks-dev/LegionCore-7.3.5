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
enum {REACT_DEFENSIVE=1,UNIT_FIELD_FLAGS=2,UNIT_FLAG_NON_ATTACKABLE=2,UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_IMMUNE_TO_NPC=512,UNIT_FLAG_PACIFIED=131072,UNIT_FLAG_NOT_SELECTABLE=33554432,QUEST_OBJECTIVE_MONSTER=0,UNIT_FIELD_NPC_FLAGS=1,UNIT_NPC_FLAG_QUESTGIVER=2,CHAT_MSG_MONSTER_WHISPER=15,REACT_PASSIVE=0,POINT_MOTION_TYPE=1,TEMPSUMMON_TIMED_DESPAWN=1,EFFECT_0=0,SPELL_EFFECT_DAMAGE_FROM_MAX_HEALTH_PCT=165};
using SpellEffIndex=int;using SpellCastResult=int;enum{SPELL_CAST_OK=0,SPELL_FAILED_DONT_REPORT=1};
#include "QuestStatusData.inc"
struct ObjectGuid {unsigned value=0;void Clear(){value=0;}bool operator<(ObjectGuid const&r)const{return value<r.value;}bool operator==(ObjectGuid const&r)const{return value==r.value;}};
struct Position {float x=0,y=0,z=0,o=0;float GetExactDist(Position const*p)const{return std::sqrt((x-p->x)*(x-p->x)+(y-p->y)*(y-p->y)+(z-p->z)*(z-p->z));}};
struct Player;struct Creature;struct ScriptedAI;using TempSummon=Creature;
struct Unit:Position {virtual Player* GetCharmerOrOwnerPlayerOrPlayerItself()const{return nullptr;}Position GetPosition(){return *this;}void RemoveAurasDueToSpell(unsigned spell,ObjectGuid){assert(spell==195789);} float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}float GetOrientation(){return o;}float GetDistance(Unit* u){return GetExactDist(u);}virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}};
struct Vehicle {Unit* passenger=nullptr;Unit* GetPassenger(int seat){assert(!seat);return passenger;}};
struct Motion {unsigned follows=0,clears=0;void MoveFollow(Player*,float,float){++follows;}void Clear(){++clears;}unsigned point=0;Position destination;void MovePoint(unsigned id,Position const&p){point=id;destination=p;}};
struct QuestObjective {unsigned ObjectID; int Type=0;};
struct Quest {unsigned id=39455;unsigned GetQuestId()const{return id;}std::vector<QuestObjective> objectives{{244564,2},{97544,0}};auto const& GetObjectives()const{return objectives;}};
struct QuestStore {Quest q;Quest const* GetQuestTemplate(unsigned id){assert(id==39455);return &q;}} store;
auto sQuestDataStore=&store;
std::map<ObjectGuid,Creature*> world;std::vector<Creature*> allocated;unsigned serial=100;
struct Player:Unit {
 ObjectGuid guid;QuestStatusData quest;bool alive=true,teleport=false,flight=false,flying=false,visible=true,samePhase=true,failExit=false,failBoard=false,failSpawn=false,failBoss=false,los=true;unsigned zone=7503,map=1220,packets=0,phaseUpdates=0;Creature* vehicle=nullptr;std::map<unsigned,std::vector<ObjectGuid>> summons;std::vector<unsigned> casts;
 struct Event{uint64 delay;std::function<void()> action;};std::vector<Event> events;
 explicit Player(unsigned id):guid{id}{quest.Status=QUEST_STATUS_INCOMPLETE;x=5307.56f;y=4997.42f;z=627.498f;}
 Player* GetCharmerOrOwnerPlayerOrPlayerItself()const override{return const_cast<Player*>(this);}Player* ToPlayer()override{return this;}ObjectGuid GetGUID(){return guid;}bool IsAlive(){return alive;}unsigned GetMapId(){return map;}bool IsBeingTeleported(){return teleport;}bool isInFlight(){return flight;}bool IsFlying(){return flying;}unsigned GetZoneId(){return zone;}bool canSeeOrDetect(Unit*){return visible;}QuestStatus GetQuestStatus(unsigned q){assert(q==39455);return quest.Status;}
 int GetQuestObjectiveData(unsigned q,unsigned entry){assert(q==39455);return quest.ObjectiveData[entry];}void SetQuestObjectiveData(Quest const*,QuestObjective const* objective,unsigned n){quest.ObjectiveData[objective->ObjectID]=n;}void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,unsigned n){assert(n==1);++packets;}bool CanCompleteQuest(unsigned){return quest.ObjectiveData[244564]&&quest.ObjectiveData[97544];}void CompleteQuest(unsigned){quest.Status=QUEST_STATUS_COMPLETE;}void IncompleteQuest(unsigned){quest.Status=QUEST_STATUS_INCOMPLETE;}
 QuestStatusData* getQuestStatus(unsigned){return &quest;}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);void AddDelayedEvent(uint64 d,std::function<void()> f){events.push_back({d,f});}
 void SetQuestStatus(unsigned id,QuestStatus status){assert(id==39455&&status==QUEST_STATUS_INCOMPLETE);quest.Status=status;++phaseUpdates;}
 void Run(unsigned max){auto work=std::move(events);events.clear();for(auto&e:work){if(e.delay<=max)e.action();else events.push_back(e);}}
 auto GetSummonList(unsigned entry){return &summons[entry];}bool InSamePhase(Unit*){return samePhase;}float GetDistance(Creature*);
 Vehicle* GetVehicle();Creature* GetVehicleBase(){return vehicle;}void ExitVehicle();void CastSpell(float,float,float,unsigned,bool);bool IsWithinDistInMap(Unit* u,float d){return GetExactDist(u)<=d;}bool IsWithinLOSInMap(Unit*){return los;}
 Creature* SummonCreature(unsigned,float,float,float,float,unsigned,unsigned,ObjectGuid);
 void Abandon(){quest=QuestStatusData{};}void Accept(){quest=QuestStatusData{};quest.Status=QUEST_STATUS_INCOMPLETE;}
};
#include "QuestDelayedEvent.inc"
struct Creature:Unit {
 ObjectGuid guid;unsigned entry;Player* owner;ScriptedAI* ai=nullptr;Creature* guard=nullptr;unsigned flags=512;bool m_CanCallAssistance=true;double multiplier=1;uint64 health=100;unsigned combatStops=0,interrupts=0;bool phase=true;bool alive=true,gone=false,privateVisible=false,walk=false;unsigned delay=0,signals=0;Motion motion;Vehicle kit;
 Creature(unsigned e,Player*p):guid{++serial},entry(e),owner(p){world[guid]=this;if(p)p->summons[e].push_back(guid);}
 Player* GetCharmerOrOwnerPlayerOrPlayerItself()const override{return owner;}double GetHealthMultiplierForTarget(Unit*){return multiplier;}uint64 GetHealth(Unit*){return health;}void SetNoCallAssistance(bool){}void InterruptNonMeleeSpells(bool){++interrupts;}void CombatStop(bool){++combatStops;}void RemoveAllAuras(){}bool InSamePhase(Creature*){return phase;}Creature* FindNearestCreature(unsigned e,float range,bool living){assert(living);assert(e==93836);auto c=guard;return c&&c->alive&&!c->gone&&GetExactDist(c)<=range?c:nullptr;}Creature* ToCreature()override{return this;}TempSummon* ToTempSummon(){return owner?this:nullptr;}Unit* GetSummoner(){return owner;}ObjectGuid GetGUID(){return guid;}unsigned GetEntry(){return entry;}bool IsAlive(){return alive;}Vehicle* GetVehicleKit(){return entry==97781?&kit:nullptr;}
 void DespawnOrUnsummon(unsigned d=0){delay=d;if(!d){gone=true;if(owner&&owner->vehicle==this){owner->failExit=false;owner->ExitVehicle();}}}void Kill(Creature*c){assert(c==this);alive=false;}
 void RemoveFlag(unsigned,unsigned f){flags&=~f;}void SetFlag(unsigned,unsigned f){flags|=f;}void AddPlayerInPersonnalVisibilityList(ObjectGuid g){assert(owner&&owner->guid==g);privateVisible=true;}void SetReactState(unsigned){}void SetWalk(bool b){walk=b;}Motion* GetMotionMaster(){return &motion;}ScriptedAI* AI(){return ai;}
};
float Player::GetDistance(Creature*c){return GetExactDist(c);}Vehicle* Player::GetVehicle(){return vehicle?&vehicle->kit:nullptr;}void Player::ExitVehicle(){if(vehicle&&!failExit){vehicle->kit.passenger=nullptr;vehicle=nullptr;}}
struct ObjectAccessor {static Creature* GetCreature(Unit&,ObjectGuid g){auto i=world.find(g);return i==world.end()||i->second->gone?nullptr:i->second;}};
struct ScriptedAI {Creature* me;explicit ScriptedAI(Creature*c):me(c){c->ai=this;}virtual ~ScriptedAI()=default;virtual void IsSummonedBy(Unit*){}virtual void MovementInform(uint32,uint32){}virtual void UpdateAI(uint32){}virtual void PassengerBoarded(Unit*,int8,bool){}virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(unsigned,unsigned){++me->signals;}std::vector<unsigned> talks;void Talk(unsigned id,ObjectGuid){talks.push_back(id);}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual ~PlayerScript()=default;virtual void OnLogin(Player*){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}virtual void OnUpdate(Player*,uint32){}};


using DamageEffectType=int;
struct AggressorAI:ScriptedAI {
 unsigned updates=0,attacks=0;
 explicit AggressorAI(Creature*c):ScriptedAI(c){}
 void UpdateAI(uint32)override{++updates;}
 virtual bool CanAIAttack(Unit const*)const{return true;}
 virtual void MoveInLineOfSight(Unit*){}
 virtual void EnterEvadeMode(){}
 virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}
 void AttackStart(Player*){++attacks;}
};
struct GameObject:Unit {bool spawned=true;unsigned GetEntry(){return 244564;}ObjectGuid GetGUID(){return {50};}bool isSpawned(){return spawned;}};
struct GameObjectScript {explicit GameObjectScript(char const*){}virtual bool OnGossipHello(Player*,GameObject*){return false;}};
struct TextMgr {unsigned replies=0;ObjectGuid recipient;void SendChat(Creature*c,unsigned group,ObjectGuid p,unsigned type){assert(c->entry==93836&&!group&&type==CHAT_MSG_MONSTER_WHISPER);++replies;recipient=p;}} textMgr;
auto sCreatureTextMgr=&textMgr;
#include "HighmountainBloodTrial.inc"
Creature* Player::SummonCreature(unsigned entry,float sx,float sy,float sz,float so,unsigned,unsigned duration,ObjectGuid visibleTo)
{
 assert(entry==97539&&duration==120000&&visibleTo==guid);if(failSpawn)return nullptr;
 auto c=new Creature(entry,this);allocated.push_back(c);c->x=sx;c->y=sy;c->z=sz;c->o=so;c->privateVisible=true;return c;
}
void Player::CastSpell(float sx,float sy,float sz,unsigned spell,bool triggered)
{
 assert(spell==192443&&triggered);casts.push_back(spell);if(failBoss)return;
 auto c=new Creature(97544,this);allocated.push_back(c);c->x=sx;c->y=sy;c->z=sz;
 new npc_blood_trial_warmonger(c);c->AI()->IsSummonedBy(this);
}
void Move(Unit&u,Position const&pos){static_cast<Position&>(u)=pos;}
int main()
{
 go_blood_trial_altar altar;player_blood_trial service;Player p(1),q(2);GameObject go;Move(go,p);
 p.los=false;altar.OnGossipHello(&p,&go);assert(!p.packets);p.los=true;p.visible=false;altar.OnGossipHello(&p,&go);assert(p.casts.empty());p.visible=true;p.samePhase=false;altar.OnGossipHello(&p,&go);assert(!p.packets);p.samePhase=true;
 p.failSpawn=true;altar.OnGossipHello(&p,&go);assert(!p.packets&&p.casts.empty());p.failSpawn=false;p.failBoss=true;altar.OnGossipHello(&p,&go);assert(!p.packets&&!BloodTrial::Existing(&p,97539));p.failBoss=false;
 altar.OnGossipHello(&p,&go);auto c=BloodTrial::Existing(&p,97544);auto witch=BloodTrial::Existing(&p,97539);auto ai=static_cast<npc_blood_trial_warmonger*>(c->AI());assert(c&&witch&&c->privateVisible&&witch->privateVisible&&p.packets==1&&BloodTrial::Done(&p,244564)&&!BloodTrial::Done(&p,97544));assert(!(c->flags&UNIT_FLAG_IMMUNE_TO_NPC)&&!c->m_CanCallAssistance&&ai->attacks==1);
 auto casts=p.casts.size();altar.OnGossipHello(&p,&go);assert(p.casts.size()==casts&&p.packets==1);
 // The same altar supports a second player's independent fight immediately.
 altar.OnGossipHello(&q,&go);auto other=BloodTrial::Existing(&q,97544);assert(other&&other!=c&&q.packets==1);
 Creature pet(1,&p),outsiderPet(1,&q),torok(93836,nullptr);Move(torok,{5293.96f,4965.95f,626.776f,0});c->guard=&torok;
 assert(ai->CanAIAttack(&p)&&ai->CanAIAttack(&pet)&&!ai->CanAIAttack(&q)&&!ai->CanAIAttack(&outsiderPet));
 uint32 damage=1000000000;ai->DamageTaken(&q,damage,0);assert(!damage&&p.packets==1);damage=1000000000;ai->DamageTaken(&outsiderPet,damage,0);assert(!damage&&p.packets==1);
 ai->UpdateAI(1);assert(ai->updates==1);c->health=100;c->multiplier=10;damage=999;ai->DamageTaken(&p,damage,0);assert(damage==999&&!ai->surrendered);damage=1000;ai->DamageTaken(&pet,damage,0);assert(!damage&&ai->surrendered&&p.packets==2&&p.quest.Status==QUEST_STATUS_COMPLETE&&q.packets==1&&c->IsAlive());assert(textMgr.recipient==p.guid&&c->combatStops==1&&c->interrupts==1&&c->delay==3000);
 damage=1000000000;ai->DamageTaken(&p,damage,0);assert(!damage&&p.packets==2);altar.OnGossipHello(&p,&go);assert(p.casts.size()==casts);
 // Fresh scaled one-shot, including a multiplier below 1.0, must surrender.
 auto otherAi=static_cast<npc_blood_trial_warmonger*>(other->AI());other->multiplier=0.01;other->health=10000;damage=1000000000;otherAi->DamageTaken(&q,damage,0);assert(!damage&&q.quest.Status==QUEST_STATUS_COMPLETE&&other->IsAlive());
 service.OnLogout(&p);assert(c->gone&&witch->gone);service.OnLogout(&q);
 Player r(3);altar.OnGossipHello(&r,&go);auto old=BloodTrial::Existing(&r,97544);auto oldWitch=BloodTrial::Existing(&r,97539);r.Abandon();r.Accept();old->AI()->UpdateAI(1);assert(old->gone&&oldWitch->gone&&!BloodTrial::Done(&r,244564));altar.OnGossipHello(&r,&go);auto resumed=BloodTrial::Existing(&r,97544);assert(resumed&&resumed!=old);
 r.alive=false;resumed->AI()->UpdateAI(1);assert(resumed->gone&&!BloodTrial::Existing(&r,97539)&&!BloodTrial::Done(&r,97544));r.alive=true;altar.OnGossipHello(&r,&go);assert(BloodTrial::Existing(&r,97544));service.OnLogout(&r);altar.OnGossipHello(&r,&go);assert(BloodTrial::Existing(&r,97544)&&!BloodTrial::Done(&r,97544));
 auto retry=BloodTrial::Existing(&r,97544);r.samePhase=false;retry->AI()->UpdateAI(1);assert(retry->gone);r.samePhase=true;altar.OnGossipHello(&r,&go);retry=BloodTrial::Existing(&r,97544);r.x+=50;retry->AI()->UpdateAI(1);assert(retry->gone);Move(r,go);altar.OnGossipHello(&r,&go);retry=BloodTrial::Existing(&r,97544);static_cast<npc_blood_trial_warmonger*>(retry->AI())->EnterEvadeMode();assert(retry->gone&&!BloodTrial::Existing(&r,97539));
 altar.OnGossipHello(&r,&go);service.OnMapChanged(&r);assert(!BloodTrial::Existing(&r,97544));
 for(auto actor:allocated){delete actor->ai;delete actor;}
 std::cout<<"PASS: extracted altar/challenger handlers, ordered credit, two-player isolation, owner pets, normalized lethal/billion-damage surrender, no duplicate reward, failed summons and death/relog/abandon/evade/map/phase/range retry.\n";
}
