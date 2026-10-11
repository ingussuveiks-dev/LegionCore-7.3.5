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
struct Quest {unsigned id=39860;unsigned GetQuestId()const{return id;}std::vector<QuestObjective> objectives{{95259,2},{97418,0}};auto const& GetObjectives()const{return objectives;}};
struct QuestStore {Quest q;Quest const* GetQuestTemplate(unsigned id){assert(id==39860);return &q;}} store;
auto sQuestDataStore=&store;
std::map<ObjectGuid,Creature*> world;std::vector<Creature*> allocated;unsigned serial=100;
struct Player:Unit {
 ObjectGuid guid;QuestStatusData quest;QuestStatus next=QUEST_STATUS_NONE;bool alive=true,teleport=false,flight=false,flying=false,visible=true,samePhase=true,failExit=false,failBoard=false,failSpawn=false,failBoss=false,los=true;unsigned zone=7503,map=1220,packets=0,phaseUpdates=0;Creature* vehicle=nullptr;std::map<unsigned,std::vector<ObjectGuid>> summons;std::vector<unsigned> casts;
 struct Event{uint64 delay;std::function<void()> action;};std::vector<Event> events;
 explicit Player(unsigned id):guid{id}{quest.Status=QUEST_STATUS_INCOMPLETE;x=5110.94f;y=5071.74f;z=627.359f;}
 Player* GetCharmerOrOwnerPlayerOrPlayerItself()const override{return const_cast<Player*>(this);}Player* ToPlayer()override{return this;}ObjectGuid GetGUID(){return guid;}bool IsAlive(){return alive;}unsigned GetMapId(){return map;}bool IsBeingTeleported(){return teleport;}bool isInFlight(){return flight;}bool IsFlying(){return flying;}unsigned GetZoneId(){return zone;}unsigned area=7807;unsigned GetAreaId(){return area;}bool canSeeOrDetect(Unit*){return visible;}QuestStatus GetQuestStatus(unsigned q){if(q==39381)return next;assert(q==39860);return quest.Status;}
 int GetQuestObjectiveData(unsigned q,unsigned entry){assert(q==39860);return quest.ObjectiveData[entry];}void SetQuestObjectiveData(Quest const*,QuestObjective const* objective,unsigned n){quest.ObjectiveData[objective->ObjectID]=n;}void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,unsigned n){assert(n==1);++packets;}bool CanCompleteQuest(unsigned){return quest.ObjectiveData[95259]&&quest.ObjectiveData[97418];}void CompleteQuest(unsigned){quest.Status=QUEST_STATUS_COMPLETE;}void IncompleteQuest(unsigned){quest.Status=QUEST_STATUS_INCOMPLETE;}
 QuestStatusData* getQuestStatus(unsigned){return &quest;}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);void AddDelayedEvent(uint64 d,std::function<void()> f){events.push_back({d,f});}
 void SetQuestStatus(unsigned id,QuestStatus status){assert(id==39860&&status==QUEST_STATUS_INCOMPLETE);quest.Status=status;++phaseUpdates;}
 void Run(unsigned max){auto work=std::move(events);events.clear();for(auto&e:work){if(e.delay<=max)e.action();else events.push_back(e);}}
 auto GetSummonList(unsigned entry){return &summons[entry];}bool InSamePhase(Unit*){return samePhase;}float GetDistance(Creature*);
 Vehicle* GetVehicle();Creature* GetVehicleBase(){return vehicle;}void ExitVehicle();void CastSpell(Unit*,unsigned,bool);bool IsWithinDistInMap(Unit* u,float d){return GetExactDist(u)<=d;}bool IsWithinLOSInMap(Unit*){return los;}
 Creature* SummonCreature(unsigned,float,float,float,float,unsigned,unsigned,ObjectGuid);
 void Abandon(){quest=QuestStatusData{};}void Accept(){quest=QuestStatusData{};quest.Status=QUEST_STATUS_INCOMPLETE;}
};
#include "QuestDelayedEvent.inc"
struct Creature:Unit {
 ObjectGuid guid;unsigned entry;Player* owner;ScriptedAI* ai=nullptr;Creature* guard=nullptr;unsigned flags=512;bool m_CanCallAssistance=true;double multiplier=1;uint64 health=100;unsigned combatStops=0,interrupts=0;bool phase=true,shown=true;void SetVisible(bool b){shown=b;}void setFaction(unsigned){}void CastSpell(Unit*,unsigned,bool);bool alive=true,gone=false,privateVisible=false,walk=false;unsigned delay=0,signals=0;Motion motion;Vehicle kit;
 Creature(unsigned e,Player*p):guid{++serial},entry(e),owner(p){world[guid]=this;if(p)p->summons[e].push_back(guid);}
 Player* GetCharmerOrOwnerPlayerOrPlayerItself()const override{return owner;}double GetHealthMultiplierForTarget(Unit*){return multiplier;}uint64 GetHealth(Unit*){return health;}void SetNoCallAssistance(bool){}void InterruptNonMeleeSpells(bool){++interrupts;}void CombatStop(bool){++combatStops;}void RemoveAllAuras(){}bool InSamePhase(Creature*){return phase;}Creature* FindNearestCreature(unsigned e,float range,bool living){assert(living);assert(e==97675);auto c=guard;return c&&c->alive&&!c->gone&&GetExactDist(c)<=range?c:nullptr;}Creature* ToCreature()override{return this;}TempSummon* ToTempSummon(){return owner?this:nullptr;}Unit* GetSummoner(){return owner;}ObjectGuid GetGUID(){return guid;}unsigned GetEntry(){return entry;}bool IsAlive(){return alive;}bool IsDespawn(){return gone||delay;}Vehicle* GetVehicleKit(){return entry==97781?&kit:nullptr;}
 void DespawnOrUnsummon(unsigned d=0){delay=d;if(!d){gone=true;if(owner&&owner->vehicle==this){owner->failExit=false;owner->ExitVehicle();}}}void Kill(Creature*c){assert(c==this);alive=false;}
 void RemoveFlag(unsigned,unsigned f){flags&=~f;}void SetFlag(unsigned,unsigned f){flags|=f;}void AddPlayerInPersonnalVisibilityList(ObjectGuid g){assert(owner&&owner->guid==g);privateVisible=true;}void SetReactState(unsigned){}void SetWalk(bool b){walk=b;}Motion* GetMotionMaster(){return &motion;}ScriptedAI* AI(){return ai;}
};
float Player::GetDistance(Creature*c){return GetExactDist(c);}Vehicle* Player::GetVehicle(){return vehicle?&vehicle->kit:nullptr;}void Player::ExitVehicle(){if(vehicle&&!failExit){vehicle->kit.passenger=nullptr;vehicle=nullptr;}}
struct ObjectAccessor {static Creature* GetCreature(Unit&,ObjectGuid g){auto i=world.find(g);return i==world.end()?nullptr:i->second;}};
struct ScriptedAI {Creature* me;explicit ScriptedAI(Creature*c):me(c){c->ai=this;}virtual ~ScriptedAI()=default;virtual void IsSummonedBy(Unit*){}virtual void MoveInLineOfSight(Unit*){}virtual void JustDied(Unit*){}virtual void MovementInform(uint32,uint32){}virtual void UpdateAI(uint32){}virtual void PassengerBoarded(Unit*,int8,bool){}virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(unsigned,unsigned){++me->signals;}std::vector<unsigned> talks;void Talk(unsigned id,ObjectGuid){talks.push_back(id);}};
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
struct GameObject:Unit {bool spawned=true;unsigned GetEntry(){return 95259;}ObjectGuid GetGUID(){return {50};}bool isSpawned(){return spawned;}};
struct GameObjectScript {explicit GameObjectScript(char const*){}virtual bool OnGossipHello(Player*,GameObject*){return false;}};
struct TextMgr {unsigned replies=0;ObjectGuid recipient;void SendChat(Creature*c,unsigned group,ObjectGuid p,unsigned type){assert(c->entry==95259&&!group&&type==CHAT_MSG_MONSTER_WHISPER);++replies;recipient=p;}} textMgr;
auto sCreatureTextMgr=&textMgr;
#include "HighmountainBloodRite.inc"
Creature* Player::SummonCreature(unsigned entry,float sx,float sy,float sz,float so,unsigned,unsigned duration,ObjectGuid visibleTo)
{
 assert((entry==97675||entry==97418)&&duration==600000&&visibleTo==guid);
 if((entry==97675&&failSpawn)||(entry==97418&&failBoss))return nullptr;
 auto c=new Creature(entry,this);allocated.push_back(c);c->x=sx;c->y=sy;c->z=sz;c->o=so;c->privateVisible=true;
 if(entry==97418){new npc_blood_rite_marakhan(c);c->AI()->IsSummonedBy(this);}return c;
}
void Player::CastSpell(Unit* target,unsigned spell,bool triggered)
{
 assert(target==this&&triggered&&(spell==190902||spell==192536));casts.push_back(spell);
 auto c=new Creature(spell==190902?97440:96286,this);allocated.push_back(c);static_cast<Position&>(*c)=*this;
}
void Creature::CastSpell(Unit* target,unsigned spell,bool triggered)
{
 assert(triggered);
 if(spell==192682){assert(target==this);return;}
 assert(spell==212758);auto c=target->ToCreature();assert(c&&c->entry==97440&&owner==c->owner);c->alive=false;
}
void Move(Unit&u,Position const&pos){static_cast<Position&>(u)=pos;}
void Fight(Creature* c)
{
 auto ai=static_cast<npc_blood_rite_marakhan*>(c->AI());
 ai->UpdateAI(1500);assert(c->shown&&!ai->fighting);
 ai->UpdateAI(2000);ai->UpdateAI(2500);ai->UpdateAI(3000);ai->UpdateAI(3000);assert(ai->fighting&&ai->attacks==1);
}
int main()
{
 Player p(1),q(2);Creature nav(95259,nullptr),marker(97675,nullptr);Move(nav,p);Move(marker,BloodRite::PortalPosition);nav.guard=&marker;
 npc_blood_rite_navarrogg navAI(&nav);player_blood_rite service;
 p.los=false;navAI.MoveInLineOfSight(&p);assert(p.casts.empty());p.los=true;p.samePhase=false;navAI.MoveInLineOfSight(&p);assert(p.casts.empty());p.samePhase=true;
 p.visible=false;navAI.MoveInLineOfSight(&p);assert(p.casts.empty());p.visible=true;
 nav.guard=nullptr;navAI.MoveInLineOfSight(&p);assert(p.casts.empty());nav.guard=&marker;
 p.failSpawn=true;navAI.MoveInLineOfSight(&p);assert(!p.packets);p.failSpawn=false;p.failBoss=true;navAI.MoveInLineOfSight(&p);assert(!p.packets&&!BloodRite::Existing(&p,97675));p.failBoss=false;
 navAI.MoveInLineOfSight(&p);auto c=BloodRite::Existing(&p,97418);auto portal=BloodRite::Existing(&p,97675);auto oakin=BloodRite::Existing(&p,97440);auto ai=static_cast<npc_blood_rite_marakhan*>(c->AI());
 assert(c&&portal&&oakin&&c->privateVisible&&!c->shown&&p.packets==1&&p.quest.ObjectiveData[95259]&&!p.quest.ObjectiveData[97418]);
 auto count=p.casts.size();navAI.MoveInLineOfSight(&p);assert(p.casts.size()==count&&p.packets==1);
 navAI.MoveInLineOfSight(&q);auto other=BloodRite::Existing(&q,97418);auto otherOakin=BloodRite::Existing(&q,97440);assert(other!=c&&q.packets==1);
 uint32 damage=1000000000;ai->DamageTaken(&p,damage,0);assert(!damage&&!ai->fighting);
 Fight(c);assert(!oakin->alive&&otherOakin->alive&&p.quest.Status==QUEST_STATUS_INCOMPLETE&&!p.quest.ObjectiveData[97418]);
 Creature pet(1,&p),foreignPet(1,&q);assert(ai->CanAIAttack(&p)&&ai->CanAIAttack(&pet)&&!ai->CanAIAttack(&q)&&!ai->CanAIAttack(&foreignPet));
 damage=100;ai->DamageTaken(&q,damage,0);assert(!damage);damage=100;ai->DamageTaken(&foreignPet,damage,0);assert(!damage);damage=100;ai->DamageTaken(&pet,damage,0);assert(damage==100);ai->UpdateAI(1);assert(ai->updates==1);
 // Actual KillRewarder credit runs 50 ms AFTER death, while the corpse still exists.
 c->alive=false;ai->JustDied(&pet);assert(!BloodRite::Existing(&p,96286));
 p.quest.ObjectiveData[97418]=1;p.quest.Status=QUEST_STATUS_COMPLETE;p.Run(100);assert(portal->gone&&BloodRite::Existing(&p,96286)&&!BloodRite::Existing(&q,96286)&&c->delay==30000);
 count=p.casts.size();service.OnLogin(&p);navAI.MoveInLineOfSight(&p);assert(p.casts.size()==count);
 BloodRite::ClearEntry(&p,96286);service.OnLogin(&p);assert(BloodRite::Existing(&p,96286));BloodRite::ClearEntry(&p,96286);p.quest.Status=QUEST_STATUS_REWARDED;service.OnLogin(&p);assert(BloodRite::Existing(&p,96286));BloodRite::ClearEntry(&p,96286);p.next=QUEST_STATUS_COMPLETE;service.OnLogin(&p);assert(BloodRite::Existing(&p,96286));BloodRite::ClearEntry(&p,96286);p.next=QUEST_STATUS_REWARDED;service.OnLogin(&p);assert(!BloodRite::Existing(&p,96286));
 auto old=other;auto oldAI=static_cast<npc_blood_rite_marakhan*>(old->AI());q.Abandon();q.Accept();oldAI->UpdateAI(1);assert(old->gone&&otherOakin->gone);navAI.MoveInLineOfSight(&q);auto retry=BloodRite::Existing(&q,97418);assert(retry&&retry!=old);
 q.alive=false;retry->AI()->UpdateAI(1);assert(retry->gone&&!BloodRite::Existing(&q,97675));q.alive=true;navAI.MoveInLineOfSight(&q);retry=BloodRite::Existing(&q,97418);assert(retry);q.x+=70;retry->AI()->UpdateAI(1);assert(retry->gone);Move(q,nav);
 navAI.MoveInLineOfSight(&q);retry=BloodRite::Existing(&q,97418);static_cast<npc_blood_rite_marakhan*>(retry->AI())->EnterEvadeMode();assert(retry->gone);navAI.MoveInLineOfSight(&q);service.OnLogout(&q);assert(!BloodRite::Existing(&q,97418)&&!BloodRite::Existing(&q,97675)&&!BloodRite::Existing(&q,97440));
 navAI.MoveInLineOfSight(&q);service.OnMapChanged(&q);assert(!BloodRite::Existing(&q,97418));
 for(auto actor:allocated){delete actor->ai;delete actor;}
 std::cout<<"PASS: extracted approach/scene handlers, no premature victory, isolated simultaneous scenes and sacrifice, owner pets, native turn-in recovery and failed summon/death/evade/relog/abandon/map retry.\n";
}
