#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
using uint8=uint8_t; using uint32=uint32_t; using uint64=uint64_t; using int32=int32_t;
enum { CRITERIA_TYPE_SCRIPT_EVENT_2=92 };
static uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct ObjectGuid { uint64 low=0, high=0; bool IsEmpty()const{return !low&&!high;} void Clear(){low=high=0;} bool operator==(ObjectGuid o)const{return low==o.low&&high==o.high;} bool operator<(ObjectGuid o)const{return high==o.high?low<o.low:high<o.high;} };
struct Progress { uint64 Counter=0; };
struct Achievement {std::map<uint32,Progress> rows; auto const*GetCriteriaProgressMap(){return &rows;}};
struct Scenario { uint32 id=944,step=1; Achievement achievement; uint32 GetScenarioId(){return id;} uint32 GetCurrentStep(){return step;} Achievement&GetAchievementMgr(){return achievement;} } scenario;
struct Scenarios { bool missing=false; Scenario*GetScenario(uint32 id){return missing||id!=42?nullptr:&scenario;} } scenarios;
auto*sScenarioMgr=&scenarios;
struct Player; struct Creature; struct Map;
struct Unit { Map*map=nullptr; ObjectGuid guid; bool alive=true; bool IsAlive(){return alive;} Map*GetMap(){return map;} ObjectGuid GetGUID(){return guid;} };
struct Ref {Player*player; Player*getSource()const{return player;}};
struct Map {struct PlayerList:std::vector<Ref>{using std::vector<Ref>::vector;bool isEmpty()const{return empty();}};uint32 id=1511;bool unload=false;PlayerList players;std::map<ObjectGuid,Creature*>actors;uint32 GetId(){return id;}uint32 GetInstanceId(){return 42;}bool IsMapUnload(){return unload;}auto const&GetPlayers(){return players;}Creature*GetCreature(ObjectGuid id){auto p=actors.find(id);return p==actors.end()?nullptr:p->second;}} world;
struct AI {uint32 attacks=0;void AttackStart(Player*){++attacks;}};
struct Creature:Unit {uint32 entry=0;bool worlded=true,IsAIEnabled=true;struct AI ai;bool IsInWorld(){return worlded;}struct AI*AI(){return IsAIEnabled?&ai:nullptr;} };
static std::vector<std::unique_ptr<Creature>>storage;
struct Player:Unit {bool eligible=true,gm=false;uint32 reject=0,failSpawn=0,conversations=0;std::vector<uint32>events;std::vector<uint32>entries;
 bool isGameMaster(){return gm;}void*GetInstanceScript(){return nullptr;}
 Creature*SummonCreature(uint32 entry,float,float,float){entries.push_back(entry);if(failSpawn){--failSpawn;return nullptr;}auto c=std::make_unique<Creature>();c->map=&world;c->entry=entry;c->guid={uint64(storage.size()+1),0x9988776655};auto*p=c.get();world.actors[p->guid]=p;storage.push_back(std::move(c));return p;}
 void UpdateAchievementCriteria(uint32 type,uint32 id){assert(type==92&&(id==47289||id==46804));events.push_back(id);if(reject==id)return;auto&value=scenario.achievement.rows[id==47289?29100:29055].Counter;++value;if(scenario.achievement.rows[29100].Counter>=3&&scenario.achievement.rows[29055].Counter)scenario.step=2;}
 void CreateConversation(uint32 id){assert(id==837);++conversations;}
};
namespace WarswordsArrival {bool IsParticipant(Player*p,uint32,void*){return p->eligible;}}
int32 irand(int32 min,int32 max){assert(min==-max);return 0;}
#include "Waves.inc"
struct Legacy {Map*instance=&world;
#include "Legacy.inc"
};
struct Run {Player player,other;WarswordsWaves::State state{&world};Run(){world=Map{};scenario=Scenario{};scenarios.missing=false;storage.clear();warnings=0;player.map=other.map=&world;world.players={{&player}};}
 void Start(){state.Start();}void Kill(uint32 slot){auto*c=world.GetCreature(state.actors[slot]);assert(c);c->alive=false;state.Death(c);}void Wave(){for(uint32 s=0;s<3;++s)Kill(s);} };
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);std::cout<<std::unitbuf;uint32 cases=0;
 {Run r;world.players.push_back({&r.other});Legacy old;old.DoSummonAdds(0);assert(storage.size()==6);std::cout<<"COUNTERFACTUAL pinned d473ec251 actual DoSummonAdds creates six actors for two players; repaired shared instance creates three.\n";++cases;}
 for(uint32 party:{1u,2u,5u}){Run r;for(uint32 i=1;i<party;++i)world.players.push_back({&r.other});r.Start();r.Start();assert(storage.size()==3);for(uint32 w=0;w<3;++w){assert(storage.size()==3*(w+1));auto old=r.state.actors;r.Kill(2);r.state.Death(world.GetCreature(old[2]));assert(r.player.events.size()==w);r.Kill(0);assert(r.player.events.size()==w);r.Kill(1);assert(r.state.wave==w+1);r.state.Death(world.GetCreature(old[1]));assert(r.player.events.size()==w+1);}assert(storage.size()==10&&storage.back()->entry==97649);auto*c=world.GetCreature(r.state.leader);c->alive=false;r.state.Death(c);r.state.Death(c);assert(scenario.step==2&&!r.state.active&&r.player.conversations==1&&r.player.events==std::vector<uint32>({47289,47289,47289,46804}));assert(r.player.entries==std::vector<uint32>({97992,98069,98069,97992,98069,98069,97994,98069,98069,97649}));++cases;}
 for(uint32 kind=0;kind<5;++kind){Run r;r.Start();Creature foreign;foreign.map=&world;foreign.alive=false;foreign.guid=r.state.actors[0];if(kind==0)++foreign.guid.high;if(kind==1)++foreign.guid.low;if(kind==2)foreign.alive=true;if(kind==3){static Map other;foreign.map=&other;}if(kind==4)foreign.guid={};r.state.Death(&foreign);assert(!r.state.dead[0]&&r.player.events.empty());++cases;}
 {Run r;r.player.failSpawn=1;r.Start();assert(storage.size()==2&&warnings==1);r.state.Update(999);assert(storage.size()==2);r.state.Update(1);assert(storage.size()==3&&r.player.entries.size()==4);r.Wave();assert(r.state.wave==1&&scenario.achievement.rows[29100].Counter==1);++cases;}
 {Run r;r.Start();auto old=r.state.actors[0];world.actors.erase(old);r.state.Update(1000);assert(storage.size()==4&&!(r.state.actors[0]==old)&&r.player.events.empty());auto*c=storage.front().get();c->alive=false;r.state.Death(c);assert(!r.state.dead[0]);++cases;}
 for(uint32 event:{47289u,46804u}){Run r;r.Start();if(event==46804){r.Wave();r.Wave();r.Wave();}r.player.reject=event;if(event==47289)r.Wave();else{auto*c=world.GetCreature(r.state.leader);c->alive=false;r.state.Death(c);}auto size=storage.size();auto calls=r.player.events.size();r.state.Update(999);assert(storage.size()==size&&r.player.events.size()==calls);r.player.reject=0;r.state.Update(1);assert(r.player.events.size()==calls+1);if(event==47289)assert(r.state.wave==1);else assert(scenario.step==2&&!r.state.active&&r.player.conversations==1);++cases;}
 for(uint32 bad=0;bad<7;++bad){Run r;if(bad==0)world.id=1;if(bad==1)world.unload=true;if(bad==2)scenario.id=909;if(bad==3)scenario.step=0;if(bad==4)scenarios.missing=true;if(bad==5)r.player.eligible=false;if(bad==6)r.player.gm=true;r.Start();r.state.Update(1000);assert(storage.empty()&&r.player.events.empty());++cases;}
 {Run r;r.player.gm=true;world.players.push_back({&r.other});r.Start();assert(r.player.entries.empty()&&r.other.entries.size()==3);r.Wave();assert(r.player.events.empty()&&r.other.events==std::vector<uint32>{47289});++cases;}
 {Run r;r.Start();r.Kill(0);r.player.eligible=false;r.Kill(1);r.Kill(2);assert(r.state.wave==0&&r.player.events.empty());r.player.eligible=true;r.state.Update(1000);assert(r.state.wave==1);++cases;}
 {Run r;scenario.achievement.rows[29100].Counter=2;r.Start();assert(r.state.wave==2&&storage.front()->entry==97994);r.Wave();assert(r.state.wave==3&&storage.back()->entry==97649);++cases;}
 {Run r;r.Start();auto ids=r.state.actors;r.state.Reset();for(auto id:ids){auto*c=world.GetCreature(id);c->alive=false;r.state.Death(c);}assert(r.player.events.empty());++cases;}
 std::cout<<"PASS "<<cases<<" ASAN actual Fury instance wave state: full GUID/death membership, one shared wave, three native waves then real leader, partial summon/removal retry, native criterion ACK/retry, duplicate/stale/alive/foreign rejection, participant/GM/group/re-entry guards; killer-independent death receipt. Map/spawn/criteria services controlled; actual client combat remains separate.\n";
}
