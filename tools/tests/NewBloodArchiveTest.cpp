#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t; using uint8=uint8_t;
constexpr int QUEST_STATUS_INCOMPLETE=3, REACT_PASSIVE=0;
enum SpellCastResult { SPELL_CAST_OK, SPELL_FAILED_BAD_TARGETS };
#define TC_LOG_INFO(...) ((void)0)
struct ObjectGuid { int id=0; bool IsEmpty()const{return !id;} };
struct Unit { uint32 entry=0; ObjectGuid guid; uint32 GetEntry(){return entry;} ObjectGuid GetGUID(){return guid;} };
struct Player { int quest=3; std::vector<uint32> spells; int GetQuestStatus(int){return quest;} SpellCastResult CastSpell(Player*,uint32 s,bool){spells.push_back(s);return SPELL_CAST_OK;} };
struct Map { std::vector<Player*> players; template<class F> void ApplyOnEveryPlayer(F f){for(auto p:players)f(p);} };
struct Scenario { uint32 id=962,step=5; uint32 GetScenarioId(){return id;} uint32 GetCurrentStep(){return step;} };
Scenario scenario;
struct Manager { Scenario* GetScenario(uint32){return &scenario;} } manager;
auto sScenarioMgr=&manager;
struct ActorAI { int signals=0; void SetData(int,int){++signals;} };
struct Creature:Unit {
 uint32 mapId=1522; Map map; ActorAI ai; bool active=false, combat=true; int react=1,smashes=0; bool failSmash=false;
 uint32 GetMapId(){return mapId;} uint32 GetInstanceId(){return 1;}
 Map* GetMap(){return &map;} void setActive(bool b){active=b;}
 void CombatStop(bool){combat=false;} void SetReactState(int r){react=r;}
 ActorAI* AI(){return &ai;} Creature* FindNearestCreature(uint32,float,bool){return nullptr;}
 SpellCastResult CastSpell(float,float,float,uint32 s,bool){assert(s==199150);++smashes;return failSmash?SPELL_FAILED_BAD_TARGETS:SPELL_CAST_OK;}
};
std::map<int,Creature*> actors;
namespace ObjectAccessor { Creature* GetCreature(Creature&,ObjectGuid g){auto i=actors.find(g.id);return i==actors.end()?nullptr:i->second;} }
struct SmartAI {
 Creature* me; int delegated=0; explicit SmartAI(Creature* c):me(c){}
 virtual void IsSummonedBy(Unit*){} virtual void UpdateAI(uint32){++delegated;}
};
#include "NewBloodArchive.inc"
int main(){
 Creature boss,guide; guide.entry=99178;guide.guid.id=7;actors[7]=&guide;
 Player player,bystander;bystander.quest=0;boss.map.players={&player,&bystander};
 npc_new_blood_jagganoth ai(&boss);ai.IsSummonedBy(&guide);
 // First callback may precede the book credit. Wait; never run in stage 5.
 scenario.step=4;ai.UpdateAI(60000);assert(!ai.started && player.spells.empty());
 scenario.step=5;
 for(int i=0;i<60;++i)ai.UpdateAI(1000);
 assert((player.spells==std::vector<uint32>{199070,199071,199072,199073,199074}));
 assert(bystander.spells.empty() && boss.smashes==1 && guide.ai.signals==1);
 assert(guide.active && !guide.combat && guide.react==REACT_PASSIVE);
 for(int i=0;i<60;++i)ai.UpdateAI(1000);
 assert(boss.smashes==1 && guide.ai.signals==1); // no duplicate start/credit
 // Other scenario / ritual cannot accidentally start the archive scene.
 scenario.id=998;npc_new_blood_jagganoth other(&boss);other.IsSummonedBy(&guide);other.UpdateAI(60000);assert(!other.started);
 scenario.id=962;boss.mapId=1502;npc_new_blood_jagganoth ritual(&boss);ritual.IsSummonedBy(&guide);ritual.UpdateAI(60000);assert(!ritual.started);
 // Missing guide must not consume the final transition; retain its exact GUID.
 boss.mapId=1522;npc_new_blood_jagganoth retry(&boss);retry.IsSummonedBy(&guide);actors.clear();
 for(int i=0;i<60;++i)retry.UpdateAI(1000);
 assert(retry.action==7);actors[7]=&guide;retry.UpdateAI(1000);assert(retry.action==8 && guide.ai.signals==2);
 // Do not advance to flight after an unsuccessful smash cast.
 npc_new_blood_jagganoth blocked(&boss);blocked.IsSummonedBy(&guide);boss.failSmash=true;
 for(int i=0;i<60;++i)blocked.UpdateAI(1000);
 assert(blocked.action==5 && guide.ai.signals==2);boss.failSmash=false;
 for(int i=0;i<10;++i)blocked.UpdateAI(1000);
 assert(blocked.action==8 && guide.ai.signals==3);
 std::cout<<"PASS: production archive timeline, delayed stage, exact guide, combat release, once-only handoff and scenario isolation\n";
}
