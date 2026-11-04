#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <list>
#include <memory>
#include <set>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
enum {SPELL_CAST_OK=0,SPELL_FAILED_NOT_HERE=1,EFFECT_0=0,SPELL_EFFECT_SEND_EVENT=61,CLASS_ROGUE=4,TYPEID_PLAYER=4,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,REACT_AGGRESSIVE=2};
enum class HighGuid {Conversation};
struct ObjectGuid {uint64 high=1,low=0;bool operator==(ObjectGuid const& o)const{return high==o.high&&low==o.low;}bool operator!=(ObjectGuid const& o)const{return !(*this==o);}bool IsEmpty()const{return !low;}void Clear(){high=1;low=0;}};
struct Progress{uint64 Counter=0;};struct Scenario {uint32 id=1123,stage=2;std::map<uint32,Progress> progress;uint32 GetScenarioId()const{return id;}auto& GetAchievementMgr(){return *this;}auto GetCriteriaProgressMap()const{return &progress;}bool CanUpdateCriteria(uint32 leaf)const{return stage==2&&progress.find(leaf)==progress.end();}};
struct Map {uint32 id=1620;Scenario scenario;};
struct Creature;struct WorldObject {
    virtual ~WorldObject()=default;
    Map* map=nullptr;ObjectGuid guid;uint32 entry=0,phase=1,phaseId=1;float x=-8822.94f,y=657.59f,z=100;uint32 area=8228;
    bool inWorld=true,preDelete=false,alive=true,expired=false,scope=true;
    auto GetGUID()const{return guid;}auto GetMap()const{return map;}uint32 GetMapId()const{return map?map->id:0;}uint32 GetInstanceId()const{return 1;}
    bool IsInWorld()const{return inWorld;}bool IsPreDelete()const{return preDelete;}bool IsAlive()const{return alive;}bool IsQuestObjectExpired()const{return expired;}
    bool InSamePhase(WorldObject const* o)const{return phase==o->phase;}bool InSamePhaseId(WorldObject const* o)const{return phaseId==o->phaseId;}
    bool CanTargetQuestObject(WorldObject const* o)const{return scope&&o->scope;}float GetVisibilityRange()const{return 170;}
    Creature* FindNearestCreature(uint32,float,bool);
    bool IsWithinDistInMap(WorldObject const* o,float range)const{return map==o->map&&std::abs(x-o->x)<=range;}
    float GetExactDist2d(WorldObject const* o)const{return std::abs(x-o->x);}float GetPositionZ()const{return z;}
    uint32 GetAreaId()const{return area;}float GetExactDist2d(float a,float b)const{return std::hypot(x-a,y-b);}
    bool InheritQuestObjectScope(WorldObject* source){return !source->expired&&source->scope;}
};
struct Creature;struct Player;struct Unit:WorldObject {uint32 kind=0;uint32 GetTypeId()const{return kind;}Player* ToPlayer();std::vector<uint32> spells;void CastSpell(Unit*,uint32 id){spells.push_back(id);}};
struct Player:Unit {
    bool contact=true,gm=false,pending=false,rewarded=false;uint32 klass=4,quest=42504,status=3,auras=0,casts=0;
    Player(){kind=4;}bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}bool HasPendingTeleport()const{return pending;}uint32 getClass()const{return klass;}
    bool GetQuestRewardStatus(uint32 id)const{return id==quest&&rewarded;}uint32 GetQuestStatus(uint32 id)const{return id==quest?status:0;}
    void AddAura(uint32,Player*){++auras;}void CastSpell(Player*,uint32 id){++casts;spells.push_back(id);}uint32 sounds=0;void PlayDirectSound(uint32 id,std::nullptr_t){assert(id==15003);++sounds;}
};
Player* Unit::ToPlayer(){return kind==4?static_cast<Player*>(this):nullptr;}
struct Motion {uint32 moves=0;void MovePoint(uint32,float,float,float){++moves;}void Clear(){}void MoveRandom(float){++moves;}};
struct Pose {float GetOrientation()const{return 0;}};
struct Creature:Unit {
    Unit* victim=nullptr;bool moving=false,canAttack=false;uint32 react=0,faces=0,baseUpdates=0,attacks=0,melees=0,talks=0;Motion motion;Player* nearby=nullptr;
    uint32 GetEntry()const{return entry;}Unit* getVictim()const{return victim;}bool canStartAttack(Unit*,bool){return canAttack;}
    bool isMoving()const{return moving;}void SetReactState(uint32 value){react=value;}void SetFacingToObject(Unit*){++faces;}void SetOrientation(float){}Pose GetHomePosition()const{return {};}
    Motion* GetMotionMaster(){return &motion;}void AddDelayedEvent(uint32,std::function<void()> callback){delayed.push_back(std::move(callback));}
    void SetWalk(bool){}
    Player* FindNearestPlayer(float){return nearby;}std::vector<std::function<void()>> delayed;
};
struct Registry {Scenario* scenario=nullptr;Scenario* GetScenario(uint32){return scenario;}} registry;auto sScenarioMgr=&registry;
struct ObjectAccessor {
    inline static std::map<uint32,Player*> players;inline static std::map<uint32,Creature*> creatures;
    static Player* GetPlayer(WorldObject const& source,ObjectGuid guid){auto at=players.find(uint32(guid.low));return at!=players.end()&&at->second->map==source.map?at->second:nullptr;}
    static Creature* GetCreature(WorldObject const& source,ObjectGuid guid){auto at=creatures.find(uint32(guid.low));return at!=creatures.end()&&at->second->map==source.map?at->second:nullptr;}
};
Creature* WorldObject::FindNearestCreature(uint32,float,bool){return nullptr;}
struct ScriptedAI {
    Creature* me;explicit ScriptedAI(Creature* actor):me(actor){}virtual ~ScriptedAI()=default;
    virtual void Reset(){}virtual void MoveInLineOfSight(Unit*){}virtual void UpdateAI(uint32){++me->baseUpdates;}virtual void SetGUID(ObjectGuid const&,int32=0){}
    bool UpdateVictim(){return me->victim;}void AttackStart(Unit* target){me->victim=target;++me->attacks;}void DoMeleeAttackIfReady(){++me->melees;}void Talk(uint32){++me->talks;}
};
struct Generator {uint64 Generate(){return 1;}};
struct ObjectManager {Generator generator;template<HighGuid>Generator* GetGenerator(){return &generator;}} manager;auto sObjectMgr=&manager;
uint32 randomValue=1;uint32 urand(uint32,uint32){return randomValue;}uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct Conversation:WorldObject {
    inline static std::set<Conversation*> allocations;inline static uint32 attempts=0,accepted=0,failEntry=0;
    inline static std::vector<ObjectGuid> recipients,bound;inline static std::function<void()> callback;
    Conversation(){inWorld=false;allocations.insert(this);}virtual ~Conversation(){allocations.erase(this);}
    virtual void AddToWorld(){inWorld=true;}
    virtual Creature* FindCreatureActor(uint32,uint32 identity,WorldObject* searcher){for(auto row:ObjectAccessor::creatures)if(row.second->entry==identity&&row.second->map==searcher->map)return row.second;return nullptr;}
    bool CreateConversation(uint64,uint32 id,Unit* recipient,std::nullptr_t,WorldObject& position) {
        assert(recipient==&position);++attempts;map=recipient->map;entry=id;if(failEntry==id)return false;
        Creature* actor=nullptr;if(id==2384){actor=FindCreatureActor(0,108057,recipient);if(!actor)return false;}
        if(callback)callback();AddToWorld();if(!inWorld)return false;
        ++accepted;recipients.push_back(recipient->guid);if(actor)bound.push_back(actor->guid);return true;
    }
    static void Clear(){callback={};while(!allocations.empty())delete *allocations.begin();attempts=accepted=failEntry=0;recipients.clear();bound.clear();warnings=0;}
};

using SpellCastResult=uint32;using SpellEffIndex=uint32;
#define PrepareSpellScript(name) public:
#define SpellCastFn(...) 0
#define SpellCheckCastFn(...) 0
#define SpellEffectFn(...) 0
struct Hook{void operator+=(int){}};
struct SpellScript{Unit* fixtureCaster=nullptr;bool prevented=false;Hook OnCheckCast,OnEffectHit,AfterCast;virtual ~SpellScript()=default;virtual void Register(){}Unit* GetCaster()const{return fixtureCaster;}void PreventHitDefaultEffect(uint32 index){assert(index==0);prevented=true;}};
void GetCreatureListWithEntryInGrid(std::list<Creature*>& actors,Unit* caster,uint32 id,float range){assert(range==30);for(auto row:ObjectAccessor::creatures)if(row.second->entry==id&&row.second->map==caster->map)actors.push_back(row.second);}
#include "SmokeAI.inc"
using Smoke=spell_kingslayers_smoke_bomb_SpellScript;
struct Fixture {inline static uint32 cases=0;Map map,other;Player rogue,visitor;Unit otherCaster;Smoke smoke;Fixture(){++cases;Conversation::Clear();ObjectAccessor::creatures.clear();registry.scenario=&map.scenario;rogue.map=visitor.map=&map;rogue.guid.low=1;visitor.guid.low=2;smoke.fixtureCaster=&rogue;}~Fixture(){Conversation::Clear();ObjectAccessor::creatures.clear();registry.scenario=nullptr;}void Receive(){map.scenario.progress[48533].Counter=1;map.scenario.stage=3;}};
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);
 for(uint32 mode=0;mode<19;++mode){Fixture f;switch(mode){case 0:f.rogue.gm=true;break;case 1:f.rogue.klass=1;break;case 2:f.rogue.status=0;break;case 3:f.rogue.rewarded=true;break;case 4:f.rogue.inWorld=false;break;case 5:f.rogue.preDelete=true;break;case 6:f.rogue.contact=false;break;case 7:f.rogue.pending=true;break;case 8:f.rogue.alive=false;break;case 9:f.rogue.expired=true;break;case 10:f.map.id=0;break;case 11:f.rogue.area=8225;break;case 12:f.rogue.x+=14;break;case 13:f.map.scenario.id=1;break;case 14:registry.scenario=nullptr;break;case 15:f.map.scenario.stage=1;break;case 16:f.Receive();break;case 17:f.smoke.fixtureCaster=nullptr;break;case 18:f.smoke.fixtureCaster=&f.otherCaster;break;}
  assert(f.smoke.CheckSmoke()==SPELL_FAILED_NOT_HERE);f.smoke.GuardSmokeEvent(0);assert(f.smoke.prevented&&!f.smoke.smokeAdmitted);f.smoke.HandleAfterCast();assert(f.rogue.spells.empty()&&!f.rogue.sounds&&!Conversation::attempts);
 }
 for(uint32 quest:{42504u,42627u})for(uint32 status:{3u,1u}){Fixture f;f.rogue.quest=quest;f.rogue.status=status;assert(f.smoke.CheckSmoke()==SPELL_CAST_OK);f.smoke.GuardSmokeEvent(0);assert(!f.smoke.prevented&&f.smoke.smokeAdmitted);f.Receive();f.smoke.HandleAfterCast();assert(f.rogue.spells==std::vector<uint32>({214683,214734})&&f.rogue.sounds==1&&Conversation::accepted==1&&!f.smoke.smokeAdmitted);f.smoke.HandleAfterCast();assert(f.rogue.spells.size()==2);}
 {Fixture f;f.smoke.GuardSmokeEvent(0);f.smoke.HandleAfterCast();assert(f.smoke.smokeAdmitted&&f.rogue.spells.empty()&&!Conversation::attempts);}
 {Fixture f;f.smoke.GuardSmokeEvent(0);f.Receive();f.smoke.fixtureCaster=&f.visitor;f.smoke.HandleAfterCast();assert(f.visitor.spells.empty()&&f.rogue.spells.empty());}
 {Fixture f;f.smoke.GuardSmokeEvent(0);f.Receive();f.smoke.GuardSmokeEvent(0);assert(f.smoke.prevented);f.smoke.HandleAfterCast();assert(f.rogue.spells.size()==2);}
 {Fixture f;f.smoke.GuardSmokeEvent(0);f.Receive();Conversation::failEntry=2412;f.smoke.HandleAfterCast();assert(f.rogue.spells.size()==2&&f.rogue.sounds==1&&!Conversation::accepted);}
 {Fixture f;std::vector<Creature> actors(10);uint32 i=0;for(uint32 id:{108159u,108155u,108178u,108158u,108177u,108161u,108179u,108157u,108160u,108107u}){auto& actor=actors[i];actor.map=&f.map;actor.entry=id;ObjectAccessor::creatures[++i]=&actor;}f.smoke.GuardSmokeEvent(0);f.Receive();f.smoke.HandleAfterCast();for(auto& actor:actors)assert(actor.spells==std::vector<uint32>{214304}&&actor.motion.moves==1);}
 std::cout<<"PASS "<<Fixture::cases<<" whole smoke SpellScript ASAN contexts: cast/effect admission, active quest variants, real-leaf receipt boundary, full caster identity, no optional replay, retained panic effects/failed presentation. Engine receipt and registration modeled; actual native casts checked separately\n";
}
