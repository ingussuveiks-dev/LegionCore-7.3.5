#include <cassert>
#include <cstdint>
#include <iostream>
#include <list>
using uint32=uint32_t;
enum {AT_ACTION_MOMENT_ENTER=1,AT_ACTION_MOMENT_UPDATE_TARGET=4,AT_ACTION_TYPE_CAST_SPELL=0};
struct Guid {uint64_t low=10,high=20;bool operator!=(Guid const& b)const{return low!=b.low||high!=b.high;}};
struct Map {};
struct Unit {bool creature=true,world=true,deleting=false,alive=true;uint32 entry=105251;Map* map;Guid guid;bool IsCreature()const{return creature;}uint32 GetEntry()const{return entry;}bool IsInWorld()const{return world;}bool IsPreDelete()const{return deleting;}bool IsAlive()const{return alive;}Map* GetMap()const{return map;}Guid GetGUID()const{return guid;}};
struct AreaTrigger {uint32 mapid=1600,real=6547,custom=11160,spell=209132,instance=1;Map* map;Guid caster,target;uint32 GetMapId()const{return mapid;}Map* GetMap()const{return map;}uint32 GetRealEntry()const{return real;}uint32 GetCustomEntry()const{return custom;}uint32 GetSpellId()const{return spell;}Guid GetCasterGUID()const{return caster;}Guid GetTargetGuid()const{return target;}uint32 GetInstanceId()const{return instance;}};
struct AreaTriggerAction {uint32 id=1,actionType=0,spellId=46598,targetFlags=1056840,moment=5;int hitMaxCount=1,maxCharges=0;};
struct AreaTriggerInfo {uint32 updateDelay=1000;std::list<AreaTriggerAction> actions{AreaTriggerAction{}};};
struct Scenario {uint32 id=1066;uint32 GetScenarioId()const{return id;}} testScenario;
struct Manager {bool present=true;Scenario* GetScenario(uint32 id){assert(id==1);return present?&testScenario:nullptr;}} manager;
auto sScenarioMgr=&manager;
#include "SharasdalWaveRide.h"
int main(){Map first,second;Unit owner;owner.map=&first;AreaTrigger at;at.map=&first;AreaTriggerInfo info;unsigned cases=0;
 for(unsigned mode=0;mode<17;++mode){auto u=owner;auto a=at;manager.present=true;testScenario.id=1066;
  switch(mode){case 1:u.creature=false;break;case 2:u.entry=105298;break;case 3:u.world=false;break;case 4:u.deleting=true;break;case 5:u.alive=false;break;case 6:a.mapid=1607;break;case 7:u.map=&second;break;case 8:a.real=6548;break;case 9:a.custom=11161;break;case 10:a.spell=209133;break;case 11:a.caster.low++;break;case 12:a.caster.high++;break;case 13:a.target.low++;break;case 14:a.target.high++;break;case 15:manager.present=false;break;case 16:testScenario.id=1123;break;}
  assert(SharasdalWaveRide::Enabled(&a,&u,info)==(mode==0));++cases;
 }
 manager.present=true;testScenario.id=1066;assert(!SharasdalWaveRide::Owns(nullptr,&owner)&&!SharasdalWaveRide::Owns(&at,nullptr));cases+=2;
 for(unsigned mode=0;mode<11;++mode){auto data=info;auto& action=data.actions.front();switch(mode){case 1:action.id=2;break;case 2:action.actionType=1;break;case 3:action.spellId=46599;break;case 4:action.targetFlags=8;break;case 5:action.hitMaxCount=2;break;case 6:action.maxCharges=1;break;case 7:action.moment=1;break;case 8:data.updateDelay=999;break;case 9:data.actions.push_back(action);break;case 10:data.actions.clear();break;}assert(SharasdalWaveRide::Enabled(&at,&owner,data)==(mode==0));++cases;}
 assert(cases==30);std::cout<<"PASS wave ride scope30 ASAN:full native identity tuple+128bit caster/target,no missing/foreign scenario or actor,original data stays disabled,exact one action/budget/flags/period;unknown/custom/shared contexts refused\n";
}
