#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>
using uint32=uint32_t;
class ObjectGuid{public:unsigned value=0;bool IsEmpty()const{return !value;}void Clear(){value=0;}bool operator==(ObjectGuid a)const{return value==a.value;}bool operator!=(ObjectGuid a)const{return value!=a.value;}bool operator<(ObjectGuid a)const{return value<a.value;}};
#include "CombatHeader.inc"
class CreatureAI{};
class InstanceScript{public:virtual ~InstanceScript()=default;};
class instance_shields_rest{public:class instance_shields_rest_InstanceMapScript:public InstanceScript{public:bool magnarGuardCreditSent=true,magnarGuardsDelivered=true;unsigned magnarGuardGeneration=1;MagnarCombat::State magnarCombat;};};
using Script=instance_shields_rest::instance_shields_rest_InstanceMapScript;
class InstanceMap{public:std::map<ObjectGuid,Creature*> actors;Creature* GetCreature(ObjectGuid id){auto i=actors.find(id);return i==actors.end()?nullptr:i->second;}};
class TempSummon;
class Creature{public:
 virtual ~Creature()=default;ObjectGuid id;unsigned entry=96034,db=361400,mapId=1495;bool world=true,alive=true,despawn=false,IsAIEnabled=true;InstanceMap* map=nullptr;InstanceScript* script=nullptr;CreatureAI* ai=nullptr;std::set<unsigned> phases{6999};unsigned removed=0;std::function<void()> onRemove;
 bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}bool IsDespawn()const{return despawn;}unsigned GetMapId()const{return mapId;}unsigned GetEntry()const{return entry;}unsigned GetDBTableGUIDLow()const{return db;}unsigned GetInstanceId()const{return 42;}InstanceMap*GetMap()const{return map;}InstanceScript*GetInstanceScript()const{return script;}CreatureAI*AI()const{return ai;}auto const&GetPhases()const{return phases;}ObjectGuid GetGUID()const{return id;}virtual TempSummon*ToTempSummon(){return nullptr;}
 void DespawnOrUnsummon(){++removed;world=false;map->actors.erase(id);auto callback=onRemove;if(callback)callback();}
};
class TempSummon:public Creature{public:ObjectGuid owner;TempSummon*ToTempSummon()override{return this;}ObjectGuid GetSummonerGUID()const{return owner;}};
class SummonList{public:std::vector<ObjectGuid> owned;void Summon(Creature*c){owned.push_back(c->GetGUID());}};
class Scenario{public:unsigned id=909,step=3;unsigned GetScenarioId()const{return id;}unsigned GetCurrentStep()const{return step;}};
class ScenarioMgr{public:Scenario scenario;bool exists=true;Scenario*GetScenario(unsigned id){assert(id==42);return exists?&scenario:nullptr;}} scenarioMgr;
ScenarioMgr*sScenarioMgr=&scenarioMgr;
#include "CombatBridge.inc"
struct Fixture{
 InstanceMap map;Script script;Creature boss;CreatureAI ai,other;InstanceScript foreign;std::array<ObjectGuid,12> roster;std::array<bool,12> deaths{};std::array<std::unique_ptr<TempSummon>,12> members;
 Fixture(){scenarioMgr={};boss.id={1};boss.map=&map;boss.script=&script;boss.ai=&ai;map.actors[boss.id]=&boss;MagnarCombat::Bind(&map,script.magnarCombat,&boss,1);for(unsigned i=0;i<12;++i){auto m=std::make_unique<TempSummon>();m->id={100+i};m->owner=boss.id;m->map=&map;m->entry=i>=10?94813:i==4||i==9?94707:97123;m->alive=i>=10;deaths[i]=i<10;roster[i]=m->id;map.actors[m->id]=m.get();members[i]=std::move(m);}}
 void Remember(){MagnarCombat::Remember(&boss,&ai,roster,deaths);}auto&State(){return script.magnarCombat;}
};
int main(){unsigned cases=0;
 for(unsigned fault=0;fault<17;++fault){Fixture f;switch(fault){case 0:f.boss.world=false;break;case 1:f.boss.alive=false;break;case 2:f.boss.despawn=true;break;case 3:f.boss.mapId=1;break;case 4:f.boss.entry=1;break;case 5:f.boss.db=1;break;case 6:f.boss.IsAIEnabled=false;break;case 7:f.boss.ai=&f.other;break;case 8:f.boss.phases.clear();break;case 9:f.boss.script=nullptr;break;case 10:f.boss.script=&f.foreign;break;case 11:f.script.magnarGuardCreditSent=false;break;case 12:f.script.magnarGuardsDelivered=false;break;case 13:f.State().actor={9};break;case 14:++f.script.magnarGuardGeneration;break;case 15:f.State().epoch=0;break;case 16:assert(!MagnarCombat::Find(nullptr,&f.ai));++cases;continue;}assert(!MagnarCombat::Find(&f.boss,&f.ai));f.Remember();assert(!f.State().earned);++cases;}
 for(unsigned fault=0;fault<10;++fault){Fixture f;switch(fault){case 0:f.deaths[0]=false;break;case 1:f.deaths[10]=true;f.members[10]->alive=false;break;case 2:f.roster[0]={};break;case 3:f.members[0]->owner={99};break;case 4:f.members[0]->entry=1;break;case 5:f.members[0]->alive=true;break;case 6:scenarioMgr.exists=false;break;case 7:scenarioMgr.scenario.id=1068;break;case 8:scenarioMgr.scenario.step=2;break;case 9:scenarioMgr.scenario.step=4;break;}f.Remember();assert(!f.State().earned);++cases;}
 {Fixture f;f.Remember();assert(f.State().earned&&f.State().roster==f.roster&&f.State().deaths==f.deaths);SummonList list;std::array<ObjectGuid,12> r{};std::array<bool,12>d{};assert(!MagnarCombat::Restore(&f.boss,&f.ai,list,r,d));scenarioMgr.scenario.step=4;assert(MagnarCombat::Restore(&f.boss,&f.ai,list,r,d)&&r==f.roster&&d==f.deaths&&list.owned.size()==12);auto saved=f.State();f.roster={};f.Remember();assert(f.State().roster==saved.roster);MagnarCombat::Bind(&f.map,f.State(),&f.boss,1);assert(f.State().earned);++cases;}
 for(unsigned fault=0;fault<5;++fault){Fixture f;f.Remember();scenarioMgr.scenario.step=4;switch(fault){case 0:f.State().earned=false;break;case 1:scenarioMgr.scenario.step=3;break;case 2:scenarioMgr.scenario.step=5;break;case 3:scenarioMgr.scenario.id=1082;break;case 4:scenarioMgr.exists=false;break;}SummonList list;std::array<ObjectGuid,12>r{};std::array<bool,12>d{};assert(!MagnarCombat::Restore(&f.boss,&f.ai,list,r,d)&&list.owned.empty());++cases;}
 {Fixture f;f.Remember();f.members[2]->owner={999};MagnarCombat::Reset(&f.map,f.State());assert(f.State().actor.IsEmpty()&&!f.State().earned);for(unsigned i=0;i<12;++i)assert(f.members[i]->removed==unsigned(i!=2));MagnarCombat::Reset(&f.map,f.State());assert(!f.members[2]->removed);++cases;}
 {Fixture f;f.Remember();auto id=f.members[0]->id;MagnarCombat::Remove(&f.map,f.State(),f.members[0].get());assert(f.State().roster[0].IsEmpty()&&f.State().earned);MagnarCombat::Remove(&f.map,f.State(),&f.boss);assert(!f.State().earned&&f.State().actor.IsEmpty()&&!f.members[0]->removed&&f.members[1]->removed==1);assert(f.map.GetCreature(id)==f.members[0].get());++cases;}
 {Fixture f;f.Remember();Creature next;next.id={2};next.map=&f.map;f.members[0]->onRemove=[&]{MagnarCombat::Bind(&f.map,f.State(),&next,2);};MagnarCombat::Reset(&f.map,f.State());assert(f.State().actor==next.id&&f.State().epoch==2&&!f.State().earned);for(auto const&m:f.members)assert(m->removed==1);++cases;}
 {Fixture f;f.Remember();Creature next;next.id={2};next.map=&f.map;MagnarCombat::Bind(&f.map,f.State(),&next,2);assert(f.State().actor==next.id&&f.State().epoch==2&&!f.State().earned);for(auto const&m:f.members)assert(m->removed==1);++cases;}
 std::cout<<"PASS actual Magnar combat state cases="<<cases<<": original actor/AI/map/phase/earned guard/epoch,10 owned death stamps,observed step4,real ownership-only adoption,scoped cleanup and reentrant bind; map/summon services controlled,full native lifecycle separate\n";
}
