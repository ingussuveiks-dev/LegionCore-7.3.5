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
#include <array>
#include <list>
#include <algorithm>
using uint64=uint64_t;using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;using int8=int8_t;
enum QuestStatus {QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED};
enum {REACT_PASSIVE,UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE=2,UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_IMMUNE_TO_NPC=512,POINT_MOTION_TYPE=1,SPELL_AURA_MOUNTED=78};
enum SpellCastResult {SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS};
#include "QuestStatusData.inc"
enum class HighGuid{Creature};
struct ObjectGuid {template<HighGuid>static ObjectGuid Create(uint32,uint32,uint64 n){return {unsigned(n)};}bool IsEmpty()const{return !value;}static ObjectGuid const Empty;unsigned value=0;bool operator<(ObjectGuid r)const{return value<r.value;}bool operator==(ObjectGuid r)const{return value==r.value;}bool operator!=(ObjectGuid r)const{return value!=r.value;}};
ObjectGuid const ObjectGuid::Empty{};
struct Position {float x=0,y=0,z=0,o=0;void Relocate(float a,float b,float c,float d){x=a;y=b;z=c;o=d;}float GetOrientation()const{return o;}float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}float GetExactDist(Position const* p)const{return std::sqrt((x-p->x)*(x-p->x)+(y-p->y)*(y-p->y)+(z-p->z)*(z-p->z));}};
enum {QUEST_OBJECTIVE_TALKTO=3,UNIT_FIELD_NPC_FLAGS=44,UNIT_NPC_FLAG_SPELLCLICK=16777216,MOVE_FLIGHT=1,EFFECT_0=0,EFFECT_1=1,SPELL_EFFECT_SUMMON=28,SPELL_EFFECT_DUMMY=3,SPELL_EFFECT_TRIGGER_SPELL=64,TARGET_UNIT_DEST_AREA_ENTRY=8,TARGET_UNIT_DEST_AREA_ENEMY=16,TARGET_UNIT_NEARBY_ENTRY=38,QUEST_OBJECTIVE_MONSTER=0,QUEST_OBJECTIVE_FLAG_OPTIONAL=4,QUEST_OBJECTIVE_FLAG_SEQUENCED=2,MOVEMENTFLAG_FALLING=1,MOVEMENTFLAG_FALLING_FAR=2,TEMPSUMMON_MANUAL_DESPAWN=8};
struct QuestObjective {int Type=0;uint32 ObjectID=0,Amount=1,Flags=0;int StorageIndex=0;};
struct Quest {uint32 StartScript=0;std::vector<QuestObjective> objectives{{3,91767,1,0,0},{0,92360,150,2,3},{0,91768,1,2,1}};auto const& GetObjectives()const{return objectives;}} definition;
struct QuestStore {bool exists=true;Quest* GetQuestTemplate(uint32 id){assert(id==38624);return exists?&definition:nullptr;}} questStore;auto sQuestDataStore=&questStore;
struct GossipMenu {bool item=true;uint32 id=18693;uint32 GetMenuId(){return id;}void* GetItem(uint32 n){return !n&&item?this:nullptr;}};
struct PlayerMenu {GossipMenu gossip;GossipMenu& GetGossipMenu(){return gossip;}void ClearMenus(){gossip.item=false;}void SendCloseGossip(){}};
namespace VMAP{enum class ModelIgnoreFlags{Nothing};}
struct Creature;struct FakeMap {Creature* GetCreature(ObjectGuid);float offset=0;bool los=true;float GetHeight(float,float,float z,bool,float){return z-2+offset;}bool isInLineOfSight(float,float,float,float,float,float,std::set<uint32>const&,VMAP::ModelIgnoreFlags){return los;}};
struct Player;struct Creature;struct ScriptedAI;
using WorldObject=struct Unit;
struct Unit:Position {ObjectGuid guid;virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}virtual uint32 GetEntry(){return 0;}Player* GetCharmerOrOwnerPlayerOrPlayerItself();float GetExactDist(Unit* u){return Position::GetExactDist(u);}using Position::GetExactDist;bool IsWithinDistInMap(Unit* u,float d){return GetExactDist(u)<=d;}ObjectGuid GetGUID(){return guid;}float GetDistance(Unit* u){return GetExactDist(u);}};
struct Spline {bool final=true;bool Finalized(){return final;}};
struct Vehicle {struct Info {uint32 ID=4133;} info;Info* GetVehicleInfo(){return &info;}Unit* passenger=nullptr;unsigned relocations=0;Unit* GetPassenger(int seat){assert(seat==0);return passenger;}void RelocatePassengers(){++relocations;}};
std::map<ObjectGuid,Creature*> world;unsigned serial=100;
struct Player:Unit {
 QuestStatusData quest;FakeMap geometry;uint32 zone=7541,instance=0,movement=0,packets=0;std::map<uint32,uint32> counters;unsigned refreshes=0;void UpdateVisibilityForPlayer(){++refreshes;}PlayerMenu menu;PlayerMenu* PlayerTalkClass=&menu;std::function<Creature*(uint32,Position const&,int)> summon;FakeMap* GetMap(){return &geometry;}uint32 GetZoneId(){return zone;}uint32 GetInstanceId(){return instance;}bool HasUnitMovementFlag(uint32 f){return movement&f;}Creature* SummonCreature(uint32 e,Position const& pos,int,int,int vehicle=0){return summon?summon(e,pos,vehicle):nullptr;}std::set<uint32> phases;auto const& GetPhases(){return phases;}void SetQuestStatus(uint32,int status){quest.Status=static_cast<QuestStatus>(status);}uint32 GetQuestObjectiveData(uint32,uint32 id){return counters[id];}void SetQuestObjectiveData(Quest const*,QuestObjective const* o,uint32 v){counters[o->ObjectID]=v;}void SendQuestUpdateAddCredit(Quest const*,ObjectGuid g,QuestObjective const&,uint32 n){assert(g==ObjectGuid::Empty&&n>0&&n<=150);++packets;}bool CanCompleteQuest(uint32){return counters[91767]&&counters[92360]>=150&&counters[91768];}void CompleteQuest(uint32){quest.Status=QUEST_STATUS_COMPLETE;}bool alive=true,inWorld=true,teleport=false,flight=false,flying=false,phase=true,visible=true,los=true,combat=false,transport=false,mounted=false,aura=true;
 unsigned map=1220,enterCalls=0,teleports=0,saves=0,controls=0;Creature* vehicle=nullptr;Unit* mover=this;Unit* view=nullptr;Spline spline;Spline* movespline=&spline;
 Position exitDest,savedPos;std::vector<int> order;
 explicit Player(unsigned id){guid={id};quest.Status=QUEST_STATUS_INCOMPLETE;}
 Player* ToPlayer()override{return this;}bool IsAlive(){return alive;}bool IsInWorld(){return inWorld;}unsigned GetMapId(){return map;}
 int GetQuestStatus(unsigned id){assert(id==38624);return quest.Status;}bool IsBeingTeleported(){return teleport;}bool isInFlight(){return flight;}bool IsFlying(){return flying;}bool isInCombat(){return combat;}bool GetTransport(){return transport;}
 Creature* GetVehicle(){return vehicle;}Unit* GetVehicleBase();Unit* GetUnitBeingMoved(){return mover;}Unit* GetViewpoint(){return view;}
 bool InSamePhase(Unit*){return phase;}bool canSeeOrDetect(Unit*){return visible;}bool IsWithinLOSInMap(Unit*){return los;}
 void SetClientControl(Unit* u,bool control){mover=u;view=u==this?nullptr:u;++controls;order.push_back(control?3:1);}
 void SetViewpoint(Unit*,bool apply){assert(!apply);view=nullptr;}
 void SendMovementFlagUpdate(bool value){assert(value);}void StopMoving(){}
 void RemoveAurasDueToSpell(unsigned id){assert(id==194359);aura=false;}void RemoveAurasByType(unsigned type){assert(type==SPELL_AURA_MOUNTED);mounted=false;}
 void EnterVehicle(Creature*,int seat,bool triggered){assert(seat==0 && triggered);++enterCalls;}
 void ExitVehicle(Position const* pos=nullptr);void NearTeleportTo(Position const& p){static_cast<Position&>(*this)=p;spline.final=true;++teleports;}
 void SaveToDB(){++saves;savedPos=*this;}
 QuestStatusData* getQuestStatus(unsigned){return &quest;}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void AddDelayedEvent(uint64,std::function<void()> callback){callback();}
 void Reaccept(){counters.clear();quest=QuestStatusData{};quest.Status=QUEST_STATUS_INCOMPLETE;}
};
#include "QuestDelayedEvent.inc"
struct Motion {Player* owner=nullptr;unsigned moves=0,id=0;Position destination;void MovePoint(unsigned point,Position const& p,bool generate,float speed){assert(!generate && speed==24);id=point;destination=p;++moves;if(owner){assert(owner->mover!=owner);owner->order.push_back(2);}}};
struct Creature:Unit {
 uint32 m_templateSpells[8]{};uint32 entry=92302,zone=7541,instance=0;uint64 dbGuid=0;uint32 GetEntry()override{return entry;}Creature* ToCreature()override{return this;}uint64 GetDBTableGUIDLow(){return dbGuid;}uint32 GetMapId(){return 1220;}uint32 GetZoneId(){return zone;}uint32 GetInstanceId(){return instance;}bool IsInWorld(){return !gone;}Spline spline;Spline* movespline=&spline;Player* owner;ScriptedAI* ai=nullptr;bool alive=true,gone=false,personal=false,fly=false,gravity=false,active=false,hasKit=true;Motion motion;Vehicle kit;
 explicit Creature(Player& p):owner(&p){guid={++serial};world[guid]=this;motion.owner=&p;}
 bool IsAlive(){return alive;}bool IsDespawn(){return gone;}Vehicle* GetVehicleKit(){return hasKit?&kit:nullptr;}Motion* GetMotionMaster(){return &motion;}
 void DespawnOrUnsummon(unsigned=0){gone=true;}void AddPlayerInPersonnalVisibilityList(ObjectGuid g){assert(g==owner->guid);personal=true;}
 void SetReactState(unsigned){}std::set<ObjectGuid> hidden;bool ShouldHideFor(ObjectGuid g){return hidden.count(g);}void AddToHideList(ObjectGuid g){hidden.insert(g);}void RemoveFromHideList(ObjectGuid g){hidden.erase(g);}void DestroyForPlayer(Player*){}uint32 flags=16777216,dirty=0;bool HasFlag(uint32,uint32 f){return flags&f;}void ForceValuesUpdateAtIndex(uint32){++dirty;}FakeMap* GetMap(){return owner->GetMap();}void SetSpeed(int,float){}void SetOwnerGUID(ObjectGuid){}std::vector<std::pair<uint32,Unit*>> casts;int CastSpell(Unit* target,uint32 spell,bool){casts.push_back({spell,target});return 0;}void SetFlag(unsigned,unsigned){}void SetCanFly(bool v){fly=v;}void SetDisableGravity(bool v){gravity=v;}void setActive(bool v){active=v;}
 void NearTeleportTo(Position const& p){static_cast<Position&>(*this)=p;}void StopMoving(){}void SetFacingTo(float o2){o=o2;}
};
using Map=FakeMap;
Creature* FakeMap::GetCreature(ObjectGuid guid){auto it=world.find(guid);return it==world.end()||it->second->gone?nullptr:it->second;}
Unit* Player::GetVehicleBase(){return vehicle;}
struct ObjectAccessor {static Creature* GetCreature(Player& p,ObjectGuid g){auto i=world.find(g);return p.map!=1220||i==world.end()||i->second->gone?nullptr:i->second;}static Player* GetPlayer(Unit& source,ObjectGuid guid){auto c=source.ToCreature();return c&&c->owner->guid==guid&&c->owner->inWorld?c->owner:nullptr;}};
Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself(){if(auto p=ToPlayer())return p;if(auto c=ToCreature())return c->owner;return nullptr;}
struct ScriptedAI {Creature* me;explicit ScriptedAI(Creature* c):me(c){c->ai=this;}virtual ~ScriptedAI()=default;virtual void IsSummonedBy(Unit*){}virtual void PassengerBoarded(Unit*,int8,bool){}virtual void MovementInform(uint32,uint32){}virtual void CalcExitVehiclePos(Position&){}virtual void JustDied(Unit*){}virtual void UpdateAI(uint32){};};
struct SmartAI:ScriptedAI {unsigned delegated=0;using ScriptedAI::ScriptedAI;virtual void sGossipHello(Player*){++delegated;}virtual void sGossipSelect(Player*,uint32,uint32){++delegated;}};
struct Template {uint32 ScriptID=1;};struct ObjectMgr {bool valid=true;Template info;Template* GetCreatureTemplate(uint32 id){info.ScriptID=id==92302?1:2;return valid?&info:nullptr;}std::string GetScriptName(uint32 id){return id==1?"npc_stormheim_dragons":"npc_stormheim_cry_thunder_target";}} objectMgr;auto sObjectMgr=&objectMgr;
void Player::ExitVehicle(Position const* pos){if(!vehicle)return;auto bird=vehicle;bird->kit.passenger=nullptr;bird->ai->PassengerBoarded(this,0,false);vehicle=nullptr;exitDest=pos?*pos:static_cast<Position const&>(*bird);if(bird->ai)bird->ai->CalcExitVehiclePos(exitDest);spline.final=false;order.push_back(4);}
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnUpdate(Player*,uint32){}virtual void OnMapChanged(Player*){}virtual void OnLogout(Player*){}virtual void OnLogin(Player*){};};
using SpellEffIndex=int;
struct SpellScript {uint32 m_scriptSpellId=0;Unit* caster=nullptr,*original=nullptr;bool suppressed=false;Unit* GetCaster(){assert(caster);return caster;}Unit* GetOriginalTarget(){return original;}void PreventHitDefaultEffect(int){suppressed=true;}struct Hook{std::vector<int> values;void operator+=(int n){values.push_back(n);}} OnCheckCast,AfterCast,OnEffectLaunch,OnEffectLaunchTarget,OnEffectHit,OnEffectHitTarget,OnObjectTargetSelect,OnObjectAreaTargetSelect;virtual void Register(){};};
#define PrepareSpellScript(name) public:
#define SpellCheckCastFn(fn) 1
#define SpellCastFn(fn) 2
#define SpellEffectFn(fn,i,t) (1000+(i)*100+(t))
#define SpellObjectTargetSelectFn(fn,i,t) (2000+(i)*100+(t))
#define SpellObjectAreaTargetSelectFn(fn,i,t) (3000+(i)*100+(t))
void GetCreatureListWithEntryInGrid(std::list<Creature*>& out,Player*,uint32 entry,float){for(auto const& row:world)if(row.second->entry==entry&&!row.second->gone)out.push_back(row.second);}
#include "CryThunderRoute.inc"
#include "StormheimCryThunder.inc"
void Move(Unit& u,Position const& pos){static_cast<Position&>(u)=pos;}
struct Run {
 Player p;Creature source;player_stormheim_cry_thunder script;std::vector<std::unique_ptr<Creature>> actors;std::vector<std::unique_ptr<SmartAI>> ais;
 Run(unsigned id):p(id),source(p){Move(p,StormheimCryThunder::Source);Move(source,p);source.entry=91767;source.dbGuid=388111;p.summon=[&](uint32 entry,Position const& pos,int vehicle)->Creature*{auto c=std::make_unique<Creature>(p);c->entry=entry;Move(*c,pos);c->kit.info.ID=vehicle;auto result=c.get();std::unique_ptr<SmartAI> ai;if(entry==92302)ai=std::make_unique<SmartAI>(result);else ai=std::make_unique<npc_stormheim_cry_thunder_target>(result);ais.push_back(std::move(ai));actors.push_back(std::move(c));return result;};}
 ~Run(){StormheimCryThunder::rides.erase(p.guid);for(auto& c:actors)world.erase(c->guid);world.erase(source.guid);}
 Creature* Dragon(){return StormheimCryThunder::Dragon(&p,StormheimCryThunder::rides.at(p.guid));}
 void Start(){spell_stormheim_cry_thunder_board spell;spell.caster=&p;spell.original=&source;assert(spell.Check()==SPELL_CAST_OK);spell.Summon(0);assert(spell.suppressed);spell.Complete();assert(StormheimCryThunder::rides.count(p.guid));}
 void Board(){Start();script.OnUpdate(&p,1);assert(!p.counters[91767]);auto c=Dragon();p.vehicle=c;c->kit.passenger=&p;p.mover=c;p.view=c;p.spline.final=false;script.OnUpdate(&p,1);assert(!p.counters[91767]);p.spline.final=true;script.OnUpdate(&p,1);assert(p.counters[91767]==1);}
 void Kill(size_t slot=0,Unit* killer=nullptr){auto& r=StormheimCryThunder::rides.at(p.guid);auto target=ObjectAccessor::GetCreature(p,r.targets[slot].guid);assert(target&&target->IsAlive());target->alive=false;target->ai->JustDied(killer?killer:Dragon());}
 void Kills(unsigned n){for(unsigned k=0;k<n;++k){auto& r=StormheimCryThunder::rides.at(p.guid);size_t slot=k%StormheimCryThunder::SeedCount;if(!ObjectAccessor::GetCreature(p,r.targets[slot].guid))script.OnUpdate(&p,8000);Kill(slot);}}
 void End(){auto c=Dragon();Move(*c,{2550,2450,400,0});Move(p,*c);spell_stormheim_cry_thunder_end end;end.caster=c;assert(end.Check()==SPELL_CAST_OK);end.Complete();assert(StormheimCryThunder::rides.at(p.guid).phase==StormheimCryThunder::Phase::Returning);}
 void Arrive(){auto c=Dragon();Move(*c,c->motion.destination);Move(p,*c);c->spline.final=true;script.OnUpdate(&p,1);}
 void Land(){assert(!p.vehicle);Move(p,p.exitDest);p.spline.final=true;script.OnUpdate(&p,1);}
};
int main(){
 for(auto faction:{469,67}){Run r(faction);r.Board();auto c=r.Dragon();auto& ride=StormheimCryThunder::rides.at(r.p.guid);assert(ride.phase==StormheimCryThunder::Phase::Assault&&r.actors.size()==StormheimCryThunder::SeedCount+1&&!r.p.counters[92360]);spell_stormheim_cry_thunder_end end;end.caster=c;Move(*c,{2550,2450,400,0});Move(r.p,*c);assert(end.Check()==SPELL_FAILED_BAD_TARGETS);
 r.Kill();assert(r.p.counters[92360]==1);auto target=r.actors[1].get();target->ai->JustDied(c);assert(r.p.counters[92360]==1);r.script.OnUpdate(&r.p,8000);r.Kills(149);assert(r.p.counters[92360]==150&&!r.p.counters[91768]&&r.p.quest.Status==QUEST_STATUS_INCOMPLETE);r.End();for(int i=0;i<4;++i){auto c=r.Dragon();auto node=StormheimCryThunder::rides.at(r.p.guid).node;r.script.OnUpdate(&r.p,1);assert(StormheimCryThunder::rides.at(r.p.guid).node==node);r.Arrive();}assert(!r.p.vehicle&&r.p.mover==&r.p&&!r.p.counters[91768]);r.script.OnUpdate(&r.p,100);assert(!r.p.counters[91768]);r.Land();assert(r.p.counters[91768]==1&&r.p.quest.Status==QUEST_STATUS_COMPLETE&&!StormheimCryThunder::rides.count(r.p.guid));}
 {Run r(3),other(4);r.Board();other.Board();auto& ride=StormheimCryThunder::rides.at(r.p.guid);auto own=ObjectAccessor::GetCreature(r.p,ride.targets[0].guid);Creature pet(r.p);own->alive=false;own->ai->JustDied(&pet);assert(r.p.counters[92360]==1&&!other.p.counters[92360]);auto foreign=ObjectAccessor::GetCreature(r.p,ride.targets[1].guid);foreign->alive=false;foreign->ai->JustDied(other.Dragon());assert(r.p.counters[92360]==1);world.erase(pet.guid);}
 for(int why=0;why<6;++why){Run r(10+why);r.Board();r.Kill();if(why==0)r.p.Reaccept();if(why==1)r.p.alive=false;if(why==2)r.p.phase=false;if(why==3)r.p.visible=false;if(why==4)r.Dragon()->alive=false;if(why==5)r.p.ExitVehicle();r.script.OnUpdate(&r.p,1);assert(!r.p.vehicle&&r.p.mover==&r.p&&!r.p.counters[91768]);r.Land();assert(!StormheimCryThunder::rides.count(r.p.guid)&&r.p.GetExactDist(&StormheimCryThunder::Source)<0.01f);}
 {Run r(20);r.Start();r.script.OnUpdate(&r.p,10000);r.script.OnUpdate(&r.p,1);assert(!StormheimCryThunder::rides.count(r.p.guid)&&!r.p.counters[91767]);}
 {Run r(21);r.Board();r.Kill();r.script.OnLogout(&r.p);assert(!r.p.vehicle&&r.p.mover==&r.p&&r.p.saves==1&&!StormheimCryThunder::rides.count(r.p.guid)&&r.p.savedPos.GetExactDist(&StormheimCryThunder::Source)<0.01f&&r.p.counters[92360]==1);}
 {Run r(22);r.Board();r.p.map=1;r.script.OnMapChanged(&r.p);assert(!r.p.vehicle&&r.p.mover==&r.p&&!r.p.teleports&&!r.p.counters[91768]);}
 {Run r(23);r.Board();auto c=r.Dragon();auto& ride=StormheimCryThunder::rides.at(r.p.guid);Creature unrelated(r.p);std::list<WorldObject*> targets{&unrelated};spell_stormheim_cry_thunder_attack attack;attack.caster=c;attack.m_scriptSpellId=183058;auto target=ObjectAccessor::GetCreature(r.p,ride.targets[0].guid);targets.push_back(target);attack.Filter(targets);assert(targets.size()==1&&targets.front()==target);Move(*c,target->GetPositionX()==0?StormheimCryThunder::Source:static_cast<Position&>(*target));attack.m_scriptSpellId=183042;targets={target,&unrelated};attack.Filter(targets);assert(targets.size()==1&&targets.front()==target);c->x+=151;targets={target};attack.Filter(targets);assert(targets.empty());world.erase(unrelated.guid);}
 {Run r(24);r.Board();Creature storm(r.p);storm.entry=92342;Move(storm,StormheimCryThunder::Seeds[0].pos);npc_stormheim_cry_thunder_tempest ai(&storm);ai.IsSummonedBy(r.Dragon());assert(storm.casts.size()==1&&storm.casts[0].first==183073);ai.UpdateAI(999);assert(storm.casts.size()==1);ai.UpdateAI(1);assert(storm.casts.size()==2&&storm.casts[1].first==183072);spell_stormheim_cry_thunder_tempest spell;spell.caster=&storm;WorldObject* selected=&storm;spell.Select(selected);assert(selected&&selected!=&storm);r.p.Reaccept();ai.UpdateAI(1000);assert(storm.gone);world.erase(storm.guid);}
 {Run r(25);r.Board();r.p.counters[92360]=150;auto c=r.Dragon();Move(*c,{2550,2450,400,0});r.p.geometry.los=false;spell_stormheim_cry_thunder_end spell;spell.caster=c;assert(spell.Check()==SPELL_FAILED_BAD_TARGETS);}
 {Run r(26);r.p.quest.Status=QUEST_STATUS_COMPLETE;r.script.OnLogin(&r.p);assert(r.p.quest.Status==QUEST_STATUS_INCOMPLETE);r.p.quest.Status=QUEST_STATUS_COMPLETE;r.p.counters={{91767,1},{92360,150},{91768,1}};r.script.OnLogin(&r.p);assert(r.p.quest.Status==QUEST_STATUS_COMPLETE);}
 for(int why=0;why<9;++why){Run r(30+why);if(why==0)r.p.alive=false;if(why==1)r.p.combat=true;if(why==2)r.p.transport=true;if(why==3)r.p.movement=MOVEMENTFLAG_FALLING;if(why==4)r.p.geometry.offset=3;if(why==5)r.p.spline.final=false;if(why==6)r.p.x+=20;if(why==7)r.source.dbGuid=42;if(why==8)r.p.quest.Status=QUEST_STATUS_NONE;assert(!StormheimCryThunder::CanStart(&r.p,&r.source));}
 {spell_stormheim_cry_thunder_board b;b.Register();spell_stormheim_cry_thunder_attack a;a.m_scriptSpellId=183065;a.Register();spell_stormheim_cry_thunder_end e;e.Register();spell_stormheim_cry_thunder_tempest t;t.Register();assert(a.OnObjectAreaTargetSelect.values.size()==2&&e.OnEffectLaunch.values.size()==1&&e.OnEffectLaunchTarget.values.size()==1&&t.OnObjectTargetSelect.values.size()==2);}
 {Run r(50);Creature original(r.p);world.erase(original.guid);original.guid={unsigned(StormheimCryThunder::Seeds[0].source)};world[original.guid]=&original;r.Board();assert(original.ShouldHideFor(r.p.guid)&&r.source.ShouldHideFor(r.p.guid));r.p.map=1;r.script.OnMapChanged(&r.p);assert(!original.ShouldHideFor(r.p.guid)&&!r.source.ShouldHideFor(r.p.guid));world.erase(original.guid);}
 {Run r(51);r.Board();auto c=r.Dragon();assert(c->m_templateSpells[0]==183058&&c->m_templateSpells[1]==183065&&c->m_templateSpells[2]==183042&&c->m_templateSpells[3]==183471);r.p.counters[92360]=150;r.End();for(int i=0;i<4;++i)r.Arrive();r.script.OnUpdate(&r.p,10000);assert(StormheimCryThunder::rides.at(r.p.guid).phase==StormheimCryThunder::Phase::Recovering&&!r.p.counters[91768]);r.p.spline.final=true;Move(r.p,StormheimCryThunder::Landing);r.script.OnUpdate(&r.p,1);assert(r.p.counters[91768]==1&&!StormheimCryThunder::rides.count(r.p.guid));}
 {Run r(52);r.Board();r.p.counters[92360]=150;r.End();r.Dragon()->spline.final=false;r.script.OnUpdate(&r.p,60000);assert(!r.p.vehicle&&!r.p.counters[91768]);r.Land();assert(!StormheimCryThunder::rides.count(r.p.guid)&&r.p.GetExactDist(&StormheimCryThunder::Source)<0.01f);}
 {Run r(53);spell_stormheim_cry_thunder_board spell;spell.caster=&r.p;spell.original=&r.source;assert(spell.Check()==SPELL_CAST_OK);r.p.Reaccept();spell.Summon(0);spell.Complete();assert(!StormheimCryThunder::rides.count(r.p.guid));}
 {Run r(54);r.source.dbGuid=99;spell_stormheim_cry_thunder_board spell;spell.caster=&r.p;spell.original=&r.source;assert(spell.Check()==SPELL_CAST_OK);spell.Summon(0);spell.Complete();assert(!spell.suppressed&&!StormheimCryThunder::rides.count(r.p.guid));}
 assert(StormheimCryThunder::rides.empty());std::cout<<"PASS: production Cry Thunder click/boarding, real owned target deaths150 cap, vehicle/pet owner isolation, native ability target filters and storm ticks, return/exit gating, cancellation/retry/logout/transfer, legacy state and startup registration. Engine movement/damage/network remain fixtures.\n";
}
