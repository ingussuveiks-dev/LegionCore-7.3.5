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
enum {QUEST_OBJECTIVE_MONSTER=0,UNIT_FIELD_NPC_FLAGS=1,UNIT_NPC_FLAG_QUESTGIVER=2,CHAT_MSG_MONSTER_WHISPER=15,REACT_PASSIVE=0,POINT_MOTION_TYPE=1,TEMPSUMMON_TIMED_DESPAWN=1,EFFECT_0=0,SPELL_EFFECT_DAMAGE_FROM_MAX_HEALTH_PCT=165};
using SpellEffIndex=int;using SpellCastResult=int;enum{SPELL_CAST_OK=0,SPELL_FAILED_DONT_REPORT=1};
#include "QuestStatusData.inc"
struct ObjectGuid {unsigned value=0;void Clear(){value=0;}bool operator<(ObjectGuid const&r)const{return value<r.value;}bool operator==(ObjectGuid const&r)const{return value==r.value;}};
struct Position {float x=0,y=0,z=0,o=0;float GetExactDist(Position const*p)const{return std::sqrt((x-p->x)*(x-p->x)+(y-p->y)*(y-p->y)+(z-p->z)*(z-p->z));}};
struct Player;struct Creature;struct ScriptedAI;using TempSummon=Creature;
struct Unit:Position {float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}float GetOrientation(){return o;}float GetDistance(Unit* u){return GetExactDist(u);}virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}};
struct Vehicle {Unit* passenger=nullptr;Unit* GetPassenger(int seat){assert(!seat);return passenger;}};
struct Motion {unsigned follows=0,clears=0;void MoveFollow(Player*,float,float){++follows;}void Clear(){++clears;}unsigned point=0;Position destination;void MovePoint(unsigned id,Position const&p){point=id;destination=p;}};
struct QuestObjective {unsigned ObjectID; int Type=0;};
struct Quest {unsigned id=39374;unsigned GetQuestId()const{return id;}std::vector<QuestObjective> objectives{{95256}};auto const& GetObjectives()const{return objectives;}};
struct QuestStore {Quest q;Quest const* GetQuestTemplate(unsigned id){assert(id==39374);return &q;}} store;
auto sQuestDataStore=&store;
std::map<ObjectGuid,Creature*> world;std::vector<Creature*> allocated;unsigned serial=100;
struct Player:Unit {
 ObjectGuid guid;QuestStatusData quest;bool alive=true,teleport=false,flight=false,flying=false,visible=true,samePhase=true,failExit=false,failBoard=false,failSpawn=false;unsigned zone=7503,map=1220,packets=0,phaseUpdates=0;Creature* vehicle=nullptr;std::map<unsigned,std::vector<ObjectGuid>> summons;std::vector<unsigned> casts;
 struct Event{uint64 delay;std::function<void()> action;};std::vector<Event> events;
 explicit Player(unsigned id):guid{id}{quest.Status=QUEST_STATUS_INCOMPLETE;x=5221.07f;y=4902.25f;z=711.837f;}
 Player* ToPlayer()override{return this;}ObjectGuid GetGUID(){return guid;}bool IsAlive(){return alive;}unsigned GetMapId(){return map;}bool IsBeingTeleported(){return teleport;}bool isInFlight(){return flight;}bool IsFlying(){return flying;}unsigned GetZoneId(){return zone;}bool canSeeOrDetect(Creature*){return visible;}QuestStatus GetQuestStatus(unsigned q){assert(q==39374);return quest.Status;}
 int GetQuestObjectiveData(unsigned q,unsigned entry){assert(q==39374);return quest.ObjectiveData[entry];}void SetQuestObjectiveData(Quest const*,QuestObjective const* objective,unsigned n){quest.ObjectiveData[objective->ObjectID]=n;}void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,unsigned n){assert(n==1);++packets;}bool CanCompleteQuest(unsigned){return quest.ObjectiveData[95256];}void CompleteQuest(unsigned){quest.Status=QUEST_STATUS_COMPLETE;}void IncompleteQuest(unsigned){quest.Status=QUEST_STATUS_INCOMPLETE;}
 QuestStatusData* getQuestStatus(unsigned){return &quest;}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);void AddDelayedEvent(uint64 d,std::function<void()> f){events.push_back({d,f});}
 void SetQuestStatus(unsigned id,QuestStatus status){assert(id==39374&&status==QUEST_STATUS_INCOMPLETE);quest.Status=status;++phaseUpdates;}
 void Run(unsigned max){auto work=std::move(events);events.clear();for(auto&e:work){if(e.delay<=max)e.action();else events.push_back(e);}}
 auto GetSummonList(unsigned entry){return &summons[entry];}bool InSamePhase(Creature*){return samePhase;}float GetDistance(Creature*);
 Vehicle* GetVehicle();Creature* GetVehicleBase(){return vehicle;}void ExitVehicle();void CastSpell(Player*,unsigned,bool);void RemoveAurasDueToSpell(unsigned);
 Creature* SummonCreature(unsigned,float,float,float,float,unsigned,unsigned,ObjectGuid);
 void Abandon(){quest=QuestStatusData{};}void Accept(){quest=QuestStatusData{};quest.Status=QUEST_STATUS_INCOMPLETE;}
};
#include "QuestDelayedEvent.inc"
struct Creature:Unit {
 ObjectGuid guid;unsigned entry;Player* owner;ScriptedAI* ai=nullptr;Creature* guard=nullptr;unsigned flags=3;bool phase=true;bool alive=true,gone=false,privateVisible=false,walk=false;unsigned delay=0,signals=0;Motion motion;Vehicle kit;
 Creature(unsigned e,Player*p):guid{++serial},entry(e),owner(p){world[guid]=this;if(p)p->summons[e].push_back(guid);}
 bool InSamePhase(Creature*){return phase;}Creature* FindNearestCreature(unsigned e,float range,bool living){assert(living);assert(e==98904);auto c=guard;return c&&c->alive&&!c->gone&&GetExactDist(c)<=range?c:nullptr;}Creature* ToCreature()override{return this;}TempSummon* ToTempSummon(){return owner?this:nullptr;}Unit* GetSummoner(){return owner;}ObjectGuid GetGUID(){return guid;}unsigned GetEntry(){return entry;}bool IsAlive(){return alive;}Vehicle* GetVehicleKit(){return entry==97781?&kit:nullptr;}
 void DespawnOrUnsummon(unsigned d=0){delay=d;if(!d){gone=true;if(owner&&owner->vehicle==this){owner->failExit=false;owner->ExitVehicle();}}}void Kill(Creature*c){assert(c==this);alive=false;}
 void RemoveFlag(unsigned,unsigned f){flags&=~f;}void SetFlag(unsigned,unsigned f){flags|=f;}void AddPlayerInPersonnalVisibilityList(ObjectGuid g){assert(owner&&owner->guid==g);privateVisible=true;}void SetReactState(unsigned){}void SetWalk(bool b){walk=b;}Motion* GetMotionMaster(){return &motion;}ScriptedAI* AI(){return ai;}
};
float Player::GetDistance(Creature*c){return GetExactDist(c);}Vehicle* Player::GetVehicle(){return vehicle?&vehicle->kit:nullptr;}void Player::ExitVehicle(){if(vehicle&&!failExit){vehicle->kit.passenger=nullptr;vehicle=nullptr;}}
struct ObjectAccessor {static Creature* GetCreature(Unit&,ObjectGuid g){auto i=world.find(g);return i==world.end()||i->second->gone?nullptr:i->second;}};
struct ScriptedAI {Creature* me;explicit ScriptedAI(Creature*c):me(c){c->ai=this;}virtual ~ScriptedAI()=default;virtual void IsSummonedBy(Unit*){}virtual void MovementInform(uint32,uint32){}virtual void UpdateAI(uint32){}virtual void PassengerBoarded(Unit*,int8,bool){}virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(unsigned,unsigned){++me->signals;}std::vector<unsigned> talks;void Talk(unsigned id,ObjectGuid){talks.push_back(id);}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual ~PlayerScript()=default;virtual void OnLogin(Player*){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}virtual void OnUpdate(Player*,uint32){}};

struct SmartAI:ScriptedAI {
 unsigned baseSummoned=0,baseUpdates=0,baseMovement=0,baseAccepted=0;
 explicit SmartAI(Creature*c):ScriptedAI(c){}
 void IsSummonedBy(Unit*)override{++baseSummoned;}
 void UpdateAI(uint32)override{++baseUpdates;}
 void MovementInform(uint32,uint32)override{++baseMovement;}
 virtual void sQuestAccept(Player*,Quest const*){++baseAccepted;}
 virtual void sQuestReward(Player*,Quest const*,uint32){}
};
struct TextMgr {unsigned replies=0;ObjectGuid recipient;void SendChat(Creature*c,unsigned group,ObjectGuid p,unsigned type){assert(c->entry==98904 && !group && type==CHAT_MSG_MONSTER_WHISPER);++replies;recipient=p;}} textMgr;
auto sCreatureTextMgr=&textMgr;
#include "HighmountainAudience.inc"
Creature* Player::SummonCreature(unsigned entry,float sx,float sy,float sz,float so,unsigned type,unsigned duration,ObjectGuid visibleTo)
{
 assert(entry==95256&&type==TEMPSUMMON_TIMED_DESPAWN&&duration==600000&&visibleTo==guid);
 if(failSpawn)return nullptr;
 auto c=new Creature(entry,this);allocated.push_back(c);c->x=sx;c->y=sy;c->z=sz;c->o=so;
 new npc_audience_oakin(c);c->AI()->IsSummonedBy(this);return c;
}
void Move(Unit&u,Position const&pos){static_cast<Position&>(u)=pos;}
int main()
{
 player_audience_oakin service;Player p(1),q(2);Creature guard(98904,nullptr);Move(guard,{5338.3f,4922.95f,692.054f,0});
 service.OnUpdate(&p,1000);auto c=Audience::Existing(&p);auto ai=static_cast<npc_audience_oakin*>(c->AI());c->guard=&guard;
 assert(c->privateVisible&&c->walk&&!ai->baseSummoned&&ai->talks==std::vector<unsigned>({0})&&!(c->flags&UNIT_NPC_FLAG_QUESTGIVER));
 service.OnUpdate(&p,1000);assert(p.summons[95256].size()==1);
 ai->UpdateAI(1);assert(ai->moving&&!p.packets);ai->MovementInform(POINT_MOTION_TYPE,1);assert(!ai->arrived); // wrong physical position
 Move(*c,Audience::Destination);Move(p,*c);Move(q,*c);ai->MovementInform(POINT_MOTION_TYPE,2);ai->UpdateAI(1);assert(!p.packets&&!ai->arrived);
 ai->MovementInform(POINT_MOTION_TYPE,1);assert(ai->arrived);ai->UpdateAI(1);assert(ai->dialogue==5000&&!p.packets);
 ai->UpdateAI(4999);assert(!p.packets);p.y+=20;ai->UpdateAI(1);assert(!p.packets&&!ai->dialogue);
 Move(p,*c);ai->UpdateAI(1);p.visible=false;ai->UpdateAI(5000);assert(!p.packets&&!ai->dialogue);p.visible=true;
 ai->UpdateAI(1);guard.alive=false;ai->UpdateAI(5000);assert(!p.packets);guard.alive=true;
 ai->UpdateAI(1);ai->UpdateAI(5000);assert(p.packets==1&&!q.packets&&p.quest.Status==QUEST_STATUS_COMPLETE);
 assert(textMgr.replies==1&&textMgr.recipient==p.guid&&(c->flags&UNIT_NPC_FLAG_QUESTGIVER));ai->UpdateAI(5000);assert(p.packets==1&&textMgr.replies==1);
 // Turn-in actor remains available, but cave acceptance hands it back to the
 // pre-existing SmartAI timeline and audience cleanup must leave it alone.
 Quest cave;cave.id=39455;ai->sQuestReward(&p,&store.q,0);assert(!ai->audience&&c->delay==60000);p.quest.Status=QUEST_STATUS_REWARDED;service.OnUpdate(&p,1000);assert(!c->gone);ai->sQuestAccept(&p,&cave);assert(!ai->audience&&ai->baseAccepted==1);p.quest.Status=QUEST_STATUS_REWARDED;service.OnUpdate(&p,1000);assert(!c->gone);ai->UpdateAI(1);assert(ai->baseUpdates==1);
 // Separate owner at the same endpoint must still run their own arrival/dialogue.
 service.OnUpdate(&q,1000);auto other=Audience::Existing(&q);other->guard=&guard;auto otherAi=static_cast<npc_audience_oakin*>(other->AI());otherAi->UpdateAI(1);assert(!q.packets);
 otherAi->MovementInform(POINT_MOTION_TYPE,1);otherAi->UpdateAI(1);service.OnLogout(&q);assert(other->gone&&!q.packets);
 service.OnUpdate(&q,1000);other=Audience::Existing(&q);assert(other&&!q.packets);other->guard=&guard;otherAi=static_cast<npc_audience_oakin*>(other->AI());otherAi->UpdateAI(1);otherAi->MovementInform(POINT_MOTION_TYPE,1);otherAi->UpdateAI(1);otherAi->UpdateAI(5000);assert(q.packets==1);
 // Complete-save recovery does not repeat credit/dialogue or reset progress.
 service.OnLogout(&q);service.OnUpdate(&q,1000);other=Audience::Existing(&q);other->AI()->UpdateAI(5000);assert(q.packets==1&&(other->flags&UNIT_NPC_FLAG_QUESTGIVER));
 Player r(3);service.OnUpdate(&r,1000);auto old=Audience::Existing(&r);r.Abandon();r.Accept();old->AI()->UpdateAI(1);assert(old->gone);service.OnUpdate(&r,1000);assert(Audience::Existing(&r)&&Audience::Existing(&r)!=old&&!r.packets);
 auto fresh=Audience::Existing(&r);auto freshAi=static_cast<npc_audience_oakin*>(fresh->AI());freshAi->UpdateAI(1);fresh->x+=35;freshAi->UpdateAI(1);assert(!freshAi->moving&&fresh->motion.clears==1);Move(r,*fresh);freshAi->UpdateAI(1);assert(freshAi->moving);freshAi->UpdateAI(120000);assert(fresh->gone&&!r.packets);
 service.OnUpdate(&r,1000);assert(Audience::Existing(&r));r.alive=false;service.OnUpdate(&r,1000);assert(!Audience::Existing(&r));r.alive=true;r.failSpawn=true;service.OnUpdate(&r,1000);assert(!Audience::Existing(&r));r.failSpawn=false;service.OnUpdate(&r,1000);assert(Audience::Existing(&r));r.map=1;service.OnMapChanged(&r);assert(!Audience::Existing(&r));r.map=1220;r.flying=true;service.OnUpdate(&r,1000);assert(!Audience::Existing(&r));r.flying=false;service.OnUpdate(&r,1000);assert(Audience::Existing(&r));
 // Static Oakin and cave summons retain normal SmartAI behavior.
 Creature staticOakin(95256,nullptr),caveOakin(95256,nullptr);npc_audience_oakin staticAi(&staticOakin),caveAi(&caveOakin);staticAi.UpdateAI(1);staticAi.sQuestAccept(&p,&cave);caveAi.IsSummonedBy(&staticOakin);caveAi.MovementInform(1,6);assert(staticAi.baseUpdates==1&&staticAi.baseAccepted==1&&caveAi.baseSummoned==1&&caveAi.baseMovement==1);
 r.samePhase=false;auto phased=Audience::Existing(&r);phased->AI()->UpdateAI(1);assert(phased->gone&&!r.packets);r.samePhase=true;service.OnLogout(&q);service.OnLogout(&r);for(auto actor:allocated){delete actor->ai;delete actor;}
 std::cout<<"PASS: production audience arrival/dialogue, owner-only credit, missing guard/phase/distance interruption, duplicate prevention, quest lifetime, relog/death/map/flight/path failure recovery, turn-in actor and cave SmartAI handoff.\n";
}
