#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <atomic>
#include <cmath>
#include <ctime>
#include "stormheim_fleet_state.h"
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int8=int8_t;
enum {ALLIANCE=469,HORDE=67,QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_REWARDED=6,
 REACT_PASSIVE,REACT_AGGRESSIVE,TEMPSUMMON_MANUAL_DESPAWN,UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK=1,UNIT_NPC_FLAG_GOSSIP=2,UNIT_NPC_FLAG_QUESTGIVER=4,
 UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE=1,UNIT_FLAG_IMMUNE_TO_PC=2,UNIT_FLAG_NOT_SELECTABLE=4,GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE=16,GO_FLAG_IN_USE=32,
 CRITERIA_TYPE_SCRIPT_EVENT_2=92,UNIT_STATE_ROOT=1,MOVE_FLIGHT=1,POINT_MOTION_TYPE=1,SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=1,EFFECT_2=2,SPELL_EFFECT_KILL_CREDIT=90,QUEST_OBJECTIVE_MONSTER=0};
using SpellCastResult=int;using SpellEffIndex=int;
enum {CHAR_SEL_STORMHEIM_MISSION,CHAR_REP_STORMHEIM_MISSION,CHAR_UPD_STORMHEIM_MISSION,CHAR_DEL_STORMHEIM_MISSION};
struct ObjectGuid {
 uint64 value=0;static ObjectGuid const Empty;uint64 GetCounter()const{return value;}
 bool IsEmpty()const{return !value;}void Clear(){value=0;}bool operator<(ObjectGuid g)const{return value<g.value;}bool operator==(ObjectGuid g)const{return value==g.value;}bool operator!=(ObjectGuid g)const{return value!=g.value;}
};ObjectGuid const ObjectGuid::Empty{};
struct Position {
 float m_positionX=0,m_positionY=0,m_positionZ=0,m_orientation=0;
 void Relocate(float x,float y,float z,float o=0){m_positionX=x;m_positionY=y;m_positionZ=z;m_orientation=o;}void Relocate(Position const& p){*this=p;}
 float GetPositionX()const{return m_positionX;}float GetPositionY()const{return m_positionY;}float GetPositionZ()const{return m_positionZ;}float GetOrientation()const{return m_orientation;}
};
struct WorldLocation:Position {uint32 mapId=0;void WorldRelocate(uint32 m,float x,float y,float z,float o){mapId=m;Relocate(x,y,z,o);}};
struct Player;struct Creature;struct TempSummon;struct GameObject;struct Transport;struct InstanceScript;struct InstanceMap;
struct SpellInfo {uint32 Id=0;};
struct Move {uint32 id=0;Position target;void MovePoint(uint32 token,float x,float y,float z,bool path){assert(!path);id=token;target.Relocate(x,y,z);}};
struct Vehicle {struct Unit* passenger=nullptr;struct Unit* GetPassenger(int seat){assert(seat==0);return passenger;}};
struct WorldObject:Position {
 ObjectGuid guid;InstanceMap* map=nullptr;bool world=true,alive=true,phase=true,visible=true,los=true;uint32 mapId=1475,instanceId=1;
 virtual ~WorldObject()=default;ObjectGuid GetGUID()const{return guid;}uint32 GetMapId()const{return mapId;}uint32 GetInstanceId()const{return instanceId;}InstanceScript* GetInstanceScript();
 bool IsInWorld(){return world;}bool IsAlive(){return alive;}float GetExactDist(Position const* p)const{return std::sqrt(std::pow(m_positionX-p->m_positionX,2)+std::pow(m_positionY-p->m_positionY,2)+std::pow(m_positionZ-p->m_positionZ,2));}
 void SetPhaseMask(uint32,bool){}void SetPhaseId(std::set<uint32> const&,bool){}void setIgnorePhaseIdCheck(bool){}void UpdateObjectVisibility(){}void SetVisible(bool value){visible=value;}
};
struct Unit:WorldObject {
 struct {struct {Position Pos;ObjectGuid Guid;int VehicleSeatIndex=0;} transport;} m_movementInfo;
 Player* playerOwner=nullptr;Vehicle vehicle;bool hasKit=false;Creature* vehicleBase=nullptr;Transport* transport=nullptr;bool combat=false;Move motion;
 virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}
 Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return ToPlayer()?ToPlayer():playerOwner;}Vehicle* GetVehicleKit(){return hasKit?&vehicle:nullptr;}Creature* GetVehicleBase(){return vehicleBase;}
 Transport* GetTransport(){return transport;}Move* GetMotionMaster(){return &motion;}bool isInCombat(){return combat;}void CombatStop(bool){combat=false;}
 void SetControlled(bool,int){}void SetReactState(int){}void SetCanFly(bool){}void SetDisableGravity(bool){}void SetSpeed(int,float,bool){}
};
struct Creature:Unit {
 uint32 entry=0,flags=0,npcflags=0,faction=35,dbGuid=0,hp=100,maxhp=100;bool despawned=false;
 Creature* ToCreature()override{return this;}uint32 GetEntry(){return entry;}uint32 GetDBTableGUIDLow(){return dbGuid;}void setFaction(uint32 value){faction=value;}
 void SetFlag(int field,uint32 f){(field==UNIT_FIELD_NPC_FLAGS?npcflags:flags)|=f;}void RemoveFlag(int field,uint32 f){(field==UNIT_FIELD_NPC_FLAGS?npcflags:flags)&=~f;}
 void SetHealth(uint32 value){hp=value;}void SetMaxHealth(uint32 value){maxhp=value;}uint64 GetHealth(Unit*){return hp;}uint64 GetMaxHealth(Unit*){return maxhp;}
 void DespawnOrUnsummon(uint32=0){despawned=true;world=false;alive=false;}
 SpellInfo const* reachWithSpellAttack(Unit*){return nullptr;}Unit* getVictim(){return nullptr;}
 std::set<uint32> auras;void AddAura(uint32 spell,Creature*){auras.insert(spell);}void RemoveAurasDueToSpell(uint32 spell){auras.erase(spell);}void RemoveAllAuras(){auras.clear();}bool HasAura(uint32 spell){return auras.count(spell);}int CastSpell(Creature*,uint32 spell,bool){auras.insert(spell);return SPELL_CAST_OK;}
};
struct TempSummon:Creature {};
struct GameObject:WorldObject {uint32 entry=0,flags=16;bool deleted=false;virtual Transport* ToTransport(){return nullptr;}uint32 GetEntry(){return entry;}void RemoveFlag(int,uint32 f){flags&=~f;}void Delete(){deleted=true;world=false;}};
struct InstanceMap {uint32 id=1;InstanceScript* script=nullptr;std::map<ObjectGuid,Creature*> creatures;std::map<ObjectGuid,GameObject*> objects;
 uint32 GetInstanceId(){return id;}Creature* GetCreature(ObjectGuid g){auto p=creatures.find(g);return p==creatures.end()||!p->second->world?nullptr:p->second;}GameObject* GetGameObject(ObjectGuid g){auto p=objects.find(g);return p==objects.end()||!p->second->world?nullptr:p->second;}
};
InstanceScript* WorldObject::GetInstanceScript(){return map?map->script:nullptr;}
uint64 nextGuid=1000;bool failSummons=false,failObjects=false;
TempSummon* Summon(InstanceMap* map,uint32 entry,Position const& pos){if(failSummons)return nullptr;auto c=new TempSummon;c->guid={++nextGuid};c->entry=entry;c->map=map;c->instanceId=map->id;c->Relocate(pos);c->hasKit=entry==92957||entry==94550||entry==94517||entry==94576||entry==93433;map->creatures[c->guid]=c;return c;}
struct Transport:GameObject {
 bool moving=false;Transport(){entry=241630;}Transport* ToTransport()override{return this;}void EnableMovement(bool v){moving=v;}
 void CalculatePassengerPosition(float& x,float& y,float& z,float*){x+=m_positionX;y+=m_positionY;z+=m_positionZ;}
 void CalculatePassengerOffset(float& x,float& y,float& z,float*){x-=m_positionX;y-=m_positionY;z-=m_positionZ;}
 void AddPassenger(Unit* unit){unit->transport=this;}void RemovePassenger(Unit* unit){unit->transport=nullptr;}
 TempSummon* SummonPassenger(uint32 entry,Position pos,int,void*,uint32,Player*){CalculatePassengerPosition(pos.m_positionX,pos.m_positionY,pos.m_positionZ,&pos.m_orientation);auto c=Summon(map,entry,pos);if(c)c->transport=this;return c;}
};
struct TransportMgr {Transport* CreateTransport(uint32 entry,int,InstanceMap* map){assert(entry==241630);auto t=new Transport;t->guid={++nextGuid};t->map=map;t->instanceId=map->id;t->Relocate(5071,3141,347);map->objects[t->guid]=t;return t;}} transportMgr;auto sTransportMgr=&transportMgr;
struct Criteria {uint32 step=0;std::map<uint32,uint32> events;void ClearMap(){step=0;events.clear();}};
struct Scenario {Criteria criteria;uint32 GetCurrentStep(){return criteria.step;}void SetCurrentStep(uint32 s){criteria.step=s;}Criteria& GetAchievementMgr(){return criteria;}};
struct ScenarioMgr {std::map<uint32,Scenario> scenarios;Scenario* GetScenario(uint32 i){return &scenarios[i];}} scenarioMgr;auto sScenarioMgr=&scenarioMgr;
struct QuestStatusData {int Status=QUEST_STATUS_INCOMPLETE;std::shared_ptr<uint8> ScriptLifetime;};
struct QuestObjective {int Type=0;uint32 ObjectID=113320;};struct Quest {std::vector<QuestObjective> objectives{{}};auto const& GetObjectives()const{return objectives;}} questTemplate;
struct QuestStore {Quest const* GetQuestTemplate(uint32){return &questTemplate;}} questStore;auto sQuestDataStore=&questStore;
struct Player:Unit {
 uint32 team=ALLIANCE,quest=39800,objective=0,credits=0,completions=0,controls=0;bool teleport=false,movie=false,casting=false;int castResult=0;
 QuestStatusData status;std::set<uint32> auras;std::vector<uint32> spells;std::vector<std::function<void()>> events;
 std::function<void(Player*)> exitCallback;
 struct {struct {Position Pos;ObjectGuid Guid;int VehicleSeatIndex=0;} transport;} m_movementInfo;
 Player* ToPlayer()override{return this;}uint32 GetTeam(){return team;}int GetQuestStatus(uint32 q){return q==quest?status.Status:QUEST_STATUS_NONE;}QuestStatusData* getQuestStatus(uint32 q){return q==quest?&status:nullptr;}
 uint32 GetPhaseMask(){return 1;}std::set<uint32> GetPhases(){return {1};}bool InSamePhase(WorldObject*){return phase;}bool canSeeOrDetect(WorldObject* o){return visible&&o->visible;}bool IsWithinLOSInMap(WorldObject*){return los;}
 bool IsBeingTeleported(){return teleport;}Vehicle* GetVehicle(){return vehicleBase?&vehicleBase->vehicle:nullptr;}void ExitVehicle(){if(vehicleBase){if(exitCallback)exitCallback(this);vehicleBase->vehicle.passenger=nullptr;}vehicleBase=nullptr;}
 void SetClientControl(Unit*,bool){++controls;}void NearTeleportTo(float x,float y,float z,float o){Relocate(x,y,z,o);}void TeleportTo(uint32 id,float x,float y,float z,float o){mapId=id;Relocate(x,y,z,o);}
 bool IsNonMeleeSpellCast(bool){return casting;}bool isWatchingMovie(){return movie;}void setWatchinMovie(bool v){movie=v;}bool HasAura(uint32 s){return auras.count(s);}void RemoveAurasDueToSpell(uint32 s){auras.erase(s);}
 int CastSpell(Player*,uint32 s,bool){spells.push_back(s);if(!castResult && (s==185614||s==185616)){auras.insert(s);movie=true;}return castResult;}
 int CastSpell(Creature*,uint32 s,bool){spells.push_back(s);return castResult;}
 TempSummon* SummonCreature(uint32 entry,Position const& pos,int,uint32,int,ObjectGuid){return Summon(map,entry,pos);}
 GameObject* SummonGameObject(uint32 entry,Position const& pos,float,int,int,int,int,uint32,ObjectGuid){if(failObjects)return nullptr;auto go=new GameObject;go->entry=entry;go->guid={++nextGuid};go->map=map;go->instanceId=instanceId;go->Relocate(pos);map->objects[go->guid]=go;return go;}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);void AddDelayedEvent(uint64,std::function<void()>&& f){events.push_back(std::move(f));}
 uint32 GetQuestObjectiveData(uint32,uint32){return objective;}void SetQuestObjectiveData(Quest const*,QuestObjective const*,uint32 v){objective=v;}void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,uint32){++credits;}
 bool CanCompleteQuest(uint32){return objective==1;}void CompleteQuest(uint32){status.Status=QUEST_STATUS_COMPLETE;++completions;}
 void UpdateAchievementCriteria(uint32 type,uint32 asset)
 {
  assert(type==92);auto& c=scenarioMgr.scenarios[instanceId].criteria;++c.events[asset];
  if(asset==44799||asset==45340)c.step=1;if(asset==44893||asset==45416&&c.events[asset]==30)c.step=2;
  if(asset==45034&&c.events[44892]&&c.events[45033])c.step=3;if(asset==45365)c.step=3;if(asset==45366)c.step=4;
  if(asset==53493&&c.events[asset]==7)c.step=4;if(asset==53445&&c.events[asset]==7)c.step=5;if(asset==45041)c.step=5;if(asset==45368)c.step=6;
 }
};
#include "QuestDelayedEvent.inc"
namespace ObjectAccessor {std::map<ObjectGuid,Player*> players;Player* FindPlayer(ObjectGuid g){return players[g];}GameObject* GetGameObject(Player& p,ObjectGuid g){return p.map->GetGameObject(g);}}
struct Stmt {int tag;std::vector<uint64> values=std::vector<uint64>(9);void setUInt8(int i,uint8 v){values[i]=v;}void setUInt32(int i,uint32 v){values[i]=v;}void setUInt64(int i,uint64 v){values[i]=v;}};
struct Field {uint64 v;uint64 GetUInt64(){return v;}uint32 GetUInt32(){return uint32(v);}uint8 GetUInt8(){return uint8(v);}};
struct Rows {std::vector<Field> values;Field* Fetch(){return values.data();}};using PreparedQueryResult=std::shared_ptr<Rows>;
struct DB {std::map<std::pair<uint64,uint64>,std::vector<uint64>> rows;Stmt* GetPreparedStatement(int tag){auto s=new Stmt;s->tag=tag;return s;}
 void DirectExecute(Stmt* s){rows[{s->values[0],s->values[1]}]={s->values[2],0,0,0,0,0,0};}
 void Execute(Stmt* s){if(s->tag==CHAR_UPD_STORMHEIM_MISSION){auto& r=rows[{s->values[6],s->values[7]}];if(r.size()==7&&r[0]==s->values[8])for(int i=0;i<6;++i)r[i+1]=s->values[i];}else if(s->tag==CHAR_DEL_STORMHEIM_MISSION){auto key=std::make_pair(s->values[0],s->values[1]);auto f=rows.find(key);if(f!=rows.end()&&f->second[0]==s->values[2])rows.erase(f);}}
 PreparedQueryResult Query(Stmt* s){auto f=rows.find({s->values[0],s->values[1]});if(f==rows.end())return nullptr;auto r=std::make_shared<Rows>();for(auto v:f->second)r->values.push_back({v});return r;}
} CharacterDatabase;
struct InstanceScript {InstanceMap* instance;explicit InstanceScript(InstanceMap* map):instance(map){map->script=this;}virtual ~InstanceScript()=default;
 virtual WorldLocation* GetClosestGraveYard(float,float,float){return nullptr;}
 virtual void OnGameObjectCreate(GameObject*){}virtual void OnCreatureCreate(Creature*){}virtual void OnPlayerEnter(Player*){}virtual void Update(uint32){}virtual void OnCreatureDamageTaken(Creature*,Unit*,uint32&){}virtual void CreatureDies(Creature*,Unit*){}
 GameObject* GetGameObjectByEntry(uint32 entry){for(auto const& o:instance->objects)if(o.second->entry==entry)return o.second;return nullptr;}
};
struct InstanceMapScript {InstanceMapScript(char const*,uint32){}virtual InstanceScript* GetInstanceScript(InstanceMap*)const{return nullptr;}};
struct SmartAI {Creature* me;explicit SmartAI(Creature* c):me(c){}virtual void PassengerBoarded(Unit*,int8,bool){}virtual void MovementInform(uint32,uint32){}virtual void UpdateAI(uint32){}bool UpdateVictim(){return me->combat;}void DoMeleeAttackIfReady(){}void DoCastVictim(uint32){}};
struct Hook {void operator+=(int){}};
struct SpellScript {Unit* caster=nullptr;Unit* target=nullptr;SpellInfo info;bool prevented=false;Hook OnCheckCast,OnEffectHitTarget,AfterCast;
 Unit* GetCaster(){return caster;}Unit* GetExplTargetUnit(){return target;}SpellInfo const* GetSpellInfo(){return &info;}void PreventHitDefaultEffect(int){prevented=true;}virtual void Register(){}};
#define PrepareSpellScript(x) public:
#define SpellCheckCastFn(x) 0
#define SpellEffectFn(...) 0
#define SpellCastFn(x) 0
struct GameObjectScript {explicit GameObjectScript(char const*){}virtual bool OnGossipHello(Player*,GameObject*){return false;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnLogin(Player*){}virtual void OnUpdate(Player*,uint32){}virtual void OnLogout(Player*){}};
#include "Fleet.inc"

Creature* Find(StormheimFleet::Mission& m,StormheimFleet::Kind kind,uint32 slot=0)
{for(auto const& a:m.actors)if(a.second.kind==kind&&a.second.slot==slot&&!a.second.done)return m.instance->GetCreature(a.first);return nullptr;}
void Tick(StormheimFleet::Mission& m,int n=1){for(int i=0;i<n;++i)m.Update(500);}
void Fly(StormheimFleet::Mission& m,Player& p)
{
 using namespace StormheimFleet;Tick(m);if(m.landed)return;
 for(auto const& o:m.objects)if(o.second.kind==Kind::Parachute){auto go=m.instance->GetGameObject(o.first);assert(go);p.Relocate(*go);go_stormheim_fleet_parachute use;use.OnGossipHello(&p,go);break;}
 auto c=Find(m,Kind::Flight);assert(c);p.Relocate(*c);assert(m.Board(&p,c));c->vehicle.passenger=&p;p.vehicleBase=c;
 npc_stormheim_fleet_actor ai(c);ai.PassengerBoarded(&p,0,true);assert(m.flight==c->guid&&p.controls);
 p.exitCallback=[&ai](Player* owner){ai.PassengerBoarded(owner,0,false);};
 for(uint32 id:{147501u,147502u,147503u}){c->Relocate(c->motion.target);ai.MovementInform(POINT_MOTION_TYPE,id);}
 assert(m.landed&&m.flight.IsEmpty()&&!p.GetVehicle());
 p.exitCallback={};
}
int main()
{
 using namespace StormheimFleet;
 for(bool alliance:{true,false})
 {
  checkpoints.clear();leases.clear();CharacterDatabase.rows.clear();scenarioMgr.scenarios.clear();ObjectAccessor::players.clear();
  InstanceMap map;Player p;p.guid={1};p.map=&map;p.team=alliance?ALLIANCE:HORDE;p.quest=alliance?39800:39801;ObjectAccessor::players[p.guid]=&p;
  Accepted(&p);Mission m(&map);m.OnPlayerEnter(&p);Tick(m);assert(m.Ship()&&!m.Ship()->moving);
  if(alliance){Creature* gun=Find(m,Kind::Gun);assert(gun&&(gun->npcflags&UNIT_NPC_FLAG_SPELLCLICK));p.Relocate(*gun);assert(m.Board(&p,gun));m.Boarded(&p,gun);assert(m.state.stage==0);gun->vehicle.passenger=&p;p.vehicleBase=gun;m.Boarded(&p,gun);assert(m.state.stage==1&&scenarioMgr.scenarios[1].criteria.events[44799]==1);m.Boarded(&p,gun);Tick(m);}
  else
  {
   p.Relocate(Boats[2]);p.UpdateAchievementCriteria(92,45340);Tick(m,2);
   Creature* ride=Find(m,Kind::Flight);assert(ride);p.Relocate(*ride);p.vehicleBase=ride;ride->vehicle.passenger=&p;
   npc_stormheim_fleet_actor interrupted(ride);interrupted.PassengerBoarded(&p,0,true);interrupted.UpdateAI(120001);
   assert(m.flight.IsEmpty()&&!m.landed&&m.state.stage==1&&!m.actors.count(ride->guid));Tick(m);assert(Find(m,Kind::Flight)&&Find(m,Kind::Flight)->guid!=ride->guid);
   Fly(m,p);
  }
  Unit pet;pet.playerOwner=&p;
  Player other;other.guid={2};other.map=&map;other.quest=p.quest;other.team=p.team;ObjectAccessor::players[other.guid]=&other;
  uint32 killed=0;
  while(killed<30)
  {
   Tick(m);Creature* bird=nullptr;for(auto const& a:m.actors)if(a.second.kind==Kind::Air&&!a.second.done){bird=map.GetCreature(a.first);if(bird)break;}
   if(!bird)continue;
   uint32 damage=100;m.OnCreatureDamageTaken(bird,&other,damage);assert(damage==0);auto before=m.state.air;m.CreatureDies(bird,&other);assert(m.state.air==before);
   m.CreatureDies(bird,&pet);m.CreatureDies(bird,&pet);assert(m.state.air==++killed);
   if(killed==10){Tick(m);assert(m.frozen);Creature* gun=Find(m,Kind::Gun);assert(gun);gun->playerOwner=&p;assert(!m.Authorized(gun));Creature* frost=Find(m,Kind::Frost);assert(frost);m.CreatureDies(frost,&pet);assert(!m.frozen&&m.state.frost==2);}
  }
  assert(m.state.stage==2&&!p.objective);
  // Re-entry after logout uses the new Player quest lifetime in the same map.
  uint64 savedSerial=m.serial;p.status.ScriptLifetime.reset();assert(!m.Owner());m.OnPlayerEnter(&p);assert(m.Owner()==&p&&m.serial==savedSerial&&m.state.air==30);
  if(alliance)
  {
   for(uint32 i=0;i<3;++i){Fly(m,p);auto captain=Find(m,Kind::Captain,i);assert(captain);if(i==2){uint32 hit=1000000000;m.OnCreatureDamageTaken(captain,&pet,hit);assert(hit==0&&captain->alive);}else m.CreatureDies(captain,&pet);assert(m.state.captains&(1u<<i));Tick(m);}
  }
  else
  {
   for(uint32 stage:{2u,3u}){Fly(m,p);std::vector<Creature*> wave;for(auto const& a:m.actors)if(a.second.kind==Kind::Guard&&!a.second.done)wave.push_back(map.GetCreature(a.first));assert(wave.size()==m.groundRequired);for(size_t i=0;i<wave.size();++i){m.CreatureDies(wave[i],&pet);if(i+1<wave.size())assert(m.state.stage==stage);}assert(m.state.stage==stage+1);Tick(m);}
  }
  Fly(m,p);assert(m.objects.size()==7);go_stormheim_fleet_flare use;
  for(uint32 i=0;i<7;++i)
  {
   GameObject* go=nullptr;for(auto const& o:m.objects)if(o.second.slot==i)go=map.GetGameObject(o.first);assert(go);p.Relocate(*go);
   p.castResult=1;use.OnGossipHello(&p,go);assert(!(m.state.flares&(1u<<i)));p.castResult=0;use.OnGossipHello(&p,go);
   spell_stormheim_fleet_flare cast;cast.caster=&p;assert(cast.Check()==SPELL_CAST_OK);
   // No completion callback after interruption means no credit/deletion.
   assert(!(m.state.flares&(1u<<i)));cast.Complete();assert(m.state.flares&(1u<<i));cast.Complete();
  }
  assert(!p.objective);Tick(m);Fly(m,p);Creature* boss=Find(m,Kind::Final);assert(boss);
  uint32 damage=1000000000;m.OnCreatureDamageTaken(boss,&other,damage);assert(damage==0&&!m.state.final);damage=1000000000;m.OnCreatureDamageTaken(boss,&pet,damage);assert(damage==0&&m.state.final&&!p.objective);m.OnCreatureDamageTaken(boss,&pet,damage);
  spell_stormheim_fleet_finale finale;finale.caster=&p;finale.info.Id=alliance?185614:185616;assert(finale.Check()==SPELL_CAST_OK);finale.Credit(2);assert(finale.prevented&&!p.objective);
  player_stormheim_fleet script;script.OnUpdate(&p,500);assert(!p.objective&&p.movie);
  script.OnLogout(&p);script.OnLogin(&p);assert(ReadCheckpoint(&p)->state.final); // durable earned finale survives login
  if(p.GetTransport())p.GetTransport()->RemovePassenger(&p);p.mapId=1220;p.Relocate(alliance?ArrivalA:ArrivalH);p.teleport=true;script.OnUpdate(&p,500);assert(!p.objective);p.teleport=false;p.movie=false;script.OnUpdate(&p,500);assert(p.objective==1&&p.credits==1&&p.completions==1);script.OnUpdate(&p,500);assert(p.credits==1);
  // Reaccept invalidates the old serial/lifetime; an old finale can't complete it.
  p.status={QUEST_STATUS_INCOMPLETE,{}};p.objective=0;Accepted(&p);assert(!ReadCheckpoint(&p)->state.final&&finale.Check()==SPELL_FAILED_BAD_TARGETS);
  // Expired runs remove their actors/objects. A new acceptance starts clean.
  p.mapId=1475;p.status.ScriptLifetime.reset();m.Update(500);assert(m.state.stage==0&&!m.state.final&&m.serial==ReadCheckpoint(&p)->serial);
  p.status.ScriptLifetime.reset();p.status.Status=QUEST_STATUS_NONE;m.Update(500);assert(m.actors.empty()&&m.objects.empty());
 }
 std::cout<<"PASS: extracted production controllers, both complete mission routes, actual boarding and three-point flight callbacks, 30 distinct owned kills, pet/frost/captain/wave handling, successful flare cast completion, stale/failing actions, durable finale proof and acknowledged arrival exactly once.\n";
}
