#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using uint8=uint8_t;using int8=int8_t;using uint32=uint32_t;using uint64=uint64_t;
#include "FunctionProcessor.inc"
constexpr uint32 CLASS_HUNTER=3,QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,SMART_EVENT_DATA_SET=38,SMART_EVENT_ON_SPELLCLICK=73;
constexpr uint32 UNIT_FIELD_FLAGS=0,UNIT_FIELD_NPC_FLAGS=1,UNIT_FIELD_CREATED_BY_SPELL=2,UNIT_FLAG_NON_ATTACKABLE=2,UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_IMMUNE_TO_NPC=512,UNIT_NPC_FLAG_SPELLCLICK=16777216,REACT_PASSIVE=0;
constexpr float INTERACTION_DISTANCE=5;
constexpr uint32 EFFECT_0=0,SPELL_EFFECT_TELEPORT=227,VEHICLE_SPELL_RIDE_HARDCODED=46598,SPELLVALUE_BASE_POINT0=0;
using SpellEffIndex=uint32;enum SpellCastResult{SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_NOT_READY};
struct ObjectGuid{uint64 high=0,low=0;bool IsEmpty()const{return !high&&!low;}bool operator==(ObjectGuid const& b)const{return high==b.high&&low==b.low;}bool operator!=(ObjectGuid const& b)const{return !(*this==b);}bool operator<(ObjectGuid const& b)const{return high<b.high||(high==b.high&&low<b.low);}};
using GuidList=std::list<ObjectGuid>;
struct Map{uint32 id=1220;} firstMap,otherMap{1511};
struct WorldLocation{uint32 map=1511;float x=3364.77f,y=1919.54f,z=.092994f;uint32 GetMapId()const{return map;}float GetExactDist(float a,float b,float c)const{return std::sqrt((x-a)*(x-a)+(y-b)*(y-b)+(z-c)*(z-c));}};
struct QuestStatusData{uint32 Status=3;std::shared_ptr<uint8> ScriptLifetime;};
struct Spline{bool done=true;bool Finalized()const{return done;}};
struct Player;struct Creature;struct Vehicle;struct CreatureAI;
struct Unit;
std::map<ObjectGuid,Unit*> registry;uint64 nextGuid=1;unsigned warnings=0;
#define TC_LOG_WARN(...) (++warnings)
#define TC_LOG_DEBUG(...) ((void)0)
struct Unit{ObjectGuid guid;Map* map=&firstMap;bool world=true,alive=true,phase=true;FunctionProcessor functions;
 Unit(uint64 type=2):guid{type,nextGuid++}{registry[guid]=this;}virtual ~Unit(){registry.erase(guid);}virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}Unit* ToUnit(){return this;}
 ObjectGuid GetGUID()const{return guid;}uint32 GetGUIDLow()const{return uint32(guid.low);}Map* GetMap()const{return map;}uint32 GetMapId()const{return map->id;}bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}bool InSamePhase(Unit* u)const{return phase&&u&&u->phase;}virtual Creature* GetVehicleBase()const{return nullptr;}virtual Vehicle* GetVehicleKit(){return nullptr;}virtual void ExitVehicle(){};
 virtual Player* GetCharmerOrOwnerPlayerOrPlayerItself()const;virtual bool HasQuestObjectScope()const{return false;}virtual bool CanTargetQuestObject(Unit*)const{return true;}
 bool IsWithinDistInMap(Unit* u,float dist)const;virtual void CastSpell(Unit*,uint32,bool);
 void AddDelayedEvent(uint64 delay,std::function<void()>&& f){functions.AddDelayedEvent(delay,std::move(f));}
};using WorldObject=Unit;
struct ObjectAccessor{static Unit* GetUnit(Unit const& base,ObjectGuid id){auto i=registry.find(id);return i!=registry.end()&&i->second->world&&i->second->map==base.map?i->second:nullptr;}static WorldObject* GetWorldObject(Unit const& base,ObjectGuid id){return GetUnit(base,id);}static Player* GetPlayer(Unit const& base,ObjectGuid id);static Creature* GetCreature(Unit const& base,ObjectGuid id);static Player* FindPlayer(ObjectGuid id);};
struct Vehicle{struct Seat{struct{ObjectGuid Guid;}Passenger;};std::map<int8,Seat> Seats{{0,{}}};Unit* passenger=nullptr;bool seat=true;bool HasEmptySeat(int8 id)const{return id==0&&seat&&!passenger;}Unit* GetPassenger(int8 id)const{return id==0?passenger:nullptr;}};
struct Player:Unit{QuestStatusData quest;uint32 klass=CLASS_HUNTER,credit=0,creditCalls=0,joins=0,exits=0,restores=0,casts=0;bool contact=true,pending=false,rewarded=false,waitOnSeat=false,inFlight=false,transport=false,visible=true,los=true,joinAccepted=true;float sourceDistance=1;WorldLocation destination;Spline spline;Spline* movespline=&spline;Vehicle* vehicle=nullptr;Creature* base=nullptr;Unit* moved=this;Unit* viewpoint=nullptr;ObjectGuid primaryPet{2,99999};std::map<uint32,GuidList> summons;std::vector<Player*> group;
 Player():Unit(1){}Player* ToPlayer()override{return this;}bool CanContact(){return contact&&world;}uint32 getClass()const{return klass;}uint32 GetQuestStatus(uint32 q)const{assert(q==40385);return quest.Status;}bool GetQuestRewardStatus(uint32 q)const{assert(q==40385);return rewarded;}QuestStatusData* getQuestStatus(uint32 q){assert(q==40385);return &quest;}
 bool HasPendingTeleport()const{return pending;}Vehicle* GetVehicle()const{return vehicle;}Creature* GetVehicleBase()const{return base;}bool GetTransport()const{return transport;}bool isInFlight()const{return inFlight;}
 bool canSeeOrDetect(Unit*)const{return visible;}bool IsWithinDistInMap(Unit* u,float d)const{return map==u->map&&sourceDistance<=d;}bool IsWithinLOSInMap(Unit*)const{return los;}float GetDistance(float,float,float)const{return sourceDistance;}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);void BindQuestObject(uint32,Creature*);GuidList* GetSummonList(uint32 entry){auto i=summons.find(entry);return i==summons.end()?nullptr:&i->second;}
 void EnterVehicle(Unit*,int8,bool);void CastCustomSpell(uint32,uint32,int,Unit*,bool);void ExitVehicle()override;
 Unit* GetUnitBeingMoved()const{return moved;}Unit* GetViewpoint()const{return viewpoint;}void SetClientControl(Unit* u,bool on){assert(u==this&&on);moved=this;++restores;}void SetViewpoint(Unit* u,bool apply){assert(u==viewpoint&&!apply);viewpoint=nullptr;}
 WorldLocation& GetTeleportDest(){return destination;}uint32 GetQuestObjectiveData(uint32 q,uint32 c)const{assert(q==40385&&c==100111);return credit;}
 void RewardPlayerAndGroupAtEvent(uint32 c,Unit*){assert(c==100111);++creditCalls;if(quest.Status==3&&!rewarded)credit=1;for(auto p:group)if(p->quest.Status==3&&!p->rewarded){++p->creditCalls;p->credit=1;}}
};
struct Creature:Unit{uint32 entry=100965,db=366115,flags=0,npcFlags=UNIT_NPC_FLAG_SPELLCLICK,despawn=0;ObjectGuid summoner;CreatureAI* ai=nullptr;Vehicle kit;bool hasKit=true,expired=false,bound=false,receiptAllows=true;std::weak_ptr<uint8> lease;
 Creature(uint32 e=100965,uint32 d=366115):entry(e),db(d){}Creature* ToCreature()override{return this;}Creature* ToTempSummon(){return summoner.IsEmpty()?nullptr:this;}ObjectGuid GetSummonerGUID()const{return summoner;}uint32 GetEntry()const{return entry;}uint32 GetDBTableGUIDLow()const{return db;}uint32 GetUInt32Value(uint32 field)const{assert(field==2);return entry==100967?198941:0;}CreatureAI* AI()const{return ai;}Vehicle* GetVehicleKit()override{return hasKit?&kit:nullptr;}bool IsQuestObjectExpired()const{return expired||(bound&&lease.expired());}bool HasFlag(uint32 field,uint32 bit)const{return ((field==UNIT_FIELD_FLAGS?flags:npcFlags)&bit)!=0;}void SetFlag(uint32,uint32 mask){flags|=mask;}void SetReactState(uint32){}void setActive(bool){}void DespawnOrUnsummon(uint32 ms=0){despawn=ms?ms:1;}Player* GetLootRecipient(){return nullptr;}
};
Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself()const{return const_cast<Unit*>(this)->ToPlayer();}
bool Unit::IsWithinDistInMap(Unit* u,float d)const{if(auto p=const_cast<Unit*>(this)->ToPlayer())return p->IsWithinDistInMap(u,d);return map==u->map;}
Player* ObjectAccessor::GetPlayer(Unit const& b,ObjectGuid id){auto u=GetUnit(b,id);return u?u->ToPlayer():nullptr;}Creature* ObjectAccessor::GetCreature(Unit const& b,ObjectGuid id){auto u=GetUnit(b,id);return u?u->ToCreature():nullptr;}Player* ObjectAccessor::FindPlayer(ObjectGuid id){auto i=registry.find(id);return i==registry.end()?nullptr:i->second->ToPlayer();}
#include "QuestDelay.inc"
void Player::BindQuestObject(uint32 q,Creature* c){assert(q==40385);c->bound=true;c->lease=quest.ScriptLifetime;}
#include "EnterVehicle.inc"
struct SpellTargetPosition{uint32 target_mapId=1511;float target_X=3364.77f,target_Y=1919.54f,target_Z=.092994f;};struct SpellMgr{SpellTargetPosition destination;bool present=true;SpellTargetPosition const* GetSpellTargetPosition(uint32 id){assert(id==198950);return present?&destination:nullptr;}} manager;auto sSpellMgr=&manager;
using ObjectList=std::vector<Unit*>;bool IsUnit(Unit* u){return u!=nullptr;}
#include "CreditPlayer.inc"
struct Row{uint32 id=0,event=0,action=0,link=0,param=0,second=0,target=0,dist=0,p1=0,p2=0;};
struct SmartScript{Creature* me;std::vector<Row> rows;uint32 nativeClicks=0,dataCalls=0;explicit SmartScript(Creature* c):me(c){}Creature* GetBaseObject(){return me;}ObjectList* GetTargets(Row const&,Unit* u){auto list=new ObjectList;if(u)list->push_back(u);return list;}void ProcessEventsFor(uint32,Unit*,uint32=0,uint32=0);};
struct CreatureAI{Creature* me;explicit CreatureAI(Creature* c):me(c){}virtual ~CreatureAI()=default;virtual void UpdateAI(uint32){}virtual void PassengerBoarded(Unit*,int8,bool){}virtual void JustDied(Unit*){}virtual void OnSpellClick(Unit*){}};
struct ScriptedAI:CreatureAI{using CreatureAI::CreatureAI;};
struct SmartAI:CreatureAI{SmartScript script;explicit SmartAI(Creature* c):CreatureAI(c),script(c){}SmartScript* GetScript(){return &script;}void OnSpellClick(Unit*)override;};
#include "SmartClick.inc"
struct Hook{template<class T>void operator+=(T){}};struct SpellInfo{uint32 Id=0;};
struct SpellScript{Unit* caster=nullptr;ObjectGuid target,hit;SpellInfo info;bool prevented=false;Hook OnCheckCast,AfterCast,OnEffectHitTarget,AfterHit;virtual ~SpellScript()=default;virtual bool Load(){return true;}virtual void Register(){}Unit* GetCaster(){return caster;}Unit* GetExplTargetUnit(){return caster?ObjectAccessor::GetUnit(*caster,target):nullptr;}Unit* GetHitUnit(){return caster?ObjectAccessor::GetUnit(*caster,hit):nullptr;}SpellInfo const* GetSpellInfo(){return &info;}void PreventHitDefaultEffect(uint32){prevented=true;}};
#define PrepareSpellScript(x) public:
#define SpellCheckCastFn(x) 0
#define SpellCastFn(x) 0
#define SpellEffectFn(...) 0
#define SpellHitFn(x) 0
#include "TalonclawBoardedDeparture.inc"
std::function<void(Unit*,Unit*,uint32,bool)> castEndpoint;
void Unit::CastSpell(Unit* target,uint32 spell,bool triggered){castEndpoint(this,target,spell,triggered);}
void Player::CastCustomSpell(uint32 spell,uint32 mod,int seat,Unit* target,bool triggered){assert(spell==46598&&mod==0&&seat==1&&triggered);assert(target);++joins;if(joinAccepted)waitOnSeat=true;}
void Player::ExitVehicle(){++exits;if(!base)return;auto old=base;old->kit.passenger=nullptr;old->kit.Seats[0].Passenger.Guid={};old->AI()->PassengerBoarded(this,0,false);base=nullptr;vehicle=nullptr;spline.done=false;}
void SmartScript::ProcessEventsFor(uint32 kind,Unit* unit,uint32 var0,uint32 var1){
 if(kind==73)++nativeClicks;if(kind==38)++dataCalls;
 for(auto const& row:rows){if(row.event!=kind)continue;if(kind==38&&(row.p1!=var0||row.p2!=var1))continue;
  Player* p=unit?unit->ToPlayer():nullptr;
  if(kind==38&&(!me->receiptAllows||!p||p->GetQuestStatus(40385)!=3||p->klass!=CLASS_HUNTER||p->map!=me->map||!p->alive))continue;
  if(row.action==33){struct Action{struct{uint32 creature;}killedMonster;}action{{row.param}};struct Entry:Row{Action action;};Entry e;static_cast<Row&>(e)=row;e.action=action;bool trigger=false;
   switch(row.action){
#include "CreditAction.inc"
   }
  }else if(row.action==85){Unit* tempLastInvoker=unit;ObjectGuid targetGUID=unit->GetGUID(),castSource=me->GetGUID();uint32 spellID=row.param,playerCastDistance=row.second==128?row.dist:0;bool isTrigger=false;
#include "InvokerQueue.inc"
  }
 }
}
static std::vector<Row> Read(char const* path){std::ifstream f(path);std::string line;std::vector<Row> rows;while(std::getline(f,line)){std::stringstream stream(line);std::string field;std::vector<std::string> v;while(std::getline(stream,field,'\t'))v.push_back(field);assert(v.size()==31);auto n=[&](size_t i){return uint32(std::stoul(v[i]));};rows.push_back({n(2),n(5),n(14),n(3),n(15),n(16),n(21),n(22),n(9),n(10)});}return rows;}
struct Fixture{Player owner,party,stranger;Creature source;std::unique_ptr<Creature> ride;std::unique_ptr<npc_talonclaw_boarded_ride> ai;std::unique_ptr<npc_talonclaw_boarding_source> sourceAI;std::vector<Row> rows;unsigned transfers=0;int transferMode=0;ObjectGuid originalPet;std::unique_ptr<spell_talonclaw_boarded_transfer> lastTransfer;
 explicit Fixture(std::vector<Row> r):rows(std::move(r)),originalPet(owner.primaryPet){warnings=0;manager.present=true;sourceAI=std::make_unique<npc_talonclaw_boarding_source>(&source);source.ai=sourceAI.get();sourceAI->script.rows=rows;castEndpoint=[this](Unit* caster,Unit* target,uint32 spell,bool triggered){assert(caster==&owner&&target==&owner&&spell==198950&&!triggered);++transfers;if(transferMode==1)return;lastTransfer=std::make_unique<spell_talonclaw_boarded_transfer>();auto& s=*lastTransfer;s.caster=&owner;s.target=s.hit=owner.GetGUID();s.info.Id=198950;assert(s.Load());if(s.Check()!=SPELL_CAST_OK)return;s.Guard(0);if(s.prevented)return;owner.pending=true;owner.contact=false;owner.destination=WorldLocation{};if(transferMode==2)owner.destination.map=1512;s.After();};}
 ~Fixture(){assert(owner.primaryPet==originalPet);castEndpoint={};}
 void Spawn(){ride=std::make_unique<Creature>(100967,0);ride->summoner=owner.GetGUID();ai=std::make_unique<npc_talonclaw_boarded_ride>(ride.get());ride->ai=ai.get();owner.summons[100967].push_back(ride->GetGUID());}
 SpellCastResult Start(bool spawn=true){spell_talonclaw_boarding_summon cast;cast.info.Id=198941;cast.caster=&owner;cast.target=source.GetGUID();assert(cast.Load());auto result=cast.Check();if(result!=SPELL_CAST_OK)return result;sourceAI->OnSpellClick(&owner);assert(!owner.credit&&!transfers);if(spawn)Spawn();cast.After();return result;}
 void Board(){assert(ride&&ai);owner.waitOnSeat=false;owner.vehicle=&ride->kit;owner.base=ride.get();ride->kit.passenger=&owner;ride->kit.Seats[0].Passenger.Guid=owner.GetGUID();owner.moved=ride.get();ai->PassengerBoarded(&owner,0,true);}
 void Tick(uint32 ms){owner.functions.Update(ms);if(ai)ai->UpdateAI(ms);}
 void Casting(){Board();Tick(1);assert(!owner.credit&&owner.exits==1&&!owner.spline.done);owner.spline.done=true;Tick(1);assert(ai->stage==npc_talonclaw_boarded_ride::Stage::Casting&&!owner.credit&&!transfers);}
};
int main(int argc,char** argv){assert(argc==3);auto old=Read(argv[1]),fixed=Read(argv[2]);assert(old.size()==2&&fixed.size()==2);assert(old[0].event==73&&old[0].action==33&&old[0].link==1);assert(fixed[0].event==38&&fixed[0].link==0&&fixed[1].event==73&&fixed[1].second==128&&fixed[1].target==130&&fixed[1].dist==15);
 // Pinned original graph's actual action33 can credit without a summon/seat.
 {Fixture f(old);f.sourceAI->script.ProcessEventsFor(73,&f.owner);assert(f.owner.credit==1&&!f.ride&&!f.owner.joins);}
 unsigned cases=1;
 {++cases;Fixture f(fixed);f.owner.group={&f.party};assert(f.Start()==SPELL_CAST_OK&&!f.owner.credit&&f.owner.joins==1);f.sourceAI->OnSpellClick(&f.owner);assert(f.owner.joins==1&&!f.sourceAI->script.nativeClicks);f.Casting();f.Tick(199);assert(!f.transfers&&!f.owner.credit);f.Tick(1);assert(f.transfers==1&&f.owner.credit==1&&f.owner.creditCalls==1&&f.party.credit==1&&f.sourceAI->script.dataCalls==1&&!warnings);f.lastTransfer->After();f.Tick(1000);assert(f.owner.creditCalls==1&&f.transfers==1&&f.owner.restores>=1);}
 {++cases;Fixture f(fixed);assert(f.Start(false)==SPELL_CAST_OK&&!f.owner.credit&&!f.owner.joins&&!f.transfers);}
 for(unsigned bad=0;bad<12;++bad){++cases;Fixture f(fixed);switch(bad){case 0:f.owner.quest.Status=0;break;case 1:f.owner.rewarded=true;break;case 2:f.owner.klass=1;break;case 3:f.owner.alive=false;break;case 4:f.source.alive=false;break;case 5:f.owner.phase=false;break;case 6:f.owner.los=false;break;case 7:f.owner.visible=false;break;case 8:f.owner.sourceDistance=5.01f;break;case 9:f.owner.pending=true;break;case 10:f.owner.transport=true;break;case 11:f.source.npcFlags=0;break;}assert(f.Start()!=SPELL_CAST_OK&&!f.owner.credit&&!f.ride);}
 for(unsigned bad=0;bad<9;++bad){++cases;Fixture f(fixed);assert(f.Start()==SPELL_CAST_OK);switch(bad){case 0:f.owner.quest.ScriptLifetime=std::make_shared<uint8>();break;case 1:f.owner.quest.Status=0;f.owner.quest.ScriptLifetime.reset();break;case 2:f.owner.map=&otherMap;break;case 3:f.owner.alive=false;break;case 4:f.owner.phase=false;break;case 5:f.ride->expired=true;break;case 6:f.ride->summoner=f.stranger.GetGUID();break;case 7:f.owner.world=false;break;case 8:f.ride->alive=false;break;}f.Tick(1);assert(!f.owner.credit&&f.ai->stage==npc_talonclaw_boarded_ride::Stage::Finished&&f.ride->despawn==1&&warnings==1);}
 {++cases;Fixture f(fixed);f.owner.joinAccepted=false;assert(f.Start()==SPELL_CAST_OK);f.Tick(4999);assert(f.owner.joins==1);f.Tick(1);assert(f.owner.joins==2);f.Tick(5000);assert(f.owner.joins==3);f.Tick(5000);assert(f.ride->despawn&&!f.owner.credit&&warnings==1);}
 {++cases;Fixture f(fixed);f.Start();f.Tick(30000);assert(f.owner.joins==1&&f.ride->despawn&&!f.owner.credit);}
 {++cases;Fixture f(fixed);f.Start();f.Casting();f.owner.quest.ScriptLifetime=std::make_shared<uint8>();f.Tick(200);assert(f.transfers==1&&!f.owner.credit&&f.ride->despawn);}
 {++cases;Fixture f(fixed);f.Start();f.Casting();f.source.world=false;f.Tick(200);assert(!f.transfers&&!f.owner.credit);}
 {++cases;Fixture f(fixed);f.Start();f.Board();f.Tick(1);f.Tick(10000);assert(!f.transfers&&!f.owner.credit&&f.ride->despawn&&warnings==1);}
 {++cases;Fixture f(fixed);f.transferMode=2;f.Start();f.Casting();f.Tick(200);assert(f.transfers==1&&!f.owner.credit&&f.ride->despawn);}
 {++cases;Fixture f(fixed);f.transferMode=1;f.Start();f.Casting();for(unsigned i=0;i<3;++i){f.Tick(200);f.Tick(5000);f.Tick(1);}assert(f.transfers==3&&!f.owner.credit&&f.ride->despawn);}
 {++cases;Fixture f(fixed);f.Start();f.Board();f.owner.ExitVehicle();assert(f.ride->despawn&&!f.owner.credit);}
 {++cases;Fixture f(fixed);f.Start();f.ai->PassengerBoarded(&f.stranger,0,true);assert(!f.owner.credit&&f.ai->stage==npc_talonclaw_boarded_ride::Stage::Joining);}
 {++cases;Fixture f(fixed);f.Start();spell_talonclaw_boarding_summon again;again.caster=&f.owner;again.target=f.source.GetGUID();again.info.Id=198941;assert(again.Load()&&again.Check()==SPELL_FAILED_NOT_READY);}
 {++cases;Fixture f(fixed);f.Start();f.Casting();f.source.receiptAllows=false;f.Tick(200);assert(f.owner.credit==0&&f.ride->despawn==1000&&warnings==1);}
 for(unsigned mode=0;mode<3;++mode){++cases;Fixture f(fixed);spell_talonclaw_boarding_summon cast;cast.caster=&f.owner;cast.target=f.source.GetGUID();cast.info.Id=198941;assert(cast.Load()&&cast.Check()==SPELL_CAST_OK);f.Spawn();if(mode==0)f.owner.quest.ScriptLifetime=std::make_shared<uint8>();if(mode==1)f.source.world=false;if(mode==2)f.owner.visible=false;cast.After();assert(f.ride->despawn==1&&f.ai->stage==npc_talonclaw_boarded_ride::Stage::Unbound&&!f.owner.joins&&!f.owner.credit);}
 {++cases;Fixture f(fixed);f.Start();f.Casting();f.owner.credit=1;f.Tick(200);assert(f.owner.credit==1&&!f.owner.creditCalls&&f.ai->stage==npc_talonclaw_boarded_ride::Stage::Finished&&!warnings);}
 {++cases;Fixture f(fixed);f.Start();f.ride->kit.passenger=&f.stranger;f.Tick(5000);assert(f.owner.joins==1&&!f.owner.credit);}
 {++cases;Fixture f(fixed);f.Start();Creature other(100,0);f.stranger.base=&other;f.ai->PassengerBoarded(&f.stranger,0,true);assert(!f.stranger.exits&&f.stranger.base==&other);}
 {++cases;Fixture f(fixed);f.Start();f.Board();f.ai->PassengerBoarded(&f.owner,0,true);f.Tick(1);assert(f.owner.exits==1&&!f.owner.credit);}
 {++cases;Fixture f(fixed);f.Start();f.Tick(0xffffffffu);assert(f.ride->despawn&&!f.owner.credit&&warnings==1);}
 std::printf("PASS %u extracted production boarding/source/summon/transfer cases + actual FunctionProcessor/quest lease/EnterVehicle/SmartAI callback/native action33 and200ms invoker closure. Old early-credit counterfactual paired with actual-board/native-exit/accepted-target receipt; casts,world/seat/spline/teleport/group endpoints controlled; client flight/camera/pet rendering separate.\n",cases);
}
