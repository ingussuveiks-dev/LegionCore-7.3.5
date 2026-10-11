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
struct Unit:Position {virtual Player* GetCharmerOrOwnerPlayerOrPlayerItself()const{return nullptr;}Position GetPosition(){return *this;}void RemoveAurasDueToSpell(unsigned spell){assert(spell==190967);} float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}float GetOrientation(){return o;}float GetDistance(Unit* u){return GetExactDist(u);}virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}};
struct Vehicle {Unit* passenger=nullptr;Unit* GetPassenger(int seat){assert(!seat);return passenger;}};
struct Motion {unsigned follows=0,clears=0;void MoveFollow(Player*,float,float){++follows;}void Clear(){++clears;}unsigned point=0;Position destination;void MovePoint(unsigned id,Position const&p){point=id;destination=p;}};
struct QuestObjective {unsigned ObjectID; int Type=0;};
struct Quest {unsigned id=39381;unsigned GetQuestId()const{return id;}std::vector<QuestObjective> objectives{{95259,2},{97418,0}};auto const& GetObjectives()const{return objectives;}};
struct QuestStore {Quest q;Quest const* GetQuestTemplate(unsigned id){assert(id==39381);return &q;}} store;
auto sQuestDataStore=&store;
std::map<ObjectGuid,Creature*> world;std::vector<Creature*> allocated;unsigned serial=100;
struct Player:Unit {
 struct TalkClass {unsigned closes=0;void SendCloseGossip(){++closes;}} talk;TalkClass* PlayerTalkClass=&talk;unsigned teleports=0,credits=0;bool failTeleport=false,deferTeleport=false;Position requested;bool TeleportTo(unsigned mapId,Position const* p){assert(mapId==1220);++teleports;requested=*p;if(failTeleport)return false;if(deferTeleport){teleport=true;return true;}static_cast<Position&>(*this)=*p;area=7829;return true;}
 ObjectGuid guid;QuestStatusData quest;QuestStatus next=QUEST_STATUS_NONE;bool alive=true,teleport=false,flight=false,flying=false,visible=true,samePhase=true,failExit=false,failBoard=false,failSpawn=false,failBoss=false,los=true;unsigned zone=7503,map=1220,packets=0,phaseUpdates=0;Creature* vehicle=nullptr;std::map<unsigned,std::vector<ObjectGuid>> summons;std::vector<unsigned> casts;
 struct Event{uint64 delay;std::function<void()> action;};std::vector<Event> events;
 explicit Player(unsigned id):guid{id}{quest.Status=QUEST_STATUS_INCOMPLETE;x=5110.94f;y=5071.74f;z=627.359f;}
 Player* GetCharmerOrOwnerPlayerOrPlayerItself()const override{return const_cast<Player*>(this);}Player* ToPlayer()override{return this;}ObjectGuid GetGUID(){return guid;}bool IsAlive(){return alive;}unsigned GetMapId(){return map;}bool IsBeingTeleported(){return teleport;}bool isInFlight(){return flight;}bool IsFlying(){return flying;}unsigned GetZoneId(){return zone;}unsigned area=7807;unsigned GetAreaId(){return area;}bool canSeeOrDetect(Unit*){return visible;}QuestStatus GetQuestStatus(unsigned q){assert(q==39381);return quest.Status;}
 int GetQuestObjectiveData(unsigned q,unsigned entry){assert(q==39381);return quest.ObjectiveData[entry];}void SetQuestObjectiveData(Quest const*,QuestObjective const* objective,unsigned n){quest.ObjectiveData[objective->ObjectID]=n;}void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,unsigned n){assert(n==1);++packets;}bool CanCompleteQuest(unsigned){return quest.ObjectiveData[95259]&&quest.ObjectiveData[97418];}void CompleteQuest(unsigned){quest.Status=QUEST_STATUS_COMPLETE;}void IncompleteQuest(unsigned){quest.Status=QUEST_STATUS_INCOMPLETE;}
 QuestStatusData* getQuestStatus(unsigned){return &quest;}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);void AddDelayedEvent(uint64 d,std::function<void()> f){events.push_back({d,f});}
 void SetQuestStatus(unsigned id,QuestStatus status){assert(id==39381&&status==QUEST_STATUS_INCOMPLETE);quest.Status=status;++phaseUpdates;}
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
struct ScriptedAI {Creature* me;explicit ScriptedAI(Creature*c):me(c){c->ai=this;}virtual ~ScriptedAI()=default;virtual void IsSummonedBy(Unit*){}virtual void DoAction(int32){}virtual void MoveInLineOfSight(Unit*){}virtual void JustDied(Unit*){}virtual void MovementInform(uint32,uint32){}virtual void UpdateAI(uint32){}virtual void PassengerBoarded(Unit*,int8,bool){}virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(unsigned,unsigned){++me->signals;}std::vector<unsigned> talks;void Talk(unsigned id,ObjectGuid){talks.push_back(id);}};
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

struct SmartAI:ScriptedAI {
 unsigned selections=0,summonCalls=0,dataCalls=0;
 explicit SmartAI(Creature*c):ScriptedAI(c){}
 virtual void sGossipSelect(Player*,uint32,uint32){++selections;}
 void IsSummonedBy(Unit*)override{++summonCalls;}
 void SetData(uint32,uint32)override{++dataCalls;}
};
#define PrepareAuraScript(name) public:
#define AuraEffectPeriodicFn(method,index,type) 0
enum {EFFECT_2=2,SPELL_AURA_PERIODIC_TRIGGER_SPELL=23};
struct AuraEffect {};
struct Hook {void operator+=(int){}};
struct AuraScript {Unit* target=nullptr;bool prevented=false;Hook OnEffectPeriodic;virtual void Register(){};void PreventDefaultAction(){prevented=true;}Unit* GetTarget(){return target;}};
#include "HighmountainRockEscape.inc"
void Player::CastSpell(Unit* u,unsigned spell,bool triggered)
{
 assert(u==this&&spell==190967&&triggered);casts.push_back(spell);
 // Native DB2 effect 0 = personal kill credit 96286; retain that timing.
 if(quest.Status==QUEST_STATUS_INCOMPLETE){quest.Status=QUEST_STATUS_COMPLETE;++credits;}
}
void Move(Unit& u,Position const& p){static_cast<Position&>(u)=p;}
void FinishDeferred(Player& p){p.teleport=false;static_cast<Position&>(p)=p.requested;p.area=7829;p.Run(100);}
int main()
{
 Player p(1),q(2);Creature nav(96286,&p),foreign(96286,&q);Move(nav,p);Move(foreign,q);npc_rock_escape_navarrogg ai(&nav),other(&foreign);ai.IsSummonedBy(&p);assert(ai.summonCalls==1);
 Creature duplicate(96286,&p);npc_rock_escape_navarrogg dupAI(&duplicate);dupAI.IsSummonedBy(&p);assert(duplicate.gone&&!dupAI.summonCalls);
 ai.sGossipSelect(&p,999,0);assert(ai.selections==1);ai.sGossipSelect(&q,18673,0);assert(p.casts.empty()&&q.casts.empty());
 p.samePhase=false;ai.sGossipSelect(&p,18673,0);p.samePhase=true;p.los=false;ai.sGossipSelect(&p,18673,0);p.los=true;p.visible=false;ai.sGossipSelect(&p,18673,0);p.visible=true;p.area=7829;ai.sGossipSelect(&p,18673,0);p.area=7807;assert(p.casts.empty());
 p.deferTeleport=true;ai.sGossipSelect(&p,18673,0);assert(ai.pending&&p.credits==1&&p.teleports==0&&p.talk.closes==1);ai.sGossipSelect(&p,18673,0);assert(p.casts.size()==1);
 other.sGossipSelect(&q,18673,0);assert(q.casts.size()==1&&q.credits==1);
 p.Run(2999);assert(!p.teleports&&!nav.gone);p.Run(3000);assert(p.teleports==1&&p.teleport&&!nav.gone&&!foreign.gone);p.Run(100);assert(!nav.gone);FinishDeferred(p);assert(nav.gone&&!foreign.gone);
 q.failTeleport=true;q.Run(3000);assert(!other.pending&&!foreign.gone&&q.quest.Status==QUEST_STATUS_COMPLETE);other.sGossipSelect(&q,18673,0);assert(other.pending&&q.casts.size()==2&&q.credits==1);q.failTeleport=false;q.Run(3000);assert(foreign.gone&&q.area==7829);
 Player r(3);Creature retry(96286,&r);Move(retry,r);npc_rock_escape_navarrogg retryAI(&retry);
 retryAI.sGossipSelect(&r,18673,0);r.Abandon();r.Accept();r.Run(3000);assert(!r.teleports);retryAI.sGossipSelect(&r,18673,0);assert(r.casts.size()==2);r.alive=false;r.Run(3000);assert(!r.teleports&&!retryAI.pending&&!retry.gone);r.alive=true;
 retryAI.sGossipSelect(&r,18673,0);r.x+=30;r.Run(3000);assert(!r.teleports&&!retryAI.pending);Move(r,retry);
 retryAI.sGossipSelect(&r,18673,0);r.map=1;r.Run(3000);assert(!r.teleports&&!retryAI.pending);r.map=1220;
 // A true return from TeleportTo is not proof of arriving: timeout preserves retry.
 r.deferTeleport=true;retryAI.sGossipSelect(&r,18673,0);r.Run(3000);for(unsigned i=0;i<51;++i)r.Run(100);assert(!retryAI.pending&&!retry.gone);r.teleport=false;r.deferTeleport=false;
 retryAI.sGossipSelect(&r,18673,0);r.Run(3000);assert(retry.gone);
 Player absent(4);Creature unused(96286,&absent);Move(unused,absent);npc_rock_escape_navarrogg unusedAI(&unused);absent.quest.Status=QUEST_STATUS_NONE;unusedAI.sGossipSelect(&absent,18673,0);absent.quest.Status=QUEST_STATUS_REWARDED;unusedAI.sGossipSelect(&absent,18673,0);assert(absent.casts.empty());
 // Native tick replaces absent 190982 only for an authorized, pending interaction.
 Player a(5);Creature guide(96286,&a);Move(guide,a);npc_rock_escape_navarrogg guideAI(&guide);a.deferTeleport=true;
 aura_rock_escape_transport tick;tick.target=&a;tick.Tick(nullptr);assert(tick.prevented&&!a.teleports);
 guideAI.sGossipSelect(&a,18673,0);tick.Tick(nullptr);assert(a.teleports==1&&guideAI.departed&&!guide.gone);tick.Tick(nullptr);a.Run(3000);assert(a.teleports==1);FinishDeferred(a);assert(guide.gone);
 // Failed native dispatch followed by retry must invalidate the old fallback timer.
 Player b(6);Creature retryGuide(96286,&b);Move(retryGuide,b);npc_rock_escape_navarrogg bAI(&retryGuide);tick.target=&b;b.failTeleport=true;bAI.sGossipSelect(&b,18673,0);tick.Tick(nullptr);assert(b.teleports==1&&!bAI.pending);
 b.failTeleport=false;bAI.sGossipSelect(&b,18673,0);b.Run(3000);assert(b.teleports==2&&retryGuide.gone);
 Player abandoned(7);Creature abandonedGuide(96286,&abandoned);Move(abandonedGuide,abandoned);npc_rock_escape_navarrogg abandonedAI(&abandonedGuide);abandonedAI.sGossipSelect(&abandoned,18673,0);abandoned.Abandon();abandoned.Accept();tick.target=&abandoned;tick.Tick(nullptr);abandoned.Run(3000);assert(!abandoned.teleports);
 std::cout<<"PASS: actual native periodic handler (default missing-spell cast suppressed, authorization, single dispatch and scoped fallback) plus escape handler, native credit timing, owner/phase/area/LOS gating, duplicate guide/click prevention, two-player isolation, delayed acknowledgement, failed/timeout transport retry and quest-lifetime cancellation.\n";
}
