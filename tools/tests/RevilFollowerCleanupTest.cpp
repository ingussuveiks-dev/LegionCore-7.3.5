#include <cassert>
#include <cstdint>
#include <list>
#include <map>
#include <iostream>
using uint32=uint32_t;using uint8=uint8_t;using ObjectGuid=uint32;
using GuidList=std::list<ObjectGuid>;
constexpr uint32 UNIT_FIELD_CREATED_BY_SPELL=0,AURA_EFFECT_HANDLE_REAL=1;
struct Creature;struct AuraApplication;struct Unit {
 using ControlList=std::list<ObjectGuid>;ControlList m_Controlled;uint32 guid=1,created=0;bool world=true;
 virtual ~Unit()=default;virtual Creature* ToCreature(){return nullptr;}virtual Creature* ToTempSummon(){return nullptr;}
 bool IsInWorld(){return world;}uint32 GetUInt32Value(uint32){return created;}void RemoveAllMinionsByFilter(uint32,uint8);
 void RemoveGameObject(uint32 spell,bool){assert(spell==199625);}void CastSpell(Unit*,uint32,bool){assert(false);}
};
struct Map{std::map<ObjectGuid,Creature*> creatures;Creature* GetCreature(ObjectGuid g){return creatures.count(g)?creatures.at(g):nullptr;}}map;
struct ObjectAccessor{static Unit* GetUnit(Unit&,ObjectGuid);static Creature* GetCreature(Unit&,ObjectGuid g){return map.GetCreature(g);}};
struct Creature:Unit{
 uint32 entry=101286,owner=1,despawns=0;bool summoned=true;
 Creature* ToCreature()override{return this;}Creature* ToTempSummon()override{return this;}bool isSummon(){return summoned;}
 uint32 GetEntry(){return entry;}ObjectGuid GetSummonerGUID(){return owner;}
 void UnSummon(){if(world){world=false;++despawns;}}void DespawnOrUnsummon(uint32=0){UnSummon();}
};
Unit* ObjectAccessor::GetUnit(Unit&,ObjectGuid g){return map.GetCreature(g);}
struct Effect{uint32 TriggerSpell=199625;};struct Info{Effect effect;Effect const* GetEffect(uint32,uint32)const{return &effect;}};
struct AuraEffect{uint32 m_diffMode=0;Info info;Info const* GetSpellInfo()const{return &info;}uint32 GetEffIndex()const{return 0;}void HandleSummonController(AuraApplication const*,uint8,bool)const;};
struct AuraApplication{Unit* owner;Unit* GetTarget()const{return owner;}};
struct Player:Unit{
 bool controller=true;GuidList followers;
 Map* GetMap(){return &map;}ObjectGuid GetGUID(){return guid;}GuidList* GetSummonList(uint32 entry){assert(entry==101286);return &followers;}
 void RemoveAurasDueToSpell(uint32 id){assert(id==199626);if(controller){controller=false;AuraApplication app{this};AuraEffect effect;effect.HandleSummonController(&app,1,false);}}
};
// Native entry-filter branch references this member on Unit. Each fixture
// unit uses the shared per-entry summon index, matching the tested owner.
std::map<uint32,GuidList> tempSummonList;
#include "RevilNativeFilter.inc"
#include "RevilNativeController.inc"
#include "RevilRetire.inc"
int main(){
 for(bool controller:{false,true})for(bool indexed:{false,true}){
  Player p;p.controller=controller;Creature follower,primary,foreign,wrongSpell;
  follower.guid=10;follower.created=199625;primary.guid=11;primary.entry=1860;primary.created=697;
  foreign.guid=12;foreign.owner=2;foreign.created=199625;wrongSpell.guid=13;wrongSpell.created=99;
  map.creatures={{10,&follower},{11,&primary},{12,&foreign},{13,&wrongSpell}};
  p.m_Controlled={11};if(indexed)p.m_Controlled.push_back(10);
  p.followers={10,11,12,13,999}; // Include stale index, foreign actor and demon controls.
  DarkRidersRevil::RetireFollower(&p);
  assert(!p.controller&&!follower.world&&follower.despawns==1);
  assert(primary.world&&foreign.world&&wrongSpell.world&&!primary.despawns&&!foreign.despawns&&!wrongSpell.despawns);
  for(int i=0;i<10;++i)DarkRidersRevil::RetireFollower(&p);assert(follower.despawns==1);
 }
 std::cout<<"PASS: production controller removal + native minion spell filter + scoped fallback retire owned Revil with present/missing aura/control index; primary demon, foreign/stale/wrong-spell entries and repeated cleanup preserved\n";
}
