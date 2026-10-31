#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <tuple>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
struct ObjectGuid{uint64 low=1,high=2;bool operator<(ObjectGuid const&o)const{return std::tie(high,low)<std::tie(o.high,o.low);}bool operator==(ObjectGuid const&o)const{return low==o.low&&high==o.high;}};
struct Map;struct Player;struct InstanceScript;
std::vector<unsigned> order;
struct Pet{std::set<uint32>auras;void RemoveAurasDueToSpell(uint32 s){auras.erase(s);}};
struct Player{
 ObjectGuid guid;Map*map=nullptr;Pet*pet=nullptr;std::set<uint32>auras;bool world=true,grid=true,deleted=false,predelete=false;uint32 phase=8,retired=0;
 ObjectGuid GetGUID()const{return guid;}uint32 GetCurrentZoneID()const{return 42;}InstanceScript*GetInstanceScript();
 Pet*GetPet(){return pet;}void RemoveAurasDueToSpell(uint32 s){assert(!deleted);auras.erase(s);}void RemoveAura(uint32 s){RemoveAurasDueToSpell(s);}
 void SetPhaseMask(uint32 value,bool update){assert(value==1&&update&&!deleted);phase=value;}
 void RemoveFromWorld(){assert(!deleted);order.push_back(4);world=false;}void UpdateObjectVisibility(bool value){assert(value&&!deleted);}
 bool IsInGrid()const{return grid;}void RemoveFromGrid(){assert(!deleted);grid=false;}void SetPreDelete(){assert(!deleted);predelete=true;}
};
struct Scenario{uint32 id=988;uint32 GetScenarioId()const{return id;}};
struct Battle{uint32 area=0,map=0;ObjectGuid last;void HandlePlayerLeaveArea(ObjectGuid p,uint32){last=p;++area;}void HandlePlayerLeaveMap(ObjectGuid p,uint32 id){assert(id==1191);last=p;++map;}};
struct InstanceScript{
 virtual~InstanceScript()=default;Map*instance=nullptr;Scenario scenario;bool hasScenario=true;std::set<ObjectGuid>spiritPlayers,boardedPlayers;std::map<ObjectGuid,uint32>arrivalTimers,_areas;Battle _battle;
 uint32 challenge=0;virtual void OnPlayerLeave(Player*){}void OnPlayerLeaveForScript(Player*){++challenge;order.push_back(2);}
 Scenario*Progress(){return hasScenario?&scenario:nullptr;}void DoRemoveAurasDueToSpellOnPlayers(uint32);
};
namespace DarkRidersRevil{void RetireFollower(Player*p){assert(p&&!p->deleted);++p->retired;p->RemoveAurasDueToSpell(199626);}}
struct Map{
 InstanceScript*data=nullptr;std::vector<Player*>players;uint32 id=1533,transports=0;bool callbackDeleted=false,callbackWorld=false;uint32 callbacks=0;
 uint32 GetId(){return id;}void SendRemoveTransports(Player*p){assert(!p->deleted);++transports;}
 void DeleteFromWorld(Player*p){assert(p->predelete);order.push_back(5);p->deleted=true;}
 template<class F>void ApplyOnEveryPlayer(F f){for(auto p:players)f(p);}void RemovePlayerFromMap(Player*,bool);
};
InstanceScript*Player::GetInstanceScript(){return map->data;}
struct Outdoor{void HandlePlayerLeaveMap(ObjectGuid,uint32){order.push_back(0);}}outdoor;auto*sOutdoorPvPMgr=&outdoor;
struct Scripts{void OnPlayerLeaveMap(Map*m,Player*p){++m->callbacks;m->callbackDeleted=p->deleted;m->callbackWorld=p->world;order.push_back(3);}}scripts;auto*sScriptMgr=&scripts;
#include "GroupAura.inc"
#include "Callbacks.inc"
#include "MapRemove.inc"
struct Proxy:InstanceScript{InstanceScript*effect=nullptr;uint32 calls=0;bool live=false;void OnPlayerLeave(Player*p)override{++calls;order.push_back(1);live=p->world&&!p->deleted;if(effect)effect->OnPlayerLeave(p);}};
struct Run{
 Map map;Player a,b;Pet ap,bp;Proxy proxy;
 Run(){a.guid={1,2};b.guid={1,3};a.map=b.map=&map;a.pet=&ap;b.pet=&bp;map.players={&a,&b};map.data=&proxy;proxy.instance=&map;order.clear();}
 void Seed(uint32 spell){for(auto p:{&a,&b})p->auras.insert(spell);ap.auras.insert(spell);bp.auras.insert(spell);}
 void Use(InstanceScript&effect){effect.instance=&map;proxy.effect=&effect;}
};
unsigned cases=0;
template<class T>void AuraCallback(std::vector<uint32>ids,bool raid=false){
 for(bool pet:{false,true}){Run r;T effect;r.Use(effect);if(!pet)r.a.pet=nullptr;for(auto id:ids)r.Seed(id);r.Seed(999999);effect.OnPlayerLeave(&r.a);
  for(auto id:ids){assert(!r.a.auras.count(id));
#ifdef BASELINE
   assert(bool(r.b.auras.count(id))==!raid);assert(bool(r.bp.auras.count(id))==!raid);
#else
   assert(r.b.auras.count(id)&&r.bp.auras.count(id));
#endif
   if(raid&&pet)assert(!r.ap.auras.count(id));}
  assert(r.a.auras.count(999999)&&r.b.auras.count(999999)&&r.ap.auras.count(999999)&&r.bp.auras.count(999999));++cases;
 }
}
int main(){
 for(uint32 map:{0u,1191u,1533u,1460u,1541u,1009u,1136u})for(bool destroy:{false,true})for(bool data:{false,true}){
  Run r;r.map.id=map;if(!data)r.map.data=nullptr;r.map.RemovePlayerFromMap(&r.a,destroy);
#ifdef BASELINE
  assert(r.proxy.calls==unsigned(data&&map==1191));assert(r.map.callbacks==unsigned(destroy));assert(r.map.callbackDeleted==destroy);
#else
  assert(r.proxy.calls==unsigned(data)&&(!data||r.proxy.live));assert(r.map.callbacks==1&&!r.map.callbackDeleted&&r.map.callbackWorld);
  auto world=std::find(order.begin(),order.end(),4u);auto script=std::find(order.begin(),order.end(),3u);assert(script<world);
  if(data)assert(std::find(order.begin(),order.end(),1u)<std::find(order.begin(),order.end(),2u)&&std::find(order.begin(),order.end(),2u)<script);
#endif
  assert(r.proxy.challenge==unsigned(data)&&!r.a.world&&!r.a.grid&&r.a.deleted==destroy&&r.a.predelete==destroy&&r.map.transports==unsigned(!destroy));
  assert(r.b.world&&r.b.grid&&!r.b.deleted);++cases;
 }
 for(uint32 scenario:{988u,1001u,1002u})for(bool tracked:{false,true})for(bool present:{false,true}){
  Run r;DarkRidersLeave effect;r.Use(effect);effect.scenario.id=scenario;effect.hasScenario=present;if(tracked)effect.spiritPlayers.insert(r.a.guid);effect.spiritPlayers.insert(r.b.guid);
  r.Seed(198654);r.Seed(199626);effect.OnPlayerLeave(&r.a);
  bool frost=present&&scenario==988;assert(bool(r.a.auras.count(198654))==!(tracked||frost));assert(r.a.retired==unsigned(frost));
  assert(r.b.auras.count(198654)&&r.b.auras.count(199626)&&!r.b.retired&&effect.spiritPlayers.count(r.b.guid));++cases;
 }
 {Run r;BrokenShoreLeave effect;r.Use(effect);effect.arrivalTimers={{r.a.guid,1},{r.b.guid,2}};effect.boardedPlayers={r.a.guid,r.b.guid};effect.OnPlayerLeave(&r.a);assert(!effect.arrivalTimers.count(r.a.guid)&&effect.arrivalTimers.count(r.b.guid)&&!effect.boardedPlayers.count(r.a.guid)&&effect.boardedPlayers.count(r.b.guid));++cases;}
 for(bool admitted:{false,true}){Run r;AshranLeave effect;r.Use(effect);if(admitted)effect._areas[r.a.guid]=1;effect._areas[r.b.guid]=2;effect.OnPlayerLeave(&r.a);assert(effect._battle.map==unsigned(admitted)&&effect._battle.area==unsigned(admitted)&&!effect._areas.count(r.a.guid)&&effect._areas.count(r.b.guid));++cases;}
 {Run r;GateLeave effect;effect.OnPlayerLeave(&r.a);assert(r.a.phase==1&&r.b.phase==8);++cases;}
 AuraCallback<HeartLeave>({126159},true);SiegeLeave siege;AuraCallback<SiegeLeave>(std::vector<uint32>(siege.removelist,siege.removelist+30),true);
 AuraCallback<HighmaulLeave>({HighmaulLeave::PlayChogallScene,HighmaulLeave::ChogallNight});
 AuraCallback<RagefireLeave>({RagefireLeave::SPELL_GOBLIN_ILLUSION});AuraCallback<DarkHeartLeave>({DarkHeartLeave::SPELL_GOBLIN_ILLUSION});
 AuraCallback<IcecrownLeave>({70923});AuraCallback<ArmyLeave>({ArmyLeave::SPELL_STARTED_OVERRIDE,ArmyLeave::SPELL_COUNT_MINIONS,ArmyLeave::SPELL_TELE_END});
 AuraCallback<TournamentLeave>({TournamentLeave::SPELL_BATTLE_PET_CHALLENGE});AuraCallback<SnowLeave>({SnowLeave::SPELL_BLIZZARD_WEATHER});
#ifdef BASELINE
 std::cout<<"EXPECTED_BROKEN "<<cases<<" pinned native Map caller/callback source cases: ordinary instance cleanup omission, transfer MapScript omission, post-delete callback publication and raid-wide aura/pet removal. Deletion/world/MapScript endpoints controlled; no old dangling pointer dereferenced.\n";
#else
 std::cout<<"PASS "<<cases<<" actual production Map caller and all13 callback bodies under ASAN: dispatch/context/order/remove flags, both full GUID owners, scenario/participant gates, departure auras/pet scope, Ashran exactly once and Broken Shore state. World/delete/script/aura/follower endpoints controlled; native integration and client separate.\n";
#endif
}
