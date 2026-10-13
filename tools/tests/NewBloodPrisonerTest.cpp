// Compile the production AI against a small map/movement double. In particular,
// a guard death does NOT dirty the prisoner's NPC flags in this fixture.
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
using uint32 = uint32_t;
using uint64 = uint64_t;
constexpr int UNIT_FIELD_NPC_FLAGS=1, UNIT_NPC_FLAG_SPELLCLICK=0x1000000;
constexpr int REACT_PASSIVE=0, UNIT_STATE_ROOT=1, MOVEMENTFLAG_FALLING=1;
constexpr int EFFECT_MOTION_TYPE=17, TEMPSUMMON_TIMED_DESPAWN=1;
constexpr int QUEST_STATUS_INCOMPLETE=3;
constexpr float INVALID_HEIGHT=-100000;
struct ObjectGuid { int id=0; void Clear(){id=0;} };
struct SpellInfo { uint32 Id; };
struct Position {
 float x=0,y=0,z=3,o=0;
 float GetPositionX()const{return x;} float GetPositionY()const{return y;}
 float GetPositionZ()const{return z;} float GetOrientation()const{return o;}
};
struct Player; struct Creature; struct TempSummon;
std::map<int,Creature*> creatures;
Player* currentPlayer=nullptr;
struct Unit { virtual ~Unit()=default; virtual Player* ToPlayer(){return nullptr;} };
struct CriteriaTree { uint32 id; };
struct Achievements {
 std::set<uint32> done;
 bool IsCompletedCriteriaTree(CriteriaTree const* tree){return done.count(tree->id)!=0;}
};
struct AchievementStore { CriteriaTree tree{}; CriteriaTree const* GetCriteriaTree(uint32 id){tree.id=id;return &tree;} } achievementStore;
auto sAchievementMgr=&achievementStore;
struct Scenario {
 uint32 id=962,step=2; Achievements achievements;
 uint32 GetScenarioId(){return id;} uint32 GetCurrentStep(){return step;}
 Achievements& GetAchievementMgr(){return achievements;}
} scenario;
struct ScenarioManager { Scenario* GetScenario(uint32 instance){return instance==10?&scenario:nullptr;} } scenarioManager;
auto sScenarioMgr=&scenarioManager;
struct SpellManager { SpellInfo spell{195039}; SpellInfo const* GetSpellInfo(uint32){return &spell;} } spellManager;
auto sSpellMgr=&spellManager;
struct Motion { bool falling=false; void MoveFall(uint32){falling=true;} void Clear(){falling=false;} };
struct Creature : Unit,Position {
 uint32 entry=98775,map=1522,instance=10; uint64 spawn=1;
 ObjectGuid guid{1}; uint32 flags=UNIT_NPC_FLAG_SPELLCLICK,updates=0,forwarded=0;
 bool rooted=false,gravity=false,despawned=false,guard=false,scavenger=false;
 std::set<uint32> auras; Motion motion; Position home;
 uint32 GetEntry()const{return entry;} uint32 GetMapId()const{return map;}
 uint32 GetInstanceId()const{return instance;} uint64 GetDBTableGUIDLow()const{return spawn;}
 ObjectGuid GetGUID()const{return guid;}
 Creature* FindNearestCreature(uint32 e,float,bool){return (e==98878?guard:scavenger)?this:nullptr;}
 void DespawnOrUnsummon(){despawned=true;creatures.erase(guid.id);}
 void RemoveAurasDueToSpell(uint32 s){auras.erase(s);}
 void SetReactState(int){} void SetControlled(bool v,int){rooted=v;}
 void SetDisableGravity(bool v){gravity=v;}
 void CastSpell(Creature* target,uint32 spell,bool){target->auras.insert(spell);}
 void AddAura(uint32 spell,Creature* target){target->auras.insert(spell);}
 bool HasAura(uint32 spell,ObjectGuid){return auras.count(spell)!=0;}
 void RemoveFlag(int,uint32 f){if(flags&f){flags&=~f;++updates;}}
 void SetFlag(int,uint32 f){if(!(flags&f)){flags|=f;++updates;}}
 void AddUnitMovementFlag(int){} void RemoveUnitMovementFlag(int){}
 Motion* GetMotionMaster(){return &motion;}
 Creature* SummonCreature(uint32,float,float,float,float,int,uint32);
 virtual TempSummon* ToTempSummon(){return nullptr;}
 float GetHeight(float,float,float,bool,float){return 0;}
 Position const& GetHomePosition(){return home;}
 void NearTeleportTo(float a,float b,float c,float d){x=a;y=b;z=c;o=d;}
};
struct TempSummon : Creature {
 uint32 timer=5000;
 TempSummon* ToTempSummon()override{return this;}
 uint32 GetTimer(){return timer;} void AddDuration(uint32 n){timer+=n;}
};
Creature* Creature::SummonCreature(uint32 e,float a,float b,float c,float d,int,uint32){
 static int next=100;
 auto summon=new TempSummon; summon->entry=e;summon->x=a;summon->y=b;summon->z=c;summon->o=d;
 summon->guid.id=++next; creatures[summon->guid.id]=summon; return summon;
}
struct Player : Unit {
 ObjectGuid guid{2};uint32 instance=10;bool connected=true,alive=true,near=true;int quest=QUEST_STATUS_INCOMPLETE;
 Player* ToPlayer()override{return this;} bool IsAlive(){return alive;}
 int GetQuestStatus(uint32){return quest;} uint32 GetInstanceId(){return instance;}
 bool IsWithinDistInMap(Creature*,float){return near;} ObjectGuid GetGUID(){return guid;}
};
namespace ObjectAccessor {
 Creature* GetCreature(Creature&,ObjectGuid g){auto i=creatures.find(g.id);return i==creatures.end()?nullptr:i->second;}
 Player* GetPlayer(Creature&,ObjectGuid g){return currentPlayer && currentPlayer->connected && currentPlayer->guid.id==g.id?currentPlayer:nullptr;}
}
struct SmartAI {
 Creature* me;explicit SmartAI(Creature* c):me(c){}
 virtual void SpellHit(Unit*,SpellInfo const*){++me->forwarded;}
 virtual void MovementInform(uint32,uint32){} virtual void UpdateAI(uint32){}
 virtual void JustRespawned(){}
};
#include "NewBloodPrisoner.inc"

int main(){
 Player p;currentPlayer=&p;SpellInfo click{195039};
 // Missing click reproducer: killing the guard changes no unit fields.
 for(uint32 entry:{98775u,98783u,98793u}){
  Creature c;c.entry=entry;c.guard=true;npc_new_blood_prisoner ai(&c);
  ai.UpdateAI(250);
  assert(c.rooted && c.gravity && c.auras.count(192214));
  for(uint32 chain:{192195u,192196u,192198u,192200u})assert(c.auras.count(chain));
  assert(!(c.flags&UNIT_NPC_FLAG_SPELLCLICK));
  auto dirty=c.updates;ai.SpellHit(&p,&click);assert(c.forwarded==0);
  c.guard=false;assert(c.updates==dirty);ai.UpdateAI(250);
  assert(c.flags&UNIT_NPC_FLAG_SPELLCLICK);assert(c.updates>dirty);
  auto anchorIds=ai.anchors;ai.UpdateAI(250);
  for(size_t i=0;i<4;++i)assert(ai.anchors[i].id==anchorIds[i].id);
  ai.SpellHit(&p,&click);ai.SpellHit(&p,&click);
  assert(ai.state==npc_new_blood_prisoner::State::Falling && c.forwarded==0);
  assert(!c.rooted && !c.gravity && !(c.flags&UNIT_NPC_FLAG_SPELLCLICK));
  for(uint32 chain:{192195u,192196u,192198u,192200u,192214u})assert(!c.auras.count(chain));
  for(auto id:anchorIds)assert(!ObjectAccessor::GetCreature(c,id));
  ai.UpdateAI(250);assert(c.forwarded==0); // Still in the air.
  c.z=0;ai.MovementInform(EFFECT_MOTION_TYPE,195039);ai.UpdateAI(250);
  assert(c.forwarded==1 && ai.state==npc_new_blood_prisoner::State::Freed);
  ai.SpellHit(&p,&click);ai.UpdateAI(250);assert(c.forwarded==1);
 }
 // Stage gate refreshes even if prisoners were first observed in Stage 2.
 Creature c;npc_new_blood_prisoner ai(&c);scenario.step=1;ai.UpdateAI(250);
 assert(!(c.flags&UNIT_NPC_FLAG_SPELLCLICK));scenario.step=2;ai.UpdateAI(250);
 assert(c.flags&UNIT_NPC_FLAG_SPELLCLICK);
 // Reappearance of either guard blocks a queued cast; opposite-side approaches cannot bypass it.
 c.scavenger=true;ai.SpellHit(&p,&click);assert(ai.state==npc_new_blood_prisoner::State::Bound);
 c.scavenger=false;p.near=false;ai.SpellHit(&p,&click);assert(c.forwarded==0);p.near=true;
 // A disconnected rescuer gets no credit; captive returns to its elevated position.
 ai.SpellHit(&p,&click);p.connected=false;ai.UpdateAI(5000);
 assert(ai.state==npc_new_blood_prisoner::State::Bound && c.z==3 && !c.forwarded);
 p.connected=true;ai.UpdateAI(250);ai.SpellHit(&p,&click);p.quest=0;c.z=0;ai.UpdateAI(250);
 assert(ai.state==npc_new_blood_prisoner::State::Bound && !c.forwarded);p.quest=QUEST_STATUS_INCOMPLETE;
 // Persisted individual progress removes a reloaded prisoner without awarding it twice.
 scenario.achievements.done.insert(46021);ai.UpdateAI(250);assert(c.despawned && !c.forwarded);
 scenario.achievements.done.clear();
 // Archive summons and the other scenario retain their original SmartAI.
 Creature copy;copy.spawn=0;npc_new_blood_prisoner archive(&copy);archive.UpdateAI(250);
 archive.SpellHit(&p,&click);assert(!archive.initialized && copy.forwarded==1);
 Creature other;scenario.id=998;npc_new_blood_prisoner unrelated(&other);unrelated.UpdateAI(250);
 unrelated.SpellHit(&p,&click);assert(!unrelated.initialized && other.forwarded==1);
 std::cout<<"PASS: production prisoner AI, visibility transitions, four chains, landing, retry, duplicate and scope checks\n";
}
